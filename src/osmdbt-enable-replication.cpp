
#include "config.hpp"
#include "db.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <iostream>

namespace {

class EnableReplicationOptions : public Options
{
public:
    EnableReplicationOptions()
    : Options("enable-replication", "Enable replication on the database.")
    {
    }
};

bool app(osmium::VerboseOutput &vout, Config const &config,
         Options const & /*options*/)
{
    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare("enable-replication",
               "SELECT * FROM pg_create_logical_replication_slot($1, "
               "'osm-logical');");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    pqxx::result const result =
        txn.exec_prepared("enable-replication", config.replication_slot());

    if (result.size() == 1 &&
        result[0][0].c_str() == config.replication_slot()) {
        vout << "Replication enabled.\n";
    }

    txn.commit();

    vout << "Done.\n";

    return true;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    EnableReplicationOptions options;
    return app_wrapper(options, argc, argv);
}
