
#include <catch.hpp>

#include "util.hpp"

TEST_CASE("replace_suffix")
{
    REQUIRE(replace_suffix("foo.baz", ".bar") == "foo.bar");
    REQUIRE(replace_suffix("foo", ".bar") == "foo.bar");
    REQUIRE(replace_suffix("foo.x.y", ".bar") == "foo.x.bar");
    REQUIRE(replace_suffix("foo.", ".bar") == "foo.bar");
    REQUIRE(replace_suffix(".", ".bar") == ".bar");
    REQUIRE(replace_suffix(".foo", ".bar") == ".bar");
    REQUIRE(replace_suffix("", ".bar") == ".bar");
}
