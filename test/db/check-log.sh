#!/bin/sh

set -e

grep ' n10 v1 c1$' osm-repl-*.log
grep ' n11 v1 c1$' osm-repl-*.log
grep ' w20 v1 c1$' osm-repl-*.log

# Remove files left over from previous runs
rm -f osm-repl.log osm-repl.osc.gz osm-repl.osc.gz.new

# Make the replication log available under a static name
ln -s osm-repl-*.log osm-repl.log

