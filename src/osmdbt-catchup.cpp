
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/verbose_output.hpp>

#include <iostream>
#include <string>

static Command command_catchup = {"catchup",
                                  "Advance replication slot to specified LSN."};

class CatchupOptions : public Options
{
public:
    CatchupOptions() : Options(command_catchup) {}

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

int main(int argc, char *argv[])
{
    try {
        CatchupOptions options;
        options.parse_command_line(argc, argv);
        osmium::VerboseOutput vout{!options.quiet()};
        options.show_version(vout);

        vout << "Reading config from '" << options.config_file() << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};

        pqxx::work txn{db};
        vout << "Database version: " << get_db_version(txn) << '\n';

        vout << "Catching up to " << options.lsn() << "...\n";
        catchup_to_lsn(txn, config.replication_slot(), options.lsn());

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
