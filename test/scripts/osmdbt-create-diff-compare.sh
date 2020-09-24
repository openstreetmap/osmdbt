#!/bin/sh
#
#  Test osmdbt-create-diff command and compare result
#

set -e
set -x

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/meta.sql
psql --quiet <$SRCDIR/moredata.sql

../src/osmdbt-get-log --config=$CONFIG --catchup

../src/osmdbt-create-diff --config=$CONFIG --sequence-number=42

zcat $TESTDIR/changes/000/000/042.osc.gz >$TESTDIR/changes/000/000/042.osc

diff -u $SRCDIR/osmdbt-create-diff-compare.osc $TESTDIR/changes/000/000/042.osc

