#include "data_base.hpp"

int main() {
    db::postgresql::DataBase pg("postgresql://postgres:123456@10.10.0.1:5432/users_db");
    std::string login = "TestUser0";
    auto user = pg.getUserByLogin(login);

    ::fmt::print("User: {}\n", nlohmann::json{user}.dump(4));

    return 0;
}
