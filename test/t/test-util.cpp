
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

TEST_CASE("dirname")
{
    REQUIRE(dirname("foo/bar") == "foo");
    REQUIRE(dirname("foo/x") == "foo");
    REQUIRE(dirname("foo/") == "foo");
    REQUIRE(dirname("foo") == ".");
    REQUIRE(dirname("ex/foo/bar") == "ex/foo");
    REQUIRE(dirname("ex/foo/x") == "ex/foo");
    REQUIRE(dirname("ex/foo/") == "ex/foo");
    REQUIRE(dirname("/foo/bar") == "/foo");
    REQUIRE(dirname("/foo/x") == "/foo");
    REQUIRE(dirname("/foo/") == "/foo");
    REQUIRE(dirname("/foo") == "/");
    REQUIRE(dirname("/ex/foo/bar") == "/ex/foo");
    REQUIRE(dirname("/ex/foo/x") == "/ex/foo");
    REQUIRE(dirname("/ex/foo/") == "/ex/foo");
    REQUIRE(dirname("/ex/foo") == "/ex");
}

TEST_CASE("get_time")
{
    REQUIRE(get_time(0) == "19700101T000000");
    REQUIRE(get_time(1345834023) == "20120824T184703");
}
