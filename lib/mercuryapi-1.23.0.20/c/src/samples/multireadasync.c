/**
 * Sample program that reads tags on multiple readers and prints the tags found.
 * @file multireadasync.c
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef WIN32
#include <unistd.h>
#endif

/* Enable this to use transportListener */
#ifndef USE_TRANSPORT_LISTENER
#define USE_TRANSPORT_LISTENER 0
#endif

typedef struct readerDesc
{
  char* uri;
  int idx;
} readerDesc;

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
  FILE *out = stdout;
  readerDesc *rdp = cookie;
  uint32_t i;

  fprintf(out, "%s %s", rdp->uri, tx ? "Sending: " : "Received:");
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
  FILE *out = stdout;

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

  TMR_Reader *r;
  readerDesc *rd;
  int rcount;
  TMR_Reader *rp;
  TMR_Status ret;
  TMR_Region region;
  TMR_ReadListenerBlock *rlb;
  TMR_ReadExceptionListenerBlock *reb;
#if USE_TRANSPORT_LISTENER
  TMR_TransportListenerBlock *tb;
#endif
  int i;

  if (argc < 2)
  {
    errx(1, "Please provide reader URLs, such as:\n"
           "tmr:///com4\n"
           "tmr://my-reader.example.com\n");
  }
  
  rcount = argc-1;
  r = (TMR_Reader*) calloc(rcount, sizeof(TMR_Reader));
  rd = (readerDesc*) calloc(rcount, sizeof(readerDesc));
  rlb = (TMR_ReadListenerBlock*) calloc(rcount, sizeof(TMR_ReadListenerBlock));
  reb = (TMR_ReadExceptionListenerBlock*) calloc(rcount, sizeof(TMR_ReadExceptionListenerBlock));
  for (i=0; i<rcount; i++)
  {
    rp = &r[i];
    rd[i].uri = argv[i+1];
    ret = TMR_create(rp, rd[i].uri);
    checkerr(rp, ret, 1, "creating reader %s");

    rd[i].idx = i;
    printf("Created reader %d: %s\n", rd[i].idx, rd[i].uri);

#if USE_TRANSPORT_LISTENER

      tb = (TMR_TransportListenerBlock*) calloc(rcount, sizeof(TMR_TransportListenerBlock));

      if (TMR_READER_TYPE_SERIAL == rp->readerType)
      {
        tb[i].listener = serialPrinter;
      }
      else
      {
        tb[i].listener = stringPrinter;
      }
      tb[i].cookie = &rd[i];

      TMR_addTransportListener(rp, &tb[i]);
#endif


    //TMR_SR_PowerMode pm = TMR_SR_POWER_MODE_FULL;
    //ret = TMR_paramSet(rp, TMR_PARAM_POWERMODE, &pm);

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

    rlb[i].listener = callback;
    rlb[i].cookie = &rd[i];

    reb[i].listener = exceptionCallback;
    reb[i].cookie = NULL;

    ret = TMR_addReadListener(rp, &rlb[i]);
    checkerr(rp, ret, 1, "adding read listener");

    ret = TMR_addReadExceptionListener(rp, &reb[i]);
    checkerr(rp, ret, 1, "adding exception listener");

    ret = TMR_startReading(rp);
    checkerr(rp, ret, 1, "starting reading");
  }

#ifndef WIN32
  sleep(5);
#else
  Sleep(5000);
#endif

  for (i=0; i<rcount; i++)
  {
    rp = &r[i];
    ret = TMR_stopReading(rp);
    checkerr(rp, ret, 1, "stopping reading");
    TMR_destroy(rp);
  }
  return 0;

#endif /* TMR_ENABLE_BACKGROUND_READS */
}


void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[128];
  readerDesc *rdp = cookie;

  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  printf("%s: %s\n", rdp->uri, epcStr);
}

void
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}
