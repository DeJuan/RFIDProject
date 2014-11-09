/**
 * Sample program that supports PSAM functionality
 * @file securereaddata.c
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
void authreq_callback(TMR_Reader *reader, const TMR_TagReadData *trd, void *cookie, TMR_TagAuthentication* auth);

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
  TMR_AuthReqListenerBlock arlb;
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

  arlb.listener = authreq_callback;
  arlb.cookie = NULL;

  reb.listener = exceptionCallback;
  reb.cookie = NULL;

  ret = TMR_addReadListener(rp, &rlb);
  checkerr(rp, ret, 1, "adding read listener");

  ret = TMR_addAuthReqListener(rp, &arlb);
  checkerr(rp, ret, 1, "adding authreq listener");

  ret = TMR_addReadExceptionListener(rp, &reb);
  checkerr(rp, ret, 1, "adding read exception listener");

  {
    /* Embedded Secure Read Tag Operation - Standalone operation not supported */
    TMR_TagOp op;
    TMR_ReadPlan plan; 
    TMR_GEN2_Password password = 0;

    /* Securely read the tag data.  In real life, this occurs in the field, after the end user has received the tag. */
    /* initialize the secure read data */
    ret = TMR_TagOp_init_GEN2_SecureReadData(&op, TMR_GEN2_BANK_TID, 0, 0, GEN2_EMBEDDED_SECURE_HIGGS3_ACCESS, 0);
    checkerr(rp, ret, 1, "initializing the Gen2 secure read data tagop");

    /* Set the password to zero */
    TMR_TA_init_gen2(&op.u.gen2.u.secureReadData.password.gen2PassWord, password);
    ret = TMR_paramSet(rp, TMR_PARAM_GEN2_ACCESSPASSWORD, &password);
    checkerr(rp, ret, 1, "setting the Gen2 access password");

    /* initialize the read plan */
    ret = TMR_RP_init_simple(&plan, 0, NULL, TMR_TAG_PROTOCOL_GEN2, 1000);
    checkerr(rp, ret, 1, "initializing the read plan");

    /* set the tag op */
    ret = TMR_RP_set_tagop(&plan, &op);
    checkerr(rp, ret, 1, "setting the tagop");

    /* commit the read plan */
    ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
    checkerr(rp, ret, 1, "setting the read plan");
  }

  ret = TMR_startReading(rp);
  checkerr(rp, ret, 1, "starting reading");

#ifndef WIN32
  sleep(1);
#else
  Sleep(5000);
#endif

  ret = TMR_stopReading(rp);
  checkerr(rp, ret, 1, "stopping reading");

  TMR_destroy(rp);
  return 0;

#endif /* TMR_ENABLE_BACKGROUND_READS */
}

void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[1024];

  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  printf("Background read: %s %d\n", epcStr,t->tag.epcByteCount);
  if (0 < t->data.len)
  {
    char dataStr[1024];
    TMR_bytesToHex(t->data.list, t->data.len, dataStr);
    printf("  data(%d): %s\n", t->data.len, dataStr);
  }
}

void
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}

void 
authreq_callback(TMR_Reader *reader, const TMR_TagReadData *trd, void *cookie, TMR_TagAuthentication* auth)
{
  static uint32_t tagAccessPassword1 = 0x11223344;
  static uint32_t tagAccessPassword2 = 0x22222222;
  static uint32_t tagAccessPassword3 = 0x33333333;
  static uint32_t tagAccessPassword4 = 0x11111111;


  /* This example simply uses a precompiled password.
   * In real life, an algorithm or server would examine the tag ID contained in trd and return an appropriate password.
   */
  int index  = trd->tag.epc[trd->tag.epcByteCount-1] % 4;
  if (index == 0)
    TMR_TA_init_gen2(auth, tagAccessPassword1);
  else if (index == 1)
    TMR_TA_init_gen2(auth, tagAccessPassword2);
  else if (index == 2)
    TMR_TA_init_gen2(auth, tagAccessPassword3);
  else if (index == 3)
    TMR_TA_init_gen2(auth, tagAccessPassword4);
}
