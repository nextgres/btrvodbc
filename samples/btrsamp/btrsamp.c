/*************************************************************************
**
**  Copyright 1982-2003 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/***************************************************************************
  BTRSAMP.C
    This is a simple sample designed to allow you to confirm your
    ability to compile, link, and execute a Btrieve application for
    your target environment using your compiler tools.

    This program demonstrates the C/C++ interface for Btrieve for DOS,
    Extended DOS, OS2 (16 and 32-bit), MS Windows, NetWare NLM, MS
    Windows NT, Windows 95, and Linux.

    This program does the following operations on the sample database:
    - gets the Microkernel Database Engine version
    - opens sample.btr
    - gets a record on a known value of Key 0
    - displays the retrieved record
    - performs a stat operation
    - creates an empty 'clone' of sample.btr and opens it
    - performs a 'Get Next Extended' operation to extract a subset
      of the records in sample.btr
    - inserts those records into the cloned file
    - closes both files

    IMPORTANT:
    You must specify the complete path to the directory that contains
    the sample Btrieve data file, 'sample.btr'.  See IMPORTANT, below.

    You can compile and run this program on any of the platforms
    supported by the interface modules.  Platforms are indicated by the
    platform switches listed in 'btrapi.h'.  For MS Windows you should
    make an application that allows standard output via printf().  Note
    that most C/C++ compilers support standard I/O Windows applications.
    For MS Windows NT or OS2, you should make a console application.

    See the prologue in 'btrapi.h' for information on how to select
    a target platform for your application.  You must specify a target
    platform.

****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <btrapi.h>
#include <btrconst.h>

/***************************************************************************
  Constants
****************************************************************************/
/********************************************************************
   IMPORTANT: You should modify the following to specify the
              complete path to 'sample.btr' for your environment.
********************************************************************/
#ifdef BTI_LINUX
#define FILE1_NAME "/usr/local/psql/data/samples/sample.btr"
#else
#ifdef BTI_NLM
#define FILE1_NAME "sys:\\pvsw\\samples\\sample.btr"
#else
#define FILE1_NAME "c:\\pvsw\\samples\\sample.btr"
#endif
#endif

#ifdef BTI_LINUX
#define FILE2_NAME "/usr/local/psql/data/samples/sample2.btr"
#else
#ifdef BTI_NLM
#define FILE2_NAME "sys:\\pvsw\\samples\\sample2.btr"
#else
#define FILE2_NAME "c:\\pvsw\\samples\\sample2.btr"
#endif
#endif

/* XXX FIXME */
#undef FILE1_NAME
#define FILE1_NAME "sample.btr"

#define EXIT_WITH_ERROR     1
#define TRUE                1
#define FALSE               0
#define VERSION_OFFSET      0
#define REVISION_OFFSET     2
#define PLATFORM_ID_OFFSET  4
#define MY_THREAD_ID        50

/* Don't pad our structures */
#if defined(__BORLANDC__)
  #pragma option -a-
#else
  #if defined(_MSC_VER) || defined(__WATCOMC__) || defined(BTI_LINUX)
    #pragma pack(1)
  #endif
#endif

/***************************************************************************
  Type definitions for Client ID and version Structures
****************************************************************************/
typedef struct
{
  BTI_CHAR networkAndNode[12];
  BTI_CHAR applicationID[2];
  BTI_WORD threadID;
} CLIENT_ID;

typedef struct
{
  BTI_SINT  Version;
  BTI_SINT  Revision;
  BTI_CHAR  MKDEId;
} VERSION_STRUCT;

/***************************************************************************
  Definition of record from 'sample.btr'
****************************************************************************/
typedef struct
{
  BTI_LONG  ID;
  BTI_CHAR  FirstName[16];
  BTI_CHAR  LastName[26];
  BTI_CHAR  Street[31];
  BTI_CHAR  City[31];
  BTI_CHAR  State[3];
  BTI_CHAR  Zip[11];
  BTI_CHAR  Country[21];
  BTI_CHAR  Phone[14];
} PERSON_STRUCT;

/***************************************************************************
  Type definitions for Stat/Create structure
****************************************************************************/
typedef struct
{
  BTI_SINT recLength;
  BTI_SINT pageSize;
  BTI_SINT indexCount;
  BTI_CHAR reserved[4];
  BTI_SINT flags;
  BTI_BYTE dupPointers;
  BTI_BYTE notUsed;
  BTI_SINT allocations;
} FILE_SPECS;

typedef struct
{
  BTI_SINT position;
  BTI_SINT length;
  BTI_SINT flags;
  BTI_CHAR reserved[4];
  BTI_CHAR type;
  BTI_CHAR null;
  BTI_CHAR notUsed[2];
  BTI_BYTE manualKeyNumber;
  BTI_BYTE acsNumber;
} KEY_SPECS;

typedef struct
{
  FILE_SPECS fileSpecs;
  KEY_SPECS  keySpecs[5];
} FILE_CREATE_BUF;

/***************************************************************************
  Structure type definitions for Get Next Extended operation
****************************************************************************/
typedef struct
{
  BTI_SINT    descriptionLen;
  BTI_CHAR    currencyConst[2];
  BTI_SINT    rejectCount;
  BTI_SINT    numberTerms;
} GNE_HEADER;

typedef struct
{
  BTI_CHAR    fieldType;
  BTI_SINT    fieldLen;
  BTI_SINT    fieldOffset;
  BTI_CHAR    comparisonCode;
  BTI_CHAR    connector;
  BTI_CHAR value[3];
} TERM_HEADER;

typedef struct
{
  BTI_SINT    maxRecsToRetrieve;
  BTI_SINT    noFieldsToRetrieve;
} RETRIEVAL_HEADER;

typedef struct
{
  BTI_SINT    fieldLen;
  BTI_SINT    fieldOffset;
} FIELD_RETRIEVAL_HEADER;

typedef struct
{
  GNE_HEADER              gneHeader;
  TERM_HEADER             term1;
  TERM_HEADER             term2;
  RETRIEVAL_HEADER        retrieval;
  FIELD_RETRIEVAL_HEADER  recordRet;
} PRE_GNE_BUFFER;

typedef struct
{
  BTI_SINT      recLen;
  BTI_LONG      recPos;
  PERSON_STRUCT personRecord;
} RETURNED_REC;

typedef struct
{
  BTI_SINT      numReturned;
  RETURNED_REC  recs[20];
} POST_GNE_BUFFER;

typedef union
{
  PRE_GNE_BUFFER  preBuf;
  POST_GNE_BUFFER postBuf;
} GNE_BUFFER, BTI_FAR* GNE_BUFFER_PTR;

/* restore structure packing */
#if defined(__BORLANDC__)
  #pragma option -a.
#else
  #if defined(_MSC_VER) || defined(__WATCOMC__) || defined(BTI_LINUX)
    #pragma pack()
  #endif
#endif

/***************************************************************************
  Main
****************************************************************************/
int main(void)
{
  /* Btrieve function parameters */
  BTI_BYTE posBlock1[128];
  BTI_BYTE posBlock2[128];
  BTI_BYTE dataBuf[255];
  BTI_WORD dataLen;
  BTI_BYTE keyBuf1[255];
  BTI_BYTE keyBuf2[255];
  BTI_WORD keyNum = 0;

  BTI_BYTE btrieveLoaded = FALSE;
  BTI_LONG personID;
  BTI_BYTE file1Open = FALSE;
  BTI_BYTE file2Open = FALSE;
  BTI_SINT status;
  BTI_SINT getStatus = -1;
  BTI_SINT i;
  BTI_SINT posCtr;

  CLIENT_ID       clientID;
  VERSION_STRUCT  versionBuffer[3];
  FILE_CREATE_BUF fileCreateBuf;
  GNE_BUFFER_PTR  gneBuffer;
  PERSON_STRUCT   personRecord;

  printf("**************** Btrieve C/C++ Interface Demo ****************\n\n");

  /* set up the Client ID */
  memset(clientID.networkAndNode, 0, sizeof(clientID.networkAndNode));
  memcpy(clientID.applicationID, "MT", 2);  /* must be greater than "AA" */
  clientID.threadID = MY_THREAD_ID;

  memset(versionBuffer, 0, sizeof(versionBuffer));
  dataLen = sizeof(versionBuffer);

  status = BTRVID(
              B_VERSION,
              posBlock1,
              &versionBuffer,
              &dataLen,
              keyBuf1,
              keyNum,
              (BTI_BUFFER_PTR)&clientID);

  if (status == B_NO_ERROR)
  {
    printf("Btrieve Versions returned are: \n");
    for (i = 0; i < 3; i++) {
      if (versionBuffer[i].Version != 0)
      {
        printf("   %d.%d %c\n", versionBuffer[i].Version,
                versionBuffer[i].Revision,
                versionBuffer[i].MKDEId);
      }
    }
    printf("\n");
    btrieveLoaded = TRUE;
  }
  else
  {
    printf("Btrieve B_VERSION status = %d\n", status);
    if (status != B_RECORD_MANAGER_INACTIVE)
    {
      btrieveLoaded = TRUE;
    }
  }

  /* clear buffers */
  if (status == B_NO_ERROR)
  {
    memset(dataBuf, 0, sizeof(dataBuf));
    memset(keyBuf1, 0, sizeof(keyBuf1));
    memset(keyBuf2, 0, sizeof(keyBuf2));
  }

  /* open sample.btr */
  if (status == B_NO_ERROR)
  {

    strcpy((BTI_CHAR *)keyBuf1, FILE1_NAME);
    strcpy((BTI_CHAR *)keyBuf2, FILE2_NAME);

    keyNum  = 0;
    dataLen = 0;

    status = BTRVID(
                B_OPEN,
                posBlock1,
                dataBuf,
                &dataLen,
                keyBuf1,
                keyNum,
                (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_OPEN status (sample.btr) = %d\n", status);
    if (status == B_NO_ERROR)
    {
      file1Open = TRUE;
    }
  }

  /* get the record with key 0 = 263512477 using B_GET_EQUAL */
  if (status == B_NO_ERROR)
  {
    memset(&personRecord, 0, sizeof(personRecord));
    dataLen = sizeof(personRecord);
    personID = 263512477;    /* this is really a social security number */
    *(BTI_LONG BTI_FAR *)&keyBuf1[0] = personID;
    keyNum = 0;

    status = BTRVID(
                B_GET_EQUAL,
                posBlock1,
                &personRecord,
                &dataLen,
                keyBuf1,
                keyNum,
                (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_GET_EQUAL status = %d\n", status);
    if (status == B_NO_ERROR)
    {
      printf("\n");
      printf("The retrieved record is:\n");
      printf("ID:      %ld\n", personRecord.ID);
      printf("Name:    %s %s\n", personRecord.FirstName,
                                 personRecord.LastName);
      printf("Street:  %s\n", personRecord.Street);
      printf("City:    %s\n", personRecord.City);
      printf("State:   %s\n", personRecord.State);
      printf("Zip:     %s\n", personRecord.Zip);
      printf("Country: %s\n", personRecord.Country);
      printf("Phone:   %s\n", personRecord.Phone);
      printf("\n");
    }
  }

  /* perform a stat operation to populate the create buffer */
  memset(&fileCreateBuf, 0, sizeof(fileCreateBuf));
  dataLen = sizeof(fileCreateBuf);
  keyNum = (BTI_WORD)-1;

  status = BTRVID(B_STAT,
                posBlock1,
                &fileCreateBuf,
                &dataLen,
                keyBuf1,
                keyNum,
                (BTI_BUFFER_PTR)&clientID);

  if (status == B_NO_ERROR)
  {
    /* create and open sample2.btr */
    keyNum = 0;
    dataLen = sizeof(fileCreateBuf);
    status = BTRVID(B_CREATE,
                  posBlock2,
                  &fileCreateBuf,
                  &dataLen,
                  keyBuf2,
                  keyNum,
                  (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_CREATE status = %d\n", status);
  }

  if (status == B_NO_ERROR)
  {
    keyNum = 0;
    dataLen = 0;

    status = BTRVID(
                B_OPEN,
                posBlock2,
                dataBuf,
                &dataLen,
                keyBuf2,
                keyNum,
                (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_OPEN status (sample2.btr) = %d\n", status);
    if (status == B_NO_ERROR)
    {
      file2Open = TRUE;
    }
  }

  /* now extract data from the original file, insert into new one */
  if (status == B_NO_ERROR)
  {
    /* getFirst to establish currency */
    keyNum = 2; /* STATE-CITY index */
    memset(&personRecord, 0, sizeof(personRecord));
    memset(&keyBuf2[0], 0, sizeof(keyBuf2));
    dataLen = sizeof(personRecord);

    getStatus = BTRVID(
                   B_GET_FIRST,
                   posBlock1,
                   &personRecord,
                   &dataLen,
                   keyBuf1,
                   keyNum,
                   (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_GET_FIRST status (sample.btr) = %d\n\n", getStatus);
  }

  gneBuffer = malloc(sizeof(GNE_BUFFER));
  if (gneBuffer == NULL)
  {
     printf("Insufficient memory to allocate buffer");
    return(EXIT_WITH_ERROR);
  }
  memset(gneBuffer, 0, sizeof(GNE_BUFFER));
  memcpy(&gneBuffer->preBuf.gneHeader.currencyConst[0], "UC", 2);
  while (getStatus == B_NO_ERROR)
  {
    gneBuffer->preBuf.gneHeader.rejectCount = 0;
    gneBuffer->preBuf.gneHeader.numberTerms = 2;
    posCtr = sizeof(GNE_HEADER);

    /* fill in the first condition */
    gneBuffer->preBuf.term1.fieldType = 11;
    gneBuffer->preBuf.term1.fieldLen = 3;
    gneBuffer->preBuf.term1.fieldOffset = 108;
    gneBuffer->preBuf.term1.comparisonCode = 5;
    gneBuffer->preBuf.term1.connector = 1;
    memcpy(&gneBuffer->preBuf.term1.value[0], "CA", 2);
    posCtr += sizeof(TERM_HEADER);

    /* fill in the second condition */
    gneBuffer->preBuf.term2.fieldType = 11;
    gneBuffer->preBuf.term2.fieldLen = 3;
    gneBuffer->preBuf.term2.fieldOffset = 108;
    gneBuffer->preBuf.term2.comparisonCode = 6;
    gneBuffer->preBuf.term2.connector = 0;
    memcpy(&gneBuffer->preBuf.term2.value[0], "TX", 2);
    posCtr += sizeof(TERM_HEADER);

    /* fill in the projection header to retrieve whole record */
    gneBuffer->preBuf.retrieval.maxRecsToRetrieve = 20;
    gneBuffer->preBuf.retrieval.noFieldsToRetrieve = 1;
    posCtr += sizeof(RETRIEVAL_HEADER);
    gneBuffer->preBuf.recordRet.fieldLen = sizeof(PERSON_STRUCT);
    gneBuffer->preBuf.recordRet.fieldOffset = 0;
    posCtr += sizeof(FIELD_RETRIEVAL_HEADER);
    gneBuffer->preBuf.gneHeader.descriptionLen = posCtr;

    dataLen = sizeof(GNE_BUFFER);
    getStatus = BTRVID(
                   B_GET_NEXT_EXTENDED,
                   posBlock1,
                   gneBuffer,
                   &dataLen,
                   keyBuf1,
                   keyNum,
                   (BTI_BUFFER_PTR)&clientID);

     printf("Btrieve B_GET_NEXT_EXTENDED status = %d\n", getStatus);

    /* Get Next Extended can reach end of file and still return some records */
    if ((getStatus == B_NO_ERROR) || (getStatus == B_END_OF_FILE))
    {
      printf("GetNextExtended returned %d records.\n", gneBuffer->postBuf.numReturned);
      for (i = 0; i < gneBuffer->postBuf.numReturned; i++)
      {
        dataLen = sizeof(PERSON_STRUCT);
        memcpy(dataBuf, &gneBuffer->postBuf.recs[i].personRecord, dataLen);
        status = BTRVID(
                    B_INSERT,
                    posBlock2,
                    dataBuf,
                    &dataLen,
                    keyBuf2,
                    -1,   /* no currency change */
                    (BTI_BUFFER_PTR)&clientID);
      }
      printf("Inserted %d records in new file, status = %d\n\n", gneBuffer->postBuf.numReturned, status);
    }
    memset(gneBuffer, 0, sizeof(GNE_BUFFER));
    memcpy(&gneBuffer->preBuf.gneHeader.currencyConst[0], "EG", 2);
  }

  free(gneBuffer);
  gneBuffer = NULL;

  /* close open files */
  if (file1Open)
  {
    dataLen = 0;

    status = BTRVID(
                B_CLOSE,
                posBlock1,
                dataBuf,
                &dataLen,
                keyBuf1,
                keyNum,
                (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_CLOSE status (sample.btr) = %d\n", status);
  }

  if (file2Open)
  {
    dataLen = 0;

    status = BTRVID(
                B_CLOSE,
                posBlock2,
                dataBuf,
                &dataLen,
                keyBuf2,
                keyNum,
                (BTI_BUFFER_PTR)&clientID);

    printf("Btrieve B_CLOSE status (sample2.btr) = %d\n", status);
  }

  /**********************************************************************
     ISSUE THE BTRIEVE STOP OPERATION - For multi-tasking environments,
     such as MS Windows, OS2, and NLM, 'stop' frees all Btrieve resources
     for this client.  In DOS and Extended DOS, it removes the Btrieve
     engine from memory, which we choose not to do in this example.  In
     multi-tasking environments, the engine will not unload on 'stop'
     unless it has no more clients.
  ***********************************************************************/
  #if !defined(BTI_DOS) && !defined(BTI_DOS_32R) && \
  !defined(BTI_DOS_32B) && !defined(BTI_DOS_32P)
  if (btrieveLoaded)
  {
    dataLen = 0;
    status = BTRVID(B_STOP, posBlock1, dataBuf, &dataLen, keyBuf1, keyNum,
             (BTI_BUFFER_PTR)&clientID);
    printf("Btrieve STOP status = %d\n", status);
    if (status != B_NO_ERROR)
    {
       status = EXIT_WITH_ERROR;
    }
  }
  #endif

  return(status);
}
