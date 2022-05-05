#!/bin/bash
#
#  Test normal use of osmdbt-get-log command
#

set -e
set -x

. "$SRCDIR/setup.sh"

# Test that replication is enabled and there are no changes
../src/osmdbt-testdb -c "$CONFIG" 2>&1 | grep "There are no changes in your configured replication slot."

# If there is no data osmdbt-get-log does nothing
../src/osmdbt-get-log --config="$CONFIG"

# Load some test data
psql --quiet <"$SRCDIR/meta.sql"
psql --quiet <"$SRCDIR/testdata.sql"

# Test that replication is enabled and there are some changes
../src/osmdbt-testdb -c "$CONFIG" 2>&1 | grep "There are 7 changes in your configured replication slot."

# Reading log without catchup
../src/osmdbt-get-log --config="$CONFIG"

# There should be exactly one log file
test $(ls -1 "$TESTDIR/log" | wc -l) -eq 1

# Determine name of log file
LOGFILE="$TESTDIR/log/"$(ls "$TESTDIR/log")

# Check content of log file
test $(wc -l <"$LOGFILE") -eq 7
grep --quiet ' n10 v1 c1$' "$LOGFILE"
grep --quiet ' n11 v1 c1$' "$LOGFILE"
grep --quiet ' n10 v2 c2$' "$LOGFILE"
grep --quiet ' n11 v2 c2$' "$LOGFILE"
grep --quiet ' w20 v1 c1$' "$LOGFILE"
grep --quiet ' r30 v1 c1$' "$LOGFILE"

rm "$LOGFILE"

# Reading log again with catchup
../src/osmdbt-get-log --config="$CONFIG" --catchup

# There should be exactly one log file
test $(ls -1 "$TESTDIR/log" | wc -l) -eq 1

# Determine name of log file
LOGFILE="$TESTDIR/log/"$(ls "$TESTDIR/log")

# Check content of log file
test $(wc -l <"$LOGFILE") -eq 7
grep --quiet ' n10 v1 c1$' "$LOGFILE"
grep --quiet ' n11 v1 c1$' "$LOGFILE"
grep --quiet ' n10 v2 c2$' "$LOGFILE"
grep --quiet ' n11 v2 c2$' "$LOGFILE"
grep --quiet ' w20 v1 c1$' "$LOGFILE"
grep --quiet ' r30 v1 c1$' "$LOGFILE"

../src/osmdbt-disable-replication --config="$CONFIG"

