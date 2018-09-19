BEGIN;

-- Simulate Entries in FILE.DDF
INSERT INTO X$File VALUES (4, 'sample', 'sample.btr', 0, NULL);

-- Simulate Entries in FIELD.DDF
INSERT INTO X$Field VALUES (18, 4, 'ID', 1, 0, 8, 0, 0);
INSERT INTO X$Field VALUES (19, 4, 'FirstName', 0, 8, 16, 0, 0);
INSERT INTO X$Field VALUES (20, 4, 'LastName', 0, 24, 26, 0, 0);
INSERT INTO X$Field VALUES (21, 4, 'Street', 0, 50, 31, 0, 0);
INSERT INTO X$Field VALUES (22, 4, 'City', 0, 81, 31, 0, 0);
INSERT INTO X$Field VALUES (23, 4, 'State', 0, 112, 3, 0, 0);
INSERT INTO X$Field VALUES (24, 4, 'Zip', 0, 115, 11, 0, 0);
INSERT INTO X$Field VALUES (25, 4, 'Country', 0, 126, 21, 0, 0);
INSERT INTO X$Field VALUES (26, 4, 'Phone', 0, 147, 14, 0, 0);

-- Simulate Entries in INDEX.DDF
INSERT INTO X$Index VALUES (4, 18, 0, 0, 0);
INSERT INTO X$Index VALUES (4, 23, 2, 0, 0);
INSERT INTO X$Index VALUES (4, 22, 2, 1, 0);

-- Add our PostgreSQL DSN
INSERT INTO B$Datasource VALUES (1, 'pgdb');

-- Add a mapping from file sample.btr to table person
INSERT INTO B$File VALUES (4, 1, 'person');

-- Add per-column mappings
INSERT INTO B$Field VALUES (18, 4, 'id');
INSERT INTO B$Field VALUES (19, 4, 'firstname');
INSERT INTO B$Field VALUES (20, 4, 'lastname');
INSERT INTO B$Field VALUES (21, 4, 'street');
INSERT INTO B$Field VALUES (22, 4, 'city');
INSERT INTO B$Field VALUES (23, 4, 'state');
INSERT INTO B$Field VALUES (24, 4, 'zip');
INSERT INTO B$Field VALUES (25, 4, 'country');
INSERT INTO B$Field VALUES (26, 4, 'phone');

COMMIT;

