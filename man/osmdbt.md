
# NAME

osmdbt - Tools for creating replication feeds from the main OSM database


# DESCRIPTION

Various tools for handling replication of OpenStreetMap data. All changes in
the main OSM database can be read regularly and dumped out into OSM change
files.


# COMMANDS

osmdbt-catchup
:   Mark changes in the PostgreSQL replication slot up to the specified LSN
    as done.

osmdbt-create-diff
:   Read replication log files created by `osmdbt-get-log` or `osmdbt-fake-log`
    and create an OSM change file.

osmdbt-disable-replication
:   Disable replication on the database.

osmdbt-enable-replication
:   Enable replication on the database.

osmdbt-fake-log
:   Create fake log file from recent changes.

osmdbt-get-log
:   Get recent changes from the database replication slot and write them into
    a log file in an internal format which can be read by
    `osmdbt-create-diff`.

osmdbt-testdb
:   Check database connection and print PostgreSQL and schema version
    and information about active replication slots.


# CONFIG FILE

All commands use a config file in YAML format. You can specify the name of the
config file with the `-c` or `\--config` command line option. By default the
file `osmdbt-config.yaml` is looked for in the current directory and in the
directory `/etc/replication`.

The config file contains the following settings:

* `database`: Information for connecting to the database:
    - `host`: Name of the host running the database (default: empty)
    - `port`: TCP port to connect to (default: 5432)
    - `dbname`: Name of the database (default: `osm`)
    - `user`: Database user (default: `osm`)
    - `password`: Password of database user (default: `osm`)
    - `replication_slot`: Name of logical decoding replication slot
      (default: `rs`)
* `log_dir`: The directory where `osmdbt-get-log` writes the log files and
  `osmdbt-create-diff` reads the log files from.
  (default: `/tmp`)
* `changes_dir`: The directory where `osmdbt-create-diff` writes the change
  files (default: `/tmp`)
* `tmp_dir`: Temporary directory used by `osmdbt-create-diff`. Must be on the
  same file system as `changes_dir`.
* `run_dir`: The directory where the commands store pid/lock files. This can
  be on a temporary filesystem like `/var/run`.
  (default: `/tmp`)


# REPLICATION LOG

The database writes a replication log using the `osm-logical` plugin.

The log has one entry per line and the following fields separated by spaces:

1. LSN (Log Sequence Number) looks something like `16/B374D848`. See
   https://www.postgresql.org/docs/current/datatype-pg-lsn.html for details.
2. xid
3. Action (`C` for "commit transaction" or `N` for an actual change)

After that, for actual changes, there is the object type ('n', 'w', or 'r'),
its id, version, and changeset.

Example:

    C/AAA1A108 59940 N n7204175493 v1 c80864722
    C/AAAF35F8 59940 N w771706111 v1 c80864722
    C/AAAF39D0 59940 N r104862 v21 c80864688

There are always zero or more `N` entries followed by one `C` entry commiting
the transaction.

If an error happened parsing the information from the database, the action
will be set to `X` and an error message added to the line. This should never
happen.

The files created by osmdbt-get-log and osmdbt-fake-log have the same format,
but the LSN is `0/0` and xid is `0`. (Use `-s` on osmdbt-get-log to get the
full data.)


# RECOVERY PROCEDURE

If the replication breaks for some reason these are the steps you have to
take to get back on track:

* Stop writing to the database and make sure all transactions are done.
* Disable any cron job that runs any of the `osmdbt-*` commands.
* Clean up any broken log or other files that might be left over from
  earlier runs.
* Run `osmdbt-disable-replication`.
* Run `osmdbt-fake-log`. Set the `-t` option to a point in time that's
  before the break, any changes before that time will not be in the output.
  Use the `-l` option (possible multiple times) with the most recent log
  files (`.log` or `.log.done`) to make sure all entries in those log files
  are recognized as already done.
* Run `osmdbt-create-diff`.
* Run `osmdbt-enable-replication`.
* Re-enable cron jobs.
* You can now enable writing to the database again.


# SEE ALSO

* **osmdbt-catchup**(1),
  **osmdbt-create-diff**(1),
  **osmdbt-disable-replication**(1),
  **osmdbt-enable-replication**(1),
  **osmdbt-fake-log**(1),
  **osmdbt-get-log**(1),
  **osmdbt-testdb**(1),

