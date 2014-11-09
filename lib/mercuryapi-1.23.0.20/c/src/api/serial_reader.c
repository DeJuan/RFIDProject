/**
 *  @file serial_reader.c
 *  @brief Mercury API - serial reader high level implementation
 *  @author Nathan Williams
 *  @date 10/28/2009
 */

/*
 * Copyright (c) 2009 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tm_reader.h"
#include "serial_reader_imp.h"
#include "tmr_utils.h"
#include "osdep.h"

#define HASPORT(mask, port) ((1 << ((port)-1)) & (mask))

#ifdef TMR_ENABLE_SERIAL_READER

static TMR_Status verifySearchStatus(TMR_Reader *reader);

static TMR_Status
initTxRxMapFromPorts(TMR_Reader *reader)
{
  TMR_Status ret;
  TMR_SR_PortDetect ports[TMR_SR_MAX_ANTENNA_PORTS];
  uint8_t i, numPorts;
  TMR_SR_SerialReader *sr;

  numPorts = numberof(ports);
  sr = &reader->u.serialReader;

  /* Need number of ports to set up Tx-Rx map */
  ret = TMR_SR_cmdAntennaDetect(reader, &numPorts, ports);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }
  sr->portMask = 0;

  /* Modify TxRxMap according to reader product */
  switch (sr->productId)
  {
    case 0x0001:
    {
      /* Ruggedized Reader (Tool Link, Vega) */
      TMR_AntennaMap newMap[] = {{1,2,2}, {2,5,5}, {3,1,1}};
      numPorts = 3;

      for (i = 0; i < numPorts; i++)
      {
        sr->portMask |= 1 << (newMap[i].antenna - 1);
        sr->staticTxRxMapData[i].antenna = newMap[i].antenna;
        sr->staticTxRxMapData[i].rxPort  = newMap[i].rxPort;
        sr->staticTxRxMapData[i].txPort  = newMap[i].txPort;

        if (0 == reader->tagOpParams.antenna && ports[i].detected)
        {
          reader->tagOpParams.antenna = ports[i].port;
        }
      }
      break;
    }

    case 0x0002:
      /*
       * USB Reader -- Default map is okay
       * M5e-C only has 1 antenna port, anyway
       */
    default:
    {
      for (i = 0; i < numPorts; i++)
      {
        sr->portMask |= 1 << (ports[i].port - 1);
        sr->staticTxRxMapData[i].antenna = ports[i].port;
        sr->staticTxRxMapData[i].txPort = ports[i].port;
        sr->staticTxRxMapData[i].rxPort = ports[i].port;

        if (0 == reader->tagOpParams.antenna && ports[i].detected)
        {
          reader->tagOpParams.antenna = ports[i].port;
        }
      }
      break;
    }
  }

  sr->staticTxRxMap.max = TMR_SR_MAX_ANTENNA_PORTS;
  sr->staticTxRxMap.len = numPorts;
  sr->staticTxRxMap.list = sr->staticTxRxMapData;
  sr->txRxMap = &sr->staticTxRxMap;

  return TMR_SUCCESS;
}

static TMR_Status
TMR_SR_configPreamble(TMR_SR_SerialReader *sr)
{
  TMR_Status ret = TMR_SUCCESS;
  bool value;

  /* Only some readers support wakeup preambles */
  switch (sr->versionInfo.hardware[0])
  {
  case TMR_SR_MODEL_M6E:
  case TMR_SR_MODEL_M6E_MICRO:
  case TMR_SR_MODEL_M6E_PRC:
    value = true;
    break;
  default:
    value = false;
    break;
  }
  sr->supportsPreamble = value;

  return ret;
}

static TMR_Status
TMR_SR_boot(TMR_Reader *reader, uint32_t currentBaudRate)
{
  TMR_Status ret;
  uint8_t program;
  bool boolval;
  TMR_SR_SerialReader *sr;
  TMR_SR_SerialTransport *transport;
  int i;
  bool value;

  ret = TMR_SUCCESS;
  sr = &reader->u.serialReader;
  transport = &sr->transport;

  /* Get current program */
  ret = TMR_SR_cmdGetCurrentProgram(reader, &program);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /* If bootloader, enter app */
  if ((program & 0x3) == 1)
  {
    ret = TMR_SR_cmdBootFirmware(reader);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }

  /*
   * Once out of bootloader, configure for wakeup preambles.
   * Bootloader doesn't support preambles, and some versions
   * fail to respond to normal commands afterwards.
   *
   * It is safe to avoid preambles up to this point because
   * TMR_SR_connect has already been talking to the module
   * to keep it awake.
   */
  ret = TMR_SR_configPreamble(sr);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /* Initialize cached power mode value */
  /* Should read power mode as soon as possible.
   * Default mode assumes module is in deep sleep and
   * adds a lengthy "wake-up preamble" to every command.
   */
  if (sr->powerMode == TMR_SR_POWER_MODE_INVALID)
  {
    ret = TMR_paramGet(reader, TMR_PARAM_POWERMODE, &sr->powerMode);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }

  /**
   * In case for M6E and it's varient  check for CRC
   **/
  if ((TMR_SR_MODEL_M6E == reader->u.serialReader.versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_PRC == reader->u.serialReader.versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_MICRO == reader->u.serialReader.versionInfo.hardware[0]))
  {
    /**
     * Get the transport/BUS type
     **/
    ret = TMR_SR_cmdGetReaderConfiguration(reader, TMR_SR_CONFIGURATION_CURRENT_MSG_TRANSPORT, &reader->u.serialReader.transportType);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    /**
     * In case of USB port disable the CRC
     **/
    if (TMR_SR_MSG_SOURCE_USB == reader->u.serialReader.transportType)
    {
      value = false;
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_SEND_CRC, &value);
      if (TMR_SUCCESS == ret)
      {
        reader->u.serialReader.crcEnabled = false;
      }
      else
      {
        /**
         * Not Fatal, Going ahead with CRC enabled
         **/ 
      }
    }
  }

  if (sr->baudRate != currentBaudRate)
  {
    if (NULL != transport->setBaudRate)
    {
      /**
       * some transport layer does not support baud rate settings.
       * for ex: TCP transport. In that case skip the baud rate
       * settings.
       */ 

      /* Bring baud rate up to the parameterized value */
      ret = TMR_SR_cmdSetBaudRate(reader, sr->baudRate);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      ret = transport->setBaudRate(transport, sr->baudRate);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
    }
  }

  /*ret = TMR_SR_cmdVersion(reader, &sr->versionInfo);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }*/

  /* If we need to check the version information for something operational,
     this is the place to do it. */
  sr->gpioDirections = -1; /* Needs fetching */
  reader->continuousReading = ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]));
  reader->continuousReading = false; //fix bug 1745 in the current release temporarily

  /**
   * This version check is required for the new reader stats.
   * Older firmwares does not support this. Currently, firmware version
   * 1.21.1.2 has support for new reader stats.
   **/
  if ((1 <= reader->u.serialReader.versionInfo.fwVersion[0]) &&
      (21 <= reader->u.serialReader.versionInfo.fwVersion[1]) &&
      (1 <= reader->u.serialReader.versionInfo.fwVersion[2]))
  {
    reader->_storeSupportsResetStats = true;
    reader->pSupportsResetStats = &(reader->_storeSupportsResetStats);
  }
  else
  {
    reader->_storeSupportsResetStats = false;
    reader->pSupportsResetStats = &(reader->_storeSupportsResetStats);
  }

  /* Initialize the paramPresent and paramConfirmed bits. */
  /* This block is expected to be collapsed by the compiler into a
   * small number of constant-value writes into the sr->paramPresent
   * array.
   */
  for (i = 0 ; i < TMR_PARAMWORDS; i++)
  {
    sr->paramPresent[i] = 0;
  }

  BITSET(sr->paramPresent, TMR_PARAM_BAUDRATE);
  BITSET(sr->paramPresent, TMR_PARAM_COMMANDTIMEOUT);
  BITSET(sr->paramPresent, TMR_PARAM_TRANSPORTTIMEOUT);
  BITSET(sr->paramPresent, TMR_PARAM_POWERMODE);
  BITSET(sr->paramPresent, TMR_PARAM_USERMODE);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_CHECKPORT);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_PORTLIST);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_CONNECTEDPORTLIST);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_PORTSWITCHGPOS);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_SETTLINGTIMELIST);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_RETURNLOSS);
  BITSET(sr->paramPresent, TMR_PARAM_ANTENNA_TXRXMAP);
  BITSET(sr->paramPresent, TMR_PARAM_GPIO_INPUTLIST);
  BITSET(sr->paramPresent, TMR_PARAM_GPIO_OUTPUTLIST);
  BITSET(sr->paramPresent, TMR_PARAM_GEN2_ACCESSPASSWORD);
  BITSET(sr->paramPresent, TMR_PARAM_GEN2_Q);
  BITSET(sr->paramPresent, TMR_PARAM_GEN2_BAP);
  BITSET(sr->paramPresent, TMR_PARAM_GEN2_TAGENCODING);
  BITSET(sr->paramPresent, TMR_PARAM_GEN2_SESSION);
  BITSET(sr->paramPresent, TMR_PARAM_GEN2_TARGET);
  BITSET(sr->paramPresent, TMR_PARAM_READ_ASYNCOFFTIME);
  BITSET(sr->paramPresent, TMR_PARAM_READ_ASYNCONTIME);
  BITSET(sr->paramPresent, TMR_PARAM_READ_PLAN);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_ENABLEPOWERSAVE);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_POWERMAX);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_POWERMIN);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_PORTREADPOWERLIST);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_PORTWRITEPOWERLIST);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_READPOWER);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_WRITEPOWER);
  BITSET(sr->paramPresent, TMR_PARAM_RADIO_TEMPERATURE);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_UNIQUEBYDATA);
  BITSET(sr->paramPresent, TMR_PARAM_TAGOP_ANTENNA);
  BITSET(sr->paramPresent, TMR_PARAM_TAGOP_PROTOCOL);
  BITSET(sr->paramPresent, TMR_PARAM_VERSION_HARDWARE);
  BITSET(sr->paramPresent, TMR_PARAM_VERSION_MODEL);
  BITSET(sr->paramPresent, TMR_PARAM_VERSION_SOFTWARE);
  BITSET(sr->paramPresent, TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS);
  BITSET(sr->paramPresent, TMR_PARAM_REGION_ID);
  BITSET(sr->paramPresent, TMR_PARAM_REGION_SUPPORTEDREGIONS);
  BITSET(sr->paramPresent, TMR_PARAM_REGION_HOPTABLE);
  BITSET(sr->paramPresent, TMR_PARAM_REGION_HOPTIME);
  BITSET(sr->paramPresent, TMR_PARAM_REGION_LBT_ENABLE);
  BITSET(sr->paramPresent, TMR_PARAM_EXTENDEDEPC);
  BITSET(sr->paramPresent, TMR_PARAM_READER_STATS);
  BITSET(sr->paramPresent, TMR_PARAM_READER_STATISTICS); 
  BITSET(sr->paramPresent, TMR_PARAM_URI);
  BITSET(sr->paramPresent, TMR_PARAM_PRODUCT_GROUP_ID);
  BITSET(sr->paramPresent, TMR_PARAM_PRODUCT_GROUP);
  BITSET(sr->paramPresent, TMR_PARAM_PRODUCT_ID);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADATA_TAGOPSUCCESSCOUNT);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADATA_TAGOPFAILURECOUNT);
  BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_ENABLEREADFILTER);
  BITSET(sr->paramPresent, TMR_PARAM_READER_WRITE_REPLY_TIMEOUT);
  BITSET(sr->paramPresent, TMR_PARAM_READER_WRITE_EARLY_EXIT);
  BITSET(sr->paramPresent, TMR_PARAM_ISO180006B_DELIMITER);
  BITSET(sr->paramPresent, TMR_PARAM_ISO180006B_MODULATION_DEPTH);
  BITSET(sr->paramPresent, TMR_PARAM_READER_STATS_ENABLE);
  if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]))

  {
    BITSET(sr->paramPresent, TMR_PARAM_LICENSE_KEY);
    BITSET(sr->paramPresent, TMR_PARAM_USER_CONFIG);
    BITSET(sr->paramPresent, TMR_PARAM_RADIO_ENABLESJC);
    BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_READFILTERTIMEOUT);
    BITSET(sr->paramPresent, TMR_PARAM_TAGREADDATA_UNIQUEBYPROTOCOL);
  }

  for (i = 0 ; i < TMR_PARAMWORDS; i++)
  {
    sr->paramConfirmed[i] = sr->paramPresent[i];
  }

  /* Get productGroupID early, so other params (e.g., txRxMap) can use it */
  {
    ret = TMR_SR_cmdGetReaderConfiguration(reader, TMR_SR_CONFIGURATION_PRODUCT_GROUP_ID, &sr->productId);
    if (TMR_SUCCESS != ret)
    {
      if (TMR_ERROR_MSG_INVALID_PARAMETER_VALUE == ret)
      {
        /*
         * Modules with firmware older than wilder will throw 0x105 error, as it was not 
         * implemented. Catch this error and but do not return.
         */
		sr->productId = 0xFFFF;
      }
      else
      {
        return ret;
      }
    }
    /* 
     * If product is ruggedized reader, 
     * set reader's GPO pin which is used for antenna port switching
     */
    if (1 == sr->productId)
    {
      uint8_t pin = 1;
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ANTENNA_CONTROL_GPIO, &pin);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
    }
  }
  /* Set region if user set the param */
  if (TMR_REGION_NONE != sr->regionId)
  {
    ret = TMR_SR_cmdSetRegion(reader, sr->regionId);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }

  ret = TMR_SR_cmdGetCurrentProtocol(reader, &sr->currentProtocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }
  reader->tagOpParams.protocol = sr->currentProtocol;

  reader->tagOpParams.antenna = 0;
  ret = initTxRxMapFromPorts(reader);

  /**
   * Enable the extended EPC flag in case
   * of M5E and its varients
   */
  if ((TMR_SR_MODEL_M6E != sr->versionInfo.hardware[0]) &&
      (TMR_SR_MODEL_M6E_MICRO != sr->versionInfo.hardware[0]) &&
      (TMR_SR_MODEL_M6E_PRC != sr->versionInfo.hardware[0]))
  {
    /* Do this only if the module is other than M6e*/
    boolval = true;
    ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_EXTENDED_EPC, &boolval);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    reader->u.serialReader.extendedEPC = boolval;
  }
  /* Report RSSI in dbm */
  if ((TMR_SR_MODEL_M6E != sr->versionInfo.hardware[0]) &&
      (TMR_SR_MODEL_M6E_MICRO != sr->versionInfo.hardware[0]) &&
      (TMR_SR_MODEL_M6E_PRC != sr->versionInfo.hardware[0]))
  {
    /* Do this only if the module is other than M6e*/
    boolval = true;
    ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_RSSI_IN_DBM, &boolval);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }
  else
  { /* Do this only in case of M6e */
    int32_t timeout = 0;

    /* Get reader's enable read filter setting */
    ret = TMR_SR_cmdGetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ENABLE_READ_FILTER, &reader->u.serialReader.enableReadFiltering);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    
    /* Get reader's read filter entry timeout */
    ret = TMR_SR_cmdGetReaderConfiguration(reader, TMR_SR_CONFIGURATION_READ_FILTER_TIMEOUT, &timeout);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    reader->u.serialReader.readFilterTimeout = (0 == timeout) ? TMR_DEFAULT_READ_FILTER_TIMEOUT : timeout;

  }

  
  return ret;
}

TMR_Status
TMR_SR_connect(TMR_Reader *reader)
{
  TMR_Status ret;
  uint32_t rate;
  TMR_SR_SerialReader *sr;
  TMR_SR_SerialTransport *transport;
  int i,count = 2;
  
  ret = TMR_SUCCESS;
  sr = &reader->u.serialReader;
  transport = &reader->u.serialReader.transport;

  ret = transport->open(transport);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }
   rate = sr->probeBaudRates.list[0]; //this fixes the compilation errors in some compilers
  
  /* Make contact at some baud rate */
  for (i = 0; (uint32_t)i < sr->probeBaudRates.len; i++)
  {
    if (i <= 1 && count)
    { 
      rate = sr->baudRate; /* Try this first */
      /* Module might be in deep sleep mode, if there is no response for the
       * first attempt, Try the same baudrate again. i = 0 and i = 1
       */
      count--;

      /* Hold i=0 (undo increment) until count has expired */

      i--;
    }
    else
    {
     rate = sr->probeBaudRates.list[i];
      if (rate == sr->baudRate)
        continue; /* We already tried this one */
    }

    if (NULL != transport->setBaudRate)
    {
      /**
       * some transport layer does not support baud rate settings.
       * for ex: TCP transport. In that case skip the baud rate
       * settings.
       */ 
      ret = transport->setBaudRate(transport, rate);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
    }

    ret = transport->flush(transport);
    if ((TMR_SUCCESS != ret) && (TMR_ERROR_UNIMPLEMENTED != ret))
    {
      return ret;
    }
contact:
    ret = TMR_SR_cmdVersion(reader, &(reader->u.serialReader.versionInfo));
    if (TMR_SUCCESS == ret)
    {
      /* Got a reply?  Then this is the right baud rate! */
      break;
    }

    /* Timeouts are okay -- they usually mean "wrong baud rate",
     * so just try the next one.  All other errors are real
     * and should be forwarded immediately. */
    else if (TMR_ERROR_TIMEOUT != ret)
    {
      if (TMR_ERROR_COMM(ret))
      {
        ret = verifySearchStatus(reader);
        if (TMR_SUCCESS == ret)
        {
          goto contact;
        }
        else
        {
          return ret;
        }
      } 
      else
      {
        return ret;
      }
    }
  }
  if (i == sr->probeBaudRates.len)
  {
    return TMR_ERROR_TIMEOUT;
  }
  reader->connected = true;

  /* Boot */
  ret = TMR_SR_boot(reader, rate);

  return ret;
}

TMR_Status
TMR_SR_destroy(TMR_Reader *reader)
{
  TMR_SR_SerialTransport *transport;

  transport = &reader->u.serialReader.transport;

  /**
   * Enable the CRC, in case it is disabled
   **/
  if (false == reader->u.serialReader.crcEnabled)
  {
    reader->u.serialReader.crcEnabled = true;
    TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_SEND_CRC, &reader->u.serialReader.crcEnabled);
  }

  transport->shutdown(transport);
  reader->connected = false;

#ifdef TMR_ENABLE_BACKGROUND_READS
  /* Cleanup background threads */
  cleanup_background_threads(reader);
#endif

  return TMR_SUCCESS;
}

static TMR_Status
autoDetectAntennaList(struct TMR_Reader *reader)
{
  TMR_Status ret;
  TMR_SR_PortDetect ports[TMR_SR_MAX_ANTENNA_PORTS];
  TMR_SR_PortPair searchList[TMR_SR_MAX_ANTENNA_PORTS];
  uint8_t i, listLen, numPorts;
  uint16_t j;
  TMR_AntennaMapList *map;
    
  ret = TMR_SUCCESS;
  map = reader->u.serialReader.txRxMap;

  /* 1. Detect current set of antennas */
  numPorts = TMR_SR_MAX_ANTENNA_PORTS;
  ret = TMR_SR_cmdAntennaDetect(reader, &numPorts, ports);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /* 2. Set antenna list based on detected antennas (Might be clever
   * to cache this and not bother sending the set-list command
   * again, but it's more code and data space).
   */
  for (i = 0, listLen = 0; i < numPorts; i++)
  {
    if (ports[i].detected)
    {
      /* Ensure that the port exists in the map */
      for (j = 0; j < map->len; j++)
        if (ports[i].port == map->list[j].txPort)
        {
          searchList[listLen].txPort = map->list[j].txPort;
          searchList[listLen].rxPort = map->list[j].rxPort;
          listLen++;
          break;
        }
    }
  }
  if (0 == listLen) /* No ports auto-detected */
  {
    return TMR_ERROR_NO_ANTENNA;
  }
  ret = TMR_SR_cmdSetAntennaSearchList(reader, listLen, searchList);
  
  return ret;
}

static TMR_Status
setAntennaList(struct TMR_Reader *reader, TMR_uint8List *antennas)
{
  TMR_SR_PortPair searchList[16];
  uint16_t i, j, listLen;
  TMR_AntennaMapList *map;

  map = reader->u.serialReader.txRxMap;

  /** @todo cache the set list and don't reset it if it hasn't changed */
  listLen = 0;
  for (i = 0; i < antennas->len ; i++)
  {
    for (j = 0; j < map->len; j++)
    {
      if (antennas->list[i] == map->list[j].antenna)
      {
        searchList[listLen].txPort = map->list[j].txPort;
        searchList[listLen].rxPort = map->list[j].rxPort;
        listLen++;
        break;
      }
    }
  }
  return TMR_SR_cmdSetAntennaSearchList(reader,(uint8_t)listLen, searchList);
}

static TMR_Status
setProtocol(struct TMR_Reader *reader, TMR_TagProtocol protocol)
{
  TMR_Status ret;

  if(reader->u.serialReader.currentProtocol != protocol)
  {
  ret = TMR_SR_cmdSetProtocol(reader, protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /* Set extended EPC -- This bit is reset when the protocol changes */
  if (reader->u.serialReader.extendedEPC)
  {  
    bool boolval;
    boolval = true;
    ret = TMR_SR_cmdSetReaderConfiguration(reader,
					   TMR_SR_CONFIGURATION_EXTENDED_EPC,
					   &boolval);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }
  reader->u.serialReader.currentProtocol = protocol;

  /* Set enable filtering -- module automatically resets this when protocol is changed */
  ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ENABLE_READ_FILTER, &reader->u.serialReader.enableReadFiltering);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  {
    /* Set the read filter timeout */
    uint32_t moduleValue = (TMR_DEFAULT_READ_FILTER_TIMEOUT == reader->u.serialReader.readFilterTimeout) ?
                                                                0 : reader->u.serialReader.readFilterTimeout;
    ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_READ_FILTER_TIMEOUT, &moduleValue);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    reader->u.serialReader.readFilterTimeout = moduleValue;
  }
  }

  return TMR_SUCCESS;
}

/**
 * Compare antenna list in readplans list, return true if antenna
 * list are consistent across the entire set of read plans.
 **/
bool
compareAntennas(TMR_MultiReadPlan *multi)
{
  TMR_ReadPlan *plan1, *plan2;

  uint8_t allAntennasNull = 0;
  uint8_t matchingPlanCount = 0;
  uint8_t i;
  bool status = false;

  for (i = 0; i < multi->planCount; i++)
  {
    plan1 = multi->plans[0];
    plan2 = multi->plans[i];

    if ((0 != plan1->u.simple.antennas.len) && (0 != plan2->u.simple.antennas.len))
    {
      uint8_t j;

      if (plan1->u.simple.antennas.len == plan2->u.simple.antennas.len)
      {
        for (j = 0; j < plan1->u.simple.antennas.len; j++)
        {
          if (plan1->u.simple.antennas.list[j] != plan2->u.simple.antennas.list[j])
          {
            status = false;
            break;
          }
        }
        if (j == plan1->u.simple.antennas.len)
        {
          matchingPlanCount++;
        }
      }
    }
    else if ((0 == plan1->u.simple.antennas.len) && (0 == plan2->u.simple.antennas.len))
    {
      allAntennasNull++;
    }
    else
    {
      status = false;
      break;
    }
  }

  if ((matchingPlanCount == multi->planCount) || (allAntennasNull == multi->planCount))
  {
    status = true;
  }

  return status;
}

static TMR_Status
prepForSearch(TMR_Reader *reader, TMR_uint8List *antennaList)
{
  TMR_Status ret;
  if (antennaList->len == 0)
  {
    ret = autoDetectAntennaList(reader);
  }
  else
  {
    ret = setAntennaList(reader, antennaList);
  }
  return ret;
}

static TMR_Status
prepEmbReadTagMultiple(TMR_Reader *reader, uint8_t *msg, uint8_t *i,
            uint16_t timeout, TMR_SR_SearchFlag searchFlag,
            const TMR_TagFilter *filter, TMR_TagProtocol protocol,
            TMR_GEN2_Password accessPassword, uint8_t *lenbyte)
{
  TMR_Status ret;

  ret = TMR_SR_msgSetupReadTagMultiple(reader,
        msg, i, (uint16_t)timeout, searchFlag, filter, protocol, accessPassword);
  
  /* Embedded command count (Currently supports only one command)*/
  SETU8(msg, *i, 1);  
  *lenbyte = (*i)++;
  return ret;
}

static TMR_Status
TMR_SR_read_internal(struct TMR_Reader *reader, uint32_t timeoutMs,
                     int32_t *tagCount, TMR_ReadPlan *rp)
{
  TMR_Status ret = TMR_SUCCESS;
  TMR_SR_SerialReader *sr;
  TMR_SR_MultipleStatus multipleStatus = {0};
  uint32_t count, elapsed, elapsed_tagop;
  uint32_t readTimeMs, starttimeLow, starttimeHigh;
  TMR_uint8List *antennaList = NULL;

  sr = &reader->u.serialReader;

  if (TMR_READ_PLAN_TYPE_MULTI == rp->type)
  {
    int i;
    TMR_TagProtocolList p;
    TMR_TagProtocolList *protocolList = &p;
    TMR_TagFilter *filters[TMR_MAX_SERIAL_MULTIPROTOCOL_LENGTH];
    TMR_TagProtocol protocols[TMR_MAX_SERIAL_MULTIPROTOCOL_LENGTH];


    if (TMR_MAX_SERIAL_MULTIPROTOCOL_LENGTH < rp->u.multi.planCount)
    {
      return TMR_ERROR_TOO_BIG ;
    }

    protocolList->len = rp->u.multi.planCount;
    protocolList->max = rp->u.multi.planCount;


    protocolList->list = protocols;

    for (i = 0; i < rp->u.multi.planCount; i++)
    {
      protocolList->list[i] = rp->u.multi.plans[i]->u.simple.protocol;
      filters[i]= rp->u.multi.plans[i]->u.simple.filter; 
    }
    
    if (
        ((0 < rp->u.multi.planCount) &&
         (rp->u.multi.plans[0]->type == TMR_READ_PLAN_TYPE_SIMPLE) &&
         (compareAntennas(&rp->u.multi)) &&
         ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) ||
          (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]) ||
          (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0])))
        || (true == reader->continuousReading)
       )
    {
      TMR_SR_SearchFlag antennas = TMR_SR_SEARCH_FLAG_CONFIGURED_LIST;
      antennas |= ((reader->continuousReading)? TMR_SR_SEARCH_FLAG_TAG_STREAMING : 0);
      antennaList = &(rp->u.multi.plans[0]->u.simple.antennas);
      ret = prepForSearch(reader, antennaList);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /**
       * take the time stamp only in case of sync read,
       * async read does not depend on this
       **/
      if (!reader->continuousReading)
      {
        /* Cache the read time so it can be put in tag read data later */
        tm_gettime_consistent(&starttimeHigh, &starttimeLow);
        sr->readTimeHigh = starttimeHigh;
        sr->readTimeLow = starttimeLow;
        sr->lastSentTagTimestampHigh = starttimeHigh;
        sr->lastSentTagTimestampLow = starttimeLow;
      }

      ret = TMR_SR_cmdMultipleProtocolSearch(reader, 
          TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE,
          protocolList, TMR_TRD_METADATA_FLAG_ALL,
          antennas,
          filters,
          (uint16_t)timeoutMs, &count);

      if (NULL != tagCount)
      {
        *tagCount += count;
      }
      return ret;
    }

  }

  if (TMR_READ_PLAN_TYPE_SIMPLE == rp->type)
  {
    antennaList = &rp->u.simple.antennas;
    reader->fastSearch = rp->u.simple.useFastSearch;
    if (!reader->continuousReading)
    {
      /* Currently only supported for sync read case */
      reader->isStopNTags = rp->u.simple.stopOnCount.stopNTriggerStatus;
      reader->numberOfTagsToRead = rp->u.simple.stopOnCount.noOfTags;
    }
  }
  else if (TMR_READ_PLAN_TYPE_MULTI == rp->type)
  {
    uint32_t subTimeout;
    int i;

    subTimeout = 0;
    if (0 == rp->u.multi.totalWeight)
    {
      subTimeout = timeoutMs / rp->u.multi.planCount;
    }

    for (i = 0; i < rp->u.multi.planCount; i++)
    {
      if (rp->u.multi.totalWeight)
      {
        subTimeout = rp->u.multi.plans[i]->weight * timeoutMs 
          / rp->u.multi.totalWeight;
      }
      ret = TMR_SR_read_internal(reader, subTimeout, tagCount, 
        rp->u.multi.plans[i]);
      if (TMR_SUCCESS != ret && TMR_ERROR_NO_TAGS_FOUND != ret)
      {
        return ret;
      }
    }
    return ret;
  }
  else
  {
    return TMR_ERROR_INVALID;
  }

  /* At this point we're guaranteed to have a simple read plan */
  ret = prepForSearch(reader, antennaList);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }
  
  /* Set protocol to that specified by the read plan. */
  ret = setProtocol(reader, rp->u.simple.protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (reader->continuousReading)
  {
    bool value = false;
    ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ENABLE_READ_FILTER, &value);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }

  /* Cache the read time so it can be put in tag read data later */
  tm_gettime_consistent(&starttimeHigh, &starttimeLow);
  sr->readTimeHigh = starttimeHigh;
  sr->readTimeLow = starttimeLow;
  sr->lastSentTagTimestampHigh = starttimeHigh;
  sr->lastSentTagTimestampLow = starttimeLow;

  /* Cache search timeout for later call to streaming receive */
  sr->searchTimeoutMs = timeoutMs;

  elapsed = tm_time_subtract(tmr_gettime_low(), starttimeLow);
  elapsed_tagop = elapsed;
  
  /**
   * Ignoring the elapsed time calculation in case of true
   * continuous reading,
   */
  if (reader->continuousReading)
  {
    elapsed = timeoutMs - 1;
  }

  while (elapsed < timeoutMs)
  {
    readTimeMs = timeoutMs - elapsed;
    if (readTimeMs > 65535)
    {
      readTimeMs = 65535;
    }

    if (NULL == rp->u.simple.tagop)
    {
      if(reader->continuousReading)
      {
        TMR_TagProtocolList p;
        TMR_TagProtocolList *protocolList = &p;
        TMR_TagFilter *filters[TMR_MAX_SERIAL_MULTIPROTOCOL_LENGTH];
        TMR_TagProtocol protocols[TMR_MAX_SERIAL_MULTIPROTOCOL_LENGTH];
        TMR_SR_SearchFlag antennas;
        
        protocolList->len = 1;
        protocolList->max = 1;
        protocolList->list = protocols;

        protocolList->list[0] = rp->u.simple.protocol;
        filters[0]= rp->u.simple.filter;
    
        antennas = TMR_SR_SEARCH_FLAG_CONFIGURED_LIST;
        antennas |= ((reader->continuousReading)? TMR_SR_SEARCH_FLAG_TAG_STREAMING : 0);
        antennaList = &(rp->u.simple.antennas);
        ret = TMR_SR_cmdMultipleProtocolSearch(reader,
                            TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE,
                            protocolList, TMR_TRD_METADATA_FLAG_ALL,
                            antennas,
                            filters,
                            (uint16_t)timeoutMs, &count);
      }
      else
      {
        ret = TMR_SR_cmdReadTagMultiple(reader,(uint16_t)readTimeMs,
          (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
          rp->u.simple.filter,
          rp->u.simple.protocol,
          &count);
      }
    }
    else
    {
      uint8_t msg[256];
      uint8_t i, lenbyte;

      /* Since this is embedded tagop, removing elapsed time based on
       * continuous reading should not be done 
       */
      readTimeMs = timeoutMs - elapsed_tagop;
      if (readTimeMs > 65535)
      {
        readTimeMs = 65535;
      }

      i = 2;
     
      /**
       * add the tagoperation
       **/
      ret = TMR_SR_addTagOp(reader, rp->u.simple.tagop, rp, msg, &i, readTimeMs, &lenbyte);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      msg[lenbyte] = i - (lenbyte + 2); /* Install length of subcommand */
      msg[1] = i - 3; /* Install length */
      ret = TMR_SR_executeEmbeddedRead(reader, msg, (uint16_t)timeoutMs, &multipleStatus);
      count = multipleStatus.tagsFound;


      /* Update embedded tagop success/failure count */
      reader->u.serialReader.tagopSuccessCount += multipleStatus.successCount;
      reader->u.serialReader.tagopFailureCount += multipleStatus.failureCount;
    }

    if (TMR_ERROR_NO_TAGS_FOUND == ret)
    {
      count = 0;
      ret = TMR_SUCCESS;
    }
	else if (TMR_ERROR_TM_ASSERT_FAILED == ret)
	{
	  return ret;
	}
    else if (TMR_ERROR_TIMEOUT == ret)
    {
      return ret;
    }
    else if (TMR_SUCCESS != ret)
    {
      uint16_t remainingTagsCount;
      TMR_Status ret1;
      reader->isStopNTags = false;

      /* Check for the tag count (in case of module error)*/
      ret1 = TMR_SR_cmdGetTagsRemaining(reader, &remainingTagsCount);
      if (TMR_SUCCESS != ret1)
      {
        return ret1;
      }
      sr->tagsRemaining += remainingTagsCount;
      if (NULL != tagCount)
      {
        *tagCount += remainingTagsCount;
      }
      return ret;
    }

    sr->tagsRemaining += count;
    if (NULL != tagCount)
    {
      *tagCount += count;
    }

    if (reader->continuousReading)
    {
      sr->tagsRemaining = 1;
      break;
    }
    else if (reader->isStopNTags && !reader->continuousReading)
    {
      /** 
       * No need to loop back for stop N tags
       **/
      break;
    }
    else
    {
      elapsed = tm_time_subtract(tmr_gettime_low(), starttimeLow);
    }
  }

  return ret;
}

/* Reset reader stats (unless command not supported by reader)
 */
static
TMR_Status
_resetReaderStats(TMR_Reader *reader, TMR_Reader_StatsFlag statFlags)
{
  TMR_Status ret;

  if ((NULL != reader->pSupportsResetStats) && (false == *(reader->pSupportsResetStats)))
  {
    /* Command not supported, just skip it */
    ret = TMR_SUCCESS;
  }
  else
  {
    ret = TMR_SR_cmdResetReaderStats(reader, statFlags);

    /* Initialize reader->pSupportsResetStats, if necessary*/
    if (NULL == reader->pSupportsResetStats)
    {
      switch (ret)
      { 
      case TMR_SUCCESS:
        reader->_storeSupportsResetStats = true;
        reader->pSupportsResetStats = &(reader->_storeSupportsResetStats);
        break;
      case TMR_ERROR_MSG_WRONG_NUMBER_OF_DATA:
      case TMR_ERROR_INVALID_OPCODE:
      case TMR_ERROR_UNIMPLEMENTED_OPCODE:
      case TMR_ERROR_UNIMPLEMENTED_FEATURE:
      case TMR_ERROR_INVALID:
      case TMR_ERROR_UNIMPLEMENTED:
      case TMR_ERROR_UNSUPPORTED:
      case TMR_ERROR_UNSUPPORTED_READER_TYPE:
        /* If command unsupported, make a note not to do it again then
         * proceed normally */
        reader->_storeSupportsResetStats = false;
        reader->pSupportsResetStats = &(reader->_storeSupportsResetStats);
        ret = TMR_SUCCESS;
        break;
      }
    }
  }
  return ret;
}

TMR_Status
TMR_SR_read(struct TMR_Reader *reader, uint32_t timeoutMs, int32_t *tagCount)
{
  TMR_Status ret;
  TMR_ReadPlan *rp;

  /**
   * Reset the reader statistics at the beginning of every search,
   * currently reader statistics only supports in m6e.. 
   * TO DO: Enable this for other readers in future.
   */ 
  ret = _resetReaderStats(reader, TMR_READER_STATS_FLAG_ALL);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (!reader->continuousReading)
  {
    /**
    * In case of sync read only
    * clear tag buffer
    */
    ret = TMR_SR_cmdClearTagBuffer(reader);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }

  reader->u.serialReader.tagsRemaining = 0;

#ifdef TMR_ENABLE_BACKGROUND_READS
  if (false == reader->backgroundEnabled)
  /**
   * if TMR_ENABLE_BACKGROUND_READS is not defined, then
   * only sync read is possible. (Continuous and pseudo async reads
   * are not available)
   **/
#endif
  {
    /* If sync read, then reset tagop result count here */
    reader->u.serialReader.tagopSuccessCount = 0;
    reader->u.serialReader.tagopFailureCount = 0;
  }
  rp = reader->readParams.readPlan;

  if (tagCount)
  {
    *tagCount = 0;
  }
  
  return TMR_SR_read_internal(reader, timeoutMs, tagCount, rp);
}

static TMR_Status
verifySearchStatus(TMR_Reader *reader)
{
  TMR_SR_SerialReader *sr;
  TMR_Status ret;
  uint8_t *msg;
  uint32_t timeoutMs;

  sr = &reader->u.serialReader;
  msg = sr->bufResponse;
  timeoutMs = sr->searchTimeoutMs;

  reader->u.serialReader.crcEnabled = false;
  TMR_SR_cmdStopReading(reader);
  
  while(true)
  {
    ret = TMR_SR_receiveMessage(reader, msg, TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE, timeoutMs);

    if(TMR_SUCCESS == ret || TMR_ERROR_DEVICE_RESET == ret)
    {
      if ((0x2F == msg[2]) && (0x02 == msg[5]))
      {
        /**
         * 0x2F with type 0x02 means stop continuous reading.
         * Module has already pushed all the tagreads before
         * sending this response. i.e., reading is finished
         **/
        reader->u.serialReader.crcEnabled = true;
        return TMR_SUCCESS;
      }
      else if ((0x2F == msg[2]) && (0x01 == msg[3] && (0x00 == msg[4])))
      {
        /**
         * 0x2F with error status can also be treated as response for
         * stop continuous reading.
         **/
        return TMR_SUCCESS;
      }
    }
    else
    {
      /**
       * We might get comm errors here, either CRC or TIMEOUT errors
       * What should we do here??
       **/
      if (TMR_ERROR_CRC_ERROR == ret)
      {
        /**
         * When the module is pushing all tags out, just don't bother about
         * CRC and keep waiting for response for stopReading
         **/
        continue;
      }
      if (TMR_ERROR_TIMEOUT == ret)
      {
        return ret;
      }
    }
  }
}

TMR_Status
TMR_SR_hasMoreTags(struct TMR_Reader *reader)
{
  TMR_SR_SerialReader* sr;
  TMR_Status ret;

  sr = &reader->u.serialReader;

#ifdef TMR_ENABLE_BACKGROUND_READS
  if ((reader->continuousReading) && (0 == sr->tagsRemainingInBuffer))
  {
    uint8_t *msg;
    uint32_t timeoutMs;
    uint8_t response_type_pos;
    uint8_t response_type;
    
    msg = sr->bufResponse;
    timeoutMs = sr->searchTimeoutMs;

    ret = TMR_SR_receiveMessage(reader, msg, TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE, timeoutMs);

    if (TMR_ERROR_TAG_ID_BUFFER_AUTH_REQUEST == ret)
    {
      /* Tag password needed to complete tagop.
       * Parse TagReadData and pass to password-generating callback,
       * which will return the appropriate authentication. */
      uint16_t flags = 0;
      uint8_t bufptr;
      TMR_TagReadData trd;
      TMR_TagAuthentication tauth;

      TMR_TRD_init(&trd);

      /* In case of streaming the flags always start at position 8 */
      bufptr = 8;
      flags = GETU16AT(msg, bufptr);
      bufptr += 2;
      bufptr++;  /* Skip tag count (always = 1) */
      TMR_SR_parseMetadataFromMessage(reader, &trd, flags, &bufptr, msg);
      TMR_SR_postprocessReaderSpecificMetadata(&trd, sr);
      trd.reader = reader;

      // printf("Parsed 0604 TagReadData: readCount=%d rssi=%d ant=%d, freq=%d, t_hi=%X, t_lo=%X\n", 
      //trd.readCount, trd.rssi, trd.antenna, trd.frequency, trd.timestampHigh, trd.timestampLow, trd.phase);
      {
        notify_authreq_listeners(reader, &trd, &tauth);		
      }

      /* TODO: Factor out password generation into callback */
      ret = TMR_SR_cmdAuthReqResponse(reader, &tauth);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      else
      {
        return TMR_ERROR_TAG_ID_BUFFER_AUTH_REQUEST;
      }
    }

    if ((TMR_SUCCESS != ret) && (TMR_ERROR_TAG_ID_BUFFER_FULL != ret)
        && ((TMR_ERROR_NO_TAGS_FOUND != ret)
        || ((TMR_ERROR_NO_TAGS_FOUND == ret) && (0 == msg[1]))))
        
    {
      return ret;
    }

    ret = (0 == GETU16AT(msg, 3)) ? TMR_SUCCESS : TMR_ERROR_CODE(GETU16AT(msg, 3));
    if ((TMR_SUCCESS == ret) && (0 == msg[1]))
    {
      /**
       * In case of streaming and ISO protocol after every search cycle
       * module sends the response for embedded operation status as
       * FF 00 22 00 00.
       * In this case return back with 0x400 error, because success response will deceive
       * the read thread to process it. For GEN2 case we got the response with 0x400 status.
       **/
      return TMR_ERROR_NO_TAGS_FOUND;
    }
    if (((0x2F == msg[2]) && (TMR_ERROR_TAG_ID_BUFFER_FULL == ret))
      ||((0x22 == msg[2]) && (TMR_ERROR_TAG_ID_BUFFER_FULL == ret)))
    {
      return ret;
    }

    if (0x2F == msg[2])
    {
      if (0x02 == msg[5])
      {
        /**
         * 0x2F with type 0x02 means stop continuous reading.
         * Module has already pushed all the tagreads before
         * sending this response. i.e., reading is finished
         **/
        reader->finishedReading = true;
        return TMR_ERROR_END_OF_READING;
      }
	  else if (0x03 == msg[5])
      {
      /**
         * 0x2F with type 0x03 means left over client Auth message/response, ignore it.
         **/		
        return TMR_ERROR_TAG_ID_BUFFER_AUTH_REQUEST;		
      }
      /**
       * Control comes here in case of the response received
       * for start continuous reading command. (0x2F with type 0x01)
       **/
      return TMR_ERROR_NO_TAGS;
    }
    else if (msg[1] < 6)
    { /* Need at least enough bytes to get to Response Type field */
      return TMR_ERROR_PARSE;
    }

    response_type_pos = (0x10 == (msg[5] & 0x10)) ? 10 : 8;

    response_type = msg[response_type_pos];
    switch (response_type)
    {
    case 0x02:
      /* Handle status stream responses */
      reader->isStatusResponse = true;
      sr->bufPointer = 9;
      return TMR_SUCCESS;
    case 0x01:
      /* Stream continues after this message */
      reader->isStatusResponse = false;
      sr->tagsRemainingInBuffer = 1;
      sr->bufPointer = 11;
      return TMR_SUCCESS;
    case 0x00:
      /* Stream ends with this message */
      sr->tagsRemaining = 0;

      if (sr->oldQ.type != TMR_SR_GEN2_Q_INVALID)
      {
        ret = TMR_paramSet(reader, TMR_PARAM_GEN2_Q, &(sr->oldQ));
        if (TMR_SUCCESS != ret)
        {
          return ret;
        }
        sr->oldQ.type = TMR_SR_GEN2_Q_INVALID;
      }

      if (NULL != reader->readParams.readPlan->u.simple.tagop)
      {
        response_type_pos += 7;
        sr->tagopSuccessCount += GETU16(msg, response_type_pos);
        sr->tagopFailureCount += GETU16(msg, response_type_pos);
      }
      if (TMR_SUCCESS == ret)
      { /* If things look good so far, signal that we are done with tags */
        return TMR_ERROR_NO_TAGS;
      }
      /* otherwise feed the error back (should only be TMR_ERROR_TAG_ID_BUFFER_FULL) */
      return ret;
    default:
      /* Unknown response type */
      return TMR_ERROR_PARSE;
    }
  }
  else
#endif
  {
    /**
     * TMR_SR_hasMoreTags control comes here only in case of sync reading
     * And in case of pseudo async reading when TMR_ENABLE_BACKGROUND_READS
     * is defined.
     **/
    ret = (sr->tagsRemaining > 0) ? TMR_SUCCESS : TMR_ERROR_NO_TAGS;
    return ret;
  }
}

TMR_Status
TMR_SR_getNextTag(struct TMR_Reader *reader, TMR_TagReadData *read)
{
  TMR_SR_SerialReader *sr;
  TMR_Status ret;
  uint8_t *msg;
  uint8_t i;
  uint16_t flags = 0;
  uint32_t timeoutMs;
  uint8_t subResponseLen = 0;
  uint8_t crclen = 2 ;
  uint8_t epclen = 0;

  sr = &reader->u.serialReader;
  timeoutMs = sr->searchTimeoutMs;

  {
    msg = sr->bufResponse;

    if (sr->tagsRemaining == 0)
    {
      return TMR_ERROR_NO_TAGS;
    }

    if (sr->tagsRemainingInBuffer == 0)
    {
      /* Fetch the next set of tags from the reader */
      if (reader->continuousReading)
      {
        ret = TMR_SR_hasMoreTags(reader);
        if (TMR_SUCCESS != ret)
        {
          return ret;
        }
      }
      else
      {
        if (reader->u.serialReader.opCode == TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE)
        {
          i = 2;
          SETU8(msg, i, TMR_SR_OPCODE_GET_TAG_ID_BUFFER);
          SETU16(msg, i, TMR_TRD_METADATA_FLAG_ALL);
          SETU8(msg, i, 0); /* read options */
          msg[1] = i-3; /* Install length */
          ret = TMR_SR_send(reader, msg);
          if (TMR_SUCCESS != ret)
          {
            return ret;
          }
          sr->tagsRemainingInBuffer = msg[8];
          sr->bufPointer = 9;
        }
        else if (reader->u.serialReader.opCode == TMR_SR_OPCODE_READ_TAG_ID_SINGLE)
        {
          TMR_SR_receiveMessage(reader, msg, reader->u.serialReader.opCode, timeoutMs);
          sr->tagsRemainingInBuffer = (uint8_t)GETU32AT(msg , 9);
          sr->tagsRemaining = sr->tagsRemainingInBuffer;
          sr->bufPointer = 13 ;
        }
        else
        {
           return TMR_ERROR_INVALID_OPCODE; 
        }
      }
    }

    i = sr->bufPointer;
    if (reader->u.serialReader.opCode == TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE)
    {
      flags = GETU16AT(msg, reader->continuousReading ? 8 : 5);
      TMR_SR_parseMetadataFromMessage(reader, read, flags, &i, msg);
      
    }
    if (reader->u.serialReader.opCode == TMR_SR_OPCODE_READ_TAG_ID_SINGLE)
    {
      flags = GETU16AT(msg, i + 6);
      subResponseLen = msg[i+1];
      i += 7;
      TMR_SR_parseMetadataOnly(reader, read, flags, &i, msg);
      epclen = subResponseLen + 4 - (i - sr->bufPointer) - crclen;
      read->tag.epcByteCount=epclen;
      memcpy(&(read->tag.epc), &msg[i], read->tag.epcByteCount);
      i+=epclen;
      read->tag.crc = GETU16(msg, i);
    }
    sr->bufPointer = i;
    
    
    TMR_SR_postprocessReaderSpecificMetadata(read, sr);

    sr->tagsRemainingInBuffer--;

    if (false == reader->continuousReading)
    {
      sr->tagsRemaining--;
    }
    read->reader = reader;

    return TMR_SUCCESS;
  }
}

TMR_Status 
TMR_SR_writeTag(struct TMR_Reader *reader, const TMR_TagFilter *filter,
                const TMR_TagData *data)
{
  TMR_Status ret;
  TMR_SR_SerialReader *sr;

  sr = &reader->u.serialReader;

  ret = setProtocol(reader, reader->tagOpParams.protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (TMR_TAG_PROTOCOL_GEN2 == reader->tagOpParams.protocol)
  {  
    return TMR_SR_cmdWriteGen2TagEpc(reader, filter, sr->gen2AccessPassword, (uint16_t)(sr->commandTimeout), 
                                 data->epcByteCount, data->epc, 0);
  }
  else
  {
    return TMR_ERROR_UNIMPLEMENTED;
  }
}


TMR_Status
TMR_SR_readTagMemWords(TMR_Reader *reader, const TMR_TagFilter *target, 
                       uint32_t bank, uint32_t wordAddress, 
                       uint16_t wordCount, uint16_t data[])
{
  TMR_Status ret;

  ret = TMR_SR_readTagMemBytes(reader, target, bank, wordAddress * 2,
                               wordCount * 2, (uint8_t *)data);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

#ifndef TMR_BIG_ENDIAN_HOST
  {
    uint16_t i;
    uint8_t *data8;
    /* We used the uint16_t data as temporary storage for the values read,
       but we need to adjust for possible endianness differences.
       This is technically correct on all platforms, though it's a no-op
       on big-endian ones. */
    data8 = (uint8_t *)data;
    for (i = 0; i < wordCount; i++)
    {
      data[i] = (data8[2*i] << 8) | data8[2*i + 1];
    }
  }
#endif

  return TMR_SUCCESS;
}


static TMR_Status
TMR_SR_readTagMemBytesUnaligned(TMR_Reader *reader,
                                const TMR_TagFilter *target, 
                                uint32_t bank, uint32_t byteAddress, 
                                uint16_t byteCount, uint8_t data[])
{
  TMR_Status ret;
  TMR_TagReadData read;
  uint16_t wordCount;
  uint8_t buf[254];
  TMR_SR_SerialReader *sr;

  sr = &reader->u.serialReader;

  wordCount = (uint16_t)((byteCount + 1 + (byteAddress & 1) ) / 2);
  read.data.max = 254;
  read.data.list = buf;
  read.metadataFlags = 0;

  ret = TMR_SR_cmdGEN2ReadTagData(reader, (uint16_t)(sr->commandTimeout),
                                  (TMR_GEN2_Bank)bank, byteAddress / 2, (uint8_t)wordCount,
                                  sr->gen2AccessPassword, target, &read);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  memcpy(data, buf + (byteAddress & 1), byteCount);

  return TMR_SUCCESS;
}


TMR_Status
TMR_SR_readTagMemBytes(TMR_Reader *reader, const TMR_TagFilter *target, 
                       uint32_t bank, uint32_t byteAddress, 
                       uint16_t byteCount, uint8_t data[])
{
  TMR_Status ret;
  TMR_TagReadData read;
  TMR_SR_SerialReader *sr;

  sr = &reader->u.serialReader;

  ret = setProtocol(reader, reader->tagOpParams.protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  read.data.max = byteCount;
  read.data.list = (uint8_t *)data;
  read.metadataFlags = 0;

  if (TMR_TAG_PROTOCOL_GEN2 == reader->tagOpParams.protocol)
  {
    /*
     * Handling unaligned reads takes spare memory; avoid allocating that
     * (on that stack) if not necessary.
     */
    if ((byteAddress & 1) || (byteCount & 1))
    {
      return TMR_SR_readTagMemBytesUnaligned(reader, target, bank, byteAddress,
                                             byteCount, data);
    }

    return TMR_SR_cmdGEN2ReadTagData(reader, (uint16_t)(sr->commandTimeout),
                                     (TMR_GEN2_Bank)bank, byteAddress / 2, byteCount / 2,
                                     sr->gen2AccessPassword, target, &read);
  }
#ifdef TMR_ENABLE_ISO180006B
  else if (TMR_TAG_PROTOCOL_ISO180006B == reader->tagOpParams.protocol)
  {
    return TMR_SR_cmdISO180006BReadTagData(reader,(uint16_t)(sr->commandTimeout),
                                           (uint8_t)byteAddress, (uint8_t)byteCount, target,
                                           &read);
  }
#endif /* TMR_ENABLE_ISO180006B */
  else
  {
    return TMR_ERROR_UNIMPLEMENTED;
  }
}

TMR_Status
TMR_SR_modifyFlash(TMR_Reader *reader, uint8_t sector, uint32_t address,
                   uint32_t password, uint8_t length, const uint8_t data[],
                   uint32_t offset)
{
  /**
   * As only M6e reader and it's varient supports modifying the flash in 
   * application mode.Hence, this check is required.
   */
  if ((TMR_SR_MODEL_M6E == reader->u.serialReader.versionInfo.hardware[0])||
      (TMR_SR_MODEL_M6E_MICRO == reader->u.serialReader.versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_PRC == reader->u.serialReader.versionInfo.hardware[0]))
  {
    return TMR_SR_cmdModifyFlashSector(reader, sector, address, password, length,
                                       data, offset);
  }
  else
    return TMR_ERROR_UNSUPPORTED;
}
TMR_Status
TMR_SR_writeTagMemWords(struct TMR_Reader *reader, const TMR_TagFilter *filter,
                        uint32_t bank, uint32_t address,
                        uint16_t count, const uint16_t data[])
{
  const uint8_t *dataPtr;
#ifndef TMR_BIG_ENDIAN_HOST
  uint8_t buf[254];
  int i;

    for (i = 0 ; i < count ; i++)
    {
      buf[2*i    ] = data[i] >> 8;
      buf[2*i + 1] = data[i] & 0xff;
    }
    dataPtr = buf;
#else
    dataPtr = (const uint8_t *)data;
#endif

    return TMR_SR_writeTagMemBytes(reader, filter, bank, address * 2, count * 2,
                                   dataPtr);
}

TMR_Status
TMR_SR_writeTagMemBytes(struct TMR_Reader *reader, const TMR_TagFilter *filter,
                        uint32_t bank, uint32_t address,
                        uint16_t count, const uint8_t data[])
{
  TMR_Status ret;
  TMR_SR_SerialReader *sr;
  TMR_GEN2_WriteMode mode;
  TMR_GEN2_WriteMode *value = &mode;
 
  sr = &reader->u.serialReader;
  TMR_paramGet(reader, TMR_PARAM_GEN2_WRITEMODE, value);

  ret = setProtocol(reader, reader->tagOpParams.protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (TMR_TAG_PROTOCOL_GEN2 == reader->tagOpParams.protocol)
  {
    /* Buffer for converting to Gen2 native word size.
     * A waste of space, but the only way to do byte/word conversions without
     * munging the original data input.  ReadTagMemBytes is deprecated, anyway,
     * along with all the other protocol-independent methods, so avoid using it,
     * if possible. */
    uint16_t data16[TMR_SR_MAX_PACKET_SIZE/2];
    uint16_t wordCount;
    uint16_t iWord;

    /* Misaligned writes are not permitted */
    if ((address & 1) || (count & 1))
    {
      return TMR_ERROR_INVALID;
    }

    wordCount = count/2;
    for (iWord=0; iWord<wordCount; iWord++)
    {
      data16[iWord] = 0;
      data16[iWord] |= data[(2*iWord)+0];
      data16[iWord] <<= 8;
      data16[iWord] |= data[(2*iWord)+1];
    }

    switch (mode)
    {
    case TMR_GEN2_WORD_ONLY:
      return TMR_SR_cmdGEN2WriteTagData(reader, (uint16_t)(sr->commandTimeout),
        (TMR_GEN2_Bank)bank, address / 2, (uint8_t)count, data,
        sr->gen2AccessPassword, filter);
    case TMR_GEN2_BLOCK_ONLY:
      return TMR_SR_cmdBlockWrite(reader,(uint16_t)sr->commandTimeout, (TMR_GEN2_Bank)bank, address / 2, (uint8_t)(count/2), data16, sr->gen2AccessPassword, filter);
    case TMR_GEN2_BLOCK_FALLBACK:

      ret =  TMR_SR_cmdBlockWrite(reader,(uint16_t)sr->commandTimeout, (TMR_GEN2_Bank)bank, address / 2, (uint8_t)(count/2), data16, sr->gen2AccessPassword, filter);
      if (TMR_SUCCESS == ret)
      {
        return ret;
      }
      else 
      {
        return TMR_SR_cmdGEN2WriteTagData(reader, (uint16_t)(sr->commandTimeout),
          (TMR_GEN2_Bank)bank, address / 2, (uint8_t)count, data,
          sr->gen2AccessPassword, filter);

      }
    default: 
      return TMR_ERROR_INVALID_WRITE_MODE;
    }


  }
#ifdef TMR_ENABLE_ISO180006B
  else if (TMR_TAG_PROTOCOL_ISO180006B == reader->tagOpParams.protocol)
  {
    if (count != 1)
    {
      return TMR_ERROR_INVALID;
    }
    return TMR_SR_cmdISO180006BWriteTagData(reader, (uint16_t)(sr->commandTimeout),
                                            (uint8_t)address, 1, data, filter);
  }
#endif
  else
  {
    return TMR_ERROR_INVALID;
  }
}


TMR_Status
TMR_SR_lockTag(struct TMR_Reader *reader, const TMR_TagFilter *filter,
               TMR_TagLockAction *action)
{
  TMR_Status ret;
  TMR_SR_SerialReader *sr;

  sr = &reader->u.serialReader;

  ret = setProtocol(reader, reader->tagOpParams.protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (TMR_TAG_PROTOCOL_GEN2 == reader->tagOpParams.protocol)
  {
    if (TMR_LOCK_ACTION_TYPE_GEN2_LOCK_ACTION != action->type)
    {
      /* Lock type doesn't match tagop protocol */
      return TMR_ERROR_INVALID;
    }
    return TMR_SR_cmdGEN2LockTag(reader, (uint16_t)sr->commandTimeout,
                                 action->u.gen2LockAction.mask, 
                                 action->u.gen2LockAction.action, 
                                 sr->gen2AccessPassword,
                                 filter);
  }
#ifdef TMR_ENABLE_ISO180006B
  else if (TMR_TAG_PROTOCOL_ISO180006B == reader->tagOpParams.protocol)
  {
    if (TMR_LOCK_ACTION_TYPE_ISO180006B_LOCK_ACTION != action->type)
    {
      /* Lock type doesn't match tagop protocol */
      return TMR_ERROR_INVALID;
    }
    return TMR_SR_cmdISO180006BLockTag(reader, (uint16_t)(sr->commandTimeout),
                                       action->u.iso180006bLockAction.address, 
                                       filter);
  }
#endif
  else
  {
    return TMR_ERROR_UNIMPLEMENTED;
  }
}

TMR_Status
TMR_SR_reboot(TMR_Reader *reader)
{
  TMR_Status ret;

  ret = TMR_SR_cmdrebootReader(reader);

  return ret;
}

TMR_Status
TMR_SR_killTag(struct TMR_Reader *reader, const TMR_TagFilter *filter,
               const TMR_TagAuthentication *auth)
{
  TMR_Status ret;
  TMR_SR_SerialReader *sr;

  sr = &reader->u.serialReader;

  ret = setProtocol(reader, reader->tagOpParams.protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (TMR_TAG_PROTOCOL_GEN2 == reader->tagOpParams.protocol)
  {
    if (TMR_AUTH_TYPE_GEN2_PASSWORD != auth->type)
    {
      /* Auth type doesn't match tagop protocol */
      return TMR_ERROR_INVALID;
    }

    return TMR_SR_cmdKillTag(reader,
                             (uint16_t)(sr->commandTimeout),
                             auth->u.gen2Password,
                             filter);
  }
  else
  {
    return TMR_ERROR_UNIMPLEMENTED;
  }
}

TMR_Status
TMR_SR_gpoSet(struct TMR_Reader *reader, uint8_t count,
              const TMR_GpioPin state[])
{
  TMR_Status ret;
  int i;
  
  for (i = 0; i < count; i++)
  {
    ret = TMR_SR_cmdSetGPIO(reader, state[i].id, state[i].high);
    if (TMR_SUCCESS != ret)
      return ret;
  }

  return TMR_SUCCESS;
}

TMR_Status TMR_SR_gpiGet(struct TMR_Reader *reader, uint8_t *count,
                         TMR_GpioPin state[])
{
  TMR_Status ret;
  TMR_GpioPin pinStates[4];
  uint8_t i,j, numPins;

  numPins = numberof(pinStates);
  ret = TMR_SR_cmdGetGPIO(reader, &numPins, pinStates);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  if (numPins > *count)
  {
    numPins = *count;
  }

  *count = 0;
  j = 0;
  for (i = 0 ; i < numPins ; i++)
  {
    if (!pinStates[i].output)
    {
      /* If pin is input, only then copy to output */
      state[j].id = pinStates[i].id;
      state[j].high = pinStates[i].high;
      state[j].output = pinStates[i].output;
      (*count)++;
      j ++;
    }
  }

  return TMR_SUCCESS;
}

#ifdef TMR_ENABLE_STDIO
TMR_Status
TMR_SR_firmwareLoad(struct TMR_Reader *reader, void *cookie,
                    TMR_FirmwareDataProvider provider)
{
  static const uint8_t magic[] =
    { 0x54, 0x4D, 0x2D, 0x53, 0x50, 0x61, 0x69, 0x6B, 0x00, 0x00, 0x00, 0x02 };

  TMR_Status ret;
  uint8_t buf[256];
  uint16_t packetLen, packetRemaining, size, offset;
  uint32_t len, rate, address, remaining;
  TMR_SR_SerialReader *sr;
  TMR_SR_SerialTransport *transport;

  sr = &reader->u.serialReader;
  transport = &sr->transport;

  remaining = numberof(magic) + 4;
  offset = 0;
  
  while (remaining > 0)
  {
    size = (uint16_t)remaining;
    if (false == provider(cookie, &size, buf + offset))
    {
      return TMR_ERROR_FIRMWARE_FORMAT;
    }
    
    remaining -= size;
    offset += size;
  }

  if (0 != memcmp(buf, magic, numberof(magic)))
  {
    return TMR_ERROR_FIRMWARE_FORMAT;
  }

  len = GETU32AT(buf, 12);

  /* @todo get any params we want to reset */

  /*
   * Drop baud to 9600 so we know for sure what it will be after going
   * back to the bootloader.  (Older firmwares always revert to 9600.
   * Newer ones keep the current baud rate.)
   */
  if (NULL != transport->setBaudRate)
  {
    /**
     * some transport layer does not support baud rate settings.
     * for ex: TCP transport. In that case skip the baud rate
     * settings.
     */ 

    ret = TMR_SR_cmdSetBaudRate(reader, 9600);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    ret = transport->setBaudRate(transport, 9600);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }
  ret = TMR_SR_cmdBootBootloader(reader);
  if ((TMR_SUCCESS != ret)
      /* Invalid Opcode okay -- means "already in bootloader" */
      && (TMR_ERROR_INVALID_OPCODE != ret))
  {
    return ret;
  }

  /*
   * Wait for the bootloader to be entered. 200ms is enough.
   */
  tmr_sleep(200);

  /* Bootloader doesn't support wakeup preambles */
  sr->supportsPreamble = false;

  /* Bootloader doesn't support high speed operation */
  rate = sr->baudRate;
  if (rate > 115200)
  {
    rate = 115200;
  }

  if (NULL != transport->setBaudRate)
  {
    /**
     * some transport layer does not support baud rate settings.
     * for ex: TCP transport. In that case skip the baud rate
     * settings.
     */ 

    ret = TMR_SR_cmdSetBaudRate(reader, rate);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    ret = transport->setBaudRate(transport, rate);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
  }
  ret = TMR_SR_cmdEraseFlash(reader, 2, 0x08959121);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  address = 0;
  remaining = len;
  while (remaining > 0)
  {
    packetLen = 240;
    if (packetLen > remaining)
    {
      packetLen = (uint16_t)remaining;
    }
    offset = 0;
    packetRemaining = packetLen;
    while (packetRemaining > 0)
    {
      size = packetRemaining;
      if (false == provider(cookie, &size, buf + offset))
      {
        return TMR_ERROR_FIRMWARE_FORMAT;
      }
      packetRemaining -= size;
      offset += size;
    }
    ret = TMR_SR_cmdWriteFlashSector(reader, 2, address, 0x02254410,(uint8_t) packetLen,
                                     buf, 0);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    address += packetLen;
    remaining -= packetLen;
  }
  
  return TMR_SR_boot(reader, rate);
}
#endif

static TMR_Status
getHardwareInfo(struct TMR_Reader *reader, void *value)
{
  //TMR_Status ret;
  //uint8_t buf[127];
  char tmp[255];
  //uint8_t count;
  TMR_SR_VersionInfo *info;

  info = &reader->u.serialReader.versionInfo;

  TMR_hexDottedQuad(info->hardware, tmp);
  /**
   * Commented below code to fix the Bug#2117
   */
  /*
     tmp[11] = '-';
     count = 127;
     ret = TMR_SR_cmdGetHardwareVersion(reader, 0, 0, &count, buf);
     if (TMR_SUCCESS != ret)
     {
     count = 0;
     tmp[11] = '\0';
     }

     TMR_bytesToHex(buf, count, tmp + 12);
     TMR_stringCopy(value, tmp, 12 + 2*count);*/
  TMR_stringCopy(value, tmp, (int)strlen(tmp));

  return TMR_SUCCESS;
}

static TMR_Status
getSerialNumber(struct TMR_Reader *reader, void *value)
{
  /* See http://trac/swtree/changeset/6498 for previous implementation */
  TMR_Status ret;
  uint8_t buf[127];
  uint8_t count;
  char tmp[127];
  int tmplen;

  count = 127;
  tmplen = 0;
  ret = TMR_SR_cmdGetHardwareVersion(reader, 0, 0x40, &count, buf);
  if (TMR_SUCCESS != ret)
  {
    count = 0;
  }
  else
  {
    int idx;
    uint8_t len;

    idx = 3;
    len = buf[idx++];
    if (len > (count-3))
    {
      ret = TMR_ERROR_UNIMPLEMENTED;
    }
    else
    {
      for (tmplen=0; tmplen<len; tmplen++)
      {
        tmp[tmplen] = (char)buf[idx+tmplen];
      }
    }
  }
  if (0 == count)
  {
    TMR_String *serial = (TMR_String *)value;
    /**
     * Command failure: 
     * Serial number not implemented on this reader
     * Leave value at default "no value" 
     */
    serial->value[0] = '\0';
    return TMR_SUCCESS;
  }
  TMR_stringCopy(value, tmp, tmplen);
  return ret;
}

/* Abuse the structure layout of TMR_SR_PortPowerAndSettlingTime a bit
 * so that this can be one function instead of three only slightly
 * different pieces of code. Since all three per-port values are
 * represented as uint16_t, take an additional 'offset' parameter that
 * gives the pointer distance from the first element to the element
 * (readPower, writePower, settlingTime) that we actually want to set.
 */
static TMR_Status
setPortValues(struct TMR_Reader *reader, const TMR_PortValueList *list,
              int offset)
{
  TMR_Status ret;
  TMR_SR_PortPowerAndSettlingTime ports[TMR_SR_MAX_ANTENNA_PORTS];
  uint8_t count;
  uint16_t i, j;

  count = numberof(ports);

  ret = TMR_SR_cmdGetAntennaPortPowersAndSettlingTime(reader, &count, ports);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /* If a value is left out, 0 is assumed */
  /*for (j = 0; j < count; j++)
  {
    *(&ports[j].readPower + offset) = 0;
  }*/

  /* 
   * For each settling time in the user's list, try to find an
   * existing entry in the list returned from the reader.
   */
  for (i = 0; i < list->len; i++)
  {
    for (j = 0 ; j < count ; j++)
    {
      if (list->list[i].port == ports[j].port)
      {
        break;
      }
    }
    if (j == count)
    {
      if (count == TMR_SR_MAX_ANTENNA_PORTS)
      {
        return TMR_ERROR_TOO_BIG;
      }
      ports[j].port = list->list[i].port;
      ports[j].readPower = 0;
      ports[j].writePower = 0;
      ports[j].settlingTime = 0;
      count++;
    }

    if (list->list[i].value > 32767 || list->list[i].value < -32768)
    {
      return TMR_ERROR_ILLEGAL_VALUE;
    }
    *(&ports[j].readPower + offset) = (int16_t)(list->list[i].value);
  }
  return TMR_SR_cmdSetAntennaPortPowersAndSettlingTime(reader, count, ports);
}

/* See comment before setPortValues() for the meaning of offset */
static TMR_Status
getPortValues(struct TMR_Reader *reader, TMR_PortValueList *list, int offset)
{
  TMR_Status ret;
  TMR_SR_PortPowerAndSettlingTime ports[TMR_SR_MAX_ANTENNA_PORTS];
  uint8_t count;
  uint16_t i, j;

  count = numberof(ports);

  ret = TMR_SR_cmdGetAntennaPortPowersAndSettlingTime(reader, &count, ports);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  for (i = 0, j = 0; i < count; i++)
  {
    if ((0 == *(&ports[i].readPower + offset)  && (TMR_SR_MODEL_M6E_MICRO != reader->u.serialReader.versionInfo.hardware[0])) 
        || ((TMR_SR_MODEL_M6E_MICRO == reader->u.serialReader.versionInfo.hardware[0]) 
          && (-32768 ==  *(&ports[i].readPower + offset))))
    {
      continue;
    }
    if (j < list->max)
    {
      list->list[j].port = ports[i].port;
      list->list[j].value = (int32_t)*(&ports[i].readPower + offset);
    }
    j++;
  }
  list->len = (uint8_t)j;

  return TMR_SUCCESS;
}

static TMR_Status
TMR_SR_paramSet(struct TMR_Reader *reader, TMR_Param key, const void *value)
{
  TMR_Status ret;
  TMR_SR_Configuration readerkey;
  TMR_SR_ProtocolConfiguration protokey;
  TMR_SR_SerialReader *sr;
  TMR_SR_SerialTransport *transport;

  ret = TMR_SUCCESS;
  sr = &reader->u.serialReader;
  readerkey = TMR_SR_CONFIGURATION_TRANSMIT_POWER_SAVE;
  protokey.protocol = TMR_TAG_PROTOCOL_GEN2;

  if (0 == BITGET(sr->paramConfirmed, key))
  {
    TMR_paramProbe(reader, key);
  }

  if (BITGET(sr->paramConfirmed, key) && (0 == BITGET(sr->paramPresent, key)))
  {
    return TMR_ERROR_NOT_FOUND;
  }

  switch (key)
  {
  case TMR_PARAM_REGION_ID:
    sr->regionId = *(TMR_Region*)value;
    if (reader->connected)
    {
      ret = TMR_SR_cmdSetRegion(reader, sr->regionId);
    }
    break;

  case TMR_PARAM_URI:
  case TMR_PARAM_PRODUCT_GROUP_ID:
  case TMR_PARAM_PRODUCT_GROUP:
  case TMR_PARAM_PRODUCT_ID:
  case TMR_PARAM_TAGREADATA_TAGOPSUCCESSCOUNT:
  case TMR_PARAM_TAGREADATA_TAGOPFAILURECOUNT:
    {
      ret = TMR_ERROR_READONLY;
      break;
    }

  case TMR_PARAM_STATUS_ENABLE_ANTENNAREPORT:
    {
      *(bool *)value ? (reader->streamStats |= TMR_SR_STATUS_ANTENNA) :
        (reader->streamStats &= ~TMR_SR_STATUS_ANTENNA) ;
      break;
    }
  case TMR_PARAM_STATUS_ENABLE_FREQUENCYREPORT:
    {
      *(bool *)value ? (reader->streamStats |= TMR_SR_STATUS_FREQUENCY) :
        (reader->streamStats &= ~TMR_SR_STATUS_FREQUENCY) ;
      break;
    }
  case TMR_PARAM_STATUS_ENABLE_TEMPERATUREREPORT:
    {
      *(bool *)value ? (reader->streamStats |= TMR_SR_STATUS_TEMPERATURE) :
        (reader->streamStats &= ~TMR_SR_STATUS_TEMPERATURE) ;
      break;
    }

  case TMR_PARAM_BAUDRATE:
  {
    uint32_t rate;

    transport = &sr->transport;
    rate = *(uint32_t *)value;

    if (reader->connected)
    {
      if (NULL != transport->setBaudRate)
      {
        /**
         * some transport layer does not support baud rate settings.
         * for ex: TCP transport. In that case skip the baud rate
         * settings.
         */ 
        ret = TMR_SR_cmdSetBaudRate(reader, rate);
        if (TMR_SUCCESS != ret)
        {
          break;
        }
        sr->baudRate = rate;
        transport->setBaudRate(transport, sr->baudRate);
      }
    }
    else
    {
      sr->baudRate = rate;
    }
    break;
  }

  case TMR_PARAM_PROBEBAUDRATES:
  {
    const TMR_uint32List *u32List;
    uint8_t i;
    u32List = value;

    if (u32List->len > sr->probeBaudRates.max)
    {
      ret = TMR_ERROR_TOO_BIG;
      break;
    }

    for(i = 0; i < u32List->len; i++)
    {
      sr->probeBaudRates.list[i] = u32List->list[i];
    }

    sr->probeBaudRates.len = u32List->len;
    break;
  }

  case TMR_PARAM_COMMANDTIMEOUT:
	{
      uint32_t val = *(uint32_t*)value;
	  if (((uint32_t)1<<31) & val)
	  {
	    ret = TMR_ERROR_ILLEGAL_VALUE; 
	  }
	  else
	  {
        sr->commandTimeout = *(uint32_t *)value;
	  }
	}
    break;
  case TMR_PARAM_TRANSPORTTIMEOUT:
    {
      uint32_t val = *(uint32_t*)value;
      if (((uint32_t)1<<31) & val)
      {
        ret = TMR_ERROR_ILLEGAL_VALUE; 
      }
      else
      {
        /**
         * In case user specified the timeout value for connect
         * Enable the usrTimoutEnable option
         */
        sr->transportTimeout = *(uint32_t *)value;
        sr->usrTimeoutEnable = true;
      }
    }
	break;
  case TMR_PARAM_RADIO_ENABLEPOWERSAVE:
	readerkey = TMR_SR_CONFIGURATION_TRANSMIT_POWER_SAVE;
	break;
  case TMR_PARAM_RADIO_ENABLESJC:
	readerkey = TMR_SR_CONFIGURATION_SELF_JAMMER_CANCELLATION;
	break;
  case TMR_PARAM_EXTENDEDEPC:
    readerkey = TMR_SR_CONFIGURATION_EXTENDED_EPC;
    break;

  case TMR_PARAM_TAGREADDATA_UNIQUEBYPROTOCOL:
    if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) || 
        (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]) ||
        (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]))
    {
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_UNIQUE_BY_PROTOCOL, value);
    }
    else
    {
      ret = TMR_ERROR_NOT_FOUND;
    }
    break;
    
  case TMR_PARAM_TAGREADDATA_READFILTERTIMEOUT:
    if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) || 
        (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]) ||
        (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]))
    {
      int32_t timeout = (TMR_DEFAULT_READ_FILTER_TIMEOUT == *(int32_t *)value) ? 0 : *(int32_t *)value;
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_READ_FILTER_TIMEOUT, &timeout);
      if (TMR_SUCCESS == ret)
      {
        reader->u.serialReader.readFilterTimeout = timeout;
      }
    }
    else
    {
      ret = TMR_ERROR_NOT_FOUND;
    }
    break;

  case TMR_PARAM_TAGREADDATA_ENABLEREADFILTER:
    if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) || (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]) ||
        (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]))
    {
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ENABLE_READ_FILTER, value);
      if (TMR_SUCCESS == ret)
      {
        reader->u.serialReader.enableReadFiltering = *(bool *)value;
      }
    }
    else
    {
      ret = TMR_ERROR_READONLY;
    }
    break;

  case TMR_PARAM_RADIO_READPOWER:
	ret = TMR_SR_cmdSetReadTxPower(reader, *(int32_t *)value);
	break;
  case TMR_PARAM_RADIO_WRITEPOWER:
    ret = TMR_SR_cmdSetWriteTxPower(reader, *(int32_t *)value);
    break;

  case TMR_PARAM_RADIO_PORTREADPOWERLIST:
    ret = setPortValues(reader, value, 0);
    break;

  case TMR_PARAM_RADIO_PORTWRITEPOWERLIST:
    ret = setPortValues(reader, value, 1);
    break;

  case TMR_PARAM_ANTENNA_SETTLINGTIMELIST:
    ret = setPortValues(reader, value, 2);
    break;

  case TMR_PARAM_ANTENNA_CHECKPORT:
    readerkey = TMR_SR_CONFIGURATION_SAFETY_ANTENNA_CHECK;
    break;

  case TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI:
    readerkey = TMR_SR_CONFIGURATION_RECORD_HIGHEST_RSSI;
    break;

  case TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM:
    readerkey = TMR_SR_CONFIGURATION_RSSI_IN_DBM;
    break;

  case TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA:
    readerkey = TMR_SR_CONFIGURATION_UNIQUE_BY_ANTENNA;
    break;

  case TMR_PARAM_TAGREADDATA_UNIQUEBYDATA:
    readerkey = TMR_SR_CONFIGURATION_UNIQUE_BY_DATA;
    break;

  case TMR_PARAM_ANTENNA_PORTSWITCHGPOS:
  {
    uint8_t portmask;
    const TMR_uint8List *u8list;
    uint16_t i;

    u8list = value;
    portmask = 0;
    for (i = 0 ; i < u8list->len && i < u8list->max ; i++)
    {
      portmask |= 1 << (u8list->list[i] - 1);
    }
    
    ret = TMR_SR_cmdSetReaderConfiguration(
      reader, TMR_SR_CONFIGURATION_ANTENNA_CONTROL_GPIO, &portmask);

    if (TMR_SUCCESS != ret)
    {
      break;
    }
    ret = initTxRxMapFromPorts(reader);
    
    break;
  }

  case TMR_PARAM_ANTENNA_TXRXMAP:
  {
    const TMR_AntennaMapList *map;
    TMR_AntennaMapList *mymap;
    uint8_t len;
    uint16_t i, j;

    map = value;
    mymap = sr->txRxMap;

    if (map->len > mymap->max)
    {
      ret = TMR_ERROR_TOO_BIG;
      break;
    }

    len = 0;

    for (i = 0 ; i < map->len ; i++)
    {
      if (!HASPORT(sr->portMask, map->list[i].txPort) ||
          !HASPORT(sr->portMask, map->list[i].rxPort))
      {
        return TMR_ERROR_NO_ANTENNA;
      }

      /* Error check for txrxmap */
      for (j = i+1; j < map->len; j++)
      {
        if (map->list[i].antenna == map->list[j].antenna)
        {
          return TMR_ERROR_INVALID_ANTENNA_CONFIG;
        }
      }

      len = i+1;
    }

    for (i = 0; i < len; i ++)
    {
      mymap->list[i] = map->list[i];
    }
    mymap->len = len;
    break;
  }

  case TMR_PARAM_REGION_HOPTABLE:
  {
    const TMR_uint32List *u32list;

    u32list = value;

    ret = TMR_SR_cmdSetFrequencyHopTable(reader, (uint8_t)u32list->len, u32list->list);
    break;
  }

  case TMR_PARAM_REGION_HOPTIME:
    ret = TMR_SR_cmdSetFrequencyHopTime(reader, *(uint32_t *)value);
    break;

  case TMR_PARAM_REGION_LBT_ENABLE:
  {
    uint32_t hopTable[64];
    uint8_t count;

    count = numberof(hopTable);
    ret = TMR_SR_cmdGetFrequencyHopTable(reader, &count, hopTable);
    if (TMR_SUCCESS != ret)
    {
      break;
    }

    ret = TMR_SR_cmdSetRegionLbt(reader, sr->regionId, *(bool *)value);
    if (TMR_SUCCESS != ret)
    {
      break;
    }

    ret = TMR_SR_cmdSetFrequencyHopTable(reader, count, hopTable);
    break;
  }

  case TMR_PARAM_TAGOP_ANTENNA:
  {
    uint16_t i;
    TMR_AntennaMapList *map;
    uint8_t antenna;
    uint8_t txPort, rxPort;

    map = sr->txRxMap;
    antenna = *(uint8_t *)value;

    txPort = rxPort = 0;
    for (i = 0; i < map->len && i < map->max; i++)
    {
      if (map->list[i].antenna == antenna)
      {
        txPort = map->list[i].txPort;
        rxPort = map->list[i].rxPort;
        reader->tagOpParams.antenna = antenna;
        break;
      }
    }
    if (txPort == 0)
    {
      ret = TMR_ERROR_NO_ANTENNA;
    }
    else
    {
      ret = TMR_SR_cmdSetTxRxPorts(reader, txPort, rxPort);
    }
    break;
  }

  case TMR_PARAM_TAGOP_PROTOCOL:
    if (0 == ((1 << (*(TMR_TagProtocol *)value - 1)) &
              sr->versionInfo.protocols))
    {
      ret = TMR_ERROR_UNSUPPORTED;
    }
    else
    {
      reader->tagOpParams.protocol = *(TMR_TagProtocol *)value;
	  if (reader->connected)
	  {
	    ret = setProtocol(reader, reader->tagOpParams.protocol);
		if (TMR_SUCCESS == ret)
		{
		  reader->u.serialReader.currentProtocol = reader->tagOpParams.protocol;
    }
	  }
    }
    break;

  case TMR_PARAM_READ_PLAN:
  {
    const TMR_ReadPlan *plan;
    TMR_ReadPlan tmpPlan;

    plan = value;
    tmpPlan = *plan;

    ret = validateReadPlan(reader, &tmpPlan, 
                sr->txRxMap, sr->versionInfo.protocols);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    
    *reader->readParams.readPlan = tmpPlan;
    break;
  }

  case TMR_PARAM_GPIO_INPUTLIST:
  case TMR_PARAM_GPIO_OUTPUTLIST:
  if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]))
  {
      const TMR_uint8List *u8list;      
      int bit, i, newDirections, pin;

      u8list = value;

      if (key == TMR_PARAM_GPIO_OUTPUTLIST)
      {
        newDirections = 0;
      }
      else
      {
        newDirections = 0x1e;
      }

      for (i = 0 ; i < u8list->len && i < u8list->max ; i++)
      {
        newDirections ^= 1 << u8list->list[i];
      }

      for (pin = 0 ; pin < u8list->len ; pin++)
      {
        bit = 1 << (pin + 1);
        if (sr->gpioDirections == -1
            || (sr->gpioDirections & bit) != (newDirections & bit))
        {
          ret = TMR_SR_cmdSetGPIODirection(reader, u8list->list[pin],
                                           (newDirections & bit) != 0);
          if (TMR_SUCCESS != ret)
          {
            return ret;
          }
        }
      }
      sr->gpioDirections = newDirections;
      break;
    }
  case TMR_PARAM_RADIO_POWERMAX:
  case TMR_PARAM_RADIO_POWERMIN:
  case TMR_PARAM_REGION_SUPPORTEDREGIONS:
  case TMR_PARAM_ANTENNA_PORTLIST:
  case TMR_PARAM_ANTENNA_CONNECTEDPORTLIST:
  case TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS:
  case TMR_PARAM_RADIO_TEMPERATURE:
  case TMR_PARAM_VERSION_HARDWARE:
  case TMR_PARAM_VERSION_MODEL:
  case TMR_PARAM_VERSION_SOFTWARE:
  case TMR_PARAM_ANTENNA_RETURNLOSS:
    ret = TMR_ERROR_READONLY;
    break;

  case TMR_PARAM_POWERMODE:
    if (reader->connected)
    {
      ret = TMR_SR_cmdSetPowerMode(reader, *(TMR_SR_PowerMode *)value);
      if (TMR_SUCCESS == ret)
      {
        sr->powerMode = *(TMR_SR_PowerMode *)value;
      }
    }
    else
    {
      sr->powerMode = *(TMR_SR_PowerMode *)value;
    }
    break;

  case TMR_PARAM_USERMODE:
    ret = TMR_SR_cmdSetUserMode(reader, *(TMR_SR_UserMode *)value);
    break;

  case TMR_PARAM_GEN2_Q:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_Q;
    break;

  case TMR_PARAM_GEN2_TAGENCODING:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TAGENCODING;
    break;

  case TMR_PARAM_GEN2_SESSION:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_SESSION;
    break;

  case TMR_PARAM_GEN2_TARGET:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARGET;
    break;

  case TMR_PARAM_GEN2_BLF:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_LINKFREQUENCY;
    break;

  case TMR_PARAM_GEN2_TARI:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARI;
    break;

  case TMR_PARAM_GEN2_WRITEMODE:
    sr->writeMode = *(TMR_GEN2_WriteMode *)value;
    break;

  case TMR_PARAM_GEN2_BAP:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_BAP;
    break;

#ifdef TMR_ENABLE_ISO180006B
  case TMR_PARAM_ISO180006B_BLF:
    protokey.protocol = TMR_TAG_PROTOCOL_ISO180006B;
    protokey.u.iso180006b = TMR_SR_ISO180006B_CONFIGURATION_LINKFREQUENCY;
    break;
  case TMR_PARAM_ISO180006B_MODULATION_DEPTH:
    {
      protokey.protocol = TMR_TAG_PROTOCOL_ISO180006B;
      protokey.u.iso180006b = TMR_SR_ISO180006B_CONFIGURATION_MODULATION_DEPTH;
      break;
    }
  case TMR_PARAM_ISO180006B_DELIMITER:
    {
      protokey.protocol = TMR_TAG_PROTOCOL_ISO180006B;
      protokey.u.iso180006b = TMR_SR_ISO180006B_CONFIGURATION_DELIMITER;
      break;
    }
#endif /* TMR_ENABLE_ISO180006B */

  case TMR_PARAM_GEN2_ACCESSPASSWORD:
    sr->gen2AccessPassword = *(TMR_GEN2_Password *)value;
    break;

  case TMR_PARAM_LICENSE_KEY:
    {
	  uint32_t supportedProtocols;
      TMR_uint8List *license = (TMR_uint8List *)value;

      ret = TMR_SR_cmdSetProtocolLicenseKey(reader, TMR_SR_SET_LICENSE_KEY, license->list, license->len, &supportedProtocols);
	}
	break;

  case TMR_PARAM_USER_CONFIG:
    {
	  TMR_SR_UserConfigOp *config = (TMR_SR_UserConfigOp *)value;

	  switch(config->op)
	  {
	  case TMR_USERCONFIG_SAVE:
	    /* Save the configuration section to flash */
	    ret = TMR_SR_cmdSetUserProfile(reader, TMR_USERCONFIG_SAVE, config->category, TMR_SR_CUSTOM_CONFIGURATION);
		break;

	  case TMR_USERCONFIG_RESTORE:
	    /* Restore the saved configuration section from flash */
	    ret = TMR_SR_cmdSetUserProfile(reader, TMR_USERCONFIG_RESTORE, config->category, TMR_SR_CUSTOM_CONFIGURATION);
		break;

	  case TMR_USERCONFIG_CLEAR:
	    /*  Clear configuration section from flash, and restore default configuration section */
	    ret = TMR_SR_cmdSetUserProfile(reader, TMR_USERCONFIG_CLEAR, config->category, TMR_SR_CUSTOM_CONFIGURATION);
		break;

    case TMR_USERCONFIG_VERIFY:
      ret = TMR_SR_cmdSetUserProfile(reader, TMR_USERCONFIG_VERIFY, config->category, TMR_SR_CUSTOM_CONFIGURATION);
      break;

  default:
    ret = TMR_ERROR_NOT_FOUND;
  }

	}
	break;
  
  case TMR_PARAM_READER_STATISTICS:
  /* Only RF On time statistic can be reset to 0 */
  ret = TMR_SR_cmdResetReaderStatistics(reader, TMR_SR_READER_STATS_ALL);
  break;

  case TMR_PARAM_READER_STATS:
  {
    if ((NULL != reader->pSupportsResetStats) && (false == *(reader->pSupportsResetStats)))
    {
      /* Command not supported, pop up the error */
      return TMR_ERROR_UNSUPPORTED;
    }

    /* Only RF On time statistic can be reset to 0 */
    ret = TMR_SR_cmdResetReaderStats(reader, TMR_READER_STATS_FLAG_ALL);
    break;
  }

  case TMR_PARAM_READER_STATS_ENABLE:
  {
    if ((NULL != reader->pSupportsResetStats) && (false == *(reader->pSupportsResetStats)))
    {
      /* Command not supported, pop up the error */
      return TMR_ERROR_UNSUPPORTED;
    }

    /* Store the statitics flags requested by the user */
    reader->statsFlag = *(TMR_Reader_StatsFlag *)value;
    break;
  }

  case TMR_PARAM_READER_WRITE_REPLY_TIMEOUT:
  case TMR_PARAM_READER_WRITE_EARLY_EXIT:
    {
      TMR_SR_Gen2ReaderWriteTimeOut timeout;

      /* in case of timeout check for range */
      switch (key)
      {
      case TMR_PARAM_READER_WRITE_REPLY_TIMEOUT:
        {
          if ( *(uint16_t *)value < 1000 || *(uint16_t *)value > 21000)
            return TMR_ERROR_MSG_INVALID_PARAMETER_VALUE;
          break;
        }
      default:
        ;
      }      

      /* Get the values before setting it */
      ret = TMR_SR_cmdGetReaderWriteTimeOut(reader, protokey.protocol, &timeout);
      if (TMR_SUCCESS != ret)
      {
        break;
      }

      /* set the parameter asked by the user */
      switch (key)
      {
      case TMR_PARAM_READER_WRITE_REPLY_TIMEOUT:
        {
          timeout.writetimeout = *(uint16_t *)value;
          break;
        }
      case TMR_PARAM_READER_WRITE_EARLY_EXIT:
        {
          timeout.earlyexit = !(*(bool *)value);
          break;
        }
      default:
        ret = TMR_ERROR_NOT_FOUND;
      }

      /* set the vakue */
      ret = TMR_SR_cmdSetReaderWriteTimeOut (reader,protokey.protocol, &timeout);
      break;
    }    

  default:
    ret = TMR_ERROR_NOT_FOUND;
  }

  switch (key)
  {
  case TMR_PARAM_ANTENNA_CHECKPORT:
  case TMR_PARAM_RADIO_ENABLEPOWERSAVE:
  case TMR_PARAM_RADIO_ENABLESJC:
  case TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI:
  case TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM:
  case TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA:
  case TMR_PARAM_TAGREADDATA_UNIQUEBYDATA:
      ret = TMR_SR_cmdSetReaderConfiguration(reader, readerkey, value);
    break;

  case TMR_PARAM_EXTENDEDEPC:
    {
      if ((TMR_SR_MODEL_M6E == reader->u.serialReader.versionInfo.hardware[0])||
          (TMR_SR_MODEL_M6E_MICRO == reader->u.serialReader.versionInfo.hardware[0])||
          (TMR_SR_MODEL_M6E_PRC == reader->u.serialReader.versionInfo.hardware[0]))
      {
        ret = TMR_ERROR_UNSUPPORTED;
      }
      else
      {
        ret = TMR_SR_cmdSetReaderConfiguration(reader, readerkey, value);
        if(TMR_SUCCESS == ret)
        {
          /* cache the extended epc setting */
          reader->u.serialReader.extendedEPC = *(bool *)value;
        }
      }
    }
    break;

  case TMR_PARAM_GEN2_BAP:
    {
      TMR_GEN2_Bap *bap;
      bap = (TMR_GEN2_Bap *)value;

      if (NULL == (TMR_GEN2_Bap *)value)
      {
        /** It means user is disabling the bap support, make the flag down 
         * and skip the command sending
         **/ 
        sr->isBapEnabled = false;
        break;
      }
      else if ((-1 > bap->powerUpDelayUs) || (-1 > bap->freqHopOfftimeUs))
      {
        /*
         * Invalid values for BAP parameters,
         * Accepts only positive values or -1 for NULL
         */ 
        return TMR_ERROR_ILLEGAL_VALUE;
      }
      else if ((-1 == bap->powerUpDelayUs) && (-1 == bap->freqHopOfftimeUs))
      {
        /**
         * Here -1 signifies NULL. API should skips the parameter and does not sent the matching
         * command to the reader. Let reader use its own default
         **/
        sr->isBapEnabled = true;
        break;
      }
      else
      {
        /*
         * do a paramGet of BAP parameters. This serves two purposes.
         * 1. API knows wheather module supports the BAP options or not.
         * 2. API can assign default values to the fields, are not set by the user.
         */
        TMR_GEN2_Bap getBapParams;

        ret = TMR_SR_cmdGetProtocolConfiguration(reader, protokey.protocol, 
            protokey, &getBapParams);

        if (TMR_SUCCESS != ret)
        {
          /* throw the error and come out from the loop */
          break;
        }
        else
        {
          /*
           * Modify the get BAP structure if either of the set BAP params are -1
           */
          sr->isBapEnabled = true;

          if (-1 == bap->powerUpDelayUs)
          {
            bap->powerUpDelayUs = getBapParams.powerUpDelayUs;
          }
          if (-1 == bap->freqHopOfftimeUs)
          {
            bap->freqHopOfftimeUs = getBapParams.freqHopOfftimeUs;
          }
        }
      }
      // No break -- fall through to regular Gen2 param handler
    }

  case TMR_PARAM_GEN2_Q:
  case TMR_PARAM_GEN2_TAGENCODING:
  case TMR_PARAM_GEN2_SESSION:
  case TMR_PARAM_GEN2_TARGET:
  case TMR_PARAM_GEN2_BLF:
  case TMR_PARAM_GEN2_TARI:
#ifdef TMR_ENABLE_ISO180006B
  case TMR_PARAM_ISO180006B_BLF:
  case TMR_PARAM_ISO180006B_MODULATION_DEPTH:
  case TMR_PARAM_ISO180006B_DELIMITER:
#endif /* TMR_ENABLE_ISO180006B */
    ret = TMR_SR_cmdSetProtocolConfiguration(reader, protokey.protocol, 
                                             protokey, value);
    break;

  default:
    ;
  }
  return ret;
}

static TMR_Status
TMR_SR_paramGet(struct TMR_Reader *reader, TMR_Param key, void *value)
{
  TMR_Status ret;
  TMR_SR_Configuration readerkey;
  TMR_SR_ProtocolConfiguration protokey;
  TMR_SR_SerialReader *sr;

  ret = TMR_SUCCESS;
  sr = &reader->u.serialReader;
  readerkey = TMR_SR_CONFIGURATION_TRANSMIT_POWER_SAVE;
  protokey.protocol = TMR_TAG_PROTOCOL_GEN2;

  if (BITGET(sr->paramConfirmed, key) && 0 == BITGET(sr->paramPresent, key))
  {
    return TMR_ERROR_NOT_FOUND;
  }

  switch (key)
  {
  case TMR_PARAM_BAUDRATE:
    *(uint32_t *)value = sr->baudRate;
    break;

  case TMR_PARAM_PROBEBAUDRATES:
    {
      TMR_uint32List *uint32List;
      uint8_t i;

      uint32List = value;

      if (sr->probeBaudRates.len > uint32List->max)
      {
        sr->probeBaudRates.len = uint32List->max;
      }

      if (sr->probeBaudRates.len)
      {
        for(i = 0; i < sr->probeBaudRates.len; i++)
        {
          uint32List->list[i] = sr->probeBaudRates.list[i];
        }
        uint32List->len = sr->probeBaudRates.len;
      }
      else
      {
        //list is empty
      }
      break;
    }

  case TMR_PARAM_URI:
    if (NULL != value)
    {
      TMR_stringCopy((TMR_String *)value, reader->uri, (int)strlen(reader->uri));
    }
    else
    {
      ret = TMR_ERROR_ILLEGAL_VALUE;
    }
    break;

  case TMR_PARAM_COMMANDTIMEOUT:
    *(uint32_t *)value = sr->commandTimeout;
    break;

  case TMR_PARAM_TRANSPORTTIMEOUT:
    *(uint32_t *)value = sr->transportTimeout;
    break;

  case TMR_PARAM_REGION_ID:
    {
      if ((TMR_REGION_NONE == sr->regionId) && (reader->connected))
      {
        ret = TMR_SR_cmdGetRegion(reader, &sr->regionId);
      }
      *(TMR_Region *)value = sr->regionId;
    }
	break;

  case TMR_PARAM_RADIO_ENABLEPOWERSAVE:
	readerkey = TMR_SR_CONFIGURATION_TRANSMIT_POWER_SAVE;
	break;

  case TMR_PARAM_RADIO_ENABLESJC:
    readerkey = TMR_SR_CONFIGURATION_SELF_JAMMER_CANCELLATION;
	break;

  case TMR_PARAM_TAGREADDATA_READFILTERTIMEOUT:
  if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) || 
      (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]) ||
      (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]))
  {
    *(int32_t *)value = sr->readFilterTimeout;
  }
  else
  {
    ret = TMR_ERROR_NOT_FOUND;
  }
  break;

  case TMR_PARAM_TAGREADDATA_ENABLEREADFILTER:
    *(bool *)value = sr->enableReadFiltering;
    break;

  case TMR_PARAM_EXTENDEDEPC:
    readerkey = TMR_SR_CONFIGURATION_EXTENDED_EPC;
    break;

  case TMR_PARAM_RADIO_POWERMAX:
  {
    TMR_SR_PowerWithLimits power;

    ret = TMR_SR_cmdGetReadTxPowerWithLimits(reader, &power);
    if (TMR_SUCCESS != ret)
    {
      break;
    }
    *(int16_t *)value = power.maxPower;
    break;
  }

  case TMR_PARAM_RADIO_POWERMIN:
  {
    TMR_SR_PowerWithLimits power;

    ret = TMR_SR_cmdGetReadTxPowerWithLimits(reader, &power);
    if (TMR_SUCCESS != ret)
      break;
    *(int16_t *)value = power.minPower;
    break;
  }

  case TMR_PARAM_RADIO_READPOWER:
    ret = TMR_SR_cmdGetReadTxPower(reader, (int32_t *)value);
    break;

  case TMR_PARAM_RADIO_WRITEPOWER:
    ret = TMR_SR_cmdGetWriteTxPower(reader, (int32_t *)value);
    break;


  case TMR_PARAM_ANTENNA_CHECKPORT:
    readerkey = TMR_SR_CONFIGURATION_SAFETY_ANTENNA_CHECK;
    break;

  case TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI:
    readerkey = TMR_SR_CONFIGURATION_RECORD_HIGHEST_RSSI;
    break;

  case TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM:
    readerkey = TMR_SR_CONFIGURATION_RSSI_IN_DBM;
    break;

  case TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA:
    readerkey = TMR_SR_CONFIGURATION_UNIQUE_BY_ANTENNA;
    break;

  case TMR_PARAM_TAGREADDATA_UNIQUEBYDATA:
    readerkey = TMR_SR_CONFIGURATION_UNIQUE_BY_DATA;
    break;

  case TMR_PARAM_TAGREADDATA_UNIQUEBYPROTOCOL:
    if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) || 
        (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]) ||
        (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]))
    {
      ret = TMR_SR_cmdGetReaderConfiguration(reader, TMR_SR_CONFIGURATION_UNIQUE_BY_PROTOCOL, value);
    }
    else
    {
      ret = TMR_ERROR_NOT_FOUND;
    }
    break;

  case TMR_PARAM_PRODUCT_GROUP_ID:
    readerkey = TMR_SR_CONFIGURATION_PRODUCT_GROUP_ID;
    break;

  case TMR_PARAM_PRODUCT_ID:
    readerkey = TMR_SR_CONFIGURATION_PRODUCT_ID;
    break;

  case TMR_PARAM_STATUS_ENABLE_ANTENNAREPORT:
    *(bool *)value = (reader->streamStats & TMR_SR_STATUS_ANTENNA) ? true : false;
    break;
  case TMR_PARAM_STATUS_ENABLE_FREQUENCYREPORT:
    *(bool *)value = (reader->streamStats & TMR_SR_STATUS_FREQUENCY) ? true : false;
    break;
  case TMR_PARAM_STATUS_ENABLE_TEMPERATUREREPORT:
    *(bool *)value = (reader->streamStats & TMR_SR_STATUS_TEMPERATURE) ? true : false;
    break;

  case TMR_PARAM_ANTENNA_PORTSWITCHGPOS:
  {
    uint8_t portmask;
    TMR_uint8List *u8list;

    u8list = value;

    ret = TMR_SR_cmdGetReaderConfiguration(
      reader, TMR_SR_CONFIGURATION_ANTENNA_CONTROL_GPIO, &portmask);
    if (TMR_SUCCESS != ret)
    {
      break;
    }
    u8list->len = 0;
    if (portmask & 1)
    {
      LISTAPPEND(u8list, 1);
    }
    if (portmask & 2)
    {
      LISTAPPEND(u8list, 2);
    }
    break;
  }

  case TMR_PARAM_ANTENNA_SETTLINGTIMELIST:
    ret = getPortValues(reader, value, 2);
    break;

  case TMR_PARAM_RADIO_PORTREADPOWERLIST:
    ret = getPortValues(reader, value, 0);
    break;

  case TMR_PARAM_RADIO_PORTWRITEPOWERLIST:
    ret = getPortValues(reader, value, 1);
    break;
  
  case TMR_PARAM_ANTENNA_RETURNLOSS:
    ret = TMR_SR_cmdGetAntennaReturnLoss(reader, value);
    break;

  case TMR_PARAM_GPIO_INPUTLIST:
  case TMR_PARAM_GPIO_OUTPUTLIST:
  {
    TMR_uint8List *u8list;

    u8list = value;

    u8list->len = 0;
    if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) ||
        (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]) ||
        (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]))
    {
      int pin, wantout;
      bool out;
      TMR_GpioPin pinStates[16];
      uint8_t numPins;

      numPins = numberof(pinStates);
      ret = TMR_SR_cmdGetGPIO(reader, &numPins, pinStates);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
 
      wantout = (key == TMR_PARAM_GPIO_OUTPUTLIST) ? 1 : 0;
      if (-1 == sr->gpioDirections)
      {
        /* Cache the current state */
        sr->gpioDirections = 0;
        for (pin = 1; pin <= numPins ; pin++)
        {
          ret = TMR_SR_cmdGetGPIODirection(reader, pin, &out);
          if (TMR_SUCCESS != ret)
          {
            return ret;
          }
          if (out)
          {
            sr->gpioDirections |= 1 << pin;
          }
        }
      }
      for (pin = 1; pin <= numPins ; pin++)
      {
        if (wantout == ((sr->gpioDirections >> pin) & 1))
        {
          LISTAPPEND(u8list, pin);
        }
      }
    }
    else
    {
      LISTAPPEND(u8list, 1);
      LISTAPPEND(u8list, 2);
    }
    break;
  }

  case TMR_PARAM_ANTENNA_PORTLIST:
  {
    uint8_t i;
    TMR_uint8List *u8list;

    u8list = value;

    u8list->len = 0;
    for (i = 0; i < reader->u.serialReader.txRxMap->len; i++)
    {
      LISTAPPEND(u8list, reader->u.serialReader.txRxMap->list[i].antenna);
    }
    break;
  }

  case TMR_PARAM_ANTENNA_CONNECTEDPORTLIST:
  {
    // Store detected ports in array for quick lookup
    bool detected[TMR_SR_MAX_ANTENNA_PORTS+1];
    uint8_t i;
    TMR_uint8List *u8list;

    for (i=0; i<TMR_SR_MAX_ANTENNA_PORTS; i++)
    {
      detected[i] = false;
    }
    u8list = value;

    {
      TMR_SR_PortDetect ports[TMR_SR_MAX_ANTENNA_PORTS];
      uint8_t numPorts;

      numPorts = numberof(ports);
      ret = TMR_SR_cmdAntennaDetect(reader, &numPorts, ports);
      if (TMR_SUCCESS != ret)
      {
        break;
      }
      for (i=0; i<numPorts; i++)
      {
        detected[ports[i].port] = ports[i].detected;
      }
    }
    
    u8list->len = 0;
    for (i=0; i<reader->u.serialReader.txRxMap->len; i++)
    {
      int ant = reader->u.serialReader.txRxMap->list[i].antenna;
      int tx  = reader->u.serialReader.txRxMap->list[i].txPort;
      int rx  = reader->u.serialReader.txRxMap->list[i].rxPort;

      if (detected[tx] && detected[rx])
      {
        LISTAPPEND(u8list, ant);
      }
    }
    break;
  }

  case TMR_PARAM_ANTENNA_TXRXMAP:
  {
    TMR_AntennaMapList *map, *mymap;
    uint16_t i;

    map = value;
    mymap = sr->txRxMap;

    for (i = 0 ; i < mymap->len && i < map->max ; i++)
      map->list[i] = mymap->list[i];
    map->len = mymap->len;
    break;
  }

  case TMR_PARAM_REGION_HOPTABLE:
  {
    TMR_uint32List *u32List;
    uint8_t count;

    u32List = value;

    count = (uint8_t)u32List->max;
    ret = TMR_SR_cmdGetFrequencyHopTable(reader,&count, u32List->list);
    if (TMR_SUCCESS != ret)
    {
      break;
    }
    u32List->len = count;
    break;
  }

  case TMR_PARAM_REGION_HOPTIME:
    ret = TMR_SR_cmdGetFrequencyHopTime(reader, value);
    break;

  case TMR_PARAM_REGION_LBT_ENABLE:
    ret = TMR_SR_cmdGetRegionConfiguration(reader,
                                           TMR_SR_REGION_CONFIGURATION_LBT_ENABLED, value);
    if (TMR_SUCCESS != ret && TMR_ERROR_IS_CODE(ret))
    {
      *(bool *)value = false;
      ret = TMR_SUCCESS;
    }
    break;

  case TMR_PARAM_TAGOP_ANTENNA:
    *(uint8_t *)value = reader->tagOpParams.antenna;
    break;

  case TMR_PARAM_TAGOP_PROTOCOL:
    *(TMR_TagProtocol *)value = reader->tagOpParams.protocol;
    break;

  case TMR_PARAM_POWERMODE:
    if (reader->connected)
    {
      TMR_SR_PowerMode pm;
      ret = TMR_SR_cmdGetPowerMode(reader, &pm);
      if (TMR_SUCCESS == ret)
      {
        sr->powerMode = pm;
      }
    }
    *(TMR_SR_PowerMode*)value = sr->powerMode;
    break;

  case TMR_PARAM_USERMODE:
    ret = TMR_SR_cmdGetUserMode(reader, value);
    break;

  case TMR_PARAM_GEN2_Q:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_Q;
    break;

  case TMR_PARAM_GEN2_TAGENCODING:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TAGENCODING;
    break;

  case TMR_PARAM_GEN2_SESSION:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_SESSION;
    break;

  case TMR_PARAM_GEN2_TARGET:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARGET;
    break;

  case TMR_PARAM_GEN2_BLF:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_LINKFREQUENCY;
    break;

  case TMR_PARAM_GEN2_TARI:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARI;
    break;
    
  case TMR_PARAM_GEN2_BAP:
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_BAP;
    break;

  case TMR_PARAM_GEN2_WRITEMODE:
    *(TMR_GEN2_WriteMode *)value = sr->writeMode;
    break;

#ifdef TMR_ENABLE_ISO180006B
  case TMR_PARAM_ISO180006B_BLF:
    protokey.protocol = TMR_TAG_PROTOCOL_ISO180006B;
    protokey.u.iso180006b = TMR_SR_ISO180006B_CONFIGURATION_LINKFREQUENCY;
    break;
  case TMR_PARAM_ISO180006B_MODULATION_DEPTH:
    {
      protokey.protocol = TMR_TAG_PROTOCOL_ISO180006B;
      protokey.u.iso180006b = TMR_SR_ISO180006B_CONFIGURATION_MODULATION_DEPTH;
      break;
    }
  case TMR_PARAM_ISO180006B_DELIMITER:
    {
      protokey.protocol = TMR_TAG_PROTOCOL_ISO180006B;
      protokey.u.iso180006b = TMR_SR_ISO180006B_CONFIGURATION_DELIMITER;
      break;
    }
#endif /* TMR_ENABLE_ISO180006B */

  case TMR_PARAM_GEN2_ACCESSPASSWORD:
    *(TMR_GEN2_Password *)value = sr->gen2AccessPassword;
    break;

  case TMR_PARAM_REGION_SUPPORTEDREGIONS:
    ret = TMR_SR_cmdGetAvailableRegions(reader, value);
    break;

  case TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS:
    ret = TMR_SR_cmdGetAvailableProtocols(reader, value);
    break;

  case TMR_PARAM_RADIO_TEMPERATURE:
    ret = TMR_SR_cmdGetTemperature(reader, value);
    break;

  case TMR_PARAM_VERSION_HARDWARE:
    ret = getHardwareInfo(reader, value);
    break;

  case TMR_PARAM_VERSION_SERIAL:
    ret = getSerialNumber(reader, value);
    break;

  case TMR_PARAM_VERSION_MODEL:
  {
    const char *model;

    switch (sr->versionInfo.hardware[0])
    {
    case TMR_SR_MODEL_M5E:
      model = "M5e";
      break;
    case TMR_SR_MODEL_M5E_COMPACT:
      model = "M5e Compact";
      break;
    case TMR_SR_MODEL_M5E_I:
      {
        /**
         * I - International . It has following Variants.
         **/
        switch (sr->versionInfo.hardware[3])
        {
          case TMR_SR_MODEL_M5E_I_REV_EU:
            model = "M5e EU";
            break;
          case TMR_SR_MODEL_M5E_I_REV_NA:
            model = "M5e NA";
            break;
          case TMR_SR_MODEL_M5E_I_REV_JP:
            model = "M5E JP";
            break;
          case TMR_SR_MODEL_M5E_I_REV_PRC:
            model = "M5e PRC";
            break;
          default:
            model = "Unknown";
            break;
        }
        break;
      }
    case TMR_SR_MODEL_M4E:
      model = "M4e";
      break;
    case TMR_SR_MODEL_M6E:
      model = "M6e";
      break;
    case TMR_SR_MODEL_M6E_MICRO:
      model = "M6e Micro";
      break;
    case TMR_SR_MODEL_M6E_PRC:
      model = "M6e PRC";
      break;
    default:
      model = "Unknown";
    }
    TMR_stringCopy(value, model, (int)strlen(model));
    break;
  }    

  case TMR_PARAM_VERSION_SOFTWARE:
  {
    char tmp[38];
    TMR_SR_VersionInfo *info;

    info = &sr->versionInfo;

    TMR_hexDottedQuad(info->fwVersion, tmp);
    tmp[11] = '-';
    TMR_hexDottedQuad(info->fwDate, tmp + 12);
    tmp[23] = '-';
    tmp[24] = 'B';
    tmp[25] = 'L';
    TMR_hexDottedQuad(info->bootloader, tmp+26);
    TMR_stringCopy(value, tmp, 37);
    break;
  }

  case TMR_PARAM_LICENSE_KEY:
    ret = TMR_ERROR_UNSUPPORTED;
	break;
  
  case TMR_PARAM_USER_CONFIG:
    ret = TMR_ERROR_UNSUPPORTED;
	break;

  case TMR_PARAM_READER_STATS:
  {
    if ((NULL != reader->pSupportsResetStats) && (false == *(reader->pSupportsResetStats)))
    {
      /* Command not supported, pop up the error */
      return TMR_ERROR_UNSUPPORTED;
    }
    
    /**
     * We should ask for the fields which are requested by the user,
     * if no fields are requested by the user, then fetch all fields.
     */
    if (TMR_READER_STATS_FLAG_NONE == reader->statsFlag)
    {
      reader->statsFlag = TMR_READER_STATS_FLAG_ALL;
    }
    ret = TMR_SR_cmdGetReaderStats(reader, reader->statsFlag, value);
    break;
  }

  case TMR_PARAM_READER_STATS_ENABLE:
  {
    if ((NULL != reader->pSupportsResetStats) && (false == *(reader->pSupportsResetStats)))
    {
      /* Command not supported, pop up the error */
      return TMR_ERROR_UNSUPPORTED;
    }

    *(TMR_Reader_StatsFlag *)value = reader->statsFlag;
    break;
  }

  case TMR_PARAM_READER_STATISTICS:
  ret = TMR_SR_cmdGetReaderStatistics(reader, TMR_SR_READER_STATS_ALL, value);
  break;

  case TMR_PARAM_PRODUCT_GROUP:
    {
      const char *group;
      TMR_SR_ProductGroupID id = (TMR_SR_ProductGroupID)sr->productId;
      switch (id)
      {
        case TMR_SR_PRODUCT_MODULE:
        case TMR_SR_PRODUCT_INVALID:
          group = "Embedded Reader";
          break;
        case TMR_SR_PRODUCT_RUGGEDIZED_READER:
          group = "Ruggedized Reader";
          break;
        case TMR_SR_PRODUCT_USB_READER:
          group = "USB Reader";
          break;
        default:
          group = "Unknown";
          break;
      }
      if (NULL != value)
      {
        TMR_stringCopy(value, group, (int)strlen(group));
      }
      else
      {
        ret = TMR_ERROR_ILLEGAL_VALUE;
      }
      break;
    }

  case TMR_PARAM_TAGREADATA_TAGOPSUCCESSCOUNT:
    {
      *(uint16_t *)value = sr->tagopSuccessCount;
      break;
    }

  case TMR_PARAM_TAGREADATA_TAGOPFAILURECOUNT:
    {
      *(uint16_t *)value = sr->tagopFailureCount;
      break;
    }
  case TMR_PARAM_READER_WRITE_REPLY_TIMEOUT:
  case TMR_PARAM_READER_WRITE_EARLY_EXIT:
    {
      TMR_SR_Gen2ReaderWriteTimeOut timeout;
      ret = TMR_SR_cmdGetReaderWriteTimeOut(reader, protokey.protocol, &timeout);
      if (TMR_SUCCESS != ret)
      {
        break;
      }
      switch (key)
      {
      case TMR_PARAM_READER_WRITE_REPLY_TIMEOUT:
        {
          *(uint16_t *)value = (uint16_t)timeout.writetimeout;
          break;
        }
      case TMR_PARAM_READER_WRITE_EARLY_EXIT:
        {
          *(bool *)value = !((bool)timeout.earlyexit);
          break;
        }
      default:
        ret = TMR_ERROR_NOT_FOUND;
      }
    }
    break;
		    
  default:
    ret = TMR_ERROR_NOT_FOUND;
  }

  switch (key)
  {
  case TMR_PARAM_ANTENNA_CHECKPORT:
  case TMR_PARAM_RADIO_ENABLEPOWERSAVE:
  case TMR_PARAM_RADIO_ENABLESJC:
  case TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI:
  case TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM:
  case TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA:
  case TMR_PARAM_TAGREADDATA_UNIQUEBYDATA:  
  case TMR_PARAM_PRODUCT_GROUP_ID:
  case TMR_PARAM_PRODUCT_ID:
	  ret = TMR_SR_cmdGetReaderConfiguration(reader, readerkey, value);
	  break;

  case TMR_PARAM_EXTENDEDEPC:
    {
      if ((TMR_SR_MODEL_M6E == sr->versionInfo.hardware[0]) ||
          (TMR_SR_MODEL_M6E_PRC == sr->versionInfo.hardware[0]) ||
          (TMR_SR_MODEL_M6E_MICRO == sr->versionInfo.hardware[0]))
      {
        ret = TMR_ERROR_UNSUPPORTED;
      }
      else
      {
        ret = TMR_SR_cmdGetReaderConfiguration(reader, readerkey, value);
      }
    }
    break;

  case TMR_PARAM_GEN2_Q:
  case TMR_PARAM_GEN2_TAGENCODING:
  case TMR_PARAM_GEN2_SESSION:
  case TMR_PARAM_GEN2_TARGET:
  case TMR_PARAM_GEN2_BLF:
  case TMR_PARAM_GEN2_TARI:
  case TMR_PARAM_GEN2_BAP:
#ifdef TMR_ENABLE_ISO180006B
  case TMR_PARAM_ISO180006B_BLF:
  case TMR_PARAM_ISO180006B_MODULATION_DEPTH:
  case TMR_PARAM_ISO180006B_DELIMITER:
#endif /* TMR_ENABLE_ISO180006B */
	  ret = TMR_SR_cmdGetProtocolConfiguration(reader, protokey.protocol, 
		  protokey, value);
	  break;

  default:
	  ;
  }

  if (0 == BITGET(sr->paramConfirmed, key))
  {
    if (TMR_SUCCESS == ret)
    {
      BITSET(sr->paramPresent, key);
    }
    BITSET(sr->paramConfirmed, key);
  }

  return ret;
}

TMR_Status
TMR_SR_SerialReader_init(TMR_Reader *reader)
{
  reader->readerType = TMR_READER_TYPE_SERIAL;

#ifndef TMR_ENABLE_SERIAL_READER_ONLY
  reader->connect = TMR_SR_connect;
  reader->destroy = TMR_SR_destroy;
  reader->read = TMR_SR_read;
  reader->hasMoreTags = TMR_SR_hasMoreTags;
  reader->getNextTag = TMR_SR_getNextTag;
  reader->executeTagOp = TMR_SR_executeTagOp;
  reader->readTagMemWords = TMR_SR_readTagMemWords;
  reader->readTagMemBytes = TMR_SR_readTagMemBytes;
  reader->writeTagMemBytes = TMR_SR_writeTagMemBytes;
  reader->writeTagMemWords = TMR_SR_writeTagMemWords;
  reader->writeTag = TMR_SR_writeTag;
  reader->killTag = TMR_SR_killTag;
  reader->lockTag = TMR_SR_lockTag;
  reader->gpiGet = TMR_SR_gpiGet;
  reader->gpoSet = TMR_SR_gpoSet;
#ifdef TMR_ENABLE_STDIO
  reader->firmwareLoad = TMR_SR_firmwareLoad;
#endif
  reader->modifyFlash = TMR_SR_modifyFlash;
  reader->reboot = TMR_SR_reboot;
#endif
#ifdef TMR_ENABLE_BACKGROUND_READS
  reader->cmdStopReading = TMR_SR_cmdStopReading;
#endif

  reader->paramSet = TMR_SR_paramSet;
  reader->paramGet = TMR_SR_paramGet;
 
  memset(reader->u.serialReader.paramConfirmed,0,
         sizeof(reader->u.serialReader.paramConfirmed));
  memset(reader->u.serialReader.paramPresent,0,
         sizeof(reader->u.serialReader.paramPresent));
  reader->u.serialReader.baudRate = 115200;
  reader->u.serialReader.currentProtocol = TMR_TAG_PROTOCOL_NONE;
  reader->u.serialReader.versionInfo.hardware[0] = TMR_SR_MODEL_UNKNOWN;
  reader->u.serialReader.supportsPreamble = false;
  reader->u.serialReader.extendedEPC = false;
  reader->u.serialReader.powerMode = TMR_SR_POWER_MODE_INVALID;
  reader->u.serialReader.transportTimeout = 1000;
  reader->u.serialReader.commandTimeout = 1000;
  reader->u.serialReader.regionId = TMR_REGION_NONE;
  reader->u.serialReader.tagsRemaining = 0;
  reader->u.serialReader.tagsRemainingInBuffer = 0;
  reader->u.serialReader.gen2AccessPassword = 0;
  reader->u.serialReader.oldQ.type = TMR_SR_GEN2_Q_INVALID;
  reader->u.serialReader.writeMode = TMR_GEN2_WORD_ONLY;
  reader->u.serialReader.tagopSuccessCount = 0;
  reader->u.serialReader.tagopFailureCount = 0;
  reader->u.serialReader.enableReadFiltering = true;
  reader->u.serialReader.readFilterTimeout = 0;
  reader->u.serialReader.usrTimeoutEnable = false;
  reader->u.serialReader.crcEnabled = true;
  reader->u.serialReader.transportType = TMR_SR_MSG_SOURCE_UNKNOWN;
  reader->u.serialReader.gen2AllMemoryBankEnabled = false;
  reader->u.serialReader.isBapEnabled = false;
  reader->u.serialReader.probeBaudRates.list = reader->u.serialReader.baudRates;
  reader->u.serialReader.probeBaudRates.max = TMR_MAX_PROBE_BAUDRATE_LENGTH;
  reader->u.serialReader.probeBaudRates.len = 0;
  {
    //initialize the probe baud rate list with the supported  baud rate values
    TMR_uint32List value;
    uint32_t rates[TMR_MAX_PROBE_BAUDRATE_LENGTH] ={9600, 115200, 921600, 19200, 38400, 57600,230400, 460800};
    value.list = rates;
    value.len = value.max = 8;

    TMR_paramSet(reader, TMR_PARAM_PROBEBAUDRATES, &value);
  }

  return TMR_reader_init_internal(reader);
}

TMR_Status
TMR_SR_executeTagOp(struct TMR_Reader *reader, TMR_TagOp *tagop, TMR_TagFilter *filter, TMR_uint8List *data)
{ 
  TMR_Status ret;
  TMR_SR_SerialReader *sr;

  sr = &reader->u.serialReader;

  switch (tagop->type)
  {
  case (TMR_TAGOP_GEN2_WRITETAG):
    {
      TMR_TagOp_GEN2_WriteTag op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.writeTag;

      return TMR_SR_cmdWriteGen2TagEpc(reader, filter, sr->gen2AccessPassword, 
        (uint16_t)(sr->commandTimeout),
        op.epcptr->epcByteCount,
        op.epcptr->epc,
        false);
    }
  case (TMR_TAGOP_GEN2_KILL):
    {
	  TMR_TagOp_GEN2_Kill op;
           
	  ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
	  if (TMR_SUCCESS != ret)
	  {
		return ret;
	  }

      op = tagop->u.gen2.u.kill;

      return TMR_SR_cmdKillTag(reader,
        (uint16_t)(sr->commandTimeout),
        op.password,
        filter);
    }
  case (TMR_TAGOP_GEN2_LOCK):
    {
	  TMR_TagOp_GEN2_Lock op;
           
	  ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
	  if (TMR_SUCCESS != ret)
	  {
		return ret;
	  }

      op = tagop->u.gen2.u.lock;

      return TMR_SR_cmdGEN2LockTag(reader, (uint16_t)sr->commandTimeout,
        op.mask, op.action, op.accessPassword, filter);
    }
  case (TMR_TAGOP_GEN2_WRITEDATA):
    {
      TMR_TagOp_GEN2_WriteData op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
	return ret;
      }

      op = tagop->u.gen2.u.writeData;
      return TMR_SR_writeTagMemWords(reader, filter, op.bank, op.wordAddress, op.data.len, op.data.list);
    }
  case (TMR_TAGOP_GEN2_READDATA):
    {
      TMR_TagOp_GEN2_ReadData op;
      TMR_TagReadData read;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
    }

      if (NULL != data)
      {
        read.data.len = 0;
        read.data.list = data->list;
        read.data.max = data->max;
      }
      else
      {
        read.data.list = NULL;
      }
      read.metadataFlags = 0;
      op = tagop->u.gen2.u.readData;
      ret = TMR_SR_cmdGEN2ReadTagData(reader, (uint16_t)(sr->commandTimeout), op.bank,
                                      op.wordAddress, op.len, sr->gen2AccessPassword, filter, &read);
      if (NULL != data)
      {
        data->len = read.data.len;
      }
      
      return ret;
    }
  case (TMR_TAGOP_GEN2_BLOCKWRITE):
    {
      TMR_TagOp_GEN2_BlockWrite op;
           
	  ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
	  if (TMR_SUCCESS != ret)
	  {
		return ret;
    }

    op = tagop->u.gen2.u.blockWrite;
    return TMR_SR_cmdBlockWrite(reader,(uint16_t)sr->commandTimeout, op.bank,
            op.wordPtr, op.data.len, op.data.list, sr->gen2AccessPassword, filter);
    }
  case (TMR_TAGOP_GEN2_BLOCKPERMALOCK):
    {
      TMR_TagOp_GEN2_BlockPermaLock op;
           
	  ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
	  if (TMR_SUCCESS != ret)
	  {
		return ret;
    }

      op = tagop->u.gen2.u.blockPermaLock;

      return TMR_SR_cmdBlockPermaLock(reader, (uint16_t)sr->commandTimeout, op.readLock, op.bank,
        op.blockPtr, op.mask.len, op.mask.list, sr->gen2AccessPassword, filter, data);
    }

  case (TMR_TAGOP_GEN2_BLOCKERASE):
    {
      TMR_TagOp_GEN2_BlockErase op;
      
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.blockErase;
      return TMR_SR_cmdBlockErase(reader, (uint16_t)sr->commandTimeout, op.bank, op.wordPtr,
                                                op.wordCount, sr->gen2AccessPassword, filter);
    }

  case (TMR_TAGOP_GEN2_ALIEN_HIGGS2_PARTIALLOADIMAGE):
    {
      TMR_TagOp_GEN2_Alien_Higgs2_PartialLoadImage op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      op = tagop->u.gen2.u.custom.u.alien.u.higgs2.u.partialLoadImage;
      if (op.epcptr->epcByteCount > 12 || op.epcptr->epcByteCount <=0 )
      { /* Only 96 bit epc */
        return TMR_ERROR_PROTOCOL_INVALID_EPC;
      }
      return TMR_SR_cmdHiggs2PartialLoadImage(reader, (uint16_t)sr->commandTimeout, 
        op.accessPassword, op.killPassword, op.epcptr->epcByteCount, op.epcptr->epc, filter);
    }
  case (TMR_TAGOP_GEN2_ALIEN_HIGGS2_FULLLOADIMAGE):
    {
      TMR_TagOp_GEN2_Alien_Higgs2_FullLoadImage op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.custom.u.alien.u.higgs2.u.fullLoadImage;
      if (op.epcptr->epcByteCount > 12 || op.epcptr->epcByteCount <= 0 )
      { /* Only 96 bit epc */
        return TMR_ERROR_PROTOCOL_INVALID_EPC;
      }
      return TMR_SR_cmdHiggs2FullLoadImage(reader, (uint16_t)sr->commandTimeout, op.accessPassword,
        op.killPassword, op.lockBits, op.pcWord, op.epcptr->epcByteCount, op.epcptr->epc, filter);
    }
  case (TMR_TAGOP_GEN2_ALIEN_HIGGS3_FASTLOADIMAGE):
    {
      /* The FastLoadImage command automatically erases the content of all
       * User Memory. If this is undesirable then use the LoadImage or multiple Write
       * Tag Data commands.
       */
      TMR_TagOp_GEN2_Alien_Higgs3_FastLoadImage op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.custom.u.alien.u.higgs3.u.fastLoadImage;
      if (op.epcptr->epcByteCount > 12 || op.epcptr->epcByteCount <= 0 )
      { /* Only 96 bit epc */
        return TMR_ERROR_PROTOCOL_INVALID_EPC;
      }
      return TMR_SR_cmdHiggs3FastLoadImage(reader, (uint16_t)sr->commandTimeout, op.currentAccessPassword, op.accessPassword,
        op.killPassword, op.pcWord, op.epcptr->epcByteCount, op.epcptr->epc, filter);
    }
  case (TMR_TAGOP_GEN2_ALIEN_HIGGS3_LOADIMAGE):
    {
      TMR_TagOp_GEN2_Alien_Higgs3_LoadImage op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.custom.u.alien.u.higgs3.u.loadImage;
      if (op.epcAndUserData->len > 76 || op.epcAndUserData->len <= 0 )
      { /* Only 76 byte epcAndUserData */
        return TMR_ERROR_MSG_INVALID_PARAMETER_VALUE;
      }
      return TMR_SR_cmdHiggs3LoadImage(reader, (uint16_t)sr->commandTimeout, op.currentAccessPassword,
        op.accessPassword, op.killPassword, op.pcWord, (uint8_t)op.epcAndUserData->len, op.epcAndUserData->list, filter);
    }
  case (TMR_TAGOP_GEN2_ALIEN_HIGGS3_BLOCKREADLOCK):
    {
      TMR_TagOp_GEN2_Alien_Higgs3_BlockReadLock op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      op = tagop->u.gen2.u.custom.u.alien.u.higgs3.u.blockReadLock;

      return TMR_SR_cmdHiggs3BlockReadLock(reader, (uint16_t)sr->commandTimeout, op.accessPassword, op.lockBits, filter);
    }
  case (TMR_TAGOP_GEN2_NXP_SETREADPROTECT):
    {
      TMR_TagOp_GEN2_NXP_SetReadProtect op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      
      op = tagop->u.gen2.u.custom.u.nxp.u.setReadProtect;

      return TMR_SR_cmdNxpSetReadProtect(reader, (uint16_t)sr->commandTimeout,
        tagop->u.gen2.u.custom.chipType, op.accessPassword, filter);
    }
  case (TMR_TAGOP_GEN2_NXP_RESETREADPROTECT):
    {
      TMR_TagOp_GEN2_NXP_ResetReadProtect op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      
      op = tagop->u.gen2.u.custom.u.nxp.u.resetReadProtect;
      return TMR_SR_cmdNxpResetReadProtect(reader, (uint16_t)sr->commandTimeout,
          tagop->u.gen2.u.custom.chipType, op.accessPassword, filter);
    }
  case (TMR_TAGOP_GEN2_NXP_CHANGEEAS):
    {
      TMR_TagOp_GEN2_NXP_ChangeEAS op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.custom.u.nxp.u.changeEAS;
      return TMR_SR_cmdNxpChangeEas(reader, (uint16_t)sr->commandTimeout, tagop->u.gen2.u.custom.chipType,
        op.accessPassword, op.reset, filter);
    }
  case (TMR_TAGOP_GEN2_NXP_EASALARM):
    {
      TMR_TagOp_GEN2_NXP_EASAlarm op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      op = tagop->u.gen2.u.custom.u.nxp.u.EASAlarm;
      return TMR_SR_cmdNxpEasAlarm(reader, (uint16_t)sr->commandTimeout, tagop->u.gen2.u.custom.chipType,
        op.dr, op.m, op.trExt, data, filter);
    }
  case (TMR_TAGOP_GEN2_NXP_CALIBRATE):
    {
      TMR_TagOp_GEN2_NXP_Calibrate op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      
      op = tagop->u.gen2.u.custom.u.nxp.u.calibrate;
      return TMR_SR_cmdNxpCalibrate(reader, (uint16_t)sr->commandTimeout, tagop->u.gen2.u.custom.chipType,
        op.accessPassword, data, filter);
    }
  case (TMR_TAGOP_GEN2_NXP_CHANGECONFIG):
    {
      TMR_TagOp_GEN2_NXP_ChangeConfig op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
       
      if(NULL != data)
      {
        data->len = 0;      
      }

      op = tagop->u.gen2.u.custom.u.nxp.u.changeConfig;
      return TMR_SR_cmdNxpChangeConfig(reader, (uint16_t)sr->commandTimeout, tagop->u.gen2.u.custom.chipType,
        op.accessPassword, op.configWord, data, filter);
    }
  case (TMR_TAGOP_GEN2_IMPINJ_MONZA4_QTREADWRITE):
    {
      TMR_TagOp_GEN2_Impinj_Monza4_QTReadWrite op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.impinj.u.monza4.u.qtReadWrite;
      return TMR_SR_cmdMonza4QTReadWrite(reader, (uint16_t)sr->commandTimeout, op.accessPassword,
        op.controlByte, op.payload, data, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_GETSENSOR):
    {
      TMR_TagOp_GEN2_IDS_SL900A_GetSensorValue op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.sensor;
      return TMR_SR_cmdSL900aGetSensorValue(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password,op.sl900A.level, op.sl900A.sensortype, data, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_GETMEASUREMENTSETUP):
    {
      TMR_TagOp_GEN2_IDS_SL900A_GetMeasurementSetup op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.measurementSetup;
      return TMR_SR_cmdSL900aGetMeasurementSetup(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password,op.sl900A.level, data, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_GETCALIBRATIONDATA):
    {
      TMR_TagOp_GEN2_IDS_SL900A_GetCalibrationData op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.calibrationData;
      return TMR_SR_cmdSL900aGetCalibrationData(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password, op.sl900A.level, data, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_SETCALIBRATIONDATA):
    {
      TMR_TagOp_GEN2_IDS_SL900A_SetCalibrationData op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.setCalibration;
      return TMR_SR_cmdSL900aSetCalibrationData(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password, op.sl900A.level, op.cal.raw, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_SETSFEPARAMETERS):
    {
      TMR_TagOp_GEN2_IDS_SL900A_SetSfeParameters op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.setSfeParameters;
      return TMR_SR_cmdSL900aSetSfeParameters(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password, op.sl900A.level, op.sfe->raw, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_GETLOGSTATE):

    {
      TMR_TagOp_GEN2_IDS_SL900A_GetLogState op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.getLog;
      return TMR_SR_cmdSL900aGetLogState(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password, op.sl900A.level, data, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_SETLOGMODE):
    {
      TMR_TagOp_GEN2_IDS_SL900A_SetLogMode op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.setLogMode;
      return TMR_SR_cmdSL900aSetLogMode(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password, op.sl900A.level, op.sl900A.dataLog, op.sl900A.rule, op.Ext1Enable, op.Ext2Enable, op.TempEnable,
        op.BattEnable, op.LogInterval, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_INITIALIZE):
    {
      TMR_TagOp_GEN2_IDS_SL900A_Initialize op;
      
      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.initialize;

      return TMR_SR_cmdSL900aInitialize(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password, op.sl900A.level, op.delayTime.raw, op.applicationData.raw, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_ENDLOG):
    {
      TMR_TagOp_GEN2_IDS_SL900A_EndLog op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.endLog;
      return TMR_SR_cmdSL900aEndLog(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password,op.sl900A.level, filter);
    }
    
  case (TMR_TAGOP_GEN2_IDS_SL900A_SETPASSWORD):
    {
      TMR_TagOp_GEN2_IDS_SL900A_SetPassword op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.setPassword;
      return TMR_SR_cmdSL900aSetPassword(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password,op.sl900A.level, op.NewPassword, op.NewPasswordLevel, filter);
    }

  case (TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOSTATUS):
    {
      TMR_TagOp_GEN2_IDS_SL900A_AccessFifoStatus op;
      
      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.accessFifoStatus;
      return TMR_SR_cmdSL900aAccessFifoStatus(reader, (uint16_t)sr->commandTimeout, op.status.AccessPassword, op.status.CommandCode,
        op.status.Password, op.status.sl900A.level, op.status.operation, data, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOREAD):
    {
      TMR_TagOp_GEN2_IDS_SL900A_AccessFifoRead op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.accessFifoRead;
      return TMR_SR_cmdSL900aAccessFifoRead(reader, (uint16_t)sr->commandTimeout, op.read.AccessPassword, op.read.CommandCode,
        op.read.Password, op.read.sl900A.level, op.read.operation,op.length, data, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOWRITE):
    {
      TMR_TagOp_GEN2_IDS_SL900A_AccessFifoWrite op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.accessFifoWrite;
      return TMR_SR_cmdSL900aAccessFifoWrite(reader, (uint16_t)sr->commandTimeout, op.write.AccessPassword, op.write.CommandCode,
        op.write.Password, op.write.sl900A.level, op.write.operation, op.payLoad, data, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_STARTLOG):
    {
      TMR_TagOp_GEN2_IDS_SL900A_StartLog op;
      
      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }
      
      op = tagop->u.gen2.u.custom.u.ids.u.startLog;

      return TMR_SR_cmdSL900aStartLog(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
        op.Password, op.sl900A.level, op.startTime, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_GETBATTERYLEVEL):
    {
      TMR_TagOp_GEN2_IDS_SL900A_GetBatteryLevel op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.batteryLevel;

      return TMR_SR_cmdSL900aGetBatteryLevel(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password, op.sl900A.level, op.batteryType, data, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_SETLOGLIMITS):
    {
      TMR_TagOp_GEN2_IDS_SL900A_SetLogLimits op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.setLogLimit;

      return TMR_SR_cmdSL900aSetLogLimit(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password, op.sl900A.level, op.limit.extremeLower, op.limit.lower, op.limit.upper, op.limit.extremeUpper, filter);
    }
  case (TMR_TAGOP_GEN2_IDS_SL900A_SETSHELFLIFE):
    {
      TMR_TagOp_GEN2_IDS_SL900A_SetShelfLife op;

      /* Set the protocol for tag opearation */
      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      /* Do some error checking */
      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.ids.u.setShelfLife;

      return TMR_SR_cmdSL900aSetShelfLife(reader, (uint16_t)sr->commandTimeout, op.AccessPassword, op.CommandCode,
          op.Password, op.sl900A.level, op.shelfLifeBlock0->raw, op.shelfLifeBlock1->raw, filter);
    }
  case (TMR_TAGOP_GEN2_DENATRAN_IAV_ACTIVATESECUREMODE):
    {
      TMR_TagOp_GEN2_Denatran_IAV_Activate_Secure_Mode op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.secureMode;
      return TMR_SR_cmdIAVDenatranCustomOp(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter);
    }
  case (TMR_TAGOP_GEN2_DENATRAN_IAV_AUTHENTICATEOBU):
    {
      TMR_TagOp_GEN2_Denatran_IAV_Authenticate_OBU op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.authenticateOBU;
      return TMR_SR_cmdIAVDenatranCustomOp(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter);
    }
  case (TMR_TAGOP_GEN2_ACTIVATE_SINIAV_MODE):
    {
      TMR_TagOp_GEN2_Denatran_IAV_Activate_Siniav_Mode op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.activateSiniavMode;
      return TMR_SR_cmdIAVDenatranCustomActivateSiniavMode(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
        op.mode, op.payload, data, filter, op.isTokenDesc, op.token);
    }
  case (TMR_TAGOP_GEN2_OBU_AUTH_ID):
    {
      TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_ID op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthId;
      return TMR_SR_cmdIAVDenatranCustomOp(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter);
    }
  case (TMR_TAGOP_GEN2_AUTHENTICATE_OBU_FULL_PASS1):
    {
      TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass1 op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass1;
      return TMR_SR_cmdIAVDenatranCustomOp(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter);
    }
  case (TMR_TAGOP_GEN2_AUTHENTICATE_OBU_FULL_PASS2):
    {
      TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass2 op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass2;
      return TMR_SR_cmdIAVDenatranCustomOp(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter);
    }
  case (TMR_TAGOP_GEN2_OBU_READ_FROM_MEM_MAP):
    {
      TMR_TagOp_GEN2_Denatran_IAV_OBU_ReadFromMemMap op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.obuReadFromMemMap;
      return TMR_SR_cmdIAVDenatranCustomReadFromMemMap(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter, op.readPtr);
    }
  case (TMR_TAGOP_GEN2_DENATRAN_IAV_READ_SEC):
    {
      TMR_TagOp_GEN2_Denatran_IAV_Read_Sec op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.readSec;
      return TMR_SR_cmdIAVDenatranCustomReadSec(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter, op.readPtr);

    }
  case (TMR_TAGOP_GEN2_OBU_WRITE_TO_MEM_MAP):
    {

      TMR_TagOp_GEN2_Denatran_IAV_OBU_WriteToMemMap op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.obuWriteToMemMap;
      return TMR_SR_cmdIAVDenatranCustomWriteToMemMap(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
        op.mode, op.payload, data, filter, op.writePtr, op.wordData,op.tagIdentification, op.dataBuf);
    }
  case (TMR_TAGOP_GEN2_DENATRAN_IAV_WRITE_SEC):
    {

      TMR_TagOp_GEN2_Denatran_IAV_Write_Sec op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.writeSec;
      return TMR_SR_cmdIAVDenatranCustomWriteSec(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, op.payload, data, filter, op.dataWords, op.dataBuf);
    }
 
  case (TMR_TAGOP_GEN2_DENATRAN_IAV_GET_TOKEN_ID):
    {
      TMR_TagOp_GEN2_Denatran_IAV_Get_Token_Id op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if ( TMR_SUCCESS != ret)
      {
        return ret;
      }

      if (NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.getTokenId;
      return TMR_SR_cmdIAVDenatranCustomGetTokenId(reader, (uint16_t)sr->commandTimeout, sr->gen2AccessPassword,
          op.mode, data, filter);
    }
  case (TMR_TAGOP_GEN2_DENATRAN_IAV_AUTHENTICATE_OBU_FULL_PASS):
    {
      TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass op;

      ret = setProtocol(reader, TMR_TAG_PROTOCOL_GEN2);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }

      if(NULL != data)
      {
        data->len = 0;
      }

      op = tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass;
      return TMR_SR_cmdIAVDenatranCustomOp(reader, (uint16_t)sr->commandTimeout,
          sr->gen2AccessPassword, op.mode, op.payload, data, filter);
    }

#ifdef TMR_ENABLE_ISO180006B
  case (TMR_TAGOP_ISO180006B_READDATA):
    {
      TMR_TagOp_ISO180006B_ReadData op;
	    TMR_TagReadData read;
	    
	    ret = setProtocol(reader, TMR_TAG_PROTOCOL_ISO180006B);
	    if (TMR_SUCCESS != ret)
	    {
	      return ret;
      }

      if (NULL != data)
      {
        read.data.max = data->max;
        read.data.len = 0;
        read.data.list = data->list;
      }
      else
      {
        read.data.list = NULL;
      }
      read.metadataFlags = 0;

      op = tagop->u.iso180006b.u.readData;
	    ret = TMR_SR_cmdISO180006BReadTagData(reader, (uint16_t)sr->commandTimeout,
                op.byteAddress, op.len, filter, &read);
      if (NULL != data)
      {
        data->len = read.data.len;
      }
      return ret;
    }
  case (TMR_TAGOP_ISO180006B_WRITEDATA):
    {
      TMR_TagOp_ISO180006B_WriteData op;
           
	  ret = setProtocol(reader, TMR_TAG_PROTOCOL_ISO180006B);
	  if (TMR_SUCCESS != ret)
	  {
		return ret;
    }
      op = tagop->u.iso180006b.u.writeData;
      return TMR_SR_cmdISO180006BWriteTagData(reader, (uint16_t)(sr->commandTimeout), op.byteAddress,
        (uint8_t)op.data.len, op.data.list, filter);
    }
  case (TMR_TAGOP_ISO180006B_LOCK):
    {
      TMR_TagOp_ISO180006B_Lock op;
  
	  ret = setProtocol(reader, TMR_TAG_PROTOCOL_ISO180006B);
	  if (TMR_SUCCESS != ret)
	  {
	    return ret;
	  }

      op = tagop->u.iso180006b.u.lock;
      return TMR_SR_cmdISO180006BLockTag(reader, (uint16_t)(sr->commandTimeout), op.address, filter);

    }

#endif /* TMR_ENABLE_ISO180006B */
  default:
    {
      return TMR_ERROR_UNIMPLEMENTED_FEATURE; 
    }

  }  
}

/**
 * Internal method used for adding the tagop
 **/ 
TMR_Status
TMR_SR_addTagOp(struct TMR_Reader *reader, TMR_TagOp *tagop,TMR_ReadPlan *rp, uint8_t *msg, uint8_t *j, uint32_t readTimeMs, uint8_t *byte )
{
 
  TMR_Status ret = TMR_SUCCESS;
  TMR_SR_SerialReader *sr;
  uint8_t i, lenbyte;
  i = *j; lenbyte = *byte;

  sr = &reader->u.serialReader;
  

  switch (tagop->type)
  {
    case (TMR_TAGOP_GEN2_WRITETAG):
      {
        TMR_TagOp_GEN2_WriteTag *args;

        args = &rp->u.simple.tagop->u.gen2.u.writeTag;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2WriteTagEPC(msg,&i, 0, args->epcptr->epc, args->epcptr->epcByteCount);
        break;
      }
    case TMR_TAGOP_GEN2_READDATA:
      {
        TMR_TagOp_GEN2_ReadData *args;

        args = &rp->u.simple.tagop->u.gen2.u.readData;

        /**
         * If user wants to read all the memory bank data,
         * In that case args.bank value should be greater than 3
         **/
        if ((uint8_t)args->bank > 3)
        {
          /* enable the gen2AllMemoryBankEnabled option */
          sr->gen2AllMemoryBankEnabled = true;
        }

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2DataRead(msg, &i, 2000, args->bank, args->wordAddress,
            args->len, 0x00, false);
        break;
      }
    case TMR_TAGOP_GEN2_SECURE_READDATA:
      {
        TMR_TagOp_GEN2_SecureReadData *args;
        uint8_t accessPassword[4];
        int index = 0;

        /* Enable the Secure ReadData option */
        isSecureAccessEnabled = true;

        args = &rp->u.simple.tagop->u.gen2.u.secureReadData;
        if (args->passwordType == TMR_SECURE_GEN2_LOOKUP_TABLE_PASSWORD)
        {
          /* Do this in case of look up table */
          accessPassword[index++] = args->password.secureAddressLength;
          accessPassword[index++] = args->password.secureAddressOffset;
          accessPassword[index++] = (args->password.secureFlashOffset >> 8);
          accessPassword[index++] = (args->password.secureFlashOffset & 0xFF);

          sr->gen2AccessPassword = (uint32_t)accessPassword[0] << 24 |
            (uint32_t)accessPassword[1] << 16 |
            (uint32_t)accessPassword[2] << 8  |
            (uint32_t)accessPassword[3];
        }
        else
        {
          /* Do this in case of Gen2 password */
          sr->gen2AccessPassword = args->password.gen2PassWord.u.gen2Password;
        }

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND | TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2DataRead(msg, &i, 2000, args->readData.bank, args->readData.wordAddress,
            args->readData.len,args->type, false);

        break;
      }
    case TMR_TAGOP_GEN2_WRITEDATA:
      {
        TMR_TagOp_GEN2_WriteData *args;
        int idx ;

        args = &rp->u.simple.tagop->u.gen2.u.writeData;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2DataWrite(msg, &i, 0, args->bank, args->wordAddress);

        for(idx = 0 ; idx< args->data.len; idx++)
        {
          msg[i++]= (args->data.list[idx]>>8) & 0xFF;
          msg[i++]= (args->data.list[idx]>>0) & 0xFF;
        }
        break;
      }
    case TMR_TAGOP_GEN2_LOCK:
      {
        TMR_TagOp_GEN2_Lock *args;

        args = &rp->u.simple.tagop->u.gen2.u.lock;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddGEN2LockTag(msg, &i, 0, args->mask, args->action, 0);
        break;
      }
    case TMR_TAGOP_GEN2_KILL:
      {
        TMR_TagOp_GEN2_Kill *args;

        args = &rp->u.simple.tagop->u.gen2.u.kill;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2KillTag(msg, &i, 0, args->password);
        break;
      }
    case TMR_TAGOP_GEN2_BLOCKWRITE:
      {
        TMR_TagOp_GEN2_BlockWrite *args;
        args = &rp->u.simple.tagop->u.gen2.u.blockWrite;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2BlockWrite(msg, &i, 0, args->bank, args->wordPtr, args->data.len, args->data.list, 0, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_BLOCKPERMALOCK:
      {
        TMR_TagOp_GEN2_BlockPermaLock *args;
        args = &rp->u.simple.tagop->u.gen2.u.blockPermaLock;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2BlockPermaLock(msg, &i, 0,args->readLock, args->bank, args->blockPtr, args->mask.len, 
            args->mask.list, 0, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_BLOCKERASE:
      {
        TMR_TagOp_GEN2_BlockErase *args;
        args = &rp->u.simple.tagop->u.gen2.u.blockErase;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddGEN2BlockErase(msg, &i, 0, args->wordPtr, args->bank, args->wordCount, 0, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_ALIEN_HIGGS2_PARTIALLOADIMAGE:
      {
        TMR_TagOp_GEN2_Alien_Higgs2_PartialLoadImage *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.alien.u.higgs2.u.partialLoadImage;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddHiggs2PartialLoadImage(msg, &i, 0, args->accessPassword, args->killPassword, 
            args->epcptr->epcByteCount, args->epcptr->epc, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_ALIEN_HIGGS2_FULLLOADIMAGE:
      {
        TMR_TagOp_GEN2_Alien_Higgs2_FullLoadImage *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.alien.u.higgs2.u.fullLoadImage;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddHiggs2FullLoadImage(msg, &i, 0, args->accessPassword, args->killPassword, args->lockBits, 
            args->pcWord, args->epcptr->epcByteCount, args->epcptr->epc, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_ALIEN_HIGGS3_FASTLOADIMAGE:
      {
        TMR_TagOp_GEN2_Alien_Higgs3_FastLoadImage *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.alien.u.higgs3.u.fastLoadImage;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->currentAccessPassword, &lenbyte);

        TMR_SR_msgAddHiggs3FastLoadImage(msg, &i, 0, args->currentAccessPassword, args->accessPassword, 
            args->killPassword, args->pcWord, args->epcptr->epcByteCount, args->epcptr->epc, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_ALIEN_HIGGS3_LOADIMAGE:
      {
        TMR_TagOp_GEN2_Alien_Higgs3_LoadImage *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.alien.u.higgs3.u.loadImage;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->currentAccessPassword, &lenbyte);

        TMR_SR_msgAddHiggs3LoadImage(msg, &i, 0, args->currentAccessPassword, args->accessPassword, 
            args->killPassword, args->pcWord, (uint8_t)args->epcAndUserData->len, args->epcAndUserData->list, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_ALIEN_HIGGS3_BLOCKREADLOCK:
      {
        TMR_TagOp_GEN2_Alien_Higgs3_BlockReadLock *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.alien.u.higgs3.u.blockReadLock;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddHiggs3BlockReadLock(msg, &i, 0, args->accessPassword, args->lockBits, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_NXP_SETREADPROTECT:
      {
        TMR_TagOp_GEN2_NXP_SetReadProtect *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.nxp.u.setReadProtect;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddNXPSetReadProtect(msg, &i, 0, rp->u.simple.tagop->u.gen2.u.custom.chipType, args->accessPassword, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_NXP_RESETREADPROTECT:
      {
        TMR_TagOp_GEN2_NXP_ResetReadProtect *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.nxp.u.resetReadProtect;

        if (rp->u.simple.tagop->u.gen2.u.custom.chipType == TMR_SR_GEN2_NXP_G2X_SILICON)
        {
          /* NXP_G2XL_ResetReadProtect can not be embedded.
           * Throw un supported exception to the user
           */
          return TMR_ERROR_UNSUPPORTED;
        }

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddNXPResetReadProtect(msg, &i, 0, rp->u.simple.tagop->u.gen2.u.custom.chipType, args->accessPassword, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_NXP_CHANGEEAS:
      {
        TMR_TagOp_GEN2_NXP_ChangeEAS *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.nxp.u.changeEAS;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddNXPChangeEAS(msg, &i, 0, rp->u.simple.tagop->u.gen2.u.custom.chipType, args->accessPassword, args->reset, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_NXP_EASALARM:
      {
        return TMR_ERROR_UNSUPPORTED;
      }
    case TMR_TAGOP_GEN2_NXP_CALIBRATE:
      {
        TMR_TagOp_GEN2_NXP_Calibrate *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.nxp.u.calibrate;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddNXPCalibrate(msg, &i, 0, rp->u.simple.tagop->u.gen2.u.custom.chipType, args->accessPassword, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_NXP_CHANGECONFIG:
      {
        TMR_TagOp_GEN2_NXP_ChangeConfig *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.nxp.u.changeConfig;

        if (rp->u.simple.tagop->u.gen2.u.custom.chipType == TMR_SR_GEN2_NXP_G2X_SILICON)
        {
          /* Change Config is not supported for G2xL silicon*/
          return TMR_ERROR_UNSUPPORTED;
        }
        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddNXPChangeConfig(msg, &i, 0, rp->u.simple.tagop->u.gen2.u.custom.chipType, 0, args->configWord, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IMPINJ_MONZA4_QTREADWRITE:
      {
        TMR_TagOp_GEN2_Impinj_Monza4_QTReadWrite *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.impinj.u.monza4.u.qtReadWrite;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->accessPassword, &lenbyte);

        TMR_SR_msgAddMonza4QTReadWrite(msg, &i, 0, 0, args->controlByte, args->payload, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_GETSENSOR:
      {
        TMR_TagOp_GEN2_IDS_SL900A_GetSensorValue *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.sensor;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aGetSensorValue(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level,
            args->sl900A.sensortype, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_GETMEASUREMENTSETUP:
      {
        TMR_TagOp_GEN2_IDS_SL900A_GetMeasurementSetup *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.measurementSetup;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aGetMeasurementSetup(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, NULL);
      }

    case TMR_TAGOP_GEN2_IDS_SL900A_GETLOGSTATE:
      {
        TMR_TagOp_GEN2_IDS_SL900A_GetLogState *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.getLog;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aGetLogState(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level,NULL);

        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_SETLOGMODE:
      {
        TMR_TagOp_GEN2_IDS_SL900A_SetLogMode *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.setLogMode;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aSetLogMode(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level,
            args->sl900A.dataLog, args->sl900A.rule, args->Ext1Enable, args->Ext2Enable, args->TempEnable,
            args->BattEnable, args->LogInterval, NULL);

        break;
      }

    case TMR_TAGOP_GEN2_IDS_SL900A_INITIALIZE:
      {
        TMR_TagOp_GEN2_IDS_SL900A_Initialize *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.initialize;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aInitialize(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level,
            args->delayTime.raw, args->applicationData.raw, NULL);

        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_ENDLOG:
      {
        TMR_TagOp_GEN2_IDS_SL900A_EndLog *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.endLog;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aEndLog(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, NULL);
        break;

      }
    case TMR_TAGOP_GEN2_IDS_SL900A_SETPASSWORD:
      {
        TMR_TagOp_GEN2_IDS_SL900A_SetPassword *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.setPassword;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aSetPassword(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level,
            args->NewPassword, args->NewPasswordLevel,  NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_STARTLOG:
      {
        TMR_TagOp_GEN2_IDS_SL900A_StartLog *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.startLog;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aStartLog(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, args->startTime, NULL);
        break;

      }
    case TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOSTATUS:
      {
        TMR_TagOp_GEN2_IDS_SL900A_AccessFifoStatus *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.accessFifoStatus;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->status.AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aAccessFifoStatus(msg, &i, 0, 0, args->status.CommandCode, args->status.Password, args->status.sl900A.level, args->status.operation, NULL);
        break;

      }
    case TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOREAD:
      {
        TMR_TagOp_GEN2_IDS_SL900A_AccessFifoRead *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.accessFifoRead;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->read.AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aAccessFifoRead(msg, &i, 0, 0, args->read.CommandCode, args->read.Password, args->read.sl900A.level, args->read.operation, args->length, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOWRITE:
      {
        TMR_TagOp_GEN2_IDS_SL900A_AccessFifoWrite *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.accessFifoWrite;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->write.AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aAccessFifoWrite(msg, &i, 0, 0, args->write.CommandCode, args->write.Password, args->write.sl900A.level, args->write.operation, args->payLoad, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_GETCALIBRATIONDATA:
      {
        TMR_TagOp_GEN2_IDS_SL900A_GetCalibrationData *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.calibrationData;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aGetCalibrationData(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_SETCALIBRATIONDATA:
      {
        TMR_TagOp_GEN2_IDS_SL900A_SetCalibrationData *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.setCalibration;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aSetCalibrationData(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, args->cal.raw, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_SETSFEPARAMETERS:
      {
        TMR_TagOp_GEN2_IDS_SL900A_SetSfeParameters *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.setSfeParameters;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aSetSfeParameters(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, args->sfe->raw, NULL);
        break;

      }
    case TMR_TAGOP_GEN2_IDS_SL900A_GETBATTERYLEVEL:
      {
        TMR_TagOp_GEN2_IDS_SL900A_GetBatteryLevel *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.batteryLevel;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aGetBatteryLevel(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, args->batteryType, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_IDS_SL900A_SETLOGLIMITS:
      {
        TMR_TagOp_GEN2_IDS_SL900A_SetLogLimits *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.ids.u.setLogLimit;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, args->AccessPassword, &lenbyte);

        TMR_SR_msgAddIdsSL900aSetLogLimit(msg, &i, 0, 0, args->CommandCode, args->Password, args->sl900A.level, args->limit.extremeLower,
            args->limit.lower, args->limit.upper, args->limit.extremeUpper, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_DENATRAN_IAV_ACTIVATESECUREMODE:
      {
        TMR_TagOp_GEN2_Denatran_IAV_Activate_Secure_Mode *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.secureMode;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomOp(msg, &i, 0, 0, args->mode, args->payload, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_DENATRAN_IAV_AUTHENTICATEOBU:
      {
        TMR_TagOp_GEN2_Denatran_IAV_Authenticate_OBU *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.authenticateOBU;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomOp(msg, &i, 0, 0, args->mode, args->payload, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_ACTIVATE_SINIAV_MODE:
      {
        TMR_TagOp_GEN2_Denatran_IAV_Activate_Siniav_Mode *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.activateSiniavMode;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomActivateSiniavMode(msg, &i, 0, 0, args->mode, args->payload, NULL, args->isTokenDesc, args->token);
        break;
      }
    case TMR_TAGOP_GEN2_OBU_AUTH_ID:
      {
        TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_ID *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthId;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomOp(msg, &i, 0, 0, args->mode, args->payload, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_AUTHENTICATE_OBU_FULL_PASS1:
      {
        TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass1 *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass1;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomOp(msg, &i, 0, 0, args->mode, args->payload, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_AUTHENTICATE_OBU_FULL_PASS2:
      {
        TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass2 *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass2;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomOp(msg, &i, 0, 0, args->mode, args->payload, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_OBU_READ_FROM_MEM_MAP:
      {
        TMR_TagOp_GEN2_Denatran_IAV_OBU_ReadFromMemMap *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.obuReadFromMemMap;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomReadFromMemMap(msg, &i, 0, 0, args->mode, args->payload, NULL, args->readPtr);
        break;
      }
    case TMR_TAGOP_GEN2_DENATRAN_IAV_READ_SEC:
      {
        TMR_TagOp_GEN2_Denatran_IAV_Read_Sec *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.readSec;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
              | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomReadSec(msg, &i, 0, 0, args->mode, args->payload, NULL, args->readPtr);
        break;
      }
 
    case TMR_TAGOP_GEN2_OBU_WRITE_TO_MEM_MAP:
      {
        TMR_TagOp_GEN2_Denatran_IAV_OBU_WriteToMemMap *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.obuWriteToMemMap;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
            | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomWriteToMemMap(msg, &i, 0, 0, args->mode, args->payload, NULL, args->writePtr, args->wordData, args->tagIdentification, args->dataBuf);
        break;
      }

    case TMR_TAGOP_GEN2_DENATRAN_IAV_WRITE_SEC:
      {
        TMR_TagOp_GEN2_Denatran_IAV_Write_Sec *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.writeSec;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
              | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomWriteSec(msg, &i, 0, 0, args->mode, args->payload, NULL, args->dataWords, args->dataBuf);
        break;
      }
 
    case TMR_TAGOP_GEN2_DENATRAN_IAV_GET_TOKEN_ID:
      {
        TMR_TagOp_GEN2_Denatran_IAV_Get_Token_Id *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.getTokenId;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
              | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomGetTokenId(msg, &i, 0, 0, args->mode, NULL);
        break;
      }
    case TMR_TAGOP_GEN2_DENATRAN_IAV_AUTHENTICATE_OBU_FULL_PASS:
      {
        TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass *args;
        args = &rp->u.simple.tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass;

        prepEmbReadTagMultiple(reader, msg, &i, (uint16_t)readTimeMs, (TMR_SR_SearchFlag)(TMR_SR_SEARCH_FLAG_CONFIGURED_LIST
              | TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND|TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT),
            rp->u.simple.filter, rp->u.simple.protocol, sr->gen2AccessPassword, &lenbyte);

        TMR_SR_msgAddIAVDenatranCustomOp(msg, &i, 0, 0, args->mode, args->payload, NULL);
        break;

      }
    case TMR_TAGOP_LIST:
      return TMR_ERROR_UNIMPLEMENTED; /* Module doesn't implement these */
    default:
      return TMR_ERROR_INVALID; /* Unknown tagop - internal error */
  }
  *j = i;
  *byte = lenbyte;
  return ret;

}

#endif /* TMR_ENABLE_SERIAL_READER */
