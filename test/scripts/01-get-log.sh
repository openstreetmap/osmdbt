#!/bin/sh

set -e

. $SRCDIR/setup.sh

../src/osmdbt-get-log --config=$CONFIG && true

# Load some test data
psql --quiet <$SRCDIR/testdata.sql

../src/osmdbt-get-log --config=$CONFIG --catchup

# There should be exactly one log file
test `ls -1 $TESTDIR/log | wc -l` -eq 1

# Determine name of log file
LOGFILE=$TESTDIR/log/`ls $TESTDIR/log`

# Check content of log file
test `wc -l <$LOGFILE` -eq 5
grep --quiet ' n10 v1 c1$' $LOGFILE
grep --quiet ' n11 v1 c1$' $LOGFILE
grep --quiet ' w20 v1 c1$' $LOGFILE

../src/osmdbt-disable-replication --config=$CONFIG

