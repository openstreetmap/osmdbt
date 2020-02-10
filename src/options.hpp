#pragma once

#include <boost/program_options.hpp>

#include <osmium/util/verbose_output.hpp>

#include <string>

char const *get_osmdbt_version() noexcept;
char const *get_osmdbt_long_version() noexcept;
char const *get_libosmium_version() noexcept;

struct Command
{
    char const *name;
    char const *synopsis;
    char const *description;
};

struct Options
{
    bool quiet = false;
    std::string config_file{"osmdbt_config.yaml"};
};

namespace po = boost::program_options;

po::options_description add_common_options();

Options check_common_options(boost::program_options::variables_map const &vm,
                             po::options_description const &desc,
                             char const *synopsis, char const *description);

Options parse_command_line(int argc, char *argv[], Command const &command);

void show_version(osmium::VerboseOutput &vout, Command const &command);
