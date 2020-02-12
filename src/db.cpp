
#include "db.hpp"

#include <cstring>
#include <stdexcept>

std::string get_db_version(pqxx::work &txn)
{
    pqxx::result const result = txn.exec("SELECT * FROM version();");
    if (result.size() != 1) {
        throw std::runtime_error{
            "Database error (version): Expected exactly one result"};
    }

    auto const &row = result[0];
    if (std::strncmp(row[0].c_str(), "PostgreSQL", 10)) {
        throw std::runtime_error{"Database error: Expected version string"};
    }

    return row[0].as<std::string>();
}

void catchup_to_lsn(pqxx::work &txn, std::string const &replication_slot,
                    std::string const &lsn)
{

    if (txn.conn().server_version() >= 110000) {
        txn.conn().prepare("advance",
                           "SELECT * FROM pg_replication_slot_advance($1,"
                           " CAST ($2 AS pg_lsn));");
    } else {
        txn.conn().prepare("advance",
                           "SELECT * FROM pg_logical_slot_get_changes($1, "
                           "CAST ($2 AS pg_lsn), NULL);");
    }

    pqxx::result const result =
        txn.prepared("advance")(replication_slot)(lsn).exec();
}
