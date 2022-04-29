
#include "lsn.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

lsn_type::lsn_type(char const *lsn)
{
    assert(lsn);

    char *end = nullptr;
    auto const upper = std::strtoull(lsn, &end, 16);

    if (*end != '-' && *end != '/') {
        throw std::runtime_error{std::string{"Error parsing LSN '"} + lsn +
                                 "'"};
    }

    auto const lower = std::strtoull(end + 1, &end, 16);

    if (*end != '\0') {
        throw std::runtime_error{std::string{"Error parsing LSN '"} + lsn +
                                 "'"};
    }

    m_lsn = (upper << 32U) + lower;
}

std::string lsn_type::str() const
{
    std::string result(20, 'x');

    int const n = std::snprintf(result.data(), result.size(), "%lX/%lX",
                                upper(), lower());
    assert(n > 0);
    result.resize(n);

    return result;
}
