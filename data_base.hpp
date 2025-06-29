#pragma once

#include <fmt/format.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <postgresql/libpq-fe.h>
#include <string>
#include <type_traits>
#include <vector>

namespace db::postgresql
{

namespace exceptions
{
struct DataBaseEx : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct ConnectionFail final : DataBaseEx {
    using DataBaseEx::DataBaseEx;
};
struct QueryFail final : DataBaseEx {
    using DataBaseEx::DataBaseEx;
};
struct NonBlock final : DataBaseEx {
    using DataBaseEx::DataBaseEx;
};
struct OutOfRange final : DataBaseEx {
    using DataBaseEx::DataBaseEx;
};
} // namespace exceptions

struct Backend {
    size_t      id = 0;
    std::string address;
    std::string region;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Backend, id, address, region);
};

struct User {
    size_t      id = 0;
    std::string login;
    std::string email;
    std::string password;
    Backend     backendId;
    std::string token;
    size_t      tokenExp = 0;
    size_t      status    = 0;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(User, id, login, email, password, backendId, token, tokenExp, status);
};

class IDataBase {
public:
    virtual ~IDataBase() = default;

    virtual User    getUserById(const std::string& id)       = 0;
    virtual User    getUserByToken(const std::string& token) = 0;
    virtual User    getUserByLogin(const std::string& login) = 0;
    virtual User    getUserByEmail(const std::string& email) = 0;
    virtual void    updateUser(const User& user)             = 0;
    virtual void    addUser(const User& user)                = 0;
    virtual Backend getBackendServerById(size_t id)          = 0;

protected:
    IDataBase() = default;
};

class DataBase final : public IDataBase {
public:
    explicit DataBase(std::string_view const& connection);
    ~DataBase() override;

    DataBase(DataBase const&)            = delete;
    DataBase(DataBase&&)                 = delete;
    DataBase& operator=(DataBase const&) = delete;
    DataBase& operator=(DataBase&&)      = delete;

    User    getUserById(const std::string& id) override;
    User    getUserByToken(const std::string& token) override;
    User    getUserByLogin(const std::string& login) override;
    User    getUserByEmail(const std::string& email) override;
    void    updateUser(const User& user) override;
    void    addUser(const User& user) override;
    Backend getBackendServerById(size_t id) override;

private:
    template<typename T>
    T getValue(std::unique_ptr<PGresult, void (*)(PGresult*)> const& answer, int const row, int const col) const {
        if (!answer) {
            if constexpr (std::is_same_v<T, std::string>) {
                return "";
            } else if constexpr (std::is_arithmetic_v<T>) {
                return 0;
            } else {
                return T{};
            }
        }

        if (row >= PQntuples(answer.get()) || col >= PQnfields(answer.get())) {
            throw exceptions::OutOfRange(::fmt::format("Index error: row {} or column {} out of range", row, col));
        }

        if (PQgetisnull(answer.get(), row, col)) {
            if constexpr (std::is_same_v<T, std::string>) {
                return "";
            } else if constexpr (std::is_arithmetic_v<T>) {
                return 0;
            } else {
                return T{};
            }
        }


        const char* value = PQgetvalue(answer.get(), row, col);

        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, size_t>) {
            return static_cast<T>(std::stoull(value));
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            return static_cast<T>(std::stod(value));
        } else {
            static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, int> || std::is_same_v<T, long> ||
                            std::is_same_v<T, size_t> || std::is_same_v<T, float> || std::is_same_v<T, double>,
                          "Unsupported type for getValue");
            return T();
        }
    }


    template<typename T>
    T getValue(std::unique_ptr<PGresult, void (*)(PGresult*)> const& answer,
               int const                                             row,
               std::string const&                                    col_name) const {
        if (!answer) {
            if constexpr (std::is_same_v<T, std::string>) {
                return "";
            } else if constexpr (std::is_arithmetic_v<T>) {
                return 0;
            } else {
                return T{};
            }
        }
        int const col = PQfnumber(answer.get(), col_name.c_str());
        if (col == -1) {
            throw exceptions::OutOfRange(::fmt::format("Column '{}' not found in query result", col_name));
        }
        return getValue<T>(answer, row, col);
    }

    template<typename T>
    T getValue(std::unique_ptr<PGresult, void (*)(PGresult*)> const& answer, std::string const& col) const {
        if (!answer) {
            if constexpr (std::is_same_v<T, std::string>) {
                return "";
            } else if constexpr (std::is_arithmetic_v<T>) {
                return 0;
            } else {
                return T{};
            }
        }
        return getValue<T>(answer, 0, col);
    }

    template<typename... Parameters>
    std::unique_ptr<PGresult, void (*)(PGresult*)> query(std::string_view const& query_with_placeholder,
                                                         Parameters&&... parameters) {
        std::lock_guard<std::mutex> lock(m_mutex_query);

        std::vector<std::string> param_strings;
        param_strings.reserve(sizeof...(parameters));

        auto add_param = [&param_strings](auto&& param) {
            using ParamType = std::decay_t<decltype(param)>;

            if constexpr (std::is_same_v<ParamType, std::string> || std::is_same_v<ParamType, std::string_view>) {
                param_strings.push_back(std::forward<decltype(param)>(param));
            } else if constexpr (std::is_arithmetic_v<ParamType>) {
                param_strings.push_back(std::to_string(param));
            } else {
                static_assert(std::is_convertible_v<ParamType, std::string>,
                              "Parameter type must be convertible to string");
                param_strings.push_back(std::string(param));
            }
        };

        (add_param(std::forward<Parameters>(parameters)), ...);

        std::array<const char*, sizeof...(parameters)> param_values;
        for (size_t i = 0; i < param_strings.size(); ++i) {
            param_values[i] = param_strings[i].c_str();
        }

        PGresult* res = PQexecParams(m_connection.get(),
                                     std::string(query_with_placeholder).c_str(),
                                     static_cast<int>(param_values.size()),
                                     nullptr,
                                     param_values.data(),
                                     nullptr,
                                     nullptr,
                                     0);

        std::unique_ptr<PGresult, void (*)(PGresult*)> answer(res, PQclear);

        auto const status = PQresultStatus(answer.get());
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            std::string error_msg = PQerrorMessage(m_connection.get());
            ::fmt::print("SQL error: {}. Query was: {}", error_msg, query_with_placeholder);
            return std::unique_ptr<PGresult, void (*)(PGresult*)>(nullptr, PQclear);
        }

        if (status == PGRES_TUPLES_OK && PQntuples(answer.get()) == 0) {
            ::fmt::print("Query returned 0 rows: {}", query_with_placeholder);
            return std::unique_ptr<PGresult, void (*)(PGresult*)>(nullptr, PQclear);
        }

        return answer;
    }

    User getUser(std::unique_ptr<PGresult, void (*)(PGresult*)> const& answer);

    std::mutex                                 m_mutex_query;                   // 40
    std::string                                m_connection_string;             // 32
    std::unique_ptr<PGconn, void (*)(PGconn*)> m_connection{nullptr, PQfinish}; // 8
};
} // namespace db::postgresql