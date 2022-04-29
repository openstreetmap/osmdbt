#!/bin/bash
#
#  Test osmdbt-create-diff command with a state file
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

../src/osmdbt-create-diff --config="$CONFIG"

# State files must be identical
cmp "$TESTDIR/changes/state.txt" "$TESTDIR/changes/000/000/024.state.txt"

# Check contents of state files
grep --invert-match --quiet '^#' "$TESTDIR/changes/state.txt"
grep --quiet '^sequenceNumber=24$' "$TESTDIR/changes/state.txt"
grep --quiet '^timestamp=' "$TESTDIR/changes/state.txt"

# Check contents of change file
zgrep --quiet 'node id="10" version="1"' "$TESTDIR/changes/000/000/024.osc.gz"
zgrep --quiet 'node id="11" version="1"' "$TESTDIR/changes/000/000/024.osc.gz"
zgrep --quiet 'way id="20" version="1"'  "$TESTDIR/changes/000/000/024.osc.gz"
zgrep --quiet 'relation id="30" version="1"'  "$TESTDIR/changes/000/000/024.osc.gz"

# There should be exactly one done log file
test $(ls -1 "$TESTDIR/log" | wc -l) -eq 1

# Determine name of done log file
LOGFILE=$(ls "$TESTDIR/log")

# Log file should have suffix ".log.done"
test ${LOGFILE%.log.done}.log.done = $LOGFILE

# There should be 6 files in the test directory (config, 2xstate, 2xchange, log)
test $(find "$TESTDIR" -type f | wc -l) -eq 6

