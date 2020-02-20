
# NAME

osmdbt-testdb - Check database connection


# SYNOPSIS

**osmdbt-testdb** \[*OPTIONS*\]


# DESCRIPTION

Check database connection and print PostgreSQL and schema version and
information about active replication slots.


# OPTIONS

@MAN_COMMON_OPTIONS@

# DIAGNOSTICS

**osmdbt-testdb** exits with exit code

0
  ~ if everything went alright,

2
  ~ if there was an error while doing its job, or

3
  ~ if there was a problem with the command line arguments or config file


# SEE ALSO

* **osmdbt**(1)

