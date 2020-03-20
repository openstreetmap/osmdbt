
# NAME

osmdbt-get-log - Write changes from replication slot to log file


# SYNOPSIS

**osmdbt-get-log** \[*OPTIONS*\]


# DESCRIPTION

Get recent changes from the database and writes them into a log file in an
internal format which can be read by `osmdbt-create-diff`.


# OPTIONS

\--catchup
:   After reading the changes and committing them to disk, mark them as done.

@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-get-log** exits with exit code

0
  ~ if everything went alright,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

