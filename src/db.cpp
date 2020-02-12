
#include "db.hpp"

int get_db_version(pqxx::work &txn)
{
    pqxx::result const result = txn.exec("SELECT * FROM version();");
    if (result.size() != 1) {
        throw std::runtime_error{
            "Database error (version): Expected exactly one result"};
    }

    if (std::strncmp(result[0][0].c_str(), "PostgreSQL", 10)) {
        throw std::runtime_error{
            "Database error: Expected version string"};
    }

    return std::atoi(result[0][0].c_str() + 11);
}

void catchup_to_lsn(pqxx::work &txn, int version, std::string const &replication_slot, std::string const &lsn)
{

    if (version >= 11) {
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

