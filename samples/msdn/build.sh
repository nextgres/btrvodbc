BTRVODBC_DATA=./ddf/btrvodbc_btrvapp.db
gcc -w -DBTI_LINUX -I../../include ../../src/*.c *.c -o btrvapp -lodbc -lsqlite3
cat ../../ddf/ddf.sql | sqlite3 ${BTRVODBC_DATA}
cat ./ddf/btrvapp.sql | sqlite3 ${BTRVODBC_DATA}
