CREATE TABLE public.bsales (
    stor_id character(4) NOT NULL,
    title_id character varying(6) NOT NULL,
    ordnum character varying(20) NOT NULL,
    qty smallint NOT NULL,
    payterms character varying(12)
);

CREATE TABLE public.title_publisher (
    title_id character varying(6) NOT NULL,
    title character varying(80) NOT NULL,
    type character(12) NOT NULL,
    pub_id character(4),
    price real,
    advance real,
    royalty integer,
    ytd_sales integer,
    pub_name character varying(40),
    city character varying(20),
    state character(2),
    country character varying(30)
);

INSERT INTO public.bsales VALUES ('7066', 'PS2091', 'QA7442.3', 75, 'ON invoice');
INSERT INTO public.bsales VALUES ('7067', 'PS2091', 'D4482', 10, 'Net 60');
INSERT INTO public.bsales VALUES ('7131', 'PS2091', 'N914008', 20, 'Net 30');
INSERT INTO public.bsales VALUES ('7131', 'MC3021', 'N914014', 25, 'Net 30');
INSERT INTO public.bsales VALUES ('8042', 'MC3021', '423LL922', 15, 'ON invoice');
INSERT INTO public.bsales VALUES ('8042', 'BU1032', '423LL930', 10, 'ON invoice');
INSERT INTO public.bsales VALUES ('6380', 'PS2091', '722a', 3, 'Net 60');
INSERT INTO public.bsales VALUES ('6380', 'BU1032', '6871', 5, 'Net 60');
INSERT INTO public.bsales VALUES ('8042', 'BU1111', 'P723', 25, 'Net 30');
INSERT INTO public.bsales VALUES ('7896', 'BU2075', 'X999', 35, 'ON invoice');
INSERT INTO public.bsales VALUES ('7896', 'BU7832', 'QQ2299', 15, 'Net 60');
INSERT INTO public.bsales VALUES ('7896', 'MC2222', 'TQ456', 10, 'Net 60');
INSERT INTO public.bsales VALUES ('8042', 'PC1035', 'QA879.1', 30, 'Net 30');
INSERT INTO public.bsales VALUES ('7066', 'PC8888', 'A2976', 50, 'Net 30');
INSERT INTO public.bsales VALUES ('7131', 'PS1372', 'P3087a', 20, 'Net 60');
INSERT INTO public.bsales VALUES ('7131', 'PS2106', 'P3087a', 25, 'Net 60');
INSERT INTO public.bsales VALUES ('7131', 'PS3333', 'P3087a', 15, 'Net 60');
INSERT INTO public.bsales VALUES ('7131', 'PS7777', 'P3087a', 25, 'Net 60');
INSERT INTO public.bsales VALUES ('7067', 'TC3218', 'P2121', 40, 'Net 30');
INSERT INTO public.bsales VALUES ('7067', 'TC4203', 'P2121', 20, 'Net 30');
INSERT INTO public.bsales VALUES ('7067', 'TC7777', 'P2121', 20, 'Net 30');

INSERT INTO public.title_publisher VALUES ('PS2106', 'Life Without Fear', 'psychology  ', '0736', 7, 6000, 10, 111, 'New Moon Books', 'Boston', 'MA', 'USA');
INSERT INTO public.title_publisher VALUES ('PS2091', 'Is Anger the Enemy?', 'psychology  ', '0736', 10.9499998, 2275, 12, 2045, 'New Moon Books', 'Boston', 'MA', 'USA');
INSERT INTO public.title_publisher VALUES ('BU2075', 'You Can Combat Computer Stress!', 'business    ', '0736', 2.99000001, 10125, 24, 18722, 'New Moon Books', 'Boston', 'MA', 'USA');
INSERT INTO public.title_publisher VALUES ('PS3333', 'Prolonged Data Deprivation: Four Case Studies', 'psychology  ', '0736', 19.9899998, 2000, 10, 4072, 'New Moon Books', 'Boston', 'MA', 'USA');
INSERT INTO public.title_publisher VALUES ('PS7777', 'Emotional Security: A New Algorithm', 'psychology  ', '0736', 7.98999977, 4000, 10, 3336, 'New Moon Books', 'Boston', 'MA', 'USA');
INSERT INTO public.title_publisher VALUES ('PS1372', 'Computer Phobic AND Non-Phobic Individuals: Behavior Variations', 'psychology  ', '0877', 21.5900002, 7000, 10, 375, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('MC3026', 'The Psychology of Computer Cooking', 'UNDECIDED   ', '0877', NULL, NULL, NULL, NULL, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('TC3218', 'Onions, Leeks, and Garlic: Cooking Secrets of the Mediterranean', 'trad_cook   ', '0877', 20.9500008, 7000, 10, 375, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('MC3021', 'The Gourmet Microwave', 'mod_cook    ', '0877', 2.99000001, 15000, 24, 22246, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('TC4203', 'Fifty Years in Buckingham Palace Kitchens', 'trad_cook   ', '0877', 11.9499998, 4000, 14, 15096, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('TC7777', 'Sushi, Anyone?', 'trad_cook   ', '0877', 14.9899998, 8000, 10, 4095, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('MC2222', 'Silicon Valley Gastronomic Treats', 'mod_cook    ', '0877', 19.9899998, 0, 12, 2032, 'Binnet & Hardley', 'Washington', 'DC', 'USA');
INSERT INTO public.title_publisher VALUES ('PC9999', 'Net Etiquette', 'popular_comp', '1389', NULL, NULL, NULL, NULL, 'Algodata Infosystems', 'Berkeley', 'CA', 'USA');
INSERT INTO public.title_publisher VALUES ('BU7832', 'Straight Talk About Computers', 'business    ', '1389', 19.9899998, 5000, 10, 4095, 'Algodata Infosystems', 'Berkeley', 'CA', 'USA');
INSERT INTO public.title_publisher VALUES ('PC1035', 'But Is It User Friendly?', 'popular_comp', '1389', 22.9500008, 7000, 16, 8780, 'Algodata Infosystems', 'Berkeley', 'CA', 'USA');
INSERT INTO public.title_publisher VALUES ('BU1111', 'Cooking with Computers: Surreptitious Balance Sheets', 'business    ', '1389', 11.9499998, 5000, 10, 3876, 'Algodata Infosystems', 'Berkeley', 'CA', 'USA');
INSERT INTO public.title_publisher VALUES ('BU1032', 'The Busy Executive''s Database Guide', 'business    ', '1389', 19.9899998, 5000, 10, 4095, 'Algodata Infosystems', 'Berkeley', 'CA', 'USA');
INSERT INTO public.title_publisher VALUES ('PC8888', 'Secrets of Silicon Valley', 'popular_comp', '1389', 20, 8000, 10, 4095, 'Algodata Infosystems', 'Berkeley', 'CA', 'USA');

