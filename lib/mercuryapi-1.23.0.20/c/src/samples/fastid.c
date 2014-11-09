/**
 * Sample program that demonstrates the Monza4QT tag fastid operation.
 * @file fastid.c
 */

#include <tm_reader.h>
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

void readAndPrintTags(TMR_Reader *rp, int timeout)
{
  TMR_Status ret;
  TMR_TagReadData trd;
  char epcString[128];

  ret = TMR_read(rp, timeout, NULL);
  checkerr(rp, ret, 1, "reading tags");
  while (TMR_SUCCESS == TMR_hasMoreTags(rp))
  {
    ret = TMR_getNextTag(rp, &trd);
    checkerr(rp, ret, 1, "fetching tag");
    TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epcString);
    printf("%s\n", epcString);
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

void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, 
    void *cookie)
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
  TMR_TagFilter filter;
  TMR_ReadPlan filteredReadPlan;
  TMR_TagOp tagop;
  TMR_GEN2_Password accessPassword = 0x0;
  uint8_t mask[4];
  uint8_t antennaList[1] = {1};
  int32_t readPower;
  int32_t writePower;
  uint8_t tagopAntenna;
  TMR_GEN2_Session session;
  TMR_Monza4_ControlByte controlByte;
  TMR_Monza4_Payload payload;
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
  
  /* setup the reader */
  readPower = 0x0BB8;
  ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &readPower);
  checkerr(rp, ret, 1, "setting read power");

  writePower = 0x0BB8;
  ret = TMR_paramSet(rp, TMR_PARAM_RADIO_WRITEPOWER, &writePower);
  checkerr(rp, ret, 1, "setting write power");

  tagopAntenna = 1;
  ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_ANTENNA, &tagopAntenna);
  checkerr(rp, ret, 1, "setting tagop antenna");

  session = TMR_GEN2_SESSION_S0;
  ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
  checkerr(rp, ret, 1, "setting session");

  /* write the accesspassword to the tag */
  if (0)
  {
    uint16_t data[] = { 0x1234, 0x5678 };
    TMR_uint16List writeData;
    writeData.len = writeData.max = sizeof(data) / sizeof(data[0]);
    writeData.list = data;

    ret = TMR_TagOp_init_GEN2_WriteData(&tagop, TMR_GEN2_BANK_RESERVED, writeData.len,
        &writeData);
    checkerr(rp, ret, 1, "writting the access password to the tag");
    ret= TMR_SR_executeTagOp(rp,&tagop, NULL, NULL);
    checkerr(rp, ret, 1, "executing writing access password tagop");
  }
  
  /* set the Gen2 access password */
  if (0)
  {
    accessPassword = 0x12345678;
    ret = TMR_paramSet(rp,TMR_PARAM_GEN2_ACCESSPASSWORD, &accessPassword);
    checkerr(rp, ret, 1, "setting accessPassword");
  }

  /* Read plan */
  TMR_RP_init_simple(&filteredReadPlan, 1, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);

  if (1)  /* Change to "if (1)" to enable filter */
  {
    mask[0] = 0xE2;
    mask[1] = 0x80;
    mask[2] = 0x11;
    mask[3] = 0x05;

    ret = TMR_TF_init_gen2_select(&filter, false, TMR_GEN2_BANK_TID, 0x00, 0x20, mask);
    checkerr(rp, ret, 1, "creating the filter");
  }
  /* commit the read plan */
  ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &filteredReadPlan);
  checkerr(rp, ret, 1, "setting read plan");
  printf("Reading tags with a Monza 4 public EPC in response\n");
  readAndPrintTags(rp, 1000);

  /* Initialize the payload and the controlByte of Monza4 */
  TMR_init_GEN2_Impinj_Monza4_Payload(&payload);
  TMR_init_GEN2_Impinj_Monza4_ControlByte(&controlByte);

  /* executing Monza4 QT Write Set Private tagop */
  payload.data = 0x0000;
  controlByte.data = 0xC0;

  ret = TMR_TagOp_init_GEN2_Impinj_Monza4_QTReadWrite(&tagop, accessPassword, controlByte,
      payload);
  checkerr(rp, ret, 1, "initialzing the tag op");
  ret = TMR_executeTagOp(rp, &tagop, NULL, NULL);
  if (TMR_SUCCESS != ret) 
  {
    printf("Error %s: %s\n", "executing Monza4 QT Write Set Private tagop", 
        TMR_strerr(rp, ret));
  }
  else
  {
    printf("Monza4 QT tag has Set to Private mode\n\n");
  }

  /* setting the session to S2 */
  session = TMR_GEN2_SESSION_S2;
  ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
  checkerr(rp, ret, 1, "setting session to S2");
  
  if (1)  /* Change to "if (1)" to enable filter */
  {
    mask[0] = 0x20;
    mask[1] = 0x01;
    mask[2] = 0xB0;
    mask[3] = 0x00;

    ret = TMR_TF_init_gen2_select(&filter, true, TMR_GEN2_BANK_TID, 0x04, 0x16, mask);
    checkerr(rp, ret, 1, "creating the filter");
    ret = TMR_RP_set_filter(&filteredReadPlan, &filter);
    checkerr(rp, ret, 1, "setting the filter");
  }
  ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &filteredReadPlan);
  checkerr(rp, ret, 1, "setting read plan");
  printf("Reading tags with a Monza 4 FastID with TID in response\n");
  readAndPrintTags(rp, 1000);

  /* Setting the session to S0 */
  session = TMR_GEN2_SESSION_S0;
  ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
  checkerr(rp, ret, 1, "setting session to 0");
  
  if (1)  /* Change to "if (1)" to enable filter */
  {
    mask[0] = 0xE2;
    mask[1] = 0x80;
    mask[2] = 0x11;
    mask[3] = 0x05;
    ret = TMR_TF_init_gen2_select(&filter, false, TMR_GEN2_BANK_TID, 0x00, 0x01, mask);
    checkerr(rp, ret, 1, "creating the filter");
    ret = TMR_RP_set_filter(&filteredReadPlan, &filter);
    checkerr(rp, ret, 1, "setting the filter");
  }
  ret = TMR_paramSet(rp, TMR_paramID("/reader/read/plan"), &filteredReadPlan);
  checkerr(rp, ret, 1, "setting read plan");
  printf("Reading tags with a Monza 4 FastID with NO TID in response\n");
  readAndPrintTags(rp, 1000);
  
  /* setting the session to S0 */
  session = TMR_GEN2_SESSION_S0;
  ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
  checkerr(rp, ret, 1, "setting session to S0");

  /* Initialize the payload and the controlByte of Monza4 */
  TMR_init_GEN2_Impinj_Monza4_Payload(&payload);
  TMR_init_GEN2_Impinj_Monza4_ControlByte(&controlByte);

  /* executing  Monza4 QT Write Set Public tagop */
  payload.data = 0x4000;
  controlByte.data = 0xC0;

  ret = TMR_TagOp_init_GEN2_Impinj_Monza4_QTReadWrite(&tagop, accessPassword, 
      controlByte, payload);
  checkerr(rp, ret, 1, "initialzing the tag op");
  ret = TMR_executeTagOp(rp, &tagop, NULL, NULL);
  if (TMR_SUCCESS != ret) 
  {
    printf("Error %s: %s\n", "executing  Monza4 QT Write Set Public tagop", 
        TMR_strerr(rp, ret));
  }
  else
  {
    printf("Monza4 QT tag has set to Public mode\n\n");
  }

  /* setting the session to S0 */
  session = TMR_GEN2_SESSION_S0;
  ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
  checkerr(rp, ret, 1, "setting session to S2");
  
  if (1)  /* Change to "if (1)" to enable filter */
  {
    mask[0] = 0x20;
    mask[1] = 0x01;
    mask[2] = 0xB0;
    mask[3] = 0x00;

    ret = TMR_TF_init_gen2_select(&filter, true, TMR_GEN2_BANK_TID, 0x04, 0x16, mask);
    checkerr(rp, ret, 1, "creating the filter");
    ret = TMR_RP_set_filter(&filteredReadPlan, &filter);
    checkerr(rp, ret, 1, "setting the filter");
  }
  ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &filteredReadPlan);
  checkerr(rp, ret, 1, "setting read plan");
  printf("Reading tags with a Monza 4 FastID with TID in response\n");
  readAndPrintTags(rp, 1000);


  /* Initialize the payload and the controlByte of Monza4 */
  TMR_init_GEN2_Impinj_Monza4_Payload(&payload);
  TMR_init_GEN2_Impinj_Monza4_ControlByte(&controlByte);

  /* executing  Monza4 QT Read tagop */
  payload.data = 0x0000;
  controlByte.data = 0x00;

  ret = TMR_TagOp_init_GEN2_Impinj_Monza4_QTReadWrite(&tagop, accessPassword, 
      controlByte, payload);
  checkerr(rp, ret, 1, "initialzing the tag op");
  ret = TMR_executeTagOp(rp, &tagop, NULL, NULL);
  if (TMR_SUCCESS != ret) 
  {
    printf("Error %s: %s\n", "executing  Monza4 QT Read tagop", TMR_strerr(rp, ret));
  }

  /* Tear down, revert back the accesspassword back to zero(0) */
  if (0)
  {
    uint16_t data[] = { 0x0000, 0x0000};
    TMR_uint16List writeData;
    writeData.len = writeData.max = sizeof(data) / sizeof(data[0]);
    writeData.list = data;

    ret = TMR_TagOp_init_GEN2_WriteData(&tagop, TMR_GEN2_BANK_RESERVED, 
        writeData.len, &writeData);
    checkerr(rp, ret, 1, "writting the access password to the tag");
    ret= TMR_executeTagOp(rp,&tagop, NULL, NULL);
    checkerr(rp, ret, 1, "executing writing access password tagop");
  }
  
  TMR_destroy(rp);
  return 0;
}
