#pragma once

#include <pqxx/pqxx>
#include <string>

std::string get_db_version(pqxx::work &txn);

void catchup_to_lsn(pqxx::work &txn, std::string const &replication_slot,
                    std::string const &lsn);
