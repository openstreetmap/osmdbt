
#include "options.hpp"

#include <iostream>

po::options_description add_common_options()
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

Options check_common_options(boost::program_options::variables_map const &vm,
                             po::options_description const &desc,
                             Command const &command)
{
    Options options;

    if (vm.count("help")) {
        std::cout << "Usage: osmdbt-" << command.name << ' ' << command.synopsis
                  << "\n\n"
                  << command.description << "\n"
                  << desc << '\n';
        std::exit(0);
    }

    if (vm.count("config")) {
        options.config_file = vm["config"].as<std::string>();
    }

    options.quiet = vm.count("quiet");

    return options;
}

Options parse_command_line(int argc, char *argv[], Command const &command,
                           po::options_description const *opts_cmd)
{
    po::options_description opts_common{add_common_options()};

    po::options_description desc;
    if (opts_cmd) {
        desc.add(*opts_cmd);
    }
    desc.add(opts_common);

    po::options_description parsed_options;
    parsed_options.add(desc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(parsed_options).run(),
              vm);
    po::notify(vm);

    return check_common_options(vm, desc, command);
}

void show_version(osmium::VerboseOutput &vout, Command const &command)
{
    vout << "Started osmdbt-" << command.name << '\n';
    vout << "  " << get_osmdbt_long_version() << '\n';
    vout << "  " << get_libosmium_version() << '\n';
}
