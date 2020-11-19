
# OSM Database Tools

Tools for creating replication feeds from the main OSM database.

**These tools are only useful if you run an OSM database like the main central
OSM database!**

[![Travis Build Status](https://secure.travis-ci.org/openstreetmap/osmdbt.svg)](https://travis-ci.org/openstreetmap/osmdbt)

## Prerequisites

You need a C++11 compliant compiler. GCC 8 and later as well as clang 7 and
later are known to work.

You also need the following libraries:

    Libosmium (>= 2.15.0)
        https://osmcode.org/libosmium
        Debian/Ubuntu: libosmium2-dev
        Fedora/CentOS: libosmium-devel

    Protozero (>= 1.6.3)
        https://github.com/mapbox/protozero
        Debian/Ubuntu: libprotozero-dev
        Fedora/CentOS: protozero-devel

    boost-filesystem (>= 1.55)
        https://www.boost.org/doc/libs/1_55_0/libs/filesystem/doc/index.htm
        Debian/Ubuntu: libboost-filesystem-dev
        Fedora/CentOS: boost-devel

    boost-program-options (>= 1.55)
        https://www.boost.org/doc/libs/1_55_0/doc/html/program_options.html
        Debian/Ubuntu: libboost-program-options-dev
        Fedora/CentOS: boost-devel

    bz2lib
        http://www.bzip.org/
        Debian/Ubuntu: libbz2-dev
        Fedora/CentOS: bzip2-devel

    zlib
        https://www.zlib.net/
        Debian/Ubuntu: zlib1g-dev
        Fedora/CentOS: zlib-devel

    Expat
        https://libexpat.github.io/
        Debian/Ubuntu: libexpat1-dev
        Fedora/CentOS: expat-devel

    cmake
        https://cmake.org/
        Debian/Ubuntu: cmake
        Fedora/CentOS: cmake

    yaml-cpp
        https://github.com/jbeder/yaml-cpp
        Debian/Ubuntu: libyaml-cpp-dev
        Fedora/CentOS: yaml-cpp-devel

    libpqxx (version 6)
        https://github.com/jtv/libpqxx/
        Debian/Ubuntu: libpqxx-dev
        Fedora/CentOS: libpqxx-devel

    Pandoc
        (Needed to build documentation, optional)
        https://pandoc.org/
        Debian/Ubuntu: pandoc
        Fedora/CentOS: pandoc

    gettext
        (envsubst command for tests)
        Debian/Ubuntu: gettext-base
        Fedora/CentOS: gettext

    PostgreSQL database and pg_virtualenv
        Debian/Ubuntu: postgresql-common, postgresql-server-dev-all
        Fedora/CentOS: postgresql-server, postgresql-server-devel

On Linux systems most of these libraries are available through your package
manager, see the list above for the names of the packages. But make sure to
check the versions. If the packaged version available is not new enough, you'll
have to install from source. Most likely this is the case for Libosmium.


## Building

Use CMake to build in the usual way, for instance:

```
mkdir build
cd build
cmake ..
cmake --build .
```

If there are several versions of PostgreSQL installed on your system, you
might have to set the `PG_CONFIG` variable to the full path like so:

```
cmake -DPG_CONFIG=/usr/lib/postgresql/9.6/bin/pg_config ..
```

If you don't want to build the PostgreSQL plugin set `BUILD_PLUGIN` to `OFF`.

```
cmake -DBUILD_PLUGIN=OFF ..
```

If you only want to build the PostgreSQL plugin:

```
cd postgresql-plugin
mkdir build
cd build
cmake ..
cmake --build .
```

## Database Setup

You need a PostgreSQL database with
* config option `wal_level=logical`,
* config option `max_replication_slots` set to at least 1,
* a user with REPLICATION attribute, and
* a database containing an OSM database where this user has access.

There is an unofficial `test/structure.sql` provided in this repository to
set up an OSM database for testing. Do not use it for production, use the
official way of installing an OSM database instead.

(The original for this file is in the openstreetmap-website repository at
https://github.com/openstreetmap/openstreetmap-website/raw/master/db/structure.sql)


## Running

There are several commands in the `build/src` directory. They all can be
called with `-h` or `--help` to see how they are run. They all need a common
config file, a template is in `osmdbt-config.yaml`. This will be found
automatically if it is in the current directory, use `-c` or `--config` to
set a different path.


## Documentation

There are man pages for all commands in `man` and an overview page in
[`man/osmdbt.md`](man/osmdbt.md).

If you have `pandoc` installed they will be built when running `make`.


## Tests

To run the tests after build call `ctest`.


## Debian Package

To create a Debian/Ubuntu package, call `debuild -I`.

The Debian package will contain the executables and the man pages. It will
not contain the PostgreSQL plugin, because that needs to be built for the
exact PostgreSQL version you have.


## Usage

First set up the configuration file and make sure you can access the database
by running:

    osmdbt-testdb

Then enable replication:

    osmdbt-enable-replication

After that get current log files once every minute (or whatever you want the
update interval to be). Use cron or something like it to handle this:

    osmdbt-get-log --catchup

To create an OSM change file from the log, call

    osmdbt-create-diff

To disable replication, use:

    osmdbt-disable-replication

For more details see the individual man pages.


## How it works in detail

This section describes how everything is supposed to work in detail. The
whole process is controlled from one shell script run once per minute. It
looks something like this:

```
#!/bin/sh

set -e

osmdbt-catchup
osmdbt-get-log
# optionally copy log file(s) to other hosts
osmdbt-catchup
osmdbt-create-diff

```

### 1. Catch up old log files

If there are complete log files left over from a crash, they will be in the
`log_dir` directory and named `*.log`.

`osmdbt-catchup` is called without command line arguments. It finds those
left-over log files and tells the PostgreSQL database the largest of the LSNs
so that the database can "forget" all changes before that.

If there was no crash, no such log files are found and `osmdbt-catchup` does
nothing.

### 2. Create log file

Now `osmdbt-get-log` is called which creates a log file in the `log_dir` named
something like `osm-repl-2020-03-18T14:18:49Z-lsn-0-1924DE0.log`. The file is
first created with the suffix `.new`, synced to disk, then renamed and the
directory is synced.

If any of these steps fail or if the host crashes, a `.new` file might be
left around, which should be flagged for the sysadmin to take care of. The
file can be removed without loosing data, but the circumstances should be
reviewed in case there is some systematic problem.

### 3. Copy log file to separate host (optional)

All files named `*.log` in the `log_dir` can now be copied (using scp or
rsync or so) to a separate host for safekeeping. These will only be used if
the local host crashes and log files on its disk are lost. In this case
manual intervention is necessary.

### 4. Catch up database to new log file

Now `osmdbt-catchup` is called to catch up the database to the log file just
created in step 2.

If the system crashes in step 2, 3, or 4 a log file might be left around
without the database being updated. In this case step 1 of the next cycle
will pick this up and do the database update.

### 5. Creating diff file

Now `osmdbt-create-diff` is called which reads any log files in the `log_dir`
and creates replication diff files. Files are first created in the `tmp_dir`
directory and then moved into place in the `changes_dir` and its
subdirectories. `osmdbt-create-diff` will also read the `state.txt` in the
`changes_dir` file and create a new one. See the manual page for
`osmdbt-create-diff` for the details on how this is done exactly.

## Log files and lock files

* The programs `osmdbt-get-log`, `osmdbt-fake-log`, and `osmdbt-catchup` use
  the same PID/lock file `run_dir/osmdbt-log` making sure that only one of
  them is running.
* The program `osmdbt-create-diff` uses a different PID/lock file, it can run
  in parallel to the other programs, but only one copy of it will run.
* `osmdbt-create-diff` can handle any number of log files, so if it is not
  run for a while it will recover by reading all log files it finds and
  creating one replication diff file with the (sorted) data from all of them.
* All programs will write files under different names or in separate
  directories, sync the files and only then move them into place atomically.
  After that directories are synced.
* After diff and state files are created in the `tmp_dir`, before they are
  moved to the `changes_dir`, a file `osmdbt-create-diff.lock` is created
  in `tmp_dir`. This is removed after `osmdbt-create-diff` finished moving
  all state and diff files to their final place and appending the `.done`
  suffix to the log file(s). If this is left lying around, `osmdbt-create-diff`
  will not run any more and the sysadmin needs to intervene.


## External processing needed

When run in production you should regularly
* remove old log files marked as done (files in `log_dir` named `*.log.done`)
* remove log file copies you might have made on a separate host

To make sure everything runs smoothly, the age of the PID files can be checked
(should never be more than a few seconds) and the existence of older (more
than a minute or so) log files named `*.log.new`.


## License

Copyright (C) 2020  Jochen Topf (jochen@topf.org)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.


## Authors

This program was written and is maintained by Jochen Topf (jochen@topf.org).

