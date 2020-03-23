
#include "io.hpp"
#include "state.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/string.hpp>

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

std::string State::to_string() const
{
    std::string str;

    str += "sequenceNumber=";
    str += std::to_string(m_sequence_number);
    str += "\ntimestamp=";

    for (auto const c : m_timestamp.to_iso_all()) {
        if (c == ':') {
            str += '\\';
        }
        str += c;
    }

    str += '\n';

    return str;
}

static std::string remove_backslash(std::string const &in)
{
    std::string out;
    std::remove_copy(in.cbegin(), in.cend(), std::back_inserter(out), '\\');
    return out;
}

State::State(std::string const &filename)
{
    std::ifstream statefile{filename};
    if (!statefile.is_open()) {
        throw std::system_error{errno, std::system_category(),
                                "Could not open state file '" + filename + "'"};
    }

    for (std::string line; std::getline(statefile, line);) {
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        auto const parts = osmium::split_string(line, '=');
        if (parts.size() != 2) {
            continue;
        }

        if (parts[0] == "sequenceNumber") {
            char *end = nullptr;
            m_sequence_number = std::strtoull(parts[1].c_str(), &end, 10);
            if (end != &*parts[1].cend()) {
                throw std::runtime_error{
                    "Invalid sequenceNumber in state file '" + filename + "'"};
            }
        } else if (parts[0] == "timestamp") {
            m_timestamp = osmium::Timestamp{remove_backslash(parts[1])};
        }
    }

    if (m_sequence_number == 0) {
        throw std::runtime_error{"Missing sequenceNumber in state file '" +
                                 filename + "'"};
    }

    if (m_timestamp == osmium::Timestamp{}) {
        throw std::runtime_error{"Missing timestamp in state file '" +
                                 filename + "'"};
    }
}

void State::write(std::string const &filename) const
{
    auto const content = to_string();

    int const fd = excl_write_open(filename);

    if (fd < 0) {
        throw std::system_error{errno, std::system_category(),
                                "Can not create state file '" + filename +
                                    "'."};
    }

    osmium::io::detail::reliable_write(fd, content.data(), content.size());
    osmium::io::detail::reliable_fsync(fd);
    osmium::io::detail::reliable_close(fd);
}

std::string State::path() const
{
    std::string path(10, 'x');
    auto const num =
        std::snprintf(&path[0], path.size(), "%09zu", m_sequence_number);
    assert(num == 9);
    path.resize(num);

    path.insert(6, 1, '/');
    path.insert(3, 1, '/');

    return path;
}

std::string State::state_path() const { return path() + ".state.txt"; }

std::string State::osc_path() const { return path() + ".osc.gz"; }

std::string State::dir1_path() const
{
    std::string p{path()};
    assert(p.size() == 11);
    p.resize(3); // only keep first path component
    return p;
}

std::string State::dir2_path() const
{
    std::string p{path()};
    assert(p.size() == 11);
    p.resize(p.size() - 4); // remove "/nnn" at the end
    return p;
}
