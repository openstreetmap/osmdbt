
# NAME

osmdbt-create-diff - Read replication log and create OSM change file from it


# SYNOPSIS

**osmdbt-create-diff** \[*OPTIONS*\]


# DESCRIPTION

Read log files created by **osmdbt-get-log** and create on OSM change file
from it.

Reads all files specified with **-f, \--log-file** or all log files in
the `log_dir`. Sorts all objects in the files by type, id, and version and
writes them out as gzipped OSM change file (`.osc.gz`). It also creates an
updated state.txt file.

The sequence of actions in detail:

1. Create `RUN_DIR/osmdbt-create-diff.pid`. If the file already exists,
   end with an error.
2. Read state from `CHANGES_DIR/state.txt` or use the sequence number from
   the **-s, \--sequence`** option.
3. Read all log files specified using **-f, \--log-file** or found in the log
   directory. Only files with suffix `.log` are read.
4. Create a change file `TMP_DIR/new-changes.osc.gz` and a new state file
   `TMP_DIR/new-state.txt`. A copy of the state file is stored with the
   name `TMP_DIR/new-state.txt.copy`. All files are synced.
5. If the option **-n, --dry-run** was specified the processing is now done.
6. Create directory hierarchy under `CHANGES_DIR` as needed and move, in that
   order, the changes file and the state file into the directory hierarchy,
   sync the directory, then move the copy of the state file into
   `CHANGES_DIR/state.txt` and sync that directory.
7. Append `.done` to all log file names used. Sync log directory.
8. Remove the pid file and end.

# OPTIONS

-f, \--log-file=FILE
:   Name of a log file to be read. The names are relative to the `log_dir`
    specified in the config file. Can be specified multiple times. If this
    option is not used, all log files (with suffix `.log`) in the log_dir
    are read.

-m, \--max-changes=NUM
:   Maximum number of changes that will be written out. The actual number
    might be larger than this, because always complete log files are read.
    Default: no maximum.

-n, \--dry-run
:   Create updated state file and new change file in tmp directory, but do
    not move them into their final locations. The log files are also not
    renamed.

-s, \--sequence-number=NUM
:   Use sequence number NUM. Do not read `state.txt`.

@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-create-diff** exits with exit code

0
  ~ if everything went alright,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

