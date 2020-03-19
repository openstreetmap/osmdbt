#pragma once

#include <osmium/osm/timestamp.hpp>

#include <cassert>
#include <cstddef>
#include <string>

class State
{
public:
    State(std::size_t sequence_number, osmium::Timestamp timestamp)
    : m_sequence_number(sequence_number), m_timestamp(timestamp)
    {}

    State(std::string const &filename);

    std::size_t sequence_number() const noexcept { return m_sequence_number; }

    osmium::Timestamp timestamp() const noexcept { return m_timestamp; }

    void write(std::string const &filename) const;

    State next(osmium::Timestamp timestamp) const noexcept
    {
        assert(sequence_number() < 999999999);
        return State{sequence_number() + 1, timestamp};
    }

    std::string dir_path() const;
    std::string state_path() const;
    std::string osc_path() const;

private:
    std::string path() const;

    std::string to_string() const;

    std::size_t m_sequence_number = 0;
    osmium::Timestamp m_timestamp{};

}; // class State
