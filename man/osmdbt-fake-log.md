
# NAME

osmdbt-fake-log - Create fake log file from recent changes


# SYNOPSIS

**osmdbt-fake-log** \[*OPTIONS*\]


# DESCRIPTION

Get recent changes from the database not by looking at the replication log but
at the changes since a specified point in time. Writes them into a log file in
the same format as **osmdbt-get-log**.

This can be used if something breaks with the replication and you need to
set everything up from a specific point in time.


# OPTIONS

-t, \--timestamp=TIMESTAMP
:   All changes after this point in time will be reported in the log (required).


@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-fake-log** exits with exit code

0
  ~ if there were changes and everything went alright,

1
  ~ if there were no changes found,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

