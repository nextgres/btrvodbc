# BTRVODBC: Btrieve to ODBC Bridge

A long time ago, I developed a C library which provided an ISAM-based interface to SQL databases. Based on that knowledge and having had experience writing drop-in replacement libraries for PostgreSQL, MySQL, and the Oracle Call Interface, I decided to build a simple prototype compatible with the Microsoft Jet API; a library enabling the substitution of Microsoft's Extensible Storage Engine with PostgreSQL. A couple years later, I ran across a forum entry where the author wanted to do a migration of their application from Btrieve to PostgreSQL without any source-level changes. At the time, the only example for such a thing was a technical publication from Microsoft on migrating from Btrieve to ODBC. The example application, however, was a mess and required a ton of custom behind-the-scenes app-specific development simulating the Btrieve API. I figured I could do better and that it would be a fun day-long exercise. The proof-of-concept, found here, has been updated to use a number of current libraries. Unforunately, the more full-featured one I did seems to have gotten lost in what I not-so-fondly refer to as _The Great Oracle Internals Data Loss Incident of 2009_.

The basic idea behind this was to wrap ODBC-based calls behind the Btrieve API and translate the operations to relevant SQL queries so that any backend database could be used.

_Note: I've only tried this with Postgres and **this code should never be used for anything other than as a proof-of-concept**. If you're looking for something production-ready, check out [BTR2SQL from Mertech](https://www.mertech.com/migrate-btrieve-to-sql/)_

## Overview of Structures

At the end of the day, Btrieve only really cared about data in terms of type, offset, and length. This can be demonstrated via the B\_CREATE operation, which permits a developer to create a new datafile with an arbitrary byte buffer, and the B\_CREATE\_INDEX operation which only cares about offsets within that buffer. While this is true, most users had the normal data dictionary files (DDF) in terms of FILE.DDF, FIELD.DDF, INDEX.DDF, et al. As we can't map a byte buffer directly to a relational table, however, we require a similar knowledge of the user's underlying structure similar to that provided in the DDF files. To that end, I've re-created a subset of the DDF catalog and use [SQLite](https://sqlite.org/) to manage it.

### Pervasive System Tables & Data Dictionary

The base catalog is stored in ddf/ddf.sql and contains the following canonical tables:

 - **X$File** - Names and locations of the tables in your database.
 - **X$Field** - Column and named index definitions.
 - **X$Index** - Index definitions.
 - **X$User** - User names, group names, and passwords.
 - **X$Rights** - User and group access rights definitions.

### SQL Mapping of DDF

In short, Btrieve concepts are mapped as follows:

 - Files are mapped to relations
 - Fields are mapped to attributes

The following tables are required to map DDF to SQL:

 - **B$Datasource** - Stores the ODBC DSNs used for a file (relation).
 - **B$File** - Maps a Btrieve file entry in X$File to a relation.
 - **B$Field** - Maps a Btrieve file field entry in X$Field to an attribute of a relation.

BTRVODBC requires valid DDFs to convert Btrieve/Pervasive files to SQL tables and migrate the data. It is absolutely essential the DDFs describe the entire layout of the data, or schema, within the Btrieve files.

## Sample Applications

The following sample applications have been included.

### Btrieve/Pervasive Sample (btrsamp.c)

This is the canonical sample included with Btrieve with no changes.

 - **samples/btrsamp** - C source code
 - **samples/btrsamp/ddf** - SQL scripts (for SQLite) to create sample tables/data
 - **samples/btrsamp/sql** - SQL scripts (for PostgreSQL) to create sample tables/data

### Microsoft Btrieve to SQL Server Sample (btrvapp.c)

This a simple data entry and reporting application that maintains information about book titles, publishers that own these titles, and sales information for each title. Its data is based on the standard PUBS database and was designed to provide developers with a starting point for migrating Btrieve applications to SQL Server. It was published as TechPub 5186 and provided with MSDN Library Visual Studio 6.0a Edition, Disc 2.

It's actually a pretty poorly written program that doesn't even really work with Btrieve or ODBC correctly. It, like this implementation, was nothing more than a proof-of-concept. But, you can find a screencast using it against PostgreSQL [here](https://asciinema.org/a/201214).

 - **samples/msdn** - C source code
 - **samples/msdn/ddf** - SQL scripts (for SQLite) to create sample tables/data
 - **samples/msdn/sql** - SQL scripts (for PostgreSQL) to create sample tables/data

## License

 - My code (btrvodbc.c) is released under the MIT License. The rest is other people's stuff.

## Copyright Acknowledgements

 - Copyright (c) 2004-2018 Jonah H. Harris.
 - Portions Copyright 1982-2008 Pervasive Software Inc. All Rights Reserved
 - Portions Copyright (c) 2005-2018 Troy D. Hanson. All rights reserved.
 - Portions Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 - Portions Copyright (c) 2015, Oran Agra
 - Portions Copyright (c) 2015, Redis Labs, Inc
 - Portions Copyright (c) 2017 rxi
 - Portions 2006 Bob Jenkins. Public Domain

