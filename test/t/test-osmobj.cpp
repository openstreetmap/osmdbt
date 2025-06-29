
#include <catch.hpp>

#include "osmobj.hpp"

#include <osmium/osm/item_type.hpp>

#include <algorithm>
#include <vector>

TEST_CASE("create osmobj and compare")
{
    osmobj const a{"n123", "v3", "c12", nullptr};
    REQUIRE(a.type() == osmium::item_type::node);
    REQUIRE(a.id() == 123);
    REQUIRE(a.version() == 3);
    REQUIRE(a.cid() == 12);

    osmobj const b{"n123", "v1", "c3", nullptr};
    REQUIRE(b.type() == osmium::item_type::node);
    REQUIRE(b.id() == 123);
    REQUIRE(b.version() == 1);
    REQUIRE(b.cid() == 3);

    osmobj const c{"w222", "v2", "c12", nullptr};
    REQUIRE(c.type() == osmium::item_type::way);
    REQUIRE(c.id() == 222);
    REQUIRE(c.version() == 2);
    REQUIRE(c.cid() == 12);

    REQUIRE_FALSE(a < b);
    REQUIRE(a > b);
    REQUIRE_FALSE(a <= b);
    REQUIRE(a >= b);

    REQUIRE(a < c);
    REQUIRE_FALSE(a > c);
    REQUIRE(a <= c);
    REQUIRE_FALSE(a >= c);

    REQUIRE(b < c);
    REQUIRE_FALSE(b > c);
    REQUIRE(b <= c);
    REQUIRE_FALSE(b >= c);
}

TEST_CASE("sorting")
{
    std::vector<osmobj> o{
        osmobj{"w133", "v10", "c1"}, osmobj{"w134", "v10", "c1"},
        osmobj{"w134", "v11", "c1"}, osmobj{"n1", "v1", "c1"},
        osmobj{"n2", "v1", "c1"},    osmobj{"n1", "v3", "c1"},
        osmobj{"r10", "v10", "c1"},  osmobj{"n3", "v2", "c1"}};

    std::sort(o.begin(), o.end());

    REQUIRE(o[0].type() == osmium::item_type::node);
    REQUIRE(o[1].type() == osmium::item_type::node);
    REQUIRE(o[2].type() == osmium::item_type::node);
    REQUIRE(o[3].type() == osmium::item_type::node);
    REQUIRE(o[4].type() == osmium::item_type::way);
    REQUIRE(o[5].type() == osmium::item_type::way);
    REQUIRE(o[6].type() == osmium::item_type::way);
    REQUIRE(o[7].type() == osmium::item_type::relation);

    REQUIRE(o[0].id() == 1);
    REQUIRE(o[1].id() == 1);
    REQUIRE(o[2].id() == 2);
    REQUIRE(o[3].id() == 3);
    REQUIRE(o[4].id() == 133);
    REQUIRE(o[5].id() == 134);
    REQUIRE(o[6].id() == 134);
    REQUIRE(o[7].id() == 10);

    REQUIRE(o[0].version() == 1);
    REQUIRE(o[1].version() == 3);
    REQUIRE(o[2].version() == 1);
    REQUIRE(o[3].version() == 2);
    REQUIRE(o[4].version() == 10);
    REQUIRE(o[5].version() == 10);
    REQUIRE(o[6].version() == 11);
    REQUIRE(o[7].version() == 10);
}

TEST_CASE("parser error")
{
    REQUIRE_THROWS(osmobj("x123", "v3", "c12"));
    REQUIRE_THROWS(osmobj("n", "v3", "c12"));
    REQUIRE_THROWS(osmobj("", "v3", "c12"));
    REQUIRE_THROWS(osmobj("n123", "x3", "c12"));
    REQUIRE_THROWS(osmobj("n123", "v", "c12"));
    REQUIRE_THROWS(osmobj("n123", "", "c12"));
    REQUIRE_THROWS(osmobj("n123", "v3", "x12"));
    REQUIRE_THROWS(osmobj("n123", "v3", "c"));
    REQUIRE_THROWS(osmobj("n123", "v3", ""));
}
