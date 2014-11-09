/**
 * Sample program that read tags in the background and track tags
 * that have been seen and toggles the GPIO when the referenced tag is found.
 * Currently GPIO cannot be handled in the read listener due to a deadlock 
 * problem in Mercury API. This may be fixed in the future, but for now, GPIO 
 * must be handled in a separate thread.
 * @file readasyncGPIOToggle.c
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
#define USE_TRANSPORT_LISTENER 0
#endif

pthread_mutex_t tagDB_lock;
pthread_t backgroundParser;
static bool exitThread = false;
struct tagdb_table
{
  char  *epc;
  struct tagdb_table *next;
};
struct tagdb_table *head = NULL;
struct tagdb_table *curr = NULL;

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

/* helper function to add the new node at end of the list */
void append(char *epc)
{
  struct tagdb_table *temp;
  /* create a new node */
  temp = (struct tagdb_table *)malloc(sizeof(struct tagdb_table));
  temp->epc = my_strdup(epc);
  temp->next = NULL;

  /* add the new node at the end of list */
  curr->next = temp;
  curr = temp;
}

void add(char *epc)
{
  struct tagdb_table *temp;
  temp = (struct tagdb_table *)malloc(sizeof(struct tagdb_table));
  temp->epc = my_strdup(epc);
  temp->next = NULL;
  if (NULL == head)
  {
    head = curr = temp;
  }
}

/* helper function to add the new node to the list */
void db_insert(char *epc)
{
  struct tagdb_table *temp;
  temp = head;
  if(NULL == temp)
  {
    add(epc);
  }
  else
  {
    append(epc);
  }
}

/**
 * helper function to search the given epc in the list. It returns NULL
 * if the tag is not found. If found, it returns pointer to the found link.
 */ 
struct tagdb_table* search_in_list(char *epc, struct tagdb_table **prev)
{
  struct tagdb_table *ptr = head;
  struct tagdb_table *tmp = NULL;
  bool found = false;

  while(NULL != ptr)
  {
    if(0 == strcmp(epc, ptr->epc))
    {
      found = true;
      break;
    }
    else
    {
      tmp = ptr;
      ptr = ptr->next;
    }
  }

  if(true == found)
  {
    if(prev)
      *prev = tmp;
    return ptr;
  }
  else
  {
    return NULL;
  }
}

/* helper function to remove given epc from the list */
int delete_from_list(char *epc)
{
  struct tagdb_table *prev = NULL;
  struct tagdb_table *del = NULL;

  del = search_in_list(epc, &prev);
  if(NULL == del)
  {
    return -1;
  }
  else
  {
    if(NULL != prev)
      prev->next = del->next;

    if(del == curr)
    {
      curr = prev;
    }
    else if(del == head)
    {
      head = del->next;
    }
  }

  free(del);
  del = NULL;

  return 0;
}

void db_free(struct tagdb_table *db)
{
  struct tagdb_table *list, *temp;

  if (NULL == db)
  {
    return;
  }
  else
  {
    list = db;
    while (NULL != list)
    {
      temp = list;
      list = list->next;
      free(temp->epc);
      free(temp);
    }
  }
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
void startBackgroundThread(TMR_Reader *reader, pthread_t *parser);

static void*
backgroundParserRoutine(void *arg)
{
  struct tagdb_table *tagDB;
  TMR_Reader *reader;
  TMR_Status ret;
  char *targetEpc = "112233445566DEADBEAF";
  char epc[256];
  TMR_GpioPin *state;
  uint8_t stateCount;
  stateCount = 1;
  state = malloc(stateCount * sizeof(*state));


  reader = (TMR_Reader *)arg;

  while (1)
  {
    /* look for a record in list */
    pthread_mutex_lock(&tagDB_lock);
    if (NULL != head)
    {
      tagDB = (struct tagdb_table *)head;
      while (NULL != tagDB)
      {
        strcpy(epc, tagDB->epc);
        pthread_mutex_unlock(&tagDB_lock);

        /* compare against targer EPC */
        if (0 == strcmp(targetEpc, epc))
        {
          /* tag matches */
          printf("Found TagID:%s\n", epc);

          pthread_mutex_lock(&tagDB_lock);
          delete_from_list(epc);
          pthread_mutex_unlock(&tagDB_lock);

          /* toggle the GPIO */
          state[0].id = 1;
          state[0].high = true;
          printf("Sound ALARM for ID: %s\n", epc);
          ret = TMR_gpoSet(reader, stateCount, state);
          if (TMR_SUCCESS != ret)
          {
            printf("Error setting GPIO pins: %s\n", TMR_strerr(reader, ret));
          }

          tmr_sleep(1000);

          state[0].id = 1;
          state[0].high = false;
          printf("ALARM OFF for ID: %s ...wait\n", epc);
          ret = TMR_gpoSet(reader, stateCount, state);
          if (TMR_SUCCESS != ret)
          {
            printf("Error setting GPIO pins: %s\n", TMR_strerr(reader, ret));
          }

          tmr_sleep(500);

         break;

        }
        else
        {
          tagDB = tagDB->next;
        }
      }
    }
    pthread_mutex_unlock(&tagDB_lock);
    if (exitThread)
    {
      /* time to exit */
      pthread_exit(NULL);
    }
  }/* End of while loop */
  return NULL;
}

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

  pthread_mutex_init(&tagDB_lock, NULL);
  startBackgroundThread(rp, &backgroundParser);

  ret = TMR_startReading(rp);
  checkerr(rp, ret, 1, "starting reading");
#ifndef WIN32
  sleep(10);
#else
  Sleep(1000);
#endif

  ret = TMR_stopReading(rp);
  checkerr(rp, ret, 1, "stopping reading");

  ret = TMR_removeReadListener(rp, &rlb);
  exitThread = true;
  /* wait for the thread to exit */
  pthread_join(backgroundParser, NULL);
  exitThread = false;
  db_free(head);
  TMR_destroy(rp);
  return 0;

#endif /* TMR_ENABLE_BACKGROUND_READS */
}


void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[128];
  static uint64_t totalTagCount;

  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  totalTagCount += t->readCount;
  pthread_mutex_lock(&tagDB_lock);
  db_insert(epcStr);
  pthread_mutex_unlock(&tagDB_lock);
}

void
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}

/**
 * helper function to create the parser thread. It accepts a 
 * reference to reader object and reference to parser thread to be
 * created.
 */ 
void startBackgroundThread(TMR_Reader *reader, pthread_t *parser)
{
  TMR_Status ret = TMR_SUCCESS;

  ret = pthread_create(parser, NULL, backgroundParserRoutine, reader);
  if (0 != ret)
  {
    printf("No threads\n");
    return;
  }
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

