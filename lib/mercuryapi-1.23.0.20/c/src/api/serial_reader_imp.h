#ifndef _SERIAL_READER_IMP_H
#define _SERIAL_READER_IMP_H

/**
 *  @file serial_reader_imp.h
 *  @brief Serial reader internal implementation header
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
#include "tm_reader.h"
#include "tmr_status.h"

#ifdef  __cplusplus
extern "C" {
#endif

  
/* This is used to enable the Gen2 secure readdata option */
bool isSecureAccessEnabled ;

typedef enum TMR_SR_OpCode
{
  TMR_SR_OPCODE_WRITE_FLASH             = 0x01,
  TMR_SR_OPCODE_READ_FLASH              = 0x02,
  TMR_SR_OPCODE_VERSION                 = 0x03,
  TMR_SR_OPCODE_BOOT_FIRMWARE           = 0x04,
  TMR_SR_OPCODE_SET_BAUD_RATE           = 0x06,
  TMR_SR_OPCODE_ERASE_FLASH             = 0x07,
  TMR_SR_OPCODE_VERIFY_IMAGE_CRC        = 0x08,
  TMR_SR_OPCODE_BOOT_BOOTLOADER         = 0x09,
  TMR_SR_OPCODE_HW_VERSION              = 0x10,
  TMR_SR_OPCODE_MODIFY_FLASH            = 0x0A,
  TMR_SR_OPCODE_GET_DSP_SILICON_ID      = 0x0B,
  TMR_SR_OPCODE_GET_CURRENT_PROGRAM     = 0x0C,
  TMR_SR_OPCODE_WRITE_FLASH_SECTOR      = 0x0D,
  TMR_SR_OPCODE_GET_SECTOR_SIZE         = 0x0E,
  TMR_SR_OPCODE_MODIFY_FLASH_SECTOR     = 0x0F,
  TMR_SR_OPCODE_READ_TAG_ID_SINGLE      = 0x21,
  TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE    = 0x22,
  TMR_SR_OPCODE_WRITE_TAG_ID            = 0x23,
  TMR_SR_OPCODE_WRITE_TAG_DATA          = 0x24,
  TMR_SR_OPCODE_LOCK_TAG                = 0x25,
  TMR_SR_OPCODE_KILL_TAG                = 0x26,
  TMR_SR_OPCODE_READ_TAG_DATA           = 0x28,
  TMR_SR_OPCODE_GET_TAG_ID_BUFFER       = 0x29,
  TMR_SR_OPCODE_CLEAR_TAG_ID_BUFFER     = 0x2A,
  TMR_SR_OPCODE_WRITE_TAG_SPECIFIC      = 0x2D,
  TMR_SR_OPCODE_ERASE_BLOCK_TAG_SPECIFIC= 0x2E,
  TMR_SR_OPCODE_MULTI_PROTOCOL_TAG_OP   = 0x2F,
  TMR_SR_OPCODE_GET_ANTENNA_PORT        = 0x61,
  TMR_SR_OPCODE_GET_READ_TX_POWER       = 0x62,
  TMR_SR_OPCODE_GET_TAG_PROTOCOL        = 0x63,
  TMR_SR_OPCODE_GET_WRITE_TX_POWER      = 0x64,
  TMR_SR_OPCODE_GET_FREQ_HOP_TABLE      = 0x65,
  TMR_SR_OPCODE_GET_USER_GPIO_INPUTS    = 0x66,
  TMR_SR_OPCODE_GET_REGION              = 0x67,
  TMR_SR_OPCODE_GET_POWER_MODE          = 0x68,
  TMR_SR_OPCODE_GET_USER_MODE           = 0x69,
  TMR_SR_OPCODE_GET_READER_OPTIONAL_PARAMS=0x6A,
  TMR_SR_OPCODE_GET_PROTOCOL_PARAM      = 0x6B,
  TMR_SR_OPCODE_GET_READER_STATS        = 0x6C,
  TMR_SR_OPCODE_GET_USER_PROFILE        = 0x6D,
  TMR_SR_OPCODE_GET_AVAILABLE_PROTOCOLS = 0x70,
  TMR_SR_OPCODE_GET_AVAILABLE_REGIONS   = 0x71,
  TMR_SR_OPCODE_GET_TEMPERATURE         = 0x72,
  TMR_SR_OPCODE_SET_ANTENNA_PORT        = 0x91,
  TMR_SR_OPCODE_SET_READ_TX_POWER       = 0x92,
  TMR_SR_OPCODE_SET_TAG_PROTOCOL        = 0x93,
  TMR_SR_OPCODE_SET_WRITE_TX_POWER      = 0x94,
  TMR_SR_OPCODE_SET_FREQ_HOP_TABLE      = 0x95,
  TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS   = 0x96,
  TMR_SR_OPCODE_SET_REGION              = 0x97,
  TMR_SR_OPCODE_SET_POWER_MODE          = 0x98,
  TMR_SR_OPCODE_SET_USER_MODE           =  0x99,
  TMR_SR_OPCODE_SET_READER_OPTIONAL_PARAMS=0x9a,
  TMR_SR_OPCODE_SET_PROTOCOL_PARAM      = 0x9B,
  TMR_SR_OPCODE_SET_USER_PROFILE        = 0x9D,
  TMR_SR_OPCODE_SET_PROTOCOL_LICENSEKEY = 0x9E,
  TMR_SR_OPCODE_SET_OPERATING_FREQ      = 0xC1,
  TMR_SR_OPCODE_TX_CW_SIGNAL            = 0xC3,
}TMR_SR_OpCode;

typedef enum TMR_SR_Gen2SingulationOptions
{
  TMR_SR_GEN2_SINGULATION_OPTION_SELECT_DISABLED         = 0x00,
  TMR_SR_GEN2_SINGULATION_OPTION_SELECT_ON_EPC           = 0x01,
  TMR_SR_GEN2_SINGULATION_OPTION_SELECT_ON_TID           = 0x02,
  TMR_SR_GEN2_SINGULATION_OPTION_SELECT_ON_USER_MEM      = 0x03,
  TMR_SR_GEN2_SINGULATION_OPTION_SELECT_ON_ADDRESSED_EPC = 0x04,
  TMR_SR_GEN2_SINGULATION_OPTION_USE_PASSWORD            = 0x05,
  TMR_SR_GEN2_SINGULATION_OPTION_INVERSE_SELECT_BIT      = 0x08,
  TMR_SR_GEN2_SINGULATION_OPTION_FLAG_METADATA           = 0x10,
  TMR_SR_GEN2_SINGULATION_OPTION_EXTENDED_DATA_LENGTH    = 0x20,
  TMR_SR_GEN2_SINGULATION_OPTION_SECURE_READ_DATA        = 0x40
}TMR_SR_Gen2SingulationOptions;

typedef enum TMR_SR_TagidOption
{
  TMR_SR_TAG_ID_OPTION_NONE    = 0x00,
  TMR_SR_TAG_ID_OPTION_REWIND  = 0x01
}TMR_SR_TagidOption;

typedef enum TMR_SR_ModelHardwareID
{
  TMR_SR_MODEL_M5E         = 0x00,
  TMR_SR_MODEL_M5E_COMPACT = 0x01,
  TMR_SR_MODEL_M5E_I       = 0x02,
  TMR_SR_MODEL_M4E         = 0x03,
  TMR_SR_MODEL_M6E         = 0x18,
  TMR_SR_MODEL_M6E_PRC     = 0x19,
  TMR_SR_MODEL_M6E_MICRO   = 0x20,
  TMR_SR_MODEL_UNKNOWN     = 0xFF,
} TMR_SR_ModelHardwareID;

typedef enum TMR_SR_ModelM5EInternational
{
  TMR_SR_MODEL_M5E_I_REV_EU  = 0x01,
  TMR_SR_MODEL_M5E_I_REV_NA  = 0x02,
  TMR_SR_MODEL_M5E_I_REV_JP  = 0x03,
  TMR_SR_MODEL_M5E_I_REV_PRC = 0x04,
}TMR_SR_ModelM5EInternational;

typedef enum TMR_SR_ProductGroupID
{
  TMR_SR_PRODUCT_MODULE = 0,
  TMR_SR_PRODUCT_RUGGEDIZED_READER = 1,
  TMR_SR_PRODUCT_USB_READER = 2,
  TMR_SR_PRODUCT_INVALID = 0xFFFF,
}TMR_SR_ProductGroupID;

TMR_Status TMR_SR_sendTimeout(TMR_Reader *reader, uint8_t *data,
                              uint32_t timeoutMs);
TMR_Status TMR_SR_send(TMR_Reader *reader, uint8_t *data);
TMR_Status TMR_SR_sendMessage(TMR_Reader *reader, uint8_t *data,
                              uint8_t *opcode, uint32_t timeoutMs);
TMR_Status TMR_SR_receiveMessage(TMR_Reader *reader, uint8_t *data,
                                 uint8_t opcode, uint32_t timeoutMs);

void TMR_SR_parseMetadataFromMessage(TMR_Reader *reader, TMR_TagReadData *read, uint16_t flags,
                                     uint8_t *i, uint8_t msg[]);

void
TMR_SR_parseMetadataOnly(TMR_Reader *reader, TMR_TagReadData *read, uint16_t flags,
                                uint8_t *i, uint8_t msg[]);
void TMR_SR_postprocessReaderSpecificMetadata(TMR_TagReadData *read,
                                              TMR_SR_SerialReader *sr);

/**
 * This structure is returned from read tag multiple embedded commands.
 */
typedef struct TMR_SR_MultipleStatus
{
  /** The number of tags found during the read. */
  uint16_t tagsFound;
  /** The number of tags for which the embedded operation succeeded. */
  uint16_t successCount;
  /** The number of tags for which the embedded operation failed. */
  uint16_t failureCount;
}TMR_SR_MultipleStatus;

/**
 * This structure is returned from TMR_SR_cmdGetTxRxPorts and
 * TMR_SR_cmdGetAntennaSearchList, and passed as a parameter to
 * TMR_SR_cmdSetAntennaSearchList.
 */
typedef struct TMR_SR_PortPair
{
  /** The transmit port. */
  uint8_t txPort;
  /** The receive port. */
  uint8_t rxPort;
}TMR_SR_PortPair;

/**
 * This structure is returned from TMR_SR_cmdAntennaDetect.
 */
typedef struct TMR_SR_PortDetect
{
  /** The port number. */
  uint8_t port;
  /** Whether an antenna was detected on the port. */
  bool detected;
}TMR_SR_PortDetect;

/**
 * Reader statistics options enum
 */ 
typedef enum TMR_SR_ReaderStatsOption
{
  /* Get statistics specified by the statistics flag */
  TMR_SR_READER_STATS_OPTION_GET=   0x00,
  /* Reset the specified statistic */
  TMR_SR_READER_STATS_OPTION_RESET= 0x01,
  /* Get the per-port statistics specified by the statistics flag*/
  TMR_SR_READER_STATS_OPTION_GET_PER_PORT = 0x02,
}TMR_SR_ReaderStatsOption;
  

/**
 *  Reader Statistics Flag Enum
 */
typedef enum TMR_SR_ReaderStatisticsFlag
{
  /* Total time the port has been transmitting, in milliseconds. Resettable */
  TMR_SR_READER_STATS_FLAG_RF_ON_TIME     = (1<<0),
  /* Detected noise floor with transmitter off. Recomputed when requested, not resettable.*/
  TMR_SR_READER_STATS_FLAG_NOISE_FLOOR    = (1<<1),
  /* Detected noise floor with transmitter on. Recomputed when requested, not resettable. */
  TMR_SR_READER_STATS_FLAG_NOISE_FLOOR_TX_ON = (1<<3),
  /* ALL */
  TMR_SR_READER_STATS_ALL = (TMR_SR_READER_STATS_FLAG_RF_ON_TIME | 
      TMR_SR_READER_STATS_FLAG_NOISE_FLOOR |
      TMR_SR_READER_STATS_FLAG_NOISE_FLOOR_TX_ON),
}TMR_SR_ReaderStatisticsFlag;

/**
 * Antenna Configuration. Returned by TMR_SR_cmdGetAntennaConfiguration.
 */
typedef struct TMR_SR_AntennaPort
{
  /** The number of physical antenna ports. */
  uint8_t numPorts;
  /** The current logical Transmit port. */
  uint8_t txPort;
  /** The current logical Recieve port. */
  uint8_t rxPort;
  /** List specifying what ports are terminated. */
  TMR_uint8List portTerminatedList;
}TMR_SR_AntennaPort;

/** Per-port power levels. Used by TMR_SR_cmd{Get,Set}AntennaPortPowers */
typedef struct TMR_SR_PortPower
{
  /** The port number. */
  uint8_t port;
  /** The power level to use for read operations, in centidBm. */
  uint16_t readPower;
  /** The power level to use for write operations, in centidBm. */
  uint16_t writePower;
} TMR_SR_PortPower;

/**
 * Per-port power levels and settling times. Used by
 * TMR_SR_cmd{Get,Set}AntennaPortPowersAndSettlingTime.
 */
typedef struct TMR_SR_PortPowerAndSettlingTime
{
  /** The port number. */
  uint8_t port;
  /** The power level to use for read operations, in centidBm. */
  int16_t readPower;
  /** The power level to use for write operations, in centidBm. */
  int16_t writePower;
  /** The duration to wait after switching to this port, in microseconds. */
  uint16_t settlingTime;
} TMR_SR_PortPowerAndSettlingTime;

/** The current power level and the bounds of the power level range. */
typedef struct TMR_SR_PowerWithLimits
{
  /** The current power level, in centidBm. */
  uint16_t setPower;
  /** The maximum power level, in centidBm. */
  uint16_t maxPower;
  /** The minimum power level, in centidBm. */
  uint16_t minPower;
} TMR_SR_PowerWithLimits;


void TMR_hexDottedQuad(const uint8_t bytes[4], char buf[12]);
TMR_Status TMR_hexDottedQuadToUint32(const char bytes[12], uint32_t *result);


/**
 * This type enumerates the serial reader configuration parameters for
 * TMR_SR_cmdSetReaderConfiguration and
 * TMR_SR_cmdGetReaderConfiguration.  Each value is
 * associated with a particular data type for the setting.
 */
typedef enum TMR_SR_Configuration
{
  /**
   *  Key tag buffer records off of antenna ID as well as EPC;
   *  i.e., keep separate records for the same EPC read on different antennas
   *  0: Disable -- Different antenna overwrites previous record.
   *  1: Enable -- Different Antenna creates a new record.
   */
  TMR_SR_CONFIGURATION_UNIQUE_BY_ANTENNA        = 0,
  /**
   *  Run transmitter in lower-performance, power-saving mode.
   *  0: Disable -- Higher transmitter bias for improved reader sensitivity
   *  1: Enable -- Lower transmitter bias sacrifices sensitivity for power consumption
   */
  TMR_SR_CONFIGURATION_TRANSMIT_POWER_SAVE      = 1,
  /**
   *  Support 496-bit EPCs (vs normal max 96 bits)
   *  0: Disable (max max EPC length = 96)
   *  1: Enable 496-bit EPCs
   */
  TMR_SR_CONFIGURATION_EXTENDED_EPC             = 2,
  /**
   *  Configure GPOs to drive antenna switch.
   *  0: No switch
   *  1: Switch on GPO1
   *  2: Switch on GPO2
   *  3: Switch on GPO1,GPO2
   */
  TMR_SR_CONFIGURATION_ANTENNA_CONTROL_GPIO     = 3,
  /**
   *  Refuse to transmit if antenna is not detected
   */
  TMR_SR_CONFIGURATION_SAFETY_ANTENNA_CHECK     = 4,
  /**
   *  Refuse to transmit if overtemperature condition detected
   */
  TMR_SR_CONFIGURATION_SAFETY_TEMPERATURE_CHECK = 5,
  /**
   *  If tag read duplicates an existing tag buffer record (key is the same),
   *  update the record's timestamp if incoming read has higher RSSI reading.
   *  0: Keep timestamp of record's first read
   *  1: Keep timestamp of read with highest RSSI
   */
  TMR_SR_CONFIGURATION_RECORD_HIGHEST_RSSI      = 6,
  /**
   *  Key tag buffer records off tag data as well as EPC;
   *  i.e., keep separate records for the same EPC read with different data
   *  0: Disable -- Different data overwrites previous record.
   *  1: Enable -- Different data creates new record.
   */
  TMR_SR_CONFIGURATION_UNIQUE_BY_DATA           = 8,
  /**
   *  Whether RSSI values are reported in dBm, as opposed to
   *  arbitrary uncalibrated units.
   */
  TMR_SR_CONFIGURATION_RSSI_IN_DBM              = 9,
  /**
   *  Self jammer cancellation
   *  User can enable/disable through level2 API
   */
  TMR_SR_CONFIGURATION_SELF_JAMMER_CANCELLATION = 0x0A,
  /**
   *  Key tag buffer records off of protocol as well as EPC;
   *  i.e., keep separate records for the same EPC read on different protocols
   *  0: Disable -- Different protocol overwrites previous record.
   *  1: Enable -- Different protocol creates a new record.
   */
  TMR_SR_CONFIGURATION_UNIQUE_BY_PROTOCOL       = 0x0B,
  /**
   *  Enable read filtering
   */
  TMR_SR_CONFIGURATION_ENABLE_READ_FILTER         = 0x0C,
  /**
   *  Tag buffer entry timeout
   */
  TMR_SR_CONFIGURATION_READ_FILTER_TIMEOUT        = 0x0D,
  /**
    * Transport (bus) type
    **/ 
  TMR_SR_CONFIGURATION_CURRENT_MSG_TRANSPORT      = 0x0E,
  /**
   * Enable the CRC calculation
   */
  TMR_SR_CONFIGURATION_SEND_CRC               = 0x1B,
  /**
   *  General category of finished reader into which module is integrated; e.g.,
   *  0: bare module
   *  1: In-vehicle Reader (e.g., Tool Link, Vega)
   *  2: USB Reader
   */
  TMR_SR_CONFIGURATION_PRODUCT_GROUP_ID         = 0x12,
  /**
   * Product ID (Group ID 0x0002 ) information
   * 0x0001 :M5e-C USB reader
   * 0x0002 :Backback NA antenna
   * 0x0003 :Backback EU antenna
   **/
  TMR_SR_CONFIGURATION_PRODUCT_ID              = 0x13,
} TMR_SR_Configuration;


/**
 * This type enumerates the region configuration parameters for
 * the TMR_SR_cmdGetRegionConfiguration() command.
 */
typedef enum TMR_SR_RegionConfiguration
{
  TMR_SR_REGION_CONFIGURATION_LBT_ENABLED = 0x40,
} TMR_SR_RegionConfiguration;

/**
 * This is the enumeration of Gen2-specific configuration values. Each
 * enumerated value is associated with a particular data type for the
 * setting value.
 */
typedef enum TMR_SR_Gen2Configuration
{
  TMR_SR_GEN2_CONFIGURATION_SESSION = 0x00,
  TMR_SR_GEN2_CONFIGURATION_TARGET  = 0x01,
  TMR_SR_GEN2_CONFIGURATION_TAGENCODING = 0x02,
  TMR_SR_GEN2_CONFIGURATION_LINKFREQUENCY = 0x10,
  TMR_SR_GEN2_CONFIGURATION_TARI    = 0x11,
  TMR_SR_GEN2_CONFIGURATION_Q       = 0x12,
  TMR_SR_GEN2_CONFIGURATION_BAP    = 0x13
} TMR_SR_Gen2Configuration;

/**
 * This struture is retuned from TMR_SR_Gen2ReaderResponseTimeOut
 */
typedef struct TMR_SR_Gen2ReaderWriteTimeOut
{
  /* Status of reader timeout */
  bool earlyexit;

  /* Timeout value used for write opearation */
  uint16_t writetimeout;
}TMR_SR_Gen2ReaderWriteTimeOut;
/**
 * This is the enumeration of ISO 18000-6B-specific configuration
 * values. Each enumerated value is associated with a particular data
 * type for the setting value.
 */
typedef enum TMR_SR_Iso180006bConfiguration
{
  TMR_SR_ISO180006B_CONFIGURATION_LINKFREQUENCY = 0x10,
  TMR_SR_ISO180006B_CONFIGURATION_MODULATION_DEPTH = 0x11,
  TMR_SR_ISO180006B_CONFIGURATION_DELIMITER = 0x12
} TMR_SR_Iso180006bConfiguration;

/**
 * This is the enumeration of iPx-specific configuration
 * values. Each enumerated value is associated with a particular data
 * type for the setting value.
 */
typedef enum TMR_SR_iPxConfiguration
{
  TMR_SR_IPX_CONFIGURATION_LINKFREQUENCY = 0x10
} TMR_SR_iPxConfiguration;


/**
 * This type selects the protocol configuration option for
 * TMR_SR_cmdSetProtocolConfiguration() and
 * TMR_SR_cmdGetProtocolConfiguration().  Each value is associated
 * with a particular data type for the setting.
 */
typedef struct TMR_SR_ProtocolConfiguration
{
  /** The protocol to configure. Determines which union member is valid. */
  TMR_TagProtocol protocol;
  union
  {
    /** The configuration key for a Gen2 option. */
    TMR_SR_Gen2Configuration gen2;
    /** The configuration key for an ISO18000-6B option. */
    TMR_SR_Iso180006bConfiguration iso180006b;
    TMR_SR_iPxConfiguration ipx;
  }u;
} TMR_SR_ProtocolConfiguration;


/**
 * Defines the values for search flags for TMR_SR_cmdReadTagMultiple.
 */
typedef enum TMR_SR_SearchFlag
{
  TMR_SR_SEARCH_FLAG_CONFIGURED_ANTENNA = 0,
  TMR_SR_SEARCH_FLAG_ANTENNA_1_THEN_2   = 1,
  TMR_SR_SEARCH_FLAG_ANTENNA_2_THEN_1   = 2,
  TMR_SR_SEARCH_FLAG_CONFIGURED_LIST    = 3,
  TMR_SR_SEARCH_FLAG_ANTENNA_MASK       = 3,
  TMR_SR_SEARCH_FLAG_EMBEDDED_COMMAND   = 4,
  TMR_SR_SEARCH_FLAG_TAG_STREAMING      = 8,
  TMR_SR_SEARCH_FLAG_LARGE_TAG_POPULATION_SUPPORT = 16,
  TMR_SR_SEARCH_FLAG_STATUS_REPORT_STREAMING = 32,
  TMR_SR_SEARCH_FLAG_RETURN_ON_N_TAGS = 64,
  TMR_SR_SEARCH_FLAG_READ_MULTIPLE_FAST_SEARCH = 128,
  TMR_SR_SEARCH_FLAG_STATS_REPORT_STREAMING = 256,
}TMR_SR_SearchFlag;

typedef enum TMR_SR_ISO180006BCommands
{
  TMR_SR_ISO180006B_COMMAND_DATA_READ           = 0x0B,
  TMR_SR_ISO180006B_COMMAND_READ                = 0x0C,
  TMR_SR_ISO180006B_COMMAND_WRITE               = 0x0D,
  TMR_SR_ISO180006B_COMMAND_WRITE_MULTIPLE      = 0x0E,
  TMR_SR_ISO180006B_COMMAND_WRITE4BYTE          = 0x1B,
  TMR_SR_ISO180006B_COMMAND_WRITE4BYTE_MULTIPLE = 0x1C,
} TMR_SR_ISO180006BCommands;

typedef enum TMR_SR_ISO180006BCommandOptions
{
  TMR_SR_ISO180006B_WRITE_OPTION_READ_AFTER           = 0x00,
  TMR_SR_ISO180006B_WRITE_OPTION_NO_VERIFY            = 0x01,
  TMR_SR_ISO180006B_WRITE_OPTION_READ_VERIFY_AFTER    = 0x02,
  TMR_SR_ISO180006B_WRITE_OPTION_GROUP_SELECT         = 0x03,
  TMR_SR_ISO180006B_WRITE_OPTION_COUNT_PROVIDED       = 0x08,
  TMR_SR_ISO180006B_WRITE_LOCK_NO                     = 0x00,
  TMR_SR_ISO180006B_WRITE_LOCK_YES                    = 0x01,
  TMR_SR_ISO180006B_LOCK_OPTION_TYPE_FOLLOWS          = 0x01,
  TMR_SR_ISO180006B_LOCK_TYPE_QUERYLOCK_THEN_LOCK     = 0x01,
} TMR_SR_ISO180006BCommandOptions;

TMR_Status TMR_SR_cmdRaw(TMR_Reader *reader, uint32_t timeout, uint8_t msgLen,
            uint8_t msg[]);
TMR_Status TMR_SR_setSerialBaudRate(TMR_Reader *reader, uint32_t rate);
TMR_Status TMR_SR_cmdVersion(TMR_Reader *reader, TMR_SR_VersionInfo *info);
TMR_Status TMR_SR_cmdBootFirmware(TMR_Reader *reader);
TMR_Status TMR_SR_cmdSetBaudRate(TMR_Reader *reader, uint32_t rate);
TMR_Status TMR_SR_cmdVerifyImage(TMR_Reader *reader, bool *status);
TMR_Status TMR_SR_cmdEraseFlash(TMR_Reader *reader, uint8_t sector, 
            uint32_t password);
TMR_Status TMR_SR_cmdWriteFlashSector(TMR_Reader *reader, uint8_t sector, 
            uint32_t address, uint32_t password, uint8_t length,
            const uint8_t data[], uint32_t offset);
TMR_Status TMR_SR_cmdGetSectorSize(TMR_Reader *reader, uint8_t sector,
            uint32_t *size);
TMR_Status TMR_SR_cmdModifyFlashSector(TMR_Reader *reader, uint8_t sector, 
            uint32_t address, uint32_t password, uint8_t length,
            const uint8_t data[], uint32_t offset);
TMR_Status TMR_SR_cmdBootBootloader(TMR_Reader *reader);
TMR_Status TMR_SR_cmdGetHardwareVersion(TMR_Reader *reader, uint8_t option,
            uint8_t flags, uint8_t* count, uint8_t data[]);
TMR_Status TMR_SR_cmdGetCurrentProgram(TMR_Reader *reader, uint8_t *program);
TMR_Status TMR_SR_cmdReadTagSingle(TMR_Reader *reader, uint16_t timeout, 
            uint16_t metadataFlags, const TMR_TagFilter *filter, 
            TMR_TagProtocol protocol, TMR_TagReadData *tagData);
TMR_Status TMR_SR_cmdReadTagMultiple(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_SearchFlag flags, const TMR_TagFilter *filter,
            TMR_TagProtocol protocol, uint32_t *tagCount);
TMR_Status TMR_SR_cmdWriteGen2TagEpc(TMR_Reader *reader, const TMR_TagFilter *filter, TMR_GEN2_Password accessPassword,
			uint16_t timeout, uint8_t count, const uint8_t *id, bool lock);
TMR_Status TMR_SR_cmdGEN2WriteTagData(TMR_Reader *reader,
            uint16_t timeout, TMR_GEN2_Bank bank, uint32_t address,
            uint8_t count, const uint8_t data[],
            TMR_GEN2_Password accessPassword, const TMR_TagFilter *filter);
TMR_Status TMR_SR_cmdGEN2LockTag(TMR_Reader *reader, uint16_t timeout, 
            uint16_t mask, uint16_t action, TMR_GEN2_Password accessPassword, 
            const TMR_TagFilter *filter);
TMR_Status TMR_SR_cmdKillTag(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password killPassword, const TMR_TagFilter *filter);
TMR_Status TMR_SR_cmdGEN2ReadTagData(TMR_Reader *reader,
            uint16_t timeout, TMR_GEN2_Bank bank,
            uint32_t address, uint8_t length, uint32_t accessPassword,
            const TMR_TagFilter *filter, TMR_TagReadData *data);
TMR_Status TMR_SR_cmdGetTagsRemaining(TMR_Reader *reader, uint16_t *remaining);
TMR_Status TMR_SR_cmdGetTagBuffer(TMR_Reader *reader, uint16_t count, bool epc496,
            TMR_TagProtocol protocol, TMR_TagData tagData[]);
TMR_Status TMR_SR_cmdClearTagBuffer(TMR_Reader *reader);
TMR_Status TMR_SR_cmdHiggs2PartialLoadImage(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword, 
            uint8_t len, const uint8_t epc[], TMR_TagFilter* target);
TMR_Status TMR_SR_cmdHiggs2FullLoadImage(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword,
            uint16_t lockBits, uint16_t pcWord, uint8_t count,
            const uint8_t epc[], TMR_TagFilter* target);
TMR_Status TMR_SR_cmdHiggs3FastLoadImage(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password currentAccessPassword, 
            TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword,
            uint16_t pcWord, uint8_t count, const uint8_t epc[], TMR_TagFilter* target);
TMR_Status TMR_SR_cmdHiggs3LoadImage(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password currentAccessPassword,
            TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword,
            uint16_t pcWord, uint8_t len, const uint8_t epcAndUserData[], TMR_TagFilter* target);
TMR_Status TMR_SR_cmdHiggs3BlockReadLock(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, uint8_t lockBits, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdNxpSetReadProtect(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_GEN2_SiliconType chip, TMR_GEN2_Password accessPassword, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdNxpResetReadProtect(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_GEN2_SiliconType chip, TMR_GEN2_Password accessPassword, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdNxpChangeEas(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_GEN2_SiliconType chip, TMR_GEN2_Password accessPassword, bool reset, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdNxpEasAlarm(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_GEN2_SiliconType chip, TMR_GEN2_DivideRatio dr, TMR_GEN2_TagEncoding m, TMR_GEN2_TrExt trExt,
            TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdNxpCalibrate(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_GEN2_SiliconType chip, TMR_GEN2_Password accessPassword, TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdNxpChangeConfig(TMR_Reader *reader, uint16_t timeout,
            TMR_SR_GEN2_SiliconType chip, TMR_GEN2_Password accessPassword, TMR_NXP_ConfigWord configWord, TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdMonza4QTReadWrite(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            TMR_Monza4_ControlByte controlByte, TMR_Monza4_Payload payload, TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aGetSensorValue(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            uint8_t CommandCode, uint32_t password, PasswordLevel level, Sensor sensortype, TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aGetMeasurementSetup(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            uint8_t CommandCode, uint32_t password, PasswordLevel level,TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aGetCalibrationData(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aSetCalibrationData(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            uint8_t CommandCode, uint32_t password, PasswordLevel level, uint64_t calibration, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aSetSfeParameters(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            uint8_t CommandCode, uint32_t password, PasswordLevel level, uint16_t sfe, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aGetLogState(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
            uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aSetLogMode(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
           uint8_t CommandCode, uint32_t password, PasswordLevel level, LoggingForm form, StorageRule rule, bool Ext1Enable,
           bool Ext2Enable, bool TempEnable, bool BattEnable, uint16_t LogInterval, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aSetLogLimit(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
           uint8_t CommandCode, uint32_t password, PasswordLevel level, uint16_t exLower,
           uint16_t lower, uint16_t upper, uint16_t exUpper, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aSetShelfLife(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
                             uint8_t CommandCode, uint32_t password, PasswordLevel level, uint32_t block0, uint32_t block1,
                             TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aInitialize(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                      uint8_t CommandCode, uint32_t password, PasswordLevel level, uint16_t delayTime,
                                      uint16_t applicatioData, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aStartLog(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t CommandCode,
                                    uint32_t password, PasswordLevel level, uint32_t time, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aEndLog(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
           uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aSetPassword(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
           uint8_t CommandCode, uint32_t password, PasswordLevel level, uint32_t newPassword,
           PasswordLevel newPasswordLevel, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aGetBatteryLevel(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                           uint8_t CommandCode, uint32_t password, PasswordLevel level,BatteryType type,
                                           TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aAccessFifoStatus(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t CommandCode,
           uint32_t password, PasswordLevel level, AccessFifoOperation opearation, TMR_uint8List * data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aAccessFifoRead(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t CommandCode,
            uint32_t password, PasswordLevel level, AccessFifoOperation opearation, uint8_t length,TMR_uint8List * data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdSL900aAccessFifoWrite(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t CommandCode,
           uint32_t password, PasswordLevel level, AccessFifoOperation opearation, TMR_uint8List *payLoad, TMR_uint8List * data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdHibikiReadLock(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, uint16_t mask, uint16_t action);
TMR_Status TMR_SR_cmdHibikiGetSystemInformation(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword,
            TMR_GEN2_HibikiSystemInformation *info);
TMR_Status TMR_SR_cmdHibikiSetAttenuate(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, uint8_t level, bool lock);
TMR_Status TMR_SR_cmdHibikiBlockLock(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, uint8_t block,
            TMR_GEN2_Password blockPassword, uint8_t mask, uint8_t action);
TMR_Status TMR_SR_cmdHibikiBlockReadLock(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Password accessPassword, uint8_t block,
            TMR_GEN2_Password blockPassword, uint8_t mask, uint8_t action);
TMR_Status TMR_SR_cmdHibikiWriteMultipleWords(TMR_Reader *reader,
            uint16_t timeout, TMR_GEN2_Password accessPassword,
            TMR_GEN2_Bank bank, uint32_t wordOffset, uint8_t count,
            const uint8_t data[]);
TMR_Status TMR_SR_cmdEraseBlockTagSpecific(TMR_Reader *reader, uint16_t timeout,
            TMR_GEN2_Bank bank, uint32_t address, uint8_t count);
TMR_Status TMR_SR_cmdGetTxRxPorts(TMR_Reader *reader, TMR_SR_PortPair *ant);
TMR_Status TMR_SR_cmdGetAntennaConfiguration(TMR_Reader *reader,
            TMR_SR_AntennaPort *config);
TMR_Status TMR_SR_cmdGetAntennaSearchList(TMR_Reader *reader, uint8_t *count,
            TMR_SR_PortPair *ants);
TMR_Status TMR_SR_cmdGetAntennaPortPowers(TMR_Reader *reader, uint8_t *count,
            TMR_SR_PortPower *ports);
TMR_Status TMR_SR_cmdGetAntennaPortPowersAndSettlingTime(TMR_Reader *reader,
            uint8_t *count, TMR_SR_PortPowerAndSettlingTime *ports);
TMR_Status TMR_SR_cmdGetAntennaReturnLoss(TMR_Reader *reader, TMR_PortValueList *ports);
TMR_Status TMR_SR_cmdAntennaDetect(TMR_Reader *reader, uint8_t *count,
            TMR_SR_PortDetect *ports);
TMR_Status TMR_SR_cmdGetReadTxPower(TMR_Reader *reader, int32_t *power);
TMR_Status TMR_SR_cmdGetReadTxPowerWithLimits(TMR_Reader *reader,
            TMR_SR_PowerWithLimits *power);
TMR_Status TMR_SR_cmdGetWriteTxPower(TMR_Reader *reader, int32_t *power);
TMR_Status TMR_SR_cmdGetWriteTxPowerWithLimits(TMR_Reader *reader,
            TMR_SR_PowerWithLimits *power);
TMR_Status TMR_SR_cmdGetCurrentProtocol(TMR_Reader *reader,
            TMR_TagProtocol *protocol);
TMR_Status TMR_SR_cmdMultipleProtocolSearch(TMR_Reader *reader,TMR_SR_OpCode op,TMR_TagProtocolList *protocols, TMR_TRD_MetadataFlag metadataFlags,TMR_SR_SearchFlag antennas, TMR_TagFilter **filter, uint16_t timeout, uint32_t *tagsFound);
TMR_Status TMR_SR_cmdGetFrequencyHopTable(TMR_Reader *reader, uint8_t *count,
            uint32_t *hopTable);
TMR_Status TMR_SR_cmdGetFrequencyHopTime(TMR_Reader *reader, uint32_t *hopTime);
TMR_Status TMR_SR_cmdGetGPIO(TMR_Reader *reader, uint8_t *count, TMR_GpioPin *state);
TMR_Status TMR_SR_cmdGetGPIODirection(TMR_Reader *reader, uint8_t pin,
            bool *out);
TMR_Status TMR_SR_cmdGetRegion(TMR_Reader *reader, TMR_Region *region);
TMR_Status TMR_SR_cmdGetRegionConfiguration(TMR_Reader *reader,
            TMR_SR_RegionConfiguration key, void *value);
TMR_Status TMR_SR_cmdGetPowerMode(TMR_Reader *reader, TMR_SR_PowerMode *mode);
TMR_Status TMR_SR_cmdGetUserMode(TMR_Reader *reader, TMR_SR_UserMode *mode);
TMR_Status TMR_SR_cmdGetReaderConfiguration(TMR_Reader *reader,
            TMR_SR_Configuration key, void *value);
TMR_Status TMR_SR_cmdIAVDenatranCustomOp(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode, uint8_t rfu,
           TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdIAVDenatranCustomActivateSiniavMode(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode, uint8_t payload,
           TMR_uint8List *data, TMR_TagFilter* target, bool tokenDesc, uint8_t *token);
TMR_Status TMR_SR_cmdIAVDenatranCustomReadFromMemMap(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode, uint8_t payload,
            TMR_uint8List *data, TMR_TagFilter* target, uint16_t wordAddress);
TMR_Status TMR_SR_cmdIAVDenatranCustomWriteToMemMap(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode, uint8_t payload,
            TMR_uint8List *data, TMR_TagFilter* target, uint16_t wordPtr, uint16_t wordData, uint8_t* tagId, uint8_t* dataBuf);
TMR_Status TMR_SR_cmdIAVDenatranCustomWriteSec(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode, uint8_t payload,
            TMR_uint8List *data, TMR_TagFilter* target, uint8_t* dataWords, uint8_t* dataBuf);
TMR_Status TMR_SR_cmdIAVDenatranCustomGetTokenId(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode,
           TMR_uint8List *data, TMR_TagFilter* target);
TMR_Status TMR_SR_cmdIAVDenatranCustomReadSec(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t mode, uint8_t payload,
            TMR_uint8List *data, TMR_TagFilter* target, uint16_t wordAddress);
TMR_Status TMR_SR_cmdGetProtocolConfiguration(TMR_Reader *reader, TMR_TagProtocol protocol,
            TMR_SR_ProtocolConfiguration key, void *value);
TMR_Status TMR_SR_cmdGetReaderStats(TMR_Reader *reader,
           TMR_Reader_StatsFlag statFlags,
           TMR_Reader_StatsValues *stats);
TMR_Status TMR_SR_cmdGetReaderStatistics(TMR_Reader *reader,
            TMR_SR_ReaderStatisticsFlag statFlags,
            TMR_SR_ReaderStatistics *stats);
TMR_Status TMR_SR_cmdGetAvailableProtocols(TMR_Reader *reader,
            TMR_TagProtocolList *protocols);
TMR_Status TMR_SR_cmdGetAvailableRegions(TMR_Reader *reader,
            TMR_RegionList *regions);
TMR_Status TMR_SR_cmdGetTemperature(TMR_Reader *reader, uint8_t *temp);
TMR_Status TMR_SR_cmdSetTxRxPorts(TMR_Reader *reader, uint8_t txPrt,
            uint8_t rxPort);
TMR_Status TMR_SR_cmdSetAntennaSearchList(TMR_Reader *reader,
            uint8_t count, const TMR_SR_PortPair *ports);
TMR_Status TMR_SR_cmdSetAntennaPortPowers(TMR_Reader *reader,
            uint8_t count, const TMR_SR_PortPower *ports);
TMR_Status TMR_SR_cmdSetAntennaPortPowersAndSettlingTime(TMR_Reader *reader,
            uint8_t count, const TMR_SR_PortPowerAndSettlingTime *ports);
TMR_Status TMR_SR_cmdSetReadTxPower(TMR_Reader *reader, int32_t power);
TMR_Status TMR_SR_cmdSetWriteTxPower(TMR_Reader *reader, int32_t power);
TMR_Status TMR_SR_cmdSetProtocol(TMR_Reader *reader, TMR_TagProtocol protocol);
TMR_Status TMR_SR_cmdSetFrequencyHopTable(TMR_Reader *reader, uint8_t count,
            const uint32_t *table);
TMR_Status TMR_SR_cmdSetFrequencyHopTime(TMR_Reader *reader, uint32_t hopTime);
TMR_Status TMR_SR_cmdSetGPIO(TMR_Reader *reader, uint8_t gpio, bool high);
TMR_Status TMR_SR_cmdSetGPIODirection(TMR_Reader *reader, uint8_t pin,
            bool out);
TMR_Status TMR_SR_cmdSetRegion(TMR_Reader *reader, TMR_Region region);
TMR_Status TMR_SR_cmdSetRegionLbt(TMR_Reader *reader, TMR_Region region, bool lbt);
TMR_Status TMR_SR_cmdSetPowerMode(TMR_Reader *reader, TMR_SR_PowerMode mode);
TMR_Status TMR_SR_cmdSetUserMode(TMR_Reader *reader, TMR_SR_UserMode mode);
TMR_Status TMR_SR_cmdSetReaderConfiguration(TMR_Reader *reader, 
            TMR_SR_Configuration key, const void *value);
TMR_Status TMR_SR_cmdSetProtocolLicenseKey(TMR_Reader *reader, TMR_SR_SetProtocolLicenseOption option, uint8_t key[], int key_len,uint32_t *retData);
TMR_Status TMR_SR_cmdSetProtocolConfiguration(TMR_Reader *reader,
            TMR_TagProtocol protocol, TMR_SR_ProtocolConfiguration key,
            const void *value);
TMR_Status TMR_SR_cmdResetReaderStats(TMR_Reader *reader,
           TMR_Reader_StatsFlag statFlags);
TMR_Status TMR_SR_cmdResetReaderStatistics(TMR_Reader *reader,
            TMR_SR_ReaderStatisticsFlag statFlags);
TMR_Status TMR_SR_cmdTestSetFrequency(TMR_Reader *reader, uint32_t frequency);
TMR_Status TMR_SR_cmdTestSendCw(TMR_Reader *reader, bool on);
TMR_Status TMR_SR_cmdTestSendPrbs(TMR_Reader *reader, uint16_t duration);
TMR_Status TMR_SR_cmdSetUserProfile(TMR_Reader *reader,
                                    TMR_SR_UserConfigOperation op,TMR_SR_UserConfigCategory category, TMR_SR_UserConfigType type);
TMR_Status TMR_SR_cmdGetUserProfile(TMR_Reader *reader, 
                                    uint8_t byte[], uint8_t length, uint8_t response[], uint8_t* response_length);
TMR_Status TMR_SR_cmdBlockWrite(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Bank bank, uint32_t wordPtr, 
                                 uint32_t wordCount, const uint16_t* data, uint32_t accessPassword, const TMR_TagFilter* target);
TMR_Status TMR_SR_cmdBlockErase(TMR_Reader *reader, uint16_t timeout, TMR_GEN2_Bank bank, uint32_t wordPtr,
                                 uint8_t wordCount, uint32_t accessPassword, TMR_TagFilter *target);
TMR_Status TMR_SR_cmdBlockPermaLock(TMR_Reader *reader, uint16_t timeout,uint32_t readLock, TMR_GEN2_Bank bank, 
                         uint32_t blockPtr, uint32_t blockRange, uint16_t* mask, uint32_t accessPassword, TMR_TagFilter* target, TMR_uint8List *data);
TMR_Status TMR_SR_msgSetupReadTagMultiple(TMR_Reader *reader, uint8_t *msg, uint8_t *i,
            uint16_t timeout, TMR_SR_SearchFlag searchFlag,
            const TMR_TagFilter *filter, TMR_TagProtocol protocol,
            TMR_GEN2_Password accessPassword);

TMR_Status
TMR_SR_msgSetupReadTagMultipleWithMetadata(TMR_Reader *reader, uint8_t *msg, uint8_t *i, uint16_t timeout,
                               TMR_SR_SearchFlag searchFlag,
							                 TMR_TRD_MetadataFlag metadataFlag,
                               const TMR_TagFilter *filter,
                               TMR_TagProtocol protocol,
                               TMR_GEN2_Password accessPassword);

TMR_Status TMR_SR_msgSetupReadTagSingle(uint8_t *msg, uint8_t *i, TMR_TagProtocol protocol,TMR_TRD_MetadataFlag metadataFlags, const TMR_TagFilter *filter,uint16_t timeout);
void TMR_SR_msgAddGEN2WriteTagEPC(uint8_t *msg, uint8_t *i, uint16_t timeout, uint8_t *epc, uint8_t count);
void TMR_SR_msgAddGEN2DataRead(uint8_t *msg, uint8_t *i, uint16_t timeout,
      TMR_GEN2_Bank bank, uint32_t wordAddress, uint8_t len, uint8_t option, bool withMetaData);
void TMR_SR_msgAddGEN2DataWrite(uint8_t *msg, uint8_t *i, uint16_t timeout,
      TMR_GEN2_Bank bank, uint32_t address);
void TMR_SR_msgAddGEN2LockTag(uint8_t *msg, uint8_t *i, uint16_t timeout,
      uint16_t mask, uint16_t action, TMR_GEN2_Password password);
void TMR_SR_msgAddGEN2KillTag(uint8_t *msg, uint8_t *i, uint16_t timeout,
      TMR_GEN2_Password password);
void
TMR_SR_msgAddGEN2BlockWrite(uint8_t *msg, uint8_t *i, uint16_t timeout,TMR_GEN2_Bank bank, uint32_t wordPtr, uint32_t wordCount, uint16_t* data, uint32_t accessPassword,TMR_TagFilter* target);

void
TMR_SR_msgAddGEN2BlockPermaLock(uint8_t *msg, uint8_t *i, uint16_t timeout, uint32_t readLock, TMR_GEN2_Bank bank, uint32_t blockPtr, uint32_t blockRange, uint16_t* mask, uint32_t accessPassword,TMR_TagFilter* target);

void
TMR_SR_msgAddGEN2BlockErase(uint8_t *msg, uint8_t *i, uint16_t timeout, uint32_t wordPtr, TMR_GEN2_Bank bank,
                            uint8_t wordCount, uint32_t accessPassword, TMR_TagFilter* target);

void 
TMR_SR_msgAddHiggs2PartialLoadImage(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword, 
                                    TMR_GEN2_Password killPassword, uint8_t len, const uint8_t *epc, TMR_TagFilter* target);
void 
TMR_SR_msgAddHiggs2FullLoadImage(uint8_t *msg, uint8_t *i, uint16_t timeout,
      TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword, uint16_t lockBits, uint16_t pcWord, uint8_t len, const uint8_t *epc, TMR_TagFilter* target);
void 
TMR_SR_msgAddHiggs3FastLoadImage(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password currentAccessPassword,
      TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword, uint16_t pcWord, uint8_t len, const uint8_t *epc, TMR_TagFilter* target);
void 
TMR_SR_msgAddHiggs3LoadImage(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password currentAccessPassword,
      TMR_GEN2_Password accessPassword, TMR_GEN2_Password killPassword, uint16_t pcWord, uint8_t len, const uint8_t *epcAndUserData, TMR_TagFilter* target);

void 
TMR_SR_msgAddHiggs3BlockReadLock(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword, uint8_t lockBits, TMR_TagFilter* target);

void 
TMR_SR_msgAddNXPSetReadProtect(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_SR_GEN2_SiliconType chip,
                            TMR_GEN2_Password accessPassword, TMR_TagFilter* target);
void 
TMR_SR_msgAddNXPResetReadProtect(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_SR_GEN2_SiliconType chip,
                            TMR_GEN2_Password accessPassword, TMR_TagFilter* target);
void
TMR_SR_msgAddNXPChangeEAS(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_SR_GEN2_SiliconType chip,
                          TMR_GEN2_Password accessPassword, bool reset, TMR_TagFilter* target);
void 
TMR_SR_msgAddNXPEASAlarm(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_SR_GEN2_SiliconType chip,
                         TMR_GEN2_DivideRatio dr, TMR_GEN2_TagEncoding m, TMR_GEN2_TrExt trExt, TMR_TagFilter* target);
void
TMR_SR_msgAddIAVDenatranCustomOp(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                         uint8_t mode, uint8_t rfu, TMR_TagFilter* target);
void 
TMR_SR_msgAddIAVDenatranCustomActivateSiniavMode(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                         uint8_t mode, uint8_t payload, TMR_TagFilter* target, bool tokenDesc, uint8_t *token);
void 
TMR_SR_msgAddIAVDenatranCustomReadFromMemMap(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                        uint8_t mode, uint8_t payload, TMR_TagFilter* target, uint16_t wordAddress);
void 
TMR_SR_msgAddIAVDenatranCustomWriteToMemMap(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                        uint8_t mode, uint8_t payload, TMR_TagFilter* target, uint16_t wordPtr, uint16_t wordData, uint8_t* tagId, uint8_t* dataBuf);
void
TMR_SR_msgAddIAVDenatranCustomWriteSec(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                        uint8_t mode, uint8_t payload, TMR_TagFilter* target, uint8_t* data, uint8_t* dataBuf);
void 
TMR_SR_msgAddIAVDenatranCustomGetTokenId(uint8_t *msg, uint8_t *i, uint16_t timeout,
                        TMR_GEN2_Password accessPassword, uint8_t mode, TMR_TagFilter* target);
void 
TMR_SR_msgAddIAVDenatranCustomReadSec(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                        uint8_t mode, uint8_t payload, TMR_TagFilter* target, uint16_t wordAddress);
void 
TMR_SR_msgAddNXPCalibrate(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_SR_GEN2_SiliconType chip,
                         TMR_GEN2_Password accessPassword, TMR_TagFilter* target);
void 
TMR_SR_msgAddNXPChangeConfig(uint8_t *msg, uint8_t *i, uint16_t timeout,
                         TMR_SR_GEN2_SiliconType chip, TMR_GEN2_Password accessPassword, TMR_NXP_ConfigWord configword, TMR_TagFilter* target);
void 
TMR_SR_msgAddMonza4QTReadWrite(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                               TMR_Monza4_ControlByte controlByte, TMR_Monza4_Payload payload, TMR_TagFilter* target);

void 
TMR_SR_msgAddIdsSL900aGetSensorValue(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                     uint8_t CommandCode, uint32_t password, PasswordLevel level, Sensor sensortype,
                                     TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aGetMeasurementSetup(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                          uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_TagFilter* target);

void 
TMR_SR_msgAddIdsSL900aGetCalibrationData(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                     uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aSetCalibrationData(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                          uint8_t CommandCode, uint32_t password, PasswordLevel level, uint64_t calibration, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aSetSfeParameters(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                          uint8_t CommandCode, uint32_t password, PasswordLevel level, uint16_t sfe,
                                          TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aGetLogState(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                  uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_TagFilter* target);
void
TMR_SR_msgAddIdsSL900aSetLogMode(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                 uint8_t CommandCode, uint32_t password, PasswordLevel level, LoggingForm form,
                                 StorageRule rule, bool Ext1Enable, bool Ext2Enable, bool TempEnable, bool BattEnable,
                                 uint16_t LogInterval, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aSetLogLimit(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                  uint8_t CommandCode, uint32_t password, PasswordLevel level, uint16_t exLower,
                                  uint16_t lower, uint16_t upper, uint16_t exUpper, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aSetShelfLife(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                   uint8_t CommandCode, uint32_t password, PasswordLevel level, uint32_t block0, uint32_t block1,
                                   TMR_TagFilter* target);
void
TMR_SR_msgAddIdsSL900aInitialize(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                 uint8_t CommandCode, uint32_t password, PasswordLevel level, uint16_t delayTime,
                                 uint16_t applicatioData, TMR_TagFilter* target);
void
TMR_SR_msgAddIdsSL900aEndLog(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                             uint8_t CommandCode, uint32_t password, PasswordLevel level, TMR_TagFilter* target);
void
TMR_SR_msgAddIdsSL900aSetPassword(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                  uint8_t CommandCode, uint32_t password, PasswordLevel level, uint32_t newPassword,
                                  PasswordLevel newPasswordLevel, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aAccessFifoStatus(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                  uint8_t CommandCode, uint32_t password, PasswordLevel level, AccessFifoOperation opearation,
                                  TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aGetBatteryLevel(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                      uint8_t CommandCode, uint32_t password, PasswordLevel level, BatteryType batteryType,
                                      TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aAccessFifoRead(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                  uint8_t CommandCode, uint32_t password, PasswordLevel level, AccessFifoOperation opearation,
                                  uint8_t length, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aAccessFifoWrite(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                                  uint8_t CommandCode, uint32_t password, PasswordLevel level, AccessFifoOperation opearation,
                                  TMR_uint8List *payLoad, TMR_TagFilter* target);
void 
TMR_SR_msgAddIdsSL900aStartLog(uint8_t *msg, uint8_t *i, uint16_t timeout, TMR_GEN2_Password accessPassword,
                               uint8_t CommandCode, uint32_t password, PasswordLevel level, uint32_t time, TMR_TagFilter* target);

TMR_Status TMR_SR_executeEmbeddedRead(TMR_Reader *reader, uint8_t *msg,
            uint16_t timeout, TMR_SR_MultipleStatus *status);

TMR_Status TMR_SR_cmdISO180006BWriteTagData(TMR_Reader *reader,
      uint16_t timeout, uint8_t address, uint8_t count, const uint8_t data[],
      const TMR_TagFilter *filter);
TMR_Status TMR_SR_cmdISO180006BReadTagData(TMR_Reader *reader,
      uint16_t timeout, uint8_t address, uint8_t length,
      const TMR_TagFilter *filter, TMR_TagReadData *read);
TMR_Status TMR_SR_cmdISO180006BLockTag(TMR_Reader *reader, uint16_t timeout,
      uint8_t address, const TMR_TagFilter *filter);
TMR_Status TMR_iso18000BBLFValToInt(int val, void *lf);
TMR_Status TMR_SR_cmdStopReading(struct TMR_Reader *reader);
TMR_Status TMR_SR_cmdGetReaderWriteTimeOut (struct TMR_Reader *reader, TMR_TagProtocol protocol,
																						TMR_SR_Gen2ReaderWriteTimeOut *value);
TMR_Status TMR_SR_cmdSetReaderWriteTimeOut (struct TMR_Reader *reader, TMR_TagProtocol protocol,
																						TMR_SR_Gen2ReaderWriteTimeOut *value);
TMR_Status TMR_SR_cmdAuthReqResponse(struct TMR_Reader *reader, TMR_TagAuthentication *auth);

TMR_Status TMR_SR_addTagOp(struct TMR_Reader *reader, TMR_TagOp *tagop,TMR_ReadPlan *rp, uint8_t *msg, uint8_t *i, uint32_t readTimeMs, uint8_t *lenbyte);

TMR_Status
TMR_fillReaderStats(TMR_Reader *reader, TMR_Reader_StatsValues* stats, uint16_t flag, uint8_t* msg, uint8_t offset);
bool compareAntennas(TMR_MultiReadPlan *multi);
TMR_Status
TMR_SR_cmdrebootReader(TMR_Reader *reader);

#ifdef TMR_ENABLE_BACKGROUND_READS
void notify_authreq_listeners(TMR_Reader *reader, TMR_TagReadData *trd, TMR_TagAuthentication *auth);
#endif


#ifdef __cplusplus
}
#endif

#endif /* _SERIAL_READER_IMP_H */
