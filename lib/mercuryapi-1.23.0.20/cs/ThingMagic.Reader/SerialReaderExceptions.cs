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

using System;

namespace ThingMagic
{
    #region M5eException

    /// <summary>
    /// M5e-related exception
    /// </summary>
    public class M5eException : ReaderException
    {
        #region Construction
        /// <summary>
        /// Create M5e-related exception
        /// </summary>
        /// <param name="message">the exception message</param>
        public M5eException(string message) : base(message) { }

        #endregion
    }

    #endregion

    #region M5eStatusException

    /// <summary>
    /// Error status code returned from M5e
    /// </summary>
    public class M5eStatusException : ReaderCodeException
    {
        #region Construction

        /// <summary>
        /// Create M5e reader error exception
        /// </summary>
        /// <param name="message">String describing error</param>
        /// <param name="code">Reader response status code</param>
        public M5eStatusException(string message, UInt16 code)
            : base(message, (int) code)
        {
        }

        /// <summary>
        /// Create M5e reader error exception with default message
        /// </summary>
        /// <param name="code">Reader response status code</param>
        public M5eStatusException(UInt16 code)
            : this("Bad M5e response code: " + code.ToString("X4"), code)
        {
        }

        #endregion
    }
 
    #endregion

    #region Common Exceptions

    #region FAULT_MSG_WRONG_NUMBER_OF_DATA_Exception

    /// <summary>
    /// Invalid number of arguments
    /// </summary>
    public class FAULT_MSG_WRONG_NUMBER_OF_DATA_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x100;

        /// <summary>
        /// Create FAULT_MSG_WRONG_NUMBER_OF_DATA Exception
        /// </summary>
        public FAULT_MSG_WRONG_NUMBER_OF_DATA_Exception()
            : base("The data length in the message is less than or more "
        + "than the number of arguments required for the opcode.", StatusCode)
        {
        }

    } 
    #endregion

    #region FAULT_INVALID_OPCODE_Exception

    /// <summary>
    /// Command opcode not recognized.
    /// </summary>
    public class FAULT_INVALID_OPCODE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x101;

        /// <summary>
        /// 
        /// </summary>
        public FAULT_INVALID_OPCODE_Exception()
            : base("The opCode received is invalid or not supported with "
        + "the current version of code.", StatusCode)
        {
        }

        /// <summary>
        /// Create FAULT_INVALID_OPCODE Exception
        /// </summary>
        public FAULT_INVALID_OPCODE_Exception(string description)
            : base(description,StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_UNIMPLEMENTED_OPCODE_Exception

    /// <summary>
    /// Command opcode recognized, but is not supported.
    /// </summary>
    public class FAULT_UNIMPLEMENTED_OPCODE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x102;

        /// <summary>
        /// Create FAULT_UNIMPLEMENTED_OPCODE Exception
        /// </summary>
        public FAULT_UNIMPLEMENTED_OPCODE_Exception()
            : base("Unimplemented Opcode", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_MSG_POWER_TOO_HIGH_Exception

    /// <summary>
    /// Requested power setting is above the allowed maximum.
    /// </summary>
    public class FAULT_MSG_POWER_TOO_HIGH_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x103;

        /// <summary>
        /// Create FAULT_MSG_POWER_TOO_HIGH Exception
        /// </summary>
        public FAULT_MSG_POWER_TOO_HIGH_Exception()
            : base("A message was sent to set the read or write power to "
        + "a level that is higher than the HW supports.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_MSG_INVALID_FREQ_RECEIVED_Exception

    /// <summary>
    /// Requested frequency is outside the allowed range.
    /// </summary>
    public class FAULT_MSG_INVALID_FREQ_RECEIVED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x104;

        /// <summary>
        /// Create FAULT_MSG_INVALID_FREQ_RECEIVED Exception
        /// </summary>
        public FAULT_MSG_INVALID_FREQ_RECEIVED_Exception()
            : base("A message was received by the reader to set the frequency "
        + "outside the supported range", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_MSG_INVALID_PARAMETER_VALUE_Exception

    /// <summary>
    /// Parameter value is outside the allowed range.
    /// </summary>
    public class FAULT_MSG_INVALID_PARAMETER_VALUE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x105;

        /// <summary>
        /// Create FAULT_MSG_INVALID_PARAMETER_VALUE Exception
        /// </summary>
        public FAULT_MSG_INVALID_PARAMETER_VALUE_Exception()
            : base("The reader received a valid command with an unsupported or "
        + "invalid parameter", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_MSG_POWER_TOO_LOW_Exception

    /// <summary>
    ///  Requested power setting is below the allowed minimum.
    /// </summary>
    public class FAULT_MSG_POWER_TOO_LOW_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x106;

        /// <summary>
        /// Create FAULT_MSG_POWER_TOO_LOW Exception
        /// </summary>
        public FAULT_MSG_POWER_TOO_LOW_Exception()
            : base("A message was received to set the read or write power to a "
        + "level that is lower than the HW supports.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_UNIMPLEMENTED_FEATURE_Exception

    /// <summary>
    /// Command not supported.
    /// </summary>
    // TODO: Find out what distinguishes FAULT_UNIMPLEMENTED_FEATURE from FAULT_INVALID_OPCODE
    public class FAULT_UNIMPLEMENTED_FEATURE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x109;

        /// <summary>
        /// Create FAULT_UNIMPLEMENTED_FEATURE Exception
        /// </summary>
        public FAULT_UNIMPLEMENTED_FEATURE_Exception()
            : base("Unimplemented feature.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_INVALID_BAUD_RATE_Exception

    /// <summary>
    /// Requested serial speed is not supported.
    /// </summary>
    public class FAULT_INVALID_BAUD_RATE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x10A;

        /// <summary>
        /// Create FAULT_INVALID_BAUD_RATE Exception
        /// </summary>
        public FAULT_INVALID_BAUD_RATE_Exception()
            : base("Invalid baud rate.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_INVALID_REGION_Exception

    /// <summary>
    /// Region is not supported.
    /// </summary>
    public class FAULT_INVALID_REGION_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x10B;

        /// <summary>
        /// Create FAULT_INVALID_REGION Exception
        /// </summary>
        public FAULT_INVALID_REGION_Exception()
            : base("Invalid Region.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_INVALID_LICENSE_KEY_Exception

    /// <summary>
    /// Command opcode not recognized.
    /// </summary>
    public class FAULT_INVALID_LICENSE_KEY_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x10c;

        /// <summary>
        /// 
        /// </summary>
        public FAULT_INVALID_LICENSE_KEY_Exception()
            : base("Invalid license key", StatusCode)
        {
        }

        /// <summary>
        /// Create FAULT_INVALID_OPCODE Exception
        /// </summary>
        public FAULT_INVALID_LICENSE_KEY_Exception(string description)
            : base(description, StatusCode)
        {
        }
    }
    #endregion

    #endregion

    #region Bootloader Exceptions

    #region FAULT_BL_INVALID_IMAGE_CRC_Exception

    /// <summary>
    /// Firmware is corrupt: Checksum doesn't match content.
    /// </summary>
    public class FAULT_BL_INVALID_IMAGE_CRC_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x200;

        /// <summary>
        /// Create FAULT_BL_INVALID_IMAGE_CRC Exception
        /// </summary>
        public FAULT_BL_INVALID_IMAGE_CRC_Exception()
            : base("CRC validation of firmware image failed", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_BL_INVALID_APP_END_ADDR_Exception

    /// <summary>
    /// Firmware corruprt: Internal address marker is invalid.
    /// </summary>
    public class FAULT_BL_INVALID_APP_END_ADDR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x201;

        /// <summary>
        /// Create FAULT_BL_INVALID_APP_END_ADDR Exception
        /// </summary>
        public FAULT_BL_INVALID_APP_END_ADDR_Exception()
            : base("The last word of the firmware image stored in the reader's "
        + "flash ROM does not have the correct address value.", StatusCode)
        {
        }
    }

    #endregion 

    #endregion

    #region Flash Exceptions

    #region FAULT_FLASH_BAD_ERASE_PASSWORD_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_BAD_ERASE_PASSWORD_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x300;

        /// <summary>
        /// Create FAULT_FLASH_BAD_ERASE_PASSWORD Exception
        /// </summary>
        public FAULT_FLASH_BAD_ERASE_PASSWORD_Exception()
            : base("A command was received to erase some part of the flash but "
        + "the password supplied with the command was incorrect.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_FLASH_BAD_WRITE_PASSWORD_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_BAD_WRITE_PASSWORD_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x301;

        /// <summary>
        /// Create FAULT_FLASH_BAD_WRITE_PASSWORD Exception
        /// </summary>
        public FAULT_FLASH_BAD_WRITE_PASSWORD_Exception()
            : base("A command was received to write some part of the flash but "
        + "the password supplied with the command was incorrect.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_FLASH_UNDEFINED_ERROR_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_UNDEFINED_ERROR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x302;
        /// <summary>
        /// Create FAULT_FLASH_UNDEFINED_ERROR Exception
        /// </summary>
        public FAULT_FLASH_UNDEFINED_ERROR_Exception()
            : base("FAULT_FLASH_UNDEFINED_ERROR - This is an internal error and "
        + "it is caused by a software problem in the M4e.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_FLASH_ILLEGAL_SECTOR_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_ILLEGAL_SECTOR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x303;

        /// <summary>
        /// Create FAULT_FLASH_ILLEGAL_SECTOR Exception
        /// </summary>
        public FAULT_FLASH_ILLEGAL_SECTOR_Exception()
            : base("An erase or write flash command was received with the sector "
        + "value and password not matching.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_FLASH_WRITE_TO_NON_ERASED_AREA_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_WRITE_TO_NON_ERASED_AREA_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x304;

        /// <summary>
        /// Create FAULT_FLASH_WRITE_TO_NON_ERASED_AREA_Exception
        /// </summary>
        public FAULT_FLASH_WRITE_TO_NON_ERASED_AREA_Exception()
            : base("The M4e received a write flash command to an area of flash "
        + "that was not previously erased.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_FLASH_WRITE_TO_ILLEGAL_SECTOR_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_WRITE_TO_ILLEGAL_SECTOR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x305;

        /// <summary>
        /// Create FAULT_FLASH_WRITE_TO_ILLEGAL_SECTOR_Exception
        /// </summary>
        public FAULT_FLASH_WRITE_TO_ILLEGAL_SECTOR_Exception()
            : base("A flash command can not access multiple sectors.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_FLASH_VERIFY_FAILED_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_FLASH_VERIFY_FAILED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x306;

        /// <summary>
        /// Create FAULT_FLASH_VERIFY_FAILED Exception
        /// </summary>
        public FAULT_FLASH_VERIFY_FAILED_Exception()
            : base("Verifying flash contents failed.", StatusCode)
        {
        }
    }
    #endregion

    #endregion

    #region Protocol Exceptions

    #region FAULT_NO_TAGS_FOUND_Exception

    /// <summary>
    /// Reader was asked to find tags, but none were detected.
    /// </summary>
    public class FAULT_NO_TAGS_FOUND_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x400;

        /// <summary>
        /// Create FAULT_NO_TAGS_FOUND Exception
        /// </summary>
        public FAULT_NO_TAGS_FOUND_Exception()
            : base("No tags found.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_NO_PROTOCOL_DEFINED_Exception

    /// <summary>
    /// RFID protocol has not been configured.
    /// </summary>
    public class FAULT_NO_PROTOCOL_DEFINED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x401;

        /// <summary>
        /// Create FAULT_NO_PROTOCOL_DEFINED Exception
        /// </summary>
        public FAULT_NO_PROTOCOL_DEFINED_Exception()
            : base("A command was received to perform a protocol command but no "
        + "protocol was set.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_INVALID_PROTOCOL_SPECIFIED_Exception

    /// <summary>
    /// Requested RFID protocol is not recognized.
    /// </summary>
    public class FAULT_INVALID_PROTOCOL_SPECIFIED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x402;

        /// <summary>
        /// Create FAULT_INVALID_PROTOCOL_SPECIFIED Exception
        /// </summary>
        public FAULT_INVALID_PROTOCOL_SPECIFIED_Exception()
            : base("A Set Protocol command was received for a protocol value that "
        + "is not supported", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_WRITE_PASSED_LOCK_FAILED_Exception

    /// <summary>
    /// For write-then-lock commands, tag was successfully written, but lock failed.
    /// </summary>
    public class FAULT_WRITE_PASSED_LOCK_FAILED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x403;

        /// <summary>
        /// Create FAULT_WRITE_PASSED_LOCK_FAILED Exception
        /// </summary>
        public FAULT_WRITE_PASSED_LOCK_FAILED_Exception()
            : base("Lock failed during a Write Tag Data for ISO18000-6B or UCODE.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_PROTOCOL_NO_DATA_READ_Exception

    /// <summary>
    /// Tag data was requested, but could not be read.
    /// </summary>
    public class FAULT_PROTOCOL_NO_DATA_READ_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x404;

        /// <summary>
        /// Create FAULT_PROTOCOL_NO_DATA_READ Exception
        /// </summary>
        public FAULT_PROTOCOL_NO_DATA_READ_Exception()
            : base("A Read Tag ID or Data command was sent but did not succeed.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_AFE_NOT_ON_Exception

    /// <summary>
    /// Reader not fully initialized and hasn't yet turned on its radio.  Have you set region?
    /// </summary>
    public class FAULT_AFE_NOT_ON_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x405;

        /// <summary>
        /// Create FAULT_AFE_NOT_ON Exception
        /// </summary>
        public FAULT_AFE_NOT_ON_Exception()
            : base("A command was received while the AFE was in the off state. "
        + "Please check that a region has been selected and the AFE has "
        + "not been disabled.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_PROTOCOL_WRITE_FAILED_Exception

    /// <summary>
    /// Write to tag failed.
    /// </summary>
    public class FAULT_PROTOCOL_WRITE_FAILED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x406;

        /// <summary>
        /// Create FAULT_PROTOCOL_WRITE_FAILED Exception
        /// </summary>
        public FAULT_PROTOCOL_WRITE_FAILED_Exception()
            : base("Tag write operation failed.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_NOT_IMPLEMENTED_FOR_THIS_PROTOCOL_Exception

    /// <summary>
    /// Command is not supported in the current RFID protocol.
    /// </summary>
    public class FAULT_NOT_IMPLEMENTED_FOR_THIS_PROTOCOL_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x407;

        /// <summary>
        /// Create FAULT_NOT_IMPLEMENTED_FOR_THIS_PROTOCOL Exception
        /// </summary>
        public FAULT_NOT_IMPLEMENTED_FOR_THIS_PROTOCOL_Exception()
            : base("A command was received which is not supported by the currently "
        + "selected protocol.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_PROTOCOL_INVALID_WRITE_DATA_Exception

    /// <summary>
    /// Data does not conform to protocol standards.
    /// For example, EPC0 and EPC1 require EPC header bits to match the length of the EPC, as defined in the EPCGlobal Tag Data Standard.
    /// </summary>
    public class FAULT_PROTOCOL_INVALID_WRITE_DATA_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x408;

        /// <summary>
        /// Create FAULT_PROTOCOL_INVALID_WRITE_DATA Exception
        /// </summary>
        public FAULT_PROTOCOL_INVALID_WRITE_DATA_Exception()
            : base("In EPC0+, the first two bits determine the tag ID length. If "
        + "the first two bits are 0b00, then the tag ID must be 96-bits. "
        + "Otherwise the tag ID is 64 bits.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_PROTOCOL_INVALID_ADDRESS_Exception

    /// <summary>
    /// Requested data address is outside the valid range.
    /// </summary>
    public class FAULT_PROTOCOL_INVALID_ADDRESS_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x409;

        /// <summary>
        /// Create FAULT_PROTOCOL_INVALID_ADDRESS Exception
        /// </summary>
        public FAULT_PROTOCOL_INVALID_ADDRESS_Exception()
            : base("A command was received to write to an invalid address in the "
        + "tag data address space.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_GENERAL_TAG_ERROR_Exception

    /// <summary>
    /// Unknown error during RFID operation.
    /// </summary>
    public class FAULT_GENERAL_TAG_ERROR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x40A;

        /// <summary>
        /// Create FAULT_GENERAL_TAG_ERROR Exception
        /// </summary>
        public FAULT_GENERAL_TAG_ERROR_Exception()
            : base("General Tag Error", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_DATA_TOO_LARGE_Exception

    /// <summary>
    /// Read Tag Data was asked for more data than it supports.
    /// </summary>
    public class FAULT_DATA_TOO_LARGE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x40B;

        /// <summary>
        /// Create FAULT_DATA_TOO_LARGE Exception
        /// </summary>
        public FAULT_DATA_TOO_LARGE_Exception()
            : base("A command was received to Read Tag Data with a data value that "
        + "is not the correct size.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_PROTOCOL_INVALID_KILL_PASSWORD_Exception

    /// <summary>
    /// Incorrect password was provided to Kill Tag.
    /// </summary>
    public class FAULT_PROTOCOL_INVALID_KILL_PASSWORD_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x40C;

        /// <summary>
        /// Create FAULT_PROTOCOL_INVALID_KILL_PASSWORD Exception
        /// </summary>
        public FAULT_PROTOCOL_INVALID_KILL_PASSWORD_Exception()
            : base("An incorrect kill password was received as part of the Kill command.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_PROTOCOL_KILL_FAILED_Exception

    /// <summary>
    /// Kill failed for unknown reason.
    /// </summary>
    public class FAULT_PROTOCOL_KILL_FAILED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x40E;

        /// <summary>
        /// Create FAULT_PROTOCOL_KILL_FAILED Exception
        /// </summary>
        public FAULT_PROTOCOL_KILL_FAILED_Exception()
            : base("Kill failed.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x40F;

        /// <summary>
        /// Create FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception
        /// </summary>
        public FAULT_PROTOCOL_BIT_DECODING_FAILED_Exception()
            : base("Protocol bit decoding failed.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_PROTOCOL_INVALID_EPC_Exception
    /// <summary>
    /// Invalid epc value was specified for an operation.
    /// </summary>
    public class FAULT_PROTOCOL_INVALID_EPC_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x410;

        /// <summary>
        /// Create FAULT_PROTOCOL_INVALID_EPC_Exception
        /// </summary>
        public FAULT_PROTOCOL_INVALID_EPC_Exception()
            : base("Invalid epc value has been specified for an operation.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_PROTOCOL_INVALID_NUM_DATA_Exception
    /// <summary>
    /// Invalid data was specified for an operation
    /// </summary>
    public class FAULT_PROTOCOL_INVALID_NUM_DATA_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x411;

        /// <summary>
        /// Create FAULT_PROTOCOL_INVALID_NUM_DATA_Exception
        /// </summary>
        public FAULT_PROTOCOL_INVALID_NUM_DATA_Exception()
            : base("Invalid data has been specified for an operation.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_GEN2_PROTOCOL_OTHER_ERROR_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_GEN2_PROTOCOL_OTHER_ERROR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x420;
        /// <summary>
        /// Create FAULT_GEN2_PROTOCOL_OTHER_ERROR_Exception
        /// </summary>
        public FAULT_GEN2_PROTOCOL_OTHER_ERROR_Exception()
            : base("Other Gen2 error.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x423;

        /// <summary>
        /// Create FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception
        /// </summary>
        public FAULT_GEN2_PROTOCOL_MEMORY_OVERRUN_BAD_PC_Exception()
            : base("Gen2 memory overrun - bad PC.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_GEN2_PROTOCOL_MEMORY_LOCKED_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_GEN2_PROTOCOL_MEMORY_LOCKED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x424;

        /// <summary>
        /// Create FAULT_GEN2_PROTOCOL_MEMORY_LOCKED_Exception
        /// </summary>
        public FAULT_GEN2_PROTOCOL_MEMORY_LOCKED_Exception()
            : base("Gen2 memory locked.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_GEN2_PROTOCOL_INSUFFICIENT_POWER_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_GEN2_PROTOCOL_INSUFFICIENT_POWER_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x42B;

        /// <summary>
        /// Create FAULT_GEN2_PROTOCOL_INSUFFICIENT_POWER_Exception
        /// </summary>
        public FAULT_GEN2_PROTOCOL_INSUFFICIENT_POWER_Exception()
            : base("Gen2 tag has insufficent power.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_GEN2_PROTOCOL_NON_SPECIFIC_ERROR_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_GEN2_PROTOCOL_NON_SPECIFIC_ERROR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x42F;

        /// <summary>
        /// Create FAULT_GEN2_PROTOCOL_NON_SPECIFIC_ERROR
        /// </summary>
        public FAULT_GEN2_PROTOCOL_NON_SPECIFIC_ERROR_Exception()
            : base("Gen2 unspecific error.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_GEN2_PROTOCOL_UNKNOWN_ERROR_Exception
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_GEN2_PROTOCOL_UNKNOWN_ERROR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x430;

        /// <summary>
        /// Create FAULT_GEN2_PROTOCOL_UNKNOWN_ERROR_Exception
        /// </summary>
        public FAULT_GEN2_PROTOCOL_UNKNOWN_ERROR_Exception()
            : base("Gen2 unknown error.", StatusCode)
        {
        }
    }
    #endregion

    #endregion

    #region Analog Hardware Abstraction Layer Exception

    #region FAULT_AHAL_ANTENNA_NOT_CONNECTED_Exception

    /// <summary>
    /// Antenna not detected during pre-transmit safety test
    /// </summary>
    public class FAULT_AHAL_ANTENNA_NOT_CONNECTED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x503;

        /// <summary>
        /// Create FAULT_AHAL_ANTENNA_NOT_CONNECTED Exception
        /// </summary>
        public FAULT_AHAL_ANTENNA_NOT_CONNECTED_Exception()
            : base("Antenna not connected.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_AHAL_TEMPERATURE_EXCEED_LIMITS_Exception

    /// <summary>
    /// Reader temperature outside safe range
    /// </summary>
    public class FAULT_AHAL_TEMPERATURE_EXCEED_LIMITS_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x504;

        /// <summary>
        /// Create FAULT_AHAL_TEMPERATURE_EXCEED_LIMITS Exception
        /// </summary>
        public FAULT_AHAL_TEMPERATURE_EXCEED_LIMITS_Exception()
            : base("The module has exceeded the maximum or minimum operating "
        + "temperature anf will not allow an RF operation until it is back "
        + "in range.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_AHAL_HIGH_RETURN_LOSS_Exception

    /// <summary>
    /// Excess power detected at transmitter port, usually due to antenna tuning mismatch.
    /// </summary>
    public class FAULT_AHAL_HIGH_RETURN_LOSS_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x505;

        /// <summary>
        /// Create FAULT_AHAL_HIGH_RETURN_LOSS Exception
        /// </summary>
        public FAULT_AHAL_HIGH_RETURN_LOSS_Exception()
            : base("The module has detected high return loss and has ended "
        + "RF operations to avoid module damage.", StatusCode)
        {
        }
    }
    
    #endregion

    #region FAULT_AHAL_INVALID_FREQ_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_AHAL_INVALID_FREQ_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x500;

        /// <summary>
        /// Create FAULT_AHAL_INVALID_FREQ_Exception
        /// </summary>
        public FAULT_AHAL_INVALID_FREQ_Exception()
            : base("Invalid frequency.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_AHAL_CHANNEL_OCCUPIED_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_AHAL_CHANNEL_OCCUPIED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x501;

        /// <summary>
        /// Create FAULT_AHAL_CHANNEL_OCCUPIED_Exception
        /// </summary>
        public FAULT_AHAL_CHANNEL_OCCUPIED_Exception()
            : base("Channel occupied.", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_AHAL_TRANSMITTER_ON_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_AHAL_TRANSMITTER_ON_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x502;

        /// <summary>
        /// Create FAULT_AHAL_TRANSMITTER_ON Exception
        /// </summary>
        public FAULT_AHAL_TRANSMITTER_ON_Exception()
            : base("Transmitter already on.", StatusCode)
        {
        }
    }
    #endregion

    #endregion

    #region Tag ID Buffer Exceptions

    #region FAULT_TAG_ID_BUFFER_NOT_ENOUGH_TAGS_AVAILABLE_Exception

    /// <summary>
    /// Asked for more tags than were available in the buffer.
    /// </summary>
    public class FAULT_TAG_ID_BUFFER_NOT_ENOUGH_TAGS_AVAILABLE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x600;

        /// <summary>
        /// Create FAULT_TAG_ID_BUFFER_NOT_ENOUGH_TAGS_AVAILABLE Exception
        /// </summary>
        public FAULT_TAG_ID_BUFFER_NOT_ENOUGH_TAGS_AVAILABLE_Exception()
            : base("Not enough tags in tag ID buffer.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_TAG_ID_BUFFER_FULL_Exception

    /// <summary>
    /// Too many tags are in buffer.  Remove some with Get Tag ID Buffer or Clear Tag ID Buffer.
    /// </summary>
    public class FAULT_TAG_ID_BUFFER_FULL_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x601;
        /// <summary>
        /// Create FAULT_TAG_ID_BUFFER_FULL Exception
        /// </summary>
        public FAULT_TAG_ID_BUFFER_FULL_Exception()
            : base("Tag ID buffer full.", StatusCode)
        {
        }
    }

    #endregion

    #region FAULT_TAG_ID_BUFFER_REPEATED_TAG_ID_Exception

    /// <summary>
    /// Internal error -- reader is trying to insert a duplicate tag record.  Contact support.
    /// </summary>
    public class FAULT_TAG_ID_BUFFER_REPEATED_TAG_ID_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x602;

        /// <summary>
        /// Create FAULT_TAG_ID_BUFFER_REPEATED_TAG_ID Exception
        /// </summary>
        public FAULT_TAG_ID_BUFFER_REPEATED_TAG_ID_Exception()
            : base("Tag ID buffer repeated tag ID.", StatusCode)
        {
        }
    } 
    #endregion

    #region FAULT_TAG_ID_BUFFER_NUM_TAG_TOO_LARGE_Exception

    /// <summary>
    /// Asked for tags than a single transaction can handle.
    /// </summary>
    public class FAULT_TAG_ID_BUFFER_NUM_TAG_TOO_LARGE_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x603;

        /// <summary>
        /// Create FAULT_TAG_ID_BUFFER_NUM_TAG_TOO_LARGE Exception
        /// </summary>
        public FAULT_TAG_ID_BUFFER_NUM_TAG_TOO_LARGE_Exception()
            : base("Number of tags too large.", StatusCode)
        {
        }
    }
    
    #endregion

    #region FAULT_TAG_ID_BUFFER_AUTH_REQUEST_Exception

    /// <summary>
    /// Asked for tags than a single transaction can handle.
    /// </summary>
    public class FAULT_TAG_ID_BUFFER_AUTH_REQUEST_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x604;

        #region Fields

        private byte[] _readerMessage;

        #endregion

        #region Properties

        /// <summary>
        /// Copy of raw message 
        /// </summary>
        public byte[] ReaderMessage { get { return (byte[])_readerMessage.Clone(); } }

        #endregion

        /// <summary>
        /// Create FAULT_TAG_ID_BUFFER_AUTH_REQUEST Exception
        /// </summary>
        public FAULT_TAG_ID_BUFFER_AUTH_REQUEST_Exception()
            : base("Indicates the client data request.", StatusCode)
        {
        }

        /// <summary>
        /// Create FAULT_TAG_ID_BUFFER_AUTH_REQUEST Exception
        /// </summary>
        /// <param name="readerMessage">Copy of raw message</param>
        public FAULT_TAG_ID_BUFFER_AUTH_REQUEST_Exception(byte[] readerMessage)
            : base("Indicates the client data request.", StatusCode)
        {
            _readerMessage = (byte[])readerMessage.Clone();
        }
    }

    #endregion

    #endregion

    #region System Exceptions

    #region FAULT_SYSTEM_UNKNOWN_ERROR_Exception

    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_SYSTEM_UNKNOWN_ERROR_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x7F00;

        /// <summary>
        /// Create FAULT_SYSTEM_UNKNOWN_ERROR Exception
        /// </summary>
        public FAULT_SYSTEM_UNKNOWN_ERROR_Exception()
            : base("System unknown error", StatusCode)
        {
        }
    }
    #endregion

    #region FAULT_TM_ASSERT_FAILED_Exception
    
    /// <summary>
    /// Internal reader error.  Contact support.
    /// </summary>
    public class FAULT_TM_ASSERT_FAILED_Exception : M5eStatusException
    {
        /// <summary>
        /// Serial protocol status code for this exception
        /// </summary>
        public const UInt16 StatusCode = 0x7F01;

        /// <summary>
        /// Create FAULT_TM_ASSERT_FAILED_Exception
        /// </summary>
        public FAULT_TM_ASSERT_FAILED_Exception(String msg)
            : base(msg, StatusCode)
        {
        }


    }
    #endregion

    #endregion
} 
