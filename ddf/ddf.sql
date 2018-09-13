BEGIN;

DROP TABLE IF EXISTS X$Rights;
DROP TABLE IF EXISTS X$User;
DROP TABLE IF EXISTS X$Index;
DROP TABLE IF EXISTS X$Field;
DROP TABLE IF EXISTS X$File;
DROP TABLE IF EXISTS B$Datasource;
DROP TABLE IF EXISTS B$File;
DROP TABLE IF EXISTS B$Field;

-- FILE.DDF
CREATE TABLE X$File (
  Xf$Id                 INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  Xf$Name               VARCHAR(20) NOT NULL UNIQUE,
  Xf$Location           VARCHAR(64) NOT NULL,
  Xf$Flags              INTEGER NOT NULL DEFAULT 0,
  Xf$Reserved           VARCHAR(10));

INSERT INTO X$File VALUES (1, 'X$File', 'file.ddf', 16, NULL);
INSERT INTO X$File VALUES (2, 'X$Field', 'field.ddf', 16, NULL);
INSERT INTO X$File VALUES (3, 'X$Index', 'index.ddf', 16, NULL);

-- FIELD.DDF
CREATE TABLE X$Field (
  Xe$Id                 INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  Xe$File               INTEGER NOT NULL REFERENCES X$File(Xf$Id),
  Xe$Name               VARCHAR(20) NOT NULL,
  Xe$DataType           INTEGER NOT NULL,
  Xe$Offset             INTEGER NOT NULL,
  Xe$Size               INTEGER NOT NULL,
  Xe$Dec                INTEGER NOT NULL,
  Xe$Flags              INTEGER NOT NULL DEFAULT 0);

CREATE UNIQUE INDEX X$Field_Xe$File_Xe$Name ON X$Field (Xe$File, Xe$Name);
/*
 Fld Field Name  Type    Size
   1 Xe$Id       Integer    2 <- Field ID starting from 1 (Unique)
   2 Xe$File     Integer    2 <- File ID (Xf$ID in FILE.DDF)
   3 Xe$Name     String    20 <- Field Name (May have duplicates)
   4 Xe$DataType Integer    1 <- Field Type (0-13). See below
   5 Xe$Offset   Integer    2 <- Field Offset starting from 0
   6 Xe$Size     Integer    2 <- Field Size
   7 Xe$Dec      Integer    1 <- Field decimal places (for Decimal type)
   8 Xe$Flags    Integer    2 <- Reserved

 Key Type       Field     Size Orde
   0 Unique     1 Xe$Id      2 Ascending

   1 Non-unique 2 Xe$File    2 Ascending

   2 Non-unique 3 Xe$Name   20 Ascending

   3 Unique     2 Xe$File    2 Ascending
                3 Xe$Name   20 Ascending
*/

INSERT INTO X$Field VALUES (1, 1, 'Xf$Id', 1, 0, 2, 0, 0);
INSERT INTO X$Field VALUES (2, 1, 'Xf$Name', 0, 2, 20, 0, 0);
INSERT INTO X$Field VALUES (3, 1, 'Xf$Loc', 0, 22, 64, 0, 0);
INSERT INTO X$Field VALUES (4, 1, 'Xf$Flags', 1, 86, 1, 0, 0);
INSERT INTO X$Field VALUES (5, 2, 'Xe$Id', 1, 0, 2, 0, 0);
INSERT INTO X$Field VALUES (6, 2, 'Xe$File', 1, 2, 2, 0, 0);
INSERT INTO X$Field VALUES (7, 2, 'Xe$Name', 0, 4, 20, 0, 0);
INSERT INTO X$Field VALUES (8, 2, 'Xe$DataType', 1, 24, 1, 0, 0);
INSERT INTO X$Field VALUES (9, 2, 'Xe$Offset', 1, 25, 2, 0, 0);
INSERT INTO X$Field VALUES (10, 2, 'Xe$Size', 1, 27, 2, 0, 0);
INSERT INTO X$Field VALUES (11, 2, 'Xe$Dec', 1, 29, 1, 0, 0);
INSERT INTO X$Field VALUES (12, 2, 'Xe$Flags', 1, 30, 2, 0, 0);
INSERT INTO X$Field VALUES (13, 3, 'Xi$File', 1, 0, 2, 0, 0);
INSERT INTO X$Field VALUES (14, 3, 'Xi$Field', 1, 2, 2, 0, 0);
INSERT INTO X$Field VALUES (15, 3, 'Xi$Number', 1, 4, 2, 0, 0);
INSERT INTO X$Field VALUES (16, 3, 'Xi$Part', 1, 6, 2, 0, 0);
INSERT INTO X$Field VALUES (17, 3, 'Xi$Flags', 1, 8, 2, 0, 0);

-- INDEX.DDF
CREATE TABLE X$Index (
  Xi$File               INTEGER NOT NULL REFERENCES X$File(Xf$Id),
  Xi$Field              INTEGER NOT NULL REFERENCES X$Field(Xe$Id),
  Xi$Number             INTEGER NOT NULL,
  Xi$Part               INTEGER NOT NULL,
  Xi$Flags              INTEGER NOT NULL DEFAULT 0);

-- USER.DDF
CREATE TABLE X$User (
  Xu$Id                 INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  Xu$Name               VARCHAR(30) NOT NULL UNIQUE,
  Xu$Password           VARCHAR(9) NOT NULL,
  Xu$Flags              INTEGER NOT NULL DEFAULT 0);

-- RIGHTS.DDF
CREATE TABLE X$Rights (
  Xr$User               INTEGER NOT NULL REFERENCES X$User(Xu$Id),
  Xr$Table              INTEGER NOT NULL REFERENCES X$File(Xf$id),
  Xr$Column             INTEGER NULL REFERENCES X$Field(Xe$Id),
  Xr$Rights             INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (Xr$User, Xr$Table, Xr$Column));

CREATE TABLE B$Datasource (
  Bd$Id                 INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  Bd$DSN                VARCHAR(64) NOT NULL UNIQUE);

CREATE TABLE B$File (
  Bf$File               INTEGER NOT NULL PRIMARY KEY REFERENCES X$File(Xf$Id),
  Bf$Datasource         INTEGER NOT NULL REFERENCES B$Datasource(Bd$Id),
  Bf$TableName          VARCHAR(64) NOT NULL UNIQUE);

CREATE TABLE B$Field (
  Be$Field              INTEGER NOT NULL PRIMARY KEY REFERENCES X$Field(Xf$Id),
  Be$File               INTEGER NOT NULL REFERENCES B$File(Bf$File),
  Be$FieldName          VARCHAR(64) NOT NULL);

CREATE UNIQUE INDEX B$Field_Be$File_Be$FieldName ON B$Field (Be$File, Be$FieldName);

DROP VIEW IF EXISTS B$FileMapping;
CREATE VIEW B$FileMapping
    AS SELECT Xf$Id,
              Xf$Name,
              Xf$Location,
              Xf$Flags,
              Bd$DSN,
              Bf$TableName
         FROM B$File,
              B$Datasource,
              X$File
        WHERE Xf$Id = Bf$File
              AND Bd$Id = Bf$Datasource;

DROP VIEW IF EXISTS B$FieldMapping;
CREATE VIEW B$FieldMapping
    AS SELECT Xe$Id,
              Xe$File,
              Xe$Name,
              Xe$DataType,
              Xe$Offset,
              Xe$Size,
              Xe$Dec,
              Xe$Flags,
              Be$FieldName
         FROM B$Field,
              X$Field
        WHERE Xe$Id = Be$Field
     ORDER BY Xe$Id;



/*
SELECT Xe$Name,
       Xe$DataType,
       Xe$Offset,
       Xe$Size,
       Xe$Dec,
       Xe$Flags,
       Be$FieldName
  FROM X$Field,
       X$
*/

COMMIT;

