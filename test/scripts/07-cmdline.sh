#!/bin/sh
#
#  Test some command line options
#

set -e

. $SRCDIR/setup.sh

for cmd in catchup create-diff disable-replication enable-replication fake-log get-log testdb; do
    ../src/osmdbt-$cmd -h | grep --quiet '^Usage'
    ../src/osmdbt-$cmd --help | grep --quiet '^Usage'
    test_exit 3 ../src/osmdbt-$cmd --unknown
done

for cmd in catchup create-diff disable-replication enable-replication get-log testdb; do
    test_exit 2 ../src/osmdbt-$cmd --config=does-not-exist
done

