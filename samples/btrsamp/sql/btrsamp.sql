CREATE TABLE person (
  id                    BIGINT,
  FirstName             VARCHAR(16),
  LastName              VARCHAR(26),
  street                VARCHAR(31),
  city                  VARCHAR(31),
  state                 VARCHAR(3),
  zip                   VARCHAR(11),
  country               VARCHAR(21),
  phone                 VARCHAR(14),
  PRIMARY KEY (id));

INSERT INTO person VALUES (215647871, 'Jonah', 'Harris', '238 Trinity Ave', 'Ambler', 'PA', '19002', 'US', '5053790968');
INSERT INTO person VALUES (263512477, 'John', 'Doe', '1 Microsoft Way', 'Seattle', 'WA', '20205', 'US', '2021234567');

