#pragma once

#include <cstdint>
#include <string>

/**
 * An implementation of the PostgreSQL LSN (Log Sequence Number) type.
 *
 * https://www.postgresql.org/docs/current/datatype-pg-lsn.html
 */
class lsn_type
{
public:
    lsn_type() = default;

    explicit lsn_type(char const *lsn);

    explicit lsn_type(std::string const &lsn) : lsn_type(lsn.c_str()) {}

    std::uint64_t value() const noexcept { return m_lsn; }

    explicit operator bool() const noexcept { return m_lsn != 0; }

    std::uint64_t upper() const noexcept { return m_lsn >> 32U; };

    std::uint64_t lower() const noexcept { return m_lsn & 0xffffffffULL; };

    std::string str() const;

    friend bool operator<(lsn_type lhs, lsn_type rhs) noexcept
    {
        return lhs.m_lsn < rhs.m_lsn;
    }

    friend bool operator>(lsn_type lhs, lsn_type rhs) noexcept
    {
        return rhs < lhs;
    }

    friend bool operator==(lsn_type lhs, lsn_type rhs) noexcept
    {
        return lhs.m_lsn == rhs.m_lsn;
    }

    friend bool operator!=(lsn_type lhs, lsn_type rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:
    std::uint64_t m_lsn = 0;

}; // class lsn_type
