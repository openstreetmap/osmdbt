BEGIN;

INSERT INTO nodes (node_id, version, changeset_id, latitude, longitude, visible, "timestamp", tile)
    VALUES (10, 1, 1, 10000000, 20000000, true, '2020-02-20T20:20:20Z', 0),
           (11, 1, 1, 11000000, 21000000, true, '2020-02-20T20:20:20Z', 0),
           (12, 1, 1, 12000000, 22000000, true, '2020-02-20T20:20:20Z', 0),
           (10, 2, 2, 10100000, 20100000, true, '2020-02-20T20:20:22Z', 0),
           (11, 2, 2, 11100000, 21100000, true, '2020-02-20T20:20:22Z', 0),
           (12, 2, 2, 12000000, 22000000, false, '2020-02-20T20:20:22Z', 0);

INSERT INTO node_tags (node_id, version, k, v)
    VALUES (11, 1, 'barrier', 'gate'),
           (11, 2, 'barrier', 'gate'),
           (12, 1, 'amenity', 'pub'),
           (12, 1, 'name', 'The Kings Head');

INSERT INTO ways (way_id, version, changeset_id, visible, "timestamp")
    VALUES (20, 1, 1, true, '2020-02-20T20:20:20Z'),
           (21, 1, 1, true, '2020-02-20T20:20:20Z'),
           (21, 2, 2, true, '2020-02-20T20:20:22Z'),
           (22, 1, 1, true, '2020-02-20T20:20:20Z'),
           (22, 2, 2, false, '2020-02-20T20:20:22Z');

INSERT INTO way_nodes (way_id, version, sequence_id, node_id)
    VALUES (20, 1, 0, 10),
           (20, 1, 1, 11),
           (21, 1, 0, 11),
           (21, 1, 1, 12),
           (21, 2, 0, 11),
           (21, 2, 1, 12),
           (22, 1, 0, 12),
           (22, 1, 1, 10);

INSERT INTO way_tags (way_id, version, k, v)
    VALUES (20, 1, 'highway', 'primary'),
           (20, 1, 'name', 'High Street'),
           (21, 1, 'highway', 'secondary'),
           (21, 2, 'highway', 'tertiary'),
           (22, 1, 'name', 'School Street'),
           (22, 1, 'highway', 'residential');

INSERT INTO relations (relation_id, version, changeset_id, timestamp, visible)
    VALUES (30, 1, 1, '2020-02-20T20:20:20Z', true),
           (30, 2, 2, '2020-02-20T20:20:22Z', true),
           (31, 1, 1, '2020-02-20T20:20:20Z', true),
           (31, 2, 2, '2020-02-20T20:20:22Z', true),
           (32, 1, 1, '2020-02-20T20:20:20Z', true),
           (32, 2, 2, '2020-02-20T20:20:22Z', false);

INSERT INTO relation_members (relation_id, version, sequence_id, member_type, member_id, member_role)
    VALUES (30, 1, 0, 'Way',  20, ''),
           (30, 1, 1, 'Node', 10, 'abc'),
           (30, 1, 2, 'Node', 11, ''),
           (30, 2, 2, 'Node', 11, 'abc'),
           (30, 2, 1, 'Node', 10, ''),
           (30, 2, 0, 'Way',  20, ''),
           (31, 1, 0, 'Way',  21, 'foo'),
           (31, 1, 1, 'Way',  21, ''),
           (32, 1, 0, 'Way',  22, '');

INSERT INTO relation_tags (relation_id, version, k, v)
    VALUES (30, 1, 'type', 'route'),
           (30, 1, 'ref', '373'),
           (31, 1, 'type', 'multipolygon'),
           (31, 1, 'natural', 'wood'),
           (31, 2, 'type', 'multipolygon'),
           (31, 2, 'landuse', 'forest');

COMMIT;
