#!/bin/sh

set -e

. $SRCDIR/setup.sh

# Load some test data
psql --quiet <$SRCDIR/testdata.sql

../src/osmdbt-fake-log -c $TESTDIR/test-config.yaml -t 2020-01-01T00:00:00Z

grep --quiet ' n10 v1 c1$' $TESTDIR/log/osm-repl-*.log
grep --quiet ' n11 v1 c1$' $TESTDIR/log/osm-repl-*.log
grep --quiet ' w20 v1 c1$' $TESTDIR/log/osm-repl-*.log

../src/osmdbt-disable-replication -c $TESTDIR/test-config.yaml

