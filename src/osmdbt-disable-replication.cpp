
#include "config.hpp"
#include "db.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <iostream>

namespace {

class DisableReplicationOptions : public Options
{
public:
    DisableReplicationOptions()
    : Options("disable-replication", "Disable replication on the database.")
    {
    }
};

bool app(osmium::VerboseOutput &vout, Config const &config,
         Options const & /*options*/)
{
    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare("disable-replication",
               "SELECT * FROM pg_drop_replication_slot($1);");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    pqxx::result const result =
        txn.exec_prepared("disable-replication", config.replication_slot());

    if (result.size() == 1 && result[0][0].c_str()[0] == '\0') {
        vout << "Replication disabled.\n";
    }

    txn.commit();

    vout << "Done.\n";

    return true;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    DisableReplicationOptions options;
    return app_wrapper(options, argc, argv);
}
