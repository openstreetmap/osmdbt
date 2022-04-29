#!/bin/bash
#
#  Test use of osmdbt-fake-log command creating multiple log files
#

set -e
set -x

. "$SRCDIR/setup.sh"

# Load some test data
psql --quiet <"$SRCDIR/meta.sql"
psql --quiet <"$SRCDIR/testdata-multi.sql"

../src/osmdbt-fake-log --config="$CONFIG" --timestamp=2020-01-01T00:00:12Z

# There should be exactly two files
test `ls -1 "$TESTDIR/log" | wc -l` -eq 2

# Determine name of first log file
LOGFILE1="$TESTDIR/log/"`ls -1 "$TESTDIR/log" | head -n1`

# Determine name of second log file
LOGFILE2="$TESTDIR/log/"`ls -1 "$TESTDIR/log" | tail -n1`

# Check content of log files
test `wc -l <"$LOGFILE1"` -eq 3
test `wc -l <"$LOGFILE2"` -eq 3

grep --quiet '^0/0 0 N n10 v1 c1$' "$LOGFILE1"
grep --quiet '^0/0 0 N n11 v1 c1$' "$LOGFILE1"
grep --quiet '^0/0 0 N n10 v2 c2$' "$LOGFILE2"
grep --quiet '^0/0 0 N n11 v2 c2$' "$LOGFILE2"
grep --quiet '^0/0 0 N w20 v1 c1$' "$LOGFILE1"
grep --quiet '^0/0 0 N r30 v1 c1$' "$LOGFILE2"

../src/osmdbt-disable-replication --config="$CONFIG"

