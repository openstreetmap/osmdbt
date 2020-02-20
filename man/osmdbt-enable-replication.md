
# NAME

osmdbt-enable-replication - Enable replication on the database


# SYNOPSIS

**osmdbt-enable-replication** \[*OPTIONS*\]


# DESCRIPTION

Enable replication on the database. From now on the database will add all
changes to the replication slot.


# OPTIONS

@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-enable-replication** exits with exit code

0
  ~ if everything went alright,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

