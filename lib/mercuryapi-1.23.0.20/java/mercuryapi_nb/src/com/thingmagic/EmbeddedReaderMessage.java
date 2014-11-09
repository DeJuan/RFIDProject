/*
 * Copyright (c) 2008 ThingMagic, Inc.
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
package com.thingmagic;
import java.util.Date;

/*
 * EmbeddedReaderMessage.java
 *
 * EmbeddedReaderMessage is the base class used to represent messages that are sent and received
 * by a M4e or M5e reader.  It defines constants that represent message opcodes.
 *
 * Created on February 25, 2007, 1:03 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

interface EmbeddedReaderMessage {
    
    public static final short
        // Bootloader specific OpCodes
        MSG_OPCODE_WRITE_FLASH              = 0x01,
        MSG_OPCODE_READ_FLASH               = 0x02, 
        MSG_OPCODE_VERSION                  = 0x03,
        MSG_OPCODE_BOOT_FIRMWARE            = 0x04,
        MSG_OPCODE_SET_BAUD_RATE            = 0x06,
        MSG_OPCODE_ERASE_FLASH              = 0x07,
        MSG_OPCODE_VERIFY_IMAGE_CRC         = 0x08,
        MSG_OPCODE_BOOT_BOOTLOADER          = 0x09,
        MSG_OPCODE_MODIFY_FLASH             = 0x0A,
        MSG_OPCODE_GET_DSP_SILICON_ID       = 0x0B,        
        MSG_OPCODE_GET_CURRENT_PROGRAM      = 0x0C,
        MSG_OPCODE_WRITE_FLASH_SECTOR       = 0x0D,
        MSG_OPCODE_GET_SECTOR_SIZE          = 0x0E,
        MSG_OPCODE_MODIFY_FLASH_SECTOR      = 0x0F,
        MSG_OPCODE_GET_HW_REVISION          = 0x10,
            // Application specific OpCodes - Read, Write, Misc ...
        MSG_OPCODE_READ_TAG_ID_SINGLE       = 0x21,
        MSG_OPCODE_READ_TAG_ID_MULTIPLE     = 0x22,
        MSG_OPCODE_WRITE_TAG_ID             = 0x23,
        MSG_OPCODE_WRITE_TAG_DATA           = 0x24,
        MSG_OPCODE_LOCK_TAG                 = 0x25,
        MSG_OPCODE_KILL_TAG                 = 0x26,
        MSG_OPCODE_VERIFY_TAG               = 0x27,
        MSG_OPCODE_READ_TAG_DATA            = 0x28,
        MSG_OPCODE_GET_TAG_BUFFER           = 0x29,
        MSG_OPCODE_CLEAR_TAG_ID_BUFFER      = 0x2A,
        MSG_OPCODE_SET_PASSWORD             = 0x2B,
        MSG_OPCODE_INITIALIZE_TAG           = 0x2C,
        MSG_OPCODE_WRITE_TAG_SPECIFIC       = 0x2D,
        MSG_OPCODE_ERASE_BLOCK_TAG_SPECIFIC = 0x2E,
        MSG_OPCODE_MULTI_PROTOCOL_TAG_OP    = 0X2F,
        MSG_OPCODE_TX_BITS                  = 0x3B,
        MSG_OPCODE_RUN_T2_TEST              = 0x3C,
        MSG_OPCODE_SET_LINK_FREQUENCY       = 0x3D,
        MSG_OPCODE_WRITE_RESPONSE           = 0x3f,
        // High Speed Mode OpCodes
        MSG_OPCODE_HS_SET_FREQ_POWER        = 0x40,
        MSG_OPCODE_HS_READ_TAG_ID_SINGLE    = 0x41,
        MSG_OPCODE_HS_DETECT                = 0x42,
        MSG_OPCODE_HS_WRITE                 = 0x43,
        MSG_OPCODE_HS_BULK_WRITE            = 0x44,
        MSG_OPCODE_HS_AUTO_WRITE            = 0x45,
        MSG_OPCODE_HS_SET_NEXT_ID_REGISTER  = 0x46,
        MSG_OPCODE_HS_GET_NEXT_ID_REGISTER  = 0x47,
        MSG_OPCODE_HS_TEST_RSS              = 0x48,
        MSG_OPCODE_HS_CAPTURE_SAMPLES       = 0x49,
        MSG_OPCODE_HS_SET_EXTENDED_FREQ     = 0x4A,
        MSG_OPCODE_HS_TEST_SENSITIVITY      = 0x4B,
        MSG_OPCODE_HS_SWEEP_SENSITIVITY     = 0x4C,
        MSG_OPCODE_HS_START_HIGH_SPEED_MODE = 0x4D,
        MSG_OPCODE_HS_STOP_HIGH_SPEED_MODE  = 0x4E,
        MSG_OPCODE_HS_GET_CAPTURED_SAMPLES  = 0x4F,
        // Application specific OpCodes - Get
        MSG_OPCODE_GET_ANTENNA_PORT         = 0x61,
        MSG_OPCODE_GET_TX_READ_POWER        = 0x62,
        MSG_OPCODE_GET_TAG_PROTOCOL         = 0x63,
        MSG_OPCODE_GET_TX_WRITE_POWER       = 0x64,
        MSG_OPCODE_GET_FREQ_HOP_TABLE       = 0x65,
        MSG_OPCODE_GET_USER_GPIO_INPUTS     = 0x66,
        MSG_OPCODE_GET_REGION               = 0x67,
        MSG_OPCODE_GET_POWER_MODE           = 0x68,
        MSG_OPCODE_GET_USER_MODE            = 0x69,
        MSG_OPCODE_GET_READER_OPTIONAL_PARAMS = 0x6A,
        MSG_OPCODE_GET_PROTOCOL_PARAM       = 0x6B,
        MSG_OPCODE_GET_READER_STATS         = 0x6C,
        MSG_OPCODE_GET_USER_PROFILE         = 0x6D,
        MSG_OPCODE_GET_AVAILABLE_PROTOCOLS  = 0x70,    
        MSG_OPCODE_GET_AVAILABLE_REGIONS    = 0x71,
        MSG_OPCODE_GET_TEMPERATURE          = 0x72,            
        // Application specific OpCodes - Set
        MSG_OPCODE_SET_ANTENNA_PORT         = 0x91,
        MSG_OPCODE_SET_TX_READ_POWER        = 0x92,
        MSG_OPCODE_SET_TAG_PROTOCOL         = 0x93,
        MSG_OPCODE_SET_TX_WRITE_POWER       = 0x94,
        MSG_OPCODE_SET_FREQ_HOP_TABLE       = 0x95,
        MSG_OPCODE_SET_USER_GPIO_OUTPUTS    = 0x96,
        MSG_OPCODE_SET_REGION               = 0x97,
        MSG_OPCODE_SET_POWER_MODE           = 0x98,
        MSG_OPCODE_SET_USER_MODE            = 0x99,
        MSG_OPCODE_SET_READER_OPTIONAL_PARAMS = 0x9A,
        MSG_OPCODE_SET_PROTOCOL_PARAM       = 0x9B,
        // Debug and FCC test OpCodes
        MSG_OPCODE_SET_USER_PROFILE         = 0X9d,
        MSG_OPCODE_SET_OPERATING_FREQ       = 0xC1,
        MSG_OPCODE_SET_TX_STATE             = 0xC2,
        MSG_OPCODE_TX_CW_SIGNAL             = 0xC3,
        MSG_OPCODE_TX_BIT_STRING            = 0xC4,
        MSG_OPCODE_SET_AFE_POWER_ENABLE     = 0xC5,
        MSG_OPCODE_RX_TEST                  = 0xC6,
        MSG_OPCODE_SET_I_Q_SWITCH           = 0xC7,
        MSG_OPCODE_GET_OPERATING_FREQ       = 0xC8,
        MSG_OPCODE_GET_TX_STATE             = 0xC9,
        MSG_OPCODE_GET_AFE_POWER_ENABLE     = 0xCA,
        MSG_OPCODE_USE_POWER_CALIBRATION    = 0xCB,
        // Reply OpCodes.  MercuryE sends messages back to the host PC
        MSG_OPCODE_TM_ASSERT                = 0xFF,
        // Place holder
        MSG_OPCODE_LAST_OPCODE              = MSG_OPCODE_TM_ASSERT;        

     public static final short
        PROT_ISO180006B                     = 0x03,
        PROT_GEN2                           = 0x05,
        PROT_UCODE                          = 0x06,
        PROT_IPX64                          = 0x07,
        PROT_IPX256                         = 0x08,
        PROT_ATA                            = 0x1D;
    
    public static final short
        PROT_CONF_KEY_GEN2_SESSION          = 0x00,
        PROT_CONF_KEY_GEN2_TARGET           = 0x01,
        PROT_CONF_KEY_GEN2_MILLERM          = 0x02,
        PROT_CONF_KEY_GEN2_Q                = 0x12,
        PROT_CONF_KEY_GEN2_BAP              = 0x81;
    
    public static final int
        GEN2_SESSION_0                      = 0x00,
        GEN2_SESSION_1                      = 0x01,
        GEN2_SESSION_2                      = 0x02,
        GEN2_SESSION_3                      = 0x03;

    public static final int
        TAG_METADATA_NONE                 = 0x0000,
        TAG_METADATA_READCOUNT            = 0x0001,
        TAG_METADATA_RSSI                 = 0x0002,
        TAG_METADATA_ANTENNAID            = 0x0004,
        TAG_METADATA_FREQUENCY            = 0x0008,
        TAG_METADATA_TIMESTAMP            = 0x0010,
        TAG_METADATA_PHASE                = 0x0020, 
        TAG_METADATA_PROTOCOL             = 0x0040,
        TAG_METADATA_DATA                 = 0x0080,
        TAG_METADATA_GPIO_STATUS          = 0x0100,
        TAG_METADATA_ALL    = TAG_METADATA_NONE | TAG_METADATA_READCOUNT | TAG_METADATA_RSSI |
                TAG_METADATA_ANTENNAID | TAG_METADATA_FREQUENCY | TAG_METADATA_TIMESTAMP | TAG_METADATA_PHASE |
                TAG_METADATA_PROTOCOL|TAG_METADATA_GPIO_STATUS;
    public static final short
        GET_TAGID_READ_OPTION_NONE          = 0x00,
        GET_TAGID_READ_OPTION_REWIND        = 0x01;
    
    public static final int 
        MSG_MAX_PACKET_LEN = 256,
        MSG_MAX_DATA_LENGTH = 250,
        MSG_MAX_TX_DATA_LEN = 248;
    
    public static final int
        TM_SUCCESS = 0x0000,
        TM_GOOD_HEADER = 0xff;
    
    public static final int
        OPT_PARAM_FLAG_LOW_POWER_TX_MODE   = 0x0001,
        OPT_PARAM_FLAG_MAX_EPC_LENGTH_496  = 0x0002;


    public static final int
        OPT_PARAM_KEY_UNIQUE_READS_ANTENNA         = 0x00,
        OPT_PARAM_KEY_PWR_SAVE                     = 0x01,
        OPT_PARAM_KEY_EXTENDED_EPC                 = 0x02,
        OPT_PARAM_KEY_ANTENNA_CTRL_GPIO            = 0x03,
        OPT_PARAM_KEY_ANT_DETECT_BEFORE_RF_ON      = 0x04,
        OPT_PARAM_KEY_TEMP_CHECK_BEFORE_RF_ON      = 0x05,
        OPT_PARAM_KEY_CHANGE_TIMESTAMP_WITH_MAXRSSI= 0x06,
        OPT_PARAM_KEY_PA_PROTECT_ENABLE            = 0x07,
        OPT_PARAM_KEY_UNIQUE_READS_DATA            = 0x08;
    
    public static final short
        SINGULATION_OPTION_SELECT_DISABLED       = 0x00,
        SINGULATION_OPTION_SELECT_ON_FULL_EPC    = 0x01,
        SINGULATION_OPTION_SELECT_ON_TID         = 0x02, 
        SINGULATION_OPTION_SELECT_ON_USERMEM     = 0x03,
        SINGULATION_OPTION_SELECT_ON_PARTIAL_EPC = 0x04,
        SINGULATION_OPTION_USE_PASSWORD          = 0x05,
        SINGULATION_OPTION_INVERSE_SELECT_BIT    = 0x08,
        SINGULATION_OPTION__FLAG_METADATA        = 0x10,
        SINGULATION_OPTION_EXTENDED_DATA_LENGTH  = 0x20,
        SINGULATION_OPTION_SECURE_READ_DATA      = 0x40;
    
    public static final short
        SINGULATION_FLAG_METADATA_ENABLED        = 0x10;
    
    public static final int
        READ_MULTIPLE_SEARCH_FLAGS_ONE_ANT                  = 0x0000,
        READ_MULTIPLE_SEARCH_FLAGS_ALL_ANT_PRIMARY_ONE      = 0x0001,
        READ_MULTIPLE_SEARCH_FLAGS_ALL_ANT_PRIMARY_TWO      = 0x0002,
        READ_MULTIPLE_SEARCH_FLAGS_SEARCH_LIST              = 0x0003,
        READ_MULTIPLE_SEARCH_FLAGS_EMBEDDED_OP              = 0x0004,
        READ_MULTIPLE_SEARCH_FLAGS_TAG_STREAMING            = 0x0008,
        READ_MULTIPLE_SEARCH_FLAGS_LARGE_TAG_POPULATION_SUPPORT = 0x0010,
        READ_MULTIPLE_SEARCH_FLAGS_STATUS_REPORT_STREAMING  = 0x0020,
        READ_MULTIPLE_FAST_SEARCH                           = 0x0080,
        READ_MULTIPLE_RETURN_ON_N_TAGS                  = 0x0040,
        READ_MULTIPLE_SEARCH_FLAGS_STATS_REPORT_STREAMING  = 0x0100;

    public static final int
        ISO180006B_COMMAND_DATA_READ           = 0x0B,
        ISO180006B_COMMAND_READ                = 0x0C,
        ISO180006B_COMMAND_WRITE               = 0x0D,
        ISO180006B_COMMAND_WRITE_MULTIPLE      = 0x0E,
        ISO180006B_COMMAND_WRITE4BYTE          = 0x1B,
        ISO180006B_COMMAND_WRITE4BYTE_MULTIPLE = 0x1C;
  
    public static final int 
        ISO180006B_WRITE_OPTION_READ_AFTER         = 0x00,
        ISO180006B_WRITE_OPTION_NO_VERIFY          = 0x01,
        ISO180006B_WRITE_OPTION_READ_VERIFY_AFTER  = 0x02,
        ISO180006B_WRITE_OPTION_GROUP_SELECT       = 0x03,
        ISO180006B_WRITE_OPTION_COUNT_PROVIDED     = 0x08,
        ISO180006B_WRITE_LOCK_NO                   = 0x00,
        ISO180006B_WRITE_LOCK_YES                  = 0x01,
        ISO180006B_LOCK_OPTION_TYPE_FOLLOWS        = 0x01,
        ISO180006B_LOCK_TYPE_QUERYLOCK_THEN_LOCK   = 0x01;

    public static final int
        ISO180006B_SELECT_OP_EQUALS      = 0,
        ISO180006B_SELECT_OP_NOTEQUALS   = 1,
        ISO180006B_SELECT_OP_LESSTHAN    = 2,
        ISO180006B_SELECT_OP_GREATERTHAN = 3,
        ISO180006B_SELECT_OP_INVERT      = 4;

    public static final short
        TAG_CHIP_TYPE_ANY                       = 0x00,
        TAG_CHIP_TYPE_ALIEN_HIGGS               = 0x01,
        TAG_CHIP_TYPE_NXP                       = 0x02,
        TAG_CHIP_TYPE_ALIEN_HIGGS3              = 0x05,
        TAG_CHIP_TYPE_HITACHI_HIBIKI            = 0x06,
        TAG_CHIP_TYPE_NXP_G2IL                  = 0x07,
        TAG_CHIP_TYPE_MONZA                     = 0x08;
    
    public static final short
        ANY_CHIP_OPTION_ERASE_BLOCK             = 0x00;
    
    public static final short
        ALIEN_HIGGS_CHIP_SUBCOMMAND_PARTIAL_LOAD_IMAGE  = 0x01,
        ALIEN_HIGGS_CHIP_SUBCOMMAND_FULL_LOAD_IMAGE     = 0x03;

    public static final short
        ALIEN_HIGGS3_CHIP_SUBCOMMAND_FAST_LOAD_IMAGE  = 0x01,
        ALIEN_HIGGS3_CHIP_SUBCOMMAND_LOAD_IMAGE       = 0x03,
        ALIEN_HIGGS3_CHIP_SUBCOMMAND_BLOCK_READ_LOCK  = 0x09;
    
    public static final short
        NXP_CHIP_SUBCOMMAND_SET_QUIET                   = 0x01,
        NXP_CHIP_SUBCOMMAND_RESET_QUIET                 = 0x02,
        NXP_CHIP_SUBCOMMAND_CHANGE_EAS                  = 0x03,
        NXP_CHIP_SUBCOMMAND_EAS_ALARM                   = 0x04,
        NXP_CHIP_SUBCOMMAND_CALIBRATE                   = 0x05,
        NXP_CHIP_SUBCOMMAND_CONFIG_CHANGE               = 0x07;

    public static final short
      HITACHI_HIBIKI_CHIP_SUBCOMMAND_READ_LOCK              = 0x00,
      HITACHI_HIBIKI_CHIP_SUBCOMMAND_GET_SYSTEM_INFORMATION = 0x01,
      HITACHI_HIBIKI_CHIP_SUBCOMMAND_SET_ATTENUATE          = 0x04,
      HITACHI_HIBIKI_CHIP_SUBCOMMAND_BLOCK_LOCK             = 0x05,
      HITACHI_HIBIKI_CHIP_SUBCOMMAND_BLOCK_READ_LOCK        = 0x06,
      HITACHI_HIBIKI_CHIP_SUBCOMMAND_WRITE_MULTIPLE_WORDS   = 0x07;

    public static final short
        FALSE=0x00,
        TRUE=0x01;

    public static final short
        READER_STATS_OPTION_GET                    = 0x00,
        READER_STATS_OPTION_RESET                  = 0x01,
        READER_STATS_OPTION_GET_PER_PORT           = 0x02;
    
    public static final short
        READER_STATS_FLAG_RF_ON_TIME               = (1<<0),
        READER_STATS_FLAG_NOISE_FLOOR              = (1<<1),
        READER_STATS_FLAG_LBT_BLOCK_COUNT          = (1<<2);

  public static final short
        FAULT_SUCCESS_CODE                            = 0x000,
        // Errors in each module
        FAULT_MSG_WRONG_NUMBER_OF_DATA                = 0x100,
        FAULT_INVALID_OPCODE                          = 0x101,
        FAULT_UNIMPLEMENTED_OPCODE                    = 0x102,
        FAULT_MSG_POWER_TOO_HIGH                      = 0x103,
        FAULT_MSG_INVALID_FREQ_RECEIVED               = 0x104,
        FAULT_MSG_INVALID_PARAMETER_VALUE             = 0x105,
        FAULT_MSG_POWER_TOO_LOW                       = 0x106,
        FAULT_MSG_WRONG_NUM_BITS_TO_TX                = 0x107,
        FAULT_MSG_TIMEOUT_TOO_LONG                    = 0x108,
        FAULT_UNIMPLEMENTED_FEATURE                   = 0x109,
        FAULT_INVALID_BAUD_RATE                       = 0x10A,

        // Bootloader faults
        FAULT_BL_INVALID_IMAGE_CRC                    = 0x200,
        FAULT_BL_INVALID_APP_END_ADDR                 = 0x201,
    
        // FLASH faults
        FAULT_FLASH_BAD_ERASE_PASSWORD                = 0x300,
        FAULT_FLASH_BAD_WRITE_PASSWORD                = 0x301,
        FAULT_FLASH_UNDEFINED_ERROR                   = 0x302,
        FAULT_FLASH_ILLEGAL_SECTOR                    = 0x303,
        FAULT_FLASH_WRITE_TO_NON_ERASED_AREA          = 0x304,
        FAULT_FLASH_CANNOT_ACCESS_MULTIPLE_SECTORS    = 0x305,
        FAULT_FLASH_VERIFY_FAILED                     = 0x306,
    
        // Protocol faults
        FAULT_NO_TAGS_FOUND                           = 0x400,
        FAULT_NO_PROTOCOL_DEFINED                     = 0x401,
        FAULT_INVALID_PROTOCOL_SPECIFIED              = 0x402,
        FAULT_WRITE_PASSED_LOCK_FAILED                = 0x403,
        FAULT_PROTOCOL_NO_DATA_READ                   = 0x404,
        FAULT_AFE_NOT_ON                              = 0x405,
        FAULT_PROTOCOL_WRITE_FAILED                   = 0x406,
        FAULT_NOT_IMPLEMENTED_FOR_THIS_PROTOCOL       = 0x407,
        FAULT_PROTOCOL_INVALID_WRITE_DATA             = 0x408,
        FAULT_PROTOCOL_INVALID_ADDRESS                = 0x409,
        FAULT_GENERAL_TAG_ERROR                       = 0x40A,
        FAULT_DATA_TOO_LARGE                          = 0x40B,
        FAULT_PROTOCOL_INVALID_KILL_PASSWORD          = 0x40C,
        FAULT_TEST_FAILED                             = 0x40D,
        FAULT_PROTOCOL_KILL_FAILED                    = 0x40E,
        FAULT_PROTOCOL_BIT_DECODING_FAILED            = 0x40F,
        FAULT_PROTOCOL_INVALID_EPC                    = 0x410,
        FAULT_PROTOCOL_INVALID_NUM_DATA               = 0x411,
        FAULT_GEN2_PROTOCOL_OTHER_ERROR               = 0x420,
        FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC     = 0x423,
        FAULT_GEN2_PROTOCOL_MEMORY_LOCKED             = 0x424,
        FAULT_GEN2_PROTOCOL_INSUFFICIENT_POWER        = 0x42B,
        FAULT_GEN2_PROTOCOL_NON_SPECIFIC_ERROR        = 0x42F,
        FAULT_GEN2_PROTOCOL_UNKNOWN_ERROR             = 0x430,
            
        // AHAL faults
        FAULT_AHAL_INVALID_FREQ                       = 0x500,
        FAULT_AHAL_CHANNEL_OCCUPIED                   = 0x501,
        FAULT_AHAL_TRANSMITTER_ON                     = 0x502,
        FAULT_AHAL_ANTENNA_NOT_CONNECTED              = 0x503,
        FAULT_AHAL_TEMPERATURE_EXCEED_LIMITS          = 0x504,
        FAULT_AHAL_HIGH_RETURN_LOSS                   = 0x505,
        FAULT_AHAL_PLL_NOT_LOCKED                     = 0x506,
        FAULT_AHAL_INVALID_ANTENNA_CONFIG             = 0x507,

        // Tag ID Buffer Faults
        FAULT_TAG_ID_BUFFER_NOT_ENOUGH_TAGS_AVAILABLE = 0x600,
        FAULT_TAG_ID_BUFFER_FULL                      = 0x601,
        FAULT_TAG_ID_BUFFER_REPEATED_TAG_ID           = 0x602,
        FAULT_TAG_ID_BUFFER_NUM_TAG_TOO_LARGE         = 0x603,
        FAULT_TAG_ID_BUFFER_AUTH_REQUEST              = 0x604,

        // System Errors
        FAULT_SYSTEM_UNKNOWN_ERROR                    = 0x7F00,
        FAULT_TM_ASSERT_FAILED                        = 0x7F01;

    
}
