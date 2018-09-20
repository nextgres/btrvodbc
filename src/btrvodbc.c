/* ========================================================================= **
** ######################################################################### **
** #####     ###      ##     ###  ##  ###    ###     ###     ####    ####### **
** #####  ##  ####  ####  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ###### **
** #####     #####  ####     ###  ##  ##  ##  ##  ##  ##     ###  ########## **
** #####  ##  ####  ####  ##  ###    ###  ##  ##  ##  ##  ##  ##  ##  ###### **
** #####     #####  ####  ##  ####  #####    ###     ###     ####    ####### **
** ######################################################################### **
** ========================================================================= **
**                                                                           **
** Copyright (c) 2004-2018 Jonah H. Harris.                                  **
**                                                                           **
** Permission is hereby granted, free of charge, to any person obtaining a   **
** copy of this software and associated documentation files (the "Software") **
** to deal in the Software without restriction, including without limitation **
** the rights to use, copy, modify, merge, publish, distribute, sublicense,  **
** and/or sell copies of the Software, and to permit persons to whom the     **
** Software is furnished to do so, subject to the following conditions:      **
**                                                                           **
** The above copyright notice and this permission notice shall be included   **
** in all copies or substantial portions of the Software.                    **
**                                                                           **
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   **
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                **
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN **
** NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  **
** DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     **
** OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE **
** USE OR OTHER DEALINGS IN THE SOFTWARE.                                    **
** ========================================================================= */

/* ========================================================================= */
/* -- INCLUSIONS ----------------------------------------------------------- */
/* ========================================================================= */

/* Standard C Headers */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

/* ODBC Headers */
#include <sql.h>
#include <sqlext.h>

/* Btrieve Headers */
#include "btrapi.h"
#include "btrconst.h"

/* SQLite Headers */
#include <sqlite3.h>

/* SDS Headers */
#include "sds.h"

/* Troy D. Hanson Headers */
#include "uthash.h"
#include "utlist.h"

/* Logging Headers */
#include "log.h"

#if 0
 * @param operation[in]       determines the action performed by the Btrieve API
 * @param posBlock    pointer to structures and positioning information
 * @param dataBuffer[out]     used to transfer data to/from the file
 * @param dataLength[in, out] the size (in bytes) of the data buffer
 * @param keyBuffer   contains the key value
 * @param keyLength   the size (in bytes) of the key buffer
 * @param ckeynum     depends on which operation is being performed
 * @param clientID    pointer to structure representing a specific client
 * @return the value of the Btrieve API function call (B_NO_ERROR on success)
#endif

/* ========================================================================= */
/* -- DEFINITIONS ---------------------------------------------------------- */
/* ========================================================================= */

#define MAXFIELDS 255
#define MAXBATCHSIZE 255
#define BTRVODBC_OP_ID_MAX                    65
#define BTRVODBC_MAX_KEY_LENGTH  255

#define BTRVODBC_CALL_PARAM_IN_POS            0x0001
#define BTRVODBC_CALL_PARAM_IN_DATABUF        0x0002
#define BTRVODBC_CALL_PARAM_IN_DATALEN        0x0004
#define BTRVODBC_CALL_PARAM_IN_KEYBUF         0x0008
#define BTRVODBC_CALL_PARAM_OUT_POS           0x0010
#define BTRVODBC_CALL_PARAM_OUT_DATABUF       0x0020
#define BTRVODBC_CALL_PARAM_OUT_DATALEN       0x0040
#define BTRVODBC_CALL_PARAM_OUT_KEYBUF        0x0080

/* ========================================================================= */
/* -- MACROS --------------------------------------------------------------- */
/* ========================================================================= */

#define BTRVODBC_DECLARE_HANDLER(pOperation)                                  \
static BTI_SINT btrvodbc_ ## pOperation (BTI_WORD, BTI_VOID_PTR,              \
  BTI_VOID_PTR, BTI_ULONG_PTR, BTI_VOID_PTR, BTI_BYTE, BTI_CHAR,              \
  BTI_BUFFER_PTR);

#define BTRVODBC_DEFINE_HANDLER(pOperation)                                   \
static BTI_SINT                                                               \
btrvodbc_ ## pOperation (                                                     \
  BTI_WORD              operation,                                            \
  BTI_VOID_PTR          posBlock,                                             \
  BTI_VOID_PTR          dataBuffer,                                           \
  BTI_ULONG_PTR         dataLength,                                           \
  BTI_VOID_PTR          keyBuffer,                                            \
  BTI_BYTE              keyLength,                                            \
  BTI_CHAR              ckeynum,                                              \
  BTI_BUFFER_PTR        clientID                                              \
)

#define CHECK_ERROR(e, s, h, t) ({                                            \
  if (SQL_SUCCESS != e && SQL_SUCCESS_WITH_INFO != e) {                       \
    extract_error(s, h, t);                                                   \
    goto exit;                                                                \
  }                                                                           \
})

#define likely(x)       __builtin_expect((x), 1)
#define unlikely(x)     __builtin_expect((x), 0)
#define min(a, b)       (((a)<(b))?(a):(b))
#define max(a, b)       (((a)>(b))?(a):(b))

/* ========================================================================= */
/* -- PRIVATE TYPES -------------------------------------------------------- */
/* ========================================================================= */

typedef struct {
  BTI_SINT                      id;
  BTI_SINT                      file_id;
  char                          name[BTRVODBC_MAX_KEY_LENGTH];
  BTI_SINT                      data_type;
  BTI_SINT                      offset;
  BTI_SINT                      size;
  BTI_SINT                      decimal;
  BTI_SINT                      flags;
  char                          column_name[BTRVODBC_MAX_KEY_LENGTH];
  UT_hash_handle                hh_id;
  UT_hash_handle                hh_name;
  UT_hash_handle                hh_column_name;
} btrvodbc_field_definition_t;

typedef struct btrvodbc_index_definition_t {
  BTI_SINT                            file_id;
  BTI_SINT                            field_id;
  BTI_CHAR                            number;
  BTI_CHAR                            part;
  BTI_SINT                            flags;
  struct btrvodbc_index_definition_t *next;
  UT_hash_handle                      hh_number;
} btrvodbc_index_definition_t;

typedef struct {
  BTI_SINT                      id;
  char                          name[BTRVODBC_MAX_KEY_LENGTH];
  char                          path[BTRVODBC_MAX_KEY_LENGTH];
  BTI_SINT                      flags;
  char                          reserved[BTRVODBC_MAX_KEY_LENGTH];
  char                          dsn[BTRVODBC_MAX_KEY_LENGTH];
  char                          table_name[BTRVODBC_MAX_KEY_LENGTH];
  btrvodbc_field_definition_t  *fields_by_id;
  btrvodbc_field_definition_t  *fields_by_name;
  btrvodbc_field_definition_t  *fields_by_column_name;
  btrvodbc_index_definition_t  *indices_by_number;
  UT_hash_handle                hh_id;
  UT_hash_handle                hh_path;
} btrvodbc_file_definition_t;

typedef struct btrvodbc_position_t {
  btrvodbc_file_definition_t   *file;
  SQLHSTMT                      hstmt;
  sds                           query;
  BTI_CHAR                      key_number;
  BTI_WORD                      last_execute_operation;
  int64_t                       relative_fetch_offset;
  uint32_t                      key_buffer_hash;
  SQLPOINTER                    results[MAXFIELDS];
  SQLPOINTER                    result_names[MAXFIELDS];
  SQLLEN                        result_lengths[MAXFIELDS];
  SQLLEN                        result_lengths_or_indicators[MAXFIELDS];
  SQLSMALLINT                   result_data_type_array[MAXFIELDS];
  struct btrvodbc_position_t   *next;
} btrvodbc_position_t;

typedef struct {
  char                          client_id[16];
  SQLHENV                       henv;
  SQLHDBC                       hdbc;
  btrvodbc_position_t          *positions;
  UT_hash_handle                hh_client_id;
} btrvodbc_client_t;

typedef struct {
  btrvodbc_position_t          *position;
  btrvodbc_client_t            *client;
} btrvodbc_position_pointer_t;

/*
SESSION
 - FILE
   - FIELDS
   - ODBC
*/

typedef BTI_SINT (*btrvodbc_op_fp_t) (BTI_WORD, BTI_VOID_PTR, BTI_VOID_PTR,
  BTI_ULONG_PTR, BTI_VOID_PTR, BTI_BYTE, BTI_CHAR, BTI_BUFFER_PTR);

/* ========================================================================= */
/* -- PRIVATE METHOD PROTOTYPES -------------------------------------------- */
/* ========================================================================= */

static void extract_error (char *, SQLHANDLE, SQLSMALLINT);
static void btrvodbc_initialize (void);
static void btrvodbc_prepare (void);
static void btrvodbc_dump_field (btrvodbc_field_definition_t *);
static btrvodbc_client_t * btrvodbc_upsert_client (BTI_BUFFER_PTR);

/* ---------------------- B-trieve Operation Handlers ---------------------- */

BTRVODBC_DECLARE_HANDLER(b_invalid_op)
BTRVODBC_DECLARE_HANDLER(common_get)

/* ---------------------- B-trieve Operation Handlers ---------------------- */

BTRVODBC_DECLARE_HANDLER(b_open)
BTRVODBC_DECLARE_HANDLER(b_close)
BTRVODBC_DECLARE_HANDLER(b_insert)
BTRVODBC_DECLARE_HANDLER(b_update)
BTRVODBC_DECLARE_HANDLER(b_delete)
BTRVODBC_DECLARE_HANDLER(b_get_equal)
BTRVODBC_DECLARE_HANDLER(b_get_next)
BTRVODBC_DECLARE_HANDLER(b_get_previous)
BTRVODBC_DECLARE_HANDLER(b_get_gt)
BTRVODBC_DECLARE_HANDLER(b_get_ge)
BTRVODBC_DECLARE_HANDLER(b_get_lt)
BTRVODBC_DECLARE_HANDLER(b_get_le)
BTRVODBC_DECLARE_HANDLER(b_get_first)
BTRVODBC_DECLARE_HANDLER(b_get_last)
BTRVODBC_DECLARE_HANDLER(b_create)
BTRVODBC_DECLARE_HANDLER(b_stat)
BTRVODBC_DECLARE_HANDLER(b_extend)
BTRVODBC_DECLARE_HANDLER(b_set_dir)
BTRVODBC_DECLARE_HANDLER(b_get_dir)
BTRVODBC_DECLARE_HANDLER(b_begin_tran)
BTRVODBC_DECLARE_HANDLER(b_end_tran)
BTRVODBC_DECLARE_HANDLER(b_abort_tran)
BTRVODBC_DECLARE_HANDLER(b_get_position)
BTRVODBC_DECLARE_HANDLER(b_get_direct)
BTRVODBC_DECLARE_HANDLER(b_step_next)
BTRVODBC_DECLARE_HANDLER(b_stop)
BTRVODBC_DECLARE_HANDLER(b_version)
BTRVODBC_DECLARE_HANDLER(b_unlock)
BTRVODBC_DECLARE_HANDLER(b_reset)
BTRVODBC_DECLARE_HANDLER(b_set_owner)
BTRVODBC_DECLARE_HANDLER(b_clear_owner)
BTRVODBC_DECLARE_HANDLER(b_build_index)
BTRVODBC_DECLARE_HANDLER(b_drop_index)
BTRVODBC_DECLARE_HANDLER(b_step_first)
BTRVODBC_DECLARE_HANDLER(b_step_last)
BTRVODBC_DECLARE_HANDLER(b_step_previous)
BTRVODBC_DECLARE_HANDLER(b_get_next_extended)
BTRVODBC_DECLARE_HANDLER(b_get_prev_extended)
BTRVODBC_DECLARE_HANDLER(b_step_next_ext)
BTRVODBC_DECLARE_HANDLER(b_step_previous_ext)
BTRVODBC_DECLARE_HANDLER(b_ext_insert)
BTRVODBC_DECLARE_HANDLER(b_misc_data)
BTRVODBC_DECLARE_HANDLER(b_continuous)
BTRVODBC_DECLARE_HANDLER(b_seek_percent)
BTRVODBC_DECLARE_HANDLER(b_get_percent)
BTRVODBC_DECLARE_HANDLER(b_chunk_update)
BTRVODBC_DECLARE_HANDLER(b_extended_stat)

/* ========================================================================= */
/* -- PRIVATE DATA --------------------------------------------------------- */
/* ========================================================================= */

static bool btrvodbc_is_initialized = false;
static uint16_t btrvodbc_op_parameters[(BTRVODBC_OP_ID_MAX + 1)] = { 0 };
static btrvodbc_op_fp_t btrvodbc_op_handler[(BTRVODBC_OP_ID_MAX + 1)] = {
  btrvodbc_b_invalid_op
};

/* ------------------------- Temporary Shit Globals ------------------------ */

static btrvodbc_client_t btrvodbc_global_client;

btrvodbc_file_definition_t *btrvodbc_global_files_by_id = NULL;
btrvodbc_file_definition_t *btrvodbc_global_files_by_path = NULL;
static btrvodbc_client_t *btrvodbc_clients = NULL;

/* ========================================================================= */
/* -- PUBLIC DATA ---------------------------------------------------------- */
/* ========================================================================= */

/* ========================================================================= */
/* -- EXTERNAL DATA -------------------------------------------------------- */
/* ========================================================================= */

/* ========================================================================= */
/* -- EXTERNAL FUNCTION PROTOTYPES ----------------------------------------- */
/* ========================================================================= */

/* Jenkins Hash */
uint32_t hashlittle (const void *, size_t, uint32_t);

/* ========================================================================= */
/* -- PRIVATE METHODS ------------------------------------------------------ */
/* ========================================================================= */

void
AllocBuffer (
  SQLUINTEGER buffSize,
  SQLPOINTER *Ptr,
  SQLLEN *BufferLen
) {
  *Ptr=malloc (buffSize);
  memset (*Ptr, ' ', buffSize);
  if (BufferLen != NULL) {
    *BufferLen=buffSize;
  }
  return;
} /* AllocBuffer() */

/* ------------------------------------------------------------------------- */

void
extract_error (
  char *fn,
  SQLHANDLE handle,
  SQLSMALLINT type
) {
  SQLINTEGER i = 0;
  SQLINTEGER NativeError;
  SQLCHAR SQLState[ 7 ];
  SQLCHAR MessageText[256];
  SQLSMALLINT TextLength;
  SQLRETURN ret;

  log_error("The driver reported the following error %s", fn);
  do {
    ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError, MessageText,
      sizeof(MessageText), &TextLength);
    if (SQL_SUCCEEDED(ret)) {
      log_error("%s:%ld:%ld:%s\n", SQLState, (long) i, (long) NativeError,
        MessageText);
    }
  }
  while (ret == SQL_SUCCESS);
}

/* ------------------------------------------------------------------------- */

/**
 * Makes a file available for access.
 *
 * @param operation[in]       the action performed by the Btrieve API
 * @param posBlock[out]       pointer to structures and positioning information
 * @param dataBuffer[in]      used to transfer data to/from the file
 * @param dataLength[in]      the size (in bytes) of the data buffer
 * @param keyBuffer[in]       contains the key value
 * @param keyLength[in]       the size (in bytes) of the key buffer
 * @param ckeynum[in]         depends on which operation is being performed
 * @param clientID[in]        pointer to structure representing a client
 * @return the value of the Btrieve API function call (B_NO_ERROR on success)
 */
BTRVODBC_DEFINE_HANDLER(b_open) {
  log_trace("Entering Function %s", __func__);

  char *owner_name = NULL;
  char *file_name = NULL;
  uint8_t open_mode = (uint8_t) ckeynum;
  btrvodbc_client_t *client = btrvodbc_upsert_client(clientID);
  btrvodbc_file_definition_t *file_entry = NULL;
  btrvodbc_position_t *position = NULL;

  if (0 < dataLength) {
    owner_name = dataBuffer;
  }

  if (0 < keyLength) {
    file_name = keyBuffer;
  }

  switch (open_mode) {
    case 0x00: /* NORMAL */
    case 0xff: /* ACCELERATED */
    case 0xfe: /* READONLY */
    case 0xfd: /* VERIFY */
    case 0xfc: /* EXCLUSIVE */
      break;
    default:
      log_error("Invalid B_OPEN mode (%02x)", open_mode);
  }

  log_trace("owner..... %s", owner_name);
  log_trace("file...... %s", file_name);
  log_trace("mode...... %u", open_mode);

  /* Search for this file in our database */
  HASH_FIND(hh_path, btrvodbc_global_files_by_path, file_name,
    strlen(file_name), file_entry);
  if (NULL == file_entry) {
    log_error("File [%s] does not exist in X$File", file_name);
    abort();
  }

  log_debug("Opening file %"PRId16" for SQL Table (%s)",
    file_entry->id, file_entry->table_name);

  position = calloc(1, sizeof(*position));
  position->key_number = -1;
  position->file = file_entry;
  LL_APPEND(client->positions, position);

  /* We only use posBlock to store the table name */
  btrvodbc_position_pointer_t *position_ptr = malloc(sizeof(*position_ptr));
  position_ptr->position = position;
  position_ptr->client = client;
//  memcpy(posBlock, keyBuffer, min(128, keyLength));
  memcpy(posBlock, position_ptr, min(128, sizeof(*position_ptr)));

  SQLRETURN retcode;

  // Allocate environment
  retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &client->henv);
  CHECK_ERROR(retcode, "SQLAllocHandle(SQL_ATTR_ODBC_VERSION)",
              client->henv, SQL_HANDLE_ENV);

  // Set ODBC Version
  retcode = SQLSetEnvAttr(client->henv, SQL_ATTR_ODBC_VERSION,
                          (SQLPOINTER*)SQL_OV_ODBC3, 0);
  CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
              client->henv, SQL_HANDLE_ENV);

  // Allocate Connection
  retcode = SQLAllocHandle(SQL_HANDLE_DBC, client->henv, &client->hdbc);
  CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
              client->hdbc, SQL_HANDLE_DBC);

  // Set Login Timeout
  retcode = SQLSetConnectAttr(client->hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5,
    0);
  CHECK_ERROR(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
              client->hdbc, SQL_HANDLE_DBC);

  // Connect to DSN
  retcode = SQLConnect(client->hdbc, (SQLCHAR*) file_entry->dsn, SQL_NTS,
                       (SQLCHAR*) NULL, 0, NULL, 0);
  CHECK_ERROR(retcode, "SQLConnect(DATASOURCE)",
              client->hdbc, SQL_HANDLE_DBC);

  // Allocate Statement Handle
  retcode = SQLAllocHandle(SQL_HANDLE_STMT, client->hdbc,
    &position->hstmt);
  CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
              position->hstmt, SQL_HANDLE_STMT);

  // Set the SQL_ATTR_ROW_BIND_TYPE statement attribute to use column-wise
  // binding. Declare the rowset size with the SQL_ATTR_ROW_ARRAY_SIZE
  // statement attribute. Set the SQL_ATTR_ROW_STATUS_PTR statement
  // attribute to point to the row status array.
  SQLSetStmtAttr(position->hstmt, SQL_ATTR_CURSOR_TYPE,
    (SQLPOINTER) SQL_CURSOR_KEYSET_DRIVEN, 0);
  SQLSetStmtAttr(position->hstmt, SQL_ATTR_ROW_BIND_TYPE,
    (SQLPOINTER) SQL_BIND_BY_COLUMN, 0);

  /*
   * Iterate over each field in our structure allocating the appropriate amount
   * of memory for it, binding it to our statement, and adding it to our base
   * SQL query.
   */
  position->query = sdsnew("SELECT ");
  int ii = 0;
  btrvodbc_field_definition_t *fld, *fldtmp;
  HASH_ITER(hh_id, file_entry->fields_by_id, fld, fldtmp) {
    if (0 < ii) {
      position->query = sdscat(position->query, ", ");
    }

    //AllocBuffer(ColumnSize, &position->results[ii],
    log_debug("Allocating %zd bytes for field [%s]",
      (size_t) fld->size, fld->name);

    AllocBuffer(128, &position->results[ii],
      &position->result_lengths[ii]);
    retcode = SQLBindCol(position->hstmt, (ii + 1), SQL_C_CHAR,
      position->results[ii], position->result_lengths[ii],
      &position->result_lengths_or_indicators[ii]);
    CHECK_ERROR(retcode, "SQLBindCol()", position->hstmt,
      SQL_HANDLE_STMT);

    /* Add this field to our base SQL query */
    position->query = sdscat(position->query, fld->column_name);

    ++ii;
  }
  position->query = sdscatfmt(position->query, " FROM %s",
    file_entry->table_name);

  return B_NO_ERROR;
exit:
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_open() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_close) {
  btrvodbc_client_t *client = btrvodbc_upsert_client(clientID);
  btrvodbc_position_pointer_t *posptr =
    (btrvodbc_position_pointer_t *) posBlock;
  btrvodbc_position_t *position = posptr->position;

  /* Is someone trying to close someone else's position? */
  if (client != posptr->client) {
    log_error("Clients don't match!");
    return B_PROGRAMMING_ERROR;
  }

  btrvodbc_position_t *found, *pos, *tmp;
  LL_FOREACH_SAFE(client->positions, pos, tmp) {
    if (pos == position) {
      log_debug("Closing position on file (%s)", position->file->name);
      LL_DELETE(client->positions, pos);
      free(position);
      return B_NO_ERROR;
    }
  }

  return B_INVALID_FUNCTION;
} /* btrvodbc_b_close() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_insert) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_insert() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_update) {
  SQLRETURN  rc;

  /* Extract the Data Structure */

#if 0
  int  numRows=0;      // Number of rows in resultset

  SQLINTEGER  cbTitleID = SQL_NTS, cbTitle = SQL_NTS, cbType = SQL_NTS, cbPubID = SQL_NTS;
  SQLINTEGER  cbPubName = SQL_NTS, cbCity = SQL_NTS, cbState = SQL_NTS, cbCountry = SQL_NTS;
  SQLINTEGER  RoyaltyInd, AdvanceInd, PriceInd, YTDSalesInd;

  // Initialize the tpRec data structure
  memset( &tpRec, 0, sizeof(tpRec) );
  memcpy(&tpRec, dataBuffer, sizeof(tpRec));

  /*  Update the current row within the cursor. We rebind the columns to update the length
  /*  of the NULL terminated string columns. We are using 0 for the the numRows
  /*  parameter to affect all rows in the rowset.  Since we have a rowset size of 1
  /*  only the positioned row will be affected. The key value of the current record
  /*  is not changing so we issue the positioned update using
  /*  SQLSetPos(SQL_UPDATE, SQL_LOCK_NO_CHANGE)
  */

  SQLBindCol(hstmtS, 1, SQL_C_CHAR, tpRec.TitleID, 7, &cbTitleID);
  SQLBindCol(hstmtS, 2, SQL_C_CHAR, tpRec.Title, 81, &cbTitle);
  SQLBindCol(hstmtS, 3, SQL_C_CHAR, tpRec.Type, 13, &cbType);
  SQLBindCol(hstmtS, 4, SQL_C_CHAR, tpRec.PubID, 5, &cbPubID);
  SQLBindCol(hstmtS, 5, SQL_C_FLOAT, &tpRec.Price, 0, &PriceInd);
  SQLBindCol(hstmtS, 6, SQL_C_FLOAT, &tpRec.Advance, 0, &AdvanceInd);
  SQLBindCol(hstmtS, 7, SQL_C_SLONG, &tpRec.Royalty, 0, &RoyaltyInd);
  SQLBindCol(hstmtS, 8, SQL_C_SLONG, &tpRec.YTD_Sales, 0, &YTDSalesInd);
  SQLBindCol(hstmtS, 9, SQL_C_CHAR, tpRec.PubName, 41, &cbPubName);
  SQLBindCol(hstmtS, 10, SQL_C_CHAR, tpRec.City, 21, &cbCity);
  SQLBindCol(hstmtS, 11, SQL_C_CHAR, tpRec.State, 3, &cbState);
  SQLBindCol(hstmtS, 12, SQL_C_CHAR, tpRec.Country, 31, &cbCountry);

  rc=SQLSetPos(hstmtS, numRows, SQL_UPDATE,  SQL_LOCK_NO_CHANGE);
  if ((rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO))
    {
      ErrorDump("SQLSetPos SQL_UPDATE for TITLEPUBLISHER FAILED", SQL_NULL_HENV, SQL_NULL_HDBC, hstmtS);
      return B_UNRECOVERABLE_ERROR;
    }

  return B_NO_ERROR;
#endif
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_update() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_delete) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_delete() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_equal) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   WHERE (col0_in_key, ..., colN_in_key)
   *            = (keyBuffer[col0_offset], ..., keyBuffer[colN_offset])
   *   ORDER BY col0_in_key, ..., colN_in_key;
   */
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_equal() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_next) {
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_next() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_previous) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_previous() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_gt) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   WHERE (col0_in_key, ..., colN_in_key)
   *            > (keyBuffer[col0_offset], ..., keyBuffer[colN_offset])
   *   ORDER BY col0_in_key, ..., colN_in_key;
   */
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_gt() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_ge) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   WHERE (col0_in_key, ..., colN_in_key)
   *            >= (keyBuffer[col0_offset], ..., keyBuffer[colN_offset])
   *   ORDER BY col0_in_key, ..., colN_in_key;
   */
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_ge() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_lt) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   WHERE (col0_in_key, ..., colN_in_key)
   *            < (keyBuffer[col0_offset], ..., keyBuffer[colN_offset])
   *   ORDER BY col0_in_key, ..., colN_in_key;
   */
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_lt() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_le) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   WHERE (col0_in_key, ..., colN_in_key)
   *            <= (keyBuffer[col0_offset], ..., keyBuffer[colN_offset])
   *   ORDER BY col0_in_key, ..., colN_in_key;
   */
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_le() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_first) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   ORDER BY col0_in_key, ..., colN_in_key;
   */
  log_trace("Entering Function %s", __func__);
  return btrvodbc_common_get(operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);
} /* btrvodbc_b_get_first() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_last) {
  /*
   * Build row-set comparison of keys as:
   *
   *  SELECT col0, ..., colN
   *    FROM table
   *   ORDER BY col0_in_key, ..., colN_in_key DESC LIMIT 1;
   */
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_last() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_create) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_create() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_stat) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_stat() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_extend) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_extend() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_set_dir) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_set_dir() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_dir) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_dir() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_begin_tran) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_begin_tran() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_end_tran) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_end_tran() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_abort_tran) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_abort_tran() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_position) {
  /* Use a ROWID? */
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_position() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_direct) {
  /* Use a ROWID? */
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_direct() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_step_next) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_step_next() */

/* ------------------------------------------------------------------------- */

/**
 * Performs termination routines for the client.
 *
 * @param operation[in]       the action performed by the Btrieve API
 * @return the value of the Btrieve API function call (B_NO_ERROR on success)
 */
BTRVODBC_DEFINE_HANDLER(b_stop) {
  log_trace("Entering Function %s", __func__);
  btrvodbc_client_t *client = btrvodbc_upsert_client(clientID);
  btrvodbc_position_t *pos, *tmp;

  /* XXX FIXME Update Free */
  LL_FOREACH_SAFE(client->positions, pos, tmp) {
    log_debug("Closing position on file %s\n", pos->file->name);

    if (SQL_NULL_HSTMT != pos->hstmt) {
      SQLFreeHandle(SQL_HANDLE_STMT, pos->hstmt);
    }
    LL_DELETE(client->positions, pos);
    free(pos);
  }

  if (SQL_NULL_HDBC != client->hdbc) {
    SQLDisconnect(client->hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, client->hdbc);
  }

  if (SQL_NULL_HENV != client->henv) {
    SQLFreeHandle(SQL_HANDLE_ENV, client->henv);
  }

  return B_NO_ERROR;
} /* btrvodbc_b_stop() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_invalid_op) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_invalid_op() */

/* ------------------------------------------------------------------------- */

/**
 * Return the MicroKernel Engine/Requester version(s).
 *
 * - Client applications
 *    - Returns the MicroKernel Engine and Requester version, if applicable.
 *      NOTE: If the client opens a file on a server or specifies a server file
 *            path name in the key buffer, this returns the MicroKernel Engine
 *            version on that server.
 * - Server applications
 *    - Returns the MicroKernel Engine version and revision numbers.
 *
 * @param operation[in]       determines the action performed by the Btrieve API
 * @param dataBuffer[out]     used to transfer data to/from the file
 * @param dataLength[in, out] the size (in bytes) of the data buffer
 * @return the value of the Btrieve API function call (B_NO_ERROR on success)
 */
BTRVODBC_DEFINE_HANDLER(b_version) {

  log_trace("Entering Function %s", __func__);
  btrvodbc_client_t *client = btrvodbc_upsert_client(clientID);

  typedef struct {
    BTI_SINT  Version;
    BTI_SINT  Revision;
    BTI_CHAR  MKDEId;
  } VERSION_STRUCT;
  if (NULL != dataBuffer && NULL != dataLength) {
    size_t length = (size_t) *dataLength;
    if (length < sizeof(VERSION_STRUCT)) {
      return B_DATALENGTH_ERROR;
    }
    VERSION_STRUCT version = {
      .Version = 6,
      .Revision = 15,
      .MKDEId = 'B'
    };
    memcpy(dataBuffer, &version, sizeof(version));
    return B_NO_ERROR;
  }
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_version() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_unlock) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_unlock() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_reset) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_reset() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_set_owner) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_set_owner() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_clear_owner) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_clear_owner() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_build_index) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_build_index() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_drop_index) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_drop_index() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_step_first) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_step_first() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_step_last) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_step_last() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_step_previous) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_step_previous() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_next_extended) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_next_extended() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_prev_extended) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_prev_extended() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_step_next_ext) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_step_next_ext() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_step_previous_ext) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_step_previous_ext() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_ext_insert) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_ext_insert() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_misc_data) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_misc_data() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_continuous) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_continuous() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_seek_percent) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_seek_percent() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_get_percent) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_get_percent() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_chunk_update) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_chunk_update() */

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(b_extended_stat) {
  return B_INVALID_FUNCTION;
} /* btrvodbc_b_extended_stat() */

/* ------------------------------------------------------------------------- */

void
hexdump (
  const void  *data,
  size_t size
) {
  char ascii[17];
  size_t i, j;
  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    printf("%02X ", ((unsigned char*)data)[i]);
    if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char*)data)[i];
    } else {
      ascii[i % 16] = '.';
    }
    if ((i+1) % 8 == 0 || i+1 == size) {
      printf(" ");
      if ((i+1) % 16 == 0) {
        printf("|  %s \n", ascii);
      } else if (i+1 == size) {
        ascii[(i+1) % 16] = '\0';
        if ((i+1) % 16 <= 8) {
          printf(" ");
        }
        for (j = (i+1) % 16; j < 16; ++j) {
          printf("   ");
        }
        printf("|  %s \n", ascii);
      }
    }
  }
}

/* ------------------------------------------------------------------------- */

BTRVODBC_DEFINE_HANDLER(common_get) {
  log_trace("Entering Function %s", __func__);

  btrvodbc_client_t *client = btrvodbc_upsert_client(clientID);

  btrvodbc_position_pointer_t *posptr =
    (btrvodbc_position_pointer_t *) posBlock;

  btrvodbc_position_t *position = posptr->position;
  btrvodbc_file_definition_t *file_entry = position->file;
  btrvodbc_index_definition_t *index_entry = NULL;

  SQLRETURN retcode;
  SQLSMALLINT fetch_orientation = SQL_FETCH_FIRST;
  SQLLEN fetch_offset = 0;

  /* Commonly used iterators */
  int ii = 0;
  btrvodbc_field_definition_t *fld = NULL;
  btrvodbc_field_definition_t *fldtmp = NULL;
  btrvodbc_index_definition_t *idx = NULL;
  btrvodbc_index_definition_t *idxtmp = NULL;

  //log_trace("File id is %"PRId16" with SQL Table [%s]",
  log_debug("File id is %"PRId16" with SQL Table [%s]",
    file_entry->id, file_entry->table_name);

  if (client != posptr->client) {
    log_error("Trying to use someone else's posblock?");
    abort();
  }

  /*
   * I haven't tested the order in which Btrieve applies these checks but, I
   * figure, an application with proper error-checking probably shouldn't be
   * negatively affected either way...
   */

  /* Validate ckeynum & return index info */
  HASH_FIND(hh_number, file_entry->indices_by_number, &ckeynum,
    sizeof(ckeynum), index_entry);
  if (NULL == index_entry) {
    log_debug("Invalid Key Number (%d)", ckeynum);
    return B_INVALID_KEYNUMBER;
  }

  if (ckeynum != position->key_number) {
    /*
     * If this is a positional operation and the requested key differs from the
     * one used to position the original result set, return an error.
     */
    if ((B_GET_NEXT == operation)
        || (B_GET_NEXT_EXTENDED == operation)
        || (B_GET_PREVIOUS == operation)
        || (B_GET_PREV_EXTENDED == operation)) {
      return B_DIFFERENT_KEYNUMBER;
    }

    /* Update the key number */
    position->key_number = ckeynum;
  }

  /*
   * If this is a first or last operation, we only need to apply the SQL ORDER
   * BY clause.
   */

  /*
   * For all key-based get operations, we build a SQL WHERE clause using
   * row-wise comparison syntax.
   *
   * While this is part of SQL92, it's not supported by most databases. As
   * such, even though we're using ODBC, the SQL generated here MAY NOT WORK
   * with the underlying database. This is used primarily because compound
   * indices will, if available, normally be selected and it gives us an
   * even closer-to-ISAM like scanning of relations using multi-part keys
   * without relying on cursors. 
   */
  sds sql = sdsdup(position->query); /* XXX Remove for fetch-only ops */
  if ((B_GET_EQUAL == operation) || (B_GET_GE == operation)
      || (B_GET_GT == operation) || (B_GET_LE == operation)
      || (B_GET_LT == operation)) {

    sds row_cmp_lvalue = sdsempty();
    sds row_cmp_rvalue = sdsempty();

    ii = 0;
    LL_FOREACH_SAFE(index_entry, idx, idxtmp) {
      log_trace("Key %d / Part %d / Field %d", idx->number, idx->part, idx->field_id);
      if (0 < ii) {
        row_cmp_lvalue = sdscat(row_cmp_lvalue, ", ");
        row_cmp_rvalue = sdscat(row_cmp_rvalue, ", ");
      }

      HASH_FIND(hh_id, file_entry->fields_by_id, &idx->field_id,
        sizeof(idx->field_id), fld);
      if (NULL == fld) {
        log_error("Couldn't find field %d", idx->field_id);
      }

      row_cmp_lvalue = sdscatfmt(row_cmp_lvalue, "%s", fld->column_name);

      switch (fld->data_type) {
        case INTEGER_TYPE:
          {
            BTI_LONG id = *(BTI_LONG BTI_FAR *) &keyBuffer[0];
            //hash = hashlittle(keyBuffer, sizeof(BTI_LONG), 0);
            log_trace("Key is %lu", id);
            row_cmp_rvalue = sdscatfmt(row_cmp_rvalue, "%U", id);
          }
          break;
        case STRING_TYPE:
        case ZSTRING_TYPE:
          {
            log_trace("Key is %s", keyBuffer);
            //hash = hashlittle(keyBuffer, fld->size, 0);
            row_cmp_rvalue = sdscatfmt(row_cmp_rvalue, "'%s'", keyBuffer);
          }
          break;
      }
      ++ii;
    }

    sds row_cmp_operator = NULL;
    switch (operation) {
      case B_GET_EQUAL:
        row_cmp_operator = sdsnew("=");
        fetch_orientation = SQL_FETCH_FIRST;
        break;
      case B_GET_GT:
        row_cmp_operator = sdsnew(">");
        fetch_orientation = SQL_FETCH_FIRST;
        break;
      case B_GET_GE:
        row_cmp_operator = sdsnew(">=");
        fetch_orientation = SQL_FETCH_FIRST;
        break;
      case B_GET_LT:
        row_cmp_operator = sdsnew("<");
        fetch_orientation = SQL_FETCH_LAST;
        break;
      case B_GET_LE:
        row_cmp_operator = sdsnew("<=");
        fetch_orientation = SQL_FETCH_LAST;
        break;
    }

    sql = sdscatfmt(sql, " WHERE (%s) %s (%s)",
      row_cmp_lvalue, row_cmp_operator, row_cmp_rvalue);

    sdsfree(row_cmp_lvalue);
    sdsfree(row_cmp_operator);
    sdsfree(row_cmp_rvalue);
  }

  if (!((B_GET_NEXT == operation)
        || (B_GET_NEXT_EXTENDED == operation)
        || (B_GET_PREVIOUS == operation)
        || (B_GET_PREV_EXTENDED == operation))) {
    sds sql_order_by_clause = sdsempty();
    ii = 0;
    LL_FOREACH_SAFE(index_entry, idx, idxtmp) {
      HASH_FIND(hh_id, file_entry->fields_by_id, &idx->field_id,
        sizeof(idx->field_id), fld);
      if (NULL == fld) {
        log_error("Couldn't find field %d", idx->field_id);
      }
      if (0 < ii) {
        sql_order_by_clause = sdscat(sql_order_by_clause, ", ");
      }
      sql_order_by_clause = sdscatfmt(sql_order_by_clause, "%s",
        fld->column_name);
      ++ii;
    }
    sql = sdscatfmt(sql, " ORDER BY %s", sql_order_by_clause);

    /* If there's already a cursor, we need to close it */
    SQLCloseCursor(position->hstmt);

    position->last_execute_operation = operation;
    position->relative_fetch_offset = 0;

    /* This can be called from other ops too XXX FIX */
    log_debug("Executing SQL [%s]", sql);
    retcode = SQLExecDirect(position->hstmt, (SQLCHAR *) sql, SQL_NTS);
    if ((retcode!=SQL_NEED_DATA)
        && (retcode!=SQL_SUCCESS)
        && (retcode!=SQL_SUCCESS_WITH_INFO)) {
      CHECK_ERROR(retcode, "SQLExecute()", position->hstmt, SQL_HANDLE_STMT);
    }
  }
  sdsfree(sql);

  /* Clear out bound buffers */
  for (ii = 0
    ; ii < HASH_CNT(hh_id, file_entry->fields_by_id)
    ; ++ii) {
    memset(position->results[ii], ' ', position->result_lengths[ii]);
    strcpy(position->results[ii], "");
  }


  if (B_GET_NEXT == operation) {
    fetch_orientation = SQL_FETCH_NEXT;
  }

  retcode = SQLFetchScroll(position->hstmt, fetch_orientation, 0);
  if (SQL_SUCCESS != retcode && SQL_SUCCESS_WITH_INFO != retcode) {
    log_debug("Nothing to fetch");

    /*
     * Are we possibly out of bounds from the original fetch?
     * Consider B_GET_EQUAL -> READ ALL EQUAL -> SHOULD RESULT IN NEXT
     */

    /* XXX FIX */
    if (B_GET_EQUAL == operation) {
      return B_KEY_VALUE_NOT_FOUND;
    } else {
      return B_END_OF_FILE;
    }
  }
  CHECK_ERROR(retcode, "SQLFetch()", position->hstmt, SQL_HANDLE_STMT);

  /* Handle relative offsets to know when we need to re-execute */
  switch (fetch_orientation) {
    case SQL_FETCH_NEXT:
      ++position->relative_fetch_offset;
      break;
    case SQL_FETCH_PREV:
      --position->relative_fetch_offset;
      break;
  }

  ii = 0;
  HASH_ITER(hh_id, file_entry->fields_by_id, fld, fldtmp) {
    btrvodbc_dump_field(fld);
    log_debug("Value... [%s]", (char *) position->results[ii]);
    switch (fld->data_type) {
      case STRING_TYPE:
      case ZSTRING_TYPE:
        log_trace("ret(STRING) -> %s\n", (char *) position->results[ii]);
#if 0
        log_trace("stpncpy(&dataBuffer[%d], \"%s\", %d);\n",
          fld->offset, (char *) ColPtrArray[ii], fld->size);
#endif
        stpncpy(&dataBuffer[fld->offset], (char *) position->results[ii], fld->size);
        break;

      case INTEGER_TYPE:
        log_trace("INTEGER\n");
        {
          switch (fld->size) {
            case sizeof(BTI_CHAR):
              {
                BTI_CHAR ret = (BTI_CHAR) strtol((char *) position->results[ii], NULL, 10);
                log_trace("ret(%zd) -> %"PRId8"\n", sizeof(BTI_CHAR), (int8_t) ret);
                *(BTI_CHAR *) &dataBuffer[fld->offset] = ret;
              }
              break;
            case sizeof(BTI_SINT):
              {
                BTI_SINT ret = (BTI_SINT) strtol((char *) position->results[ii], NULL, 10);
                log_trace("ret(%zd) -> %"PRId16"\n", sizeof(BTI_SINT), (int16_t) ret);
                *(BTI_SINT *) &dataBuffer[fld->offset] = ret;
              }
              break;
            case sizeof(BTI_INT):
              {
                BTI_INT ret = (BTI_INT) strtol((char *) position->results[ii], NULL, 10);
                log_trace("ret(%zd) -> %"PRId32"\n", sizeof(BTI_INT), (int32_t) ret);
                *(BTI_INT *) &dataBuffer[fld->offset] = ret;
              }
              break;
            case sizeof(BTI_LONG):
              {
                BTI_LONG ret = (BTI_LONG) strtol((char *) position->results[ii], NULL, 10);
                log_trace("ret(%zd) -> %"PRId64"\n", sizeof(BTI_LONG), (int64_t) ret);
                *(BTI_LONG *) &dataBuffer[fld->offset] = ret;
              }
              break;
          }
        }
        break;

      case IEEE_TYPE:
        log_trace("IEEE\n");
        {
          switch (fld->size) {
            case sizeof(float):
              {
                float ret = strtof((char *) position->results[ii], NULL);
                log_trace("ret(%zd) -> %f\n", sizeof(float), ret);
                *(float *) &dataBuffer[fld->offset] = ret;
              }
              break;
            case sizeof(double):
              {
                double ret = strtod((char *) position->results[ii], NULL);
                log_trace("ret(%zd) -> %f\n", sizeof(double), ret);
                *(double *) &dataBuffer[fld->offset] = ret;
              }
              break;
          }
        }
    }
    ++ii;
  }

  /* Copy back the key into keyBuffer */
  LL_FOREACH_SAFE(index_entry, idx, idxtmp) {
    /* XXX FIX */
  }

  return B_NO_ERROR;

exit:

  return B_INVALID_FUNCTION;
}

/* ------------------------------------------------------------------------- */

static btrvodbc_client_t *
btrvodbc_upsert_client (
  BTI_BUFFER_PTR        clientID
) {
  btrvodbc_client_t *client = NULL;

  if (NULL == clientID) {
    log_fatal("clientID should never be NULL?");
    abort();
  }

  HASH_FIND(hh_client_id, btrvodbc_clients, clientID,
    sizeof(btrvodbc_global_client.client_id), client);
  if (NULL == client) {
    log_trace("Client was not found... adding");
    client = calloc(1, sizeof(btrvodbc_client_t));
    memcpy(client->client_id, clientID, sizeof(client->client_id));
    HASH_ADD(hh_client_id, btrvodbc_clients, client_id,
      sizeof(client->client_id), client);
  } else {
    log_trace("client was found");
  }
  return client;

} /* btrvodbc_upsert_client() */

/* ------------------------------------------------------------------------- */

static void
btrvodbc_dump_field (
  btrvodbc_field_definition_t  *field
) {
  log_trace("id............. %d", (int) field->id);
  log_trace("file_id........ %d", (int) field->file_id);
  log_trace("name........... [%s]", field->name);
  log_trace("data_type...... %d", (int) field->data_type);
  log_trace("offset......... %d", (int) field->offset);
  log_trace("size........... %d", (int) field->size);
  log_trace("decimal........ %d", (int) field->decimal);
  log_trace("flags.......... %d", (int) field->flags);
  log_trace("column_name.... [%s]", field->column_name);
} /* btrvodbc_dump_field() */

/* ------------------------------------------------------------------------- */

static int
btrvodbc_sqlite_file_cb (
  void                 *data,
  int                   argc,
  char                **argv,
  char                **azColName
) {
  BTI_SINT ret;
  btrvodbc_file_definition_t *file_entry = calloc(1, sizeof(*file_entry));

  file_entry->id = (BTI_SINT) strtol(argv[0], NULL, 10);
  stpncpy(file_entry->name, argv[1], BTRVODBC_MAX_KEY_LENGTH);
  stpncpy(file_entry->path, argv[2], BTRVODBC_MAX_KEY_LENGTH);
  file_entry->flags = (BTI_SINT) strtol(argv[3], NULL, 10);
  stpncpy(file_entry->dsn, argv[4], BTRVODBC_MAX_KEY_LENGTH);
  stpncpy(file_entry->table_name, argv[5],
    BTRVODBC_MAX_KEY_LENGTH);

  /* Add this to our hash table indexed by name */
  HASH_ADD(hh_id, btrvodbc_global_files_by_id, id,
    sizeof(file_entry->id), file_entry);

  /* Add this to our hash table indexed by path */
  HASH_ADD(hh_path, btrvodbc_global_files_by_path, path,
    strlen(file_entry->path), file_entry);

  log_debug("Loaded FILE.DDF entry for file (%s)", file_entry->name);

  return 0;
} /* btrvodbc_sqlite_file_cb() */

/* ------------------------------------------------------------------------- */

static int
btrvodbc_sqlite_field_cb (
  void                 *data,
  int                   argc,
  char                **argv,
  char                **azColName
) {
  btrvodbc_file_definition_t *file_entry = NULL;
  BTI_SINT ret;
  btrvodbc_field_definition_t *field_entry = calloc(1, sizeof(*field_entry));

  field_entry->id = (BTI_SINT) strtol(argv[0], NULL, 10);
  field_entry->file_id = (BTI_SINT) strtol(argv[1], NULL, 10);
  stpncpy(field_entry->name, argv[2], BTRVODBC_MAX_KEY_LENGTH);
  field_entry->data_type = (BTI_SINT) strtol(argv[3], NULL, 10);
  field_entry->offset = (BTI_SINT) strtol(argv[4], NULL, 10);
  field_entry->size = (BTI_SINT) strtol(argv[5], NULL, 10);
  field_entry->decimal = (BTI_SINT) strtol(argv[6], NULL, 10);
  field_entry->flags = (BTI_SINT) strtol(argv[7], NULL, 10);
  stpncpy(field_entry->column_name, argv[8], BTRVODBC_MAX_KEY_LENGTH);

  HASH_FIND(hh_id, btrvodbc_global_files_by_id, &field_entry->file_id,
    sizeof(field_entry->file_id), file_entry);
  if (NULL == file_entry) {
    log_error("Couldn't find file entry (%d) for field (%s)",
      field_entry->file_id, field_entry->name);
    free(field_entry);
    return 1;
  }

  /* Add this to our hash table indexed by id */
  HASH_ADD(hh_id, file_entry->fields_by_id, id,
    sizeof(field_entry->id), field_entry);

  /* Add this to our hash table indexed by name */
  HASH_ADD(hh_name, file_entry->fields_by_name, name,
    strlen(field_entry->name), field_entry);

  /* Add this to our hash table indexed by name */
  HASH_ADD(hh_column_name, file_entry->fields_by_column_name, column_name,
    strlen(field_entry->column_name), field_entry);

  log_debug("Loaded FIELD.DDF entry for field (%s.%s)",
    file_entry->name, field_entry->name);

  return 0;
} /* btrvodbc_sqlite_field_cb() */

/* ------------------------------------------------------------------------- */

static int
btrvodbc_sqlite_index_cb (
  void                 *data,
  int                   argc,
  char                **argv,
  char                **azColName
) {
  btrvodbc_file_definition_t *file_entry = NULL;
  btrvodbc_field_definition_t *field_entry = NULL;
  btrvodbc_index_definition_t *head_index_entry = NULL;
  btrvodbc_index_definition_t *index_entry = calloc(1, sizeof(*index_entry));

  index_entry->file_id = (BTI_SINT) strtol(argv[0], NULL, 10);
  index_entry->field_id = (BTI_SINT) strtol(argv[1], NULL, 10);
  index_entry->number = (BTI_CHAR) strtol(argv[2], NULL, 10);
  index_entry->part = (BTI_CHAR) strtol(argv[3], NULL, 10);
  index_entry->flags = (BTI_SINT) strtol(argv[4], NULL, 10);

  HASH_FIND(hh_id, btrvodbc_global_files_by_id, &index_entry->file_id,
    sizeof(index_entry->file_id), file_entry);
  if (NULL == file_entry) {
    log_error("Couldn't find file (%d) for index (%d part %d) on field (%d)",
      index_entry->file_id, index_entry->number, index_entry->part,
      index_entry->field_id);
    free(index_entry);
    return 1;
  }

  HASH_FIND(hh_id, file_entry->fields_by_id, &index_entry->field_id,
    sizeof(index_entry->field_id), field_entry);
  if (NULL == field_entry) {
    log_error("Couldn't find field (%d) for index (%d part %d) on field (%d)",
      index_entry->file_id, index_entry->number, index_entry->part,
      index_entry->field_id);
    free(index_entry);
    return 1;
  }

  HASH_FIND(hh_number, file_entry->indices_by_number, &index_entry->number,
    sizeof(index_entry->number), head_index_entry);
  if (NULL == head_index_entry) {
    HASH_ADD(hh_number, file_entry->indices_by_number, number,
      sizeof(index_entry->number), index_entry);
  } else {
    LL_APPEND(head_index_entry, index_entry);
  }

  log_debug("Loaded INDEX.DDF entry for field (%s.%s)",
    file_entry->name, field_entry->name);

  return 0;
} /* btrvodbc_sqlite_field_cb() */

/* ------------------------------------------------------------------------- */

void
btrvodbc_initialize (
  void
) {
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  log_trace("Entering Function %s", __func__);
  //log_set_level(LOG_DEBUG);
  log_set_level(LOG_INFO);

  /* Open database */
  char *db_name = getenv("BTRVODBC_DATA");
  if (NULL == db_name) {
    log_fatal("Environment variable (%s) is unset.", "BTRVODBC_DATA");
    abort();
  }
  log_debug("Loading DDF from %s", db_name);
  rc = sqlite3_open(db_name, &db);
  if (SQLITE_OK != rc) {
    abort();
  }

  /* Load our equivalent of FILE.DDF */
  rc = sqlite3_exec(db, "SELECT * FROM B$FileMapping", btrvodbc_sqlite_file_cb,
    (void *) NULL, &zErrMsg);
  if (SQLITE_OK != rc) {
    log_fatal("Error querying B$FileMapping: %s", zErrMsg);
    sqlite3_free(zErrMsg);
    abort();
  }

  /* Load our equivalent of FIELD.DDF */
  rc = sqlite3_exec(db, "SELECT * FROM B$FieldMapping",
    btrvodbc_sqlite_field_cb, (void *) NULL, &zErrMsg);
  if (SQLITE_OK != rc) {
    log_fatal("Error querying B$FieldMapping: %s", zErrMsg);
    sqlite3_free(zErrMsg);
    abort();
  }

  /* Load our equivalent of INDEX.DDF */
  rc = sqlite3_exec(db,
    "SELECT * FROM X$Index ORDER BY Xi$File, Xi$Number, Xi$Part",
    btrvodbc_sqlite_index_cb, (void *) NULL, &zErrMsg);
  if (SQLITE_OK != rc) {
    log_fatal("Error querying X$Index: %s", zErrMsg);
    sqlite3_free(zErrMsg);
    abort();
  }

  log_trace("* Internal Structures");
  {
    btrvodbc_file_definition_t *fle, *fle_tmp;
    HASH_ITER(hh_path, btrvodbc_global_files_by_path, fle, fle_tmp) {
      log_trace("*** FILE [%s]", fle->name);

      btrvodbc_field_definition_t *fde, *fde_tmp;
      HASH_ITER(hh_id, fle->fields_by_id, fde, fde_tmp) {
        log_trace("**** FIELD (%d) [%s/%s]", fde->id, fde->name, fde->column_name);
      }
    }
  }

  /* Assign operation-specific handlers */
  btrvodbc_op_handler[B_OPEN] = btrvodbc_b_open;
  btrvodbc_op_handler[B_CLOSE] = btrvodbc_b_close;
  btrvodbc_op_handler[B_INSERT] = btrvodbc_b_insert;
  btrvodbc_op_handler[B_UPDATE] = btrvodbc_b_update;
  btrvodbc_op_handler[B_DELETE] = btrvodbc_b_delete;
  btrvodbc_op_handler[B_GET_EQUAL] = btrvodbc_b_get_equal;
  btrvodbc_op_handler[B_GET_NEXT] = btrvodbc_b_get_next;
  btrvodbc_op_handler[B_GET_PREVIOUS] = btrvodbc_b_get_previous;
  btrvodbc_op_handler[B_GET_GT] = btrvodbc_b_get_gt;
  btrvodbc_op_handler[B_GET_GE] = btrvodbc_b_get_ge;
  btrvodbc_op_handler[B_GET_LT] = btrvodbc_b_get_lt;
  btrvodbc_op_handler[B_GET_LE] = btrvodbc_b_get_le;
  btrvodbc_op_handler[B_GET_FIRST] = btrvodbc_b_get_first;
  btrvodbc_op_handler[B_GET_LAST] = btrvodbc_b_get_last;
  btrvodbc_op_handler[B_CREATE] = btrvodbc_b_create;
  btrvodbc_op_handler[B_STAT] = btrvodbc_b_stat;
  btrvodbc_op_handler[B_EXTEND] = btrvodbc_b_extend;
  btrvodbc_op_handler[B_SET_DIR] = btrvodbc_b_set_dir;
  btrvodbc_op_handler[B_GET_DIR] = btrvodbc_b_get_dir;
  btrvodbc_op_handler[B_BEGIN_TRAN] = btrvodbc_b_begin_tran;
  btrvodbc_op_handler[B_END_TRAN] = btrvodbc_b_end_tran;
  btrvodbc_op_handler[B_ABORT_TRAN] = btrvodbc_b_abort_tran;
  btrvodbc_op_handler[B_GET_POSITION] = btrvodbc_b_get_position;
  btrvodbc_op_handler[B_GET_DIRECT] = btrvodbc_b_get_direct;
  btrvodbc_op_handler[B_STEP_NEXT] = btrvodbc_b_step_next;
  btrvodbc_op_handler[B_STOP] = btrvodbc_b_stop;
  btrvodbc_op_handler[B_VERSION] = btrvodbc_b_version;
  btrvodbc_op_handler[B_UNLOCK] = btrvodbc_b_unlock;
  btrvodbc_op_handler[B_RESET] = btrvodbc_b_reset;
  btrvodbc_op_handler[B_SET_OWNER] = btrvodbc_b_set_owner;
  btrvodbc_op_handler[B_CLEAR_OWNER] = btrvodbc_b_clear_owner;
  btrvodbc_op_handler[B_BUILD_INDEX] = btrvodbc_b_build_index;
  btrvodbc_op_handler[B_DROP_INDEX] = btrvodbc_b_drop_index;
  btrvodbc_op_handler[B_STEP_FIRST] = btrvodbc_b_step_first;
  btrvodbc_op_handler[B_STEP_LAST] = btrvodbc_b_step_last;
  btrvodbc_op_handler[B_STEP_PREVIOUS] = btrvodbc_b_step_previous;
  btrvodbc_op_handler[B_GET_NEXT_EXTENDED] = btrvodbc_b_get_next_extended;
  btrvodbc_op_handler[B_GET_PREV_EXTENDED] = btrvodbc_b_get_prev_extended;
  btrvodbc_op_handler[B_STEP_NEXT_EXT] = btrvodbc_b_step_next_ext;
  btrvodbc_op_handler[B_STEP_PREVIOUS_EXT] = btrvodbc_b_step_previous_ext;
  btrvodbc_op_handler[B_EXT_INSERT] = btrvodbc_b_ext_insert;
  btrvodbc_op_handler[B_MISC_DATA] = btrvodbc_b_misc_data;
  btrvodbc_op_handler[B_CONTINUOUS] = btrvodbc_b_continuous;
  btrvodbc_op_handler[B_SEEK_PERCENT] = btrvodbc_b_seek_percent;
  btrvodbc_op_handler[B_GET_PERCENT] = btrvodbc_b_get_percent;
  btrvodbc_op_handler[B_CHUNK_UPDATE] = btrvodbc_b_chunk_update;
  btrvodbc_op_handler[B_EXTENDED_STAT] = btrvodbc_b_extended_stat;

  btrvodbc_is_initialized = true;

} /* btrvodbc_initialize() */

/* ------------------------------------------------------------------------- */

void
btrvodbc_prepare (
  void
) {
} /* btrvodbc_prepare() */

/* ------------------------------------------------------------------------- */

/* ========================================================================= */
/* -- PUBLIC METHODS ------------------------------------------------------- */
/* ========================================================================= */

BTI_SINT
BTRCALLID (
  BTI_WORD              operation,
  BTI_VOID_PTR          posBlock,
  BTI_VOID_PTR          dataBuffer,
  BTI_ULONG_PTR         dataLength,
  BTI_VOID_PTR          keyBuffer,
  BTI_BYTE              keyLength,
  BTI_CHAR              ckeynum,
  BTI_BUFFER_PTR        clientID
) {
  log_trace("Entering Function %s", __func__);

#if 0
  log_info("%s (%d, %p, %p, %ld, %p, %d, %d, %p)",
    __func__, operation, posBlock, dataBuffer, (long int) *dataLength,
    keyBuffer, keyLength, ckeynum, clientID);
#endif

  if (unlikely(false == btrvodbc_is_initialized)) {
    btrvodbc_initialize();
  }

  if (unlikely(BTRVODBC_OP_ID_MAX < operation || 0 > operation)) {
    log_error("Invalid Btrieve operation (%d)", operation);
    return B_INVALID_FUNCTION;
  }

  /* Check input */
  //op_parms( B_VERSION, in_datalen | out_databuf | out_datalen );

  return btrvodbc_op_handler[operation](operation, posBlock, dataBuffer,
    dataLength, keyBuffer, keyLength, ckeynum, clientID);

} /* BTRCALLID() */

/* ------------------------------------------------------------------------- */

BTI_SINT
BTRCALL (
  BTI_WORD              operation,
  BTI_VOID_PTR          posBlock,
  BTI_VOID_PTR          dataBuffer,
  BTI_ULONG_PTR         dataLength,
  BTI_VOID_PTR          keyBuffer,
  BTI_BYTE              keyLength,
  BTI_CHAR              ckeynum
) {
  log_trace("Entering Function %s", __func__);

  BTI_BYTE id[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'M', 'T', 0, 1 };
  return BTRCALLID(operation, posBlock, dataBuffer, dataLength,
    keyBuffer, keyLength, ckeynum, (BTI_BUFFER_PTR) &id);
} /* BTRCALL() */

