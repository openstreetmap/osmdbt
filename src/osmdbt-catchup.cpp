
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "util.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/verbose_output.hpp>

#include <iostream>
#include <string>

class CatchupOptions : public Options
{
public:
    CatchupOptions()
    : Options({"catchup", "Advance replication slot to specified LSN."})
    {}

    std::string const &lsn() const noexcept { return m_lsn; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("lsn,l", po::value<std::string>(), "LSN (Log Sequence number) (required)");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(
        boost::program_options::variables_map const &vm) override
    {
        if (vm.count("lsn")) {
            m_lsn = vm["lsn"].as<std::string>();
        } else {
            throw argument_error{
                "Missing '--lsn LSN' or '-l LSN' on command line"};
        }
    }

    std::string m_lsn;
}; // class CatchupOptions

bool app(osmium::VerboseOutput &vout, Config const &config,
         CatchupOptions const &options)
{
    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Catching up to " << options.lsn() << "...\n";
    catchup_to_lsn(txn, config.replication_slot(), options.lsn());

    txn.commit();

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    CatchupOptions options;
    return app_wrapper(options, argc, argv);
}
