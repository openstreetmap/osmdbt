#pragma once

#include <pqxx/pqxx> // IWYU pragma: export

#include <string>

std::string get_db_version(pqxx::dbtransaction &txn);
int get_db_major_version(pqxx::dbtransaction &txn);

void catchup_to_lsn(pqxx::dbtransaction &txn,
                    std::string const &replication_slot,
                    std::string const &lsn);
