
# NAME

osmdbt-catchup - Mark changes in log as done


# SYNOPSIS

**osmdbt catchup** \[*OPTIONS*\] -l LSN


# DESCRIPTION

Advances the replication slot to the specified LSN (Log Sequence Number)
marking all changes up to that point as done.

Used mainly when testing. In production use, the catchup is usually done
with the `--catchup` option to the `osmdbt-get-log` command.


# OPTIONS

-l, \--lsn
:   The LSN (Log Sequence Number). This option is required.

@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-catchup** exits with exit code

0
  ~ if everything went alright,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

