
#include <catch.hpp>

#include "config.hpp"
#include "exception.hpp"

TEST_CASE("config file not found")
{
    osmium::VerboseOutput vout{false};
    REQUIRE_THROWS_AS(Config("does-not-exist", vout), YAML::Exception);
}

TEST_CASE("empty config file")
{
    osmium::VerboseOutput vout{false};
    Config config{"test/t/test_config_empty.yaml", vout};

    REQUIRE(config.db_connection() ==
            "host=localhost port=5432 dbname=osm user=osm password=osm");
    REQUIRE(config.replication_slot() == "rs");
    REQUIRE(config.log_dir() == "/tmp");
    REQUIRE(config.changes_dir() == "/tmp");
    REQUIRE(config.run_dir() == "/tmp");
}

TEST_CASE("default config file")
{
    osmium::VerboseOutput vout{false};
    Config config{"osmdbt_config.yaml", vout};

    REQUIRE(config.db_connection() ==
            "host=localhost port=5432 dbname=osm user=osm password=osm");
    REQUIRE(config.replication_slot() == "rs");
    REQUIRE(config.log_dir() == "/tmp");
    REQUIRE(config.changes_dir() == "/tmp");
    REQUIRE(config.run_dir() == "/tmp");
}

TEST_CASE("invalid database section")
{
    osmium::VerboseOutput vout{false};
    REQUIRE_THROWS_AS(Config("test/t/test_config_invalid_db.yaml", vout),
                      config_error);
    REQUIRE_THROWS_WITH(Config("test/t/test_config_invalid_db.yaml", vout),
                        "Config error: 'database' entry must be a Map.");
}

TEST_CASE("invalid yaml")
{
    osmium::VerboseOutput vout{false};
    REQUIRE_THROWS_AS(Config("test/t/test_config_invalid_yaml.yaml", vout),
                      YAML::Exception);
}
