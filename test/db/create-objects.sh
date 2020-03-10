#!/bin/sh

set -e

# Remove replication logs from previous runs
rm -f td/log/*.log td/log/*.done

psql --quiet <<"EOF"

BEGIN;

INSERT INTO users (id, email, pass_crypt, creation_time, display_name)
    VALUES (1, 'user@example.com', 'xxx', '2020-02-20T00:00:00Z', 'testuser');

INSERT INTO changesets (id, user_id, created_at, closed_at)
    VALUES (1, 1, '2020-02-20T20:20:20Z', '2020-02-20T20:20:21Z');

INSERT INTO nodes (node_id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
    VALUES (10, 10000000, 20000000, 1, true, '2020-02-20T20:20:20Z', 0, 1),
           (11, 11000000, 21000000, 1, true, '2020-02-20T20:20:20Z', 0, 1);

INSERT INTO ways (way_id, changeset_id, visible, "timestamp", version)
    VALUES (20, 1, true, '2020-02-20T20:20:20Z', 1);

INSERT INTO way_nodes (way_id, version, sequence_id, node_id)
    VALUES (20, 1, 0, 10),
           (20, 1, 1, 11);

INSERT INTO way_tags (way_id, version, k, v)
    VALUES (20, 1, 'highway', 'primary'),
           (20, 1, 'name', 'High Street');

COMMIT;

EOF

