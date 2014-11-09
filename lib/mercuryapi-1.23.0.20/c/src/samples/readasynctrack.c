/**
 * Sample program that reads tags in the background and track tags
 * that have been seen;
 * @file readasynctrack.c
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef WIN32
#include <string.h>
#include <unistd.h>
#endif

/* Enable this to use transportListener */
#ifndef USE_TRANSPORT_LISTENER
#define USE_TRANSPORT_LISTENER 1
#endif

typedef struct tagdb_table
{
  char *epc;
  struct tagdb_table *next;
} tagdb_table;

typedef struct tag_database
{
  int size;            /* the size of the database */
  tagdb_table **table;
} tag_database;

tag_database *seenTags;
uint32_t db_size = 10;

char *my_strdup(const char *string)
{
  char *nstr;
  
  nstr = (char *) malloc(strlen(string) + 1);
  if (nstr)
  {
    strcpy(nstr, string);
  }
  return nstr;
}

tag_database *init_tag_database(uint32_t size)
{
  tag_database *db;
  uint32_t i;
    
  if (size < 1) 
  {
    return NULL;
  }
  
  if ((db = malloc(sizeof(tag_database))) == NULL)
  {
    return NULL;
  }
  
  if ((db->table = malloc(sizeof(tagdb_table *) * size)) == NULL)
  {
    return NULL;
  }
  
  for(i = 0; i < size; i++)
  {
    db->table[i] = NULL;
  }

  db->size = size;
  return db;
}

unsigned int hash(tag_database *db, char *epc)
{
  unsigned int tagHash = 0;
    
  for(; *epc != '\0'; epc++)
  {
    tagHash = *epc + (tagHash << 5) - tagHash;
  }
  return tagHash % db->size;
}

tagdb_table *db_lookup(tag_database *db, char *epc)
{
  tagdb_table *list;
  unsigned int tagHash = hash(db, epc);
  for(list = db->table[tagHash]; list != NULL; list = list->next)
  {
    if (strcmp(epc, list->epc) == 0)
    {
      return list;
    }
  }
  return NULL;
}

TMR_Status db_insert(tag_database *db, char *epc)
{
  tagdb_table *new_list;
  unsigned int tagHash = hash(db, epc);
  
  if ((new_list = malloc(sizeof(tagdb_table))) == NULL)
  {
    return TMR_ERROR_OUT_OF_MEMORY;
  }
  
  /* Insert the record now */
  new_list->epc = my_strdup(epc);
  new_list->next = db->table[tagHash];
  db->table[tagHash] = new_list;
  
  return TMR_SUCCESS;
}

void db_free(tag_database *db)
{
  int i;
  tagdb_table *list, *temp;

  if (db == NULL)
  {
    return;
  }
  
  /**
   * Free the memory for every item in tag_database
   */
  for (i = 0; i < db->size; i++)
  {
    list = db->table[i];
    while (list != NULL)
    {
      temp = list;
      list = list->next;
      free(temp->epc);
      free(temp);
    }
  }
  
  /* Free the table itself */
  free(db->table);
  free(db);
}


void errx(int exitval, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  exit(exitval);
}

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
  if (TMR_SUCCESS != ret)
  {
    errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
  }
}

void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
                   uint32_t timeout, void *cookie)
{
  FILE *out = cookie;
  uint32_t i;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  for (i = 0; i < dataLen; i++)
  {
    if (i > 0 && (i & 15) == 0)
      fprintf(out, "\n         ");
    fprintf(out, " %02x", data[i]);
  }
  fprintf(out, "\n");
}

void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, void *cookie)
{
  FILE *out = cookie;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  fprintf(out, "%s\n", data);
}

void callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie);
void exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie);

int main(int argc, char *argv[])
{

#ifndef TMR_ENABLE_BACKGROUND_READS
  errx(1, "This sample requires background read functionality.\n"
          "Please enable TMR_ENABLE_BACKGROUND_READS in tm_config.h\n"
          "to run this codelet\n");
  return -1;
#else

  TMR_Reader r, *rp;
  TMR_Status ret;
  TMR_Region region;
  TMR_ReadListenerBlock rlb;
  TMR_ReadExceptionListenerBlock reb;
 #if USE_TRANSPORT_LISTENER
  TMR_TransportListenerBlock tb;
#endif
 
  if (argc < 2)
  {
    errx(1, "Please provide reader URL, such as:\n"
           "tmr:///com4\n"
           "tmr://my-reader.example.com\n");
  }
  
  rp = &r;
  ret = TMR_create(rp, argv[1]);
  checkerr(rp, ret, 1, "creating reader");

#if USE_TRANSPORT_LISTENER

  if (TMR_READER_TYPE_SERIAL == rp->readerType)
  {
    tb.listener = serialPrinter;
  }
  else
  {
    tb.listener = stringPrinter;
  }
  tb.cookie = stdout;

  TMR_addTransportListener(rp, &tb);
#endif

  ret = TMR_connect(rp);
  checkerr(rp, ret, 1, "connecting reader");

  region = TMR_REGION_NONE;
  ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
  checkerr(rp, ret, 1, "getting region");

  if (TMR_REGION_NONE == region)
  {
    TMR_RegionList regions;
    TMR_Region _regionStore[32];
    regions.list = _regionStore;
    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
    regions.len = 0;

    ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
    checkerr(rp, ret, __LINE__, "getting supported regions");

    if (regions.len < 1)
    {
      checkerr(rp, TMR_ERROR_INVALID_REGION, __LINE__, "Reader doesn't supportany regions");
    }
    region = regions.list[0];
    ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
    checkerr(rp, ret, 1, "setting region");  
  }

  rlb.listener = callback;
  rlb.cookie = NULL;

  reb.listener = exceptionCallback;
  reb.cookie = NULL;

  ret = TMR_addReadListener(rp, &rlb);
  checkerr(rp, ret, 1, "adding read listener");

  ret = TMR_addReadExceptionListener(rp, &reb);
  checkerr(rp, ret, 1, "adding exception listener");

  seenTags = init_tag_database(db_size);

  ret = TMR_startReading(rp);
  checkerr(rp, ret, 1, "starting reading");
  
#ifndef WIN32
  sleep(1);
#else
  Sleep(1000);
#endif

  ret = TMR_stopReading(rp);
  checkerr(rp, ret, 1, "stopping reading");

  ret = TMR_removeReadListener(rp, &rlb);
  db_free(seenTags);
  TMR_destroy(rp);
  return 0;

#endif /* TMR_ENABLE_BACKGROUND_READS */
}


void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[128];
  TMR_Status ret;
  static int uniqueCount, totalCount;

  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  /**
   * If the tag is not present in the database, only then 
   * insert the tag. 
   */
  if ( NULL == db_lookup(seenTags, epcStr))
  {   
    ret = db_insert(seenTags, epcStr);
    uniqueCount ++;
  }  
  printf("Background read: %s, total tags seen = %d, unique tags seen = %d\n", epcStr, ++totalCount, uniqueCount);
}

void
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}
