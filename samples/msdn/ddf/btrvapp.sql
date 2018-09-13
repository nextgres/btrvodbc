BEGIN;

-- Simulate Entries in FILE.DDF
INSERT INTO X$File VALUES (4, 'titlepub', 'titlepub.btr', 0, NULL);
INSERT INTO X$File VALUES (5, 'sales', 'sales.btr', 0, NULL);

-- Simulate Entries in FIELD.DDF
INSERT INTO X$Field VALUES (18, 4, 'TitleID', 0, 0, 7, 0, 0);
INSERT INTO X$Field VALUES (19, 4, 'Title', 0, 7, 81, 0, 0);
INSERT INTO X$Field VALUES (20, 4, 'Type', 0, 88, 13, 0, 0);
INSERT INTO X$Field VALUES (21, 4, 'PubID', 0, 101, 5, 0, 0);
INSERT INTO X$Field VALUES (22, 4, 'Price', 2, 108, 4, 0, 0);
INSERT INTO X$Field VALUES (23, 4, 'Advance', 2, 112, 4, 0, 0);
INSERT INTO X$Field VALUES (24, 4, 'Royalty', 1, 116, 4, 0, 0);
INSERT INTO X$Field VALUES (25, 4, 'YTD_Sales', 1, 120, 4, 0, 0);
INSERT INTO X$Field VALUES (26, 4, 'PubName', 0, 124, 14, 0, 0);
INSERT INTO X$Field VALUES (27, 4, 'City', 0, 165, 14, 0, 0);
INSERT INTO X$Field VALUES (28, 4, 'State', 0, 186, 14, 0, 0);
INSERT INTO X$Field VALUES (29, 4, 'Country', 0, 189, 14, 0, 0);
INSERT INTO X$Field VALUES (30, 5, 'StorID', 0, 0, 5, 0, 0);
INSERT INTO X$Field VALUES (31, 5, 'TitleID', 0, 5, 7, 0, 0);
INSERT INTO X$Field VALUES (32, 5, 'OrdNum', 0, 12, 21, 0, 0);
INSERT INTO X$Field VALUES (33, 5, 'Qty', 1, 36, 4, 0, 0);
INSERT INTO X$Field VALUES (34, 5, 'PayTerms', 0, 40, 13, 0, 0);

-- Add our PostgreSQL DSN
INSERT INTO B$Datasource VALUES (1, 'pgdb');

-- Add a mapping from our files to tables
INSERT INTO B$File VALUES (4, 1, 'title_publisher');
INSERT INTO B$File VALUES (5, 1, 'bsales');

-- Add per-column mappings
INSERT INTO B$Field VALUES (18, 4, 'title_id');
INSERT INTO B$Field VALUES (19, 4, 'title');
INSERT INTO B$Field VALUES (20, 4, 'type');
INSERT INTO B$Field VALUES (21, 4, 'pub_id');
INSERT INTO B$Field VALUES (22, 4, 'price');
INSERT INTO B$Field VALUES (23, 4, 'advance');
INSERT INTO B$Field VALUES (24, 4, 'royalty');
INSERT INTO B$Field VALUES (25, 4, 'ytd_sales');
INSERT INTO B$Field VALUES (26, 4, 'pub_name');
INSERT INTO B$Field VALUES (27, 4, 'city');
INSERT INTO B$Field VALUES (28, 4, 'state');
INSERT INTO B$Field VALUES (29, 4, 'country');
INSERT INTO B$Field VALUES (30, 5, 'stor_id');
INSERT INTO B$Field VALUES (31, 5, 'title_id');
INSERT INTO B$Field VALUES (32, 5, 'ordnum');
INSERT INTO B$Field VALUES (33, 5, 'qty');
INSERT INTO B$Field VALUES (34, 5, 'payterms');
  
COMMIT;

