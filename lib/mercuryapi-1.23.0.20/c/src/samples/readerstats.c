/**
 * Sample programme that gets and prints the reader stats
 * It shows both the sync and async way of getting the reader stats.
 * @file readerstats.c
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
void statsCallback (TMR_Reader *reader, const TMR_Reader_StatsValues* stats, void *cookie);

static char _protocolNameBuf[32];
static const char* protocolName(enum TMR_TagProtocol value)
{
  switch (value)
  {
  case TMR_TAG_PROTOCOL_NONE:
    return "NONE";
  case TMR_TAG_PROTOCOL_GEN2:
    return "GEN2";
  case TMR_TAG_PROTOCOL_ISO180006B:
    return "ISO180006B";
  case TMR_TAG_PROTOCOL_ISO180006B_UCODE:
    return "ISO180006B_UCODE";
  case TMR_TAG_PROTOCOL_IPX64:
    return "IPX64";
  case TMR_TAG_PROTOCOL_IPX256:
    return "IPX256";
  default:
    snprintf(_protocolNameBuf, sizeof(_protocolNameBuf), "TagProtocol:%d", (int)value);
    return _protocolNameBuf;
  }
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
#if USE_TRANSPORT_LISTENER
  TMR_TransportListenerBlock tb;
#endif
  TMR_ReadListenerBlock rlb;
  TMR_ReadExceptionListenerBlock reb;
  TMR_StatsListenerBlock slb;
 
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
    /* Code to get the reader stats after the sync read */
    TMR_Reader_StatsValues stats;
    TMR_Reader_StatsFlag setFlag;
    TMR_PortValueList value;
    TMR_PortValue valueList[64];
    int i, j;

    printf("\nReader stats after the sync read\n");

    /** request for the statics fields of your interest, before search */
    setFlag = TMR_READER_STATS_FLAG_ALL;
    value.max = sizeof(valueList)/sizeof(valueList[0]);
    value.list = valueList;

    ret = TMR_paramSet(rp, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
    checkerr(rp, ret, 1, "setting the  fields");

    for (j = 1; j < 4; j++)
    {
      /**
       * perform three iterations to see that reader stats are
       * resetting after each search operation.
       **/ 
      printf("\nIteration:%d\n", j);
      /**
       * performing the search operation. for 1 sec,
       * Individual search will reset the reader stats, before doing the search
       */
      printf("Performing the search operation. for 1 sec\n");
      ret = TMR_read(rp, 1000, NULL);
      checkerr(rp, ret, 1, "reading tags");

      /** Initialize the reader statics variable to default values */
      TMR_STATS_init(&stats);

      /* Search is completed. Get the reader stats */
      printf("Search is completed. Get the reader stats\n");
      ret = TMR_paramGet(rp, TMR_PARAM_READER_STATS, &stats);
      checkerr(rp, ret, 1, "getting the reader statistics");

      /** Each  field should be validated before extracting the value */
      if (TMR_READER_STATS_FLAG_CONNECTED_ANTENNAS & stats.valid)
      {
        printf("Antenna Connection Status\n");
        for (i = 0; i < stats.connectedAntennas.len; i += 2)
        {
          printf("Antenna %d |%s\n", stats.connectedAntennas.list[i],
              stats.connectedAntennas.list[i + 1] ? "connected":"Disconnected");
        }
      }

      /* Get the antenna return loss value, this parameter is not the part of reader stats */
      ret = TMR_paramGet(rp, TMR_PARAM_ANTENNA_RETURNLOSS, &value);
      checkerr(rp, ret, 1, "getting the antenna return loss");
      printf("Antenna Return Loss\n");
      for (i = 0; i < value.len && i < value.max; i++)
      {
        printf("Antenna %d | %d \n", value.list[i].port, value.list[i].value);
      }

      if (TMR_READER_STATS_FLAG_NOISE_FLOOR_SEARCH_RX_TX_WITH_TX_ON & stats.valid)
      {
        printf("Noise Floor With Tx On\n");
        for (i = 0; i < stats.perAntenna.len; i++)
        {
          printf("Antenna %d | %d db\n", stats.perAntenna.list[i].antenna, stats.perAntenna.list[i].noiseFloor);
        }
      }

      if (TMR_READER_STATS_FLAG_RF_ON_TIME & stats.valid)
      {
        printf("RF On Time\n");
        for (i = 0; i < stats.perAntenna.len; i++)
        {
          printf("Antenna %d | %d ms\n", stats.perAntenna.list[i].antenna, stats.perAntenna.list[i].rfOnTime);
        }
      }

      if (TMR_READER_STATS_FLAG_FREQUENCY & stats.valid)
      {
        printf("Frequency %d(khz)\n", stats.frequency);
      }
      if (TMR_READER_STATS_FLAG_TEMPERATURE & stats.valid)
      {
        printf("Temperature %d(C)\n", stats.temperature);
      }
      if (TMR_READER_STATS_FLAG_PROTOCOL & stats.valid)
      {
        printf("Protocol %s\n", protocolName(stats.protocol));
      }
      if (TMR_READER_STATS_FLAG_ANTENNA_PORTS & stats.valid)
      {
        printf("currentAntenna %d\n", stats.antenna);
      }
    }
  }

  {
    /* Code to get the reader stats after the async read */
    TMR_Reader_StatsFlag setFlag = TMR_READER_STATS_FLAG_ALL;

    rlb.listener = callback;
    rlb.cookie = NULL;

    reb.listener = exceptionCallback;
    reb.cookie = NULL;

    slb.listener = statsCallback;
    slb.cookie = NULL;

    ret = TMR_addReadListener(rp, &rlb);
    checkerr(rp, ret, 1, "adding read listener");

    ret = TMR_addReadExceptionListener(rp, &reb);
    checkerr(rp, ret, 1, "adding exception listener");

    ret = TMR_addStatsListener(rp, &slb);
    checkerr(rp, ret, 1, "adding the stats listener");

    printf("\nReader stats after the async read \n");

    /** request for the statics fields of your interest, before search */
    ret = TMR_paramSet(rp, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
    checkerr(rp, ret, 1, "setting the  fields");

    printf("Initiating the search operation. for 1 sec and the listener will provide the reader stats\n");

    ret = TMR_startReading(rp);
    checkerr(rp, ret, 1, "starting reading");

#ifndef WIN32
    sleep(1);
#else
    Sleep(1000);
#endif

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
  printf("Background read: %s\n", epcStr);
}

void 
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}

void statsCallback (TMR_Reader *reader, const TMR_Reader_StatsValues* stats, void *cookie)
{
  uint8_t i = 0;

  /** Each  field should be validated before extracting the value */
  if (TMR_READER_STATS_FLAG_CONNECTED_ANTENNAS & stats->valid)
  {
    printf("Antenna Connection Status\n");
    for (i = 0; i < stats->connectedAntennas.len; i += 2)
    {
      printf("Antenna %d |%s\n", stats->connectedAntennas.list[i],
          stats->connectedAntennas.list[i + 1] ? "connected":"Disconnected");
    }
  }

  if (TMR_READER_STATS_FLAG_NOISE_FLOOR_SEARCH_RX_TX_WITH_TX_ON & stats->valid)
  {
    printf("Noise Floor With Tx On\n");
    for (i = 0; i < stats->perAntenna.len; i++)
    {
      printf("Antenna %d | %d db\n", stats->perAntenna.list[i].antenna, stats->perAntenna.list[i].noiseFloor);
    }
  }

  if (TMR_READER_STATS_FLAG_RF_ON_TIME & stats->valid)
  {
    printf("RF On Time\n");
    for (i = 0; i < stats->perAntenna.len; i++)
    {
      printf("Antenna %d | %d ms\n", stats->perAntenna.list[i].antenna, stats->perAntenna.list[i].rfOnTime);
    }
  }

  if (TMR_READER_STATS_FLAG_FREQUENCY & stats->valid)
  {
    printf("Frequency %d(khz)\n", stats->frequency);
  }
  if (TMR_READER_STATS_FLAG_TEMPERATURE & stats->valid)
  {
    printf("Temperature %d(C)\n", stats->temperature);
  }
  if (TMR_READER_STATS_FLAG_PROTOCOL & stats->valid)
  {
    printf("Protocol %s\n", protocolName(stats->protocol));
  }
  if (TMR_READER_STATS_FLAG_ANTENNA_PORTS & stats->valid)
  {
    printf("currentAntenna %d\n", stats->antenna);
  }
}
