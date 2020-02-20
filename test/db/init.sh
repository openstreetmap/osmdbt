#!/bin/sh

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
TESTDIR=`pwd`

cat >test-config.yaml <<EOF
---
database:
    host: $PGHOST
    port: $PGPORT
    dbname: $PGDATABASE
    user: $PGUSER
    password: $PGPASSWORD
    replication_slot: rs
log_dir: $TESTDIR
changes_dir: $TESTDIR
run_dir: $TESTDIR
EOF

psql <$SRCDIR/../../structure.sql

