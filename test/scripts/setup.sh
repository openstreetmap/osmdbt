#!/bin/sh

set -e

test_exit() {
    local rc=$1
    shift
    if $*; then
        false
    else
        test $? -eq $rc
    fi
}

rm -fr $TESTDIR
mkdir -p $TESTDIR/changes $TESTDIR/log $TESTDIR/run $TESTDIR/tmp

export CONFIG=$TESTDIR/test-config.yaml

# Create special config file for this test
# 'envsubst' from package 'gettext-base'
envsubst <$SRCDIR/test-config.yaml.tmpl >$CONFIG

# Initialize database schema
psql --quiet --file=$SRCDIR/../structure.sql

# Test database access
../src/osmdbt-testdb -c $CONFIG

# Enable replication on the database
../src/osmdbt-enable-replication -c $CONFIG

