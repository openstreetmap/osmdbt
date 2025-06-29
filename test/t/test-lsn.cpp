
#include <catch.hpp>

#include "lsn.hpp"

#include <string>

TEST_CASE("Valid LSN")
{
    lsn_type const l1;
    REQUIRE(l1.value() == 0ULL);
    REQUIRE(l1.upper() == 0ULL);
    REQUIRE(l1.lower() == 0ULL);
    REQUIRE(l1.str() == "0/0");

    lsn_type const l2{"1-1a"};
    REQUIRE(l2.value() == 4294967322ULL);
    REQUIRE(l2.upper() == 1ULL);
    REQUIRE(l2.lower() == 0x1aULL);
    REQUIRE(l2.str() == "1/1A");

    lsn_type const l3{"0/deadbeef"};
    REQUIRE(l3.value() == 0xdeadbeefULL);
    REQUIRE(l3.upper() == 0ULL);
    REQUIRE(l3.lower() == 0xdeadbeefULL);
    REQUIRE(l3.str() == "0/DEADBEEF");

    lsn_type const l4{"11223344/55667788"};
    REQUIRE(l4.value() == (0x11223344ULL << 32U) + 0x55667788ULL);
    REQUIRE(l4.upper() == 0x11223344ULL);
    REQUIRE(l4.lower() == 0x55667788ULL);
    REQUIRE(l4.str() == "11223344/55667788");

    REQUIRE(l1 < l2);
    REQUIRE(l1 < l3);
    REQUIRE(l3 < l2);
    REQUIRE(l3 < l4);
    REQUIRE_FALSE(l1 > l2);
    REQUIRE_FALSE(l1 > l3);
    REQUIRE_FALSE(l3 > l2);
    REQUIRE_FALSE(l3 > l4);
}

TEST_CASE("Invalid LSN")
{
    REQUIRE_THROWS(lsn_type{"foo"});
    REQUIRE_THROWS(lsn_type{"12.34"});
    REQUIRE_THROWS(lsn_type{"some/thing"});
    REQUIRE_THROWS(lsn_type{"123-3x"});
}
