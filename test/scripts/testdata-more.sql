BEGIN;

INSERT INTO nodes (node_id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
    VALUES (12, 10000000, 20000000, 1, true, '2020-02-20T20:20:20Z', 0, 1),
           (13, 11000000, 21000000, 1, true, '2020-02-20T20:20:20Z', 0, 1);

INSERT INTO ways (way_id, changeset_id, visible, "timestamp", version)
    VALUES (21, 1, true, '2020-02-20T20:21:20Z', 1);

INSERT INTO way_nodes (way_id, version, sequence_id, node_id)
    VALUES (21, 1, 0, 12),
           (21, 1, 1, 13);

INSERT INTO way_tags (way_id, version, k, v)
    VALUES (21, 1, 'highway', 'primary'),
           (21, 1, 'name', 'High Street');

COMMIT;
