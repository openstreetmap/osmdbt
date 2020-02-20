#!/bin/sh

# Remove replication logs from previous runs
rm osm-repl-*.log

psql <<"EOF"

INSERT INTO users (id, email, pass_crypt, creation_time, display_name)
    VALUES (1, 'user@example.com', 'xxx', '2020-02-20T00:00:00Z', 'testuser');

INSERT INTO changesets (id, user_id, created_at, closed_at)
    VALUES (1, 1, '2020-02-20T20:20:20Z', '2020-02-20T20:20:21Z');

INSERT INTO nodes (node_id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
    VALUES (1, 1.0, 2.0, 1, true, '2020-02-20T20:20:20Z', 0, 1);

EOF

