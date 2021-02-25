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

    const auto s = state.to_string();
    REQUIRE(s == "sequenceNumber=3921359\ntimestamp=2020-03-06T15\\:09\\:02Z\n");

    const auto sc = state.to_string(state.timestamp().seconds_since_epoch());
    REQUIRE(sc == "#Fri Mar 06 15:09:02 UTC 2020\nsequenceNumber=3921359\ntimestamp=2020-03-06T15\\:09\\:02Z\n");
}

TEST_CASE("Read valid minimal state file")
{
    State const state{"test/data/minimal.state.txt"};

    REQUIRE(state.sequence_number() == 3921359);
    REQUIRE(state.timestamp() == osmium::Timestamp{"2020-03-06T15:09:02Z"});

    const auto s = state.to_string();
    REQUIRE(s == "sequenceNumber=3921359\ntimestamp=2020-03-06T15\\:09\\:02Z\n");
}

TEST_CASE("Read valid minimal state file with unescaped timestamp")
{
    State const state{"test/data/sane-time.state.txt"};

    REQUIRE(state.sequence_number() == 3921359);
    REQUIRE(state.timestamp() == osmium::Timestamp{"2020-03-06T15:09:02Z"});

    const auto s = state.to_string();
    REQUIRE(s == "sequenceNumber=3921359\ntimestamp=2020-03-06T15\\:09\\:02Z\n");
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

    const auto s = state.to_string();
    REQUIRE(s == "sequenceNumber=1234\ntimestamp=2020-02-02T02\\:02\\:02Z\n");

    osmium::Timestamp comment_timestamp{"2021-01-01T01:23:45Z"};
    state.write(TEST_DIR "/some-state.txt", comment_timestamp.seconds_since_epoch());

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
    REQUIRE(State{1, ts}.osc_path() == "000/000/001.osc");
    REQUIRE(State{1, ts}.dir1_path() == "000");
    REQUIRE(State{1, ts}.dir2_path() == "000/000");
    REQUIRE(State{123456789, ts}.state_path() == "123/456/789.state.txt");
    REQUIRE(State{123456789, ts}.osc_path() == "123/456/789.osc");
    REQUIRE(State{123456789, ts}.dir1_path() == "123");
    REQUIRE(State{123456789, ts}.dir2_path() == "123/456");
}
