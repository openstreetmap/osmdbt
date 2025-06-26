
#include "options.hpp"
#include "version.hpp"

#include <iostream>

static po::options_description add_common_options()
{
    po::options_description options{"COMMON OPTIONS"};

    // clang-format off
    options.add_options()
        ("config,c", po::value<std::string>(), "Config file")
        ("help,h", "Show usage help")
        ("quiet,q", "Disable verbose mode");
    // clang-format on

    return options;
}

void Options::check_common_options(po::variables_map const &vm,
                                   po::options_description const &desc)
{
    if (vm.count("help")) {
        std::cout << "Usage: osmdbt-" << m_name << " [OPTIONS]\n\n"
                  << m_description << "\n"
                  << desc << '\n';
        std::exit(0);
    }

    if (vm.count("config")) {
        m_config_file = vm["config"].as<std::string>();
    }

    m_quiet = vm.count("quiet");
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
void Options::parse_command_line(int argc, char *argv[])
{
    po::options_description desc;

    add_command_options(desc);
    po::options_description const opts_common{add_common_options()};
    desc.add(opts_common);

    po::options_description parsed_options;
    parsed_options.add(desc);

    po::positional_options_description const p;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(parsed_options)
                  .positional(p)
                  .run(),
              vm);
    po::notify(vm);

    check_common_options(vm, desc);
    check_command_options(vm);
}

void Options::show_version(osmium::VerboseOutput &vout)
{
    vout << "Started osmdbt-" << m_name << '\n';
    vout << "  " << get_osmdbt_long_version() << '\n';
    vout << "  " << get_libosmium_version() << '\n';
}
