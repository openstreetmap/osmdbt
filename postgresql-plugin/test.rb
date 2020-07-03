require 'rubygems'
require 'pg'
require 'securerandom'

def load_structure(conn)
  dir = File.dirname(__FILE__)
  File.open(File.join(dir, "../test/structure.sql")) do |fh|
    conn.exec(fh.read)
  end
end

def run_test(conn)
  # insert some basic data into the tables
  conn.exec(<<SQL)
INSERT INTO users
  (id, email, pass_crypt, creation_time, display_name, data_public)
values
  (1, 'user_1@example.com', '', '2013-11-14T02:10:00Z', 'user_1', true),
  (2, 'user_2@example.com', '', '2013-11-14T02:10:00Z', 'user_2', false);

INSERT INTO changesets
  (id, user_id, created_at, closed_at)
values
  (1, 1, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z'),
  (2, 1, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z'),
  (4, 2, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z');

INSERT INTO current_nodes
  (id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
values
  (1,       0,       0, 1, true,  '2013-11-14T02:10:00Z', 3221225472, 1),
  (2, 1000000, 1000000, 1, true,  '2013-11-14T02:10:01Z', 3221227032, 1),
  (3,       0,       0, 2, false, '2015-03-02T18:27:00Z', 3221225472, 2),
  (4,       0,       0, 4, true,  '2015-03-02T19:25:00Z', 3221225472, 1);

INSERT INTO nodes
  (node_id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
values
  (1,       0,       0, 1, true,  '2013-11-14T02:10:00Z', 3221225472, 1),
  (2, 1000000, 1000000, 1, true,  '2013-11-14T02:10:01Z', 3221227032, 1),
  (3,       0,       0, 2, false, '2015-03-02T18:27:00Z', 3221225472, 2),
  (4,       0,       0, 4, true,  '2015-03-02T19:25:00Z', 3221225472, 1);
SQL

  sleep 1 # sleep for a second to make sure changes are available
  res = conn.exec("SELECT * FROM pg_logical_slot_get_changes('replication_slot', NULL, NULL)")
  logical_data = res.map {|row| row['data']}
  expected = ['N n1 v1 c1', 'N n2 v1 c1', 'N n3 v2 c2', 'N n4 v1 c4', 'C']
  unless logical_data == expected
    raise Exception.new("Expected #{expected.inspect}, but got #{logical_data.inspect}")
  end
end

def run_test_with_replication(conn)
  # set up the logical replication slot
  res = conn.exec(<<SQL)
SELECT * FROM pg_create_logical_replication_slot('replication_slot', 'osm-logical');
SQL

  begin
    run_test(conn)

  ensure
    conn.exec(<<SQL)
SELECT pg_drop_replication_slot('replication_slot');
SQL
  end
end

nonce = SecureRandom.hex(3)
db_name = "osm_logical_test_#{nonce}"

db_conn = PG.connect("dbname=postgres")
db_conn.exec("SET client_min_messages TO WARNING")
db_conn.exec("DROP DATABASE IF EXISTS \"#{db_name}\"")
db_conn.exec("CREATE DATABASE \"#{db_name}\"")
db_conn.flush

conn = nil
begin
  conn = PG.connect("dbname=#{db_name}")

  load_structure(conn)

  run_test_with_replication(conn)

  puts "OK!"

ensure
  conn.finish unless conn.nil?
  db_conn.exec("DROP DATABASE \"#{db_name}\"")
end
