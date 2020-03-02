
# NAME

osmdbt-create-diff - Read replication log and create OSM change file from it


# SYNOPSIS

**osmdbt-create-diff** \[*OPTIONS*\]


# DESCRIPTION

Read the log file created by **osmdbt-get-log** and create on OSM change file
from it.


# OPTIONS

-f, \--log-file=FILE
:   Name of the log file to be read (required).

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

