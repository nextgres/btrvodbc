BTRVODBC_DATA=./ddf/btrvodbc_btrsamp.db
gcc -DLOG_USE_COLOR -DBTI_LINUX -I../../include ../../src/*.c *.c -o btrsamp -lodbc -lsqlite3
cat ../../ddf/ddf.sql | sqlite3 ${BTRVODBC_DATA}
cat ./ddf/btrsamp.sql | sqlite3 ${BTRVODBC_DATA}
