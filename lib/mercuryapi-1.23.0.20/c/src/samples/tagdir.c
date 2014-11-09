/** Sample program that reads tags in the background
 * and calculates direction of travel.
 * Assumes antennas 1 and 2 are connected and facing in opposing directions.
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef WIN32
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#endif

/* Enable this to use transportListener */
#ifndef USE_TRANSPORT_LISTENER
#define USE_TRANSPORT_LISTENER 0
#endif

#ifndef WIN32
int _interrupted = 0;
void
signal_interrupt_handler(int signum)
{
  _interrupted = 1;
}


int
getcharNonblocking(void)
{
  struct timeval tv = { 0L, 0L };
  int kbhit;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  kbhit = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

  if (!kbhit)
  {
    return EOF;
  }
  else
  {
    return getchar();
  }
}
#endif

int _graph_enable = 1;
int _json_enable = 0;
int _print_enable = 0;

/** JSON-formatting macros insert JSON text into a character buffer
 *
 * JSON_START(bufsize) allocates and initializes the necessary state
 * JSON_END() closes out state
 * JSON_PRINT(format, args) adds text, printf-style
 *   e.g., PRINT("{") adds "{"
 * JSON_OPTCOMMA() adds comma, except for first time.  Call before every element.
 * JSON_RESET_OPTCOMMA() resets JSON_OPTCOMMA element count
 * JSON_VALUE(name, format, args) adds formatted value
 *   e.g., JSON_VALUE("power", "%d", 123) adds "\"power\":123"
 * JSON_STRING(name, format, args) adds quoted formatted value
 *   e.g., JSON_VALUE("power", "%d", 123) adds "\"power\":123"
 * JSON_CONTENT is a string containing the formatted JSON content
 */

#define JSON_START(bufsize) {                   \
  char JSON_buf[bufsize];                       \
  char* JSON_pos = JSON_buf;                    \
  char* JSON_end = JSON_buf + sizeof(JSON_buf); \
  int JSON_elt = 0; 
#define JSON_PRINT(...) { JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, ## __VA_ARGS__); }
#define JSON_OPTCOMMA()                                                      \
  {                                                                     \
    if (0 < JSON_elt) { JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, ","); } \
    JSON_elt++;                                                         \
  }
#define JSON_RESET_OPTCOMMA() { JSON_elt = 0; }
#define JSON_VALUE(name, ...)                                                \
  {                                                                     \
    JSON_OPTCOMMA();                                                         \
    JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, "\"" name "\":"); \
    JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, ## __VA_ARGS__);  \
  }
#define JSON_STRING(name, ...)                                               \
  {                                                                     \
    JSON_OPTCOMMA();                                                         \
    JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, "\"" name "\":\""); \
    JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, ## __VA_ARGS__);  \
    JSON_pos += snprintf(JSON_pos, JSON_end-JSON_pos, "\"");            \
  }
#define JSON_END() }
#define JSON_CONTENT (JSON_buf)

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

typedef struct TagState
{
  /* Ring buffer of last 10 data values,
   * used to calculate moving average */
  int valData[10];
  int valOffset;
  int valTotal;
  int valN;
  /* Moving average */
  float avg;

  /* Multi-tag state */
  int id;
  int order;  /* Display order */
  char name[64];
  struct TagState* next;
} TagState;
void
TS_init(TagState* self)
{
  memset(self->valData, 0, sizeof(self->valData));
  self->valOffset = -1;
  self->valTotal = 0;
  self->valN = sizeof(self->valData) / sizeof(self->valData[0]);

  self->id = 0;
  self->name[0] = '\0';
  self->next = NULL;
}
void
TS_addVal(TagState* self, int value)
{
  int newOffset = (self->valOffset + 1) % self->valN;

  /* Subtract old "tail" value */
  self->valTotal -= self->valData[newOffset];
  /* Add new "head" value */
  self->valData[newOffset] = value;
  self->valTotal += self->valData[newOffset];
  self->avg = (float)self->valTotal / (float)self->valN;

  self->valOffset = newOffset;
  
  /*
  printf("RGS{offset:%d, total:%d, N:%d, avg:%f, data:[", self->valOffset, self->valTotal, self->valN, self->avg);
  { int i; for (i=0; i<self->valN; i++) {
      printf(" %d", self->valData[i]); }}
  printf("]\n");
  */
}
/** Create a new, initialized TagState structure */
TagState* TS_new(void)
{
  TagState* node = (TagState*)malloc(sizeof(TagState));
  if (NULL != node)
  {
    TS_init(node);
  }
  return node;
}
/** Search for a particular entry in a list, creating it if not found
 * @param ppNode Pointer to a node pointer; e.g., if NULL==*ppNode, we can create a new node with *ppNode=TS_new()
 * @param id Unique key for looking up node
 * @param name Human-readable name for node
 */
static TagState*
TS_find(TagState** ppNode, int id, const char* name)
{
  int iNode = 0;
  for (; NULL != *ppNode; ppNode = &((*ppNode)->next))
  {
    if (id == (*ppNode)->id)
    {
      return *ppNode;
    }
    iNode++;
  }

  /* Node not found, add a new one */
  TagState* newNode = TS_new();
  newNode->id = id;
  newNode->order = iNode;
  strncpy(newNode->name, name, sizeof(newNode->name));
  *ppNode = newNode;

  return *ppNode;
}


/** Dump the contents of a TagState list */
void
TS_dumpJson(TagState* ts)
{
  /*
   * Locking is not necessary because we can never be reading an invalid state
   *
   *  * Nodes are only added, never removed.  At worst, we'll miss a node
   *    being added and catch up on the next cycle.
   *
   *  * Node fields are always in a valid state.  TS_find initializes them
   *    before adding to the list.  After that, name and order never change.
   *
   *  * The avg field does change, but we don't really care if we're a cycle behind.
   *    * It would be a different story if we needed to do multiple reads;
   *      i.e., valTotal / valN. Then they might be out of sync if we read
   *      in the middle of a write sequence.  (Note: I'm not 100% positive
   *      that a float is written atomically.  If not, we're in trouble, but for now,
   *      it's not critical enough to deal with.)
   */

  int i;

  JSON_START(8*1024);
  JSON_PRINT("Content-type: application/json\n\n");
  JSON_PRINT("{\"data\":[\n");

  i = 0;
  for (; ts != NULL; ts = ts->next)
  {
    if (0 < i) { JSON_PRINT(",\n"); }

    JSON_PRINT("{");
    JSON_RESET_OPTCOMMA();
    JSON_STRING("name", "%s", ts->name);
    JSON_VALUE("order", "%d", ts->order);
    JSON_VALUE("avg", "%f", ts->avg);
    JSON_PRINT("}");

    i++;
  }

  JSON_PRINT("\n]}");
  printf("%s\n", JSON_CONTENT);
  JSON_END();
}

typedef struct AppState
{
  TagState* tagdb;
} AppState;
void
AS_init(AppState* self)
{
  self->tagdb = NULL;
}

int
main(int argc, char *argv[])
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
  AppState appState;
  TMR_ReadExceptionListenerBlock reb;
#if use_transport_listener
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
  region = TMR_REGION_NA;
  ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
    checkerr(rp, ret, 1, "setting region");
  }

  rlb.listener = callback;
  AS_init(&appState);
  rlb.cookie = &appState;

  reb.listener = exceptionCallback;
  reb.cookie = NULL;

  ret = TMR_addReadListener(rp, &rlb);
  checkerr(rp, ret, 1, "adding read listener");

  ret = TMR_addReadExceptionListener(rp, &reb);
  checkerr(rp, ret, 1, "adding exception listener");

  {
    TMR_GEN2_Target target = TMR_GEN2_TARGET_AB;
    ret = TMR_paramSet(rp, TMR_PARAM_GEN2_TARGET, &target);
    checkerr(rp, ret, __LINE__, "setting Target AB");
  }
  {
    uint32_t ontime = 5000;
    ret = TMR_paramSet(rp, TMR_PARAM_READ_ASYNCONTIME, &ontime);
    checkerr(rp, ret, __LINE__, "setting async on time");
  }

  ret = TMR_startReading(rp);
  checkerr(rp, ret, 1, "starting reading");

  /* Free run until Enter is pressed */
  //getchar();
#ifndef WIN32
  signal(SIGINT, signal_interrupt_handler);
  signal(SIGQUIT, signal_interrupt_handler);
#endif
  int iters = 0;
  while (!_interrupted)
  {
#ifndef WIN32
    int ch = getcharNonblocking();
    if (ch == 'q') { break; }
    switch (ch)
    {
    case 'g': _graph_enable ^= 1; break;
    case 'j': _json_enable ^= 1; break;
    case 'p': _print_enable ^= 1; break;
    }
#endif
    if (_json_enable) { TS_dumpJson(appState.tagdb); }
    iters++;
    sleep(1);
  }

  ret = TMR_stopReading(rp);
  checkerr(rp, ret, 1, "stopping reading");

  TMR_destroy(rp);

  return 0;

#endif /* TMR_ENABLE_BACKGROUND_READS */
}


bool
isTargetTag(const TMR_TagReadData *trd, void *cookie)
{
  return 1;
  return 1
    && 12 == trd->tag.epcByteCount
    && (0
        || (1
            /* 3028354D82020280000005A0 */
            /* 3028354D82020280000005B2 */
            /* 3028354D82020280000005D7 */
            && 0x30 == trd->tag.epc[ 0]
            && 0x28 == trd->tag.epc[ 1]
            && 0x05 == trd->tag.epc[10]
            && (0
                || 0xA0 == trd->tag.epc[11]
                || 0xB2 == trd->tag.epc[11]
                || 0xD7 == trd->tag.epc[11]
                )
            )
        || (1
            /* 067EFF58B145490000001234 */
            /* 067EFF58B145490000005678 */
            /* 067EFF58B145490000009ABC */
            && 0x06 == trd->tag.epc[0]
            && 0x7E == trd->tag.epc[1]
            )
        )
    ;
}

void
printx(const char* value, int reps)
{
  int i;
  for (i=0; i<reps; i++)
  {
    printf("%s", value);
  }
}

int
snprintx(char* buf, size_t bufsize, const char* value, int reps)
{
  char* pos = buf;
  char* end = buf + bufsize;
  int i;
  for (i=0; i<reps; i++)
  {
    pos += snprintf(pos, end-pos, "%s", value);
  }
  return pos - buf;
}

int
snprint_graph(char* buf, size_t bufsize, const int values[], int width, const char* left, const char* right, const char* divider)
{
  char* pos = buf;
  char* end = pos + bufsize;
  pos += snprintx(pos, end-pos, " ", width - values[0]);
  pos += snprintx(pos, end-pos, left, values[0]);
  pos += snprintx(pos, end-pos, divider, 1);
  pos += snprintx(pos, end-pos, right, values[1]);
  pos += snprintx(pos, end-pos, " ", width - values[1]);
  return pos - buf;
}

void
graph_body(const char* left, const char* right, const char* label, int value,
           const TagState* ts, const TMR_TagReadData *trd, void *cookie)
{
  if (!isTargetTag(trd, cookie)) { return; }

  int values[] = { 0, 0 };
  if (value < 0)
  {
    values[0] = -value;
  }
  else
  {
    values[1] = value;
  }

  char label_first4[4+1];
  char label_last4[4+1];
  char divider[16];
  char buf[256];

  strncpy(label_first4, label, 4);
  label_first4[4] = '\0';
  strncpy(label_last4, label + strlen(label) - 4, 5);
  snprintf(divider, sizeof(divider), "|%s.%s|", label_first4, label_last4);
  snprint_graph(buf, sizeof(buf), values, 20, left, right, divider);

  /* printf("%s %3d", label, value); */
  printx(" ", ts->order * 20);
  printf("%s", buf);
  printf("\n");
}

int min(int a, int b)
{
  return (a<b) ? a : b;
}
int max(int a, int b)
{
  return (a>b) ? a : b;
}

void
graph_callback(TMR_Reader *reader, const TMR_TagReadData *trd, void *cookie)
{
  if (!isTargetTag(trd, cookie)) { return; }

  AppState* appst = (AppState*)cookie;
  int sign;
  int count;
  int rssi;
  int rssicount;
  char epcStr[128];

  switch (trd->antenna)
  {
  case 1:  sign = -1; break;
  case 2:  sign =  1; break;
  default: sign =  0; break;
  }
  count     = sign * trd->readCount;
  rssi      = sign * (trd->rssi + 80) / 5;
  rssicount = sign * count * rssi / 2;

  TMR_bytesToHex(trd->tag.epc, trd->tag.epcByteCount, epcStr);
  TagState* tagst = TS_find(&appst->tagdb, trd->tag.crc, epcStr);

  TS_addVal(tagst, rssicount);

  if (_graph_enable) { graph_body("<", ">", epcStr, tagst->avg, tagst, trd, cookie); }
}

void
print_callback(TMR_Reader *reader, const TMR_TagReadData *trd, void *cookie)
{
  char epcStr[128];
  char timeStr[128];

  if (!isTargetTag(trd, cookie)) { return; }

  TMR_bytesToHex(trd->tag.epc, trd->tag.epcByteCount, epcStr);

#ifdef WIN32
  {
    FILETIME ft;
    SYSTEMTIME st;
    char* timeEnd;
    char* end;
		
    ft.dwHighDateTime = trd->timestampHigh;
    ft.dwLowDateTime = trd->timestampLow;

    FileTimeToLocalFileTime( &ft, &ft );
    FileTimeToSystemTime( &ft, &st );
    timeEnd = timeStr + sizeof(timeStr)/sizeof(timeStr[0]);
    end = timeStr;
    end += sprintf(end, "%d-%d-%d", st.wYear,st.wMonth,st.wDay);
    end += sprintf(end, "T%d:%d:%d", st.wHour,st.wMinute,st.wSecond );
    end += sprintf(end, ".%06d", trd->dspMicros);
    printf("EPC:%s ant:%d count:%d Time:%s\n", epcStr, trd->antenna, trd->readCount, timeStr);
  }
#else
  {
    uint8_t shift;
    uint64_t timestamp;
    time_t seconds;
    int micros;
    char* timeEnd;
    char* end;

    shift = 32;
    timestamp = ((uint64_t)trd->timestampHigh<<shift) | trd->timestampLow;
    seconds = timestamp / 1000;
    micros = (timestamp % 1000) * 1000;

    /*
     * Timestamp already includes millisecond part of dspMicros,
     * so subtract this out before adding in dspMicros again
     */
    micros -= trd->dspMicros / 1000;
    micros += trd->dspMicros;

    timeEnd = timeStr + sizeof(timeStr)/sizeof(timeStr[0]);
    end = timeStr;
    end += strftime(end, timeEnd-end, "%FT%H:%M:%S", localtime(&seconds));
    end += snprintf(end, timeEnd-end, ".%06d", micros);
    end += strftime(end, timeEnd-end, "%z", localtime(&seconds));
    /* printf("EPC:%s ant:%d count:%d Time:%jd.%jd\n", epcStr, trd->antenna, trd->readCount, timestamp/1000, timestamp%1000); */

    JSON_START(256);
    JSON_PRINT("{ ");
    JSON_STRING("epc", "%s", epcStr);
    JSON_VALUE("ant", "%d", trd->antenna);
    JSON_VALUE("count", "%d", trd->readCount);
    JSON_VALUE("phase", "%d", trd->phase);
    JSON_VALUE("rssi", "%d", trd->rssi);
    JSON_VALUE("timestamp", "%jd.%jd", timestamp/1000, timestamp%1000);
    JSON_VALUE("time", "%s", timeStr);
    /* pos += snprintf(pos, end-pos, "\"EPC\":\"%s\" ant:%d count:%d Time:%jd.%jd", epcStr, trd->antenna, trd->readCount, timestamp/1000, timestamp%1000); */
    JSON_PRINT(" }");

    printf("%s\n", JSON_CONTENT);
    JSON_END();
  }
#endif
}

void 
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}

void
callback(TMR_Reader *reader, const TMR_TagReadData *trd, void *cookie)
{
  if (_print_enable) { print_callback(reader, trd, cookie); }
  graph_callback(reader, trd, cookie);
}
