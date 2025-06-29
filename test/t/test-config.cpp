
#include <catch.hpp>

#include "config.hpp"
#include "exception.hpp"

#include <osmium/util/verbose_output.hpp>

#include <string>

TEST_CASE("config file not found")
{
    osmium::VerboseOutput vout{false};
    REQUIRE_THROWS_AS(Config("does-not-exist", vout), std::system_error);
}

TEST_CASE("empty config file")
{
    osmium::VerboseOutput vout{false};
    Config const config{"test/t/test-config-empty.yaml", vout};

    REQUIRE(config.db_connection() ==
            "port=5432 dbname=osm user=osm password=osm");
    REQUIRE(config.replication_slot() == "osm_repl");
    REQUIRE(config.log_dir() == "/tmp/");
    REQUIRE(config.changes_dir() == "/tmp/");
    REQUIRE(config.run_dir() == "/tmp/");
}

TEST_CASE("default config file")
{
    osmium::VerboseOutput vout{false};
    Config const config{"osmdbt-config.yaml", vout};

    REQUIRE(config.db_connection() ==
            "port=5432 dbname=osm user=osm password=osm");
    REQUIRE(config.replication_slot() == "osm_repl");
    REQUIRE(config.log_dir() == "/tmp/");
    REQUIRE(config.changes_dir() == "/tmp/");
    REQUIRE(config.run_dir() == "/tmp/");
}

TEST_CASE("invalid database section")
{
    osmium::VerboseOutput vout{false};
    REQUIRE_THROWS_AS(Config("test/t/test-config-invalid-db.yaml", vout),
                      config_error);
    REQUIRE_THROWS_WITH(Config("test/t/test-config-invalid-db.yaml", vout),
                        "Config error: 'database' entry must be a Map.");
}

TEST_CASE("invalid yaml")
{
    osmium::VerboseOutput vout{false};
    REQUIRE_THROWS_AS(Config("test/t/test-config-invalid-yaml.yaml", vout),
                      YAML::Exception);
}
