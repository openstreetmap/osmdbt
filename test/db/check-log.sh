#!/bin/sh

set -e

grep --quiet ' n10 v1 c1$' td/log/osm-repl-*.log
grep --quiet ' n11 v1 c1$' td/log/osm-repl-*.log
grep --quiet ' w20 v1 c1$' td/log/osm-repl-*.log

# Remove files left over from previous runs
rm -f td/log/*.done
rm -f td/tmp/new-state.txt.copy td/tmp/new-state.txt td/tmp/new-change.osc.gz

# Remove fake log file from earlier test
rm -f td/log/osm-repl-????-??-??T??:??:??Z-2*Z.log
