/**
 * Sample program that reads tags for a fixed period of time (500ms)
 * and prints the tags found.
 * @file LicenseKey.c
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "serial_reader_imp.h"

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

  // Set license key
  {
    uint8_t keyData[] = {
      /* Replace following value with actual license key */
      0x12, 0x34, 0x56, 0x78
    };
    TMR_uint8List key;

    key.list = keyData;
    key.len = key.max = sizeof(keyData) / sizeof(keyData[0]);
    ret = TMR_paramSet(rp, TMR_PARAM_LICENSE_KEY, &key);
    if (TMR_SUCCESS != ret)
    {
      fprintf(stderr, "Error setting protocol license key: %s\n", TMR_strerr(rp, ret));
    }
    else
    {
      printf("Set Protocol License Key succeeded.\n");
    }
  }

  // Report protocols enabled by current license key
  {
    TMR_TagProtocolList protocols;
    TMR_TagProtocol protocolData[16];
    int i;

    protocols.list = protocolData;
    protocols.max = sizeof(protocolData) / sizeof(protocolData[0]);
    protocols.len = 0;

    ret = TMR_paramGet(rp, TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS, &protocols);
    checkerr(rp, ret, 1, "getting supported protocols");
    printf("Supported Protocols:");
    for (i=0; i<protocols.len; i++)
    {
      char* protocolName;

      switch (protocols.list[i])
      {
      case TMR_TAG_PROTOCOL_GEN2:
        protocolName = "GEN2";
        break;
      case TMR_TAG_PROTOCOL_ISO180006B:
        protocolName = "ISO18000-6B";
        break;
      case TMR_TAG_PROTOCOL_ISO180006B_UCODE:
        protocolName = "ISO18000-6B_UCODE";
        break;
      case TMR_TAG_PROTOCOL_IPX64:
        protocolName = "IPX64";
        break;
      case TMR_TAG_PROTOCOL_IPX256:
        protocolName = "IPX256";
        break;
      default:
        protocolName = NULL;
        break;
      }

      if (NULL != protocolName)
      {
        printf(" %s", protocolName);
      }
      else
      {
        printf(" 0x%02X", protocols.list[i]);
      }
    }
    printf("\n");
  }
 
  TMR_destroy(rp);
  return 0;
}
