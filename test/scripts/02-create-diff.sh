#!/bin/sh

set -e

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/testdata.sql

../src/osmdbt-get-log -c $TESTDIR/test-config.yaml --catchup

grep --quiet ' n10 v1 c1$' $TESTDIR/log/osm-repl-*.log
grep --quiet ' n11 v1 c1$' $TESTDIR/log/osm-repl-*.log
grep --quiet ' w20 v1 c1$' $TESTDIR/log/osm-repl-*.log

../src/osmdbt-create-diff -c $TESTDIR/test-config.yaml -s 42 -n

zgrep --quiet 'node id="10" version="1"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'node id="11" version="1"' $TESTDIR/tmp/new-change.osc.gz
zgrep --quiet 'way id="20" version="1"' $TESTDIR/tmp/new-change.osc.gz

