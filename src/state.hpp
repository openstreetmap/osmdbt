#pragma once

#include <osmium/osm/timestamp.hpp>

#include <cassert>
#include <cstddef>
#include <string>

/**
 * The replication diff state contains a monotonically increasing sequence
 * number and the timestamp when this state was reached. It is usually
 * stored in a file called `state.txt` or `SEQUENCE_NUM.state.txt`.
 */
class State
{
public:
    State(std::size_t sequence_number, osmium::Timestamp timestamp)
    : m_sequence_number(sequence_number), m_timestamp(timestamp)
    {}

    explicit State(std::string const &filename);

    std::size_t sequence_number() const noexcept { return m_sequence_number; }

    osmium::Timestamp timestamp() const noexcept { return m_timestamp; }

    void write(std::string const &filename) const;

    State next(osmium::Timestamp timestamp) const noexcept
    {
        assert(sequence_number() < 999999999);
        return State{sequence_number() + 1, timestamp};
    }

    std::string dir1_path() const;
    std::string dir2_path() const;
    std::string state_path() const;
    std::string osc_path() const;

    std::string to_string() const;

private:
    std::string path() const;

    std::size_t m_sequence_number = 0;
    osmium::Timestamp m_timestamp{};

}; // class State
