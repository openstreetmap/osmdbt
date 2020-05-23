
# NAME

osmdbt-disable-replication - Disable replication on the database


# SYNOPSIS

**osmdbt-disable-replication** \[*OPTIONS*\]


# DESCRIPTION

Disable replication on the database. The database will no longer write changes
to the internal replication slot. Any entries currently in the replication slot
will be lost.


# OPTIONS

@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-disable-replication** exits with exit code

0
  ~ if everything went alright,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

