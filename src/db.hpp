
#pragma once

#include <pqxx/pqxx>

int get_db_version(pqxx::work &txn);
void catchup_to_lsn(pqxx::work &txn, int version,
                    std::string const &replication_slot,
                    std::string const &lsn);
