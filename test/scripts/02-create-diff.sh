#!/bin/sh

set -e

. $SRCDIR/setup.sh

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

../src/osmdbt-create-diff --config=$CONFIG --sequence-number=42 --dry-run

zgrep --quiet 'node id="10" version="1"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'node id="11" version="1"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'way id="20" version="1"' $TESTDIR/tmp/new-change.osc.gz

