/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.thingmagic;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;

/**
 *
 * @author qvantel
 */
public class ReaderUtil
{

    /**
     * De-duplication logic
     * @param tagvec
     * @throws ReaderException
     */
    public static void removeDuplicates(List<TagReadData> tagvec, Object uniqueByAntenna, Object uniqueByData, Object highestRSSI) throws ReaderException {
        HashMap<String, TagReadData> map = new HashMap<String, TagReadData>();
        //     List<TagReadData> tagReads = new Vector();
        String key;
        byte i = (byte) (((Boolean) uniqueByAntenna ? 0x10 : 0x00) + ((Boolean) uniqueByData ? 0x01 : 0x00));

        for (TagReadData tag : tagvec)
        {
            switch (i)
            {
                case 0x00:
                    key = tag.epcString();
                    break;
                case 0x01:
                    key = tag.epcString() + ";" + byteArrayToHexString(tag.data);
                    break;
                case 0x10:
                    key = tag.epcString() + ";" + tag.getAntenna();
                    break;
                default:
                    key = tag.epcString() + ";" + tag.getAntenna() + ";" + byteArrayToHexString(tag.data);
                    break;
            }

            if (!map.containsKey(key))
            {
                map.put(key, tag);

            }
            else //see the tag again
            {
                map.get(key).readCount = map.get(key).getReadCount() + tag.getReadCount();
                if ((Boolean) highestRSSI)
                {
                    if (tag.getRssi() > map.get(key).getRssi())
                    {
                        int tmp = map.get(key).getReadCount();
                        map.put(key, tag);
                        map.get(key).readCount = tmp;
                    }
                }
            }
        }
        tagvec.clear();
        tagvec.addAll(map.values());
    }

    /**
     * convert byte array to hex string
     * @param in Input array of bytes to be converted
     * @return A Hex string
     */
    public static String byteArrayToHexString(byte in[])
    {

        byte ch = 0x00;
        int i = 0;

        if (in == null || in.length <= 0)
        {
            return null;
        }

        String pseudo[] = {"0", "1", "2",
            "3", "4", "5", "6", "7", "8",
            "9", "A", "B", "C", "D", "E",
            "F"};

        StringBuilder out = new StringBuilder(in.length * 2);

        while (i < in.length)
        {
            ch = (byte) (in[i] & 0xF0); // Strip off high nibble
            ch = (byte) (ch >>> 4);
            // shift the bits down
            ch = (byte) (ch & 0x0F);
            // must do this is high order bit is on!
            out.append(pseudo[(int) ch]); // convert the nibble to a String Character
            ch = (byte) (in[i] & 0x0F); // Strip off low nibble
            out.append(pseudo[(int) ch]); // convert the nibble to a String Character
            i++;
        }
        return new String(out);

    }

    /**
     * Utility method to convert hex string to byte array
     * @param hexStr The input hex string
     * @return A byte array
     */
    public static byte[] hexStringToByteArray(final String hexStr)
    {

        if (hexStr.length() % 2 != 0)
        {
            throw new IllegalArgumentException("Input string must contain an even number of characters");
        }
        int len = hexStr.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2)
        {
            data[i / 2] = (byte) ((Character.digit(hexStr.charAt(i), 16) << 4)
                    + Character.digit(hexStr.charAt(i + 1), 16));
        }
        return data;
    }

    public static String arrayToString(String[] a, String separator)
    {
        StringBuilder result = new StringBuilder();
        if (a.length > 0)
        {
            result.append(a[0]);
            for (int i = 1; i < a.length; i++)
            {
                result.append(separator);
                result.append(a[i]);
            }
        }
        return result.toString();
    }

    /**
     * Utility method to convert List<String> to Bytes
     * @param strList Input list of strings.
     * @return A Byte array
     */
    public static byte[] convertListToBytes(List<String> strList)
    {

        if (strList.isEmpty())
        {
            return null;
        }

        String[] strArray = strList.toArray(new String[0]);
        StringBuilder sb = new StringBuilder(strArray[0]);

        for (int i = 0; i < strArray.length; i++)
        {
            sb.append(", ").append(strArray[i]);
        }
        return sb.toString().getBytes();
    }

    /**
     * Utility method to convert List<String> to String
     * @param strList Input list of strings
     * @param delimiter Delimeter
     * @return A String
     */
    public static String convertListToString(List<String> strList, String delimiter)
    {

        if (strList == null)
        {
            return null;
        }
        String[] strArray = strList.toArray(new String[0]);
        StringBuilder sb = new StringBuilder(strArray[0]);

        if (delimiter != null)
        {
            for (int i = 1; i < strArray.length; i++)
            {
                sb.append(delimiter);
                sb.append(strArray[i]);
            }
            return sb.toString();
        }
        for (int i = 1; i < strArray.length; i++)
        {
            sb.append(strArray[i]);
        }
        return sb.toString();
    }

    /**
     * convert short array to byte array
     * @param shortData
     * @return byteData
     */
    public static byte[] convertShortArraytoByteArray(short[] shortData)
    {
        byte[] byteData = new byte[shortData.length * 2];
        for (int i = 0; i < shortData.length; i++)
        {
            byteData[i * 2] = (byte) ((shortData[i] >> 8) & 0xff);
            byteData[i * 2 + 1] = (byte) ((shortData[i]) & 0xff);
        }

        return byteData;
    }

    public static byte[] shortToByteArray(short s)
    {
        return new byte[]{(byte) ((s>>8) & 0x00FF), (byte) (s & 0x00FF)};
    }

    public static long byteArrayToLong(byte[] data)
    {
        long value = 0;
        for (int i=0; i<8; i++) {
            value <<= 8;
            value ^= (long) data[i] & 0xff;
        }
        return value;
    }

    public static String join (String delim, String[] data)
    {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < data.length; i++)
        {
            sb.append(data[i]);
            if (i >= data.length - 1)
            {
                break;
            }
            sb.append(delim);
        }
        return sb.toString();
    }

    public static short byteArrayToShort(byte[] bytes, int offset)
    {
        ByteBuffer wrap = ByteBuffer.wrap(bytes);
        short as = wrap.getShort(offset);
        return as;
    }

    /**
     * convert byte array to short array
     * @param byteData
     * @return shortData
     */
    public static short[] convertByteArrayToShortArray(byte[] byteData)
    {
        short[] shortData = new short[byteData.length / 2];
        for (int i=0,j=0; i < byteData.length; i+=2, j++)
        {
            shortData[j] = (short)(((byteData[i]) << 8) | (byteData[i + 1] & 0xff)) ;
        }
        return shortData;
    }

   public static int byteArrayToInt(byte[] data, int offset)
   {
        int value = 0;
        for (int count = 0; count < 4; count++)
        {
            value <<= 8;
            value ^= (data[count + offset] & 0x000000FF);
        }
        return value;
    }

    public static byte[] intToByteArray(int value)
    {
        return new byte[] {
                (byte)(value >>> 24),
                (byte)(value >>> 16),
                (byte)(value >>> 8),
                (byte)value};
    }

    public static byte[] longToBytes(long v)
    {
        byte[] writeBuffer = new byte[8];

        writeBuffer[0] = (byte) (v >>> 56 & 0xff);
        writeBuffer[1] = (byte) (v >>> 48 & 0xff);
        writeBuffer[2] = (byte) (v >>> 40 & 0xff);
        writeBuffer[3] = (byte) (v >>> 32 & 0xff);
        writeBuffer[4] = (byte) (v >>> 24 & 0xff);
        writeBuffer[5] = (byte) (v >>> 16 & 0xff);
        writeBuffer[6] = (byte) (v >>> 8 & 0xff);
        writeBuffer[7] = (byte) (v >>> 0);
        return writeBuffer;
     }

    /**
     * convert Integer List to primitive int array
     * @param integers
     * @return intArray
     */
    public static int[] buildIntArray(List<Integer> integers)
    {
        int[] intArray = new int[integers.size()];
        int i = 0;
        for (Integer n : integers)
        {
            intArray[i++] = n;
        }
        return intArray;
    }

    /*
     *Create bitmask with specified position and size; e.g., makeBitMask(3, 2) returns 0x24
     */
    public static long makeBitMask(int offset, int length)
    {
        long mask;
        mask = 0;
        mask = ~mask;
        mask <<= length;
        mask = ~mask;
        mask <<= offset;
        return mask;
    }

    // Get bits within an integer;  e.g., GetBits(0x1234, 4, 4) returns 0x3
    public static long getBits(long raw, int offset, int length)
    {
        long mask = makeBitMask(offset, length);
        return (raw & mask) >> offset;
    }

    
    public static long initBits(long raw, int offset, int length, long value, String desc)
    {
        long lengthMask = makeBitMask(0, length);
        if ((value & lengthMask) != value)
        {
            throw new IllegalArgumentException("{0}value does not fit within {1} bits : {2}" + desc +length + "0x" + value);
        }
        long mask = makeBitMask(offset, length);
        raw &= ~mask;
        raw |= (value << offset) & mask;
        return raw;
    }

    /**
     * Firmware load on ThingMagic Readers using web requests.
     * @param fwStr
     * @param reader
     * @param loadOptions
     * @throws ReaderException
     * @throws IOException
     */
    public static void firmwareLoadUtil(InputStream fwStr, Reader reader,FirmwareLoadOptions loadOptions)
            throws ReaderException, IOException
    {
        URL u;
        HttpURLConnection uc;
        String encoding;
        ClientHttpRequest c;
        InputStream replyStream;
        BufferedReader replyReader;
        StringBuilder replyBuf;
        String reply;
        char buf[] = new char[1024];
        int len;
        String hostName = reader.uri.getHost();

        // These are about to stop working        
        String firmwareURL = null;

        // Assume that a system with an RQL interpreter has the standard
        // web interface and password. This isn't really an RQL operation,
        // but it will work most of the time.

        boolean eraseContents = false;
        boolean revertSettings = false;
        if (loadOptions != null)
        {
            FixedReaderFirmwareLoadOptions frOptions = (FixedReaderFirmwareLoadOptions) loadOptions;
            eraseContents = frOptions.getEraseFirmware();
            revertSettings = frOptions.getRevertDefaultSettings();
            if( eraseContents == true && revertSettings == false)
            {
                firmwareURL = "/cgi-bin/firmware.cgi?" + "wipe=" + eraseContents ;
            }
            else if(eraseContents == true && revertSettings == true)
            {
                firmwareURL = "/cgi-bin/firmware.cgi?" + "revert=" + revertSettings + "wipe=" + eraseContents ;
            }
            else if(eraseContents == false  && revertSettings == false)
            {
                firmwareURL = "/cgi-bin/firmware.cgi?";
            }
            else if(!eraseContents && revertSettings)
            {
                throw new ReaderException("Invalid firmware load arguments");
            }
            u = new URL("http", hostName, 80, firmwareURL);
        }
        else
        {
            u = new URL("http", hostName, 80, "/cgi-bin/firmware.cgi");
        }

        uc = (HttpURLConnection) u.openConnection();

        encoding = "d2ViOnJhZGlv"; // base64 encoding of "web:radio"
        uc.setRequestProperty("Authorization", "Basic " + encoding);

        c = new ClientHttpRequest(uc);

        // "firmware.tmfw" is arbitrary
        c.setParameter("uploadfile", "firmware.tmfw", fwStr);

        replyStream = c.post();

        replyReader = new BufferedReader(new InputStreamReader(replyStream));
        replyBuf = new StringBuilder();

        do
        {
            len = replyReader.read(buf, 0, 1024);
            if (len > 0)
            {
                replyBuf.append(buf);
            }
        } while (len >= 0);

        replyStream.close();
        reply = replyBuf.toString();

        if (reply.indexOf("replace the new firmware with older firmware") != -1 || (reply.indexOf("Firmware upgrade started") != -1))
        {
            // We've been asked to confirm using an older firmware
            if (loadOptions != null)
            {
                u = new URL("http", hostName, 80, firmwareURL + "&confirm=true"+"&DOWNGRADE=Continue");
            }
            else
            {
                u = new URL("http", hostName, 80, "/cgi-bin/firmware.cgi?confirm=true&DOWNGRADE=Continue");
            }

            uc = (HttpURLConnection) u.openConnection();
            uc.setRequestProperty("Authorization", "Basic " + encoding);
            uc.setRequestMethod("GET");
            uc.connect();

            replyStream = uc.getInputStream();
            replyReader = new BufferedReader(new InputStreamReader(replyStream));
            replyBuf = new StringBuilder();

            do
            {
                len = replyReader.read(buf, 0, 1024);
                if (len > 0)
                {
                    replyBuf.append(buf);
                }
            } while (len >= 0);

            replyStream.close();
            reply = replyBuf.toString();
        }

        if (reply.indexOf("Firmware update complete") != -1 || (reply.indexOf("Firmware upgrade started") != -1))
        {
            // Restart reader
            u = new URL("http", hostName, 80, "/cgi-bin/reset.cgi");
            uc = (HttpURLConnection) u.openConnection();
            uc.setRequestProperty("Authorization", "Basic " + encoding);
            c = new ClientHttpRequest(uc);
            c.setParameter("dummy", "dummy");
            c.post();

            // Wait for reader to come back up.
            try
            {
                Thread.sleep(90 * 1000);
            }
            catch (InterruptedException ie)
            {
                System.out.println("Thread interrupted while reader is coming back " + ie.getMessage());
            }
        }
        else
        {
            throw new ReaderException("Firmware update failed");
        }
    }//end of firmwareloadutil method
}
