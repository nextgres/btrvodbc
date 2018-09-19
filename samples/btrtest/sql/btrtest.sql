CREATE TABLE person (
  id                    BIGINT NOT NULL,
  FirstName             VARCHAR(16),
  LastName              VARCHAR(26),
  street                VARCHAR(31),
  city                  VARCHAR(31),
  state                 VARCHAR(3),
  zip                   VARCHAR(11),
  country               VARCHAR(21),
  phone                 VARCHAR(14),
  PRIMARY KEY (id));

INSERT INTO person VALUES (215646787, 'Bill', 'Gates', '1 Microsoft Way', 'Redmond', 'WA', '98052', 'US', '425-882-8080');
INSERT INTO person VALUES (215676647, 'Jonah', 'Harris', '238 Trinity Ave', 'Ambler', 'PA', '19002', 'US', '505-379-0968');
INSERT INTO person VALUES (263512477, 'Larry', 'Ellison', '500 Oracle Parkway', 'Redwood Shores', 'CA', '94065', 'US', '650-506-7000');

