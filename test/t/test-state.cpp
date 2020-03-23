#include <catch.hpp>

#include "state.hpp"

#include <osmium/osm/timestamp.hpp>

#include <string>

TEST_CASE("Read empty state file")
{
    REQUIRE_THROWS_WITH(
        State{"test/data/empty.state.txt"},
        "Missing sequenceNumber in state file 'test/data/empty.state.txt'");
}

TEST_CASE("Read valid state file")
{
    State const state{"test/data/359.state.txt"};

    REQUIRE(state.sequence_number() == 3921359);
    REQUIRE(state.timestamp() == osmium::Timestamp{"2020-03-06T15:09:02Z"});
}

TEST_CASE("Read valid minimal state file")
{
    State const state{"test/data/minimal.state.txt"};

    REQUIRE(state.sequence_number() == 3921359);
    REQUIRE(state.timestamp() == osmium::Timestamp{"2020-03-06T15:09:02Z"});
}

TEST_CASE("Read valid minimal state file with unescaped timestamp")
{
    State const state{"test/data/sane-time.state.txt"};

    REQUIRE(state.sequence_number() == 3921359);
    REQUIRE(state.timestamp() == osmium::Timestamp{"2020-03-06T15:09:02Z"});
}

TEST_CASE("Fail on non-existing state file")
{
    REQUIRE_THROWS_WITH(
        State{"test/data/does-not-exist.state.txt"},
        "Could not open state file 'test/data/does-not-exist.state.txt': No "
        "such file or directory");
}

TEST_CASE("Write specified state and read it again")
{
    State const state{1234, osmium::Timestamp{"2020-02-02T02:02:02Z"}};
    REQUIRE(state.sequence_number() == 1234);
    REQUIRE(state.timestamp() == osmium::Timestamp{"2020-02-02T02:02:02Z"});

    state.write(TEST_DIR "/some-state.txt");

    State const state2{TEST_DIR "/some-state.txt"};
    REQUIRE(state2.sequence_number() == 1234);
    REQUIRE(state2.timestamp() == osmium::Timestamp{"2020-02-02T02:02:02Z"});

    // can not write a second time, because file now exists
    REQUIRE_THROWS(state.write(TEST_DIR "/some-state.txt"));
}

TEST_CASE("Calculate next state")
{
    State const state{7878, osmium::Timestamp{"2020-02-02T02:02:00Z"}};
    auto const next_state =
        state.next(osmium::Timestamp{"2020-02-02T02:03:00Z"});
    REQUIRE(next_state.sequence_number() == 7879);
    REQUIRE(next_state.timestamp() ==
            osmium::Timestamp{"2020-02-02T02:03:00Z"});
}

TEST_CASE("Get state file path")
{
    osmium::Timestamp const ts{"2019-09-21T09:30:00Z"};
    REQUIRE(State{1, ts}.state_path() == "000/000/001.state.txt");
    REQUIRE(State{1, ts}.osc_path() == "000/000/001.osc.gz");
    REQUIRE(State{1, ts}.dir1_path() == "000");
    REQUIRE(State{1, ts}.dir2_path() == "000/000");
    REQUIRE(State{123456789, ts}.state_path() == "123/456/789.state.txt");
    REQUIRE(State{123456789, ts}.osc_path() == "123/456/789.osc.gz");
    REQUIRE(State{123456789, ts}.dir1_path() == "123");
    REQUIRE(State{123456789, ts}.dir2_path() == "123/456");
}
