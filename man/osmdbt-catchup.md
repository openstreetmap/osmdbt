
# NAME

osmdbt-catchup - Mark changes in log as done


# SYNOPSIS

**osmdbt-catchup** \[*OPTIONS*\]


# DESCRIPTION

Advances the replication slot marking all changes up to that point as done.
If the option **-l, \--lsn** is used, catch up to the specified LSN (Log
Sequence Number). If not, the command will look in `log_dir` at all files
called `*.log` there and use the newest LSN found in the file names.


# OPTIONS

-l, \--lsn=LSN
:   The LSN (Log Sequence Number).

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

