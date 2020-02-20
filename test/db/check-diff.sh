#!/bin/sh

set -e

zgrep 'node id="10" version="1"' osm-repl.osc.gz
zgrep 'node id="11" version="1"' osm-repl.osc.gz
zgrep 'way id="20" version="1"' osm-repl.osc.gz

