/**
 * Sample program that reads the number of tags requested
 * and prints the tags. It is currently not supporting streaming.
 * stop trigger.
 * @file readstopTrigger.c 
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
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


int main(int argc, char *argv[])
{
  TMR_Reader r, *rp;
  TMR_Status ret;
  TMR_Region region;
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

{
  TMR_ReadPlan simpleReadPlan, multiReadPlan;
  TMR_ReadPlan subplans[5];
  TMR_ReadPlan* subplanPtrs[5];
  TMR_TagProtocolList value;
  TMR_TagProtocol valueList[32];
  int subplanCount = 0;
  int j;
  TMR_SR_GEN2_Q qValue;
  uint8_t antenna[] = {1};
  uint32_t tagCount = 1;

  /* Initial test setup */
  /* Set the Q value as per the tag population */
  qValue.type = TMR_SR_GEN2_Q_STATIC;
  qValue.u.staticQ.initialQ = 0;

  ret = TMR_paramSet(rp, TMR_PARAM_GEN2_Q, &qValue);
  checkerr(rp, ret, 1, "setting the Q Value");  

  /* The simple read plan */
  TMR_RP_init_simple (&simpleReadPlan, 1, antenna, TMR_TAG_PROTOCOL_GEN2, 1000);

  /* Stop N trigger */
  TMR_RP_set_stopTrigger (&simpleReadPlan, tagCount);

  /* The multi read plan */
  {

    value.max = 32;
    value.list = valueList;

    /* Berfore setting the readplen, we must get list of supported protocols */
    ret = TMR_paramGet(rp, TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS, &value);
    checkerr(rp, ret, 1, "Getting the supported protocols");

    {
      ret = TMR_RP_init_simple(&subplans[subplanCount], 1, antenna,TMR_TAG_PROTOCOL_GEN2 , 1000);
      TMR_RP_set_stopTrigger (&subplans[subplanCount], tagCount);
      subplanCount++;
      ret = TMR_RP_init_simple(&subplans[subplanCount], 1, antenna, TMR_TAG_PROTOCOL_ISO180006B, 1000);
      TMR_RP_set_stopTrigger (&subplans[subplanCount], tagCount);
      subplanCount++;

    }

    for (j = 0; j < subplanCount; j++)
    {
      subplanPtrs[j] = &subplans[j];
    }

    TMR_RP_init_multi(&multiReadPlan, subplanPtrs, subplanCount, 0);
  }

  /* Set the read plan */
   ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &simpleReadPlan);
  //enable this for multi read plan
  //ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &multiReadPlan);
  checkerr(rp, ret, 1, "setting read plan");

    /* The sync read */
  {
    ret = TMR_read(rp, 1000, NULL);
    checkerr(rp, ret, 1, "reading tags");

    while (TMR_SUCCESS == TMR_hasMoreTags(rp))
    {
      TMR_TagReadData trd;
      char epcStr[128];

      ret = TMR_getNextTag(rp, &trd);
      checkerr(rp, ret, 1, "fetching tag");

      TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epcStr);
      printf("EPC:%s\n", epcStr);
    }
  }
}

TMR_destroy(rp);
return 0;
}

