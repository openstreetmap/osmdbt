#pragma once

#include <boost/program_options.hpp>

#include <osmium/util/verbose_output.hpp>

#include <string>

namespace po = boost::program_options;

char const *get_osmdbt_version() noexcept;
char const *get_osmdbt_long_version() noexcept;
char const *get_libosmium_version() noexcept;

struct Command
{
    char const *name;
    char const *synopsis;
    char const *description;
};

class Options
{
public:
    explicit Options(Command const &command) : m_command(command) {}

    virtual ~Options() = default;

    void parse_command_line(int argc, char *argv[],
                            po::options_description const *opts_cmd = nullptr);

    bool quiet() const noexcept { return m_quiet; };

    std::string const &config_file() const noexcept { return m_config_file; }

    void show_version(osmium::VerboseOutput &vout);

private:
    void check_common_options(boost::program_options::variables_map const &vm,
                              po::options_description const &desc);

    virtual void
    check_command_options(boost::program_options::variables_map const & /*vm*/)
    {}

    Command m_command;
    bool m_quiet = false;
    std::string m_config_file{"osmdbt_config.yaml"};
};
