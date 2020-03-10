#!/bin/sh

set -e

if [ "x$PG_CLUSTER_CONF_ROOT" = "x" ]; then
    echo
    echo "X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X"
    echo
    echo "Not running under pg_virtualenv. See README.md"
    echo
    echo "X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X"
    echo
    exit 1
fi

SRCDIR=`dirname $0`
TESTDIR=`pwd`/td

mkdir -p $TESTDIR/changes $TESTDIR/log $TESTDIR/run $TESTDIR/tmp

cat >test-config.yaml <<EOF
---
database:
    host: $PGHOST
    port: $PGPORT
    dbname: $PGDATABASE
    user: $PGUSER
    password: $PGPASSWORD
    replication_slot: rs
log_dir: $TESTDIR/log
changes_dir: $TESTDIR/changes
tmp_dir: $TESTDIR/tmp
run_dir: $TESTDIR/run
EOF

psql --quiet --file=$SRCDIR/../../structure.sql

