BTRVODBC_DATA=./ddf/btrvodbc_btrtest.db
gcc -DLOG_USE_COLOR -DBTI_LINUX -I../../include ../../src/*.c *.c -o btrtest -lodbc -lsqlite3
cat ../../ddf/ddf.sql | sqlite3 ${BTRVODBC_DATA}
cat ./ddf/btrtest.sql | sqlite3 ${BTRVODBC_DATA}
