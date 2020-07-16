
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "options.hpp"
#include "util.hpp"

#include <iostream>

bool app(osmium::VerboseOutput &vout, Config const &config,
         Options const & /*options*/)
{
    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    pqxx::read_transaction txn{db};

    int const db_version = get_db_major_version(txn);
    vout << "Database version: " << db_version << " [" << get_db_version(txn)
         << "]\n";

    {
        pqxx::result const result =
            db_version >= 10
                ? txn.exec(
                      "SELECT slot_name, database, confirmed_flush_lsn FROM "
                      "pg_replication_slots WHERE slot_type = 'logical' AND "
                      "plugin = 'osm-logical';")
                : txn.exec(
                      "SELECT slot_name, database, 'unknown' AS lsn FROM "
                      "pg_replication_slots WHERE slot_type = 'logical' AND "
                      "plugin = 'osm-logical';");

        if (result.empty()) {
            vout << "Replication not enabled\n";
        } else {
            vout << "Active replication slots:\n";
            for (auto const &row : result) {
                vout << "  name=" << row[0] << " db=" << row[1]
                     << " lsn=" << row[2] << '\n';
            }
        }
    }

    pqxx::result const result =
        txn.exec("SELECT max(version) FROM schema_migrations WHERE "
                 "char_length(version) = 14;");

    if (result.size() != 1) {
        throw database_error{"Expected single result (schema_migration)."};
    }

    vout << "Schema version: " << result[0][0].c_str() << '\n';

    txn.commit();

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    Options options{"testdb", "Test connection to the database."};
    return app_wrapper(options, argc, argv);
}
