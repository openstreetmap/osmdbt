#!/bin/sh
#
#  Test osmdbt-create-diff command and compare result
#

set -e
set -x

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/meta.sql
psql --quiet <$SRCDIR/osmdbt-create-diff-compare.sql

../src/osmdbt-get-log --config=$CONFIG --catchup

../src/osmdbt-create-diff --config=$CONFIG --sequence-number=42 --with-pbf-output

CHANGE_FILE=$TESTDIR/changes/000/000/042.osc

zcat $CHANGE_FILE.gz >$CHANGE_FILE

diff -u $SRCDIR/osmdbt-create-diff-compare.osc $CHANGE_FILE

