/**
 * Sample program that uses custom transport scheme.
 * It shows the usage of tcp serial transport scheme.
 * @file readcustomtransport.c
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef WIN32
#include <unistd.h>
#endif

#if WIN32
#define snprintf sprintf_s
#endif

/* Enable this to use transportListener */
#ifndef USE_TRANSPORT_LISTENER
#define USE_TRANSPORT_LISTENER 0
#endif

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
    {
      fprintf(out, "\n         ");
    }
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
#if USE_TRANSPORT_LISTENER
  TMR_TransportListenerBlock tb;
#endif
  TMR_ReadListenerBlock rlb;
  TMR_ReadExceptionListenerBlock reb;
 
  if (argc < 2)
  {
    errx(1, "Please provide reader URL, such as:\n"
        "customschemename://readerIP:portname\n");
  }

  rp = &r;

  /**
   * Add the custom transport scheme before calling TMR_create().
   * This can be done by using C API helper function TMR_setSerialTransport().
   * It accepts two arguments. scheme and nativeInit.
   * scheme: the custom transport scheme name. For demonstration using scheme as "tcp".
   * nativeInit: reference to the transport factory init function.
   */
  ret = TMR_setSerialTransport("tcp", &TMR_SR_SerialTransportTcpNativeInit);
  checkerr(rp, ret, 1, "adding the custom transport scheme");

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

  {
    /* Initiate a sync read */
    printf("\nDoing a sync read for 1sec duration\n");
    ret = TMR_read(rp, 1000, NULL);
    if (TMR_SUCCESS != ret)
    {
      fprintf(stderr, "Error reading tags: %s\n", TMR_strerr(rp, ret));
      /* Don't exit, tags might still have been read before the error occurred. */
    }

    while (TMR_SUCCESS == TMR_hasMoreTags(rp))
    {
      TMR_TagReadData trd;
      char epcStr[128];

      ret = TMR_getNextTag(rp, &trd);
      checkerr(rp, ret, 1, "fetching tag");

      TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epcStr);
      printf("Background read: %s Protocol:%0x \n", epcStr, trd.tag.protocol);
    }
  }

  {
    /* Initiate an async read */
    printf("\nDoing an async read for 1sec duration\n");

    rlb.listener = callback;
    rlb.cookie = NULL;

    reb.listener = exceptionCallback;
    reb.cookie = NULL;

    ret = TMR_addReadListener(rp, &rlb);
    checkerr(rp, ret, 1, "adding read listener");

    ret = TMR_addReadExceptionListener(rp, &reb);
    checkerr(rp, ret, 1, "adding exception listener");

    ret = TMR_startReading(rp);
    checkerr(rp, ret, 1, "starting reading");

    tmr_sleep(1000);

    ret = TMR_stopReading(rp);
    checkerr(rp, ret, 1, "stopping reading");
  }

  TMR_destroy(rp);
  return 0;
#endif /* TMR_ENABLE_BACKGROUND_READS */
}

void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[128];

  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  printf("Background read: %s Protocol:%0x \n", epcStr, t->tag.protocol);
  tmr_sleep(100);
}

void 
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}
