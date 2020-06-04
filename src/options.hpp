#pragma once

#include <boost/program_options.hpp>

#include <osmium/util/verbose_output.hpp>

#include <string>

namespace po = boost::program_options;

/**
 * Handles command line options.
 *
 * Some commands that only have the common options use this class directly,
 * others use a class derived from this class implementing any options specific
 * to that command.
 */
class Options
{
public:
    explicit Options(char const *name, char const *description)
    : m_name(name), m_description(description)
    {}

    virtual ~Options() = default;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    void parse_command_line(int argc, char *argv[]);

    bool quiet() const noexcept { return m_quiet; };

    std::string const &config_file() const noexcept { return m_config_file; }

    void show_version(osmium::VerboseOutput &vout);

private:
    void check_common_options(boost::program_options::variables_map const &vm,
                              po::options_description const &desc);

    virtual void add_command_options(po::options_description & /*desc*/) {}

    virtual void
    check_command_options(boost::program_options::variables_map const & /*vm*/)
    {}

    char const *m_name;
    char const *m_description;
    bool m_quiet = false;
    std::string m_config_file{"osmdbt-config.yaml"};
}; // class Options
