#!/bin/sh
#
#  Test normal use of osmdbt-get-log command
#

set -e
set -x

. $SRCDIR/setup.sh

# If there is no data osmdbt-get-log does nothing
../src/osmdbt-get-log --config=$CONFIG

# Load some test data
psql --quiet <$SRCDIR/meta.sql
psql --quiet <$SRCDIR/testdata-2.sql

# Reading log without catchup
../src/osmdbt-get-log --config=$CONFIG --max-changes=2

# There should be exactly one log file
test `ls -1 $TESTDIR/log | wc -l` -eq 1

# Determine name of log file
LOGFILE=$TESTDIR/log/`ls $TESTDIR/log`

# Check content of log file
test `wc -l <$LOGFILE` -eq 5
grep --quiet ' n10 v1 c1$' $LOGFILE
grep --quiet ' n11 v1 c1$' $LOGFILE
grep --quiet ' n10 v2 c2$' $LOGFILE
grep --quiet ' n11 v2 c2$' $LOGFILE

rm $LOGFILE

# Reading log again with catchup
../src/osmdbt-get-log --config=$CONFIG --catchup --max-changes=2

# There should be exactly one log file
test `ls -1 $TESTDIR/log | wc -l` -eq 1

# Determine name of log file
LOGFILE=$TESTDIR/log/`ls $TESTDIR/log`

# Check content of log file
test `wc -l <$LOGFILE` -eq 5
grep --quiet ' n10 v1 c1$' $LOGFILE
grep --quiet ' n11 v1 c1$' $LOGFILE
grep --quiet ' n10 v2 c2$' $LOGFILE
grep --quiet ' n11 v2 c2$' $LOGFILE

rm $LOGFILE

# Reading another batch
../src/osmdbt-get-log --config=$CONFIG --catchup --max-changes=2

# There should be exactly one log file
test `ls -1 $TESTDIR/log | wc -l` -eq 1

# Determine name of log file
LOGFILE=$TESTDIR/log/`ls $TESTDIR/log`

# Check content of log file
test `wc -l <$LOGFILE` -eq 2
grep --quiet ' w20 v1 c1$' $LOGFILE

../src/osmdbt-disable-replication --config=$CONFIG

