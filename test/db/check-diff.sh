#!/bin/sh

set -e

zgrep --quiet 'node id="10" version="1"' td/tmp/new-change.osc.gz
zgrep --quiet 'node id="11" version="1"' td/tmp/new-change.osc.gz
zgrep --quiet 'way id="20" version="1"' td/tmp/new-change.osc.gz

