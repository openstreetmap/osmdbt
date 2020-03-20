
#include <catch.hpp>

#include "util.hpp"

#include <string>

TEST_CASE("create_replication_log_name")
{
    REQUIRE(create_replication_log_name("foo", 0) ==
            "/osm-repl-1970-01-01T00:00:00Z-foo.log");
    REQUIRE(create_replication_log_name("bar", 1345834023) ==
            "/osm-repl-2012-08-24T18:47:03Z-bar.log");
}
