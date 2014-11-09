/**
* Sample program that perform multi protocol read
* @file multiprotocolsearch.c
*/
#include "tm_reader.h"
#include "serial_reader_imp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

const char* protocolName(TMR_TagProtocol protocol)
{
	switch (protocol)
	{
	case TMR_TAG_PROTOCOL_NONE:
		return "NONE";
	case TMR_TAG_PROTOCOL_ISO180006B:
		return "ISO180006B";
	case TMR_TAG_PROTOCOL_GEN2:
		return "GEN2";
	case TMR_TAG_PROTOCOL_ISO180006B_UCODE:
		return "ISO180006B_UCODE";
	case TMR_TAG_PROTOCOL_IPX64:
		return "IPX64";
	case TMR_TAG_PROTOCOL_IPX256:
		return "IPX256";
	default:
		return "unknown";
	}
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
#define SUBPLAN_MAX (5)
    TMR_ReadPlan multiplan;
    TMR_ReadPlan subplans[SUBPLAN_MAX];
    TMR_ReadPlan* subplanPtrs[SUBPLAN_MAX];
    TMR_TagProtocolList value;
    TMR_TagProtocol valueList[32];
    int subplanCount = 0;
    int j;

    value.max = 32;
    value.list = valueList;

    /* Berfore setting the readplen, we must get list of supported protocols */
    ret = TMR_paramGet(rp, TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS, &value);
    checkerr(rp, ret, 1, "Getting the supported protocols");

    for (j = 0; j < value.len && j < value.max; j++)
    {
      ret = TMR_RP_init_simple(&subplans[subplanCount++], 0, NULL, value.list[j], 0);
    }

    {
      int i;
      for (i=0; i<subplanCount; i++)
      {
        subplanPtrs[i] = &subplans[i];
      }
    }
    ret = TMR_RP_init_multi(&multiplan, subplanPtrs, subplanCount, 0);
    checkerr(rp, ret, 1, "creating multi read plan");
    ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &multiplan);
    checkerr(rp, ret, 1, "setting read plan");

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
      printf("%s %s\n", protocolName(trd.tag.protocol), epcStr);
    }
  }

  TMR_destroy(rp);
  return 0;
}

