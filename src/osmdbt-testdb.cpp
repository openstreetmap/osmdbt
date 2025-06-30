
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "options.hpp"
#include "util.hpp"

#include <iostream>

namespace {

class TestDbOptions : public Options
{
public:
    TestDbOptions() : Options("testdb", "Test connection to the database.") {}
};

void print_config(osmium::VerboseOutput &vout, pqxx::read_transaction &txn,
                  std::string const &setting)
{
    pqxx::result const result = txn.exec("SHOW " + setting);

    if (result.size() != 1) {
        throw database_error{"Expected single result (" + setting + ")."};
    }

    vout << "  " << setting << "=" << result[0][0].c_str() << "\n";
}

bool app(osmium::VerboseOutput &vout, Config const &config,
         Options const & /*options*/)
{
    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    pqxx::read_transaction txn{db};

    int const db_version = get_db_major_version(txn);
    vout << "Database version: " << db_version << " [" << get_db_version(txn)
         << "]\n";

    vout << "Database config:\n";
    print_config(vout, txn, "wal_level");
    print_config(vout, txn, "max_replication_slots");

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
            bool has_configured_replication_slot = false;
            vout << "Active replication slots:\n";
            for (auto const &row : result) {
                if (config.replication_slot() == row[0].c_str()) {
                    has_configured_replication_slot = true;
                }
                vout << "  name=" << row[0].c_str() << " db=" << row[1].c_str()
                     << " lsn=" << row[2].c_str() << '\n';
            }
            if (has_configured_replication_slot) {
                db.prepare("peek",
                           "SELECT * FROM pg_logical_slot_peek_changes($1, "
                           "NULL, NULL);");
                pqxx::result const result_peek =
                    txn.exec_prepared("peek", config.replication_slot());
                if (result_peek.empty()) {
                    vout << "There are no";
                } else {
                    vout << "There are " << result_peek.size();
                }
                vout << " changes in your configured replication slot.\n";
            } else {
                vout << "Your configured replication slot is not active!\n";
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

} // anonymous namespace

int main(int argc, char *argv[])
{
    TestDbOptions options;
    return app_wrapper(options, argc, argv);
}
