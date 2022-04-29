#!/bin/bash
#
#  Test osmdbt-create-diff command with --max-changes
#

set -e
set -x

. "$SRCDIR/setup.sh"

# Load some test data
psql --quiet <"$SRCDIR/meta.sql"
psql --quiet <"$SRCDIR/testdata.sql"

cat >"$TESTDIR/changes/state.txt" <<"EOF"
sequenceNumber=23
timestamp=2020-01-01T01\:02\:03Z
EOF

../src/osmdbt-get-log --config="$CONFIG" --catchup

# More test data
psql --quiet <"$SRCDIR/testdata-more.sql"

../src/osmdbt-get-log --config="$CONFIG" --catchup

../src/osmdbt-create-diff --config="$CONFIG" --max-changes=3

# State files must be identical
cmp "$TESTDIR/changes/state.txt" "$TESTDIR/changes/000/000/024.state.txt"

# Check contents of state files
grep --quiet '^sequenceNumber=24$' "$TESTDIR/changes/state.txt"
grep --quiet '^timestamp=' "$TESTDIR/changes/state.txt"

# Check contents of change file
OSC="$TESTDIR/changes/000/000/024.osc.gz"
zgrep --quiet 'node id="10" version="1"' "$OSC"
zgrep --quiet 'node id="11" version="1"' "$OSC"
zgrep --quiet 'way id="20" version="1"' "$OSC"
zgrep --quiet 'relation id="30" version="1"' "$OSC"

zgrep --quiet --invert-match 'node id="12" version="1"' "$OSC"
zgrep --quiet --invert-match 'node id="13" version="1"' "$OSC"
zgrep --quiet --invert-match 'way id="21" version="1"' "$OSC"

# There should be exactly two log files
test $(ls -1 "$TESTDIR/log" | wc -l) -eq 2

# There should be exactly one done log file
test $(ls -1 "$TESTDIR/log" | grep done | wc -l) -eq 1

# Determine name of done log file
LOGFILE=$(ls $TESTDIR/log/*.log.done)

# There should be 7 files in the test directory (config, 2xstate, 2xchange, 2xlog)
test $(find "$TESTDIR" -type f | wc -l) -eq 7

