#!/bin/sh
#
#  Test that osmdbt-get-log, osmdbt-fake-log, and osmdbt-catchup will not
#  start if their pid file is already there.
#

set -e

. $SRCDIR/setup.sh

echo "fake pid file" >$TESTDIR/run/osmdbt-log.pid

test_exit 2 ../src/osmdbt-get-log  --config=$CONFIG
test_exit 2 ../src/osmdbt-fake-log --config=$CONFIG --timestamp=2020-01-01T00:00:00Z
test_exit 2 ../src/osmdbt-catchup  --config=$CONFIG

