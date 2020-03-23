#!/bin/sh
#
#  Test normal use of osmdbt-fake-log command
#

set -e
set -x

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/meta.sql
psql --quiet <$SRCDIR/testdata.sql

../src/osmdbt-fake-log --config=$CONFIG --timestamp=2020-01-01T00:00:00Z

# There should be exactly one log file
test `ls -1 $TESTDIR/log | wc -l` -eq 1

# Determine name of log file
LOGFILE=$TESTDIR/log/`ls $TESTDIR/log`

# Check content of log file
test `wc -l <$LOGFILE` -eq 5
grep --quiet '^0/0 0 N n10 v1 c1$' $LOGFILE
grep --quiet '^0/0 0 N n11 v1 c1$' $LOGFILE
grep --quiet '^0/0 0 N n10 v2 c2$' $LOGFILE
grep --quiet '^0/0 0 N n11 v2 c2$' $LOGFILE
grep --quiet '^0/0 0 N w20 v1 c1$' $LOGFILE

../src/osmdbt-disable-replication --config=$CONFIG

