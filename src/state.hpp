#pragma once

#include <osmium/osm/timestamp.hpp>

#include <cassert>
#include <cstddef>
#include <ctime>
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

    [[nodiscard]] std::size_t sequence_number() const noexcept
    {
        return m_sequence_number;
    }

    [[nodiscard]] osmium::Timestamp timestamp() const noexcept
    {
        return m_timestamp;
    }

    void write(std::string const &filename,
               std::time_t comment_timestamp = 0) const;

    [[nodiscard]] State next(osmium::Timestamp timestamp) const noexcept
    {
        assert(sequence_number() < 999999999);
        return State{sequence_number() + 1, timestamp};
    }

    [[nodiscard]] std::string dir1_path() const;
    [[nodiscard]] std::string dir2_path() const;
    [[nodiscard]] std::string state_path() const;
    [[nodiscard]] std::string osc_path() const;

    [[nodiscard]] std::string
    to_string(std::time_t comment_timestamp = 0) const;

private:
    [[nodiscard]] std::string path() const;

    std::size_t m_sequence_number = 0;
    osmium::Timestamp m_timestamp{};

}; // class State
