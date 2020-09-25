BEGIN;

INSERT INTO nodes (node_id, version, changeset_id, latitude, longitude, "timestamp", tile, visible)
    VALUES (12, 1, 1, 10000000, 20000000, '2020-02-20T20:20:20Z', 0, true),
           (13, 1, 1, 11000000, 21000000, '2020-02-20T20:20:20Z', 0, true);

INSERT INTO ways (way_id, version, changeset_id, "timestamp", visible)
    VALUES (21, 1, 1, '2020-02-20T20:21:20Z', true);

INSERT INTO way_nodes (way_id, version, sequence_id, node_id)
    VALUES (21, 1, 0, 12),
           (21, 1, 1, 13);

INSERT INTO way_tags (way_id, version, k, v)
    VALUES (21, 1, 'highway', 'primary'),
           (21, 1, 'name', 'High Street');

COMMIT;
