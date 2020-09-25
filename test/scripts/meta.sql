BEGIN;

INSERT INTO users (id, email, pass_crypt, creation_time, display_name)
    VALUES (1, 'user1@example.com', 'xxx', '2020-02-20T00:00:00Z', 'testuser1'),
           (2, 'user2@example.com', 'xxx', '2020-02-20T00:00:00Z', 'testuser2');

INSERT INTO changesets (id, user_id, created_at, closed_at)
    VALUES (1, 1, '2020-02-20T20:20:20Z', '2020-02-20T20:20:21Z'),
           (2, 2, '2020-02-20T20:20:22Z', '2020-02-20T20:20:23Z');

INSERT INTO redactions (id, user_id) VALUES (10, 1);

COMMIT;
