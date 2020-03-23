#!/bin/sh
#
#  Test osmdbt-create-diff command in dry-run mode
#

set -e
set -x

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/meta.sql
psql --quiet <$SRCDIR/testdata.sql

../src/osmdbt-get-log --config=$CONFIG --catchup

../src/osmdbt-create-diff --config=$CONFIG --sequence-number=42 --dry-run

zgrep --quiet 'node id="10" version="1"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'node id="11" version="1"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'node id="10" version="2"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'node id="11" version="2"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'way id="20" version="1"'  $TESTDIR/tmp/new-change.osc.gz

