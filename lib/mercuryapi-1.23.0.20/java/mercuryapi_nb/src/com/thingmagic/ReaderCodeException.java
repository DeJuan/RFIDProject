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

/**
 * Thrown when an error code is received from a reader device.
 */ 
public class ReaderCodeException extends ReaderException
{
  int code;

  ReaderCodeException(int code, String message)
  {
    super(message);
    this.code = code;
  }

  ReaderCodeException(int code)
  {
    this(code, faultCodeToMessage(code));
  }

  public int getCode()
  {
    return code;
  }

  static String faultCodeToMessage(int status) {
    String messageString;
    switch (status) {
    case 0x100:
      messageString = "The data length in the message is less than or more "
        + "than the number of arguments required for the opcode.";
      break;
    case 0x101:
      messageString = "The opCode received is invalid or not supported with "
        + "the current version of code.";
      break;
    case 0x102:
      messageString = "Unimplemented Opcode";
      break;
    case 0x103:
      messageString = "A message was sent to set the read or write power to "
        + "a level that is higher than the HW supports.";
      break;
    case 0x104:
      messageString = "A message was received by the reader to set the frequency "
        + "outside the supported range";
      break;
    case 0x105:
      messageString = "The reader received a valid command with an unsupported or "
        + "invalid parameter";
      break;
    case 0x106:
      messageString = "A message was received to set the read or write power to a "
        + "level that is lower than the HW supports.";
      break;                        
    case 0x107:
      messageString = "Wrong number of bits to transmit.";
      break;
    case 0x108:
      messageString = "Timeout too long.";
      break;
    case 0x109:
      messageString = "Unimplemented feature.";
      break;
    case 0x10a:
      messageString = "Invalid baud rate.";
      break;
    case 0x10b:
      messageString = "Invalid Region.";
      break;
    case 0x10c:
      messageString = "Invalid License Key";
      break;
    case 0x200:
      messageString = "CRC validation of firmware image failed";
      break;                        
    case 0x201:
      messageString = "The last word of the firmware image stored in the reader's " 
        + "flash ROM does not have the correct address value.";
      break;                        
    case 0x300:
      messageString = "A command was received to erase some part of the flash but "
        + "the password supplied with the command was incorrect.";
      break;                        
    case 0x301:
      messageString = "A command was received to write some part of the flash but "
        + "the password supplied with the command was incorrect.";
      break;                        
    case 0x302:
      messageString = "FAULT_FLASH_UNDEFINED_ERROR - This is an internal error and "
        + "it is caused by a software problem in the M4e.";
      break;                        
    case 0x303:
      messageString = "An erase or write flash command was received with the sector "
        + "value and password not matching.";
      break;                        
    case 0x304:
      messageString = "The M4e received a write flash command to an area of flash "
        + "that was not previously erased.";
      break;                        
    case 0x305:
      messageString = "A flash command can not access multiple sectors.";
      break;
    case 0x306:
      messageString = "Verifying flash contents failed.";
      break;
    case 0x400:
      messageString = "No tags found.";
      break;                        
    case 0x401:
      messageString = "A command was received to perform a protocol command but no "
        + "protocol was set.";
      break;                        
    case 0x402:
      messageString = "A Set Protocol command was received for a protocol value that "
        + "is not supported";
      break;                        
    case 0x403:
      messageString = "Lock failed during a Write Tag Data for ISO18000-6B or UCODE.";
      break;                        
    case 0x404:
      messageString = "A Read Tag ID or Data command was sent but did not succeed.";
      break;                        
    case 0x405:
      messageString = "A command was received while the AFE was in the off state. "
        + "Please check that a region has been selected and the AFE has "
        + "not been disabled.";
      break;                        
    case 0x406:
      messageString = "Tag write operation failed.";
      break;                        
    case 0x407:
      messageString = "A command was received which is not supported by the currently "
        + "selected protocol.";
      break;                        
    case 0x408:
      messageString = "In EPC0+, the first two bits determine the tag ID length. If "
        + "the first two bits are 0b00, then the tag ID must be 96-bits. "
        + "Otherwise the tag ID is 64 bits.";
      break;                        
    case 0x409:
      messageString = "A command was received to write to an invalid address in the "
        + "tag data address space.";
      break;                        
    case 0x40a:
      messageString = "General Tag Error (this error is used by the M5e GEN2 module).";
      break;                        
    case 0x40b:
      messageString = "A command was received to Read Tag Data with a data value that "
        + "is not the correct size.";
      break;                        
    case 0x40c:
      messageString = "An incorrect kill password was received as part of the Kill command.";
      break;                        
    case 0x40d:
      messageString = "Test failed.";
      break;
    case 0x40e:
      messageString = "Kill failed.";
      break;
    case 0x40f:
      messageString = "Bit decoding failed.";
      break;
    case 0x410:
      messageString = "Invalid EPC on tag.";
      break;
    case 0x411:
      messageString = "Invalid quantity of data in tag message.";
      break;
    case 0x420:
      messageString = "Other Gen2 error.";
      break;
    case 0x423:
      messageString = "Gen2 memory overrun - bad PC.";
      break;
    case 0x424:
      messageString = "Gen2 memory locked.";
      break;
    case 0x42b:
      messageString = "Gen2 tag has insufficent power.";
      break;
    case 0x42f:
      messageString = "Gen2 unspecific error.";
      break;
    case 0x430:
      messageString = "Gen2 unknown error.";
      break;
    case 0x500:
      messageString = "Invalid frequency.";
      break;
    case 0x501:
      messageString = "Channel occupied.";
      break;
    case 0x502:
      messageString = "Transmitter already on.";
      break;
    case 0x503:
      messageString = "Antenna not connected.";
      break;
    case 0x504:
      messageString = "The module has exceeded the maximum or minimum operating "
        + "temperature anf will not allow an RF operation until it is back "
        + "in range.";
      break;
    case 0x505:
      messageString = "The module has detected high return loss and has ended "
        + "RF operations to avoid module damage.";
      break;
    case 0x506:
      messageString = "PLL not locked.";
      break;
    case 0x507:
      messageString = "Invalid antenna configuration.";
      break;
    case 0x600:
      messageString = "Not enough tags in tag ID buffer.";
      break;
    case 0x601:
      messageString = "Tag ID buffer full.";
      break;
    case 0x602:
      messageString = "Tag ID buffer repeated tag ID.";
      break;
    case 0x603:
      messageString = "Number of tags too large.";
      break;
    case 0x7f00:
      messageString = "System unknown error";
      break;                        
    case 0x7f01:
      messageString = "Software assert.";
      break;                                        
    default:
      messageString = "Unknown status code 0x" + Integer.toHexString(status);
    }
    return messageString;
  }
}
