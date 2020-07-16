#pragma once

#include "lsn.hpp"

#include <pqxx/pqxx> // IWYU pragma: export

#include <string>

std::string get_db_version(pqxx::basic_transaction &txn);
int get_db_major_version(pqxx::basic_transaction &txn);

void catchup_to_lsn(pqxx::basic_transaction &txn, std::string const &replication_slot,
                    lsn_type lsn);
