#pragma once

#include "config.hpp"
#include "exception.hpp"

#include <osmium/util/verbose_output.hpp>

#include <boost/program_options.hpp>

#include <csignal>
#include <ctime>
#include <string>

std::string get_time(std::time_t now);
std::string create_replication_log_name(std::string const &name,
                                        std::time_t time = std::time(nullptr));
void write_data_to_file(std::string const &data, std::string const &dir_name,
                        std::string const &file_name);

template <typename TOptions>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
int app_wrapper(TOptions &options, int argc, char *argv[])
{
    // NOLINTNEXTLINE(cert-err33-c,cppcoreguidelines-pro-type-cstyle-cast)
    std::signal(SIGPIPE, SIG_IGN);

    try {
        options.parse_command_line(argc, argv);
        osmium::VerboseOutput vout{!options.quiet()};
        options.show_version(vout);

        vout << "Reading config from '" << options.config_file() << "'\n";
        Config config{options.config_file(), vout};

        return app(vout, config, options) ? 0 : 1;
    } catch (argument_error const &e) {
        std::cerr << e.what() << '\n';
        return 3;
    } catch (boost::program_options::error const &e) {
        std::cerr << e.what() << '\n';
        return 3;
    } catch (YAML::Exception const &e) {
        std::cerr << e.what() << '\n';
        return 3;
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
