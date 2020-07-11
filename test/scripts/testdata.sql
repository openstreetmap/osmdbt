BEGIN;

INSERT INTO nodes (node_id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
    VALUES (10, 10000000, 20000000, 1, true, '2020-02-20T20:20:20Z', 0, 1),
           (11, 11000000, 21000000, 1, true, '2020-02-20T20:20:20Z', 0, 1),
           (10, 10000001, 20000001, 2, true, '2020-02-20T20:20:21Z', 0, 2),
           (11, 11000001, 21000001, 2, true, '2020-02-20T20:20:21Z', 0, 2);

INSERT INTO ways (way_id, changeset_id, visible, "timestamp", version)
    VALUES (20, 1, true, '2020-02-20T20:21:20Z', 1);

INSERT INTO way_nodes (way_id, version, sequence_id, node_id)
    VALUES (20, 1, 0, 10),
           (20, 1, 1, 11);

INSERT INTO way_tags (way_id, version, k, v)
    VALUES (20, 1, 'highway', 'primary'),
           (20, 1, 'name', 'High Street');

INSERT INTO relations (relation_id, changeset_id, timestamp, version, visible)
    VALUES (30, 1, '2020-02-20T20:20:30Z', 1, true);

INSERT INTO relation_members (relation_id, member_type, member_id, member_role, version, sequence_id)
    VALUES (30, 'Way',  20, '', 1, 1),
           (30, 'Node', 10, '', 1, 2),
           (30, 'Node', 11, '', 1, 3);

INSERT INTO relation_tags (relation_id, version, k, v)
    VALUES (30, 1, 'type', 'route'),
           (30, 1, 'ref',  '373');
COMMIT;
