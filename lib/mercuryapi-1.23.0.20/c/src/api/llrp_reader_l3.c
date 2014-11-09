/**
 *  @file llrp_reader_l3.c
 *  @brief Mercury API - LLRP reader low level implementation
 *  @author Somu
 *  @date 05/25/2011
 */

/*
 * Copyright (c) 2011 ThingMagic, Inc.
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

/**
 * Define this to convert string representation of time
 * to time structure
 */ 
#define _XOPEN_SOURCE


#include "tm_reader.h"
#include "osdep.h"
#ifdef TMR_ENABLE_LLRP_READER

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include "llrp_reader_imp.h"
#include "tmr_utils.h"

#define BACKGROUND_RECEIVER_LOOP_PERIOD 100

uint8_t TMR_LLRP_gpiList[] = {3,4,6,7};
uint8_t TMR_LLRP_gpoList[] = {0,1,2,5};

static TMR_Status
TMR_LLRP_llrpToTmGpi(llrp_u16_t llrpNum, uint8_t* tmNum)
{
  int index = llrpNum - 1;
  int gpiCount = sizeof(TMR_LLRP_gpiList)/sizeof(TMR_LLRP_gpiList[0]);
  if ((index < 0) || (gpiCount <= index))
  {
    return TMR_ERROR_LLRP_UNDEFINED_VALUE;
  }
  *tmNum = TMR_LLRP_gpiList[llrpNum - 1];
  return TMR_SUCCESS;
}

static TMR_Status
TMR_LLRP_tmToLlrpGpo(uint8_t tmNum, llrp_u16_t *llrpNum)
{
  switch (tmNum)
  {
  case 0: *llrpNum = 1; break;
  case 1: *llrpNum = 2; break;
  case 2: *llrpNum = 3; break;
  case 5: *llrpNum = 4; break;
  default: return TMR_ERROR_INVALID;
  }
  return TMR_SUCCESS;
}

/**
 * Notify transport listener,
 * Called from SendMessage and ReceiveMessage
 *
 * @param reader The reader
 * @param pMsg Pointer to Message to send (of type LLRP_tSMessage * for llrp reader)
 * @param tx True if called from SendMessage, false if called from ReceiveMessage
 * @param timeout Timeout value.
 */
TMR_Status
TMR_LLRP_notifyTransportListener(TMR_Reader *reader, LLRP_tSMessage *pMsg, bool tx, int timeout)
{
  char buf[100*1024];

  if (LLRP_RC_OK != LLRP_toXMLString(&pMsg->elementHdr, buf, sizeof(buf)))
  {
    TMR__notifyTransportListeners(reader, tx, 0, (uint8_t *)buf, timeout);
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR; 
  }

  TMR__notifyTransportListeners(reader, tx, 0, (uint8_t *)buf, timeout);

  return TMR_SUCCESS;
}

/**
 * Send a message to the reader
 *
 * @param reader The reader
 * @param[in] pMsg Pointer to Message to send.
 * @param timeoutMs Timeout value.
 */
TMR_Status
TMR_LLRP_sendMessage(TMR_Reader *reader, LLRP_tSMessage *pMsg, int timeoutMs)
{
  LLRP_tSConnection *pConn = reader->u.llrpReader.pConn;
  timeoutMs += reader->u.llrpReader.transportTimeout;

  if (NULL == pConn)
  {
    return TMR_ERROR_LLRP_SENDIO_ERROR;
  }

  pMsg->MessageID = reader->u.llrpReader.msgId ++;

  if (NULL != reader->transportListeners)
  {
    TMR_LLRP_notifyTransportListener(reader, pMsg, true, timeoutMs);
  }
  /*
   * If LLRP_Conn_sendMessage() returns other than LLRP_RC_OK
   * then there was an error.
   */
  if (LLRP_RC_OK != LLRP_Conn_sendMessage(pConn, pMsg))
  {
    const LLRP_tSErrorDetails *pError = LLRP_Conn_getSendError(pConn);
    sprintf(reader->u.llrpReader.errMsg, "ERROR: %s sendMessage failed, %s",
                pMsg->elementHdr.pType->pName,
                pError->pWhatStr ? pError->pWhatStr : "no reason given");

    return TMR_ERROR_LLRP_SENDIO_ERROR;
  }

  return TMR_SUCCESS;
}

/**
 * Receive a response.
 *
 * @param reader The reader
 * @param[out] pMsg Message received.
 * @param timeoutMs Timeout value.
 */
TMR_Status
TMR_LLRP_receiveMessage(TMR_Reader *reader, LLRP_tSMessage **pMsg, int timeoutMs)
{
  LLRP_tSConnection *pConn = reader->u.llrpReader.pConn;
  timeoutMs += reader->u.llrpReader.transportTimeout;

  if (NULL == pConn)
  {
    return TMR_ERROR_LLRP_RECEIVEIO_ERROR;
  }

  /*
   * Receive the message subject to a time limit
   */
  *pMsg = LLRP_Conn_recvMessage(pConn, timeoutMs);
  /*
   * If LLRP_Conn_recvMessage() returns NULL then there was
   * an error.
   */
  if(NULL == *pMsg)
  {
    const LLRP_tSErrorDetails *pError = LLRP_Conn_getRecvError(pConn);
    
    sprintf(reader->u.llrpReader.errMsg,
            "ERROR: recvMessage failed, %s",
            pError->pWhatStr ? pError->pWhatStr : "no reason given");
    
    return TMR_ERROR_LLRP_RECEIVEIO_ERROR;
  }

  TMR_LLRP_notifyTransportListener(reader, *pMsg, false, timeoutMs);
  return TMR_SUCCESS;
}

/**
 * Send a message and receive a response with timeout.
 *
 * @param reader The reader
 * @param[in] pMsg Message to send
 * @param[out] pRsp Message received.
 * @param timeoutMs Timeout value.
 */
TMR_Status
TMR_LLRP_sendTimeout(TMR_Reader *reader, LLRP_tSMessage *pMsg, LLRP_tSMessage **pRsp, int timeoutMs)
{
  TMR_Status ret;

  if (false == reader->continuousReading)
  {
    /**
     * If not in continuous reading, then disable background receiver
     **/
    TMR_LLRP_setBackgroundReceiverState(reader, false);
  }

  ret = TMR_LLRP_sendMessage(reader, pMsg, timeoutMs);
  if (TMR_SUCCESS != ret)
  {
    goto out;
  }

repeat:
  ret = TMR_LLRP_receiveMessage(reader, pRsp, timeoutMs);
  if (TMR_SUCCESS != ret)
  {
    goto out;
  }

  if (pMsg->elementHdr.pType->pResponseType != (*pRsp)->elementHdr.pType)
  {
    /**
     * If the response message type and the expected response type
     * are not equal, (i.e., you can get any asynchronous response like events
     * which is not the correct response for the message sent)
     * Handle the other response, and go back to receive.
     * No need for error checking though.
     **/
    TMR_LLRP_processReceivedMessage(reader, *pRsp);
    goto repeat;
  }

out:
  if (false == reader->continuousReading)
  {
    /**
     * Re-enable background reader when not in continuous reading.
     **/
    TMR_LLRP_setBackgroundReceiverState(reader, true);
  }
  return ret;
}

/**
 * Send a message and receive a response.
 *
 * @param reader The reader
 * @param[in] pMsg Message to send
 * @param[out] pRsp Message received.
 */
TMR_Status
TMR_LLRP_send(TMR_Reader *reader, LLRP_tSMessage *pMsg, LLRP_tSMessage **pRsp)
{
  return TMR_LLRP_sendTimeout(reader, pMsg, pRsp,
                              reader->u.llrpReader.commandTimeout
                              + reader->u.llrpReader.transportTimeout);
}

/**
 * Free LLRP message
 *
 * @param pMsg Message to free
 */
void
TMR_LLRP_freeMessage(LLRP_tSMessage *pMsg)
{
  /* Free only if the message is not null */
  if (NULL != pMsg)
  {
  LLRP_Element_destruct(&pMsg->elementHdr);
    pMsg = NULL;
  }
}

/**
 * Check LLRP Message status
 * When a message is received, call this function
 * to check the LLRP message status
 *
 * @param pLLRPStatus Pointer to LLRP status parameter in LLRP message
 */
TMR_Status
TMR_LLRP_checkLLRPStatus(LLRP_tSLLRPStatus *pLLRPStatus)
{
  if (LLRP_StatusCode_M_Success != pLLRPStatus->eStatusCode)
  {
    return TMR_ERROR_LLRP;
  }
  else
  {
    return TMR_SUCCESS;
  }
}

/**
 * Command to get region id
 *
 * @param reader Reader pointer
 * @param[out] region Pointer to TMR_Region object to hold the region value
 */
TMR_Status
TMR_LLRP_cmdGetRegion(TMR_Reader *reader, TMR_Region *region)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMRegionConfig;
  LLRP_tSParameter                      *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/region/id" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMRegionConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMRegionConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., region configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMRegionConfig, LLRP_ThingMagicControlConfiguration_ThingMagicRegionConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMRegionConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMRegionConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract region id from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    *region = LLRP_ThingMagicRegionConfiguration_getRegionID((LLRP_tSThingMagicRegionConfiguration *) pCustParam);
  }
  else
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }
  
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}
/**
 * Command to get the read async offtime
 *
 * @param reader Reader pointer
 * @param[out] offtime of uin32_t to hold the off time
 **/ 
TMR_Status
TMR_LLRP_cmdGetTMAsyncOffTime(TMR_Reader *reader, uint32_t *offtime)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMAsyncOffTime;
  LLRP_tSParameter                      *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/asyncofftime" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMAsyncOffTime = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMAsyncOffTime)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., AsyncOffTime)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMAsyncOffTime, LLRP_ThingMagicControlConfiguration_ThingMagicAsyncOFFTime);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMAsyncOffTime->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMAsyncOffTime);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract async offtime from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    *offtime = LLRP_ThingMagicAsyncOFFTime_getAsyncOFFTime((LLRP_tSThingMagicAsyncOFFTime *) pCustParam);
  }
  else
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to Detect antenna connection status
 *
 * @param reader Reader pointer
 * @param count[out] Number of antennas detected 
 * @param[out] ports Pointer to TMR_LLRP_PortDetect object
 */
TMR_Status
TMR_LLRP_cmdAntennaDetect(TMR_Reader *reader, uint8_t *count, TMR_LLRP_PortDetect *ports)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSAntennaProperties              *pAntProps;
  uint8_t                               i;

  ret = TMR_SUCCESS;
  i = 0;
  /**
   * /reader/antenna/connectedPortList parameter is available
   * as an LLRP standard parameter GET_READER_CONFIG_RESPONSE.AntennaProperties.AntennaConnected.
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_AntennaProperties);
  LLRP_GET_READER_CONFIG_setAntennaID(pCmd, 0);

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract connected port list from it
   **/
  for (pAntProps = LLRP_GET_READER_CONFIG_RESPONSE_beginAntennaProperties(pRsp),
      i = 0;
      (pAntProps != NULL) && (i < *count);
      pAntProps = LLRP_GET_READER_CONFIG_RESPONSE_nextAntennaProperties(pAntProps),
      i ++)
  {
    ports[i].port       = pAntProps->AntennaID;
    ports[i].connected  = pAntProps->AntennaConnected;
    ports[i].gain       = pAntProps->AntennaGain;
  }
  *count = i;

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to get thingmagic Device Information Capabilities
 *
 * @param reader Reader pointer
 * @param version Pointer to TMR_String to hold the version hardware value 
 */
TMR_Status
TMR_LLRP_cmdGetTMDeviceInformationCapabilities(TMR_Reader *reader, TMR_String *version)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CAPABILITIES              *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CAPABILITIES_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlCapabilities  *pTMCaps;
  LLRP_tSParameter                            *pCustParam;
  llrp_utf8v_t                                 hardwareVersion;

  ret = TMR_SUCCESS;

  /**
   * Initialize GET_READER_CAPABILITIES message
   **/
  pCmd = LLRP_GET_READER_CAPABILITIES_construct();
  LLRP_GET_READER_CAPABILITIES_setRequestedData(pCmd, LLRP_GetReaderCapabilitiesRequestedData_General_Device_Capabilities);

  /**
   * /reader/version/hardware is a custom parameter.And is available as part of
   * ThingMagicDeviceControlCapabilities.ThingMagicControlCapabilities.DeviceInformationCapabilities
   **/
  pTMCaps = LLRP_ThingMagicDeviceControlCapabilities_construct();
  if (NULL == pTMCaps)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * set the requested data
   * and add to GET_READER_CAPABILITIES message
   **/
  LLRP_ThingMagicDeviceControlCapabilities_setRequestedData(pTMCaps, LLRP_ThingMagicControlCapabilities_DeviceInformationCapabilities);
  if (LLRP_RC_OK != LLRP_GET_READER_CAPABILITIES_addCustom(pCmd, &pTMCaps->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMCaps);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }
  pCmdMsg       = &pCmd->hdr;

  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CAPABILITIES_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }
  /**
   * Response is success
   * Extract hardware version from it
   **/
  pCustParam = LLRP_GET_READER_CAPABILITIES_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    hardwareVersion = ((LLRP_tSDeviceInformationCapabilities *)pCustParam)->HardwareVersion;
    TMR_stringCopy(version, (char *)hardwareVersion.pValue, (int)hardwareVersion.nValue);
  }
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

TMR_GEN2_Tari
TMR_convertTari(uint16_t tari)
{
  switch(tari) {
  case 6250:
    return TMR_GEN2_TARI_6_25US;
    break;
  case 12500:
    return TMR_GEN2_TARI_12_5US;
    break;
  case 25000:
    return TMR_GEN2_TARI_25US;
    break;
  default:
    return TMR_GEN2_TARI_25US;
    break;
  }
}
/**
 * Command to get Reader capabilities
 *
 * @param reader Reader pointer
 * @param capabilities Pointer to TMR_LLRP_ReaderCapabilities
 */
TMR_Status
TMR_LLRP_cmdGetReaderCapabilities(TMR_Reader *reader, TMR_LLRP_ReaderCapabilities *capabilities)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CAPABILITIES          *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSGET_READER_CAPABILITIES_RESPONSE *pRsp;
  TMR_GEN2_Tari minTari, maxTari;

  ret = TMR_SUCCESS;

  /**
   * Retreive all reader capabilities
   * Initialize GET_READER_CAPABILITIES message 
   **/
  pCmd = LLRP_GET_READER_CAPABILITIES_construct();
  LLRP_GET_READER_CAPABILITIES_setRequestedData(pCmd, LLRP_GetReaderCapabilitiesRequestedData_All);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CAPABILITIES_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract all required information
   **/

  /**
   * We can also cache, model and software version
   * This eliminates explicit functions to get model and software versions.
   * Extract General device capabilities
   **/
  {
    /**
     * Extract model
     **/
    LLRP_tSGeneralDeviceCapabilities        *pReaderCap;
    pReaderCap = LLRP_GET_READER_CAPABILITIES_RESPONSE_getGeneralDeviceCapabilities(pRsp);

    if (TM_MANUFACTURER_ID == pReaderCap->DeviceManufacturerName)
    {
      capabilities->model = pReaderCap->ModelName;
    }

    /**
     * Extract software version
     **/
    memcpy(capabilities->softwareVersion, pReaderCap->ReaderFirmwareVersion.pValue, 
                                                  pReaderCap->ReaderFirmwareVersion.nValue );
    capabilities->softwareVersion[pReaderCap->ReaderFirmwareVersion.nValue] = '\0';
  }

  /**
   * Extract Regulatory capabilities
   **/
  {
    LLRP_tSRegulatoryCapabilities *pRegCaps;
    pRegCaps = LLRP_GET_READER_CAPABILITIES_RESPONSE_getRegulatoryCapabilities(pRsp);

    {
      /**
       * Extract power leve table from response.
       **/
      LLRP_tSTransmitPowerLevelTableEntry     *pTableEntry;

      /*Initialize cached powerTable */
      TMR_uint16List *table = &capabilities->powerTable;
      table->max = 255;
      table->len = 0;
      table->list = capabilities->powerList;

      /* Before extracting the power level initialize the powerList table to zero*/
      memset(table->list, 0, table->max*(sizeof(uint16_t)));


      for (pTableEntry = LLRP_UHFBandCapabilities_beginTransmitPowerLevelTableEntry(
                                                              pRegCaps->pUHFBandCapabilities);
          (pTableEntry != NULL);
          pTableEntry = LLRP_UHFBandCapabilities_nextTransmitPowerLevelTableEntry(pTableEntry))
      {
        table->list[pTableEntry->Index] = pTableEntry->TransmitPowerValue;
        table->len ++;
      }
    }

    {
      /**
       * Extract Frequency information from response
       **/
      LLRP_tSFrequencyHopTable  *pFreqHopTable;
      llrp_u16_t count;

      /*Initialize cached frequency table */
      TMR_uint32List *table = &capabilities->freqTable;
      table->max = 64;
      table->len = 0;
      table->list = capabilities->freqList;

      /* Before extracting the frequency  initialize the freqList table to zero*/
      memset(table->list, 0, table->max*(sizeof(uint32_t)));

      pFreqHopTable = LLRP_FrequencyInformation_beginFrequencyHopTable(
                                pRegCaps->pUHFBandCapabilities->pFrequencyInformation);

      for (count = 0; count < pFreqHopTable->Frequency.nValue; count ++)
      {
        table->list[count] = pFreqHopTable->Frequency.pValue[count];
        table->len ++;
      }
    }
    {
      /**
       * Iterate through listAirProtocolUHFRFModeTable
       **/
      LLRP_tSParameter *pRFModeTable;

      for (pRFModeTable = LLRP_UHFBandCapabilities_beginAirProtocolUHFRFModeTable(pRegCaps->pUHFBandCapabilities);
          (pRFModeTable != NULL);
          pRFModeTable = LLRP_UHFBandCapabilities_nextAirProtocolUHFRFModeTable(pRFModeTable))
      {
        /* Check for Mode table protocol */
        if (&LLRP_tdC1G2UHFRFModeTable == pRFModeTable->elementHdr.pType)
        {
          /**
           * It is a Gen2 protocol RFMode table.
           * Currently we are using only BLF and tagEncoding values
           **/
          LLRP_tSC1G2UHFRFModeTableEntry *pModeEntry;

          /* Iterate through list of entries in table */
          for (pModeEntry = LLRP_C1G2UHFRFModeTable_beginC1G2UHFRFModeTableEntry(
                                                (LLRP_tSC1G2UHFRFModeTable *)pRFModeTable);
              (pModeEntry != NULL);
              pModeEntry = LLRP_C1G2UHFRFModeTable_nextC1G2UHFRFModeTableEntry(pModeEntry))
          {
            TMR_GEN2_LinkFrequency blf;
            llrp_u32_t index        = pModeEntry->ModeIdentifier;
            llrp_u32_t bdr          = pModeEntry->BDRValue;
            llrp_u32_t minTariValue = pModeEntry->MinTariValue;
            llrp_u32_t maxTariValue = pModeEntry->MaxTariValue;

            switch (bdr)
            {
              case 250000:
                blf = TMR_GEN2_LINKFREQUENCY_250KHZ;
                break;

              case 640000:
                blf = TMR_GEN2_LINKFREQUENCY_640KHZ;
                break;

              case 320000:
                blf = TMR_GEN2_LINKFREQUENCY_320KHZ;
                break;

              default:
                blf = TMR_GEN2_LINKFREQUENCY_250KHZ;
                break;
            }
            minTari = TMR_convertTari(minTariValue);
            maxTari = TMR_convertTari(maxTariValue);

            /** Cache blf value */
            capabilities->u.gen2Modes[index].blf = blf;
            /** Cache m value */
            capabilities->u.gen2Modes[index].m = pModeEntry->eMValue;
            /** Cache minTari value */
            capabilities->u.gen2Modes[index].minTari = minTari;
            /** Cache maxTari value */
            capabilities->u.gen2Modes[index].maxTari = maxTari;
          }
        }
        else
        {
          /**
           * TODO: Need to implement for other protocols
           **/
          TMR_LLRP_freeMessage(pRspMsg);
          return TMR_ERROR_UNIMPLEMENTED_FEATURE;
        }
      }
    } /* End of iterating through listAirProtocolUHFRFModeTable */
  } /* End of extracting Regulatory capabilities */

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to set GPO state
 *
 * @param reader Reader pointer
 * @param pins[in] Pointer to TMR_GpioPin array
 */
TMR_Status
TMR_LLRP_cmdSetGPOState(TMR_Reader *reader, uint8_t count,
                        const TMR_GpioPin state[])
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  uint8_t i;

  ret = TMR_SUCCESS;

  /**
   * GPO state can be set  LLRP standard parameter
   * SET_READER_CONFIG.GPOWriteData
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Set GPO state as per the list supplied */
  for (i=0; i<count; i++)
  {
    uint8_t id;
    bool high;
    llrp_u16_t llrpId;
    LLRP_tSGPOWriteData *pParam;

    id = state[i].id;
    high = state[i].high;

    /* Construct LLRP parameter */
    pParam = LLRP_GPOWriteData_construct();
    ret = TMR_LLRP_tmToLlrpGpo(id, &llrpId);
    if (TMR_SUCCESS != ret) { return ret; }
    LLRP_GPOWriteData_setGPOPortNumber(pParam, llrpId);
    LLRP_GPOWriteData_setGPOData(pParam, high);

    /* Add param */
    LLRP_SET_READER_CONFIG_addGPOWriteData(pCmd, pParam);
  }

  /**
   * Now the message is framed completely and send the message
   **/
  pCmdMsg = &pCmd->hdr;

  if (reader->continuousReading)
  {
    ret = TMR_LLRP_sendMessage(reader, pCmdMsg, reader->u.llrpReader.transportTimeout);
    if (TMR_SUCCESS != ret)
    {
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return ret;
    }

    while (false == reader->u.llrpReader.isResponsePending)
    {
      tmr_sleep(10);
    }

    if (NULL != reader->u.llrpReader.unhandledAsyncResponse.lMsg)
    {
      if (pCmdMsg->elementHdr.pType->pResponseType == reader->u.llrpReader.unhandledAsyncResponse.lMsg->elementHdr.pType)
      {
        TMR_LLRP_freeMessage((LLRP_tSMessage *)reader->u.llrpReader.unhandledAsyncResponse.lMsg);
        reader->u.llrpReader.isResponsePending = false;
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);

        return TMR_SUCCESS;
      }
    }
    else
    {
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return TMR_ERROR_LLRP;
    }
  }
  else
  {
    ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
    /**
     * Done with the command, free the message
     * and check for message status
     **/ 
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }

    /**
     * Check response message status
     **/
    pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
    if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
    {
      TMR_LLRP_freeMessage(pRspMsg);
      return TMR_ERROR_LLRP; 
    }

    /**
     * Done with the response, free the message
     **/
    TMR_LLRP_freeMessage(pRspMsg);
  }

  return ret;
}


/**
 * Command to set Read Transmit power list
 *
 * @param reader Reader pointer
 * @param pPortValueList[in] Pointer to TMR_PortValueList
 */
TMR_Status
TMR_LLRP_cmdSetReadTransmitPowerList(TMR_Reader *reader, TMR_PortValueList *pPortValueList)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  uint8_t                               i;

  ret = TMR_SUCCESS;
  i = 0;
  /**
   * Port Read Power list can be set
   * through  LLRP standard parameter SET_READER_CONFIG.AntennaConfiguration.RFTransmitter.TransmitPower
   * Initialize SET_READER_CONFIG message 
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Set antenna configuration as per the list supplied */
  for (i = 0; i < pPortValueList->len; i ++)
  {
    LLRP_tSAntennaConfiguration   *pAntConfig;
    LLRP_tSRFTransmitter          *pRfTransmitter;
    uint16_t                       index, power = 0;

    /* Construct RFTransmitter */
    pRfTransmitter = LLRP_RFTransmitter_construct();
    if (NULL == pRfTransmitter)
    {
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return TMR_ERROR_LLRP;
    }

    /* Get the index of corresponding power from powerTable  */
    for (index = 1; index <= reader->u.llrpReader.capabilities.powerTable.len; 
                                                                  index ++)
    {
      power = reader->u.llrpReader.capabilities.powerTable.list[index];
      if (pPortValueList->list[i].value == power)
      {
        break;
      }
      else
      {
        /*
         * we are cacheing the power level in 0.2db resolution.
         * if the value provided by the user is within the range
         * but not in the powertable always down grade the value
         * to the nearest available lower power value
         */
        if((pPortValueList->list[i].value > power)&&
            (pPortValueList->list[i].value < reader->u.llrpReader.capabilities.powerTable.list[index+1]))
        {
          power = reader->u.llrpReader.capabilities.powerTable.list[index];
          break;
        }
      }
    }

    if (index > reader->u.llrpReader.capabilities.powerTable.len)
    {
      /* power value specified is unknown or out of range */
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pRfTransmitter);
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return TMR_ERROR_ILLEGAL_VALUE;
    }

    if ((TMR_LLRP_MODEL_ASTRA_EX == reader->u.llrpReader.capabilities.model)
        && (1 == pPortValueList->list[i].port)
        && (TMR_REGION_NA == reader->u.llrpReader.regionId))
    {
      if (3000 < power)
      {
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pRfTransmitter);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
        return TMR_ERROR_MSG_POWER_TOO_HIGH;
      }
    }

    LLRP_RFTransmitter_setTransmitPower(pRfTransmitter, index);

    /* Construct AntennaConfiguration */
    pAntConfig = LLRP_AntennaConfiguration_construct();
    LLRP_AntennaConfiguration_setAntennaID(pAntConfig, pPortValueList->list[i].port);
    LLRP_AntennaConfiguration_setRFTransmitter(pAntConfig, pRfTransmitter);

    /* Add AntennaConfiguration */
    LLRP_SET_READER_CONFIG_addAntennaConfiguration(pCmd, pAntConfig);
  }

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to get Version Serial
 *
 * @param reader Reader pointer
 * @param version  Pointer to TMR_String to hold the version serial value
 */
TMR_Status
TMR_LLRP_cmdGetVersionSerial(TMR_Reader *reader, TMR_String *version)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;

  ret = TMR_SUCCESS;

  /**
   * Version Serial  can be retreived
   * through  LLRP standard parameter GET_READER_CONFIG_IDENTIFICATION_READERID
   * Initialize GET_READER_CONFIG message 
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }
  /**
   * Response is success, extract reader ID from it
   **/
  TMR_bytesToHex(pRsp->pIdentification->ReaderID.pValue, pRsp->pIdentification->ReaderID.nValue, version->value);
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

/**
 * Command to get GPI state
 *
 * @param reader Reader pointer
 * @param count Pointer to uint8_t to hold number of elements written into state
 * @param state Pointer to TMR_GpioPin array to hold the GPI state value
   */
TMR_Status
TMR_LLRP_cmdGetGPIState(TMR_Reader *reader, uint8_t *count, TMR_GpioPin state[])
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;

  *count = 0;
  ret = TMR_SUCCESS;

  /**
   * GPI state can be retreived
   * through  LLRP standard parameter GET_READER_CONFIG_RESPONSE GPIPortCurrentState
   * Initialize GET_READER_CONFIG message 
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_GPIPortCurrentState);

  /**
   * Now the message is framed completely and send the message
   **/
  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }
  /**
   * Response is success, extract GPI state from it
   **/
  {
    LLRP_tSGPIPortCurrentState* list = pRsp->listGPIPortCurrentState;
    while (NULL != list)
    {
      TMR_GpioPin* pin = &state[*count];
      ret = TMR_LLRP_llrpToTmGpi(list->GPIPortNum, &pin->id);
      if (TMR_SUCCESS != ret) { return ret; }
      pin->high = (LLRP_GPIPortState_High == list->eState);
      pin->output = false;
      (*count)++;
      list = (LLRP_tSGPIPortCurrentState*)list->hdr.pNextSubParameter;
    }
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

/**
 * Command to get Read transmit power list
 *
 * @param reader Reader pointer
 * @param pPortValueList[out] Pointer to TMR_PortValueList to hold the power value list
 */
TMR_Status 
TMR_LLRP_cmdGetReadTransmitPowerList(TMR_Reader *reader, TMR_PortValueList *pPortValueList)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSAntennaConfiguration           *pAntConfig;
  uint8_t                               i;

  ret = TMR_SUCCESS;
  i = 0;
  /**
   * Port Power list can be retreived
   * through  LLRP standard parameter GET_READER_CONFIG_RESPONSE.AntennaConfiguration.RFTransmitter.TransmitPower
   * Initialize GET_READER_CONFIG message 
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_AntennaConfiguration);

  /* Get antenna configuration for all antennas*/
  LLRP_GET_READER_CONFIG_setAntennaID(pCmd, 0);

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract read power from it
   **/
  pPortValueList->len = 0;
  for (pAntConfig = LLRP_GET_READER_CONFIG_RESPONSE_beginAntennaConfiguration(pRsp),
          i = 0;
          (pAntConfig != NULL);
          pAntConfig = LLRP_GET_READER_CONFIG_RESPONSE_nextAntennaConfiguration(pAntConfig),
          i ++)
  {
    uint16_t index;

    if (i > pPortValueList->max)
    {
      break;
    }
    pPortValueList->list[i].port  = pAntConfig->AntennaID;
    index = pAntConfig->pRFTransmitter->TransmitPower;
    if (NULL != reader->u.llrpReader.capabilities.powerTable.list)
    {
    pPortValueList->list[i].value = (int32_t)reader->u.llrpReader.capabilities.powerTable.list[index];
    pPortValueList->len ++;
  }
  }
  
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to set write transmit power list
 *
 * @param reader Reader pointer
 * @param pPortValueList[in] Pointer to TMR_PortValueList
 */
TMR_Status
TMR_LLRP_cmdSetWriteTransmitPowerList(TMR_Reader *reader, TMR_PortValueList *pPortValueList)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  uint8_t                               i;

  ret = TMR_SUCCESS;
  i = 0;
  /**
   * Port Write Power list is custom parameter and can be set
   * through  parameter SET_READER_CONFIG.AntennaConfiguration.RFTransmitter.TransmitPower
   * Initialize SET_READER_CONFIG message 
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Set antenna configuration as per the list supplied */
  for (i = 0; i < pPortValueList->len; i ++)
  {
    LLRP_tSThingMagicAntennaConfiguration   *pAntConfig;
    LLRP_tSWriteTransmitPower               *pWriteTransmitPower;
    uint16_t                                index, power = 0;

    /* Construct AntennaConfiguration */
    pAntConfig = LLRP_ThingMagicAntennaConfiguration_construct();
    if (NULL == pAntConfig)
    {
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return TMR_ERROR_LLRP;
    }
    LLRP_ThingMagicAntennaConfiguration_setAntennaID(pAntConfig, pPortValueList->list[i].port);
    pWriteTransmitPower = LLRP_WriteTransmitPower_construct();
    if (NULL == pWriteTransmitPower)
    {
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pAntConfig);
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return TMR_ERROR_LLRP;
    }
 
    /* Get the index of corresponding power from powerTable  */
    for (index = 1; index <= reader->u.llrpReader.capabilities.powerTable.len; 
                                                                  index ++)
    {
      power = reader->u.llrpReader.capabilities.powerTable.list[index];
      if (pPortValueList->list[i].value == power)
      {
        break;
      }
      else
      {
        /*
         * we are cacheing the power level in 0.2db resolution.
         * if the value provided by the user is within the range
         * but not in the powertable always down grade the value
         * to the nearest available lower power value
         */
        if((pPortValueList->list[i].value > power)&&
            (pPortValueList->list[i].value < reader->u.llrpReader.capabilities.powerTable.list[index+1]))
        {
          power = reader->u.llrpReader.capabilities.powerTable.list[index];
          break;
        }
      }
    }

    if (index > reader->u.llrpReader.capabilities.powerTable.len)
    {
      /* power value specified is unknown or out of range */
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pWriteTransmitPower);
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pAntConfig);
      TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
      return TMR_ERROR_ILLEGAL_VALUE;
    }

    if ((TMR_LLRP_MODEL_ASTRA_EX == reader->u.llrpReader.capabilities.model)
        && (1 == pPortValueList->list[i].port)
        && (TMR_REGION_NA == reader->u.llrpReader.regionId))
    {
      if (3000 < power)
      {
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pWriteTransmitPower);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pAntConfig);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
        return TMR_ERROR_MSG_POWER_TOO_HIGH;
      }
    }

    LLRP_WriteTransmitPower_setWriteTransmitPower(pWriteTransmitPower, index);
    LLRP_ThingMagicAntennaConfiguration_setWriteTransmitPower(pAntConfig, pWriteTransmitPower);

    /* Add AntennaConfiguration as a custom parameter*/
    LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pAntConfig);
  }

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to get write transmit power list
 *
 * @param reader Reader pointer
 * @param pPortValueList[out] Pointer to TMR_PortValueList to hold the power list
 */
TMR_Status
TMR_LLRP_cmdGetWriteTransmitPowerList(TMR_Reader *reader, TMR_PortValueList *pPortValueList)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG                      *pCmd;
  LLRP_tSMessage                                *pCmdMsg;
  LLRP_tSMessage                                *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE             *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration   *pTMConfig;
  LLRP_tSWriteTransmitPower                     *pWriteTransmitPower;
  LLRP_tSParameter                              *pCustParam;
  uint8_t                                       i;

  ret = TMR_SUCCESS;
  i = 0;

  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/radio/portWritePowerList" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.ThingMagicAntennaConfiguration
   * Initialize the custom parameter
   **/
  pTMConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., Antenna configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMConfig, LLRP_ThingMagicControlConfiguration_ThingMagicAntennaConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract port write power list from response
   **/
  pPortValueList->len = 0;
  for (pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp),
          i = 0;
          (pCustParam != NULL);
          pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_nextCustom(pCustParam),
          i ++)
  {
    uint16_t index;

    if (i > pPortValueList->max)
    {
      break;
    }
    pPortValueList->list[i].port
      = ((LLRP_tSThingMagicAntennaConfiguration *) pCustParam)->AntennaID;
    pWriteTransmitPower =
      LLRP_ThingMagicAntennaConfiguration_getWriteTransmitPower((LLRP_tSThingMagicAntennaConfiguration *) pCustParam);
    index = LLRP_WriteTransmitPower_getWriteTransmitPower(pWriteTransmitPower);
    if (NULL != reader->u.llrpReader.capabilities.powerTable.list)
    {
    pPortValueList->list[i].value = (int32_t)reader->u.llrpReader.capabilities.powerTable.list[index];
    pPortValueList->len ++;
  }
  }
 
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to delete all ROSpecs on LLRP Reader
 *
 * @param reader Reader pointer
 */
TMR_Status
TMR_LLRP_cmdDeleteAllROSpecs(TMR_Reader *reader, bool receiveResponse)
{
  TMR_Status ret;
  LLRP_tSDELETE_ROSPEC          *pCmd;
  LLRP_tSMessage                *pCmdMsg;
  LLRP_tSMessage                *pRspMsg;
  LLRP_tSDELETE_ROSPEC_RESPONSE *pRsp;

  ret = TMR_SUCCESS;

  /**
   * Create delete rospec message
   **/
  pCmd = LLRP_DELETE_ROSPEC_construct();
  LLRP_DELETE_ROSPEC_setROSpecID(pCmd, 0);        /* All */

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  if (false == receiveResponse)
  {
    ret = TMR_LLRP_sendMessage(reader, pCmdMsg, reader->u.llrpReader.transportTimeout);
    /*
     * done with the command, free it
     */
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  }
  else
  {
    ret = TMR_LLRP_sendTimeout(reader, pCmdMsg, &pRspMsg,
                               TMR_LLRP_STOP_TIMEOUT
                               + reader->u.llrpReader.commandTimeout
                               + reader->u.llrpReader.transportTimeout);
    /**
     * Done with the command, free the message
     * and check for message status
     **/ 
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSDELETE_ROSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, Done with the response message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  }

  return ret;
}

/**
 * Convert API specific TMR_TagFilter into
 * LLRP specific filter element.
 **/
static TMR_Status
TMR_LLRP_prepareTagFilter(LLRP_tSInventoryParameterSpec *ipSpec, 
                          TMR_TagProtocol protocol, const TMR_TagFilter *filter)
{
  TMR_Status ret;
  LLRP_tSAntennaConfiguration   *pAntConfig;

  ret = TMR_SUCCESS;

  /* Initialize antenna configuration */
  pAntConfig     = LLRP_AntennaConfiguration_construct();

  /* Add C1G2Filter */
  if (TMR_TAG_PROTOCOL_GEN2 == protocol)
  { 
    LLRP_tSC1G2Filter             *pFilter;
    LLRP_tSC1G2TagInventoryMask   *pMask;
    LLRP_tSC1G2InventoryCommand   *pInvCommand;
    LLRP_tSC1G2TagInventoryStateUnawareFilterAction
                                  *pUnawareAction;

    /* Initialize */
    pFilter        = LLRP_C1G2Filter_construct();
    pMask          = LLRP_C1G2TagInventoryMask_construct();
    pInvCommand    = LLRP_C1G2InventoryCommand_construct();
    pUnawareAction = LLRP_C1G2TagInventoryStateUnawareFilterAction_construct();

    /* Set antenna id to antConfig (all antennas) */
    pAntConfig->AntennaID = 0;

    /* Set TagInventoryStateAwareAction to false */
    pInvCommand->TagInventoryStateAware = false;

    /* Set TagInventory StateUnaware Action */
    LLRP_C1G2TagInventoryStateUnawareFilterAction_setAction(pUnawareAction,
                                LLRP_C1G2StateUnawareAction_Select_Unselect);

    /**
     * If Gen2 protocol select filter
     **/
    if (TMR_FILTER_TYPE_GEN2_SELECT == filter->type)
    {
      const TMR_GEN2_Select *fp;
      llrp_u1v_t  tmpMask;
      fp = &filter->u.gen2Select;

      /**
       * Initialize Mask 
       **/
      /* Set Memory bank */
      LLRP_C1G2TagInventoryMask_setMB(pMask, fp->bank);

      /* Set tag mask */
      tmpMask = LLRP_u1v_construct(fp->maskBitLength);
      memcpy(tmpMask.pValue, fp->mask, fp->maskBitLength / 8);
      LLRP_C1G2TagInventoryMask_setTagMask(pMask, tmpMask);

      /* Set bit Pointer */
      LLRP_C1G2TagInventoryMask_setPointer(pMask, fp->bitPointer);

      if(fp->invert)
      {
        /* If invert is set true, Unselect matched and select unmatched */
        LLRP_C1G2TagInventoryStateUnawareFilterAction_setAction(pUnawareAction, 
                                LLRP_C1G2StateUnawareAction_Unselect_Select);
      }
    }
    /**
     * If Gen2 tag data filter
     **/
    else if (TMR_FILTER_TYPE_TAG_DATA == filter->type)
    {
      const TMR_TagData *fp;
      llrp_u1v_t  tmpMask;
      fp = &filter->u.tagData;

      /**
       * Initialize Mask 
       **/
      /* Set Memory bank to EPC always, in case of TagData */
      LLRP_C1G2TagInventoryMask_setMB(pMask, TMR_GEN2_BANK_EPC);

      /* Set tag mask */
      tmpMask = LLRP_u1v_construct(fp->epcByteCount * 8);
      memcpy(tmpMask.pValue, fp->epc, fp->epcByteCount);
      LLRP_C1G2TagInventoryMask_setTagMask(pMask, tmpMask);

      /**
       * Set bit Pointer
       * (EPC always starts from bit 32)
       **/
      LLRP_C1G2TagInventoryMask_setPointer(pMask, 32);
    }
    /**
     * Else, Unknown filter type. Return error
     **/
    else
    {
      return TMR_ERROR_INVALID;
    }

    /* Set mask to filter */
    LLRP_C1G2Filter_setC1G2TagInventoryMask(pFilter, pMask);

    /* Set InventoryStateUnawareFilter Action to filter */
    LLRP_C1G2Filter_setC1G2TagInventoryStateUnawareFilterAction(pFilter, pUnawareAction);

    /* Add filter to InventoryCommand */
    LLRP_C1G2InventoryCommand_addC1G2Filter(pInvCommand, pFilter);

    /* Add InvCommand to antenna configuration */
    LLRP_AntennaConfiguration_addAirProtocolInventoryCommandSettings(pAntConfig,
                                            (LLRP_tSParameter *)pInvCommand);
  }
  else
  {
    if (TMR_TAG_PROTOCOL_ISO180006B == protocol)
    {

#ifdef TMR_ENABLE_ISO180006B

      LLRP_tSThingMagicISO180006BInventoryCommand   *pTMISOInventory;
      LLRP_tSThingMagicISO180006BTagPattern         *pTMISOTagPattern;

      /* initialize  the TM inventory commmand */
      pTMISOInventory = LLRP_ThingMagicISO180006BInventoryCommand_construct();

      /* initialize  the  TM tagpattern */
      pTMISOTagPattern = LLRP_ThingMagicISO180006BTagPattern_construct();

    /**
       * If ISO180006B select filter
     **/
      if (TMR_FILTER_TYPE_ISO180006B_SELECT == filter->type)
      {
        const TMR_ISO180006B_Select *fp;
        llrp_u2_t   selectOp;
        llrp_u8v_t  tmpMask;

        fp = &filter->u.iso180006bSelect;

        /* Initialize the filter type */
        LLRP_ThingMagicISO180006BTagPattern_setFilterType(pTMISOTagPattern,
            LLRP_ThingMagicISO180006BFilterType_ISO180006BSelect);

        /* Initialize the select tag op */
        selectOp = fp->op;
        LLRP_ThingMagicISO180006BTagPattern_setSelectOp(pTMISOTagPattern, selectOp);

        /* set address of tag memory */
        LLRP_ThingMagicISO180006BTagPattern_setAddress(pTMISOTagPattern, fp->address);

        /* set the tag mask */
        LLRP_ThingMagicISO180006BTagPattern_setMask(pTMISOTagPattern, fp->mask);

        /* set the data to compare */
        /* In case of iso the data length must be eight */ 
        tmpMask = LLRP_u8v_construct((llrp_u16_t)(sizeof(fp->data))/(sizeof(fp->data[0])));
        memcpy(tmpMask.pValue, fp->data, tmpMask.nValue);
        LLRP_ThingMagicISO180006BTagPattern_setTagData(pTMISOTagPattern, tmpMask);

        /* set the selct action */
        LLRP_ThingMagicISO180006BTagPattern_setInvert(pTMISOTagPattern, (llrp_u1_t)fp->invert);

        /* Add TagPattern to the TM ISOInventory */
        LLRP_ThingMagicISO180006BInventoryCommand_setThingMagicISO180006BTagPattern(pTMISOInventory, pTMISOTagPattern);

        /* Add InvCommand to antenna configuration */
        LLRP_AntennaConfiguration_addAirProtocolInventoryCommandSettings(pAntConfig,
            (LLRP_tSParameter *)pTMISOInventory);

      }
      /**
       * If Iso tag filter
       **/
      else if(TMR_FILTER_TYPE_TAG_DATA == filter->type)
      {
        const TMR_TagData *fp;
        llrp_u8v_t  tmpMask;

        /* Initialize the filter type */
        LLRP_ThingMagicISO180006BTagPattern_setFilterType(pTMISOTagPattern,
            LLRP_ThingMagicISO180006BFilterType_ISO180006BTagData);

        fp = &filter->u.tagData;
        /* Initialize the select tag op */
        /* Set the Select op as TMR_ISO180006B_SELECT_OP_EQUALS 
         * in case of TagData
         */
        LLRP_ThingMagicISO180006BTagPattern_setSelectOp(pTMISOTagPattern, (llrp_u2_t) TMR_ISO180006B_SELECT_OP_EQUALS);
        /* Set the tag memory address to Zero , in case of TagData */
        LLRP_ThingMagicISO180006BTagPattern_setAddress(pTMISOTagPattern, (llrp_u8_t)0);
        /* Set the mask to 0xff, in case TagData */
        LLRP_ThingMagicISO180006BTagPattern_setMask(pTMISOTagPattern, (llrp_u8_t)0xFF);

        /* set the data to compare */
        /* In case of iso the data length must be eight */
        tmpMask = LLRP_u8v_construct((llrp_u16_t)fp->epcByteCount);
        memcpy(tmpMask.pValue, fp->epc, tmpMask.nValue);
        LLRP_ThingMagicISO180006BTagPattern_setTagData(pTMISOTagPattern, tmpMask);

        /* Set the select action false, in case of TagData */
        LLRP_ThingMagicISO180006BTagPattern_setInvert(pTMISOTagPattern, (llrp_u1_t)false);

        /* Add TagPattern to the TM ISOInventory */
        LLRP_ThingMagicISO180006BInventoryCommand_setThingMagicISO180006BTagPattern(pTMISOInventory, pTMISOTagPattern);

        /* Add InvCommand to antenna configuration */
        LLRP_AntennaConfiguration_addAirProtocolInventoryCommandSettings(pAntConfig,
            (LLRP_tSParameter *)pTMISOInventory);

      }
      else
      {
    return TMR_ERROR_INVALID;
  }
#endif /* TMR_ENABLE_ISO180006B */
    }
    else
    {
      /* Unsuppoerd Protocol */
      return TMR_ERROR_INVALID;
    }
  }

  /* Finally, Add antenna configuration to InventoryParameterSpec */
  LLRP_InventoryParameterSpec_addAntennaConfiguration(ipSpec, pAntConfig);
  return ret;
}

/** 
 * Add FastSearch option
 * to the read
 **/
TMR_Status
TMR_LLRP_addFastSearch(LLRP_tSInventoryParameterSpec *ipSpec, bool fastSearch)
{
  TMR_Status ret;
  LLRP_tSThingMagicFastSearchMode *pFastSearch;
  LLRP_tSC1G2InventoryCommand     *pInvCommand;
  LLRP_tSAntennaConfiguration     *pAntConfig;

  ret = TMR_SUCCESS;

  /* Initialize antenna configuration */
  pAntConfig     = LLRP_AntennaConfiguration_construct();
  pFastSearch = LLRP_ThingMagicFastSearchMode_construct();
  pInvCommand    = LLRP_C1G2InventoryCommand_construct();

  LLRP_ThingMagicFastSearchMode_setThingMagicFastSearch(pFastSearch,
      (LLRP_tEThingMagicFastSearchValue)fastSearch);

  /* Add FastSearch to InventoryCommand */
  LLRP_C1G2InventoryCommand_addCustom (pInvCommand,
      (LLRP_tSParameter *)pFastSearch); 

  /* Add InvCommand to antenna configuration */
  LLRP_AntennaConfiguration_addAirProtocolInventoryCommandSettings(pAntConfig,
      (LLRP_tSParameter *)pInvCommand);

  /* Finally, Add antenna configuration to InventoryParameterSpec */
  LLRP_InventoryParameterSpec_addAntennaConfiguration(ipSpec, pAntConfig);

  return ret;
}

/**
 * Command to Add an ROSpec
 *
 * @param reader Reader pointer
 * @param readDuration Duration Trigger (Duration of AISpec in milliseconds)
 * @param antennaList Pointer to TMR_uint8List containing the antennas on which to inventory
 * @param filter Pointer to Tag filter
 * @param protocol Protocol to be used
 */ 
TMR_Status
TMR_LLRP_cmdAddROSpec(TMR_Reader *reader, uint16_t readDuration,
                                  TMR_uint8List *antennaList,
                                  const TMR_TagFilter *filter,
                                  TMR_TagProtocol protocol)
{
  TMR_Status ret;
  LLRP_tSADD_ROSPEC               *pCmd;
  LLRP_tSMessage                  *pCmdMsg;
  LLRP_tSMessage                  *pRspMsg;
  LLRP_tSADD_ROSPEC_RESPONSE      *pRsp;

  LLRP_tSROSpec                   *pROSpec;
  LLRP_tSROSpecStartTrigger       *pROSpecStartTrigger;
  LLRP_tSROSpecStopTrigger        *pROSpecStopTrigger;
  LLRP_tSROBoundarySpec           *pROBoundarySpec;
  LLRP_tSAISpecStopTrigger        *pAISpecStopTrigger;
  LLRP_tSInventoryParameterSpec   *pInventoryParameterSpec;
  LLRP_tSAISpec                   *pAISpec;
  LLRP_tSTagReportContentSelector *pTagReportContentSelector;
  LLRP_tSROReportSpec             *pROReportSpec;

  llrp_u16v_t                     AntennaIDs;
  int                             i;

  ret = TMR_SUCCESS;

  /**
   * Initialize AddROSpec message
   **/
  pCmd = LLRP_ADD_ROSPEC_construct();
  /**
   *  Initialize ROSpec and
   *  1. Set rospec id
   *  2. Set priority
   *  3. Set current state
   *  4. Set ROBoundarySpec
   *     4.1 Set ROSpecStartTrigger
   *     4.2 Set ROSpecStopTrigger
   *  5. Set AISpec
   *     5.1 Set AISpecStopTrigger
   *     5.2 Set AntennaID
   *     5.3 Set InventoryParameterSpec
   *  6. Set ROReportSpec
   **/

  /* Construct ROSpec message */
  pROSpec = LLRP_ROSpec_construct();
  {
    /* 1. Set ROSpec id */
    LLRP_ROSpec_setROSpecID(pROSpec, reader->u.llrpReader.roSpecId);

    /* 2. Set priority  */
    LLRP_ROSpec_setPriority(pROSpec, 0);

    /* 3. Set current State */
    LLRP_ROSpec_setCurrentState(pROSpec, LLRP_ROSpecState_Disabled);


    /** 
     * 4. Initialize and set ROBoundarySpec
     **/
    {
      pROBoundarySpec = LLRP_ROBoundarySpec_construct();

      /* Initialize ROSpec start trigger */
      pROSpecStartTrigger = LLRP_ROSpecStartTrigger_construct();
 
      if ((true == reader->continuousReading) &&
           (TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type))
      {
#ifdef TMR_ENABLE_BACKGROUND_READS
        /**
         * When TMR_ENABLE_BACKGROUND_READS is not defined, control never
         * comes here.
         **/
        /**
         * In case of continuous reading and if multiple read plans exist,
         * then each simple read plan ROSpec's start trigger is set to
         * duration trigger with the duration set to total read duration.
         **/
        LLRP_tSPeriodicTriggerValue *pPeriod;

        LLRP_ROSpecStartTrigger_setROSpecStartTriggerType(pROSpecStartTrigger,
                                        LLRP_ROSpecStartTriggerType_Periodic);

        /**
         * Initialize Periodic trigger value.
         **/
        pPeriod = LLRP_PeriodicTriggerValue_construct();
        /**
         * Total read time is equal to asyncOnTime, which is sliced to
         * each simple read plan as per their wheitage. So the total
         * period after which each ROSpec has to re-run is asyncOnTime.
         **/
        pPeriod->Period = reader->readParams.asyncOnTime;
        LLRP_ROSpecStartTrigger_setPeriodicTriggerValue(pROSpecStartTrigger, pPeriod);
#endif
      }
      else
      {
        /**
         * In all other cases, ROSpec start trigger is set to null.
         **/
        LLRP_ROSpecStartTrigger_setROSpecStartTriggerType(pROSpecStartTrigger, 
                                              LLRP_ROSpecStartTriggerType_Null);
      }

      /* Initialize ROSpec stop trigger */
      pROSpecStopTrigger = LLRP_ROSpecStopTrigger_construct();
      LLRP_ROSpecStopTrigger_setROSpecStopTriggerType(pROSpecStopTrigger, LLRP_ROSpecStopTriggerType_Null);

      /* Set start and stop triggers to BoundarySpec */
      LLRP_ROBoundarySpec_setROSpecStartTrigger(pROBoundarySpec, pROSpecStartTrigger);
      LLRP_ROBoundarySpec_setROSpecStopTrigger(pROBoundarySpec, pROSpecStopTrigger);

      /* Set ROBoundarySpec to ROSpec  */
      LLRP_ROSpec_setROBoundarySpec(pROSpec, pROBoundarySpec);
    }


    /**
     * 5. Initialize and set AISpec 
     **/
    {
      pAISpec = LLRP_AISpec_construct();

      /* Initialize AISpec stop trigger  */
      pAISpecStopTrigger = LLRP_AISpecStopTrigger_construct();

      if ((true == reader->continuousReading) &&
          (TMR_READ_PLAN_TYPE_SIMPLE == reader->readParams.readPlan->type))
      {
        /**
         * Only in case of continuous reading and if the readplan is a 
         * simple read plan, then set the duration trigger of AISpec
         * to NULL.
         **/
        LLRP_AISpecStopTrigger_setAISpecStopTriggerType(pAISpecStopTrigger, LLRP_AISpecStopTriggerType_Null);
        LLRP_AISpecStopTrigger_setDurationTrigger(pAISpecStopTrigger, 0);
      }
      else
      {
        /**
         * In all other cases, i.e., for both sync and async read
         * AISpec stop trigger should be duration based.
         **/
        LLRP_AISpecStopTrigger_setAISpecStopTriggerType(pAISpecStopTrigger, LLRP_AISpecStopTriggerType_Duration);
        LLRP_AISpecStopTrigger_setDurationTrigger(pAISpecStopTrigger, readDuration);
      }

      /* Set AISpec stop trigger */
      LLRP_AISpec_setAISpecStopTrigger(pAISpec, pAISpecStopTrigger);

      /* set antenna ids to aispec */
      {
        AntennaIDs = LLRP_u16v_construct(TMR_SR_MAX_ANTENNA_PORTS);
        AntennaIDs.nValue = 0;

        if (NULL != antennaList->list)
        {
          /**
           * In case of user set readplan, use the antenna list
           * set in read plan
           **/
          if (TMR_SR_MAX_ANTENNA_PORTS < antennaList->len)
          {
            antennaList->len = TMR_SR_MAX_ANTENNA_PORTS;
          }

          for (i = 0; i < antennaList->len; i++)
          {
            AntennaIDs.pValue[i] = antennaList->list[i];
            AntennaIDs.nValue ++;
          }
        }
        else
        {
          /**
           * In case of default read plan, use "0" to read on
           * all antennas.
           **/
          AntennaIDs.pValue[0] = 0;
          AntennaIDs.nValue    = 1;
        }

        LLRP_AISpec_setAntennaIDs(pAISpec, AntennaIDs);
      }

      /**
       *  Initialize InventoryParameterSpec
       *  and set it to AISpec
       **/
      pInventoryParameterSpec = LLRP_InventoryParameterSpec_construct();

      /* Set protocol */
      if (TMR_TAG_PROTOCOL_GEN2 == protocol)
      {
        LLRP_InventoryParameterSpec_setProtocolID(pInventoryParameterSpec, LLRP_AirProtocols_EPCGlobalClass1Gen2);
      }
#ifdef TMR_ENABLE_ISO180006B
      else if (TMR_TAG_PROTOCOL_ISO180006B == protocol)
        {
          /* For other protocol specify the Inventory Parameter protocol ID as unspecified */
          LLRP_InventoryParameterSpec_setProtocolID(pInventoryParameterSpec, LLRP_AirProtocols_Unspecified);
          LLRP_tSThingMagicCustomAirProtocols *pInventoryParameterCustom;
          pInventoryParameterCustom = LLRP_ThingMagicCustomAirProtocols_construct();
          LLRP_ThingMagicCustomAirProtocols_setcustomProtocolId(pInventoryParameterCustom,
              LLRP_ThingMagicCustomAirProtocolList_Iso180006b);

          /* add this as a custom parameter to InventoryParameterSpec */
          LLRP_InventoryParameterSpec_addCustom(pInventoryParameterSpec, (LLRP_tSParameter *)pInventoryParameterCustom);
        }
#endif /* TMR_ENABLE_ISO180006B */
      else
      {
        return TMR_ERROR_UNIMPLEMENTED_FEATURE;
      }

      /* Set InventoryParameterSpec id */
      LLRP_InventoryParameterSpec_setInventoryParameterSpecID(pInventoryParameterSpec, 1);

      /**
       * Add filter to Inventory parameter
       **/
      if (NULL != filter)
      {
        ret = TMR_LLRP_prepareTagFilter(pInventoryParameterSpec, protocol, filter);
        if (TMR_SUCCESS != ret)
        {
          return ret;
        }
      }

      /**
       * Add FastSearch option as Custom
       * parameter to the Inventory parameter
       **/
      if (reader->fastSearch)
      {
        ret = TMR_LLRP_addFastSearch(pInventoryParameterSpec, reader->fastSearch);
        if (TMR_SUCCESS != ret)
        {
          return ret;
        }
      }

      /* Add InventoryParameterSpec to AISpec */
      LLRP_AISpec_addInventoryParameterSpec(pAISpec, pInventoryParameterSpec);

      /* Now AISpec is fully framed and add it to ROSpec Parameter list */
      LLRP_ROSpec_addSpecParameter(pROSpec, (LLRP_tSParameter *)pAISpec);
    }

    /**
     * 6. Initialize and Set ROReportSpec
     **/
    {
      pROReportSpec = LLRP_ROReportSpec_construct();

      /* Set ROReportSpecTrigger type  */
      LLRP_ROReportSpec_setROReportTrigger(pROReportSpec, LLRP_ROReportTriggerType_Upon_N_Tags_Or_End_Of_ROSpec);
      
      if (reader->continuousReading)
      {
        /**
         * In case of continuous Reading, report is
         * requested for every 10 tags. (10 being the moderate value)
         * In case there is no report (when there are not enough tags),
         * we are supposed to GET_REPORT from reader.
         **/
        LLRP_ROReportSpec_setN(pROReportSpec, 1);
      }
      else
      {
        /**
         * For sync read, only one report is expected at the end of search
         * Setting N = 0, will send report only at the end of ROSpec.
         **/
        LLRP_ROReportSpec_setN(pROReportSpec, 0);
      }

      /* Initialize and Set ReportContent selection */
      pTagReportContentSelector = LLRP_TagReportContentSelector_construct();

      pTagReportContentSelector->EnableROSpecID                 = 1;
      pTagReportContentSelector->EnableSpecIndex                = 1;
      pTagReportContentSelector->EnableInventoryParameterSpecID = 1;
      pTagReportContentSelector->EnableAntennaID                = 1;
      pTagReportContentSelector->EnableChannelIndex             = 1;
      pTagReportContentSelector->EnablePeakRSSI                 = 1;
      pTagReportContentSelector->EnableFirstSeenTimestamp       = 1;
      pTagReportContentSelector->EnableLastSeenTimestamp        = 1;
      pTagReportContentSelector->EnableTagSeenCount             = 1;
      pTagReportContentSelector->EnableAccessSpecID             = 1;

      if (TMR_TAG_PROTOCOL_GEN2 == protocol)
      {
        /**
         * If Gen2 protocol, ask for CRC and PCBits as well.
         **/
        LLRP_tSC1G2EPCMemorySelector *pGen2MemSelector;

        pGen2MemSelector = LLRP_C1G2EPCMemorySelector_construct();
        LLRP_C1G2EPCMemorySelector_setEnableCRC(pGen2MemSelector, 1);
        LLRP_C1G2EPCMemorySelector_setEnablePCBits(pGen2MemSelector, 1);

        /* Add C1G2MemorySelector to ReportContentSelector  */
        LLRP_TagReportContentSelector_addAirProtocolEPCMemorySelector(
            pTagReportContentSelector, (LLRP_tSParameter *)pGen2MemSelector);

      }

      LLRP_ROReportSpec_setTagReportContentSelector(pROReportSpec, pTagReportContentSelector);
      /**
       * Add thingMagicTagReportContentSelector
       * For Backwared compatibility check the version
       **/
      if (((atoi(&reader->u.llrpReader.capabilities.softwareVersion[0]) == 4) 
            && (atoi(&reader->u.llrpReader.capabilities.softwareVersion[2]) >= 17))
          || (atoi(&reader->u.llrpReader.capabilities.softwareVersion[0]) > 4))
      {
        LLRP_tSThingMagicTagReportContentSelector *pTMTagReportContentSelector;

        /* Initialize and Set the ThingMagicReportContentSelector */
        pTMTagReportContentSelector = LLRP_ThingMagicTagReportContentSelector_construct();

        /* Set the phase angel mode */
        LLRP_ThingMagicTagReportContentSelector_setPhaseMode(
            pTMTagReportContentSelector, (LLRP_tEThingMagicPhaseMode)
            LLRP_ThingMagicPhaseMode_Enabled);

        /**
         * Set the TMTagReportContentSelector as custom
         * parameter to the ROReportSpec
         **/
        LLRP_ROReportSpec_addCustom(pROReportSpec, (LLRP_tSParameter *)pTMTagReportContentSelector);
      }

      /* Now ROReportSpec is fully framed and set it to ROSpec */
      LLRP_ROSpec_setROReportSpec(pROSpec, pROReportSpec);
    }
  }

  /* Now ROSpec is fully framed, add to AddROSpec */
  LLRP_ADD_ROSPEC_setROSpec(pCmd, pROSpec);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSADD_ROSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to Enable the ROSpec
 *
 * @param reader Reader pointer
 */
TMR_Status
TMR_LLRP_cmdEnableROSpec(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSENABLE_ROSPEC          *pCmd;
  LLRP_tSMessage                *pCmdMsg;
  LLRP_tSMessage                *pRspMsg;
  LLRP_tSENABLE_ROSPEC_RESPONSE *pRsp;
  
  ret = TMR_SUCCESS;

  /**
   * Initialize EnableROSpec message
   **/
  pCmd = LLRP_ENABLE_ROSPEC_construct();
  LLRP_ENABLE_ROSPEC_setROSpecID(pCmd, reader->u.llrpReader.roSpecId);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSENABLE_ROSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}
TMR_Status
TMR_LLRP_cmdDisableROSpec(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSDISABLE_ROSPEC          *pCmd;
  LLRP_tSMessage                *pCmdMsg;
  LLRP_tSMessage                *pRspMsg;
  LLRP_tSDISABLE_ROSPEC_RESPONSE *pRsp;
  
  ret = TMR_SUCCESS;

  /**
   * Initialize EnableROSpec message
   **/
  pCmd = LLRP_DISABLE_ROSPEC_construct();
  LLRP_DISABLE_ROSPEC_setROSpecID(pCmd, reader->u.llrpReader.roSpecId);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSDISABLE_ROSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to Start the ROSpec
 *
 * @param reader Reader pointer
 */
TMR_Status
TMR_LLRP_cmdStartROSpec(TMR_Reader *reader, llrp_u32_t roSpecId)
{
  TMR_Status ret;
  LLRP_tSSTART_ROSPEC           *pCmd;
  LLRP_tSMessage                *pCmdMsg;
  LLRP_tSMessage                *pRspMsg;
  LLRP_tSSTART_ROSPEC_RESPONSE  *pRsp;
  
  ret = TMR_SUCCESS;

  /**
   * Initialize StartROSpec message
   **/
  pCmd = LLRP_START_ROSPEC_construct();
  LLRP_START_ROSPEC_setROSpecID(pCmd, roSpecId);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSTART_ROSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to Stop ROSpec
 *
 * @param reader Reader pointer
 * @param receiveResponse Boolean parameter, whether to receive response or not.
 *        true = receive response for STOP_ROSPEC message
 *        false = Do not receive response
 */
TMR_Status
TMR_LLRP_cmdStopROSpec(TMR_Reader *reader, bool receiveResponse)
{
  TMR_Status ret;
  LLRP_tSSTOP_ROSPEC            *pCmd;
  LLRP_tSMessage                *pCmdMsg;
  LLRP_tSMessage                *pRspMsg;
  LLRP_tSSTOP_ROSPEC_RESPONSE   *pRsp;

  ret = TMR_SUCCESS;

  /**
   * Initialize StartROSpec message
   **/
  pCmd = LLRP_STOP_ROSPEC_construct();
  LLRP_STOP_ROSPEC_setROSpecID(pCmd, reader->u.llrpReader.roSpecId);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  if (false == receiveResponse)
  {
    ret = TMR_LLRP_sendMessage(reader, pCmdMsg, reader->u.llrpReader.transportTimeout);
    /**
     * done with the command, free it
     **/
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  }
  else
  {
    ret = TMR_LLRP_sendTimeout(reader, pCmdMsg, &pRspMsg,
                               TMR_LLRP_STOP_TIMEOUT
                               + reader->u.llrpReader.commandTimeout
                               + reader->u.llrpReader.transportTimeout);

    /**
     * Done with the command, free the message
     * and check for message status
     **/ 
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSTOP_ROSPEC_RESPONSE *) pRspMsg;
    if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
      return TMR_ERROR_LLRP;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  }

  return ret;
}

/**
 * Prepare ROSpec
 * This method does the initial preparations inorder
 * to perform Reader operation.
 *
 * Note: This does not start the RO Operation.
 *
 * @param reader Reader pointer
 * @param timeout Read duration
 * @param antennaList Antenna list to be used
 * @param filter Tag Filter to be used
 * @param protocol Protocol to be used
 */
TMR_Status 
TMR_LLRP_cmdPrepareROSpec(TMR_Reader *reader, uint16_t timeout,
                            TMR_uint8List *antennaList,
                            const TMR_TagFilter *filter, 
                            TMR_TagProtocol protocol)
{
  TMR_Status ret;
  TMR_LLRP_LlrpReader *lr;
  
  ret = TMR_SUCCESS;
  lr = &reader->u.llrpReader;

  /**
   *  1. ADD_ROSPEC
   *  2. ENABLE_ROSPEC
   **/
 
  /**
   * 1. AddROSpec
   **/
  ret = TMR_LLRP_cmdAddROSpec(reader, timeout, antennaList, filter, protocol);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * 2. Enable ROSpec
   **/
  return TMR_LLRP_cmdEnableROSpec(reader);
}

TMR_Status
TMR_LLRP_verifyReadOperation(TMR_Reader *reader, int32_t *tagCount)
{
  LLRP_tSTagReportData    *pTagReportData;
  LLRP_tSRO_ACCESS_REPORT *pReport;
  TMR_LLRP_LlrpReader *lr;
  uint32_t count, i;

  lr = &reader->u.llrpReader;
  /**
   * Wait until the read is finished, meanwhile
   * the tag reads are buffered by the background receiver.
   **/
  while (true)
  {
    pthread_mutex_lock(&lr->receiverLock);
    while(lr->numOfROSpecEvents > 0 ) {
      pthread_cond_wait(&lr->receiverCond, &lr->receiverLock);
    }
    if (0 >= lr->numOfROSpecEvents)
  {
      /* We have received all ROSpec events */
      if (-1 == lr->numOfROSpecEvents)
    {
      pthread_mutex_unlock(&lr->receiverLock);
        return TMR_ERROR_LLRP_READER_CONNECTION_LOST;
    }
      pthread_mutex_unlock(&lr->receiverLock);
      break;
  }
    pthread_mutex_unlock(&lr->receiverLock);

    /* Wait until all rospec events arrive */
  }

  for (i = 0; i < lr->bufPointer; i ++)
  {
    pReport = (LLRP_tSRO_ACCESS_REPORT *)lr->bufResponse[i];
    if (NULL == pReport)
    {
    /**
       * If NULL, no tag reports have been received.
       * i.e., no tags found, continue operation
     **/
      continue;
    }

    count = 0;
      /**
       * Count the number of tag reports and fill tagCount
       **/
      for(pTagReportData = pReport->listTagReportData;
          NULL != pTagReportData;
          pTagReportData = (LLRP_tSTagReportData *)pTagReportData->hdr.pNextSubParameter)
      {
        count ++;
      }

    lr->tagsRemaining += count;
      if (NULL != tagCount)
      {
        *tagCount += count;
      }
    }

  return TMR_SUCCESS;
}

/**
 * Internal method to parse metadata from LLRP Report response
 * This method constructs a TMR_TagReadData object by extracting the 
 * information from LLRP RO and ACCESS Reports
 *
 * @param reader Reader pointer
 * @param data[out] Pointer to TMR_TagReadData
 * @param pTagReportData[in] Pointer to LLRP_tSTagReportData which needs to be parsed.
 */
TMR_Status
TMR_LLRP_parseMetadataFromMessage(TMR_Reader *reader, TMR_TagReadData *data, LLRP_tSTagReportData *pTagReportData)
{
  TMR_Status ret;
  LLRP_tSParameter *pEPC;
  llrp_u16_t        ChannelIndex;

  ret = TMR_SUCCESS;

  if (NULL != pTagReportData)
  {
    if(NULL != pTagReportData->pEPCParameter)
    {
      /**
       * If EPCParameter is present, get epc bytecount and copy 
       * the value of epc.
       **/
      pEPC = pTagReportData->pEPCParameter;
      if (&LLRP_tdEPCData == pEPC->elementHdr.pType)
      {
        /**
         * If EPCParameter is of type LLRP_tSEPCData
         **/
        LLRP_tSEPCData *pEPCData = (LLRP_tSEPCData *)pEPC;
      data->tag.epcByteCount = (pEPCData->EPC.nBit + 7u) / 8u;
      memcpy(data->tag.epc, pEPCData->EPC.pValue, data->tag.epcByteCount);
    }
      else
      {
        /**
         * Else it is a LLRP_tSEPC_96 EPC, hence the epcByteCount
         * is always 12 bytes
         **/
        LLRP_tSEPC_96 *pEPC_96 = (LLRP_tSEPC_96 *)pEPC;
        data->tag.epcByteCount = 12;
        memcpy(data->tag.epc, pEPC_96->EPC.aValue, data->tag.epcByteCount);
      }
    }

    /**
     * Copy the timestamp
     **/
    llrp_u64_t msSinceEpoch = (pTagReportData->pLastSeenTimestampUTC->Microseconds)/1000;
    data->dspMicros = (msSinceEpoch % 1000);
    data->timestampHigh = (msSinceEpoch>>32) & 0xFFFFFFFF;
    data->timestampLow  = (msSinceEpoch>> 0) & 0xFFFFFFFF;
    data->metadataFlags |= TMR_TRD_METADATA_FLAG_TIMESTAMP;

    /**
     * Copy the rest of metadata
     * TODO: Add support for other metadata when the 
     * server side has implemenation.
     **/
    data->antenna = pTagReportData->pAntennaID->AntennaID;
    data->metadataFlags |= TMR_TRD_METADATA_FLAG_ANTENNAID;
    data->readCount = pTagReportData->pTagSeenCount->TagCount;
    data->metadataFlags |= TMR_TRD_METADATA_FLAG_READCOUNT;
    data->rssi = pTagReportData->pPeakRSSI->PeakRSSI;
    data->metadataFlags |= TMR_TRD_METADATA_FLAG_RSSI;

    /**
     * Copy the RF Carrier frequency with which the tag was read
     * right now only one HopeTable is supported
     **/
    ChannelIndex = pTagReportData->pChannelIndex->ChannelIndex;
    /* as for llrp the indexing always starts with one
     * but in api indexing always starts with zero
     * so, decrementing the index
     */
    if (NULL != reader->u.llrpReader.capabilities.freqTable.list)
    {
    data->frequency = reader->u.llrpReader.capabilities.freqTable.list[ChannelIndex - 1];
    data->metadataFlags |= TMR_TRD_METADATA_FLAG_FREQUENCY;
    }

    /**
     * TODO: Currently protocol is not available as part of the metadata,
     * Add support to that when server side has the implementation for it.
     * For now, hardcoding it to GEN2
     **/
    {
      /** 
       * Initialize the protocol form the readPlanProtocolList
       * depending upon the rospec id
       **/ 
      int protocolindex;
      for (protocolindex = 0; protocolindex <= pTagReportData->pROSpecID->ROSpecID; protocolindex++)
      {
        if (protocolindex == pTagReportData->pROSpecID->ROSpecID)
          data->tag.protocol = reader->u.llrpReader.readPlanProtocol[protocolindex].rospecProtocol;
      }
    }
    data->metadataFlags |= TMR_TRD_METADATA_FLAG_PROTOCOL;

    if (TMR_TAG_PROTOCOL_GEN2 == data->tag.protocol)
    {
      LLRP_tSParameter *pParameter;

      /**
       * If protocol is Gen2, extract CRC and PCBits
       **/
      for(pParameter = pTagReportData->listAirProtocolTagData;
          NULL != pParameter;
          pParameter = (LLRP_tSParameter *)pParameter->pNextSubParameter)
      {
        if (&LLRP_tdC1G2_PC == pParameter->elementHdr.pType)
        {
          llrp_u16_t pc;

          pc = LLRP_C1G2_PC_getPC_Bits((LLRP_tSC1G2_PC *)pParameter);
          data->tag.u.gen2.pc[0] = pc & 0xFF;
          data->tag.u.gen2.pc[1] = (pc & 0xFF00) >> 8;

          data->tag.u.gen2.pcByteCount = 2;

          /** TODO: Add support for XPC bits */
        }
        else if (&LLRP_tdC1G2_CRC == pParameter->elementHdr.pType)
        {
          data->tag.crc = LLRP_C1G2_CRC_getCRC((LLRP_tSC1G2_CRC *)pParameter);
        }
      } /* End of for loop */
    }
    /**
     * Extract TMCustomParameters, if any
     * For backward compatibility check for version
     **/
    if (((atoi(&reader->u.llrpReader.capabilities.softwareVersion[0]) == 4) 
          && (atoi(&reader->u.llrpReader.capabilities.softwareVersion[2]) >= 17))
          || (atoi(&reader->u.llrpReader.capabilities.softwareVersion[0]) > 4))
    {
      LLRP_tSParameter *pParameter;
      llrp_u16_t        phase;

      for (pParameter = pTagReportData->listCustom;
          NULL != pParameter;
          pParameter = (LLRP_tSParameter *)pParameter->pNextSubParameter)
      {
    /**
         * Currently phase is coming as custom parameter
         **/ 
        phase = LLRP_ThingMagicRFPhase_getPhase(
            (LLRP_tSThingMagicRFPhase *)pParameter);

        /* Copy the value to tagReport */
        data->phase = phase;
  }
    }

    /**
     * Extract OpSpecResults if any. 
     **/
    {
      LLRP_tSParameter *pParameter;

      for (pParameter = pTagReportData->listAccessCommandOpSpecResult;
           NULL != pParameter;
           pParameter = (LLRP_tSParameter *)pParameter->pNextSubParameter)
      {
        /**
         * Each airProtocol's OpSpecResult is expressed as a different
         * LLRP parameter. Verify the OpSpecResult status, if not success,
         * then notify the exception to user.
         **/
        TMR_Status retVal;

        retVal = TMR_LLRP_verifyOpSpecResultStatus(reader, pParameter);
        if (TMR_SUCCESS != retVal)
        {
          /**
           * The OpSpec has failed to execute, notify the execption to 
           * user.
           * FIXME: Notifying the error through exception listener.
           * This is exact behavior in case of continuous reading, 
           * How do we handle this for sync reads? We can not return
           * Error though..
           **/
#ifdef TMR_ENABLE_BACKGROUND_READS
          notify_exception_listeners(reader, retVal);
#endif

          /**
           * Continue with next OpSpecResult parameter.
           **/
          continue;
        }

        /**
         * At this point, we have verified the OpSpecResult status,
         * and it is success.
         * But currently, only for ReadData (i.e., C1G2ReadOpSpecResult),
         * we extract the read data and place it in TMR_TagReadData->data
         * TODO: For other tag operations which return tagop data,
         * where do we store that?
         **/
        if (NULL != &data->data)
        {
          TMR_LLRP_parseTagOpSpecData(pParameter, &data->data);
          data->metadataFlags |= TMR_TRD_METADATA_FLAG_DATA;
        }
      } /* End of for loop */
    } /* End of parsing OpSpecResults */
  }
  else
  {
    return TMR_ERROR_LLRP;
  }

  return ret;
}

/**
 * Internal method to parse TagOp data from TagOpSpecResult parameter
 * This method extracts the data from TagOpSpecResult and 
 * fills the data variable as a uint8List.
 *
 * @param pParameter[in] Pointer to LLRP_tSParameter which contains the opspec result
 * @param data[out] Pointer to TMR_uint8List which contains the extracted data.
 */
void
TMR_LLRP_parseTagOpSpecData(LLRP_tSParameter *pParameter, TMR_uint8List *data)
{
  switch (pParameter->elementHdr.pType->TypeNum)
  {
    /**
     * C1G2ReadOpSpecResult
     **/
    case TMR_LLRP_C1G2READOPSPECRESULT:
      {
        LLRP_tSC1G2ReadOpSpecResult *pReadOpSpecResult;
        int copyLen;

        /**
         * If the parameter is C1G2ReadOpSpecResult, i.e., Gen2.ReadData
         **/
        pReadOpSpecResult = (LLRP_tSC1G2ReadOpSpecResult *)pParameter;

        /* nValue is the length in 16-bit words */
        data->len = pReadOpSpecResult->ReadData.nValue * 2;
        copyLen = data->len;

        /* couple of validations and error checks */
        if (copyLen > data->max)
        {
          copyLen = data->max;
        }

        if (NULL != data->list)
        {
          /* Copy the data  */
          int i, j;

          for (i = 0, j = 0; i < copyLen; i += 2, j ++)
          {
            /* Hi byte */
            data->list[i] = pReadOpSpecResult->ReadData.pValue[j] >> 8;
            /* Lo byte */
            data->list[i + 1] = pReadOpSpecResult->ReadData.pValue[j] & 0xff;
          }
        }
        break;
      }

    /**
     * EASAlarm
     * 8 bytes of EAS alarm data will be returned, on successful operation
     **/
    case TMR_LLRP_CUSTOM_G2IEASALARMOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2IEASAlarmOpSpecResult *pResult;
        int copyLen;

        pResult = (LLRP_tSThingMagicNXPG2IEASAlarmOpSpecResult *)pParameter;
        
        /**
         * EASAlarm code is returned as uint8 list, so copy it directly
         **/
        data->len = pResult->EASAlarmCode.nValue;
        copyLen = data->len;

        /* couple of validations and error checks */
        if (copyLen > data->max)
        {
          copyLen = data->max;
        }

        if (NULL != data->list)
        {
          memcpy(data->list, pResult->EASAlarmCode.pValue, copyLen);
        }
        break;
      }

    /**
     * EASAlarm
     * 8 bytes of EAS alarm data will be returned, on successful operation
     **/
    case TMR_LLRP_CUSTOM_G2XEASALARMOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XEASAlarmOpSpecResult *pResult;
        int copyLen;

        pResult = (LLRP_tSThingMagicNXPG2XEASAlarmOpSpecResult *)pParameter;
        
        /**
         * EASAlarm code is returned as uint8 list, so copy it directly
         **/
        data->len = pResult->EASAlarmCode.nValue;
        copyLen = data->len;

        /* couple of validations and error checks */
        if (copyLen > data->max)
        {
          copyLen = data->max;
        }

        if (NULL != data->list)
        {
          memcpy(data->list, pResult->EASAlarmCode.pValue, copyLen);
        }
        break;
      }

    /**
     * Calibrate
     * 64 bytes of calibration data will be
     * returned on a successful operation
     **/
    case TMR_LLRP_CUSTOM_G2ICALIBRATEOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2ICalibrateOpSpecResult *pResult;
        int copyLen;

        pResult = (LLRP_tSThingMagicNXPG2ICalibrateOpSpecResult *)pParameter;

        /**
         * Calibrate data is returned as uint8 list, so copy it directly
         **/
        data->len = pResult->CalibrateData.nValue;
        copyLen = data->len;
        /* couple of validations and error checks */
        if (copyLen > data->max)
        {
          copyLen = data->max;
        }

        if (NULL != data->list)
        {
          memcpy(data->list, pResult->CalibrateData.pValue, copyLen);
        }
        break;
      }

    /**
     * Calibrate
     * 64 bytes of calibration data will be
     * returned on a successful operation
     **/
    case TMR_LLRP_CUSTOM_G2XCALIBRATEOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XCalibrateOpSpecResult *pResult;
        int copyLen;

        pResult = (LLRP_tSThingMagicNXPG2XCalibrateOpSpecResult *)pParameter;

        /**
         * Calibrate data is returned as uint8 list, so copy it directly
         **/
        data->len = pResult->CalibrateData.nValue;
        copyLen = data->len;
        /* couple of validations and error checks */
        if (copyLen > data->max)
        {
          copyLen = data->max;
        }

        if (NULL != data->list)
        {
          memcpy(data->list, pResult->CalibrateData.pValue, copyLen);
        }
        break;
      }

    /**
     * ChangeConfig
     * Returns current configword setting on the tag
     * on a successful operation
     **/
    case TMR_LLRP_CUSTOM_G2ICHANGECONFIGOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2IChangeConfigOpSpecResult *pResult;

        pResult = (LLRP_tSThingMagicNXPG2IChangeConfigOpSpecResult *)pParameter;

        /**
         * Configword is returned as uint16. So
         * send it as an array of two uint8's
         **/
        data->len = 2;
        /* couple of validations and error checks */
        if ((data->len < data->max) &&
            (NULL != data->list))
        {
          /* Hi byte */
          data->list[0] = pResult->ConfigData >> 8;
          /* Lo byte */
          data->list[1] = pResult->ConfigData & 0xff;
        }
        break;
      }

    /**
       * IDS GetBatteryLevel Command
       * Returns following fields on successful operation
       */
    case TMR_LLRP_CUSTOM_IDS_GETBATTERYLEVELOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AGetBatteryLevelOpSpecResult *pResult;

        pResult = (LLRP_tSThingMagicIDSSL900AGetBatteryLevelOpSpecResult *)pParameter;

        /**
         * IDS GetBatteryLevel Data comes as byte array
         * send it as an array of  uint8's
         */
        data->len = pResult->pThingMagicIDSBatteryLevel->batteryValueByteStream.nValue;
        /* couple of validations and error checks */
        if (data->len > data->max)
        {
          data->len = data->max;
        }
        if (NULL != data->list)
        {
          memcpy (data->list, pResult->pThingMagicIDSBatteryLevel->batteryValueByteStream.pValue,
              (size_t)(pResult->pThingMagicIDSBatteryLevel->batteryValueByteStream.nValue * sizeof(uint8_t)));
        }
        break;
      }

    /**
     * IDS GetMeasurementSetup Command
     * returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_IDS_GETMEASUREMENTSETUPOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AGetMeasurementSetupOpSpecResult *pResult;

        pResult = (LLRP_tSThingMagicIDSSL900AGetMeasurementSetupOpSpecResult *)pParameter;

        /**
         * IDS GetMeasurementsetup Data comes as byte array
         * send it as an array of  uint8's
         */
        data->len = pResult->measurementByteStream.nValue;
        /* couple of validations and error checks */
        if (data->len > data->max)
        {
          data->len = data->max;
        }
        if (NULL != data->list)
        {
          memcpy (data->list, pResult->measurementByteStream.pValue,
              (size_t)(pResult->measurementByteStream.nValue * sizeof(uint8_t)));
        }
        break;
      }

    /**
     * QT ReadWrite
     * Returns the payload on a successful operation
     **/
    case TMR_LLRP_CUSTOM_MONZA4QTREADWRITEOPSPECRESULT:
      {
        LLRP_tSThingMagicImpinjMonza4QTReadWriteOpSpecResult *pResult;

        pResult = (LLRP_tSThingMagicImpinjMonza4QTReadWriteOpSpecResult *)pParameter;

        /**
         * Payload is returned as uint16. So
         * send it as an array of two uint8's
         **/
        data->len = 2;
        /* couple of validations and error checks */
        if ((data->len < data->max) &&
            (NULL != data->list))
        {
          /* Hi byte */
          data->list[0] = pResult->Payload >> 8;
          /* Lo byte */
          data->list[1] = pResult->Payload & 0xff;
        }
        break;
      }

      /**
       * IDS GetSensor Command
       * Returns following fields on a successful operation
       */
    case TMR_LLRP_CUSTOM_IDS_GETSENSORVALUEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ASensorValueOpSpecResult  *pResult;

        pResult = (LLRP_tSThingMagicIDSSL900ASensorValueOpSpecResult *)pParameter;

        /**
         * IDSGetSensor return value as uint16. So
         * send it as an array of two uint8's
         **/
        data->len = 2;
        /* couple of validations and error checks */
        if (data->len > data->max)
        {
          data->len = data->max;
        }

        if (NULL != data->list)
        {
          /* Hi byte */
          data->list[0] = pResult->raw >> 8;
          /* Lo byte */
          data->list[1] = pResult->raw & 0xff;
        }
        break;
      }

      /**
       * IDS GetLogState Command
       * Returns following fields on successful operation
       */ 
    case TMR_LLRP_CUSTOM_IDS_GETLOGSTATEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ALogStateOpSpecResult  *pResult;

        pResult = (LLRP_tSThingMagicIDSSL900ALogStateOpSpecResult *)pParameter;

        /**
         * IDS GetLogState Data comes as byte array
         * send it as an array of  uint8's
         **/
        data->len = pResult->LogStateByteStream.nValue;
        /* couple of validations and error checks */
        if (data->len > data->max)
        {
          data->len = data->max;
        }

        if (NULL != data->list)
        {
          memcpy (data->list, pResult->LogStateByteStream.pValue, (llrp_u16_t)data->len); 
        }
        break;
      }

      /**
       * IDS AcccessFifoStatus Command
       * Returns following fields on successful operation
       */
    case TMR_LLRP_CUSTOM_IDS_ACCESSFIFOSTATUSOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AAccessFIFOStatusOpSpecResult  *pResult;

        pResult = (LLRP_tSThingMagicIDSSL900AAccessFIFOStatusOpSpecResult *)pParameter;

        /**
         * IDS AccessFifoStatus response comes as uint8_t value
         * copy the response
         */
        data->len = 1;
        /* couple of validations and error checks */
        if (data->len > data->max)
        {
          data->len = data->max;
        }

        if (NULL != data->list)
        {
          memcpy (data->list, &(pResult->FIFOStatusRawByte), (size_t)((data->len) * sizeof(uint8_t)));
        }
        break;
      }

      /**
       * IDS AccessFifoRead Command
       * Returns following fields on a successful operation
       */ 
    case TMR_LLRP_CUSTOM_IDS_ACCESSFIFOREADOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AAccessFIFOReadOpSpecResult  *pResult;

        pResult = (LLRP_tSThingMagicIDSSL900AAccessFIFOReadOpSpecResult *)pParameter;

        /**
         * IDS AccessFifoRead response comes as byte array
         * sent it as array of uin8_t values
         */
        data->len = pResult->readPayLoad.nValue;
        /* couple of validations and error checks */
        if (data->len > data->max)
        {
          data->len = data->max;
        }

        if (NULL != data->list)
        {
          memcpy (data->list, pResult->readPayLoad.pValue, (size_t)(pResult->readPayLoad.nValue * sizeof(uint8_t)));
        }

        break;
      }

      /**
       * IDS GetCalibrationData Command
       * Returns following fields on a successful operation
       */
    case TMR_LLRP_CUSTOM_IDS_GETCALIBRATIONDATAOPSPECRESULT:
    {
    	LLRP_tSThingMagicIDSSL900AGetCalibrationDataOpSpecResult  *pResult;

      pResult = (LLRP_tSThingMagicIDSSL900AGetCalibrationDataOpSpecResult *)pParameter;

      /**
       * IDS GetCalibrationData Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->pThingMagicIDSCalibrationData->calibrationValueByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->pThingMagicIDSCalibrationData->calibrationValueByteStream.pValue,
        		(size_t)(pResult->pThingMagicIDSCalibrationData->calibrationValueByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV ActivateSecureMode Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_ACTIVATESECUREMODEOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVActivateSecureModeOpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVActivateSecureModeOpSpecResult *)pParameter;

      /**
       * DenatranIAV ActivateSecureMode Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->ActivateSecureModeByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->ActivateSecureModeByteStream.pValue,
            (size_t)(pResult->ActivateSecureModeByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV OuthenticateOBU Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEOBUOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVAuthenticateOBUOpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVAuthenticateOBUOpSpecResult *)pParameter;

      /**
       * DenatranIAV AuthenticateOBU Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->AuthenitcateOBUByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->AuthenitcateOBUByteStream.pValue,
            (size_t)(pResult->AuthenitcateOBUByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV ActivateSiniavMode Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_ACTIVATESINIAVMODEOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVActivateSiniavModeOpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVActivateSiniavModeOpSpecResult *)pParameter;

      /**
       * DenatranIAV ActivateSiniavMode Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->ActivateSiniavModeByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->ActivateSiniavModeByteStream.pValue,
            (size_t)(pResult->ActivateSiniavModeByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV OBUAuthenticateID Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEIDOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUAuthenticateIDOpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVOBUAuthenticateIDOpSpecResult *)pParameter;

      /**
       * DenatranIAV OBUAuthenticateID Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->OBUAuthenticateIDByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->OBUAuthenticateIDByteStream.pValue,
            (size_t)(pResult->OBUAuthenticateIDByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV AuthenticateOBU FullPass1 Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEFULLPASS1OPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass1OpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass1OpSpecResult *)pParameter;

      /**
       * DenatranIAV AuthenticateOBU FullPass1 Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->OBUAuthenticateFullPass1ByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->OBUAuthenticateFullPass1ByteStream.pValue,
            (size_t)(pResult->OBUAuthenticateFullPass1ByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV AuthenticateOBU FullPass2 Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEFULLPASS2OPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass2OpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass2OpSpecResult *)pParameter;

      /**
       * DenatranIAV AuthenticateOBU FullPass2 Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->OBUAuthenticateFullPass2ByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->OBUAuthenticateFullPass2ByteStream.pValue,
            (size_t)(pResult->OBUAuthenticateFullPass2ByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV OBU ReadFromMemMap Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_OBUREADFROMMEMMAPOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUReadFromMemMapOpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVOBUReadFromMemMapOpSpecResult *)pParameter;

      /**
       * DenatranIAV OBU ReadFromMemMap Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->OBUReadMemoryMapByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->OBUReadMemoryMapByteStream.pValue,
            (size_t)(pResult->OBUReadMemoryMapByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

    /**
     * DenatranIAV OBU WriteMemoryMap Command
     * Returns following fields on successful operation
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_OBUWRITETOMEMMAPOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUWriteToMemMapOpSpecResult *pResult;

      pResult = (LLRP_tSThingMagicDenatranIAVOBUWriteToMemMapOpSpecResult *)pParameter;

      /**
       * DenatranIAV OBU WriteToMemMap Command response comes as byte array
       * sent it as array of uin8_t values
       */
      data->len = pResult->OBUWriteMemoryMapByteStream.nValue;
      /* couple of validations and error checks */
      if (data->len > data->max)
      {
        data->len = data->max;
      }

      if (NULL != data->list)
      {
        memcpy (data->list, pResult->OBUWriteMemoryMapByteStream.pValue,
            (size_t)(pResult->OBUWriteMemoryMapByteStream.nValue * sizeof(uint8_t)));
      }

      break;
    }

#ifdef TMR_ENABLE_ISO180006B
    case TMR_LLRP_CUSTOM_ISO_READDATAOPSPECRESULT:
      {
        LLRP_tSThingMagicISO180006BReadOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicISO180006BReadOpSpecResult *)pParameter;
        memcpy(data->list, pResult->ReadData.pValue, (pResult->ReadData.nValue * sizeof(uint8_t)));

        break; 
      }
#endif /* TMR_ENABLE_ISO180006B */

    default:
      {
        /**
         * Tag operations other than the above might not return
         * any data. Do nothing here.
         **/
      }
  }
}

/**
 * Command to get ThingMagic DeDuplication
 * @param reader Reader pointer
 * @param duplication pointer to TMR_LLRP_TMDeDuplication
 * to hold the value of ThingMagic DeDuplication parameter
 */
TMR_Status
TMR_LLRP_cmdGetThingMagicDeDuplication(TMR_Reader *reader, TMR_LLRP_TMDeDuplication *duplication)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMDeDuplication;
  LLRP_tSParameter                      *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd,
      LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * Thingmagic DeDuplication  is available as a custom parameter under
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMDeDuplication = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMDeDuplication)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., reader DeDuplication)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMDeDuplication,
      LLRP_ThingMagicControlConfiguration_ThingMagicDeDuplication);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMDeDuplication->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMDeDuplication);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Response is success, extract reader configuration from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    /* Get the Record Highest RSSI */
    duplication->highestRSSI = LLRP_ThingMagicDeDuplication_getRecordHighestRSSI((LLRP_tSThingMagicDeDuplication*)pCustParam);

    /* Get the Unique By antenna */
    duplication->uniquebyAntenna = LLRP_ThingMagicDeDuplication_getUniqueByAntenna((LLRP_tSThingMagicDeDuplication*)pCustParam);

    /* Get the Unique By data */
    duplication->uniquebyData = LLRP_ThingMagicDeDuplication_getUniqueByData((LLRP_tSThingMagicDeDuplication*)pCustParam);
  }
  else
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to set ThingMagic DeDuplication
 * @param reader Reader pointer
 * @param duplication pointer to TMR_LLRP_TMDeDuplication
 * to hold the value of ThingMagic DeDuplication parameter
 */
TMR_Status
TMR_LLRP_cmdSetThingMagicDeDuplication(TMR_Reader *reader, TMR_LLRP_TMDeDuplication *duplication)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeDuplication        *pTMDeDuplication;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic DeDuplication is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize DeDuplication */
  pTMDeDuplication = LLRP_ThingMagicDeDuplication_construct();

  /* set the highestRSSI to DeDuplication */
  pTMDeDuplication->RecordHighestRSSI = duplication->highestRSSI;

  /* set the unique by antenna to DeDuplication */
  pTMDeDuplication->UniqueByAntenna = duplication->uniquebyAntenna;

  /* set the unique by data to DeDuplication */
  pTMDeDuplication->UniqueByData = duplication->uniquebyData;

  /* Add DeDuplication as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pTMDeDuplication);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

void 
TMR_LLRP_freeTMReaderConfiguration(TMR_LLRP_TMReaderConfiguration *config)
{
  if (NULL != config->description.pValue)
  {
    free(config->description.pValue);
  }

  if (NULL != config->role.pValue)
  {
    free(config->role.pValue);
  }
}


/**
 * Command to get Thingmagic Reader configuration
 * It is the user responsibility to free the "config" after 
 * a successful get (Call TMR_LLRP_freeTMReaderConfiguration to free)
 *
 * @param reader Reader pointer
 * @param[out] config Pointer to TMR_LLRP_TMReaderConfiguration which holds the configuration
 */
TMR_Status
TMR_LLRP_cmdGetThingmagicReaderConfiguration(TMR_Reader *reader, 
                       TMR_LLRP_TMReaderConfiguration *config)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMReaderConfig;
  LLRP_tSParameter                      *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, 
                        LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * Thingmagic Reader configuration is available as a custom parameter under 
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMReaderConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMReaderConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., reader configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMReaderConfig, 
            LLRP_ThingMagicControlConfiguration_ThingMagicReaderConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMReaderConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMReaderConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract reader configuration from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    /* Get description */
    {
      llrp_utf8v_t description;
      description = LLRP_ThingMagicReaderConfiguration_getReaderDescription((LLRP_tSThingMagicReaderConfiguration*) pCustParam);
      config->description = LLRP_utf8v_copy(description);

    }
    /* Get Role */
    {
      llrp_utf8v_t role;
      role = LLRP_ThingMagicReaderConfiguration_getReaderRole((LLRP_tSThingMagicReaderConfiguration*) pCustParam);
      config->role = LLRP_utf8v_copy(role);
    }
    /* Get host name */
    {
      llrp_utf8v_t hostname;
      hostname = LLRP_ThingMagicReaderConfiguration_getReaderHostName((LLRP_tSThingMagicReaderConfiguration*) pCustParam);
      config->hostName = LLRP_utf8v_copy(hostname);
    }
  }
  else
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }
  
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to set Thingmagic reader configuration
 *
 * @param reader Reader pointer
 * @param config[in] Pointer to TMR_LLRP_TMReaderConfiguration to be set
 */
TMR_Status
TMR_LLRP_cmdSetThingmagicReaderConfiguration(TMR_Reader *reader, 
                                  TMR_LLRP_TMReaderConfiguration *config)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicReaderConfiguration  *pReaderConfig;
  
  ret = TMR_SUCCESS;
  /**
   * Thingmagic Reader configuration is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Reader configuration */
  pReaderConfig = LLRP_ThingMagicReaderConfiguration_construct();

  /* Set description to reader config */
  pReaderConfig->ReaderDescription = LLRP_utf8v_copy(config->description);
  /* Set Reader role */
  pReaderConfig->ReaderRole = LLRP_utf8v_copy(config->role);
  /* Set reader host name */
  pReaderConfig->ReaderHostName = LLRP_utf8v_copy(config->hostName);

  /* Add ThingMagic reader configuration as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pReaderConfig);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to get Thingmagic Current Time
 *
 * @param reader Reader pointer
 * @param curTime Pointer to TMR_String  to
 *  hold the value of Thingmagic Current Time
 */
TMR_Status
TMR_LLRP_cmdGetThingMagicCurrentTime(TMR_Reader *reader, struct tm *curTime)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMCurrentTime;
  LLRP_tSParameter                      *pCustParam;
  
  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd,
      LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * Thingmagic Current Time  is available as a custom parameter under
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMCurrentTime = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMCurrentTime)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., Current Time)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMCurrentTime,
      LLRP_ThingMagicControlConfiguration_ThingMagicCurrentTime);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMCurrentTime->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMCurrentTime);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Response is success, extract current  time  from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    /* Get the reader current time */
    {
      llrp_utf8v_t readerCT;
      readerCT = LLRP_ThingMagicCurrentTime_getReaderCurrentTime ((LLRP_tSThingMagicCurrentTime *) pCustParam);
      if (!strptime((const char *)readerCT.pValue, "%Y-%m-%dT%H:%M:%S", curTime))
      {
        return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
      }
    }
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}


/**
 * Command to get Thingmagic Reader Module Temperature
 *
 * @param reader Reader pointer
 * @param readerTemp Pointer to uint8_t  to
 *  hold the value of Thingmagic Reader Module Temperature
 */
TMR_Status
TMR_LLRP_cmdGetThingMagicReaderModuleTemperature(TMR_Reader *reader, uint8_t *readerTemp)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMReaderTemp;
  LLRP_tSParameter                      *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd,
      LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * Thingmagic Reader Module Temperature  is available
   * as a custom parameter under
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMReaderTemp = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMReaderTemp)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e. Reader Temperature,)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMReaderTemp,
      LLRP_ThingMagicControlConfiguration_ThingMagicReaderModuleTemperature);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMReaderTemp->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMReaderTemp);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Response is success, extract reader temperature  from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    /* get the reader module temperature */
    llrp_u8_t temp;
    temp = LLRP_ThingMagicReaderModuleTemperature_getReaderModuleTemperature ((
         LLRP_tSThingMagicReaderModuleTemperature *) pCustParam);

   *(uint8_t *)readerTemp = (uint8_t)temp;
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to get Thingmagic Antenna Detection
 *
 * @param reader Reader pointer
 * @param antennaport Pointer to bool  to
 *  hold the value of Thingmagic Antenna  Detection
 */
TMR_Status
TMR_LLRP_cmdGetThingMagicAntennaDetection(TMR_Reader *reader, bool *antennaport)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration  *pTMAntennaDetection;
  LLRP_tSParameter                      *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd,
      LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * Thingmagic Antenna Detection   is available
   * as a custom parameter under
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMAntennaDetection = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMAntennaDetection)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e. Antenna Detection,)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMAntennaDetection,
      LLRP_ThingMagicControlConfiguration_ThingMagicAntennaDetection);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMAntennaDetection->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMAntennaDetection);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Response is success, extract antenna detection  from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    /* get the antenna checkport */
    llrp_u1_t temp;
    temp = LLRP_ThingMagicAntennaDetection_getAntennaDetection ((
          LLRP_tSThingMagicAntennaDetection *) pCustParam);

    *(bool*)antennaport = (bool)temp;
  }
  else
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

/**
 * Command to set Thingmagic Antenna Detection 
 *
 * @param reader Reader pointer
 * @param antennaport Pointer to bool  to
 *  hold the value of Thingmagic Antenna  Detection 
 */
TMR_Status
TMR_LLRP_cmdSetThingMagicAntennaDetection(TMR_Reader *reader, bool *antennaport)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSThingMagicAntennaDetection        *pTMAntennaDetection;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic Antenna Detection is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Antenna Detection */
  pTMAntennaDetection = LLRP_ThingMagicAntennaDetection_construct();

  /* set  Antenna Detction */
  pTMAntennaDetection->AntennaDetection = *(bool*)antennaport;

  /* Add Antenna Detection as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pTMAntennaDetection);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

/**
 * Command to get Thingmagic Device Protocol Capabilities
 *
 * @param reader Reader pointer
 * @param protocol Pointer to TMR_TagProtocolList  to
 *  hold the value of Thingmagic Device protocol Capabilities
 */
TMR_Status
TMR_LLRP_cmdGetTMDeviceProtocolCapabilities(TMR_Reader *reader, TMR_TagProtocolList *protocolList)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CAPABILITIES              *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CAPABILITIES_RESPONSE     *pRsp;
  LLRP_tSThingMagicDeviceControlCapabilities  *pTMCaps;
  LLRP_tSParameter                            *pCustParam;
  LLRP_tSSupportedProtocols                   *pSupportedProtocols;
  uint8_t                                     i;

  ret = TMR_SUCCESS;

  /**
   * Initialize GET_READER_CAPABILITIES message
   **/
  pCmd = LLRP_GET_READER_CAPABILITIES_construct();
  LLRP_GET_READER_CAPABILITIES_setRequestedData(pCmd, LLRP_GetReaderCapabilitiesRequestedData_General_Device_Capabilities);

  /**
   * /reader/version/supportedProtocols is a custom parameter.And is available as part of
   * ThingMagicDeviceControlCapabilities.ThingMagicControlCapabilities.DeviceProtocolCapabilities
   **/
  pTMCaps = LLRP_ThingMagicDeviceControlCapabilities_construct();
  if (NULL == pTMCaps)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * set the requested data
   * and add to GET_READER_CAPABILITIES message
   **/
  LLRP_ThingMagicDeviceControlCapabilities_setRequestedData(pTMCaps, LLRP_ThingMagicControlCapabilities_DeviceProtocolCapabilities);
  if (LLRP_RC_OK != LLRP_GET_READER_CAPABILITIES_addCustom(pCmd, &pTMCaps->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMCaps);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }
  pCmdMsg       = &pCmd->hdr;

  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CAPABILITIES_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }
  /**
   * Response is success
   * Extract supported protocol  from it
   **/
  pCustParam = LLRP_GET_READER_CAPABILITIES_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    protocolList->len = 0;

    for (pSupportedProtocols = LLRP_DeviceProtocolCapabilities_beginSupportedProtocols(
          (LLRP_tSDeviceProtocolCapabilities *) pCustParam), i = 0;
        (NULL != pSupportedProtocols);
        pSupportedProtocols = LLRP_DeviceProtocolCapabilities_nextSupportedProtocols(
          pSupportedProtocols), i ++)
    {
      protocolList->list[i] = LLRP_SupportedProtocols_getProtocol(pSupportedProtocols);
      /**
       * Currently we are suppoerting only GEn2 ans Iso protocol
       **/
      if (TMR_TAG_PROTOCOL_GEN2 == protocolList->list[i] || 
                                 TMR_TAG_PROTOCOL_ISO180006B == protocolList->list[i]) 
        reader->u.llrpReader.supportedProtocols |= (1 << (protocolList->list[i] -1 ));
      protocolList->len ++;
    }

  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;

}

/**
 * Command to get active RFControl
 *
 * @param reader Reader pointer
 * @param rfControl[out] Pointer to TMR_LLRP_RFControl
 */
TMR_Status 
TMR_LLRP_cmdGetActiveRFControl(TMR_Reader *reader, TMR_LLRP_RFControl *rfControl)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSAntennaConfiguration           *pAntConfig;

  ret = TMR_SUCCESS;
  /**
   * RFControl can be retreived
   * through  LLRP standard parameter GET_READER_CONFIG_RESPONSE.AntennaConfiguration.
   * airProtocolInventoryCommand.RFControl
   * Initialize GET_READER_CONFIG message 
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_AntennaConfiguration);

  /* Get antenna configuration for all antennas*/
  LLRP_GET_READER_CONFIG_setAntennaID(pCmd, 0);

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract mode index from it
   **/
  for (pAntConfig = LLRP_GET_READER_CONFIG_RESPONSE_beginAntennaConfiguration(pRsp);
      (pAntConfig != NULL);
      pAntConfig = LLRP_GET_READER_CONFIG_RESPONSE_nextAntennaConfiguration(pAntConfig))
  {
    LLRP_tSParameter *pInventoryCommand;

    for (pInventoryCommand = LLRP_AntennaConfiguration_beginAirProtocolInventoryCommandSettings(pAntConfig);
        (pInventoryCommand != NULL);
        pInventoryCommand = LLRP_AntennaConfiguration_nextAirProtocolInventoryCommandSettings(pInventoryCommand))
    {
      LLRP_tSC1G2RFControl *pRFControl;
      pRFControl = LLRP_C1G2InventoryCommand_getC1G2RFControl(
          (LLRP_tSC1G2InventoryCommand *)pInventoryCommand);
      rfControl->index = pRFControl->ModeIndex;

      /* Convert Tari value */
      switch (pRFControl->Tari)
      {
        case 25000:
          rfControl->tari = TMR_GEN2_TARI_25US;
          break;

        case 12500:
          rfControl->tari = TMR_GEN2_TARI_12_5US;
          break;

        case 6250:
          rfControl->tari = TMR_GEN2_TARI_6_25US;
          break;

        default:
          rfControl->tari = TMR_GEN2_TARI_INVALID;
      }
    }
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to set active RFControl
 *
 * @param reader Reader pointer
 * @param rfControl[in] Pointer to TMR_LLRP_RFControl
 */
TMR_Status 
TMR_LLRP_cmdSetActiveRFControl(TMR_Reader *reader, TMR_LLRP_RFControl *rfControl)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSAntennaConfiguration           *pAntConfig;
  LLRP_tSC1G2RFControl                  *pRFControl;
  LLRP_tSC1G2InventoryCommand           *pInventoryCommand;


  ret = TMR_SUCCESS;
  /**
   * RFControl can be set
   * through  LLRP standard parameter GET_READER_CONFIG_RESPONSE.AntennaConfiguration.
   * airProtocolInventoryCommand.RFControl
   * Initialize SET_READER_CONFIG message 
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize antenna configuration */
  pAntConfig = LLRP_AntennaConfiguration_construct();
  LLRP_AntennaConfiguration_setAntennaID(pAntConfig, 0); /* For all antennas */

  /* Initialize Inventory Command */
  pInventoryCommand = LLRP_C1G2InventoryCommand_construct();

  /* Initialize RFControl */
  pRFControl = LLRP_C1G2RFControl_construct();
  pRFControl->ModeIndex = rfControl->index;

  /* Convert Tari into nano seconds */
  switch (rfControl->tari)
  {
    case TMR_GEN2_TARI_25US:
      pRFControl->Tari = 25000;
      break;

    case TMR_GEN2_TARI_12_5US:
      pRFControl->Tari = 12500;
      break;

    case TMR_GEN2_TARI_6_25US:
      pRFControl->Tari = 6250;
      break;

    default:
      /**
       * FIXME: should we return error in case of default value
       * or send Tari as 0? Considering 0 as default value
       **/
      pRFControl->Tari = 0;
  }

  /* Set RFControl to inventoryCommand */
  LLRP_C1G2InventoryCommand_setC1G2RFControl(pInventoryCommand, pRFControl);

  /* Set InventoryCommand to antenna configuration */
  LLRP_AntennaConfiguration_addAirProtocolInventoryCommandSettings(
      pAntConfig, (LLRP_tSParameter *)pInventoryCommand);

  /* Set antenna configuration to SET_READER_CONFIG */
  LLRP_SET_READER_CONFIG_addAntennaConfiguration(pCmd, pAntConfig);

  pCmdMsg       = &pCmd->hdr;

  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to get Gen2 Q value
 *
 * @param reader Reader pointer
 * @param[out] q Pointer to TMR_GEN2_Q object to hold the region value
 */
TMR_Status
TMR_LLRP_cmdGetGen2Q(TMR_Reader *reader, TMR_GEN2_Q *q)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG                    *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE           *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration *pTMProtoConfig;
  LLRP_tSParameter                            *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/gen2/q" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMProtoConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMProtoConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., protocol configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMProtoConfig, 
        LLRP_ThingMagicControlConfiguration_ThingMagicProtocolConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMProtoConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMProtoConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract Gen2 Q from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    LLRP_tSGen2CustomParameters *gen2Custom;
    LLRP_tSGen2Q *gen2Q;

    gen2Custom = LLRP_ThingMagicProtocolConfiguration_getGen2CustomParameters(
                        (LLRP_tSThingMagicProtocolConfiguration *)pCustParam);
    gen2Q = LLRP_Gen2CustomParameters_getGen2Q(gen2Custom);

    if(gen2Q->eGen2QType)
    {
      /* If static Q, then get the q value */
      q->type = TMR_SR_GEN2_Q_STATIC;
      q->u.staticQ.initialQ = gen2Q->InitQValue;
    }
    else
    {
      q->type = TMR_SR_GEN2_Q_DYNAMIC;
    }
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }
  
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Command to Set Gen2 Q value
 *
 * @param reader Reader pointer
 * @param[in] q Pointer to TMR_GEN2_Q object which needs to be set
 */
TMR_Status
TMR_LLRP_cmdSetGen2Q(TMR_Reader *reader, TMR_GEN2_Q *q)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicProtocolConfiguration  *pProtoConfig;
  LLRP_tSGen2CustomParameters             *pGen2Custom;
  LLRP_tSGen2Q                            *pGen2Q;
  
  ret = TMR_SUCCESS;
  /**
   * Thingmagic Protocol configuration is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Protocol configuration */
  pProtoConfig = LLRP_ThingMagicProtocolConfiguration_construct();

  /* Initialize gen2 Q */
  pGen2Q = LLRP_Gen2Q_construct();
  pGen2Q->eGen2QType = q->type;

  if (TMR_SR_GEN2_Q_STATIC == q->type)
  {
    /* If static q, set the value */
    pGen2Q->InitQValue = q->u.staticQ.initialQ;
  }
  else
  {
    /* If dynamic, set initQ value to 0 */
    pGen2Q->InitQValue = 0;
  }

  /* Initialize gen2 custom parameter */
  pGen2Custom = LLRP_Gen2CustomParameters_construct();
  LLRP_Gen2CustomParameters_setGen2Q(pGen2Custom, pGen2Q);

  /* Set Gen2 Custom parameter to protocol configuration */
  LLRP_ThingMagicProtocolConfiguration_setGen2CustomParameters(pProtoConfig,
                                                                pGen2Custom);
    
  /* Add ThingMagic Protocol configuration as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pProtoConfig);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}


/**
 * Command to get Gen2 Session value
 *
 * @param reader Reader pointer
 * @param session Pointer to TMR_Gen2_session
 */
TMR_Status
TMR_LLRP_cmdGetGen2Session(TMR_Reader *reader, TMR_GEN2_Session *session)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSAntennaConfiguration           *pAntConfig;

  ret = TMR_SUCCESS;
  /**
   * Gen2 Session can be retreived
   * through  LLRP standard parameter GET_READER_CONFIG_RESPONSE.AntennaConfiguration.
   * airProtocolInventoryCommand.SingulationControl
   * Initialize GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_AntennaConfiguration);

  /* Get antenna configuration for all antennas*/
  LLRP_GET_READER_CONFIG_setAntennaID(pCmd, 0);

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Response is success, extract Session value from response
   **/
  for (pAntConfig = LLRP_GET_READER_CONFIG_RESPONSE_beginAntennaConfiguration(pRsp);
      (pAntConfig != NULL);
      pAntConfig = LLRP_GET_READER_CONFIG_RESPONSE_nextAntennaConfiguration(pAntConfig))
  {
    LLRP_tSParameter *pInventoryCommand;
    llrp_u2_t        temp;

    for (pInventoryCommand = LLRP_AntennaConfiguration_beginAirProtocolInventoryCommandSettings(pAntConfig);
        (pInventoryCommand != NULL);
        pInventoryCommand = LLRP_AntennaConfiguration_nextAirProtocolInventoryCommandSettings(pInventoryCommand))
    {
      LLRP_tSC1G2SingulationControl *pSingulationControl;

      pSingulationControl = LLRP_C1G2InventoryCommand_getC1G2SingulationControl(
          (LLRP_tSC1G2InventoryCommand *)pInventoryCommand);

      /* get the session */
      temp = LLRP_C1G2SingulationControl_getSession(pSingulationControl);

      /* convert the session value */
      switch (temp)
      {
        case 0:
          {
            *session = TMR_GEN2_SESSION_S0;
            break;
          }
        case 1:
          {
            *session = TMR_GEN2_SESSION_S1;
            break;
          }
        case 2:
          {
            *session = TMR_GEN2_SESSION_S2;
            break;
          }
        case 3:
          {
            *session = TMR_GEN2_SESSION_S3;
            break;
          }
        default:
          {
            *session = TMR_GEN2_SESSION_INVALID;
            break;
          }
      }
    }
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to set Gen2 Session
 *
 * @param reader Reader pointer
 * @param session Pointer to TMR_GEN2_Session
 */
TMR_Status
TMR_LLRP_cmdSetGen2Session(TMR_Reader *reader, TMR_GEN2_Session *session)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG              *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE     *pRsp;
  LLRP_tSAntennaConfiguration           *pAntConfig;
  LLRP_tSC1G2SingulationControl         *pSingulationControl;
  LLRP_tSC1G2InventoryCommand           *pInventoryCommand;

  ret = TMR_SUCCESS;
  /**
   * Gen2 Session can be set
   * through  LLRP standard parameter GET_READER_CONFIG_RESPONSE.AntennaConfiguration.
   * airProtocolInventoryCommand.SingulationControl
   * Initialize SET_READER_CONFIG message
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize antenna configuration */
  pAntConfig = LLRP_AntennaConfiguration_construct();
  if (NULL == pAntConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  LLRP_AntennaConfiguration_setAntennaID(pAntConfig, 0); /* For all antennas */

  /* Initialize Inventory Command */
  pInventoryCommand = LLRP_C1G2InventoryCommand_construct();
  if (NULL == pInventoryCommand)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pAntConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /* Initialize SingulationControl */
  pSingulationControl = LLRP_C1G2SingulationControl_construct();
  if (NULL == pSingulationControl)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pInventoryCommand);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pAntConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }


  /* convert the session value */
  switch (*session)
  {
    case TMR_GEN2_SESSION_S0:
      {
        pSingulationControl->Session = 0;
        break;
      }
    case TMR_GEN2_SESSION_S1:
      {
        pSingulationControl->Session = 1;
        break;
      }
    case TMR_GEN2_SESSION_S2:
      {
        pSingulationControl->Session = 2;
        break;
      }
    case TMR_GEN2_SESSION_S3:
      {
        pSingulationControl->Session = 3;
        break;
      }
    default:
      {
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pSingulationControl);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pInventoryCommand);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pAntConfig);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
        return TMR_ERROR_INVALID;
      }
  }

  /* Set Singulation Control to inventoryCommand */
  LLRP_C1G2InventoryCommand_setC1G2SingulationControl(pInventoryCommand, pSingulationControl);

  /* Set InventoryCommand to antenna configuration */
  LLRP_AntennaConfiguration_addAirProtocolInventoryCommandSettings(
      pAntConfig, (LLRP_tSParameter *)pInventoryCommand);

  /* Set antenna configuration to SET_READER_CONFIG */
  LLRP_SET_READER_CONFIG_addAntennaConfiguration(pCmd, pAntConfig);

  pCmdMsg       = &pCmd->hdr;

  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to get Gen2 Target value
 *
 * @param reader Reader pointer
 * @param target  Pointer to TMR_GEN2_Target object to hold the Gen2 target value
 */ 
TMR_Status
TMR_LLRP_cmdGetGen2Target(TMR_Reader *reader, TMR_GEN2_Target *target)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG                    *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE           *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration *pTMProtoConfig;
  LLRP_tSParameter                            *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/gen2/target" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMProtoConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMProtoConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., protocol configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMProtoConfig, 
      LLRP_ThingMagicControlConfiguration_ThingMagicProtocolConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMProtoConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMProtoConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract Gen2 target from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    LLRP_tSGen2CustomParameters *gen2Custom;
    LLRP_tSThingMagicTargetStrategy  *gen2target;

    gen2Custom = LLRP_ThingMagicProtocolConfiguration_getGen2CustomParameters(
        (LLRP_tSThingMagicProtocolConfiguration *)pCustParam);

    gen2target = LLRP_Gen2CustomParameters_getThingMagicTargetStrategy(gen2Custom);
    *target = (TMR_GEN2_Target)gen2target->eThingMagicTargetStrategyValue;
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Set Gen2 Target value
 *
 * @param reader Reader pointer
 * @param target Pointer to TMR_GEN2_Target which needs to be set
 */
TMR_Status
TMR_LLRP_cmdSetGen2Target(TMR_Reader *reader, TMR_GEN2_Target *target)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicProtocolConfiguration  *pProtoConfig;
  LLRP_tSGen2CustomParameters             *pGen2Custom;
  LLRP_tSThingMagicTargetStrategy         *gen2target;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic Protocol configuration is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Protocol configuration */
  pProtoConfig = LLRP_ThingMagicProtocolConfiguration_construct();

  /* Initialize gen2 Target */
  gen2target = LLRP_ThingMagicTargetStrategy_construct();
  gen2target->eThingMagicTargetStrategyValue = (LLRP_tEThingMagicC1G2TargetStrategy)*target;

  /* Initialize gen2 custom parameter */
  pGen2Custom = LLRP_Gen2CustomParameters_construct();
  LLRP_Gen2CustomParameters_setThingMagicTargetStrategy(pGen2Custom, gen2target);

  /* Set Gen2 Custom parameter to protocol configuration */
  LLRP_ThingMagicProtocolConfiguration_setGen2CustomParameters(pProtoConfig,
      pGen2Custom);

  /* Add ThingMagic Protocol configuration as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pProtoConfig);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Set  TMLicenseKey value
 *
 * @param reader Reader pointer
 * @param license Pointer to TMR_uint8List which needs to be set
 */
TMR_Status
TMR_LLRP_cmdSetTMLicenseKey(TMR_Reader *reader, TMR_uint8List *license)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicLicenseKey             *pTMLicense;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic  LicenseKey is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize ThingMagic LicenseKey  */
  pTMLicense = LLRP_ThingMagicLicenseKey_construct();

  /* Set the License key */
  llrp_u8v_t key;
  key = LLRP_u8v_construct(license->len);
  key.nValue = license->len;
  memcpy(key.pValue, license->list, license->len);

  LLRP_ThingMagicLicenseKey_setLicenseKey(pTMLicense, key);

  /* Add ThingMagic License key  as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pTMLicense);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Set  TMAsyncOfftime value
 *
 * @param reader Reader pointer
 * @param license Pointer to TMR_uint8List which needs to be set
   */
TMR_Status
TMR_LLRP_cmdSetTMAsyncOffTime(TMR_Reader *reader, uint32_t offtime)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicAsyncOFFTime           *pTMAsyncOffTime;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic  LicenseKey is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize ThingMagic LicenseKey  */
  pTMAsyncOffTime = LLRP_ThingMagicAsyncOFFTime_construct();

  /* Set the License key */

  LLRP_ThingMagicAsyncOFFTime_setAsyncOFFTime(pTMAsyncOffTime, (llrp_u32_t)offtime);

  /* Add ThingMagic License key  as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pTMAsyncOffTime);

  pCmdMsg = &pCmd->hdr;
 /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

#ifdef TMR_ENABLE_ISO180006B
/**
 * Command to get ISO 18K6B delimiter value
 *
 * @param reader Reader pointer
 * @param delimiter  Pointer to TMR_ISO180006B_Delimiter object to hold the ISO 18K6B delimiter value
 */ 
TMR_Status
TMR_LLRP_cmdGetISO18K6BDelimiter(TMR_Reader *reader, TMR_ISO180006B_Delimiter *delimiter)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG                    *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE           *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration *pTMProtoConfig;
  LLRP_tSParameter                            *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/iso180006b/delimiter" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMProtoConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMProtoConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., protocol configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMProtoConfig, 
      LLRP_ThingMagicControlConfiguration_ThingMagicProtocolConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMProtoConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMProtoConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract ISO 18K6B delimiter  from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    LLRP_tSISO18K6BCustomParameters *iso18k6bCustParam;
    LLRP_tSThingMagicISO180006BDelimiter  *iso18k6bDelimiter;

    iso18k6bCustParam = LLRP_ThingMagicProtocolConfiguration_getISO18K6BCustomParameters(
        (LLRP_tSThingMagicProtocolConfiguration *)pCustParam);

    if (NULL != iso18k6bCustParam) 
    {
      iso18k6bDelimiter = LLRP_ISO18K6BCustomParameters_getThingMagicISO180006BDelimiter(iso18k6bCustParam);
      *delimiter = (TMR_ISO180006B_Delimiter)iso18k6bDelimiter->eISO18K6BDelimiter;
    }
    else
    {
      return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
    }
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Set ISO18K6B delimiter value
 *
 * @param reader Reader pointer
 * @param delimiter Pointer to TMR_ISO180006B_Delimiter which needs to be set
 */
TMR_Status
TMR_LLRP_cmdSetISO18K6BDelimiter(TMR_Reader *reader, TMR_ISO180006B_Delimiter *delimiter)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicProtocolConfiguration  *pProtoConfig;
  LLRP_tSISO18K6BCustomParameters         *pISO18k6bCustParam;
  LLRP_tSThingMagicISO180006BDelimiter    *pISO18k6bDelimiter;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic Protocol configuration is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Protocol configuration */
  pProtoConfig = LLRP_ThingMagicProtocolConfiguration_construct();

  /* Initialize ISO 18K6B dedlimiter */
  pISO18k6bDelimiter = LLRP_ThingMagicISO180006BDelimiter_construct();
  pISO18k6bDelimiter->eISO18K6BDelimiter = (LLRP_tEThingMagicCustom18K6BDelimiter)*delimiter;

  /* Initialize ISO 18K6B custom parameter */
  pISO18k6bCustParam = LLRP_ISO18K6BCustomParameters_construct();
  LLRP_ISO18K6BCustomParameters_setThingMagicISO180006BDelimiter(pISO18k6bCustParam, pISO18k6bDelimiter);

  /* Set ISO 18K6B Custom parameter to protocol configuration */
  LLRP_ThingMagicProtocolConfiguration_setISO18K6BCustomParameters(pProtoConfig,
      pISO18k6bCustParam);

  /* Add ThingMagic Protocol configuration as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pProtoConfig);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to get ISO18K6B modulation Depth value
 *
 * @param reader Reader pointer
 * @param modDepth  Pointer to TMR_ISO180006B_ModulationDepth object to hold the ISO 18K6B modulation Depth value
 */ 
TMR_Status
TMR_LLRP_cmdGetISO18K6BModDepth(TMR_Reader *reader, TMR_ISO180006B_ModulationDepth *modDepth)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG                    *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE           *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration *pTMProtoConfig;
  LLRP_tSParameter                            *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/iso180006b/modulationDepth" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMProtoConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMProtoConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., protocol configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMProtoConfig, 
      LLRP_ThingMagicControlConfiguration_ThingMagicProtocolConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMProtoConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMProtoConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract ISO 18K6B modulation depth from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    LLRP_tSISO18K6BCustomParameters          *pISO18k6bCustParam;
    LLRP_tSThingMagicISO18K6BModulationDepth *pISO18k6bModDepth;

    pISO18k6bCustParam = LLRP_ThingMagicProtocolConfiguration_getISO18K6BCustomParameters(
        (LLRP_tSThingMagicProtocolConfiguration *)pCustParam);

    if (NULL != pISO18k6bCustParam) 
    {
      pISO18k6bModDepth = LLRP_ISO18K6BCustomParameters_getThingMagicISO18K6BModulationDepth(pISO18k6bCustParam);
      *modDepth = (TMR_ISO180006B_ModulationDepth)pISO18k6bModDepth->eISO18K6BModulationDepth;
    }
    else {
      return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
    }
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Set ISO18K6B Modulation Depth value
 *
 * @param reader Reader pointer
 * @param modDepth Pointer to TMR_ISO180006B_ModulationDepth which needs to be set
 */
TMR_Status
TMR_LLRP_cmdSetISO18K6BModDepth(TMR_Reader *reader, TMR_ISO180006B_ModulationDepth *modDepth)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicProtocolConfiguration  *pProtoConfig;
  LLRP_tSISO18K6BCustomParameters         *pISO18k6bCustParam;
  LLRP_tSThingMagicISO18K6BModulationDepth *pISO18k6bModDepth;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic Protocol configuration is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Protocol configuration */
  pProtoConfig = LLRP_ThingMagicProtocolConfiguration_construct();

  /* Initialize ISO 18K6B modulation Depth */
  pISO18k6bModDepth = LLRP_ThingMagicISO18K6BModulationDepth_construct();
  pISO18k6bModDepth->eISO18K6BModulationDepth = (LLRP_tEThingMagicCustom18K6BModulationDepth)*modDepth;

  /* Initialize ISO 18K6B custom parameter */
  pISO18k6bCustParam = LLRP_ISO18K6BCustomParameters_construct();
  LLRP_ISO18K6BCustomParameters_setThingMagicISO18K6BModulationDepth(pISO18k6bCustParam, pISO18k6bModDepth);

  /* Set ISO 18K6B Custom parameter to protocol configuration */
  LLRP_ThingMagicProtocolConfiguration_setISO18K6BCustomParameters(pProtoConfig,
      pISO18k6bCustParam);

  /* Add ThingMagic Protocol configuration as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pProtoConfig);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to get ISO18K6B link frequency value
 *
 * @param reader Reader pointer
 * @param linkFreq Pointer to TMR_ISO180006B_linkFrequency object to hold the ISO 18K6B linkfrequency value
 */ 
TMR_Status
TMR_LLRP_cmdGetISO18K6BLinkFrequency(TMR_Reader *reader, TMR_ISO180006B_LinkFrequency *linkFreq)
{
  TMR_Status ret;
  LLRP_tSGET_READER_CONFIG                    *pCmd;
  LLRP_tSMessage                              *pCmdMsg;
  LLRP_tSMessage                              *pRspMsg;
  LLRP_tSGET_READER_CONFIG_RESPONSE           *pRsp;
  LLRP_tSThingMagicDeviceControlConfiguration *pTMProtoConfig;
  LLRP_tSParameter                            *pCustParam;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_READER_CONFIG message
   **/
  pCmd = LLRP_GET_READER_CONFIG_construct();
  LLRP_GET_READER_CONFIG_setRequestedData(pCmd, LLRP_GetReaderConfigRequestedData_Identification);

  /**
   * "/reader/iso180006b/BLF" is a custom parameter. And is available as part of
   * ThingMagicDeviceControlConfiguration.
   * Initialize the custom parameter
   **/
  pTMProtoConfig = LLRP_ThingMagicDeviceControlConfiguration_construct();
  if (NULL == pTMProtoConfig)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  /**
   * Set the requested data (i.e., protocol configuration)
   * And add to GET_READER_CONFIG message.
   **/
  LLRP_ThingMagicDeviceControlConfiguration_setRequestedData(pTMProtoConfig, 
      LLRP_ThingMagicControlConfiguration_ThingMagicProtocolConfiguration);
  if (LLRP_RC_OK != LLRP_GET_READER_CONFIG_addCustom(pCmd, &pTMProtoConfig->hdr))
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pTMProtoConfig);
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  pCmdMsg       = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, extract Iso linkfreq from response
   **/
  pCustParam = LLRP_GET_READER_CONFIG_RESPONSE_beginCustom(pRsp);
  if (NULL != pCustParam)
  {
    LLRP_tSISO18K6BCustomParameters *iso18k6bCustParam;
    LLRP_tSThingMagicISO18K6BLinkFrequency  *iso18k6blinkFreq;

    iso18k6bCustParam = LLRP_ThingMagicProtocolConfiguration_getISO18K6BCustomParameters(
        (LLRP_tSThingMagicProtocolConfiguration *)pCustParam);

    if (NULL != iso18k6bCustParam) 
    {
      iso18k6blinkFreq =  LLRP_ISO18K6BCustomParameters_getThingMagicISO18K6BLinkFrequency(iso18k6bCustParam);
      *linkFreq = (TMR_ISO180006B_LinkFrequency)iso18k6blinkFreq->eISO18K6BLinkFrequency;
    }
    else
    {
      return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
    }
  }
  else
  {
    return TMR_ERROR_LLRP_MSG_PARSE_ERROR;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Set ISO18K6B linkFrequency value
 *
 * @param reader Reader pointer
 * @param linkFreq Pointer to TMR_ISO180006B_linkFrequency which needs to be set
 */
TMR_Status
TMR_LLRP_cmdSetISO18K6BLinkFrequency(TMR_Reader *reader, TMR_ISO180006B_LinkFrequency *linkFreq)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSThingMagicProtocolConfiguration  *pProtoConfig;
  LLRP_tSISO18K6BCustomParameters         *pISO18k6bCustParam;
  LLRP_tSThingMagicISO18K6BLinkFrequency  *pISO18k6blinkFreq;

  ret = TMR_SUCCESS;
  /**
   * Thingmagic Protocol configuration is available as a custom parameter
   * and can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Protocol configuration */
  pProtoConfig = LLRP_ThingMagicProtocolConfiguration_construct();

  /* Initialize ISO 18K6B linkFrequency */
  pISO18k6blinkFreq = LLRP_ThingMagicISO18K6BLinkFrequency_construct();
  pISO18k6blinkFreq->eISO18K6BLinkFrequency = (LLRP_tEThingMagicCustom18K6BLinkFrequency)*linkFreq;

  /* Initialize ISO 18K6B custom parameter */
  pISO18k6bCustParam = LLRP_ISO18K6BCustomParameters_construct();
  LLRP_ISO18K6BCustomParameters_setThingMagicISO18K6BLinkFrequency(pISO18k6bCustParam, pISO18k6blinkFreq);

  /* Set ISO 18K6B Custom parameter to protocol configuration */
  LLRP_ThingMagicProtocolConfiguration_setISO18K6BCustomParameters(pProtoConfig,
      pISO18k6bCustParam);

  /* Add ThingMagic Protocol configuration as a custom parameter*/
  LLRP_SET_READER_CONFIG_addCustom(pCmd, (LLRP_tSParameter *)pProtoConfig);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}
#endif /* TMR_ENABLE_ISO180006B */

TMR_Status
TMR_LLRP_stopActiveROSpecs(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSGET_ROSPECS                    *pCmd;
  LLRP_tSMessage                        *pCmdMsg;
  LLRP_tSMessage                        *pRspMsg;
  LLRP_tSGET_ROSPECS_RESPONSE           *pRsp;
  LLRP_tSROSpec                         *pROSpec;

  ret = TMR_SUCCESS;
  /**
   * Initialize the GET_ROSPECS message
   **/
  pCmd = LLRP_GET_ROSPECS_construct();
  pCmdMsg = &pCmd->hdr;
  
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSGET_ROSPECS_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus)) 
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Response is success, extract list of ROSpecs from response
   **/
  for (pROSpec = LLRP_GET_ROSPECS_RESPONSE_beginROSpec(pRsp);
        NULL != pROSpec;
        pROSpec = LLRP_GET_ROSPECS_RESPONSE_nextROSpec(pROSpec))
  {
    /* If this ROSpec is actively running, then stop it */
    if (LLRP_ROSpecState_Active == pROSpec->eCurrentState)
    {
      reader->u.llrpReader.roSpecId = pROSpec->ROSpecID;
      ret = TMR_LLRP_cmdStopROSpec(reader, true);
    }
    if (LLRP_ROSpecStartTriggerType_Periodic ==
        pROSpec->pROBoundarySpec->pROSpecStartTrigger->eROSpecStartTriggerType)
    {
      reader->u.llrpReader.roSpecId = pROSpec->ROSpecID;
      ret = TMR_LLRP_cmdDisableROSpec(reader);
    }
  }

  /* Revert back to default value */
  reader->u.llrpReader.roSpecId = 0;
  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

TMR_Status
TMR_LLRP_enableEventsAndReports(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSENABLE_EVENTS_AND_REPORTS *pCmd;
  LLRP_tSMessage                   *pCmdMsg;

  ret = TMR_SUCCESS;

  /**
   * Initialize ENABLE_EVENTS_AND_REPORTS message
   **/
  pCmd = LLRP_ENABLE_EVENTS_AND_REPORTS_construct();
  pCmdMsg = &pCmd->hdr;
 
  /**
   * For LLRP_ENABLE_EVENTS_AND_REPORTS message, there
   * will be no response. So just send the message
   **/
  ret = TMR_LLRP_sendMessage(reader, pCmdMsg, 
          reader->u.llrpReader.transportTimeout);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  return ret;
}

TMR_Status
TMR_LLRP_setHoldEventsAndReportsStatus(TMR_Reader *reader, llrp_u1_t status)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSEventsAndReports                 *pEvents;

  ret = TMR_SUCCESS;
  /**
   * EventsAndReports can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize EventsAndReports  */
  pEvents = LLRP_EventsAndReports_construct();
  LLRP_EventsAndReports_setHoldEventsAndReportsUponReconnect(pEvents, status); 


  /* Add EventsAndReports to SET_READER_CONFIG*/
  LLRP_SET_READER_CONFIG_setEventsAndReports(pCmd, pEvents);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus)) 
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}


TMR_Status
TMR_LLRP_setKeepAlive(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSKeepaliveSpec                    *pKeepAlive;

  ret = TMR_SUCCESS;
  /**
   * Keep alive can be set through SET_READER_CONFIG
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Keep Alive Spec  */
  pKeepAlive = LLRP_KeepaliveSpec_construct();
  LLRP_KeepaliveSpec_setKeepaliveTriggerType(pKeepAlive, LLRP_KeepaliveTriggerType_Periodic);
  LLRP_KeepaliveSpec_setPeriodicTriggerValue(pKeepAlive, TMR_LLRP_KEEP_ALIVE_TIMEOUT);


  /* Add KeepaliveSpec to SET_READER_CONFIG*/
  LLRP_SET_READER_CONFIG_setKeepaliveSpec(pCmd, pKeepAlive);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus)) 
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  /**
   * Start LLRP background receiver
   **/
  return TMR_LLRP_startBackgroundReceiver(reader);
}

TMR_Status
TMR_LLRP_handleReaderEvents(TMR_Reader *reader, LLRP_tSMessage *pMsg)
{
  TMR_Status ret;
  LLRP_tSReaderEventNotificationData *pEventData;
  
  ret = TMR_SUCCESS;
  pEventData = ((LLRP_tSREADER_EVENT_NOTIFICATION *)pMsg)->pReaderEventNotificationData;

  /**
   * Currently we care only for ROSpec event
   **/
  if (NULL != pEventData->pROSpecEvent)
  {
    /**
     * Check for ROSpec event type.
     * We need End of ROSpec event only for sync reads.
     **/
    if ((LLRP_ROSpecEventType_End_Of_ROSpec ==
        pEventData->pROSpecEvent->eEventType) &&
        (false == reader->continuousReading))
    {
      /**
       * If end of rospec, then decrement
       * numROSpecEvents...
       **/
      pthread_mutex_lock(&reader->u.llrpReader.receiverLock);
      reader->u.llrpReader.numOfROSpecEvents --;
      if(reader->u.llrpReader.numOfROSpecEvents <= 0 ) {
        pthread_cond_broadcast(&reader->u.llrpReader.receiverCond);
      }
      pthread_mutex_unlock(&reader->u.llrpReader.receiverLock);
    }
  }

  /**
   * Place for handling other events.
   * For future use.
   **/

  /**
   * Done with reader event notification message.
   * Free it.
   **/
  TMR_LLRP_freeMessage(pMsg);
  return ret;
}

TMR_Status
TMR_LLRP_handleKeepAlive(TMR_Reader *reader, LLRP_tSMessage *pMsg)
{
  TMR_Status ret;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSKEEPALIVE_ACK                    *pAck;

  ret = TMR_SUCCESS;

  /**
   * Free keepalive message, assuming the message type
   * is keepalivespec.
   **/
  TMR_LLRP_freeMessage(pMsg);

  /**
   * Send keep alive acknowledgement
   **/
  pAck = LLRP_KEEPALIVE_ACK_construct();
  pCmdMsg = &pAck->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_sendMessage(reader, pCmdMsg,
                reader->u.llrpReader.transportTimeout);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /*
   * Response is success, and done with the command
   */
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pAck);

  return TMR_SUCCESS;
}

static void *
llrp_receiver_thread(void *arg)
{
  TMR_Status ret;
  TMR_Reader *reader;
  TMR_LLRP_LlrpReader *lr;
  LLRP_tSMessage *pMsg;
  LLRP_tSConnection *pConn;
  struct timeval tv;
  fd_set set;
  bool ka_start_flag = true;
  bool receive_failed = false;

  ret = TMR_SUCCESS;
  reader = arg;
  lr = &reader->u.llrpReader;

  /**
   * You are going to kill me, if you set runInBackground to false.
   * Please be kind enough !!
   **/
  while (true)
  {
    pConn = reader->u.llrpReader.pConn;
    if (NULL != pConn)
    {
      pthread_mutex_lock(&lr->receiverLock);
      lr->receiverRunning = false;
      pthread_cond_broadcast(&lr->receiverCond);
      while (false == lr->receiverEnabled)
      {
        pthread_cond_wait(&lr->receiverCond, &lr->receiverLock);
      }
    
      lr->receiverRunning = true;
      pthread_mutex_unlock(&lr->receiverLock);

      FD_ZERO(&set);
      FD_SET(pConn->fd, &set);
      tv.tv_sec = 0;
      tv.tv_usec = BACKGROUND_RECEIVER_LOOP_PERIOD * 1000;
      ret = select(pConn->fd + 1, &set, NULL, NULL, &tv);
      if (0 < ret)
      {
        /* check for new message in Inbox */
        ret = TMR_LLRP_receiveMessage(reader, &pMsg, lr->transportTimeout);
        if (TMR_SUCCESS != ret)
        {
          /**
           * If not success, then could be that no message
           * has been arrived yet, lets wait for some more time.
           * Do nothing here
           **/
          receive_failed = true;
        }
        else
        {
          /**
           * Ahaa!! New message has arrived
           * process received Message.
           **/
          ret = TMR_LLRP_processReceivedMessage(reader, pMsg);
          ka_start_flag = true;
          receive_failed = false;
        }
      }
    }
    else
    {
      receive_failed = true;
      /* We shouldn't be in here if pConn is NULL, but if it is sleep for a little bit and hopefully things will get better */
      tmr_sleep(1000);
    }

    if(true == receive_failed)
    {
      /**
       * The select call has failed. Could be that there is no data
       * to read because of connection problem. Wait to see if
       * the connection recovers back.
       **/
      uint64_t diffTime;

      if (ka_start_flag)
      {
        reader->u.llrpReader.ka_start = tmr_gettime();
        ka_start_flag = false;
      }
      reader->u.llrpReader.ka_now = tmr_gettime();
      diffTime = reader->u.llrpReader.ka_now - reader->u.llrpReader.ka_start;
      if ((TMR_LLRP_KEEP_ALIVE_TIMEOUT * 4) < diffTime)
      {
        /**
         * We have waited for enough time (4 times keep alive duration), 
         * and still there is no response from reader. 
         * Connection might be lost. Indicate an error so that the
         * continuous reading will be stopped.
         **/

        /**
         * Set numOfROSpec events to -1, indicating an unknown error
         * occured during the read process.
         **/
        pthread_mutex_lock(&lr->receiverLock);
        lr->numOfROSpecEvents = -1;
        pthread_cond_broadcast(&lr->receiverCond);
        pthread_mutex_unlock(&lr->receiverLock);
        ka_start_flag = true;
      }
    }

    if (true == reader->u.llrpReader.threadCancel)
    {
      /** Time to exit */
      pthread_exit(NULL);
    }
  } /* End of while */
  return NULL;
}

TMR_Status
TMR_LLRP_processReceivedMessage(TMR_Reader *reader, LLRP_tSMessage *pMsg)
{
  TMR_Status ret;
  TMR_LLRP_LlrpReader *lr;

  ret = TMR_SUCCESS;
  lr = &reader->u.llrpReader;

  /* Check if it is a keepalive */
  if (&LLRP_tdKEEPALIVE == pMsg->elementHdr.pType)
  {
    /**
     * Handle keep alive messages, no need
     * for error checking.
     **/
    ret = TMR_LLRP_handleKeepAlive(reader, pMsg);
  }

  /**
   * Check if it is a RO_ACCESS_REPORT
   **/
  else if (&LLRP_tdRO_ACCESS_REPORT == pMsg->elementHdr.pType)
  {
    /**
     * Handle RO_ACCESS_REPORTS,
     * We receive RO_ACCESS_REPORTS here only incase of sync read.
     * Buffer the message pointer, so that it can be used later.
     **/
    if (NULL == lr->bufResponse)
    {
      /* We haven't had opportunity to allocate the buffer yet, so allocate the
       * first
       */
      lr->bufResponse = (LLRP_tSMessage **) malloc( 1 * sizeof(LLRP_tSMessage *));
      lr->bufPointer = 0;
    }
    lr->bufResponse[lr->bufPointer ] = pMsg;

    /**
     * Reallocate the memory for bufResponse, so that it can hold the
     * pointer to next RO_ACCESS_REPORT in case if it is received.
     **/
    {
      LLRP_tSMessage **newResponse;

      newResponse = realloc(lr->bufResponse, (lr->bufPointer + 2) * sizeof(lr->bufResponse[0]));
      if (NULL == newResponse)
      {
        /**
         * FIXME: How to handle this error. This would'nt happen
         * in general, but can expect when running on reader.
         * How to handle this.
         **/
      }
      lr->bufResponse = newResponse;
    }

    /**
     * Do not free pMsg here. We hold that memory for further
     * processing of tagReads, and will be freed later.
     **/
    lr->bufPointer += 1;
  }

  /**
   * Check if it is a ReaderEventNotification
   **/
  else if (&LLRP_tdREADER_EVENT_NOTIFICATION  == pMsg->elementHdr.pType)
  {
    /**
     * Handle Reader event notifications.
     **/
    ret = TMR_LLRP_handleReaderEvents(reader, pMsg);
  }

  else
  {
    /**
     * What kind of message is this. I do not
     * know how to handle, ignoring for now
     * and free the message.
     **/
    TMR_LLRP_freeMessage(pMsg);
  }

  return ret;
}

void
TMR_LLRP_setBackgroundReceiverState(TMR_Reader *reader, bool state)
{
  if (true == reader->u.llrpReader.receiverSetup)
  {
    if (false == state)
    {
      /**
       * Disable background receiving
       **/
      pthread_mutex_lock(&reader->u.llrpReader.receiverLock);
      reader->u.llrpReader.receiverEnabled = false;
      while (true == reader->u.llrpReader.receiverRunning)
      {
        pthread_cond_wait(&reader->u.llrpReader.receiverCond, &reader->u.llrpReader.receiverLock);
      }
      pthread_mutex_unlock(&reader->u.llrpReader.receiverLock);
    }
    else
    {
      /**
       * Enable background receiving
       **/
      pthread_mutex_lock(&reader->u.llrpReader.receiverLock);
      reader->u.llrpReader.receiverEnabled = true;
      pthread_cond_broadcast(&reader->u.llrpReader.receiverCond);
      pthread_mutex_unlock(&reader->u.llrpReader.receiverLock);
    }
  }
}

TMR_Status
TMR_LLRP_startBackgroundReceiver(TMR_Reader *reader)
{
  TMR_Status ret;
  TMR_LLRP_LlrpReader *lr = &reader->u.llrpReader;

  ret = TMR_SUCCESS;

  /* Initialize background llrp receiver */
  pthread_mutex_lock(&lr->receiverLock);

  ret = pthread_create(&lr->llrpReceiver, NULL,
                      llrp_receiver_thread, reader);
  if (0 != ret)
  {
    pthread_mutex_unlock(&lr->receiverLock);
    return TMR_ERROR_NO_THREADS;
  }
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  
  lr->receiverSetup = true;
  lr->receiverEnabled = true;
  pthread_mutex_unlock(&lr->receiverLock);

  return TMR_SUCCESS;
}

TMR_Status
TMR_LLRP_cmdSetEventNotificationSpec(TMR_Reader *reader, bool state)
{
  TMR_Status ret;
  LLRP_tSSET_READER_CONFIG                *pCmd;
  LLRP_tSMessage                          *pCmdMsg;
  LLRP_tSMessage                          *pRspMsg;
  LLRP_tSSET_READER_CONFIG_RESPONSE       *pRsp;
  LLRP_tSReaderEventNotificationSpec      *pEventNotificationSpec;
  LLRP_tSEventNotificationState           *pEventNotificationState;

  ret = TMR_SUCCESS;
  /**
   * Reader event notifications can be set through
   * Initialize SET_READER_CONFIG
   **/
  pCmd = LLRP_SET_READER_CONFIG_construct();

  /* Initialize Reader Event Notification spec */
  pEventNotificationSpec = LLRP_ReaderEventNotificationSpec_construct();

  /**
   * Initialize events
   * Currently we are using only ROSPEC event
   **/
  pEventNotificationState = LLRP_EventNotificationState_construct();
  LLRP_EventNotificationState_setEventType(pEventNotificationState,
                                   LLRP_NotificationEventType_ROSpec_Event);
  LLRP_EventNotificationState_setNotificationState(pEventNotificationState, (uint8_t)state);

  /* Set Event to EventNotificationSpec */
  LLRP_ReaderEventNotificationSpec_addEventNotificationState(pEventNotificationSpec,
                                                    pEventNotificationState);


  /* Add EventNotificationSpec to SET_READER_CONFIG*/
  LLRP_SET_READER_CONFIG_setReaderEventNotificationSpec(pCmd, pEventNotificationSpec);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSSET_READER_CONFIG_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP;
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);
  return ret;
}

/**
 * Command to Enable AccessSpec
 *
 * @param reader Reader pointer
 * @param accessSpecId AccessSpec Id
 */
TMR_Status
TMR_LLRP_cmdEnableAccessSpec(TMR_Reader *reader, llrp_u32_t accessSpecId)
{
  TMR_Status ret;
  LLRP_tSENABLE_ACCESSSPEC          *pCmd;
  LLRP_tSMessage                    *pCmdMsg;
  LLRP_tSMessage                    *pRspMsg;
  LLRP_tSENABLE_ACCESSSPEC_RESPONSE *pRsp;
  
  ret = TMR_SUCCESS;

  /**
   * Initialize EnableAccessSpec message
   **/
  pCmd = LLRP_ENABLE_ACCESSSPEC_construct();
  LLRP_ENABLE_ACCESSSPEC_setAccessSpecID(pCmd, accessSpecId);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSENABLE_ACCESSSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Prepare AccessCommand
 *
 * @param reader Reader pointer
 * @param pAccessCommand Pointer to AccessCommand parameter
 * @param filter Pointer to Tag filter
 * @param tagop Pointer to TMR_TagOp
 */
TMR_Status
TMR_LLRP_msgPrepareAccessCommand(TMR_Reader *reader,
                                  LLRP_tSAccessCommand *pAccessCommand, 
                                  TMR_TagFilter *filter, 
                                  TMR_TagOp *tagop)
{
  TMR_Status ret;

  ret = TMR_SUCCESS;

  /**
   * 1. Prepare and Add TagSpec
   **/
  {
    LLRP_tSC1G2TagSpec *pTagSpec;
    LLRP_tSC1G2TargetTag *pTargetTag;

    /* Initialize TagSpec */
    pTagSpec = LLRP_C1G2TagSpec_construct();
    pTargetTag = LLRP_C1G2TargetTag_construct();
  
    /**
     * TagFilter is not supported as part of AccessSpec's TagSpec.
     * So nothing to do with the filter passed in.
     * All we do is construct a dummy TagSpec and set as a 
     * TargetTag.
     **/
    
    /* Add TargetTag to TagSpec */
    LLRP_C1G2TagSpec_addC1G2TargetTag(pTagSpec, pTargetTag);

    /**
     * Add TagSpec to AccessSpec
     **/
    LLRP_AccessCommand_setAirProtocolTagSpec(pAccessCommand,
                                              (LLRP_tSParameter *)pTagSpec);
  }

  /**
   * 2. Prepare and Add OpSpec
   **/
  {
    reader->u.llrpReader.opSpecId ++;

    /**
     * Check for the tagop type and add appropriate
     * OpSpec.
     **/
    switch (tagop->type)
    {
      case TMR_TAGOP_GEN2_READDATA:
        {
          TMR_TagOp_GEN2_ReadData *args;
          LLRP_tSC1G2Read         *pC1G2Read;

          args = &tagop->u.gen2.u.readData;
          
          /* Construct and initialize C1G2Read */
          pC1G2Read = LLRP_C1G2Read_construct();
          /* Set OpSpec Id */
          LLRP_C1G2Read_setOpSpecID(pC1G2Read, reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_C1G2Read_setAccessPassword(pC1G2Read, reader->u.llrpReader.gen2AccessPassword);
          /* Set Memory Bank */
          LLRP_C1G2Read_setMB(pC1G2Read, (llrp_u2_t)args->bank);
          /* Set word pointer */
          LLRP_C1G2Read_setWordPointer(pC1G2Read, args->wordAddress);
          /* Set word length to read */
          LLRP_C1G2Read_setWordCount(pC1G2Read, args->len);

         /**
          * Set C1G2Read as OpSpec to accessSpec
          **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand, 
                                          (LLRP_tSParameter *)pC1G2Read); 
          break;
        }

      case TMR_TAGOP_GEN2_BLOCKERASE:
        {
          TMR_TagOp_GEN2_BlockErase *args;
          LLRP_tSC1G2BlockErase     *pC1G2BlockErase;

          args = &tagop->u.gen2.u.blockErase;

          /* Construct and initialize C1G2BlockErase */
          pC1G2BlockErase = LLRP_C1G2BlockErase_construct();
          /* Set OpSpec Id */
          LLRP_C1G2BlockErase_setOpSpecID(pC1G2BlockErase, reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_C1G2BlockErase_setAccessPassword(pC1G2BlockErase, reader->u.llrpReader.gen2AccessPassword);
          /* Set Memory Bank */
          LLRP_C1G2BlockErase_setMB(pC1G2BlockErase, (llrp_u2_t)args->bank);
          /* Set word pointer */
          LLRP_C1G2BlockErase_setWordPointer(pC1G2BlockErase, args->wordPtr);
          /* Set word count to erase */
          LLRP_C1G2BlockErase_setWordCount(pC1G2BlockErase, (llrp_u16_t)args->wordCount);

          /**
           * Set C1G2BlockErase as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pC1G2BlockErase);
          break;
        }

      case TMR_TAGOP_GEN2_WRITETAG:
        {
          TMR_TagOp_GEN2_WriteTag     *args;
          LLRP_tSThingMagicWriteTag   *pC1G2WriteTag;
          llrp_u16v_t                  data;
          int                          i,j;

          args = &tagop->u.gen2.u.writeTag;

          /* Construct and initialize ThingMagicWriteTag */
          pC1G2WriteTag = LLRP_ThingMagicWriteTag_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicWriteTag_setOpSpecID(pC1G2WriteTag, reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_ThingMagicWriteTag_setAccessPassword(pC1G2WriteTag, reader->u.llrpReader.gen2AccessPassword);
          /* Set the data to be written */
          /* As API epc datatype is uint8_t but
           * llrp takes epc of type uint16_t
           * so, mapping the uint8_t data into uint16_t
           */
          data = LLRP_u16v_construct((llrp_u16_t) (args->epcptr->epcByteCount / 2));
          for(i = 0, j = 0;i < data.nValue; i++, j+=2)
          {
            data.pValue[i] = (args->epcptr->epc[j] << 8) | args->epcptr->epc[j + 1];
          }

          LLRP_ThingMagicWriteTag_setWriteData (pC1G2WriteTag, data);

          /**
           * Set ThingMagicWriteTag as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pC1G2WriteTag);
         
          break;
        }

      case TMR_TAGOP_GEN2_WRITEDATA:
        {
          TMR_TagOp_GEN2_WriteData *args;
          LLRP_tSC1G2Write         *pC1G2WriteData;
          llrp_u16v_t               data;

          args = &tagop->u.gen2.u.writeData;

          /* Construct and initialize C1GWrite */
          pC1G2WriteData = LLRP_C1G2Write_construct();
          /* Set OpSpec Id */
          LLRP_C1G2Write_setOpSpecID(pC1G2WriteData, reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_C1G2Write_setAccessPassword(pC1G2WriteData, reader->u.llrpReader.gen2AccessPassword);
          /* Set Memory Bank */
          LLRP_C1G2Write_setMB(pC1G2WriteData, (llrp_u2_t)args->bank);
          /* Set word pointer */
          LLRP_C1G2Write_setWordPointer(pC1G2WriteData, args->wordAddress);
          /* Set the data to be written */
          data = LLRP_u16v_construct (args->data.len);
          memcpy (data.pValue, args->data.list, data.nValue * sizeof(uint16_t));
          LLRP_C1G2Write_setWriteData (pC1G2WriteData, data);

          /**
           * Set C1G2WriteData as OpSpec to accessSpec
           **/
          
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pC1G2WriteData);
         
          break;
        }

      case TMR_TAGOP_GEN2_BLOCKPERMALOCK:
        {
          TMR_TagOp_GEN2_BlockPermaLock      *args;
          LLRP_tSThingMagicBlockPermalock    *pC1G2BlockPermaLock;
          llrp_u16v_t                        mask;

          args = &tagop->u.gen2.u.blockPermaLock;

          /* Construct and initialize BlockPermaLock */
          pC1G2BlockPermaLock = LLRP_ThingMagicBlockPermalock_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicBlockPermalock_setOpSpecID(pC1G2BlockPermaLock, 
                                            reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_ThingMagicBlockPermalock_setAccessPassword(pC1G2BlockPermaLock,
                                      reader->u.llrpReader.gen2AccessPassword);
          /* Set ReadLock */
          LLRP_ThingMagicBlockPermalock_setReadLock(pC1G2BlockPermaLock, args->readLock);
          /* Set Memory Bank */
          LLRP_ThingMagicBlockPermalock_setMB(pC1G2BlockPermaLock, (llrp_u2_t)args->bank);
          /* Set block pointer */
          LLRP_ThingMagicBlockPermalock_setBlockPointer(pC1G2BlockPermaLock, 
                                                            args->blockPtr);
          /* Set block range mask */
          mask = LLRP_u16v_construct(args->mask.len);
          memcpy(mask.pValue, args->mask.list, mask.nValue * sizeof(uint16_t));
          LLRP_ThingMagicBlockPermalock_setBlockMask(pC1G2BlockPermaLock, mask);

          /**
           * Set BlockPermaLock as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pC1G2BlockPermaLock);
 
          break;
        }

      case TMR_TAGOP_GEN2_ALIEN_HIGGS2_PARTIALLOADIMAGE:
        {
          TMR_TagOp_GEN2_Alien_Higgs2_PartialLoadImage *args;
          LLRP_tSThingMagicHiggs2PartialLoadImage      *pHiggs2PartialLoadImage;
          llrp_u8v_t                                    epc;

          args = &tagop->u.gen2.u.custom.u.alien.u.higgs2.u.partialLoadImage;

          /**
           * Error check
           **/
          if ((args->epcptr->epcByteCount > 12) || 
              (args->epcptr->epcByteCount <=0))
          { 
            /* Only 96 bit epc */
            return TMR_ERROR_PROTOCOL_INVALID_EPC;
          }    

          /* Construct and initialize Higgs2PartialLoadImage */
          pHiggs2PartialLoadImage = LLRP_ThingMagicHiggs2PartialLoadImage_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicHiggs2PartialLoadImage_setOpSpecID(pHiggs2PartialLoadImage, 
                                                      reader->u.llrpReader.opSpecId);
          /* Set access password to use to write on tag */
          LLRP_ThingMagicHiggs2PartialLoadImage_setCurrentAccessPassword(
              pHiggs2PartialLoadImage, reader->u.llrpReader.gen2AccessPassword);
          /* Set Kill Password to be written on tag */
          LLRP_ThingMagicHiggs2PartialLoadImage_setKillPassword(pHiggs2PartialLoadImage,
                                                                    args->killPassword);
          /* Set access password to be written on tag */
          LLRP_ThingMagicHiggs2PartialLoadImage_setAccessPassword(pHiggs2PartialLoadImage,
                                                                    args->accessPassword);
          /* Set EPC to be written */
          epc = LLRP_u8v_construct (args->epcptr->epcByteCount * sizeof(uint8_t));
          memcpy (epc.pValue, args->epcptr->epc, epc.nValue);
          LLRP_ThingMagicHiggs2PartialLoadImage_setEPCData(pHiggs2PartialLoadImage, epc);

          /**
           * Set Higgs2PartialLoadImage as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pHiggs2PartialLoadImage);
 
          break;
        }

      case TMR_TAGOP_GEN2_ALIEN_HIGGS2_FULLLOADIMAGE:
        {
          TMR_TagOp_GEN2_Alien_Higgs2_FullLoadImage *args;
          LLRP_tSThingMagicHiggs2FullLoadImage      *pOpSpec;
          llrp_u8v_t                                epc;

          args = &tagop->u.gen2.u.custom.u.alien.u.higgs2.u.fullLoadImage;

          /**
           * Error check
           **/
          if ((args->epcptr->epcByteCount > 12) || 
              (args->epcptr->epcByteCount <=0))
          { 
            /* Only 96 bit epc */
            return TMR_ERROR_PROTOCOL_INVALID_EPC;
          }    

          /* Construct and initialize Higgs2FullLoadImage */
          pOpSpec = LLRP_ThingMagicHiggs2FullLoadImage_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicHiggs2FullLoadImage_setOpSpecID(pOpSpec, 
                                  reader->u.llrpReader.opSpecId);
          /* Set access password to use to write on tag */
          LLRP_ThingMagicHiggs2FullLoadImage_setCurrentAccessPassword(
                      pOpSpec, reader->u.llrpReader.gen2AccessPassword);
          /* Set Kill Password to be written on tag */
          LLRP_ThingMagicHiggs2FullLoadImage_setKillPassword(pOpSpec, args->killPassword);
          /* Set access password to be written on tag */
          LLRP_ThingMagicHiggs2FullLoadImage_setAccessPassword(pOpSpec, args->accessPassword);
          /* Set LockBits */
          LLRP_ThingMagicHiggs2FullLoadImage_setLockBits(pOpSpec, args->lockBits);
          /* Set PCWord */
          LLRP_ThingMagicHiggs2FullLoadImage_setPCWord(pOpSpec, args->pcWord);
          /* Set EPC to be written */
          epc = LLRP_u8v_construct (args->epcptr->epcByteCount * sizeof(uint8_t));
          memcpy (epc.pValue, args->epcptr->epc, epc.nValue);
          LLRP_ThingMagicHiggs2FullLoadImage_setEPCData(pOpSpec, epc);

          /**
           * Set Higgs2FullLoadImage as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pOpSpec);
 
          break;
        }

      case TMR_TAGOP_GEN2_ALIEN_HIGGS3_FASTLOADIMAGE:
        {
          TMR_TagOp_GEN2_Alien_Higgs3_FastLoadImage *args;
          LLRP_tSThingMagicHiggs3FastLoadImage      *pOpSpec;
          llrp_u8v_t                                epc;

          args = &tagop->u.gen2.u.custom.u.alien.u.higgs3.u.fastLoadImage;

          /**
           * Error check
           **/
          if ((args->epcptr->epcByteCount > 12) || 
              (args->epcptr->epcByteCount <=0))
          { 
            /* Only 96 bit epc */
            return TMR_ERROR_PROTOCOL_INVALID_EPC;
          }    

          /* Construct and initialize Higgs3FastLoadImage */
          pOpSpec = LLRP_ThingMagicHiggs3FastLoadImage_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicHiggs3FastLoadImage_setOpSpecID(pOpSpec, 
                                  reader->u.llrpReader.opSpecId);
          /* Set access password to use to write on tag */
          LLRP_ThingMagicHiggs3FastLoadImage_setCurrentAccessPassword(
                                  pOpSpec, args->currentAccessPassword);
          /* Set Kill Password to be written on tag */
          LLRP_ThingMagicHiggs3FastLoadImage_setKillPassword(pOpSpec, args->killPassword);
          /* Set access password to be written on tag */
          LLRP_ThingMagicHiggs3FastLoadImage_setAccessPassword(pOpSpec, args->accessPassword);
          /* Set PCWord */
          LLRP_ThingMagicHiggs3FastLoadImage_setPCWord(pOpSpec, args->pcWord);
          /* Set EPC to be written */
          epc = LLRP_u8v_construct (args->epcptr->epcByteCount * sizeof(uint8_t));
          memcpy (epc.pValue, args->epcptr->epc, epc.nValue);
          LLRP_ThingMagicHiggs3FastLoadImage_setEPCData(pOpSpec, epc);

          /**
           * Set Higgs3FastLoadImage as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pOpSpec);
 
          break;
        }

      case TMR_TAGOP_GEN2_ALIEN_HIGGS3_LOADIMAGE:
        {
          TMR_TagOp_GEN2_Alien_Higgs3_LoadImage *args;
          LLRP_tSThingMagicHiggs3LoadImage      *pOpSpec;
          llrp_u8v_t                            epcAndUserData;

          args = &tagop->u.gen2.u.custom.u.alien.u.higgs3.u.loadImage;

          /**
           * Error Check
           **/
          if ((args->epcAndUserData->len > 76) || 
              (args->epcAndUserData->len <= 0))                                                                                                 
          { 
            /* Only 76 byte epcAndUserData */
            return TMR_ERROR_MSG_INVALID_PARAMETER_VALUE;
          }

          /* Construct and initialize Higgs3LoadImage */
          pOpSpec = LLRP_ThingMagicHiggs3LoadImage_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicHiggs3LoadImage_setOpSpecID(pOpSpec, 
                               reader->u.llrpReader.opSpecId);
          /* Set access password to use to write on tag */
          LLRP_ThingMagicHiggs3LoadImage_setCurrentAccessPassword(
                              pOpSpec, args->currentAccessPassword);
          /* Set Kill Password to be written on tag */
          LLRP_ThingMagicHiggs3LoadImage_setKillPassword(pOpSpec, args->killPassword);
          /* Set access password to be written on tag */
          LLRP_ThingMagicHiggs3LoadImage_setAccessPassword(pOpSpec, args->accessPassword);
          /* Set PCWord */
          LLRP_ThingMagicHiggs3LoadImage_setPCWord(pOpSpec, args->pcWord);
          /* Set EPC And User data to be written */
          epcAndUserData = LLRP_u8v_construct (args->epcAndUserData->len);
          memcpy (epcAndUserData.pValue, args->epcAndUserData->list,
                                 epcAndUserData.nValue);
          LLRP_ThingMagicHiggs3LoadImage_setEPCAndUserData(pOpSpec, epcAndUserData);

          /**
           * Set Higgs3LoadImage as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pOpSpec);
 
          break;
        }

      case TMR_TAGOP_GEN2_ALIEN_HIGGS3_BLOCKREADLOCK:
        {
          TMR_TagOp_GEN2_Alien_Higgs3_BlockReadLock *args;
          LLRP_tSThingMagicHiggs3BlockReadLock      *pOpSpec;

          args = &tagop->u.gen2.u.custom.u.alien.u.higgs3.u.blockReadLock;

          /* Construct and initialize Higgs3BlockReadLock */
          pOpSpec = LLRP_ThingMagicHiggs3BlockReadLock_construct();
          /* Set OpSpec Id */
          LLRP_ThingMagicHiggs3BlockReadLock_setOpSpecID(pOpSpec, 
                                  reader->u.llrpReader.opSpecId);
          /* Set AccessPassword */
          LLRP_ThingMagicHiggs3BlockReadLock_setAccessPassword(
                                  pOpSpec, args->accessPassword);
          /* Set LockBits */
          LLRP_ThingMagicHiggs3BlockReadLock_setLockBits(
                                  pOpSpec, args->lockBits);

          /**
           * Set Higgs3BlockReadLock as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pOpSpec);
 
          break;
        }

      case TMR_TAGOP_GEN2_LOCK:
        {
          TMR_TagOp_GEN2_Lock        *args;
          LLRP_tSC1G2Lock            *pC1G2Lock;
          LLRP_tSC1G2LockPayload     *pC1G2LockPayload;
          int                         index;

          args = &tagop->u.gen2.u.lock;
          /* Construct and initialize C1G2Lock */
          pC1G2Lock = LLRP_C1G2Lock_construct();
          /* Set OpSpec Id */
          LLRP_C1G2Lock_setOpSpecID(pC1G2Lock, reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_C1G2Lock_setAccessPassword(pC1G2Lock, args->accessPassword);
          /* Construct and initialize C1G2LockPayload */
          pC1G2LockPayload = LLRP_C1G2LockPayload_construct();
          /* Set the LockPrivilege  */
          switch (args->mask)
          {
            case TMR_GEN2_LOCK_BITS_USER_PERM:
              {
                index = 0;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_User_Memory;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Lock;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_USER:
              {
                index = 1;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_User_Memory;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Read_Write;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_TID_PERM:
              {
                index = 2;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_TID_Memory;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Lock;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_TID:
              {
                index = 3;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_TID_Memory;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Read_Write;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_EPC_PERM:
              {
                index = 4;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_EPC_Memory;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Lock;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_EPC:
              {
                index = 5;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_EPC_Memory;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Read_Write;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_ACCESS_PERM:
              {
                index = 6;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_Access_Password;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Lock;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_ACCESS:
              {
                index = 7;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_Access_Password;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Read_Write;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_KILL_PERM:
              {
                index = 8;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_Kill_Password;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Lock;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Perma_Unlock;
                }
                break;
              }
            case TMR_GEN2_LOCK_BITS_KILL:
              {
                index = 9;
                pC1G2LockPayload->eDataField = LLRP_C1G2LockDataField_Kill_Password;
                if (1 == (args->action >> index))
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Read_Write;
                }
                else
                {
                  pC1G2LockPayload->ePrivilege = LLRP_C1G2LockPrivilege_Unlock;
                }
                break;
              }

            default:
              {
                /* Unknown lockaction  return invalid error */
                return TMR_ERROR_INVALID; 

              }
          }

          /**
           * Set C1G2LockPayload  to C1G2Lock
           **/
          LLRP_C1G2Lock_addC1G2LockPayload (pC1G2Lock, pC1G2LockPayload);
          /**
           * Set C1G2Lock as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pC1G2Lock);

          break;
        }

      case TMR_TAGOP_GEN2_KILL:
        {
          TMR_TagOp_GEN2_Kill   *args;
          LLRP_tSC1G2Kill       *pC1G2Kill;

          args = &tagop->u.gen2.u.kill;
          /* Construct and initialize C1G2Kill */
          pC1G2Kill = LLRP_C1G2Kill_construct();
          /* Set OpSpec Id */
          LLRP_C1G2Kill_setOpSpecID(pC1G2Kill, reader->u.llrpReader.opSpecId);
          /* Set the kill password */
          LLRP_C1G2Kill_setKillPassword(pC1G2Kill, args->password);

          /**
           * Set C1G2Kill as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                      (LLRP_tSParameter *)pC1G2Kill);
          break;
        }

      case TMR_TAGOP_GEN2_BLOCKWRITE:
        {
          TMR_TagOp_GEN2_BlockWrite  *args;
          LLRP_tSC1G2BlockWrite      *pC1G2BlockWrite;
          llrp_u16v_t                 data;

          args = &tagop->u.gen2.u.blockWrite;
          /* Construct and initialize C1G2BlockWrite */
          pC1G2BlockWrite = LLRP_C1G2BlockWrite_construct();
          /* Set OpSpec Id */
          LLRP_C1G2BlockWrite_setOpSpecID(pC1G2BlockWrite, reader->u.llrpReader.opSpecId);
          /* Set access password */
          LLRP_C1G2BlockWrite_setAccessPassword(pC1G2BlockWrite, reader->u.llrpReader.gen2AccessPassword);
          /* Set Memory Bank */
          LLRP_C1G2BlockWrite_setMB(pC1G2BlockWrite, (llrp_u2_t)args->bank);
          /* Set word pointer */
          LLRP_C1G2BlockWrite_setWordPointer(pC1G2BlockWrite, args->wordPtr);
          /* Set the data to be written */
          data = LLRP_u16v_construct (args->data.len);
          memcpy (data.pValue, args->data.list, data.nValue * sizeof(uint16_t));
          LLRP_C1G2BlockWrite_setWriteData(pC1G2BlockWrite, data);

          /**
           * Set C1G2BlockWrite as OpSpec to accessSpec
           **/

          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pC1G2BlockWrite);

          break;
        }

      case TMR_TAGOP_GEN2_NXP_SETREADPROTECT:
        {
          TMR_TagOp_GEN2_NXP_SetReadProtect *args;
          LLRP_tSThingMagicNXPG2ISetReadProtect *pG2I;
          LLRP_tSThingMagicNXPG2XSetReadProtect *pG2X;

          args = &tagop->u.gen2.u.custom.u.nxp.u.setReadProtect;

          if (TMR_SR_GEN2_NXP_G2I_SILICON == tagop->u.gen2.u.custom.chipType)
          {
            /* Construct and initialize G2I setReadProtect */
            pG2I = LLRP_ThingMagicNXPG2ISetReadProtect_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2ISetReadProtect_setOpSpecID(
                    pG2I, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2ISetReadProtect_setAccessPassword(
                                      pG2I, args->accessPassword);
            /**
             * Set G2I SetReadProtect as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                          (LLRP_tSParameter *)pG2I);
          }
          else
          {
            /* Construct and initialize G2X setReadProtect */
            pG2X = LLRP_ThingMagicNXPG2XSetReadProtect_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2XSetReadProtect_setOpSpecID(
                        pG2X, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2XSetReadProtect_setAccessPassword(
                                      pG2X, args->accessPassword);
            /**
             * Set G2X SetReadProtect as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                            (LLRP_tSParameter *)pG2X);
          }
          break;
        }

      case TMR_TAGOP_GEN2_NXP_RESETREADPROTECT:
        {
          TMR_TagOp_GEN2_NXP_ResetReadProtect   *args;
          LLRP_tSThingMagicNXPG2IResetReadProtect *pG2I;
          LLRP_tSThingMagicNXPG2XResetReadProtect *pG2X;

          args = &tagop->u.gen2.u.custom.u.nxp.u.resetReadProtect;

          if (TMR_SR_GEN2_NXP_G2I_SILICON == tagop->u.gen2.u.custom.chipType)
          {
            /* Construct and initialize G2I resetReadProtect */
            pG2I = LLRP_ThingMagicNXPG2IResetReadProtect_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2IResetReadProtect_setOpSpecID(
                    pG2I, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2IResetReadProtect_setAccessPassword(
                                      pG2I, args->accessPassword);
            /**
             * Set G2I ResetReadProtect as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                          (LLRP_tSParameter *)pG2I);
          }
          else
          {
            /* Construct and initialize G2X resetReadProtect */
            pG2X = LLRP_ThingMagicNXPG2XResetReadProtect_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2XResetReadProtect_setOpSpecID(
                        pG2X, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2XResetReadProtect_setAccessPassword(
                                      pG2X, args->accessPassword);
            /**
             * Set G2X ResetReadProtect as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                            (LLRP_tSParameter *)pG2X);
          }
          break;
        }

      case TMR_TAGOP_GEN2_NXP_CHANGEEAS:
        {
          TMR_TagOp_GEN2_NXP_ChangeEAS     *args;
          LLRP_tSThingMagicNXPG2IChangeEAS *pG2I;
          LLRP_tSThingMagicNXPG2XChangeEAS *pG2X;

          args = &tagop->u.gen2.u.custom.u.nxp.u.changeEAS;

          if (TMR_SR_GEN2_NXP_G2I_SILICON == tagop->u.gen2.u.custom.chipType)
          {
            /* Construct and initialize G2I changeEAS */
            pG2I = LLRP_ThingMagicNXPG2IChangeEAS_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2IChangeEAS_setOpSpecID(
                    pG2I, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2IChangeEAS_setAccessPassword(
                                      pG2I, args->accessPassword);
            /* Set EASStatus */
            LLRP_ThingMagicNXPG2IChangeEAS_setReset(
                                      pG2I, args->reset);
            /**
             * Set G2I ChangeEAS as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                          (LLRP_tSParameter *)pG2I);
          }
          else
          {
            /* Construct and initialize G2X ChangeEAS */
            pG2X = LLRP_ThingMagicNXPG2XChangeEAS_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2XChangeEAS_setOpSpecID(
                        pG2X, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2XChangeEAS_setAccessPassword(
                                      pG2X, args->accessPassword);
            /* Set EASStatus */
            LLRP_ThingMagicNXPG2XChangeEAS_setReset(
                                      pG2X, args->reset);
            /**
             * Set G2X ChangeEAS as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                            (LLRP_tSParameter *)pG2X);
          }
          break;
        }

      case TMR_TAGOP_GEN2_NXP_EASALARM:
        {
          TMR_TagOp_GEN2_NXP_EASAlarm     *args;
          LLRP_tSThingMagicNXPG2IEASAlarm *pG2I;
          LLRP_tSThingMagicNXPG2XEASAlarm *pG2X;

          args = &tagop->u.gen2.u.custom.u.nxp.u.EASAlarm;

          if (TMR_SR_GEN2_NXP_G2I_SILICON == tagop->u.gen2.u.custom.chipType)
          {
            /* Construct and initialize G2I EASAlarm */
            pG2I = LLRP_ThingMagicNXPG2IEASAlarm_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2IEASAlarm_setOpSpecID(
                    pG2I, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2IEASAlarm_setAccessPassword(
                pG2I, reader->u.llrpReader.gen2AccessPassword);
            /* Set DivideRatio */
            LLRP_ThingMagicNXPG2IEASAlarm_setDivideRatio(pG2I, args->dr);
            /* Set TagEncoding */
            LLRP_ThingMagicNXPG2IEASAlarm_setTagEncoding(pG2I, args->m);
            /* Set TrExt */
            LLRP_ThingMagicNXPG2IEASAlarm_setPilotTone(pG2I, args->trExt);

            /**
             * Set G2I EASAlarm as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                          (LLRP_tSParameter *)pG2I);
          }
          else
          {
            /* Construct and initialize G2X EASAlarm */
            pG2X = LLRP_ThingMagicNXPG2XEASAlarm_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2XEASAlarm_setOpSpecID(
                    pG2X, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2XEASAlarm_setAccessPassword(
                pG2X, reader->u.llrpReader.gen2AccessPassword);
            /* Set DivideRatio */
            LLRP_ThingMagicNXPG2XEASAlarm_setDivideRatio(pG2X, args->dr);
            /* Set TagEncoding */
            LLRP_ThingMagicNXPG2XEASAlarm_setTagEncoding(pG2X, args->m);
            /* Set TrExt */
            LLRP_ThingMagicNXPG2XEASAlarm_setPilotTone(pG2X, args->trExt);

            /**
             * Set G2X EASAlarm as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                            (LLRP_tSParameter *)pG2X);
          }
          break;
        }

      case TMR_TAGOP_GEN2_NXP_CALIBRATE:
        {
          TMR_TagOp_GEN2_NXP_Calibrate     *args;
          LLRP_tSThingMagicNXPG2ICalibrate *pG2I;
          LLRP_tSThingMagicNXPG2XCalibrate *pG2X;

          args = &tagop->u.gen2.u.custom.u.nxp.u.calibrate;

          if (TMR_SR_GEN2_NXP_G2I_SILICON == tagop->u.gen2.u.custom.chipType)
          {
            /* Construct and initialize G2I calibrate */
            pG2I = LLRP_ThingMagicNXPG2ICalibrate_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2ICalibrate_setOpSpecID(
                    pG2I, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2ICalibrate_setAccessPassword(
                                      pG2I, args->accessPassword);
            /**
             * Set G2I Calibrate as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                          (LLRP_tSParameter *)pG2I);
          }
          else
          {
            /* Construct and initialize G2X Calibrate */
            pG2X = LLRP_ThingMagicNXPG2XCalibrate_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2XCalibrate_setOpSpecID(
                        pG2X, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2XCalibrate_setAccessPassword(
                                      pG2X, args->accessPassword);
            /**
             * Set G2X Calibrate as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                            (LLRP_tSParameter *)pG2X);
          }
          break;
        }

      case TMR_TAGOP_GEN2_NXP_CHANGECONFIG:
        {
          TMR_TagOp_GEN2_NXP_ChangeConfig     *args;
          LLRP_tSThingMagicNXPG2IChangeConfig *pG2I;
          LLRP_tSThingMagicNXPConfigWord      *pConfigWord;

          args = &tagop->u.gen2.u.custom.u.nxp.u.changeConfig;

          if (TMR_SR_GEN2_NXP_G2I_SILICON == tagop->u.gen2.u.custom.chipType)
          {
            /* Construct and initialize G2I changeconfig */
            pG2I = LLRP_ThingMagicNXPG2IChangeConfig_construct();
            /* Set OpSpec Id */
            LLRP_ThingMagicNXPG2IChangeConfig_setOpSpecID(
                    pG2I, reader->u.llrpReader.opSpecId);
            /* Set access password */
            LLRP_ThingMagicNXPG2IChangeConfig_setAccessPassword(
                                      pG2I, args->accessPassword);

            /* Construct and initialize configword */
            pConfigWord = LLRP_ThingMagicNXPConfigWord_construct();
            
            pConfigWord->PSFAlarm = args->configWord.bits.psfAlarm;
            pConfigWord->ReadProtectTID = args->configWord.bits.readProtectTID;
            pConfigWord->ReadProtectEPC = args->configWord.bits.readProtectEPC;
            pConfigWord->ReadProtectUser = args->configWord.bits.readProtectUser;
            pConfigWord->PrivacyMode = args->configWord.bits.privacyMode;
            pConfigWord->DigitalOutput = args->configWord.bits.digitalOutput;
            pConfigWord->MaxBackscatterStrength = args->configWord.bits.maxBackscatterStrength;
            pConfigWord->ConditionalReadRangeReduction_OpenShort =
                  args->configWord.bits.conditionalReadRangeReduction_openShort;
            pConfigWord->ConditionalReadRangeReduction_OnOff =
                  args->configWord.bits.conditionalReadRangeReduction_onOff;
            pConfigWord->DataMode = args->configWord.bits.dataMode;
            pConfigWord->TransparentMode = args->configWord.bits.transparentMode;
            pConfigWord->InvertDigitalOutput = args->configWord.bits.invertDigitalOutput;
            pConfigWord->ExternalSupply = args->configWord.bits.externalSupply;
            pConfigWord->TamperAlarm = args->configWord.bits.tamperAlarm;

            /* Set configword to opSpec */
            LLRP_ThingMagicNXPG2IChangeConfig_setThingMagicNXPConfigWord(
                                                        pG2I, pConfigWord);
            /**
             * Set G2I Calibrate as OpSpec to accessSpec
             **/
            LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
                                          (LLRP_tSParameter *)pG2I);
          }
          else
          {
            /**
             * ChangeConfig works only for G2I tags.
             **/
            return TMR_ERROR_UNSUPPORTED;
          }
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_SETSFEPARAMETERS:
        {
          TMR_TagOp_GEN2_IDS_SL900A_SetSfeParameters *args;
          LLRP_tSThingMagicIDSSL900ASetSFEParams     *pTMIdsSetSfeParameters;
          LLRP_tSThingMagicIDSSFEParam               *pTMIdsSfeParam;
          LLRP_tSThingMagicIDSSL900ACommandRequest   *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.setSfeParameters;

          /* Construct and initialize TMIDS SetSfeParameters opspec */
          pTMIdsSetSfeParameters = LLRP_ThingMagicIDSSL900ASetSFEParams_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS SetSfeParameters operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS SetSfeParameters opspec */
          LLRP_ThingMagicIDSSL900ASetSFEParams_setThingMagicIDSSL900ACommandRequest(pTMIdsSetSfeParameters,
              pTMIdsCommandRequest);
          /* Construct and initialize the TMIDS SetSfeParams */
          {
            pTMIdsSfeParam = LLRP_ThingMagicIDSSFEParam_construct();
            /* Raw 16-bit SFE parameters value */
            LLRP_ThingMagicIDSSFEParam_setraw(pTMIdsSfeParam, (llrp_u16_t)args->sfe->raw);
            /* External sensor 2 range */
            LLRP_ThingMagicIDSSFEParam_setrange(pTMIdsSfeParam, (llrp_u8_t)args->sfe->Rang);
            /* External sensor 1 range */
            LLRP_ThingMagicIDSSFEParam_setseti(pTMIdsSfeParam, (llrp_u8_t)args->sfe->Seti);
            /* External sensor 1 type */
            LLRP_ThingMagicIDSSFEParam_setExt1(pTMIdsSfeParam, (llrp_u8_t)args->sfe->Ext1);
            /* External sensor 2 type */
            LLRP_ThingMagicIDSSFEParam_setExt2(pTMIdsSfeParam, (llrp_u8_t)args->sfe->Ext2);
            /* Use preset range */
            LLRP_ThingMagicIDSSFEParam_setAutoRangeDisable(pTMIdsSfeParam, (llrp_u1_t)args->sfe->AutorangeDisable);
            /* Sensor used in limit check */
            LLRP_ThingMagicIDSSFEParam_setVerifySensorID(pTMIdsSfeParam, (llrp_u8_t)args->sfe->VerifySensorID);
            /* specifies the type of field user want to modify */
            LLRP_ThingMagicIDSSFEParam_setSFEType(pTMIdsSfeParam, (LLRP_tEThingMagicCustomIDSSFEType)args->sfe->type);

            LLRP_ThingMagicIDSSL900ASetSFEParams_setThingMagicIDSSFEParam(pTMIdsSetSfeParameters,
                (LLRP_tSThingMagicIDSSFEParam *)pTMIdsSfeParam);
          }

          /**
           * Set TMIDS SetSfeParameters as OpSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsSetSfeParameters);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_GETMEASUREMENTSETUP:
        {
          TMR_TagOp_GEN2_IDS_SL900A_GetMeasurementSetup *args;
          LLRP_tSThingMagicIDSSL900AGetMeasurementSetup *pTMIdsGetMeasurementSetup;
          LLRP_tSThingMagicIDSSL900ACommandRequest      *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.measurementSetup;

          /* Construct and initialize TMIDS GetMeasurementSetUp opspec */
          pTMIdsGetMeasurementSetup = LLRP_ThingMagicIDSSL900AGetMeasurementSetup_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS GetMeasurementSetup operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS GetMeasurementSetup opspec */
          LLRP_ThingMagicIDSSL900AGetMeasurementSetup_setThingMagicIDSSL900ACommandRequest(pTMIdsGetMeasurementSetup,
              pTMIdsCommandRequest);

          /**
           * Set TMIDS GetMeasurementSetup as OpSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsGetMeasurementSetup);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_GETBATTERYLEVEL:
        {
          TMR_TagOp_GEN2_IDS_SL900A_GetBatteryLevel *args;
          LLRP_tSThingMagicIDSSL900AGetBatteryLevel *pTMIdsGetBatteryLevel;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.batteryLevel;

          /* Construct and initialize TMIDS GetBatteryLevel opspec */
          pTMIdsGetBatteryLevel = LLRP_ThingMagicIDSSL900AGetBatteryLevel_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS GetBatteryLevel operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS GetBatteryLevel opspec */
          LLRP_ThingMagicIDSSL900AGetBatteryLevel_setThingMagicIDSSL900ACommandRequest(pTMIdsGetBatteryLevel,
              pTMIdsCommandRequest);

          /**
           * Set TMIDS GetBatteryLevel as OpSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsGetBatteryLevel);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_SETLOGLIMITS:
        {
          TMR_TagOp_GEN2_IDS_SL900A_SetLogLimits    *args;
          LLRP_tSThingMagicIDSSL900ASetLogLimits    *pTMIdsSetLogLimits;
          LLRP_tSThingMagicIDSLogLimits             *pTMIdsLogLimits;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.setLogLimit;

          /* Construct and initialize TMIDS SetLogLimits opspec */
          pTMIdsSetLogLimits = LLRP_ThingMagicIDSSL900ASetLogLimits_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS SetLogLimits operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS SetLogLimits opspec */
          LLRP_ThingMagicIDSSL900ASetLogLimits_setThingMagicIDSSL900ACommandRequest(pTMIdsSetLogLimits,
              pTMIdsCommandRequest);
          /* Construct and initialize the TMIDS LogLimits */
          {
            pTMIdsLogLimits = LLRP_ThingMagicIDSLogLimits_construct();
            /* Specifying the extreme lower limit */
            LLRP_ThingMagicIDSLogLimits_setextremeLower(pTMIdsLogLimits, (llrp_u16_t)args->limit.extremeLower);
            /* Specifying the lower limit */
            LLRP_ThingMagicIDSLogLimits_setlower(pTMIdsLogLimits, (llrp_u16_t)args->limit.lower);
            /* Specifying the upper limit */
            LLRP_ThingMagicIDSLogLimits_setupper(pTMIdsLogLimits, (llrp_u16_t)args->limit.upper);
            /* Specifying the extreme upper limit */
            LLRP_ThingMagicIDSLogLimits_setextremeUpper(pTMIdsLogLimits, (llrp_u16_t)args->limit.extremeUpper);

            LLRP_ThingMagicIDSSL900ASetLogLimits_setThingMagicIDSLogLimits(pTMIdsSetLogLimits,
                (LLRP_tSThingMagicIDSLogLimits *)pTMIdsLogLimits);
          }

          /**
           * Set TMIDS SetLogLimits as OpSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsSetLogLimits);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_SETSHELFLIFE:
        {
          TMR_TagOp_GEN2_IDS_SL900A_SetShelfLife    *args;
          LLRP_tSThingMagicIDSSetShelfLife          *pTMIdsSetShelfLife;
          LLRP_tSThingMagicIDSSLBlock0              *pTMIdsBlock0;
          LLRP_tSThingMagicIDSSLBlock1              *pTMIdsBlock1;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.setShelfLife;

          /* Construct and initialize TMIDS SetShelfLife opspec */
          pTMIdsSetShelfLife = LLRP_ThingMagicIDSSetShelfLife_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS SetShelfLife operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS SetShelfLife opspec */
          LLRP_ThingMagicIDSSetShelfLife_setThingMagicIDSSL900ACommandRequest(pTMIdsSetShelfLife,
              pTMIdsCommandRequest);
          /* Construct and Initialize the TMIDS Block0 Data */
          {
            pTMIdsBlock0 = LLRP_ThingMagicIDSSLBlock0_construct();
            /* raw value of block0 */
            LLRP_ThingMagicIDSSLBlock0_setraw(pTMIdsBlock0, (llrp_u32_t)args->shelfLifeBlock0->raw);
            /* Tmax */
            LLRP_ThingMagicIDSSLBlock0_setTimeMax(pTMIdsBlock0, (llrp_u8_t)args->shelfLifeBlock0->Tmax);
            /* Tmin */
            LLRP_ThingMagicIDSSLBlock0_setTimeMin(pTMIdsBlock0, (llrp_u8_t)args->shelfLifeBlock0->Tmin);
            /* Tstd */
            LLRP_ThingMagicIDSSLBlock0_setTimeStd(pTMIdsBlock0, (llrp_u8_t)args->shelfLifeBlock0->Tstd);
            /* Ea */
            LLRP_ThingMagicIDSSLBlock0_setEa(pTMIdsBlock0, (llrp_u8_t)args->shelfLifeBlock0->Ea);

            LLRP_ThingMagicIDSSetShelfLife_setThingMagicIDSSLBlock0(pTMIdsSetShelfLife,
                (LLRP_tSThingMagicIDSSLBlock0 *)pTMIdsBlock0);
          }
          /* Construct and Initialize the TMIDS Block1 Data */
          {
            pTMIdsBlock1 = LLRP_ThingMagicIDSSLBlock1_construct();
            /* raw value for block 1 */
            LLRP_ThingMagicIDSSLBlock1_setraw(pTMIdsBlock1, (llrp_u32_t)args->shelfLifeBlock1->raw);
            /* SLinit */
            LLRP_ThingMagicIDSSLBlock1_setSLInit(pTMIdsBlock1, (llrp_u16_t)args->shelfLifeBlock1->SLinit);
            /* Tint */
            LLRP_ThingMagicIDSSLBlock1_setTInit(pTMIdsBlock1, (llrp_u16_t)args->shelfLifeBlock1->Tint);
            /* SensorID */
            LLRP_ThingMagicIDSSLBlock1_setSensorID(pTMIdsBlock1, (llrp_u8_t)args->shelfLifeBlock1->sensorID);
            /* Enable negative shelf life */
            LLRP_ThingMagicIDSSLBlock1_setenableNegative(pTMIdsBlock1, (llrp_u1_t)args->shelfLifeBlock1->enableNegative);
            /* Shelf life algorithem enable */
            LLRP_ThingMagicIDSSLBlock1_setalgorithmEnable(pTMIdsBlock1, (llrp_u1_t)args->shelfLifeBlock1->algorithmEnable);
            /* RFU Bytes */
            LLRP_ThingMagicIDSSLBlock1_setRFU(pTMIdsBlock1, (llrp_u8_t)args->shelfLifeBlock1->rfu);

            LLRP_ThingMagicIDSSetShelfLife_setThingMagicIDSSLBlock1(pTMIdsSetShelfLife,
                (LLRP_tSThingMagicIDSSLBlock1 *)pTMIdsBlock1);
          }

          /**
           * Set TMIDS SetShelfLife as OpSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsSetShelfLife);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_SETPASSWORD:
        {
          TMR_TagOp_GEN2_IDS_SL900A_SetPassword     *args;
          LLRP_tSThingMagicIDSSL900ASetIDSPassword  *pTMIdsSetPassword;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.setPassword;
          /* Construct and initialize TMIDS SetPassword opspec */
          pTMIdsSetPassword = LLRP_ThingMagicIDSSL900ASetIDSPassword_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS SetPassword operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS SetPassword opspec */
          LLRP_ThingMagicIDSSL900ASetIDSPassword_setThingMagicIDSSL900ACommandRequest(pTMIdsSetPassword,
              pTMIdsCommandRequest);
          /* Set the IDS New PasswordLevel */
          LLRP_ThingMagicIDSSL900ASetIDSPassword_setNewPasswordLevel(pTMIdsSetPassword,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->NewPasswordLevel);
          /* Set the IDS New Password */
          LLRP_ThingMagicIDSSL900ASetIDSPassword_setNewIDSPassword(pTMIdsSetPassword,
              (llrp_u32_t)args->NewPassword);

          /**
           * Set TMIDS SetPassword as OpSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsSetPassword);
          break;
        }

      case TMR_TAGOP_GEN2_IMPINJ_MONZA4_QTREADWRITE:
        {
          TMR_TagOp_GEN2_Impinj_Monza4_QTReadWrite *args;
          LLRP_tSThingMagicImpinjMonza4QTReadWrite *pQTReadWrite;
          LLRP_tSThingMagicMonza4ControlByte       *pControlByte;
          LLRP_tSThingMagicMonza4Payload           *pPayload;

          args = &tagop->u.gen2.u.custom.u.impinj.u.monza4.u.qtReadWrite;

          /* Construct and initialize monza4 opspec */
          pQTReadWrite = LLRP_ThingMagicImpinjMonza4QTReadWrite_construct();
          /* Set OpSpecId */
          LLRP_ThingMagicImpinjMonza4QTReadWrite_setOpSpecID(
                    pQTReadWrite, reader->u.llrpReader.opSpecId);
          /* Set Access Password */
          LLRP_ThingMagicImpinjMonza4QTReadWrite_setAccessPassword(
                    pQTReadWrite, args->accessPassword);
          /* Initialize and set controlbyte */
          pControlByte = LLRP_ThingMagicMonza4ControlByte_construct();
          pControlByte->Persistance = args->controlByte.bits.persistence;
          pControlByte->ReadWrite = args->controlByte.bits.readWrite;

          LLRP_ThingMagicImpinjMonza4QTReadWrite_setThingMagicMonza4ControlByte(
              pQTReadWrite, pControlByte);

          /* Initialize and set payload */
          pPayload = LLRP_ThingMagicMonza4Payload_construct();
          pPayload->QT_MEM = args->payload.bits.QT_MEM;
          pPayload->QT_SR = args->payload.bits.QT_SR;

          LLRP_ThingMagicImpinjMonza4QTReadWrite_setThingMagicMonza4Payload(
              pQTReadWrite, pPayload);

          /**
           * Set Monza4 QTReadWrite as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pQTReadWrite);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_GETSENSOR:
        {
          TMR_TagOp_GEN2_IDS_SL900A_GetSensorValue  *args;
          LLRP_tSThingMagicIDSSL900ASensorValue     *pTMIdsGetSensor;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.sensor;

          /* Construct and initialize TMIDS GetSensor opspec */
          pTMIdsGetSensor = LLRP_ThingMagicIDSSL900ASensorValue_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS GetSensor operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS GetSensor opspec */
          LLRP_ThingMagicIDSSL900ASensorValue_setThingMagicIDSSL900ACommandRequest(pTMIdsGetSensor,
              pTMIdsCommandRequest);
          /* Set the Sensor type to TMIDS GetSensor opspec */
          LLRP_ThingMagicIDSSL900ASensorValue_setSensorType(pTMIdsGetSensor,
              (LLRP_tEThingMagicCustomIDSSensorType)args->sl900A.sensortype);

          /**
           * Set TMIDS GetSensor as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsGetSensor);
          break;

        }

      case TMR_TAGOP_GEN2_IDS_SL900A_INITIALIZE:
        {
          TMR_TagOp_GEN2_IDS_SL900A_Initialize      *args;
          LLRP_tSThingMagicIDSSL900AInitialize      *pTMIdsInitialize;
          LLRP_tSThingMagicIDSDelayTime             *pTMIdsDelayTime;
          LLRP_tSThingMagicIDSApplicationData       *pTMIdsApplicationData;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.initialize;

          /* Construct and initialize TMIDS Initialize opspec */
          pTMIdsInitialize = LLRP_ThingMagicIDSSL900AInitialize_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS Initialize operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS Initialize opspec */
          LLRP_ThingMagicIDSSL900AInitialize_setThingMagicIDSSL900ACommandRequest(pTMIdsInitialize,
              pTMIdsCommandRequest);
          /* Set the Delay time parameters */
          pTMIdsDelayTime = LLRP_ThingMagicIDSDelayTime_construct();
          pTMIdsDelayTime->delayMode = (llrp_u8_t) args->delayTime.Mode;
          pTMIdsDelayTime->delayTime = (llrp_u16_t) args->delayTime.Time;
          pTMIdsDelayTime->timerEnable = (llrp_u1_t) args->delayTime.IrqTimerEnable;
          LLRP_ThingMagicIDSSL900AInitialize_setThingMagicIDSDelayTime(pTMIdsInitialize,
              pTMIdsDelayTime);
          /* Set the Application Data */
          pTMIdsApplicationData = LLRP_ThingMagicIDSApplicationData_construct();
          pTMIdsApplicationData->brokenWordPointer = (llrp_u8_t) args->applicationData.BrokenWordPointer;
          pTMIdsApplicationData->numberOfWords = (llrp_u16_t) args->applicationData.NumberOfWords;
          LLRP_ThingMagicIDSSL900AInitialize_setThingMagicIDSApplicationData(pTMIdsInitialize,
              pTMIdsApplicationData);

          /**
           * Set TMIDS Initialize as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsInitialize);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_SETLOGMODE:
        {
          TMR_TagOp_GEN2_IDS_SL900A_SetLogMode      *args;
          LLRP_tSThingMagicIDSSL900ASetLogMode      *pTMIdsSetLogMode;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.setLogMode;

          /* Construct and initialize TMIDS SetLogMode opspec */
          pTMIdsSetLogMode = LLRP_ThingMagicIDSSL900ASetLogMode_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS SetLogMode operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS SetLogMode opspec */
          LLRP_ThingMagicIDSSL900ASetLogMode_setThingMagicIDSSL900ACommandRequest(pTMIdsSetLogMode,
              pTMIdsCommandRequest);
          /* Set the logging Form */
          LLRP_ThingMagicIDSSL900ASetLogMode_setLoggingForm(pTMIdsSetLogMode,
              (LLRP_tEThingMagicCustomIDSLoggingForm)args->sl900A.dataLog);
          /* Set the Storage rule */
          LLRP_ThingMagicIDSSL900ASetLogMode_setStorageRule(pTMIdsSetLogMode,
              (LLRP_tEThingMagicCustomIDSStorageRule)args->sl900A.rule);
          /* Enable log for EXT1 external sensor */
          LLRP_ThingMagicIDSSL900ASetLogMode_setExt1Enable(pTMIdsSetLogMode,
              (llrp_u1_t)args->Ext1Enable);
          /* Enable log for EXT2 external sensor */
          LLRP_ThingMagicIDSSL900ASetLogMode_setExt2Enable(pTMIdsSetLogMode,
              (llrp_u1_t)args->Ext2Enable);
          /* Enable log for Temperature sensor */
          LLRP_ThingMagicIDSSL900ASetLogMode_setTempEnable(pTMIdsSetLogMode,
              (llrp_u1_t)args->TempEnable);
          /* Enable log for BATT sensor */
          LLRP_ThingMagicIDSSL900ASetLogMode_setBattEnable(pTMIdsSetLogMode,
              (llrp_u1_t)args->BattEnable);
          /* Set the log interval */
          LLRP_ThingMagicIDSSL900ASetLogMode_setLogInterval(pTMIdsSetLogMode,
              (llrp_u16_t)args->LogInterval);

          /**
           * Set TMIDS SetLogMode as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsSetLogMode);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_STARTLOG:
        {
          TMR_TagOp_GEN2_IDS_SL900A_StartLog        *args;
          LLRP_tSThingMagicIDSSL900AStartLog        *pTMIdsStartLog;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.startLog;

          /* Construct and initialize TMIDS StartLog opspec */
          pTMIdsStartLog = LLRP_ThingMagicIDSSL900AStartLog_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS StartLog operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS StartLog opspec */
          LLRP_ThingMagicIDSSL900AStartLog_setThingMagicIDSSL900ACommandRequest(pTMIdsStartLog,
              pTMIdsCommandRequest);
          /* Set the Time to initialize log timestamp counter with */
          LLRP_ThingMagicIDSSL900AStartLog_setStartTime(pTMIdsStartLog,
              (llrp_u32_t)args->startTime);

          /**
           * Set TMIDS StartLog as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsStartLog);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_GETLOGSTATE:
        {
          TMR_TagOp_GEN2_IDS_SL900A_GetLogState     *args;
          LLRP_tSThingMagicIDSSL900AGetLogState     *pTMIdsGetLogState;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.getLog;

          /* Construct and initialize TMIDS StartLog opspec */
          pTMIdsGetLogState = LLRP_ThingMagicIDSSL900AGetLogState_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS GetLogState operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS GetLogState opspec */
          LLRP_ThingMagicIDSSL900AGetLogState_setThingMagicIDSSL900ACommandRequest(pTMIdsGetLogState,
              pTMIdsCommandRequest);

          /**
           * Set TMIDS GetLogState as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsGetLogState);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_ENDLOG:
        {
          TMR_TagOp_GEN2_IDS_SL900A_EndLog          *args;
          LLRP_tSThingMagicIDSSL900AEndLog          *pTMIdsEndLog;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.endLog;

          /* Construct and initialize TMIDS EndLOg opspec */
          pTMIdsEndLog = LLRP_ThingMagicIDSSL900AEndLog_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS EndLog operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS EndLog opspec */
          LLRP_ThingMagicIDSSL900AEndLog_setThingMagicIDSSL900ACommandRequest(pTMIdsEndLog,
              pTMIdsCommandRequest);

          /**
           * Set TMIDS EndLog as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsEndLog);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOSTATUS:
        {
          TMR_TagOp_GEN2_IDS_SL900A_AccessFifoStatus  *args;
          LLRP_tSThingMagicIDSSL900AAccessFIFOStatus  *pTMIdsAccessFifoStatus;
          LLRP_tSThingMagicIDSSL900ACommandRequest    *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.accessFifoStatus;

          /* Construct and initialize TMIDS FifoStatus opspec */
          pTMIdsAccessFifoStatus = LLRP_ThingMagicIDSSL900AAccessFIFOStatus_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->status.AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->status.sl900A.level);
          /* Set the Command code for IDS AccessFifoStatus operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->status.CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->status.Password);
          /* Set the IDS Command Request to TMIDS AccessFifoStatus opspec */
          LLRP_ThingMagicIDSSL900AAccessFIFOStatus_setThingMagicIDSSL900ACommandRequest(pTMIdsAccessFifoStatus,
              pTMIdsCommandRequest);

          /**
           * Set TMIDS AccessFifoStatus as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsAccessFifoStatus);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOWRITE:
        {
          TMR_TagOp_GEN2_IDS_SL900A_AccessFifoWrite  *args;
          LLRP_tSThingMagicIDSSL900AAccessFIFOWrite  *pTMIdsAccessFifoWrite;
          LLRP_tSThingMagicIDSSL900ACommandRequest   *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.accessFifoWrite;

          /* Construct and initialize TMIDS FifoWrite opspec */
          pTMIdsAccessFifoWrite = LLRP_ThingMagicIDSSL900AAccessFIFOWrite_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->write.AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->write.sl900A.level);
          /* Set the Command code for IDS AccessFifoWrite operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->write.CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->write.Password);
          /* Set the IDS Command Request to TMIDS AccessFifoWrite opspec */
          LLRP_ThingMagicIDSSL900AAccessFIFOWrite_setThingMagicIDSSL900ACommandRequest(pTMIdsAccessFifoWrite,
              pTMIdsCommandRequest);
          /* Set the writePayLoad */
          {
            llrp_u8v_t  writePayLoad;
            /* construct and initialize the writePayLoad */
            writePayLoad = LLRP_u8v_construct((llrp_u16_t)args->payLoad->len);
            memcpy(writePayLoad.pValue, args->payLoad->list,
                (size_t) (args->payLoad->len * sizeof (args->payLoad->list[0])));
            LLRP_ThingMagicIDSSL900AAccessFIFOWrite_setwritePayLoad(pTMIdsAccessFifoWrite,
                writePayLoad);
          }

          /**
           * Set TMIDS AccessFifoWrite as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsAccessFifoWrite);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_ACCESSFIFOREAD:
        {
          TMR_TagOp_GEN2_IDS_SL900A_AccessFifoRead  *args;
          LLRP_tSThingMagicIDSSL900AAccessFIFORead  *pTMIdsAccessFifoRead;
          LLRP_tSThingMagicIDSSL900ACommandRequest  *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.accessFifoRead;

          /* Construct and initialize TMIDS FifoRead opspec */
          pTMIdsAccessFifoRead = LLRP_ThingMagicIDSSL900AAccessFIFORead_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->read.AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->read.sl900A.level);
          /* Set the Command code for IDS AccessFifoRead operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->read.CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->read.Password);
          /* Set the IDS Command Request to TMIDS AccessFifoRead opspec */
          LLRP_ThingMagicIDSSL900AAccessFIFORead_setThingMagicIDSSL900ACommandRequest(pTMIdsAccessFifoRead,
              pTMIdsCommandRequest);
          /* Specify the fifo read length */
          LLRP_ThingMagicIDSSL900AAccessFIFORead_setFIFOReadLength(pTMIdsAccessFifoRead,
              (llrp_u8_t)args->length);

          /**
           * Set TMIDS AccessFifoRead as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsAccessFifoRead);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_GETCALIBRATIONDATA:
        {
          TMR_TagOp_GEN2_IDS_SL900A_GetCalibrationData *args;
          LLRP_tSThingMagicIDSSL900AGetCalibrationData *pTMIdsGetCalibrationData;
          LLRP_tSThingMagicIDSSL900ACommandRequest     *pTMIdsCommandRequest;

          args = &tagop->u.gen2.u.custom.u.ids.u.calibrationData;

          /* Construct and initialize TMIDS GetCalibration opspec */
          pTMIdsGetCalibrationData = LLRP_ThingMagicIDSSL900AGetCalibrationData_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS GetCalibrationData operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS GetCalibrationData opspec */
          LLRP_ThingMagicIDSSL900AGetCalibrationData_setThingMagicIDSSL900ACommandRequest(pTMIdsGetCalibrationData,
              pTMIdsCommandRequest);

          /**
           * Set TMIDS GetCalibrationData as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsGetCalibrationData);
          break;
        }

      case TMR_TAGOP_GEN2_IDS_SL900A_SETCALIBRATIONDATA:
        {
          TMR_TagOp_GEN2_IDS_SL900A_SetCalibrationData *args;
          LLRP_tSThingMagicIDSSL900ASetCalibrationData *pTMIdsSetCalibrationData;
          LLRP_tSThingMagicIDSSL900ACommandRequest     *pTMIdsCommandRequest;
          LLRP_tSThingMagicIDSCalibrationData          *pTMIdsCalibrationData;

          args = &tagop->u.gen2.u.custom.u.ids.u.setCalibration;

          /* Construct and initialize TMIDS SetCalibration opspec */
          pTMIdsSetCalibrationData = LLRP_ThingMagicIDSSL900ASetCalibrationData_construct();
          /* Construct and initialize TMIDS Command Request */
          pTMIdsCommandRequest = LLRP_ThingMagicIDSSL900ACommandRequest_construct();
          /* Set OpSpecId to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setOpSpecID(pTMIdsCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set access password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setAccessPassword(pTMIdsCommandRequest,
              (llrp_u32_t) args->AccessPassword);
          /* Set the IDS password level to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setPasswordLevel(pTMIdsCommandRequest,
              (LLRP_tEThingMagicCustomIDSPasswordLevel)args->sl900A.level);
          /* Set the Command code for IDS SetCalibration operation to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setCommandCode(pTMIdsCommandRequest,
              (llrp_u8_t)args->CommandCode);
          /* Set the IDS Password to TMIDS Command Request */
          LLRP_ThingMagicIDSSL900ACommandRequest_setIDSPassword(pTMIdsCommandRequest,
              (llrp_u32_t)args->Password);
          /* Set the IDS Command Request to TMIDS SetCalibrationData opspec */
          LLRP_ThingMagicIDSSL900ASetCalibrationData_setThingMagicIDSSL900ACommandRequest(pTMIdsSetCalibrationData,
              pTMIdsCommandRequest);
          /* Construct and initialize the TMIDS CalibrationData */
          {
            pTMIdsCalibrationData = LLRP_ThingMagicIDSCalibrationData_construct();
            /* Set the Raw Binary stream to CalibrationData */
            LLRP_ThingMagicIDSCalibrationData_setraw(pTMIdsCalibrationData, (llrp_u64_t)args->cal.raw);
            /* AD1 lower voltage reference - fine - DO NOT MODIFY */
            LLRP_ThingMagicIDSCalibrationData_setad1(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Ad1);
            /* AD2 lower voltage reference - fine - DO NOT MODIFY */
            LLRP_ThingMagicIDSCalibrationData_setad2(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Ad2);
            /* AD1 lower voltage reference - coarse */
            LLRP_ThingMagicIDSCalibrationData_setcoars1(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Coarse1);
            /* AD2 lower voltage reference - coarse */
            LLRP_ThingMagicIDSCalibrationData_setcoars2(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Coarse2);
            /* Switches the lower AD voltage reference to ground */
            LLRP_ThingMagicIDSCalibrationData_setgndSwitch(pTMIdsCalibrationData, (llrp_u1_t)args->cal.GndSwitch);
            /* POR voltage level for 1.5V system */
            LLRP_ThingMagicIDSCalibrationData_setselp12(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Selp12);
            /* Main reference voltage calibration -- DO NOT MODIFY */
            LLRP_ThingMagicIDSCalibrationData_setadf(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Adf);
            /* RTC oscillator calibration */
            LLRP_ThingMagicIDSCalibrationData_setdf(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Df);
            /* Controlled battery supply for external sensor - the battery voltage is connected to the EXC pin */
            LLRP_ThingMagicIDSCalibrationData_setswExtEn(pTMIdsCalibrationData, (llrp_u1_t)args->cal.SwExtEn);
            /* POR voltage level for 3V system */
            LLRP_ThingMagicIDSCalibrationData_setselp22(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Selp22);
            /* Voltage level interrupt level for external sensor -- ratiometric */
            LLRP_ThingMagicIDSCalibrationData_setirlev(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Irlev);
            /* Main system clock oscillator calibration -- DO NOT MODIFY */
            LLRP_ThingMagicIDSCalibrationData_setringCal(pTMIdsCalibrationData, (llrp_u8_t)args->cal.RingCal);
            /* Temperature conversion offset calibration -- DO NOT MODIFY */
            LLRP_ThingMagicIDSCalibrationData_setoffInt(pTMIdsCalibrationData, (llrp_u8_t)args->cal.OffInt);
            /* Bandgap voltage temperature coefficient calibration -- DO NOT MODIFY */
            LLRP_ThingMagicIDSCalibrationData_setreftc(pTMIdsCalibrationData, (llrp_u8_t)args->cal.Reftc);
            /* Excitate for resistive sensors without DC */
            LLRP_ThingMagicIDSCalibrationData_setexcRes(pTMIdsCalibrationData, (llrp_u1_t)args->cal.ExcRes);
            /* Reserved for Future Use */
            LLRP_ThingMagicIDSCalibrationData_setRFU(pTMIdsCalibrationData, (llrp_u8_t)args->cal.RFU);

            LLRP_ThingMagicIDSSL900ASetCalibrationData_setThingMagicIDSCalibrationData(pTMIdsSetCalibrationData,
                (LLRP_tSThingMagicIDSCalibrationData *)pTMIdsCalibrationData);
          }

          /**
           * Set TMIDS SetCalibrationData as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMIdsSetCalibrationData);
          break;
        }

      case TMR_TAGOP_GEN2_DENATRAN_IAV_ACTIVATESECUREMODE:
        {
          TMR_TagOp_GEN2_Denatran_IAV_Activate_Secure_Mode *args;
          LLRP_tSThingMagicDenatranIAVActivateSecureMode   *pTMDenatranIAVActivateSecureMode;
          LLRP_tSThingMagicDenatranIAVCommandRequest       *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.secureMode;

          /* Construct and initialize the TMDenatranIAV ActivateSecureMode OpSpec */
          pTMDenatranIAVActivateSecureMode = LLRP_ThingMagicDenatranIAVActivateSecureMode_construct();
          /* Construct and initialize the TM DenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVActivateSecureMode_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVActivateSecureMode,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVActivateSecureMode);
          break;
        }
      case TMR_TAGOP_GEN2_DENATRAN_IAV_AUTHENTICATEOBU:
        {
          TMR_TagOp_GEN2_Denatran_IAV_Authenticate_OBU *args;
          LLRP_tSThingMagicDenatranIAVAuthenticateOBU  *pTMDenatranIAVAuthenticateOBU;
          LLRP_tSThingMagicDenatranIAVCommandRequest   *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.authenticateOBU;

          /* Construct and Initialize the TMDenatranIAV AuthenticateOBU OpSpec */
          pTMDenatranIAVAuthenticateOBU = LLRP_ThingMagicDenatranIAVAuthenticateOBU_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVAuthenticateOBU_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVAuthenticateOBU,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVAuthenticateOBU);
          break;
        }
      case TMR_TAGOP_GEN2_ACTIVATE_SINIAV_MODE:
        {
          TMR_TagOp_GEN2_Denatran_IAV_Activate_Siniav_Mode *args;
          LLRP_tSThingMagicDenatranIAVActivateSiniavMode   *pTMDenatranIAVSiniavMode;
          LLRP_tSThingMagicDenatranIAVCommandRequest       *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.activateSiniavMode;

          /* Construct and Initialize the TMDenatranIAV ActivateSiniavMode OpSpec */
          pTMDenatranIAVSiniavMode = LLRP_ThingMagicDenatranIAVActivateSiniavMode_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          {
            /**
             * The length and the format of the token field depends on the
             * token description bits. Alocate memory accordingly.
             **/
            llrp_u8v_t temp;
            uint8_t len = 8;

            temp = LLRP_u8v_construct((llrp_u16_t)len);
            memcpy(temp.pValue, args->token, (size_t)temp.nValue);
            LLRP_ThingMagicDenatranIAVActivateSiniavMode_settokenData(pTMDenatranIAVSiniavMode,
                temp);
          }

          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVActivateSiniavMode_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVSiniavMode,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVSiniavMode);
          break;
        }
      case TMR_TAGOP_GEN2_OBU_AUTH_ID:
        {
          TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_ID       *args;
          LLRP_tSThingMagicDenatranIAVOBUAuthenticateID *pTMDenatranIAVOBUAuthID;
          LLRP_tSThingMagicDenatranIAVCommandRequest    *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthId;

          /* Construct and Initialize the TMDenatranIAV OBU AuthID OpSpec */
          pTMDenatranIAVOBUAuthID = LLRP_ThingMagicDenatranIAVOBUAuthenticateID_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVOBUAuthenticateID_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVOBUAuthID,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVOBUAuthID);
          break;
        }
      case TMR_TAGOP_GEN2_AUTHENTICATE_OBU_FULL_PASS1:
        {
          TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass1      *args;
          LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass1 *pTMDenatranIAVOBUFullPass1;
          LLRP_tSThingMagicDenatranIAVCommandRequest           *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass1;

          /* Construct and Initialize the TMDenatranIAV OBU Full Pass1 OpSpec */
          pTMDenatranIAVOBUFullPass1 = LLRP_ThingMagicDenatranIAVOBUAuthenticateFullPass1_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVOBUAuthenticateFullPass1_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVOBUFullPass1,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVOBUFullPass1);
          break;
        }
      case TMR_TAGOP_GEN2_AUTHENTICATE_OBU_FULL_PASS2:
        {
          TMR_TagOp_GEN2_Denatran_IAV_OBU_Auth_Full_Pass2      *args;
          LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass2 *pTMDenatranIAVOBUFullPass2;
          LLRP_tSThingMagicDenatranIAVCommandRequest           *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.obuAuthFullPass2;

          /* Construct and Initialize the TMDenatranIAV OBU Full Pass2 OpSpec */
          pTMDenatranIAVOBUFullPass2 = LLRP_ThingMagicDenatranIAVOBUAuthenticateFullPass2_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVOBUAuthenticateFullPass2_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVOBUFullPass2,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVOBUFullPass2);
          break;
        }
      case TMR_TAGOP_GEN2_OBU_READ_FROM_MEM_MAP:
        {
          TMR_TagOp_GEN2_Denatran_IAV_OBU_ReadFromMemMap *args;
          LLRP_tSThingMagicDenatranIAVOBUReadFromMemMap  *pTMDenatranIAVOBUReadFromMemMap;
          LLRP_tSThingMagicDenatranIAVCommandRequest     *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.obuReadFromMemMap;

          /* Construct and Initialize the TMDenatranIAV OBU ReadFromMemMap OpSpec */
          pTMDenatranIAVOBUReadFromMemMap = LLRP_ThingMagicDenatranIAVOBUReadFromMemMap_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the read pointer to the TMDenatranIAV OBU ReadFromMemMap read pointer */
          LLRP_ThingMagicDenatranIAVOBUReadFromMemMap_setReadPtr(pTMDenatranIAVOBUReadFromMemMap,
              args->readPtr);
          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVOBUReadFromMemMap_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVOBUReadFromMemMap,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVOBUReadFromMemMap);
          break;
        }
      case TMR_TAGOP_GEN2_OBU_WRITE_TO_MEM_MAP:
        {
          TMR_TagOp_GEN2_Denatran_IAV_OBU_WriteToMemMap *args;
          LLRP_tSThingMagicDenatranIAVOBUWriteToMemMap  *pTMDenatranIAVOBUWriteToMemMap;
          LLRP_tSThingMagicDenatranIAVCommandRequest     *pTMDenatranIAVCommandRequest;

          args = &tagop->u.gen2.u.custom.u.IavDenatran.u.obuWriteToMemMap;

          /* Construct and Initialize the TMDenatranIAV OBU WriteToMemMap OpSpec */
          pTMDenatranIAVOBUWriteToMemMap = LLRP_ThingMagicDenatranIAVOBUWriteToMemMap_construct();
          /* Construct and Initialize the TMDenatranIAV Command request */
          pTMDenatranIAVCommandRequest = LLRP_ThingMagicDenatranIAVCommandRequest_construct();
          /* Set the OpSpecId to the TM Denatran IAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setOpSpecID(pTMDenatranIAVCommandRequest,
              reader->u.llrpReader.opSpecId);
          /* Set the DenatranIAV Payload value to the TM DenatranIAV Command request */
          LLRP_ThingMagicDenatranIAVCommandRequest_setPayLoad(pTMDenatranIAVCommandRequest,
              args->payload);
          /* Set the write pointer to the TM DenatranIAV command write to mem map */
          LLRP_ThingMagicDenatranIAVOBUWriteToMemMap_setWritePtr(pTMDenatranIAVOBUWriteToMemMap,
              args->writePtr);
          /* Set the data to be written to the TM DenatranIAV Command writeToMemMap */
          LLRP_ThingMagicDenatranIAVOBUWriteToMemMap_setWordData(pTMDenatranIAVOBUWriteToMemMap,
              args->wordData);

          {
            /**
             * Here is length of the tag identification is predefined to 
             * be 8 bytes. allocate that much of memory only
             **/
            llrp_u8v_t temp; 
            uint8_t len = 8;

            temp = LLRP_u8v_construct((llrp_u16_t)len);
            memcpy(temp.pValue, args->dataBuf, (size_t)temp.nValue);
            LLRP_ThingMagicDenatranIAVOBUWriteToMemMap_setTagIdentification(pTMDenatranIAVOBUWriteToMemMap,
                temp);
          }

          {
            /** 
             * Here is length is predefined to be 16 bytes,
             * allocate that much of memory only
             **/
            llrp_u8v_t temp; 
            uint8_t len = 16;

            temp = LLRP_u8v_construct((llrp_u16_t)len);
            memcpy(temp.pValue, args->dataBuf, (size_t)temp.nValue);
            LLRP_ThingMagicDenatranIAVOBUWriteToMemMap_setDataBuf(pTMDenatranIAVOBUWriteToMemMap,
                temp);
          }

          /* Set the DenatranIAV Command request to the TM DenatranIAV ActivateSecureMode */
          LLRP_ThingMagicDenatranIAVOBUWriteToMemMap_setThingMagicDenatranIAVCommandRequest(pTMDenatranIAVOBUWriteToMemMap,
              pTMDenatranIAVCommandRequest);

          /**
           * Set TMDenatranIAV ActivateecureMode as opSpec to accessSpec
           */
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMDenatranIAVOBUWriteToMemMap);
          break;
        }

#ifdef TMR_ENABLE_ISO180006B
      case TMR_TAGOP_ISO180006B_READDATA:
        {
          TMR_TagOp_ISO180006B_ReadData    *args;
          LLRP_tSThingMagicISO180006BRead  *pTMisoRead;
          llrp_u16_t                        data;

          args = &tagop->u.iso180006b.u.readData;

          /* Construct and initialize TMISO Read opspec */
          pTMisoRead = LLRP_ThingMagicISO180006BRead_construct();
          /* Set OpSpecId */
          LLRP_ThingMagicISO180006BRead_setOpSpecID(pTMisoRead, reader->u.llrpReader.opSpecId);
          /* Set the byte address */
          /* As API  datatype is uint8_t but
           * llrp takes  of type uint16_t
           * so, mapping the uint8_t data into uint16_t
           */
          data = args->byteAddress;
          LLRP_ThingMagicISO180006BRead_setByteAddress(pTMisoRead, data);

          /* Set the byte len */
          data = args->len;
          LLRP_ThingMagicISO180006BRead_setByteLen(pTMisoRead, data);

          /**
           * Set TMisoRead as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMisoRead);
          break;
        }
       
      case TMR_TAGOP_ISO180006B_WRITEDATA:
        {
          TMR_TagOp_ISO180006B_WriteData    *args;
          LLRP_tSThingMagicISO180006BWrite  *pTMisoWriteData;
          llrp_u8v_t                         writeData;
          llrp_u16_t                         data;

          args = &tagop->u.iso180006b.u.writeData;

          /* Construct and initialize the TM iso write data */
          pTMisoWriteData = LLRP_ThingMagicISO180006BWrite_construct();
          /*  Set the OpSpec id */
          LLRP_ThingMagicISO180006BWrite_setOpSpecID(pTMisoWriteData, reader->u.llrpReader.opSpecId);
          /* Set the byte addresss */
          /* As API datatype  is uint8_t but
           * llrp takes  of type uint16_t
           * so, mapping the uint8_t data into uint16_t
           **/
          data = args->byteAddress;
          LLRP_ThingMagicISO180006BWrite_setByteAddress(pTMisoWriteData, data);
          /* set the data to be written */
          writeData = LLRP_u8v_construct(args->data.len);
          memcpy(writeData.pValue, args->data.list, writeData.nValue);
          LLRP_ThingMagicISO180006BWrite_setWriteData(pTMisoWriteData,
              writeData);

          /**
           * Set TMisoRead as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMisoWriteData);

          break;
        }
        
      case TMR_TAGOP_ISO180006B_LOCK:
        {
          TMR_TagOp_ISO180006B_Lock          *args;
          LLRP_tSThingMagicISO180006BLock    *pTMisoLock;

          args = &tagop->u.iso180006b.u.lock;

          /* Construct and initialize the TM iso write data */
          pTMisoLock = LLRP_ThingMagicISO180006BLock_construct();
          /*  Set the OpSpec id */
          LLRP_ThingMagicISO180006BLock_setOpSpecID(pTMisoLock, reader->u.llrpReader.opSpecId);
          /* Set the address to lock */
          LLRP_ThingMagicISO180006BLock_setAddress(pTMisoLock, args->address);

          /**
           * Set TMisoLock as OpSpec to accessSpec
           **/
          LLRP_AccessCommand_addAccessCommandOpSpec(pAccessCommand,
              (LLRP_tSParameter *)pTMisoLock);

          break;
        }
#endif /* TMR_ENABLE_ISO180006B */
      case TMR_TAGOP_LIST:
        {
          /**
           * We support only one OpSpec per AccessSpec.
           * So return unsupported error.
           **/
          return TMR_ERROR_UNSUPPORTED;
        }

      default:
        {
          /* Unknown tagop - return invalid error */
          return TMR_ERROR_INVALID; 
        }
    }
  }

  return ret;
}

/**
 * Command to Add an AccessSpec
 *
 * @param reader Reader pointer
 * @param protocol Protocol to be used
 * @param filter Pointer to Tag filter
 * @param roSpecId ROSpecID with which this AccessSpec need to be associated
 * @param tagop Pointer to TMR_TagOp
 * @param isStandalone Boolean variable to indicate whether a standalone
 *        or embedded operation.
 *        true = standalone operation, false = embedded operation.
 */ 
TMR_Status
TMR_LLRP_cmdAddAccessSpec(TMR_Reader *reader, 
                          TMR_TagProtocol protocol,
                          TMR_TagFilter *filter,
                          llrp_u32_t roSpecId,
                          TMR_TagOp *tagop,
                          bool isStandalone)
{
  TMR_Status ret;
  LLRP_tSADD_ACCESSSPEC               *pCmd;
  LLRP_tSMessage                      *pCmdMsg;
  LLRP_tSMessage                      *pRspMsg;
  LLRP_tSADD_ACCESSSPEC_RESPONSE      *pRsp;

  LLRP_tSAccessSpec                   *pAccessSpec;
  LLRP_tSAccessSpecStopTrigger        *pAccessSpecStopTrigger;
  LLRP_tSAccessCommand                *pAccessCommand;

  ret = TMR_SUCCESS;
  
  /**
   * Initialize AddAccessSpec message
   **/
  pCmd = LLRP_ADD_ACCESSSPEC_construct();

  /**
   *  Initialize AccessSpec and
   *  1. Set AccessSpec Id
   *  2. Set Antenna Id
   *  3. Set current state
   *  4. Set Protocol
   *  5. Set ROSpec Id
   *  6. Set AccessSpec Stop trigger
   *  7. Set Access Command operation
   *  8. Set AccessReportSpec
   **/

  /* Construct AccessSpec parameter */
  pAccessSpec = LLRP_AccessSpec_construct();
  if (NULL == pAccessSpec)
  {
    TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
    return TMR_ERROR_LLRP;
  }

  {
    /* 1. Set AccessSpec Id */
    LLRP_AccessSpec_setAccessSpecID(pAccessSpec, reader->u.llrpReader.accessSpecId);
    
    /* 2. Set Antenna Id */
    /**
     * In case of both embedded tag operation,
     * the antenna list is adjusted in
     * ROSpec's AISpec. And here set antenna Id to 0, so that this 
     * spec is operational on all antennas mentioned in AISpec.
     **/
    if (isStandalone)
    {
      /**
       * For standalone tag operation prepare antennaList
       * The operation has to be performed on the antenna specified
       * in the /reader/tagop/antenna parameter.
       **/ 
      uint8_t antenna = 0;

      antenna = reader->tagOpParams.antenna;
      LLRP_AccessSpec_setAntennaID(pAccessSpec, antenna);
    }
    else
    {
      LLRP_AccessSpec_setAntennaID(pAccessSpec, 0);
    }

    /* 3. Set Current State */
    LLRP_AccessSpec_setCurrentState(pAccessSpec, 
                                LLRP_AccessSpecState_Disabled);

    /* 4. Set Protocol */
    switch (protocol)
    {
      case TMR_TAG_PROTOCOL_GEN2:
        {
          LLRP_AccessSpec_setProtocolID(pAccessSpec,
              LLRP_AirProtocols_EPCGlobalClass1Gen2);

          break;
        }
      case TMR_TAG_PROTOCOL_ISO180006B:
        {
          LLRP_AccessSpec_setProtocolID(pAccessSpec,
              LLRP_AirProtocols_Unspecified);

          break;
        }
      default:
        {
          TMR_LLRP_freeMessage((LLRP_tSMessage *)pAccessSpec);
          TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
          return TMR_ERROR_UNIMPLEMENTED_FEATURE;

          break;
        }
    }


    /* 5. Set ROSpec ID */
    LLRP_AccessSpec_setROSpecID(pAccessSpec, roSpecId); 

    /* 6. Set AccessSpec Stop trigger */
    {
      /* Construct and initialize AccessSpec stop trigger */
      pAccessSpecStopTrigger = LLRP_AccessSpecStopTrigger_construct();

      /**
       * Set AccessSpec StopTrigger type to operation count
       **/
      LLRP_AccessSpecStopTrigger_setAccessSpecStopTrigger(pAccessSpecStopTrigger,
                              LLRP_AccessSpecStopTriggerType_Operation_Count);

      /**
       * If stand alone operation, set operation count to 1.
       * else set it to 0 (i.e., No stop trigger defined)
       **/
      if (isStandalone)
      {
        /**
         * In case of standalone operation, we want to execute this operation
         * only once on the first matching tag. So set operation count to 1
         **/
        LLRP_AccessSpecStopTrigger_setOperationCountValue(pAccessSpecStopTrigger, 1);
      }
      else
      {
        /**
         * In case of embedded operation, the operation count is set 0.
         * i.e., perform this operation on as many tags found during the
         * inventory.
         **/
        LLRP_AccessSpecStopTrigger_setOperationCountValue(pAccessSpecStopTrigger, 0);
      }

      /* Set stoptrigger to accessspec */
      LLRP_AccessSpec_setAccessSpecStopTrigger(pAccessSpec, pAccessSpecStopTrigger);
    }

    /* 7. Set Access Command operation */
    {
      /* Initialize AccessCommand parameter */
      pAccessCommand = LLRP_AccessCommand_construct();

      /* prepare TagSpec and OpSpec and add them to AccessCommand */
      ret = TMR_LLRP_msgPrepareAccessCommand(reader, pAccessCommand, filter, tagop);
      if (TMR_SUCCESS != ret)
      {
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pAccessCommand);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pAccessSpec);
        TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
        return ret;
      }

      /* Set AccessCommand to AccessSpec parameter */
      LLRP_AccessSpec_setAccessCommand(pAccessSpec, pAccessCommand);
    }

    /* 8. Set AccessReportSpec */
    /**
     * AccessReportSpec is optional, and the default value 
     * (i.e., Whenever ROReport is generated for the RO that
     * triggered the execution of this AccessSpec)
     * is what we want to set in any way. So nothing to do here.
     **/
  }

  /* Now AccessSpec is fully framed, add to ADD_ACCESSSPEC message */
  LLRP_ADD_ACCESSSPEC_setAccessSpec(pCmd, pAccessSpec);

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSADD_ACCESSSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

/**
 * Parse ThingMagic Custom TagOpSpec Result parameter status
 * @param status The Result type parameter
 **/
TMR_Status
TMR_LLRP_parseCustomTagOpSpecResultType(LLRP_tEThingMagicCustomTagOpSpecResultType status)
{
  switch (status)
  {
    case LLRP_ThingMagicCustomTagOpSpecResultType_Success:
      {
        return TMR_SUCCESS;
      }

    case LLRP_ThingMagicCustomTagOpSpecResultType_Nonspecific_Tag_Error:
      {
        return TMR_ERROR_GENERAL_TAG_ERROR;
      }

    case LLRP_ThingMagicCustomTagOpSpecResultType_No_Response_From_Tag:
      {
        return TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
      }

    case LLRP_ThingMagicCustomTagOpSpecResultType_Nonspecific_Reader_Error:
      {
        return TMR_ERROR_LLRP_READER_ERROR;
      }

    default:
      return TMR_ERROR_LLRP;
  }
}

/**
 * Verify OpSpecResult status and return
 * appropriate API status code.
 *
 * @param reader Reader pointer
 * @param pParameter Pointer to LLRP_tSParameter which point to OpSpecResult
 **/
 
TMR_Status
TMR_LLRP_verifyOpSpecResultStatus(TMR_Reader *reader, 
                                  LLRP_tSParameter *pParameter)
{
  TMR_Status ret;

  ret = TMR_SUCCESS;

  /**
   * Compare the OpSpec Results based on the typenums
   **/
  switch (pParameter->elementHdr.pType->TypeNum)
  {

    /**
     * Always 0 means success, if eResult is other than zero
     * then it is an error, translate the error into appropriate
     * API error.
     **/

    /**
     * C1G2ReadOpSpecResult
     **/
    case TMR_LLRP_C1G2READOPSPECRESULT:
      {
        LLRP_tSC1G2ReadOpSpecResult *pC1G2ReadOpSpecResult;
        pC1G2ReadOpSpecResult = (LLRP_tSC1G2ReadOpSpecResult *)pParameter;

        switch (pC1G2ReadOpSpecResult->eResult)
        {
          case LLRP_C1G2ReadResultType_Success:
            {
              ret = TMR_SUCCESS;
              break;
            }

          case LLRP_C1G2ReadResultType_Nonspecific_Tag_Error:
            {
              ret = TMR_ERROR_GENERAL_TAG_ERROR;
              break;
            }

          case LLRP_C1G2ReadResultType_No_Response_From_Tag:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
              break;
            }

          case LLRP_C1G2ReadResultType_Nonspecific_Reader_Error:
            {
              ret = TMR_ERROR_LLRP_READER_ERROR;
              break;
            }

          case LLRP_C1G2ReadResultType_Memory_Overrun_Error:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC;
              break;
            }

          case LLRP_C1G2ReadResultType_Memory_Locked_Error:
            {
              ret = TMR_ERROR_GEN2_PROCOCOL_MEMORY_LOCKED;
              break;
            }

          default:
            ret = TMR_ERROR_LLRP;
        }
        break;
      }

    /**
     * C1G2WriteOpSpecResult
     **/
    case TMR_LLRP_C1G2WRITEOPSPECRESULT:
      {
        LLRP_tSC1G2WriteOpSpecResult *pC1G2WriteOpSpecResult;
        pC1G2WriteOpSpecResult = (LLRP_tSC1G2WriteOpSpecResult *)pParameter;

        switch (pC1G2WriteOpSpecResult->eResult)
        {
          case LLRP_C1G2WriteResultType_Success:
            {
              ret = TMR_SUCCESS;
              break;
            }

          case LLRP_C1G2WriteResultType_Tag_Memory_Overrun_Error:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC;
              break;
            }

          case LLRP_C1G2WriteResultType_Tag_Memory_Locked_Error:
            {
              ret = TMR_ERROR_GEN2_PROCOCOL_MEMORY_LOCKED;
              break;
            }

          case LLRP_C1G2WriteResultType_Insufficient_Power:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_INSUFFICIENT_POWER;
              break;
            }

          case LLRP_C1G2WriteResultType_Nonspecific_Tag_Error:
            {
              ret = TMR_ERROR_GENERAL_TAG_ERROR;
              break;
            }

          case LLRP_C1G2WriteResultType_No_Response_From_Tag:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
              break;
            }

          case LLRP_C1G2WriteResultType_Nonspecific_Reader_Error:
            {
              ret = TMR_ERROR_LLRP_READER_ERROR;
              break;
            }

          default:
            ret = TMR_ERROR_LLRP;
        }
        break;
      }

    /**
     * C1G2KillOpSpecResult
     **/
    case TMR_LLRP_C1G2KILLOPSPECRESULT:
      {
        LLRP_tSC1G2KillOpSpecResult *pC1G2KillOpSpecResult;
        pC1G2KillOpSpecResult = (LLRP_tSC1G2KillOpSpecResult *)pParameter;

        switch (pC1G2KillOpSpecResult->eResult)
        {
          case LLRP_C1G2KillResultType_Success:
            {
              ret = TMR_SUCCESS;
              break;
            }

          case LLRP_C1G2KillResultType_Zero_Kill_Password_Error:
            {
              ret = TMR_ERROR_PROTOCOL_INVALID_KILL_PASSWORD;
              break;
            }

          case LLRP_C1G2KillResultType_Insufficient_Power:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_INSUFFICIENT_POWER;
              break;
            }

          case LLRP_C1G2KillResultType_Nonspecific_Tag_Error:
            {
              ret = TMR_ERROR_GENERAL_TAG_ERROR;
              break;
            }

          case LLRP_C1G2KillResultType_No_Response_From_Tag:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
              break;
            }

          case LLRP_C1G2KillResultType_Nonspecific_Reader_Error:
            {
              ret = TMR_ERROR_LLRP_READER_ERROR;
              break;
            }

          default:
            ret = TMR_ERROR_LLRP;
        }
        break;
      }

    /**
     * C1G2LockOpSpecResult
     **/
    case TMR_LLRP_C1G2LOCKOPSPECRESULT:
      {
        LLRP_tSC1G2LockOpSpecResult *pC1G2LockOpSpecResult;
        pC1G2LockOpSpecResult = (LLRP_tSC1G2LockOpSpecResult *)pParameter;

        switch (pC1G2LockOpSpecResult->eResult)
        {
          case LLRP_C1G2LockResultType_Success:
            {
              ret = TMR_SUCCESS;
              break;
            }

          case LLRP_C1G2LockResultType_Insufficient_Power:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_INSUFFICIENT_POWER;
              break;
            }

          case LLRP_C1G2LockResultType_Nonspecific_Tag_Error:
            {
              ret = TMR_ERROR_GENERAL_TAG_ERROR;
              break;
            }

          case LLRP_C1G2LockResultType_No_Response_From_Tag:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
              break;
            }

          case LLRP_C1G2LockResultType_Nonspecific_Reader_Error:
            {
              ret = TMR_ERROR_LLRP_READER_ERROR;
              break;
            }

          default:
            ret = TMR_ERROR_LLRP;
        }
        break;
      }

    /**
     * C1G2BlockEraseOpSpecResult
     **/
    case TMR_LLRP_C1G2BLOCKERASEOPSPECRESULT:
      {
        LLRP_tSC1G2BlockEraseOpSpecResult *pC1G2BlockEraseOpSpecResult;
        pC1G2BlockEraseOpSpecResult = (LLRP_tSC1G2BlockEraseOpSpecResult *)pParameter;

        switch (pC1G2BlockEraseOpSpecResult->eResult)
        {
          case LLRP_C1G2BlockEraseResultType_Success:
            {
              ret = TMR_SUCCESS;
              break;
            }

          case LLRP_C1G2BlockEraseResultType_Tag_Memory_Overrun_Error:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC;
              break;
            }

          case LLRP_C1G2BlockEraseResultType_Tag_Memory_Locked_Error:
            {
              ret = TMR_ERROR_GEN2_PROCOCOL_MEMORY_LOCKED;
              break;
            }

          case LLRP_C1G2BlockEraseResultType_Insufficient_Power:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_INSUFFICIENT_POWER;
              break;
            }

          case LLRP_C1G2BlockEraseResultType_Nonspecific_Tag_Error:
            {
              ret = TMR_ERROR_GENERAL_TAG_ERROR;
              break;
            }

          case LLRP_C1G2BlockEraseResultType_No_Response_From_Tag:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
              break;
            }

          case LLRP_C1G2BlockEraseResultType_Nonspecific_Reader_Error:
            {
              ret = TMR_ERROR_LLRP_READER_ERROR;
              break;
            }

          default:
            ret = TMR_ERROR_LLRP;
        }
        break;
      }

    /**
     * C1G2BlockWriteOpSpecResult
     **/
    case TMR_LLRP_C1G2BLOCKWRITEOPSPECRESULT:
      {
        LLRP_tSC1G2BlockWriteOpSpecResult *pC1G2BlockWriteOpSpecResult;
        pC1G2BlockWriteOpSpecResult = (LLRP_tSC1G2BlockWriteOpSpecResult *)pParameter;

        switch (pC1G2BlockWriteOpSpecResult->eResult)
        {
          case LLRP_C1G2BlockWriteResultType_Success:
            {
              ret = TMR_SUCCESS;
              break;
            }

          case LLRP_C1G2BlockWriteResultType_Tag_Memory_Overrun_Error:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC;
              break;
            }

          case LLRP_C1G2BlockWriteResultType_Tag_Memory_Locked_Error:
            {
              ret = TMR_ERROR_GEN2_PROCOCOL_MEMORY_LOCKED;
              break;
            }

          case LLRP_C1G2BlockWriteResultType_Insufficient_Power:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_INSUFFICIENT_POWER;
              break;
            }

          case LLRP_C1G2BlockWriteResultType_Nonspecific_Tag_Error:
            {
              ret = TMR_ERROR_GENERAL_TAG_ERROR;
              break;
            }

          case LLRP_C1G2BlockWriteResultType_No_Response_From_Tag:
            {
              ret = TMR_ERROR_GEN2_PROTOCOL_OTHER_ERROR;
              break;
            }

          case LLRP_C1G2BlockWriteResultType_Nonspecific_Reader_Error:
            {
              ret = TMR_ERROR_LLRP_READER_ERROR;
              break;
            }

          default:
            ret = TMR_ERROR_LLRP;
        }
        break;
      }

    /**
     * ThingMagicBlockPermalock result.
     **/
    case TMR_LLRP_CUSTOM_BLOCKPERMALOCKOPSPECRESULT:
      {
        LLRP_tSThingMagicBlockPermalockOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicBlockPermalockOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * Higgs2PartialLoadImage result.
     **/
    case TMR_LLRP_CUSTOM_HIGGS2PARTIALLOADIMAGEOPSPECRESULT:
      {
        LLRP_tSThingMagicHiggs2PartialLoadImageOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicHiggs2PartialLoadImageOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

       break;
      }

    /**
     * Higgs2FullLoadImage result
     **/
    case TMR_LLRP_CUSTOM_HIGGS2FULLLOADIMAGEOPSPECRESULT:
      {
        LLRP_tSThingMagicHiggs2FullLoadImageOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicHiggs2FullLoadImageOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * Higgs3FastLoadImage result
     **/
    case TMR_LLRP_CUSTOM_HIGGS3FASTLOADIMAGEOPSPECRESULT:
      {
        LLRP_tSThingMagicHiggs3FastLoadImageOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicHiggs3FastLoadImageOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * Higgs3LoadImage result
     **/
    case TMR_LLRP_CUSTOM_HIGGS3LOADIMAGEOPSPECRESULT:
      {
        LLRP_tSThingMagicHiggs3LoadImageOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicHiggs3LoadImageOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * Higgs3BlockReadLock result
     **/
    case TMR_LLRP_CUSTOM_HIGGS3BLOCKREADLOCKOPSPECRESULT:
      {
        LLRP_tSThingMagicHiggs3BlockReadLockOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicHiggs3BlockReadLockOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * G2ISetReadProtect result
     **/
    case TMR_LLRP_CUSTOM_G2ISETREADPROTECTOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2ISetReadProtectOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2ISetReadProtectOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2XSetReadProtect result
     **/
    case TMR_LLRP_CUSTOM_G2XSETREADPROTECTOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XSetReadProtectOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2XSetReadProtectOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2IResetReadProtect result
     **/
    case TMR_LLRP_CUSTOM_G2IRESETREADPROTECTOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2IResetReadProtectOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2IResetReadProtectOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2XResetReadProtect result
     **/
    case TMR_LLRP_CUSTOM_G2XRESETREADPROTECTOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XResetReadProtectOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2XResetReadProtectOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2IChangeEAS result
     **/
    case TMR_LLRP_CUSTOM_G2ICHANGEEASOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2IChangeEASOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2IChangeEASOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2XChangeEAS result
     **/
    case TMR_LLRP_CUSTOM_G2XCHANGEEASOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XChangeEASOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2XChangeEASOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2IEASAlarm result
     **/
    case TMR_LLRP_CUSTOM_G2IEASALARMOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2IEASAlarmOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2IEASAlarmOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2XEASAlarm result
     **/
    case TMR_LLRP_CUSTOM_G2XEASALARMOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XEASAlarmOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2XEASAlarmOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2ICalibrate result
     **/
    case TMR_LLRP_CUSTOM_G2ICALIBRATEOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2ICalibrateOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2ICalibrateOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * G2XCalibrate result
     **/
    case TMR_LLRP_CUSTOM_G2XCALIBRATEOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2XCalibrateOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2XCalibrateOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * G2IChangeConfig result
     **/
    case TMR_LLRP_CUSTOM_G2ICHANGECONFIGOPSPECRESULT:
      {
        LLRP_tSThingMagicNXPG2IChangeConfigOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicNXPG2IChangeConfigOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
 
    /**
     * Monza4QTReadWrite result
     **/
    case TMR_LLRP_CUSTOM_MONZA4QTREADWRITEOPSPECRESULT:
      {
        LLRP_tSThingMagicImpinjMonza4QTReadWriteOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicImpinjMonza4QTReadWriteOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

     /**
      * ThingMagicSetSfeParameters Result
      */
    case TMR_LLRP_CUSTOM_IDS_SETSFEPARAMETERSOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ASetSFEParamsOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900ASetSFEParamsOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * ThingMagicGetMeasurementSetup result
     */
    case TMR_LLRP_CUSTOM_IDS_GETMEASUREMENTSETUPOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AGetMeasurementSetupOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AGetMeasurementSetupOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * ThingMagicGetBatteryLevel result
     */
    case TMR_LLRP_CUSTOM_IDS_GETBATTERYLEVELOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AGetBatteryLevelOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AGetBatteryLevelOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * ThingMagicSetLogLimits result
     */
    case TMR_LLRP_CUSTOM_IDS_SETLOGLIMITSOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ASetLogLimitsOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900ASetLogLimitsOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    /**
     * ThingMagicSetShelfLife result
     */
    case TMR_LLRP_CUSTOM_IDS_SETSHELFLIFEOPSPECRESULT:
    {
    	LLRP_tSThingMagicIDSSetShelfLifeOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicIDSSetShelfLifeOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagicSetPassword result
     */
    case TMR_LLRP_CUSTOM_IDS_SETPASSWORDOPSPECRESULT:
    {
      LLRP_tSThingMagicIDSSL900ASetPasswordOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicIDSSL900ASetPasswordOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

     /**
      * ThingMagicWriteTag result
      **/
    case TMR_LLRP_CUSTOM_WRITETAGOPSPECRESULT:
      {
        LLRP_tSThingMagicWriteTagOpSpecResult  *pResult;
        pResult = (LLRP_tSThingMagicWriteTagOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSGetSensor result
       */
    case TMR_LLRP_CUSTOM_IDS_GETSENSORVALUEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ASensorValueOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900ASensorValueOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSSetLogMode result
       */
    case TMR_LLRP_CUSTOM_IDS_SETLOGMODEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ASetLogModeOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900ASetLogModeOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSStartLog result
       */
    case TMR_LLRP_CUSTOM_IDS_STARTLOGMODEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AStartLogOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AStartLogOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSGetLogState result
       */
    case TMR_LLRP_CUSTOM_IDS_GETLOGSTATEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900ALogStateOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900ALogStateOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSEndLog result
       */
    case TMR_LLRP_CUSTOM_IDS_ENDLOGOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AEndLogOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AEndLogOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSInitialize result
       */
    case TMR_LLRP_CUSTOM_IDS_INITIALIZEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AInitializeOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AInitializeOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicIDSAccessFifoStatus result
       */
    case TMR_LLRP_CUSTOM_IDS_ACCESSFIFOSTATUSOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AAccessFIFOStatusOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AAccessFIFOStatusOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicAccessFifoWrite result
       */
    case TMR_LLRP_CUSTOM_IDS_ACCESSFIFOWRITEOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AAccessFIFOWriteOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AAccessFIFOWriteOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicAccessFifoRead result
       */
    case TMR_LLRP_CUSTOM_IDS_ACCESSFIFOREADOPSPECRESULT:
      {
        LLRP_tSThingMagicIDSSL900AAccessFIFOReadOpSpecResult *pResult;
        pResult = (LLRP_tSThingMagicIDSSL900AAccessFIFOReadOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

      /**
       * ThingMagicGetCalibrationData result
       */
    case TMR_LLRP_CUSTOM_IDS_GETCALIBRATIONDATAOPSPECRESULT:
    {
    	LLRP_tSThingMagicIDSSL900AGetCalibrationDataOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicIDSSL900AGetCalibrationDataOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagicSetCalibrationData result
     */
    case TMR_LLRP_CUSTOM_IDS_SETCALIBRATIONDATAOPSPECRESULT:
    {
    	LLRP_tSThingMagicIDSSL900ASetCalibrationDataOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicIDSSL900ASetCalibrationDataOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV ActivateSecureMode result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_ACTIVATESECUREMODEOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVActivateSecureModeOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVActivateSecureModeOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV AuthenticateOBU result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEOBUOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVAuthenticateOBUOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVAuthenticateOBUOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV ActivateSiniavMode result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_ACTIVATESINIAVMODEOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVActivateSiniavModeOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVActivateSiniavModeOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV OBUAuthenticateID result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEIDOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUAuthenticateIDOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVOBUAuthenticateIDOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV AuthenticateOBU FullPass1 result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEFULLPASS1OPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass1OpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass1OpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV AuthenticateOBU FullPass2 result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_AUTHENTICATEFULLPASS2OPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass2OpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVOBUAuthenticateFullPass2OpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV OBU ReadFromMEM result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_OBUREADFROMMEMMAPOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUReadFromMemMapOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVOBUReadFromMemMapOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }

    /**
     * ThingMagic DenatranIAV OBU WriteToMem result
     */
    case TMR_LLRP_CUSTOM_DENATRAN_IAV_OBUWRITETOMEMMAPOPSPECRESULT:
    {
      LLRP_tSThingMagicDenatranIAVOBUWriteToMemMapOpSpecResult *pResult;
      pResult = (LLRP_tSThingMagicDenatranIAVOBUWriteToMemMapOpSpecResult *)pParameter;
      ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

      break;
    }


#ifdef TMR_ENABLE_ISO180006B
    case TMR_LLRP_CUSTOM_ISO_READDATAOPSPECRESULT:
      {
        LLRP_tSThingMagicISO180006BReadOpSpecResult  *pResult;
        pResult = (LLRP_tSThingMagicISO180006BReadOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }

    case TMR_LLRP_CUSTOM_ISO_WRITEDATAOPSPECRESULT:
      {
        LLRP_tSThingMagicISO180006BWriteOpSpecResult  *pResult;
        pResult = (LLRP_tSThingMagicISO180006BWriteOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
    case TMR_LLRP_CUSTOM_ISO_LOCKOPSPECRESULT:
      {
        LLRP_tSThingMagicISO180006BLockOpSpecResult  *pResult;
        pResult = (LLRP_tSThingMagicISO180006BLockOpSpecResult *)pParameter;
        ret = TMR_LLRP_parseCustomTagOpSpecResultType(pResult->eResult);

        break;
      }
#endif /* TMR_ENABLE_ISO180006B */

    default:
      {
        /**
         * I do not know what kind of OpSpecResult is this.
         * So returning msg parse error.
         **/
        ret = TMR_ERROR_LLRP_MSG_PARSE_ERROR;
      }
  }

  return ret;
}

/**
 * Command to delete all AccessSpecs on LLRP Reader
 *
 * @param reader Reader pointer
 */
TMR_Status
TMR_LLRP_cmdDeleteAllAccessSpecs(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSDELETE_ACCESSSPEC          *pCmd;
  LLRP_tSMessage                    *pCmdMsg;
  LLRP_tSMessage                    *pRspMsg;
  LLRP_tSDELETE_ACCESSSPEC_RESPONSE *pRsp;

  ret = TMR_SUCCESS;

  /**
   * Create delete accessspec message
   **/
  pCmd = LLRP_DELETE_ACCESSSPEC_construct();
  LLRP_DELETE_ACCESSSPEC_setAccessSpecID(pCmd, 0);        /* All */

  pCmdMsg = &pCmd->hdr;
  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSDELETE_ACCESSSPEC_RESPONSE *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Response is success, Done with the response message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  return ret;
}

TMR_Status
TMR_LLRP_cmdStopReading(struct TMR_Reader *reader)
{
  if (TMR_READ_PLAN_TYPE_SIMPLE == reader->readParams.readPlan->type)
  {
    /* receiveResponse = false, as we do not need here */
    return TMR_LLRP_cmdStopROSpec(reader, false);
  }
  else
  {
    /**
     * In case of multiple readplans, sending STOP_ROSPEC doesn't
     * actually stop the rospec execution, since their start trigger
     * was set to periodic based. So delete all rospecs.
     **/
    return TMR_LLRP_cmdDeleteAllROSpecs(reader, false);
  }
}

TMR_Status
TMR_LLRP_cmdGetReport(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSGET_REPORT *pCmd;
  LLRP_tSMessage    *pCmdMsg;

  ret = TMR_SUCCESS;

  /**
   * Initialize GET_REPORT message
   **/
  pCmd = LLRP_GET_REPORT_construct();
  pCmdMsg = &pCmd->hdr;
 
  /**
   * Response to GET_REPORT message will be RO_ACCESS_REPORTs,
   * which needs to be processed in other place.
   * Here we just send the message.
   **/
  ret = TMR_LLRP_sendMessage(reader, pCmdMsg, 
          reader->u.llrpReader.transportTimeout);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  return ret;
}

TMR_Status
TMR_LLRP_cmdrebootReader(TMR_Reader *reader)
{
  TMR_Status ret;
  LLRP_tSTHINGMAGIC_CONTROL_REQUEST_POWER_CYCLE_READER  *pCmd;
  LLRP_tSMessage                                        *pCmdMsg;
  LLRP_tSMessage                                        *pRspMsg;
  LLRP_tSTHINGMAGIC_CONTROL_RESPONSE_POWER_CYCLE_READER *pRsp;

  ret = TMR_SUCCESS;

  /**
   * Initialize Custom message THINGMAGIC_CONTROL_REQUEST_POWER_CYCLE_READER
   **/
  pCmd = LLRP_THINGMAGIC_CONTROL_REQUEST_POWER_CYCLE_READER_construct();

  /* Add the thingmagic magic number to recycle the reader */
  LLRP_THINGMAGIC_CONTROL_REQUEST_POWER_CYCLE_READER_setMagicNumber(pCmd, TMR_POWER_CYCLE_MAGIC_NUMBER);

  /* Add the safe mode option */
  LLRP_THINGMAGIC_CONTROL_REQUEST_POWER_CYCLE_READER_setBootToSafeMode(pCmd, false);

  pCmdMsg = &pCmd->hdr;

  /**
   * Now the message is framed completely and send the message
   **/
  ret = TMR_LLRP_send(reader, pCmdMsg, &pRspMsg);
  /**
   * Done with the command, free the message
   * and check for message status
   **/ 
  TMR_LLRP_freeMessage((LLRP_tSMessage *)pCmd);
  if (TMR_SUCCESS != ret)
  {
    return ret;
  }

  /**
   * Check response message status
   **/
  pRsp = (LLRP_tSTHINGMAGIC_CONTROL_RESPONSE_POWER_CYCLE_READER *) pRspMsg;
  if (TMR_SUCCESS != TMR_LLRP_checkLLRPStatus(pRsp->pLLRPStatus))  
  {
    TMR_LLRP_freeMessage(pRspMsg);
    return TMR_ERROR_LLRP; 
  }

  /**
   * Done with the response, free the message
   **/
  TMR_LLRP_freeMessage(pRspMsg);

  /**
   * Wait for the reader to  reboot. 90s is enough.
   */
  tmr_sleep(90000);

  return ret;
}
#endif  /* TMR_ENABLE_LLRP_READER */

