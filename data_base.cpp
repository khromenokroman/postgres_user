#include "data_base.hpp"

#include <cstdlib>
#include <fmt/format.h>
#include <postgresql/libpq-fe.h>

namespace db::postgresql
{

DataBase::DataBase(std::string_view const& connection) : m_connection_string{connection} {
    PGconn* conn = PQconnectdb(connection.data());
    if (PQstatus(conn) != CONNECTION_OK) {
        throw exceptions::ConnectionFail(PQerrorMessage(conn));
    }
    m_connection.reset(conn);

    if (PQsetnonblocking(m_connection.get(), 1) != 0) {
        throw exceptions::NonBlock(::fmt::format(
          "Couldn't set nonblocking mode to psql {} / {}", connection, PQerrorMessage(m_connection.get())));
    }
    ::fmt::print("Connection is OK for {}", connection);
}
DataBase::~DataBase() {
    ::fmt::print("Connections is CLOSE for {}", m_connection_string);
}

User DataBase::getUser(std::unique_ptr<PGresult, void (*)(PGresult*)> const& answer) {
    return User{.id        = getValue<size_t>(answer, "id"),
                .login     = getValue<std::string>(answer, "login"),
                .email     = getValue<std::string>(answer, "email"),
                .password  = getValue<std::string>(answer, "password"),
                .backendId   = getBackendServerById(getValue<size_t>(answer, "\"backendId\"")),
                .token     = getValue<std::string>(answer, "token"),
                .tokenExp = getValue<size_t>(answer, "\"tokenExp\""),
                .status    = getValue<size_t>(answer, "status")};
}

User DataBase::getUserById(std::string const& id) {

    auto const res = query("SELECT * FROM users WHERE id = $1", id);
    if (!res) {
        return User{};
    }
    return getUser(res);
}

User DataBase::getUserByToken(std::string const& token) {
    auto const res = query("SELECT * FROM users WHERE token = $1", token);
    if (!res) {
        return User{};
    }
    return getUser(res);
}

User DataBase::getUserByLogin(std::string const& login) {
    auto const res = query("SELECT * FROM users WHERE login = $1", login);
    if (!res) {
        return User{};
    }
    return getUser(res);
}

User DataBase::getUserByEmail(std::string const& email) {
    auto const res = query("SELECT * FROM users WHERE email = $1", email);
    if (!res) {
        return User{};
    }
    return getUser(res);
}

void DataBase::updateUser(const User& user) {
    if (auto const res = query("UPDATE users "
                               "SET login = $1, email = $2, password = $3, \"backendId\" = $4, token = $5, \"tokenExp\" "
                               "= $6, status = $7 "
                               "WHERE id = $8",
                               user.login,
                               user.email,
                               user.password,
                               user.backendId.id,
                               user.token,
                               user.tokenExp,
                               user.status,
                               user.id);
        !res) {
        ::fmt::print("Update user FAIL");
    } else {
        ::fmt::print("Update user OK");
    }
}

Backend DataBase::getBackendServerById(size_t id) {
    auto const res = query("SELECT * FROM backend WHERE id = $1", id);
    if (!res) {
        return Backend{};
    }
    return Backend{.id      = getValue<size_t>(res, "id"),
                   .address = getValue<std::string>(res, "address"),
                   .region  = getValue<std::string>(res, "region")};
}

void DataBase::addUser(const User& user) {
    if (auto const res = query("INSERT INTO users (login, email, password, \"backendId\", token, \"tokenExp\", status) "
                               "VALUES ($1, $2, $3, $4, $5, $6, $7)",
                               user.login,
                               user.email,
                               user.password,
                               user.backendId.id,
                               user.token,
                               user.tokenExp,
                               user.status);
        !res) {
        ::fmt::print("Add user FAIL");
    } else {
        ::fmt::print("Add user OK");
    }
}

} // namespace db::postgresql
