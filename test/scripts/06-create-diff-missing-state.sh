#!/bin/sh
#
#  Test osmdbt-create-diff command with missing state file
#

set -e

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/testdata.sql

../src/osmdbt-get-log --config=$CONFIG --catchup

# If there is no state file osmdbt-create-diff must fail
test_exit 2 ../src/osmdbt-create-diff --config=$CONFIG

# Empty state file, still fail
touch $TESTDIR/changes/state.txt
test_exit 2 ../src/osmdbt-create-diff --config=$CONFIG

# Missing sequenceNumber, still fail
cat >$TESTDIR/changes/state.txt <<"EOF"
timestamp=2020-01-01T01\:02\:03Z
EOF
test_exit 2 ../src/osmdbt-create-diff --config=$CONFIG

