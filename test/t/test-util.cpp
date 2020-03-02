
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

TEST_CASE("create_replication_log_name")
{
    REQUIRE(create_replication_log_name("foo", 0) ==
            "/osm-repl-1970-01-01T00:00:00Z-foo.log");
    REQUIRE(create_replication_log_name("bar", 1345834023) ==
            "/osm-repl-2012-08-24T18:47:03Z-bar.log");
}
