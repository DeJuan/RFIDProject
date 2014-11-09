/**
 * Sample program to load firmware
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
  char *filename = NULL;
  FILE* f = NULL;
#if USE_TRANSPORT_LISTENER
  TMR_TransportListenerBlock tb;
#endif

  if (argc < 2)
  {
    errx(1, "Please provide reader URL, such as:\n"
            "tmr:///com4\n"
            "tmr://my-reader.example.com\n"
            "Usage: %s readeruri firmwarefilename\n", argv[0]);
  }
  
  rp = &r;
  ret = TMR_create(rp, argv[1]);
  checkerr(rp, ret, 1, "creating reader");

  if (argc < 3)
  {
    errx(2, "Please provide firmware filename\n"
            "Usage: %s readeruri firmwarefilename\n", argv[0]);
  }
  filename = argv[2];
  
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
  switch (ret)
  {
  case TMR_ERROR_BL_INVALID_IMAGE_CRC:
  case TMR_ERROR_BL_INVALID_APP_END_ADDR:
    fprintf(stderr, "Error: App image corrupt.  Proceeding to load firmware, anyway.\n");
    break;
  default:
    checkerr(rp, ret, 1, "connecting reader");
    break;
  }
  printf("Connected to reader\n");

  printf("Opening \"%s\"\n", filename);
  f = fopen(filename, "rb");
  if (NULL == f)
  {
    perror("Can't open file");
    return 1;
  }
  
  printf("Loading firmware\n");
  ret = TMR_firmwareLoad(rp, f, TMR_fileProvider);
  checkerr(rp, ret, 1, "loading firmware");

  {
    TMR_String value;
    char value_data[64];
    value.value = value_data;
    value.max = sizeof(value_data)/sizeof(value_data[0]);

    ret = TMR_paramGet(rp, TMR_PARAM_VERSION_SOFTWARE, &value);
    checkerr(rp, ret, 1, "getting software version");
    printf("New firmware version: %s\n", value.value);
  }

  TMR_destroy(rp);
  return 0;
}
