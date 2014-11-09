/**
 * Sample program that reads tags for a fixed period of time (500ms)
 * @file SavedConfig.c
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
  TMR_SR_UserConfigOp config;
 #if USE_TRANSPORT_LISTENER
  TMR_TransportListenerBlock tb;
#endif
 TMR_TagProtocol protocol;
  TMR_String model;
  char str[64];
  model.value = str;
  model.max = 64;

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

  protocol = TMR_TAG_PROTOCOL_GEN2;
  ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_PROTOCOL, &protocol);   // This to set the protocol
  checkerr(rp, ret, 1, "setting protocol");

  TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);
  if ((0 == strcmp("M6e", model.value)) || (0 == strcmp("M6e PRC", model.value))
      || (0 == strcmp("M6e Micro", model.value))) 
  {
		//Init UserConfigOp structure to save configuration
  TMR_init_UserConfigOp(&config, TMR_USERCONFIG_SAVE);
		ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
		checkerr(rp, ret, 1, "setting user configuration: save all configuration");
		printf("User config set option:save all configuration\n");

		//Init UserConfigOp structure to Restore all saved configuration parameters
  TMR_init_UserConfigOp(&config, TMR_USERCONFIG_RESTORE);
		ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
		checkerr(rp, ret, 1, "setting configuration: restore all saved configuration params");
		printf("User config set option:restore all saved configuration params\n");

		//Init UserConfigOp structure to verify all saved configuration parameters
  TMR_init_UserConfigOp(&config, TMR_USERCONFIG_VERIFY);
		ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
		checkerr(rp, ret, 1, "setting configuration: verify all saved configuration params");
		printf("User config set option:verify all configuration\n");


  // Get User Profile
  {
    TMR_Region region;
    TMR_TagProtocol proto;
    uint32_t baudrate;

    ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
    printf("Get user config success - option:Region\n");
    printf("%d\n", region);

    ret = TMR_paramGet(rp, TMR_PARAM_TAGOP_PROTOCOL, &proto);
    printf("Get user config success - option:Protocol\n");
    printf("%s\n", protocolName(proto));

    ret = TMR_paramGet(rp, TMR_PARAM_BAUDRATE, &baudrate);
    printf("Get user config success option:Baudrate\n");
    printf("%d\n", baudrate);
		}

		//Init UserConfigOp structure to reset/clear all configuration parameter
  TMR_init_UserConfigOp(&config, TMR_USERCONFIG_CLEAR);
		ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
		checkerr(rp, ret, 1, "setting user configuration option: reset all configuration parameters");
		printf("User config set option:reset all configuration parameters\n");
	}
	else
	{
		printf("Error: This codelet works only on M6e and it's variant\n");
	}

  TMR_destroy(rp);
  return 0;
}
