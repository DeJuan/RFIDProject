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

import com.thingmagic.Gen2.Bank;
import com.thingmagic.Gen2.ReadData;
import java.util.Calendar;
import java.util.Iterator;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Vector;
import java.io.InputStream;
import java.io.IOException;


// Make the reader constants available here
import static com.thingmagic.EmbeddedReaderMessage.*;
import com.thingmagic.TagReadData.TagMetadataFlag;
import org.slf4j.LoggerFactory;
import org.apache.log4j.PropertyConfigurator; 
import org.slf4j.impl.Log4jLoggerAdapter;
import static com.thingmagic.TMConstants.*;

/**
 * The SerialReader class is an implementation of a Reader object that
 * communicates with a ThingMagic embedded RFID module via the
 * embedded module serial protocol. In addition to the Reader
 * interface, direct access to the commands of the embedded module
 * serial protocol is supported.
 * <p>
 *
 * Instances of the SerialReader class are created with the com.thingmagic.Reader.create 
 * method with a "eapi" URI or a generic "tmr" URI that references a local serial port.
 */
public class SerialReader extends Reader 
{
  // Connection state and (relatively) fixed values
  int txrxPorts[][];
  String serialDevice;
  VersionInfo versionInfo = null;
  Set<TagProtocol> protocolSet;
  int[] powerLimits;
  int[] ports;
  Set<Integer> portSet;  
  Region region;
  boolean useStreaming;
  int opCode;  //stores command code of message sent
  // Values affected by parameter operations
  int currentAntenna;
  int[] searchList;
  TagProtocol currentProtocol;
  int baudRate=115200;
  int[][] portParamList;
  PowerMode powerMode = PowerMode.INVALID;
  byte gpioDirections = (byte)0xFF;
  String model;
  boolean extendedEPC = false;
  boolean supportsPreamble = false;    
  boolean uniqueByAntenna = false;
  boolean uniqueByData = false;
  boolean uniqueByProtocol = true;
  int productGroupID = -1;
  int productID = -1;
  int statusFlags = 0x00;
  int statsFlags = 0x00;
  int tagOpSuccessCount = 0;
  int tagOpFailuresCount = 0;
  boolean continuousReading = false;
  
  boolean antennaStatusEnable = false;
  boolean frequencyStatusEnable = false;
  boolean temperatureStatusEnable = false;
  private boolean  isGen2AllMemoryBankEnabled = false;
  private final int DEFAULT_READ_FILTER_TIMEOUT = -1;
  boolean _enableFiltering = true;
  int _readFilterTimeout = DEFAULT_READ_FILTER_TIMEOUT;
  boolean isSecurePasswordLookup = false;
  boolean isSecureAccessEnabled = false;
  private final static String STR_FLUSH_READS = "Flush Reads";
  private final static String STR_STOP_READING = "Stop Reading";

  private boolean  isBapEnabled = false;
  private boolean  isCRCEnabled = true;
  private boolean statsSupportedFlags= false;
  private ReaderStatsFlag[] statsEnabledFlags;
  boolean  isStopNTags = false;
  int numberOfTagsToRead = 0;
  private static Log4jLoggerAdapter logger;
  private SerialTransport st; 
  static
   {
       logger = (Log4jLoggerAdapter) LoggerFactory.getLogger(SerialReader.class);
       PropertyConfigurator.configure(SerialReader.class.getClassLoader().getResource("log4j.properties"));      
   }

  private final String[] M6E_MODEL_GROUP={TMR_READER_M6E.toUpperCase(),TMR_READER_M6E_MICRO.toUpperCase(),TMR_READER_M6E_PRC.toUpperCase()};
  private final List<String> M6E_FAMILY_LIST= Arrays.asList(M6E_MODEL_GROUP);
  /**
   * This timestamp is used to verity if current timestamp is same as previous tag timestamp
   */
  private long  cacheTimeStamp = 0;  

  Map<Integer,int[]> antennaPortMap = new HashMap<Integer, int[]>();
  Map<Integer,Integer> antennaPortReverseMap = new HashMap<Integer, Integer>();
  Map<Integer,Integer> antennaPortTransmitMap = new HashMap<Integer, Integer>();
  
  int[][] _txrxMap = null;

  static final VersionNumber m4eType1 = new VersionNumber(0xff,0xff,0xff,0xff);
  static final VersionNumber m4eType2 = new VersionNumber(0x01,0x01,0x00,0x00);
  public static final int
    TMR_SR_MODEL_M5E         = 0x00,
    TMR_SR_MODEL_M5E_COMPACT = 0x01,
    TMR_SR_MODEL_M5E_I       = 0x02,
    TMR_SR_MODEL_M4E         = 0x03,
    TMR_SR_MODEL_M6E         = 0x18,
    TMR_SR_MODEL_M6E_PRC     = 0x19,
    TMR_SR_MODEL_M6E_MICRO   = 0x20;

  public static final int
    TMR_SR_MODEL_M5E_I_REV_EU  = 0x01,
    TMR_SR_MODEL_M5E_I_REV_NA  = 0x02,
    TMR_SR_MODEL_M5E_I_REV_JP  = 0x03,
    TMR_SR_MODEL_M5E_I_REV_PRC = 0x04;

  static final Map<TagProtocol,Integer> protocolToCodeMap;
  static
  {
    protocolToCodeMap = new EnumMap<TagProtocol,Integer>(TagProtocol.class);
    protocolToCodeMap.put(TagProtocol.ISO180006B, (int)PROT_ISO180006B);
    protocolToCodeMap.put(TagProtocol.GEN2, (int)PROT_GEN2);
    protocolToCodeMap.put(TagProtocol.ISO180006B_UCODE, (int)PROT_UCODE);
    protocolToCodeMap.put(TagProtocol.IPX64, (int)PROT_IPX64);
    protocolToCodeMap.put(TagProtocol.IPX256, (int)PROT_IPX256);
    protocolToCodeMap.put(TagProtocol.ATA, (int)PROT_ATA);
  }

  static final Map<Integer,TagProtocol> codeToProtocolMap;
  static
  {
    codeToProtocolMap = new HashMap<Integer,TagProtocol>();
    codeToProtocolMap.put((int)PROT_ISO180006B, TagProtocol.ISO180006B);
    codeToProtocolMap.put((int)PROT_GEN2, TagProtocol.GEN2);
    codeToProtocolMap.put((int)PROT_UCODE, TagProtocol.ISO180006B_UCODE);
    codeToProtocolMap.put((int)PROT_IPX64, TagProtocol.IPX64);
    codeToProtocolMap.put((int)PROT_IPX256, TagProtocol.IPX256);
    codeToProtocolMap.put((int)PROT_ATA, TagProtocol.ATA);
  }

  static final Map<TagMetadataFlag,Integer> tagMetadataFlagValues;
  static
  {
    tagMetadataFlagValues = new EnumMap<TagMetadataFlag,Integer>(TagMetadataFlag.class);
    tagMetadataFlagValues.put(TagMetadataFlag.READCOUNT, TAG_METADATA_READCOUNT);
    tagMetadataFlagValues.put(TagMetadataFlag.RSSI, TAG_METADATA_RSSI);
    tagMetadataFlagValues.put(TagMetadataFlag.ANTENNAID, TAG_METADATA_ANTENNAID);
    tagMetadataFlagValues.put(TagMetadataFlag.FREQUENCY, TAG_METADATA_FREQUENCY);
    tagMetadataFlagValues.put(TagMetadataFlag.TIMESTAMP, TAG_METADATA_TIMESTAMP);
    tagMetadataFlagValues.put(TagMetadataFlag.PHASE, TAG_METADATA_PHASE);
    tagMetadataFlagValues.put(TagMetadataFlag.PROTOCOL, TAG_METADATA_PROTOCOL);
    tagMetadataFlagValues.put(TagMetadataFlag.DATA, TAG_METADATA_DATA);
    tagMetadataFlagValues.put(TagMetadataFlag.GPIO_STATUS, TAG_METADATA_GPIO_STATUS);
    tagMetadataFlagValues.put(TagMetadataFlag.ALL, TAG_METADATA_ALL);
  }

  static class SimpleTransportListener implements TransportListener
  {
    public void message(boolean tx, byte[] data, int timeout)
    {
      System.out.print(tx ? "Sending: " : "Received:");
      for (int i = 0; i < data.length; i++)
      {
        if (i > 0 && (i & 15) == 0)
          System.out.printf("\n         ");
        System.out.printf(" %02x", data[i]);
      }
      System.out.printf("\n");
    }
  }
  static
  {
    simpleTransportListener = new SimpleTransportListener();
  }

    @Override
    public void startReading() {
        try 
        {
            tagOpSuccessCount = 0;
            tagOpFailuresCount = 0;
            continuousReading = true;

            ReadPlan rp= (ReadPlan)paramGet(TMR_PARAM_READ_PLAN);
            if (rp instanceof StopTriggerReadPlan)
            {
               throw new UnsupportedOperationException("Unsupported operation "
                    + rp.getClass().getName());
            }
            else if ( rp instanceof MultiReadPlan)
            {
                 MultiReadPlan mrp = (MultiReadPlan) rp;
                 for (ReadPlan r : mrp.plans)
                 {
                     if (r instanceof StopTriggerReadPlan)
                     {
                         throw new UnsupportedOperationException("Unsupported operation "
                                 + r.getClass().getName());
                     }
                 }
            }
            if (M6E_FAMILY_LIST.contains(model.toUpperCase())&&
                ((Integer) paramGet(TMR_PARAM_READ_ASYNCOFFTIME) == 0) &&
                (rp instanceof SimpleReadPlan ||(rp instanceof MultiReadPlan && compareAntennas((MultiReadPlan)rp))))
            {
                
                useStreaming = true;
                isTrueAsyncStopped = false;
                startReadingGivenRead(true);
            }
            else
            {                
                startReadingGivenRead(false);
            }
        }
        catch (ReaderException ex)
        {
            logger.error(ex.getMessage());
        }        
    }

    @Override
    public boolean stopReading()  {
        try
        {
            continuousReading = false;
            stopReadingGivenRead();
            if (true == useStreaming)
            {                
                cmdStopContinuousRead(this);
                cmdSetReaderConfiguration(Configuration.ENABLE_FILTERING, this._enableFiltering);
            }                        
        }
        catch(ReaderException rce)
        {
            notifyExceptionListeners(rce);
            return false;
        }
        catch (Exception ex)
        {
            logger.error(ex.getMessage());
            return false;
        }
        finally
        {
            try
            {
                if (M6E_FAMILY_LIST.contains(model.toUpperCase())&& ((Integer) paramGet(TMR_PARAM_READ_ASYNCOFFTIME) == 0))
                {
                    useStreaming = false;
                }
            }
            catch (ReaderException re)
            {
                notifyExceptionListeners(re);
            }
        }
        return true;
    }
    /**
     * Compare antenna list in readplans list, return true if antenna
     * list are consistent across the entire set of read plans.
    **/
    private boolean compareAntennas(MultiReadPlan multiReadPlan){
        boolean status= true;
        SimpleReadPlan plan1;
        SimpleReadPlan plan2;
        boolean antennaMatched= false;
        int matchingPlanCount = 0;
        for (int i = 0; i < multiReadPlan.plans.length-1; i++) {
            plan1=(SimpleReadPlan) multiReadPlan.plans[i];
            plan2=(SimpleReadPlan) multiReadPlan.plans[i+1];
            if(0!=plan1.antennas.length && 0!=plan2.antennas.length){
                if(plan1.antennas.length == plan2.antennas.length){
                    for (int j = 0; j < plan1.antennas.length ; j++) {
                        int p1Antenna = plan1.antennas[j];
                        antennaMatched = false;
                        for (int k = 0; k < plan2.antennas.length; k++) {
                            int p2Antenna = plan2.antennas[j];
                            if(p1Antenna ==p2Antenna){
                                antennaMatched = true;
                                break;
                            }
                        }//End of plan2 antenna
                        if(!antennaMatched){
                           break;
                        }
                    }//End of plan1 antenna
                    if(antennaMatched){
                        matchingPlanCount++; //antenna value of 2 pans are matched
                    }else{
                        status = false;
                        break;
                    }
                }else{
                    status = false;
                    break;
                }
            }else if (0 == plan1.antennas.length && 0== plan2.antennas.length){
                matchingPlanCount++; //antenna value of 2 pans are matched
            }else{
                status = false;
                break;
            }
            if(matchingPlanCount == multiReadPlan.plans.length-1){
                status = true;
                break;
            }
        }
        return status;
    }
    private void cmdStopContinuousRead(Reader reader) throws ReaderException
    {
        cmdStopContinuousRead(reader, STR_STOP_READING);
    }
    
    private void cmdStopContinuousRead(Reader reader, String value) throws ReaderException
    {
        Message m = new Message();
        
        m.setu8(MSG_OPCODE_MULTI_PROTOCOL_TAG_OP);
        m.setu16(0x00); //  currently ignored
        m.setu8(0x02); // option 2 - stop continuous reading

        isTrueAsyncStopped = true;                
        try
        {
            sendMessage(commandTimeout, m);
        } 
        catch (ReaderException re)
        {
            notifyExceptionListeners(re);
        }
        do
        {
            opCode = MSG_OPCODE_READ_TAG_ID_MULTIPLE;
            if(value.equals(STR_STOP_READING))
            {
            receiveResponseStream(commandTimeout, m);
    }
            else
            {
                receiveBufferedReads(commandTimeout, m);
            }
        }while(m.data[2]!=0x2F);
    }

    @Override
    public void firmwareLoad(InputStream firmware, FirmwareLoadOptions loadOptions) throws ReaderException, IOException {
        if(loadOptions!=null)
        {
            throw new ReaderException("Firmware Load Options not yet implemented for Serial Readers");
        }
        else
        {
            firmwareLoad(firmware);
        }
    }

    /**
     * retrieve all tag reads
     * @param baseTime
     * @param tagCount
     * @param tagProtocol
     * @return
     */
    private List<TagReadData> getAllTagReads(long baseTime, int tagCount, TagProtocol tagProtocol) throws ReaderException
    {
        int count = 0;
        List<TagReadData> tagReads = new ArrayList<TagReadData>();
        timeStart = System.currentTimeMillis(); 
        while (count < tagCount)
        {
            TagReadData tr[];
            tr = cmdGetTagBuffer(allMeta, false, tagProtocol);
            for (TagReadData t : tr)
            {
                t.readBase = baseTime;
                t.antenna = antennaPortReverseMap.get(t.antenna);
                t.reader = this;
                if (null != t && t.tag !=null)
                {
                tagReads.add(t);
                }               
                count++;
            }//end of for
        }//end of while

        return tagReads;

    }

    private void receiveBufferedReads(int readTimeout, Message m) throws ReaderException
    {
        try
        {
            m.readIndex = 0;
            receiveMessage(readTimeout, m);
        }
        catch (ReaderException re)
        {
            if (re instanceof ReaderCodeException && ((ReaderCodeException)re).getCode() == FAULT_NO_TAGS_FOUND)
            {
                // just ignore no tags found response
                m.readIndex += 5;               
            }
            else if(re instanceof ReaderCodeException && ((ReaderCodeException) re).getCode() == FAULT_TAG_ID_BUFFER_FULL)
            {
                if(m.data[2] != 0x2F)
                {
                    // get 2f response and then restart thread
                    // opCode = 0x2f;
                    receiveMessage(readTimeout, m);
                    isTrueAsyncStopped = false;
                    throw new ReaderCodeException(FAULT_TAG_ID_BUFFER_FULL);
                }
            }
            else if(re instanceof ReaderFatalException){
                if(re.getMessage().contains("Reader assert") && continuousReading)
                {
                    isTrueAsyncStopped = true;
                    readerThread.interrupt();
                }
                notifyExceptionListeners(re);
            }
            else if(re instanceof ReaderCommException || (re instanceof ReaderCodeException &&
                    ((((ReaderCodeException) re).getCode() == FAULT_SYSTEM_UNKNOWN_ERROR) ||
                    ((ReaderCodeException) re).getCode() == FAULT_TM_ASSERT_FAILED)))
            {
                if(continuousReading)
                {
                    isTrueAsyncStopped = true;
                    readerThread.interrupt();
                    throw new ReaderCommException(re.getMessage());
                }
            }
            else if (re instanceof ReaderCodeException && ((ReaderCodeException)re).getCode() == FAULT_TAG_ID_BUFFER_AUTH_REQUEST)
            {
                /* Tag password needed to complete tagop.
                 * Parse TagReadData and pass to password-generating callback,
                 * which will return the appropriate authentication. */

                TagReadData read = new TagReadData();
                read.reader = this;
                m.readIndex = 8;

                Set<TagMetadataFlag> metadataFlags = tagMetadataSet(m.getu16());
                m.readIndex++;/* Skip tag count (always = 1) */
                metadataFromMessage(read, m, metadataFlags);
                int epcLen = m.getu16() / 8;
                read.tag = parseTag(m, epcLen, read.readProtocol);
                Gen2.Password accessPassword = null;
                try
                {
                    notifyAuthenticationListeners(read, this);
                    accessPassword = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD));
                } 
                catch (Exception ex)
                {
                    ex.printStackTrace();
                    logger.error(ex.getMessage());
                }
                cmdAuthReqResponse(accessPassword);
                opCode = m.data[2];
            }
            else if(re instanceof ReaderCodeException) // more generic catch at the bottom of the exception handling
            {
                //any other reader code exception like 0x504, 0x505,
                //throw new ReaderCodeException(((ReaderCodeException)re).code);
                notifyExceptionListeners(re);
            }
            else
            {
                notifyExceptionListeners(re);
        }
        }
    }
    /**
     * Parsing tag data response     
     * @param readTimeout
     * @param m     
     */
    private void receiveResponseStream(int readTimeout, Message m) throws ReaderException
    {
        long baseTime = System.currentTimeMillis();
        receiveBufferedReads(readTimeout, m);
        // this 2f response says true continuous reading is stopped, so return from the response streaming
        if((m.data[2] == 0x2F && isTrueAsyncStopped)||isSecureAccessEnabled)
        {
            return;
        }
        if (m.data[1] == 0 && m.data[2] == 0x22)
        {
            /**
             * In case of streaming and ISO protocol after every search cycle
             * module sends the response for embedded operation status as
             * FF 00 22 00 00. In this case avoid parsing.
             */
            return;
        }
        byte flags = (byte) m.getu8();
        int responseTypeIndex = (((byte) flags & (byte) 0x10) == (byte) 0x10) ? 10 : 8;
        byte responseByte = m.data[responseTypeIndex];

        // skipping the search flags
        m.readIndex += 2;
        BoolResponse br = new BoolResponse();
        parseResponseByte(responseByte, br);
        
        if(br.parseResponse)
        {
            if(br.statusResponse)
            {
                if((statusListeners!=null && !statusListeners.isEmpty()) &&
                        (statsListeners == null || statsListeners.isEmpty()))
                {
                    List<StatusReport> sReport = new ArrayList<StatusReport>();
                    m.readIndex++; // skip response byte
                    int contentFlags = m.getu16(); // content flags
                    //if noise floor is in response, then skip one byte
                    if(0 != (contentFlags & ReaderStatusFlag.FREQUENCY.value))
                    {
                        FrequencyStatusReport fsr = new FrequencyStatusReport();
                        fsr.frequency = m.getu24();
                        sReport.add(fsr);
                    }
                    if(0 != (contentFlags & ReaderStatusFlag.TEMPERATURE.value))
                    {
                        TemperatureStatusReport tsr = new TemperatureStatusReport();
                        tsr.temperature = m.getu8();
                        sReport.add(tsr);
                    }
                    if(0 != (contentFlags & ReaderStatusFlag.CURRENT_ANTENNAS.value))
                    {
                        AntennaStatusReport asr = new AntennaStatusReport();
                        asr.antenna = antennaPortReverseMap.get((((m.getu8() & 0xF) << 4) | (m.getu8() & 0xF)));
                        sReport.add(asr);
                    }
                    br.statusResponse = false;
                    notifyStatusListeners(sReport.toArray(new StatusReport[0]));
                }
                else if((statusListeners == null || statusListeners.isEmpty()) &&
                                (statsListeners != null && !statsListeners.isEmpty()))
                {
                    /* Get status content flags */
                    if ((0x80) > statsFlags)
                    {
                      m.readIndex += 2;
                    }
                    else
                    {
                      m.readIndex += 3;
                    }
                    notifyStatsListeners(fillReaderStats(m));
                }
                else
                {
                    notifyExceptionListeners(new ReaderException("Getting both the reader stats and status is not supported"));
                }
            }
            else  // no status response in the message
            {
                TagReadData t = new TagReadData();
                t.isAsyncRead = true;
                Set<TagMetadataFlag> metadataFlags = tagMetadataSet(m.getu16());
                m.readIndex += 1; // skip response type
                if(!metadataFlags.isEmpty())
                {
                    metadataFromMessage(t, m, metadataFlags);
                    if (baseTime - cacheTimeStamp < 1)
                    {
                        /**
                         * Adding counter milliseconds to current timestamp if both current and previous tags have same timestamp
                         */
                        baseTime += (cacheTimeStamp-baseTime) + 1;
//                        System.out.println("timestamp diff found " + (baseTime - cacheTimeStamp) + " baseTime " + baseTime);
                    }
                    t.antenna = antennaPortReverseMap.get(t.antenna);
                    int epcLen = m.getu16() / 8;
                    t.tag = parseTag(m, epcLen, t.readProtocol);
                    t.readBase = baseTime;
                    // caching the last seen tag timestamp
                    cacheTimeStamp = baseTime;
                    t.reader = this;
                    if(null != t.tag)
                    {
                    notifyReadListeners(t);
                }
            }
        }
        }
        else // if parse response is false
        {
            SimpleReadPlan srp = null;
            MultiReadPlan mrp = null;
            TagOp tagop = null;
            try
            {
                ReadPlan rp = (ReadPlan)paramGet(TMR_PARAM_READ_PLAN);
                if(rp instanceof SimpleReadPlan)
                {
                    srp = (SimpleReadPlan) rp;
                    tagop = srp.Op;
                }
                else
                {
                    mrp = (MultiReadPlan) rp;
                    tagop = ((SimpleReadPlan)mrp.plans[0]).Op;
                }
            }
            catch (ReaderException ex)
            {
                logger.error(ex.getMessage());
            }
            if (null != tagop)
            {
                m.readIndex += 7;
                tagOpSuccessCount += m.getu16();
                tagOpFailuresCount += m.getu16();
            }
        }// end of else
    }
    
    private class BoolResponse
    {
        boolean parseResponse = true;
        boolean statusResponse = false;
    }

    /**
     * Internal method to parse response byte in the 22h command response
     * @param responseByte
     * @return
     */
    private void parseResponseByte(byte responseByte, BoolResponse br)
    {        
        switch (responseByte)
        {
            case 0x02:
                //mid stream status response
                br.statusResponse = true;
                break;
            case 0x01:
                // mid stream tag buffer response
                break;
            case 0x00:
                // final stream response
                br.parseResponse = false;
        }        
    }

    /**
     * msgAddHiggs2PartialLoadImage
     * @param m
     * @param readTimeout
     * @param accessPassword
     * @param killPassword
     * @param epc
     */
    private void msgAddHiggs2PartialLoadImage(Message m, int readTimeout, int accessPassword, int killPassword, byte[] epc)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(readTimeout);
        m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS);
        m.setu8(ALIEN_HIGGS_CHIP_SUBCOMMAND_PARTIAL_LOAD_IMAGE);
        m.setu32(killPassword);
        m.setu32(accessPassword);
        m.setbytes(epc,0,epc.length);
    }

    /**
     * msgAddHiggs2FullLoadImage
     * @param m
     * @param readTimeout
     * @param accessPassword
     * @param killPassword
     * @param lockBits
     * @param pcWord
     * @param epc
     */
    private void msgAddHiggs2FullLoadImage(Message m, int timeout, int accessPassword,
            int killPassword, int lockBits, int pcWord, byte[] epc)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS);
        m.setu8(ALIEN_HIGGS_CHIP_SUBCOMMAND_FULL_LOAD_IMAGE);
        m.setu32(killPassword);
        m.setu32(accessPassword);
        m.setu16(lockBits);
        m.setu16(pcWord);
        m.setbytes(epc,0,epc.length);
    }

    /**
     * msgAddHiggs3BlockReadLock
     * @param m
     * @param timeout
     * @param accessPassword
     * @param lockBits
     * @param target
     */
    private void msgAddHiggs3BlockReadLock(Message m, int timeout, int accessPassword, int lockBits, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS3);

        optByte = m.writeIndex++;
        m.setu16(ALIEN_HIGGS3_CHIP_SUBCOMMAND_BLOCK_READ_LOCK);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        
        m.setu32(accessPassword);
        m.setu8(lockBits);
    }

    /**
     * msgAddHiggs3FastLoadImage
     * @param m
     * @param timeout
     * @param currentAccessPassword
     * @param accessPassword
     * @param killPassword
     * @param pcWord
     * @param epc
     * @param target
     */
    private void msgAddHiggs3FastLoadImage(Message m, int timeout, int currentAccessPassword, int accessPassword, 
            int killPassword, int pcWord, byte[] epc, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS3);

        optByte = m.writeIndex++;
        m.setu16(ALIEN_HIGGS3_CHIP_SUBCOMMAND_FAST_LOAD_IMAGE);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        
        m.setu32(currentAccessPassword);
        m.setu32(killPassword);
        m.setu32(accessPassword);
        m.setu16(pcWord);
        m.setbytes(epc,0,epc.length);
    }

    /**
     * msgAddHiggs3LoadImage
     * @param m
     * @param timeout
     * @param currentAccessPassword
     * @param accessPassword
     * @param killPassword
     * @param pcWord
     * @param EPCAndUserData
     * @param target
     */
    private void msgAddHiggs3LoadImage(Message m, int timeout, int currentAccessPassword, int accessPassword,
            int killPassword, int pcWord, byte[] EPCAndUserData, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS3);

        optByte = m.writeIndex++;
        m.setu16(ALIEN_HIGGS3_CHIP_SUBCOMMAND_LOAD_IMAGE);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        
        m.setu32(currentAccessPassword);
        m.setu32(killPassword);
        m.setu32(accessPassword);
        m.setu16(pcWord);
        m.setbytes(EPCAndUserData);
    }

    /**
     * IDS GetLogState command
     * @param timeout
     * @param tagop
     * @param filter
     * @return LogState data
     * @throws ReaderException
     */
    private Gen2.IDS.SL900A.LogState cmdIdsSL900aGetLogState(int timeout, Gen2.IDS.SL900A.GetLogState tagop, TagFilter filter) throws ReaderException
    {
        Message m = new Message();
        msgAddIdsSL900aGetLogState(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
        m.readIndex = 9;
        byte[] data = new byte[9];
        m.getbytes(data, 9);
        return new Gen2.IDS.SL900A.LogState(data);
    }

    /**
     * IDS GetSensorValue command
     * @param timeout
     * @param tagop
     * @param filter
     * @return SensorReading data
     * @throws ReaderException
     */
    private Gen2.IDS.SL900A.SensorReading cmdIdsSL900aGetSensorValue(int timeout, Gen2.IDS.SL900A.GetSensorValue tagop, TagFilter filter) throws ReaderException{
        Message m = new Message();
        msgAddIdsSL900aGetSensorValue(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
        m.readIndex = 9;
        byte[] data = new byte[2];
        m.getbytes(data, 2);
        return new Gen2.IDS.SL900A.SensorReading(data);
    }

    /**
     * msgAddIdsSL900aEndLog
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aEndLog(Message msg, int timeout, Gen2.IDS.SL900A.EndLog tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
    }

    /**
     * IDS EndLog command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
   private void cmdIdsSL900aEndLog(int timeout, Gen2.IDS.SL900A.EndLog tagop, TagFilter filter) throws ReaderException
    {
        Message m = new Message();
        msgAddIdsSL900aEndLog(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
    }

    /**
     * msgAddIdsSL900aCommonHeader
     * @param m
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aCommonHeader(Message m, int timeout, Gen2.IDS.SL900A tagop, TagFilter target)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(tagop.chipType);
        int optByte = m.writeIndex++;
        m.setu8(0x00);
        m.setu8(tagop.commandCode);
        filterBytes(TagProtocol.GEN2, m, optByte, target, tagop.accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        int password = (Enum.valueOf(Gen2.IDS.SL900A.Level.class, tagop.passwordLevel.toString())).rep;
        m.setu8(password);
        m.setbytes(ReaderUtil.intToByteArray(tagop.password));
    }

    /**
     * IDS StartLog command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
    private void cmdIdsSL900aStartLog(int timeout, Gen2.IDS.SL900A.StartLog tagop, TagFilter filter) throws ReaderException
    {
            Message m = new Message();
            msgAddIdsSL900aStartLog(m, timeout, tagop, filter);
            sendTimeout(timeout, m);
    }

    // Convert DateTime to SL900A time
    public static int toSL900aTime(Calendar calendar) throws ReaderException
    {
        int t32 = 0;
        Calendar rightNow = calendar.getInstance();
        int year = Calendar.YEAR;
        if(0 <= (rightNow.get(year) - 2010))
        {
            t32 |= (rightNow.get(year) - 2010) << 26;
        }
        else
        {
            throw new ReaderException("Year must be >= 2010: " + rightNow.get(year));
        }
        t32 |= rightNow.get(Calendar.MONTH) << 22;
        t32 |= rightNow.get(Calendar.DAY_OF_MONTH) << 17;
        t32 |= rightNow.get(Calendar.HOUR_OF_DAY) << 12;
        t32 |= rightNow.get(Calendar.MINUTE) << 6;
        t32 |= rightNow.get(Calendar.SECOND);
        return t32;
    }

    // Convert SL900A time to DateTime
    public static Calendar fromSL900aTime(int t32)
    {
        Calendar calendar = Calendar.getInstance();
        calendar.set(2010 + (int) ((t32 >> 26) & 0x3F), //year
                    ((t32 >> 22) & 0xF), //Month
                    ((t32 >> 17) & 0x1F), //Day
                    ((t32 >> 12) & 0x1F),//Hour
                    ((t32 >> 6) & 0x3F), //Minute
                    ((t32 >> 0) & 0x3F));//Sec
        return calendar;
    }
    /**
     * msgAddIdsSL900aStartLog
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aStartLog(Message msg, int timeout, Gen2.IDS.SL900A.StartLog tagop, TagFilter target) throws ReaderException
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        msg.setbytes(ReaderUtil.intToByteArray(toSL900aTime(tagop.startTime)));
    }

    /**
     * msgAddIdsSL900aGetLogState
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aGetLogState(Message msg, int timeout, Gen2.IDS.SL900A.GetLogState tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
    }

    /**
     * msgAddIdsSL900aInitialize
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aInitialize(Message msg, int timeout, Gen2.IDS.SL900A.Initialize tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        short aas = tagop.delayTime.raw;
        msg.setbytes(ReaderUtil.shortToByteArray(aas));
        short app = tagop.appData.raw;
        msg.setbytes(ReaderUtil.shortToByteArray(app));
    }

    /**
     * IDS Initialize command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
    private void cmdIdsSL900aInitialize(int timeout, Gen2.IDS.SL900A.Initialize tagop, TagFilter filter) throws ReaderException
    {
            Message m = new Message();
            msgAddIdsSL900aInitialize(m, timeout, tagop, filter);
            sendTimeout(timeout, m);
    }

    /**
     * msgAddIdsSL900aGetSensorValue
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aGetSensorValue(Message msg, int timeout, Gen2.IDS.SL900A.GetSensorValue tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        int data = (Enum.valueOf(Gen2.IDS.SL900A.Sensor.class, tagop.sensorType.toString())).rep;
        msg.setu8(data);
    }

    /**
     * msgAddIdsSL900aSetLogMode
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aSetLogMode(Message msg, int timeout, Gen2.IDS.SL900A.SetLogMode tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        int form = (Enum.valueOf(Gen2.IDS.SL900A.LoggingForm.class, tagop.form.toString())).rep;
        int storage = (Enum.valueOf(Gen2.IDS.SL900A.StorageRule.class, tagop.storage.toString())).rep;
        int logmode = 0;
        logmode |= form << 21;
            logmode |= storage << 20;
            logmode |= (tagop.ext1Enable ? 1 : 0) << 19;
            logmode |= (tagop.ext2Enable ? 1 : 0) << 18;
            logmode |= (tagop.tempEnable ? 1 : 0) << 17;
            logmode |= (tagop.battEnable ? 1 : 0) << 16;
            logmode |=  tagop._logInterval << 1;
            msg.setu8((logmode >> 16) & 0xFF);
            msg.setu8((logmode >> 8) & 0xFF);
            msg.setu8((logmode >> 0) & 0xFF);
    }

    /**
     * IDS SetLogMode command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
    private void cmdIdsSL900aSetLogMode(int timeout, Gen2.IDS.SL900A.SetLogMode tagop, TagFilter filter) throws ReaderException
    {
            Message m = new Message();
            msgAddIdsSL900aSetLogMode(m, timeout, tagop, filter);
            sendTimeout(timeout, m);
    }

    /**
     * IDS AccessFifo command
     * @param timeout
     * @param tagop
     * @param filter
     * @return Object
     * @throws ReaderException
     */
    private Object cmdIdsSL900aAccessFifo(int timeout, Gen2.IDS.SL900A.AccessFifo tagop, TagFilter filter) throws ReaderException
    {
        Message m = new Message();
        msgAddIdsSL900aAccessFifo(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
        m.readIndex += 4;
        if (tagop instanceof  Gen2.IDS.SL900A.AccessFifoRead)
        {
            int length = ((Gen2.IDS.SL900A.AccessFifoRead) tagop).length;
                byte[] rv = new byte[length];
                m.getbytes(rv, length);
                return rv;
        }
        else if (tagop instanceof  Gen2.IDS.SL900A.AccessFifoStatus)
        {
                byte[] rv = new byte[1];
                m.getbytes(rv, 1);
                return new Gen2.IDS.SL900A.FifoStatus(rv);
        }
        else if (tagop instanceof  Gen2.IDS.SL900A.AccessFifoWrite)
        {
            return null;
        }
        else
        {
            throw new IllegalArgumentException("Unsupported AccessFifo tagop: " + tagop);
        }
    }

    /**
     * msgAddIdsSL900aAccessFifo
     * @param m
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aAccessFifo(Message m, int timeout, Gen2.IDS.SL900A.AccessFifo tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(m, timeout, tagop, target);
        byte length = 0;
        byte[] payload = null;
        if (tagop instanceof Gen2.IDS.SL900A.AccessFifoRead)
        {
            Gen2.IDS.SL900A.AccessFifoRead op = (Gen2.IDS.SL900A.AccessFifoRead) tagop;
            length = op.length;
        }
        else if (tagop instanceof Gen2.IDS.SL900A.AccessFifoWrite)
        {
            Gen2.IDS.SL900A.AccessFifoWrite op = (Gen2.IDS.SL900A.AccessFifoWrite) tagop;
            length = (byte) op.payload.length;
            payload = op.payload;
        }
        int optByte = m.writeIndex++;
        int value = (Enum.valueOf(Gen2.IDS.SL900A.AccessFifo.SubcommandCode.class, tagop.subcommand.toString())).rep;
        m.data[optByte] = (byte) ((byte) value | length);
        if (null != payload)
        {
            m.setbytes(payload);
        }
    }

     /**
     * msgAddIdsSL900aSetSfeParameters
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aSetSfeParameters(Message msg, int timeout, Gen2.IDS.SL900A.SetSfeParameters tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        msg.setbytes(ReaderUtil.shortToByteArray(tagop.sfeParameter.raw));
    }

    /**
     * msgAddIdsSL900aSetCalibrationData
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aSetCalibrationData(Message msg, int timeout, Gen2.IDS.SL900A.SetCalibrationData tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        byte[] calBytes = new byte[8];
        calBytes = ReaderUtil.longToBytes(tagop.cal.raw);
        msg.setbytes(calBytes, 1, 7);
    }

    /**
     * msgAddIdsSL900aGetCalibrationData
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
    private void msgAddIdsSL900aGetCalibrationData(Message msg, int timeout, Gen2.IDS.SL900A.GetCalibrationData tagop, TagFilter target)
    {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
    }

    /**
     * IDS GetCalibrationData command
     * @param timeout
     * @param tagop
     * @param filter
     * @return CalSfe data
     * @throws ReaderException
     */
    private Gen2.IDS.SL900A.CalSfe cmdIdsSL900aGetCalibrationData(int timeout, Gen2.IDS.SL900A.GetCalibrationData tagop, TagFilter filter) throws ReaderException
    {
        Message m = new Message();
        msgAddIdsSL900aGetCalibrationData(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
        m.readIndex += 4;
        int length = 9;
        byte[] rv = new byte[length];
        m.getbytes(rv, length);
        return new Gen2.IDS.SL900A.CalSfe(rv, 0);
    }

    /**
     * IDS SetCalibrationData command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
    private void cmdIdsSL900aSetCalibrationData(int timeout, Gen2.IDS.SL900A.SetCalibrationData tagop, TagFilter filter) throws ReaderException
    {
        Message m = new Message();
        msgAddIdsSL900aSetCalibrationData(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
    }

    /**
     * IDS SetSfeParameter command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
    private void cmdIdsSL900aSetSfeParameters(int timeout, Gen2.IDS.SL900A.SetSfeParameters tagop, TagFilter filter) throws ReaderException
    {
        Message m = new Message();
        msgAddIdsSL900aSetSfeParameters(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
    }

    /**
     * msgAddIdsSL900aGetMeasurementSetup
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
     private void msgAddIdsSL900aGetMeasurementSetup(Message msg, int timeout, Gen2.IDS.SL900A.GetMeasurementSetup tagop, TagFilter target)
     {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
     }

    /**
     * IDS GetBatteryLevel command
     * @param timeout
     * @param tagop
     * @param filter
     * @return BatteryLevelReading Data
     * @throws ReaderException
     */
     private Gen2.IDS.SL900A.BatteryLevelReading cmdIdsSL900aGetBatteryLevel(int timeout, Gen2.IDS.SL900A.GetBatteryLevel tagop, TagFilter filter) throws ReaderException{
        Message m = new Message();
        msgAddIdsSL900aGetBatteryLevel(m, timeout, tagop, filter);
        sendTimeout(timeout, m);
        m.readIndex = 9;
        byte[] data = new byte[2];
        m.getbytes(data, 2);
        return new Gen2.IDS.SL900A.BatteryLevelReading(data);
     }

    /**
     * msgAddIdsSL900aGetBatteryLevel
     * @param msg
     * @param timeout
     * @param tagop
     * @param target
     */
     private void msgAddIdsSL900aGetBatteryLevel(Message msg, int timeout, Gen2.IDS.SL900A.GetBatteryLevel tagop, TagFilter target)
     {
        msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
        int data = (Enum.valueOf(Gen2.IDS.SL900A.BatteryType.class, tagop.type.toString())).rep;
        msg.setu8(data);
     }

     /**
     * IDS GetMeasurementSetup command
     * @param timeout
     * @param tagop
     * @param filter
     * @return MeasurementSetupData Data
     * @throws ReaderException
     */
     private Gen2.IDS.SL900A.MeasurementSetupData cmdIdsSL900aGetMeasurementSetup(int timeout, Gen2.IDS.SL900A.GetMeasurementSetup tagop, TagFilter filter) throws ReaderException
     {
          Message msg = new Message();
          msgAddIdsSL900aGetMeasurementSetup(msg, timeout, tagop, filter);
          sendTimeout(timeout, msg);
          msg.readIndex += 4;
          int length = 16;
          byte[] measurementSetupData = new byte[length];
          msg.getbytes(measurementSetupData, length);
          return new Gen2.IDS.SL900A.MeasurementSetupData(measurementSetupData, 0);
     }

      /**
      * msgAddIdsSL900aSetLogLimit
      * @param msg
      * @param timeout
      * @param tagop
      * @param target
      */
     private void msgAddIdsSL900aSetLogLimit(Message msg, int timeout, Gen2.IDS.SL900A.SetLogLimit tagop, TagFilter target)
     {
         msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
         byte[] value = ReaderUtil.longToBytes(tagop.logLimit.raw);
         msg.setbytes(value, 3, 5);
     }

     /**
     * IDS SetLogLimit command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
     private void cmdIdsSL900aSetLogLimit(int timeout, Gen2.IDS.SL900A.SetLogLimit tagop, TagFilter filter) throws ReaderException
     {
         Message m = new Message();
         msgAddIdsSL900aSetLogLimit(m, timeout, tagop, filter);
         sendTimeout(timeout, m);
     }

     /**
      * msgAddIdsSL900ASetPassword
      * @param msg
      * @param timeout
      * @param tagop
      * @param target
      */
     private void msgAddIdsSL900ASetPassword(Message msg, int timeout, Gen2.IDS.SL900A.SetPassword tagop, TagFilter target)
     {
         msgAddIdsSL900aCommonHeader(msg, timeout, tagop, target);
         msg.setu8(tagop.newPasswordLevel);
         msg.setbytes(ReaderUtil.intToByteArray(tagop.newPassword));
     }

     /**
     * IDS SetPassword command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
     private void cmdIdsSL900ASetPassword(int timeout, Gen2.IDS.SL900A.SetPassword tagop, TagFilter filter) throws ReaderException
     {
        Message msg = new Message();
        msgAddIdsSL900ASetPassword(msg, timeout, tagop, filter);
        sendTimeout(timeout, msg);
     }

     /**
     * IDS Set Shelf Life command
     * @param timeout
     * @param tagop
     * @param filter
     * @throws ReaderException
     */
     private void cmdIdsSL900aSetShelfLife(int timeout, Gen2.IDS.SL900A.SetShelfLife tagop, TagFilter filter) throws ReaderException
     {
         Message msg = new Message();
         msgAddIdsSL900aSetShelfLife(msg, timeout, tagop, filter);
         sendTimeout(timeout, msg);
     }

     /**
      * msgAddIdsSL900aSetShelfLife
      * @param msg
      * @param timeout
      * @param tagop
      * @param target
      */
     private void msgAddIdsSL900aSetShelfLife(Message msg, int timeout, Gen2.IDS.SL900A.SetShelfLife tagop, TagFilter filter)
     {
         msgAddIdsSL900aCommonHeader(msg, timeout, tagop, filter);
         byte[] s1Block0 = ReaderUtil.intToByteArray(tagop.slBlock0.raw);
         msg.setbytes(s1Block0, 0, 4);
         byte[] s1Block1 = ReaderUtil.intToByteArray(tagop.slBlock1.raw);
         msg.setbytes(s1Block1, 0, 4);
     }

     /**
     * msgAddNxpResetReadProtect
     * @param m
     * @param timeout
     * @param accessPassword
     * @param chipType
     * @param target
     */
    private void msgAddNxpResetReadProtect(Message m, int timeout, int accessPassword, int chipType, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chipType);
        optByte = m.writeIndex++;
        m.setu16(NXP_CHIP_SUBCOMMAND_RESET_QUIET);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu32(accessPassword);
    }

    /**
     * msgAddNxpSetReadProtect
     * @param m
     * @param timeout
     * @param accessPassword
     * @param chipType
     * @param target
     */
    private void msgAddNxpSetReadProtect(Message m, int timeout, int accessPassword, int chipType, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chipType);

        optByte = m.writeIndex++;
        m.setu16(NXP_CHIP_SUBCOMMAND_SET_QUIET);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option

        m.setu32(accessPassword);
    }

    /**
     * msgAddNxpCalibrate
     * @param m
     * @param timeout
     * @param accessPassword
     * @param chipType
     * @param target
     */
    private void msgAddNxpCalibrate(Message m, int timeout, int accessPassword, int chipType, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chipType);
        optByte = m.writeIndex++;
        m.setu16(NXP_CHIP_SUBCOMMAND_CALIBRATE);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu32(accessPassword);
    }

    /**
     * msgAddNxpChangeEas
     * @param m
     * @param timeout
     * @param accessPassword
     * @param reset
     * @param chipType
     * @param target
     */
    private void msgAddNxpChangeEas(Message m, int timeout, int accessPassword, boolean reset, int chipType, TagFilter target)
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chipType);

        optByte = m.writeIndex++;
        m.setu16(NXP_CHIP_SUBCOMMAND_CHANGE_EAS);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, false);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option

        m.setu32(accessPassword);
        m.setu8(reset ? 2 : 1);
    }

    /**
     * msgAddNxpChangeConfig
     * @param m
     * @param timeout
     * @param accessPassword
     * @param configWord
     * @param chipType
     * @param target
     * @throws FeatureNotSupportedException
     */
    private void msgAddNxpChangeConfig(Message m, int timeout, int accessPassword, int configWord, int chipType, TagFilter target) throws FeatureNotSupportedException
    {
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chipType); // Only G2iL as of now
        optByte = m.writeIndex++;
        m.setu16(NXP_CHIP_SUBCOMMAND_CONFIG_CHANGE);
        //m.setu32(accessPassword);               
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);        
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8 (0x00); // RFU
        m.setu16 (configWord);
    }

   
    /**
     * Impinj Monza4 QTReadWrite Command
     * @param m
     * @param timeout - the timeout of the operation, in milliseconds. Valid range is 0-65535.
     * @param accessPassword 
     * @param controlByte - comprises of qtReadWrite and persistence
     * @param payload - comprises of qtSR and qtMEM
     * @param target - target filter
     */
    private void msgAddMonza4QTReadWrite(Message m, int timeout, int accessPassword, int controlByte, int payload, TagFilter target)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(TAG_CHIP_TYPE_MONZA);
        // option byte comprises of command option. formed based on different inputs like filter type, password included or not etc
        int optByte = m.writeIndex++;
        //m.setu8(0x40);
        m.setu8(0x00);
        m.setu8(0x00);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option

        //control byte        
        m.setu8(controlByte);
        
        //payload        
        m.setu16(payload);
    }
    
    private void assignStatusFlags()
    {        
        if(frequencyStatusEnable)
        {
            statusFlags |= 0x0002;
        }
        if(temperatureStatusEnable)
        {
            statusFlags |= 0x0004;
        }
        if(antennaStatusEnable)
        {
            statusFlags |= 0x0008;
        }                
    }

    private void resetStatusFlags()
    {
        statusFlags = 0x00;
    }
    

  static class Message
  {
    byte[] data;
    int writeIndex;
    int readIndex;

    Message()
    {
      this(256);
    }

    Message(int size)
    {
      data = new byte[size];
      writeIndex = 2;
    }

    void setu8(int val)
    {
      data[writeIndex++] = (byte)(val & 0xff);
    }

    void setu16(int val)
    {
      data[writeIndex++] = (byte)((val >> 8) & 0xff);
      data[writeIndex++] = (byte)((val >> 0) & 0xff);
    }

    void setu32(int val)
    {
      data[writeIndex++] = (byte)((val >> 24) & 0xff);
      data[writeIndex++] = (byte)((val >> 16) & 0xff);
      data[writeIndex++] = (byte)((val >>  8) & 0xff);
      data[writeIndex++] = (byte)((val >>  0) & 0xff);
    }

    void setbytes(byte[] array)
    {
     if(array!=null)
     {
       setbytes(array, 0, array.length);
     }
    }

    void setbytes(byte[] array, int start, int length)
    {
      System.arraycopy(array, start, data, writeIndex, length);
      writeIndex += length;
    }

    int getu8()
    {
      return getu8at(readIndex++);
    }

    int getu16()
    {
      int val;
      val = getu16at(readIndex);
      readIndex += 2;
      return val;
    }

    short gets16()
    {
      short val;
      val = (short)getu16at(readIndex);
      readIndex += 2;
      return val;
    }

    int getu24()
    {
      int val;
      val = getu24at(readIndex);
      readIndex += 3;
      return val;
    }

    int getu32()
    {
      int val;
      val = getu32at(readIndex);
      readIndex += 4;
      return val;
    }

    void getbytes(byte[] destination, int length)
    {
      System.arraycopy(data, readIndex, destination, 0, length);
      readIndex += length;
    }

    int getu8at(int offset)
    {
      return data[offset] & 0xff;
    }

    int getu16at(int offset)
    {
      return ( (data[offset] & 0xff) <<  8)
        | ((data[offset + 1] & 0xff) <<  0);
    }

    int getu24at(int offset)
    {
      return ( (data[offset] & 0xff) << 16)
        | ((data[offset + 1] & 0xff) <<  8)
        | ((data[offset + 0] & 0xff) <<  0);
    }

    int getu32at(int offset)
    {
      return ( (data[offset] & 0xff) << 24)
        | ((data[offset + 1] & 0xff) << 16)
        | ((data[offset + 2] & 0xff) <<  8)
        | ((data[offset + 3] & 0xff) <<  0);
    }


  }


     class BackgroundParser implements Runnable
    {
        Message msg = new Message();
        TagProtocol tagProtocol;
        long baseTime;
        int readIndex;
        ArrayList<TagReadData> tagData = new ArrayList<TagReadData>();

        public BackgroundParser(Message m, TagProtocol protocol, long baseTime, int readIndex)
        {
            this.msg = m;
            this.msg.readIndex = readIndex;
            this.tagProtocol = protocol;
            this.baseTime = baseTime;
        }

        Message getMessage()
        {
            return msg;
        }

        ArrayList<TagReadData> getData()
        {
            return tagData;
        }

        public void run()
        {
            TagReadData t = new TagReadData();
            Set<TagMetadataFlag> metadataFlags = tagMetadataSet(msg.getu16());
            msg.readIndex += 1; // skip response type
            metadataFromMessage(t, msg, metadataFlags);
            t.antenna = antennaPortReverseMap.get(t.antenna);
            int epcLen = msg.getu16() / 8;
            t.tag = parseTag(msg, epcLen, tagProtocol);
            t.readBase = baseTime;
            tagData.add(t);
        }
    }//end of backgroundparser class


  /* (note: explicitly not a javadoc)
   * Send a raw message to the serial reader.
   *
   * @param timeout the duration in milliseconds to wait for a response
   * @param message The bytes of the message to send to the reader,
   * starting with the opcode. The message header, length, and
   * trailing CRC are not included. The message can not be empty, or
   * longer than 251 bytes.
   * @return The bytes of the response, from the opcode to the end of
   * the message. Header, length, and CRC are not included.
   * @throws ReaderCommException in the event of a timeout (failure to
   * recieve a complete message in the specified time) or a CRC
   * error. Does not generate exceptions for non-zero status
   * responses.
   */
  public synchronized byte[] cmdRaw(int timeout, byte... message)
    throws ReaderException
  {
    Message m = new Message();

    if (message.length < 1)
    {
      throw new IllegalArgumentException("Raw serial message can not be empty");
    }

    System.arraycopy(message, 0, m.data, 2, message.length);

    sendTimeout(timeout, m);

    int len = m.getu8at(1);
    byte[] response = new byte[len];
    System.arraycopy(m.data, 2, response, 0, len);
    return response;
  }

  private synchronized void sendMessage(int timeout, Message m)
    throws ReaderException
  {
      /* Wake up processor from deep sleep.  Tickle the RS-232 line, then
       * wait a fixed delay while the processor spins up communications again. */
      if (((powerMode == powerMode.INVALID) || (powerMode == powerMode.SLEEP)) && supportsPreamble)
      {
          byte[] flushBytes = {
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
              (byte) 0xFF, (byte) 0xFF,};
      /* Calculate fixed delay in terms of byte-lengths at current speed */
      /** @todo Optimize delay length.  This value (100 bytes at 9600bps) is taken
       * directly from arbser, which was itself using a hastily-chosen value.*/ 
      int bytesper100ms = (baudRate/50);
      for(int bytesSent=0;bytesSent<bytesper100ms;bytesSent+=flushBytes.length)
      {
        st.sendBytes(flushBytes.length, flushBytes, 0, transportTimeout);
        if (hasSerialListeners)
        {
          byte[] message = new byte[flushBytes.length];
          System.arraycopy(flushBytes, 0, message, 0, flushBytes.length);
          for (TransportListener l : serialListeners)
          {
            l.message(true, message, transportTimeout);
          }
        }
      }
    }
    m.data[0] = (byte)0xff;
    m.data[1] = (byte)(m.writeIndex - 3);

    opCode = m.data[2] & 0xff;  //Save opCode to check on Receive

    m.setu16(calcCrc(m.data, 1, m.writeIndex - 1));

    int len = m.writeIndex;

    if (hasSerialListeners)
    {
      byte[] message = new byte[len];
      System.arraycopy(m.data, 0, message, 0, len);
      for (TransportListener l : serialListeners)
      {
        l.message(true, message, timeout + transportTimeout);
      }
    }

    st.sendBytes(len, m.data, 0, timeout + transportTimeout);
  }

  private synchronized void receiveMessage(int timeout, Message m)
    throws ReaderException
  {
    int sofPosition;
    boolean sofFound=false;
    long enterTime=System.currentTimeMillis();
    int messageLength = 0;

    if(isCRCEnabled)
    {
        messageLength = 7;
    }
    else
    {
        messageLength = 5;
    }
    while((System.currentTimeMillis()-enterTime)<timeout)
    {
        // System.out.println("timeout" +timeout + "  transportTimeout = " +transportTimeout+"  Total ="+(timeout + transportTimeout));
      byte[] receiveBytes = st.receiveBytes(messageLength, m.data, 0, timeout + transportTimeout);
      if(m.data[0]!=(byte)0xff)
      {
        int i=0;
        for(i=1;i<(messageLength - 1);i++)
        {
          if(m.data[i]==(byte)0xff)
          {
            sofPosition=i;
            sofFound=true;
            System.arraycopy(m.data,sofPosition,m.data,0,messageLength-sofPosition);
            st.receiveBytes(sofPosition, m.data,messageLength-sofPosition, timeout + transportTimeout);
            break;
          }
        }
        if(sofFound)
          break;

      }
      else
      {
        sofFound=true;
        break;
      }
    }
    if(sofFound==false)
      {
        if (hasSerialListeners)
        {
          // Keep value of len set to length of data
          byte[] message = new byte[messageLength];
          System.arraycopy(m.data, 0, message, 0, messageLength);
          for (TransportListener l : serialListeners)
          {
            l.message(false, message, timeout + transportTimeout);
          }
        }
        throw new ReaderCommException(
        String.format("No soh FOund"));
      }

    int len = m.data[1] & 0xff;
    //Now pull in the rest of the data, if exists, + the CRC

    if (len != 0) 
    {
        st.receiveBytes(len, m.data, messageLength, timeout + transportTimeout);
    }

    if (hasSerialListeners)
    {
      // Keep value of len set to length of data
      byte[] message = new byte[len+messageLength];
      try
      {
          System.arraycopy(m.data, 0, message, 0, len+messageLength);
      }
      catch(ArrayIndexOutOfBoundsException ex)
      {
          throw new ReaderCommException("Invalid M6e response header, SOH not found in response");
      }
      for (TransportListener l : serialListeners)
      {
        l.message(false, message, timeout + transportTimeout);
      }
    }

    
    if(isCRCEnabled)
    {
    //Calculate the crc for the data
        int crc = calcCrc(m.data, 1, len + 4);
    //Compare with message's crc
        if ((m.data[len + 5] != (byte) ((crc >> 8) & 0xff))
                || (m.data[len + 6] != (byte) (crc & 0xff)))
        {
           if(useStreaming)
           {
               receiveMessage(timeout, m);
           }
           else
           {
               throw new ReaderCommException(
                    String.format("Reader failed crc check.  Message crc %x %x data crc %x %x", m.data[len + 5], m.data[len + 6], (crc >> 8 & 0xff), (crc & 0xff)));
           }
    }
    }
    if ((m.data[2] != (byte) opCode)&& (m.data[2] != 0x2F || !useStreaming))
    {
      /* We got a response for a different command than the one we
       * sent. This usually means we recieved the boot-time message from
       * a M6e, and thus that the device was rebooted somewhere between
       * the previous command and this one. Report this as a problem.
     */
      throw new ReaderCommException(
        String.format("Device was reset externally.  "+
                      "Response opcode (%02x) did not match command (%02x)",
                      opCode, m.data[2]));
    }


    int status = m.getu16at(3);

    if ((status & 0x7f00) == 0x7f00)
    {
      // Module assertion. Decode the assert string from the response.
      int lineNum;

      lineNum = m.getu32at(5);
      
      throw new ReaderFatalException(
        String.format("Reader assert 0x%x at %s:%d", status, 
                      new String(m.data, 9, m.writeIndex - 9), lineNum));
    }

    if (status != TM_SUCCESS)
    {
      throw new ReaderCodeException(status);
    }
    m.writeIndex = 5 + (m.data[1] & 0xff);  //Set the write index to start of CRC
    m.readIndex = 5;                        //Set read index to start of message

  }

  private synchronized Message sendTimeout(int timeout, Message m)
    throws ReaderException
  {
    
    sendMessage(timeout, m);
    receiveMessage(timeout, m);

    return m;
  }

  private Message send(Message m)
    throws ReaderException
  {
    
    return sendTimeout(commandTimeout, m);
  }

  private Message sendOpcode(int opcode)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(opcode);
    return sendTimeout(commandTimeout, m);
  }

  // ThingMagic-mutated CRC used for messages.
  // Notably, not a CCITT CRC-16, though it looks close.
  private static int crcTable[] = 
  {
    0x0000, 0x1021, 0x2042, 0x3063,
    0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b,
    0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  };

  // calculates ThingMagic's CRC-16
  private static short calcCrc(byte[] message, int offset, int length)
  {
    int crc = 0xffff;

    for (int i = offset; i < offset + length; i++)
    {
      crc = ((crc << 4) | ((message[i] >> 4) & 0xf)) ^ crcTable[crc >> 12];
      crc &= 0xffff;
      crc = ((crc << 4) | ((message[i] >> 0) & 0xf)) ^ crcTable[crc >> 12];
      crc &= 0xffff;
    }
    return (short)crc;
  }

  protected List<TransportListener> serialListeners;
  protected boolean hasSerialListeners;

  /** 
   * Register an object to be notified of serial packets.
   *
   * @param listener the SerialListener to add
   */
  public void addTransportListener(TransportListener listener)
  {
      if(null != listener)
      {
        serialListeners.add(listener);
        hasSerialListeners = true;
      }
  }

  class ListenerAdapter implements TransportListener
  {
    public void message(boolean tx, byte[] data, int timeout)
    {
      for (TransportListener l : serialListeners)
      {
        l.message(tx, data, timeout);
      }
    }
  }

  TransportListener listenerAdapter = new ListenerAdapter();

  public void removeTransportListener(TransportListener l)
  {
    serialListeners.remove(l);
    if (serialListeners.isEmpty())
    {
      hasSerialListeners = false;
    }
  }

    @Override
    public void addStatusListener(StatusListener listener)
    {
        assignStatusFlags();
        statusListeners.add(listener);        
    }

    @Override
    public void removeStatusListener(StatusListener listener)
    {
        statusListeners.remove(listener);
        // resetting statusflags
        resetStatusFlags();
        // assingStatusFlags() to update the flags with latest settings
        assignStatusFlags();
    }

    @Override
    public void addStatsListener(StatsListener listener)
    {
        statsListeners.add(listener);
    }

    @Override
    public void removeStatsListener(StatsListener listener)
    {
        statsListeners.remove(listener);
    }    

  /**
   * Set the baud rate of the serial port in use.  
   * <p>
   *
   * NOTE: This is a low-level command and should only be used in
   * conjunction with cmdSetBaudRate() or cmdBootBootloader()
   * below. For changing the rate used by the API in general, see the
   * "/reader/baudRate" parameter.
   */
  public void setSerialBaudRate(int rate)
    throws ReaderException
  {
    st.setBaudRate(rate);    
  }

  // "Level 3" interface - direct wrappers around specific serial
  // commands


  /**
   * This class represents a version number for a component of the module.
   * Instances of this class are immutable.
   */
  public static final class VersionNumber
    implements Comparable<VersionNumber>
  {
    final int part1, part2, part3, part4;
    final long compositeVersion;

    /**
     * Construct a new VersionNumber object given the individual components.
     * Note that all version number components are discussed and
     * presented in hexadecimal format, that is, in the version number
     * "9.5.12.0", the 12 is 0x12 and should be passed to this
     * constructor as such.
     */
     
    public VersionNumber(int all)
    {
      part1 = (all >> 24) & 0xff;
      part2 = (all >> 16) & 0xff;
      part3 = (all >>  8) & 0xff;
      part4 = (all >>  0) & 0xff;
      compositeVersion = all;
    }
    
    /**
     * @param part1 the first part of the version number
     * @param part2 the second part of the version number
     * @param part3 the third part of the version number
     * @param part4 the fourth part of the version number
     */
    public VersionNumber(int part1, int part2, int part3, int part4)
    {
      if ((part1 < 0 || part1 > 0xff) ||
          (part2 < 0 || part2 > 0xff) ||
          (part3 < 0 || part3 > 0xff) ||
          (part4 < 0 || part4 > 0xff))
      {
        throw new IllegalArgumentException(
          "Version field not in range 0x0-0xff");
      }
          
      this.part1 = part1;
      this.part2 = part2;
      this.part3 = part3;
      this.part4 = part4;
      compositeVersion =
        (part1 << 24) | 
        (part2 << 16) |
        (part3 <<  8) |
        (part4 <<  0);
    }

    public int compareTo(VersionNumber v)
    {
      return (int)(this.compositeVersion - v.compositeVersion);
    }

    @Override
    /**
     * Return a string representation of the version number, as a
     * sequence of four two-digit hexadecimal numbers separated by
     * dots, for example "09.05.12.0".
     */
    public String toString()
    {
      return String.format("%02x.%02x.%02x.%02x", part1, part2, part3, part4);
    }

    @Override
    public boolean equals(Object o)
    {
      if (!(o instanceof VersionNumber))
      {
        return false;
      }
      VersionNumber vn = (VersionNumber)o;
      return compositeVersion == vn.compositeVersion;
    }

    @Override
    public int hashCode()
    {
      return (int)compositeVersion;
    }

  }

  /**
   * Container class for the version information about the device,
   * including a list of the protocols that are supported.
   */
  public static final class VersionInfo
  {
    public VersionNumber bootloader;
    public VersionNumber hardware;
    public VersionNumber fwDate;
    public VersionNumber fwVersion;
    public TagProtocol[] protocols;
  }

  /**
   * Read the contents of flash from the specified address in the specified flash sector.
   *
   * @deprecated
   * @param sector the flash sector, as described in the embedded module user manual
   * @param address the byte address to start reading from
   * @param length the number of bytes to read. Limited to 248 bytes.
   * @return the bytes read
   */

  public byte[] cmdReadFlash(int sector, int address, int length)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_READ_FLASH);
    m.setu32(address);
    m.setu8(sector);
    m.setu8(length);

    sendTimeout(3000, m);

    byte[] response = new byte[length];
    System.arraycopy(m.data, 5, response, 0, length);

    return response;
  }

  /**
   * Get the version information about the device.
   *
   * 
   * @return the VersionInfo structure describing the device.
   */
  private VersionInfo cmdVersion()
    throws ReaderException
  {
    VersionInfo vInfo = null;
    Message m = new Message();
    m.setu8(MSG_OPCODE_VERSION);

    int transTimeout = (Integer)paramGet(TMR_PARAM_TRANSPORTTIMEOUT);
    try
    {
        if(!userTransportTimeout)
        {
        paramSet(TMR_PARAM_TRANSPORTTIMEOUT, 100);
        }
        userTransportTimeout = false;
        vInfo = parseVersion(sendTimeout(10,m));
    }
    finally
    {
        paramSet(TMR_PARAM_TRANSPORTTIMEOUT, transTimeout);
    }
    return vInfo;            
  }

  private VersionInfo parseVersion(Message m)
  {
    VersionInfo  v = new VersionInfo();

    v.bootloader = new VersionNumber(m.getu32());
    v.hardware = new VersionNumber(m.getu32());
    v.fwDate = new VersionNumber(m.getu32());
    v.fwVersion = new VersionNumber(m.getu32());

    int protocolBits = m.getu32();
    int protocolCount = 0;
    for (int i = 0 ; i < 32; i++)
    {
      if ((protocolBits & (1 << i)) != 0)
      {
        protocolCount++;
      }
    }

    v.protocols = new TagProtocol[protocolCount];
    int j = 0;
    for (int i = 0 ; i < 32; i++)
    {
      if ((protocolBits & (1 << i)) != 0)
      {
        TagProtocol p = codeToProtocolMap.get(i + 1);
        v.protocols[j++] = p;
      }
    }

    return v;
  }

  /**
   * Tell the boot loader to verify the application firmware and execute it.
   *   
   */
  private VersionInfo cmdBootFirmware()
    throws ReaderException
  {
    Message m = sendOpcode(MSG_OPCODE_BOOT_FIRMWARE);
    return parseVersion(m);
  }

  /**
   * Tell the device to change the baud rate it uses for
   * communication. Note that this does not affect the host side of
   * the serial interface; it will need to be changed separately.
   *
   * @param rate the new baud rate to use.
   */
  private void cmdSetBaudRate(int rate)
    throws ReaderException
  {
    Message m = new Message();
    int val;

    m.setu8(MSG_OPCODE_SET_BAUD_RATE);

    val = 0;
    switch (rate)
    {
    case 9600:   val = 488; break;
    case 19200:  val = 244; break;
    case 38400:  val = 122; break;
    case 57600:  val =  84; break;
    case 115200: val =  41; break; 
    case 230400: val =  20; break;
    case 460800: val =  10; break;
    case 921600: val =   5; break;
    }

    if (0 != val)
    {
      m.setu16(val);
    }
    else
    {
      m.setu32(rate);
    }
    send(m);
  }

  /**
   * Verify that the application image in flash has a valid checksum.
   * The device will report an invalid checksum with a error code
   * response, which would normally generate a ReaderCodeException;
   * this routine traps that particular exception and simply returns
   * "false".
   *
   * @deprecated
   * @return whether the image is valid
   */
  public boolean cmdVerifyImage()
    throws ReaderException
  {
    try 
    {
      sendOpcode(MSG_OPCODE_VERIFY_IMAGE_CRC);
    }
    catch (ReaderCodeException re)
    {
      if (re.getCode() == FAULT_BL_INVALID_IMAGE_CRC)
      {
        return false;
      }
      throw re;
    }
    return true;
  }

  /**
   * Erase a sector of the device's flash.
   *
   * @param sector the flash sector, as described in the embedded
   * module user manual
   * @param password the erase password for the sector
   */
  public void cmdEraseFlash(int sector, int password)
    throws ReaderException
  {
    if (sector < 0 || sector > 255)
    {
      throw new IllegalArgumentException("illegal sector " + sector);
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_ERASE_FLASH);
    m.setu32(password);
    m.setu8(sector);

    sendTimeout(30000, m);
  }

  /**
   * Write data to a previously erased region of the device's flash.
   *
   * @deprecated
   * @param sector the flash sector, as described in the embedded module user manual
   * @param address the byte address to start writing from
   * @param password the write password for the sector
   * @param data the data to write (from offset to offset + length - 1)
   * @param offset the index of the data to be written in the data array
   */
  public void cmdWriteFlash(int sector, int address, int password,
                            byte[] data, int offset)
    throws ReaderException
  {

    if (sector < 0 || sector > 255)
    {
      throw new IllegalArgumentException("illegal sector " + sector);
    }
    if (data.length > 240)
    {
      throw new IllegalArgumentException("data too long");
    }
//    if (length > data.length - offset)
//    {
//      throw new IllegalArgumentException("not enough data supplied");
//    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_WRITE_FLASH_SECTOR);
    m.setu32(password);
    m.setu32(address);
    m.setu8(sector);
    m.setbytes(data, offset, data.length);

    sendTimeout(3000, m);
  }

  /**
   * Return the size of a flash sector of the device.
   *
   * @param sector the flash sector, as described in the embedded module user manual
   */
  public int cmdGetSectorSize(int sector)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_SECTOR_SIZE);
    m.setu8(sector);
    send(m);

    return m.getu32();
  }

  /**
   * Write data to the device's flash, erasing if necessary.
   *
   * @deprecated
   * @param sector the flash sector, as described in the embedded module user manual
   * @param address the byte address to start writing from
   * @param password the write password for the sector
   * @param data the data to write (from offset to offset + length - 1)
   * @param offset the index of the data to be writtin in the data array
   */
  public void cmdModifyFlash(int sector, int address, int password,
                             byte[] data, int offset)
    throws ReaderException
  {

    if (sector < 0 || sector > 255)
    {
      throw new IllegalArgumentException("illegal sector " + sector);
    }
    if (data.length > 240)
    {
      throw new IllegalArgumentException("data too long");
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_MODIFY_FLASH_SECTOR);
    m.setu32(password);
    m.setu32(address);
    m.setu8(sector);
    m.setbytes(data, offset, data.length);

    sendTimeout(3000, m);
  }
  
  /**
   * Quit running the application and execute the bootloader. Note
   * that this changes the reader's baud rate to 9600; the user is
   * responsible for changing the local serial sort speed.
   *
   * @deprecated
   */ 
  public void cmdBootBootloader()
    throws ReaderException
  {
    isCRCEnabled = true;
    sendOpcode(MSG_OPCODE_BOOT_BOOTLOADER);
  }

  /** 
   * Return the identity of the program currently running on the
   * device (bootloader or application).
   */ 
  public int cmdGetCurrentProgram()
    throws ReaderException
  {
    Message m;

    m = sendOpcode(MSG_OPCODE_GET_CURRENT_PROGRAM);
    return m.getu8();
  }


  /**
   * The antenna configuration to use for {@link #cmdReadTagMultiple}.
   */
  public enum AntennaSelection
  {
      CONFIGURED_ANTENNA (0),
      ANTENNA_1_THEN_2   (1),
      ANTENNA_2_THEN_1   (2),
      CONFIGURED_LIST    (3),
      READ_MULTIPLE_SEARCH_FLAGS_EMBEDDED_OP (4),
      READ_MULTIPLE_SEARCH_FLAGS_TAG_STREAMING (8),
      LARGE_TAG_POPULATION_SUPPORT (10);

      final int value;
      AntennaSelection(int v)
      {
        value = v;
      }
  }

    /**
     * Product Group Information
     */
    private enum ProductGroupID 
    {
        MODULE(0),
        RUGGEDIZED_READER(1),
        USB_READER(2),
        INVALID(0xFFFF);
        final int rep;

        private ProductGroupID(int v)
        {
            rep = v;
        }
    }


  /**
   * Search for a single tag for up to a specified amount of time.
   *
   * @deprecated 
   * @param timeout the duration in milliseconds to search for a tag. Valid range is 0-65535
   * @param metadataFlags the set of metadata values to retrieve and store in the returned object
   * @param filter a specification of the air protocol filtering to perform
   * @param protocol the protocol to expect in the returned data
   * @return a TagReadData object containing the tag found and the
   * metadata associated with the successful search, or null if no tag
   * is found.
   */
  public TagReadData cmdReadTagSingle(int timeout, 
                                      Set<TagMetadataFlag> metadataFlags,
                                      TagFilter filter, TagProtocol protocol)
    throws ReaderException
  {
    Message m;
    TagReadData tr;
    int optByte, option, metadataBits, epcLen;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    m.setu8(MSG_OPCODE_READ_TAG_ID_SINGLE);

    option = 0;
    metadataBits = tagMetadataSetValue(metadataFlags);

    if (metadataBits != 0)
    {
      option |= 0x10;
    }

    m.setu16(timeout);
    optByte = m.writeIndex++;
    if (metadataBits != 0)
    {
      m.setu16(metadataBits);
    }
    filterBytes(protocol, m, optByte, filter, 0, true);
    m.data[optByte] |= option;

    long startTime = System.currentTimeMillis();
    sendTimeout(timeout, m);

    tr = new TagReadData();
    metadataFromMessage(tr, m, metadataFlags);
    epcLen = m.writeIndex - m.readIndex;
    tr.tag = parseTag(m, epcLen, protocol);
    tr.readBase = startTime;
    return tr;
  }

  private void msgSetupReadTagMultiple(Message m, int timeout,
                                       int searchFlags,
                                       TagFilter filter, TagProtocol protocol,
                                       TagMetadataFlag metadataFlags,
                                       int accessPassword, boolean fastSearch)
  {
    int optByte;

    m.setu8(MSG_OPCODE_READ_TAG_ID_MULTIPLE);
    if (isBapEnabled)
    {
        m.setu8(PROT_CONF_KEY_GEN2_BAP);
    }
    if (isStopNTags && !useStreaming) 
    {
        searchFlags |= READ_MULTIPLE_RETURN_ON_N_TAGS;
    }
    optByte = m.writeIndex++;
    if (useStreaming)
    {
        searchFlags |= READ_MULTIPLE_SEARCH_FLAGS_TAG_STREAMING | READ_MULTIPLE_SEARCH_FLAGS_LARGE_TAG_POPULATION_SUPPORT;
        if(0 != statusFlags)
        {
            searchFlags |=  READ_MULTIPLE_SEARCH_FLAGS_STATUS_REPORT_STREAMING;
        }
        else if(0 != statsFlags)
        {
            searchFlags |=  READ_MULTIPLE_SEARCH_FLAGS_STATS_REPORT_STREAMING;
        }
        if(fastSearch)
        {
            searchFlags |= READ_MULTIPLE_FAST_SEARCH;
        }
        m.setu16(searchFlags);
    }
    else
    {
        if(!fastSearch)
        {
            m.setu16(searchFlags | READ_MULTIPLE_SEARCH_FLAGS_LARGE_TAG_POPULATION_SUPPORT);
        }
        else
        {
            m.setu16(searchFlags | READ_MULTIPLE_SEARCH_FLAGS_LARGE_TAG_POPULATION_SUPPORT | READ_MULTIPLE_FAST_SEARCH);
        }
    }
    m.setu16(timeout);
     if (isStopNTags)
     {
        m.setu32(numberOfTagsToRead);
     }

    if (useStreaming)
    {
        m.setu16(allMetaBits);
        // adding streaming option
        if (0 != statusFlags)
        {
            m.setu16(statusFlags);
        } 
        else if (0 != statsFlags)
        {
            m.setu16(statsFlags);
        }
    }

    // We skip filterbytes() for a null filter and gen2 0 access
    // password so that we don't pass any filtering information at all
    // unless necessary; for some protocols (such as ISO180006B) the
    // "null" filter is not zero-length, but we don't need to send
    // that with this command.
    if(protocol == TagProtocol.ISO180006B && filter == null)
    {
      // ISO18000-6B should not include any filter arg bytes if null
    }
    else
    {
        filterBytes(protocol, m, optByte, filter, accessPassword, true);
    }
    
    if (useStreaming)
    {
        m.data[optByte] |= SINGULATION_FLAG_METADATA_ENABLED;
    }
  }

  private void msgAddGEN2DataRead(Message m, int timeout,
                                  int metadataBits, int bankValue,
                                  int address, int length, int optionByte)
  {
    m.setu8(MSG_OPCODE_READ_TAG_DATA);
    m.setu16(timeout);
    m.setu8(optionByte);//Embedded cmd option
    if (0 != metadataBits)
    {
      m.setu16(metadataBits);
    }
    m.setu8(bankValue);
    m.setu32(address);
    m.setu8(length);
  }

    /**
     * Gen2 Write Data
     * @param m
     * @param timeout
     * @param bank
     * @param address
     */
    private void msgAddGEN2DataWrite(Message m, int timeout,
            Gen2.Bank bank, int address)
    {
        msgAddGEN2DataWrite(m, timeout, bank, address, null);
    }

    /**
     * Gen2 Write Data
     * @param m
     * @param timeout
     * @param bank
     * @param address
     * @param data
     */
    private void msgAddGEN2DataWrite(Message m, int timeout,
            Gen2.Bank bank, int address, short[] data)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_DATA);
        m.setu16(timeout);
        m.setu8(0);  // Option byte - initialize
        m.setu32(address);
        m.setu8(bank.rep);
        if(data!=null && data.length!=0)
        {
            m.setbytes(ReaderUtil.convertShortArraytoByteArray(data));
        }        
    }

  private void msgAddGEN2LockTag(Message m, int timeout, int mask, int action,
                                 int password)
  {
    m.setu8(MSG_OPCODE_LOCK_TAG);
    m.setu16(timeout);
    m.setu8(0); // Option byte - initialize
    m.setu32(password);
    m.setu16(mask);
    m.setu16(action);
  }

  /**
   * msgAddGEN2WriteTag
   * @param m
   * @param timeout
   * @param epc
   * @param target
   * @throws ReaderException
   */
  private void msgAddGEN2WriteTag(Message m, int timeout, TagData epc, TagFilter target) throws ReaderException
  {
      if (timeout < 0 || timeout > 65535)
      {
          throw new IllegalArgumentException("illegal timeout " + timeout);
      }
      Gen2.Password accPw = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
      m.setu8(MSG_OPCODE_WRITE_TAG_ID);
      m.setu16(timeout);
      if (target == null)
      {
          m.setu16(0);
      } 
      else
      {
          //m.setu8(1);
          int optByte = m.writeIndex++;
          filterBytes(TagProtocol.GEN2, m, optByte, target, accPw.value, true);// writetag (23h) supported only on Gen2 tags
      }
      m.setbytes(epc.epcBytes());
  }

  private void msgAddGEN2KillTag(Message m, int timeout, int killPassword)
  {
    m.setu8(MSG_OPCODE_KILL_TAG);
    m.setu16(timeout);
    m.setu8(0); // Option byte - initialize
    m.setu32(killPassword);
    m.setu8(0); // RFU
  }

  private void msgAddGEN2BlockWrite(Message m, int timeout,Gen2.Bank bank,
                                  int wordPtr,byte wordCount,short[] data,int accessPassword,TagFilter target)
  throws ReaderException
  {
    int optByte;
    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC); //opcode
    m.setu16(timeout);
    m.setu8(0x00);//chip type
    optByte = m.writeIndex++;
    m.setu8(0x00);//block write opcode
    m.setu8(0xC7);//block write opcode
    filterBytes(TagProtocol.GEN2, m,optByte, target,accessPassword, true);
    m.data[optByte] = (byte)(0x40|(m.data[optByte]));//option
    m.setu8(0x00);//Write Flags
    m.setu8(bank.rep);
    m.setu32(wordPtr);
    m.setu8(wordCount);
    byte[] blockWrite = ReaderUtil.convertShortArraytoByteArray(data);
    m.setbytes(blockWrite,0,blockWrite.length);
    
   }

  private void msgAddGEN2BlockPermaLock(Message m, int timeout,byte readLock, Gen2.Bank memBank, int blockPtr, byte blockRange,short[] mask, int accessPassword, TagFilter target)
  throws ReaderException
  {

    int optByte;
    m.setu8(MSG_OPCODE_ERASE_BLOCK_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(0x00);//chip type
    optByte = m.writeIndex++;
    m.setu8(0x01);
    filterBytes(TagProtocol.GEN2, m,optByte, target,accessPassword, true);
    m.data[optByte] = (byte)(0x40|(m.data[optByte]));//option
    m.setu8(0x00);//RFU
    m.setu8(readLock);
    m.setu8(memBank.rep);
    m.setu32(blockPtr);
    m.setu8(blockRange);
    if (readLock==0x01)
    {
      m.setbytes(ReaderUtil.convertShortArraytoByteArray(mask));
    }
    
  }

  private void msgAddGEN2BlockErase(Message m, int timeout, Gen2.Bank memBank, int wordPtr, byte wordCount, int accessPassword, TagFilter target)
  throws ReaderException
  {
    int optByte;
    m.setu8(MSG_OPCODE_ERASE_BLOCK_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(0x00);//chip type
    optByte = m.writeIndex++;
    m.setu8(0x00); // Block erase
    filterBytes(TagProtocol.GEN2, m,optByte, target,accessPassword, true);
    m.data[optByte] = (byte)(0x40|(m.data[optByte]));//option            
    m.setu32(wordPtr);
    m.setu8(memBank.rep);
    m.setu8(wordCount);    
  }

  /**
   * Search for tags for a specified amount of time.
   *
   * 
   * @param timeout the duration in milliseconds to search for
   * tags. Valid range is 0-65535
   * @param selection the antenna or antennas to use for the search
   * @param filter a specification of the air protocol filtering to perform
   * @return the number of tags found
   * @see #cmdGetTagBuffer
   */
  public int cmdReadTagMultiple(int timeout, AntennaSelection selection, TagProtocol protocol, TagFilter filter, boolean fastSearch)
    throws ReaderException
  {

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    Message m = new Message();
    msgSetupReadTagMultiple(m, timeout, selection.value, filter,
                            protocol, TagMetadataFlag.ALL, 0, fastSearch);

    Message msg = sendTimeout(timeout, m);
    int dat = msg.data[1];
    switch(dat)
    {
        case 8:
                // Later 4-byte count: Large-tag-population support and ISO18k select option included in reply.
                return m.getu32at(9);
        case 7:
                // Plain 4-byte count: Reader with large-tag-population support
                return m.getu32at(8);
        case 5:
                // Later 1-byte count: ISO18k select option included in reply
                return m.getu8at(9);
        case 4:
                // Plain 1-byte count: Reader without large-tag-population support
                return m.getu8at(8);
        default:
            throw new ReaderParseException("Unrecognized Read Tag Multiple response length: " + m.data.length);

    }
  }

  private int[] executeEmbeddedRead(int timeout, Message m)
    throws ReaderException
  {

    try 
    {
      sendTimeout(timeout, m);
    }
    catch (ReaderCodeException re) 
    {
      if (re.getCode() != FAULT_NO_TAGS_FOUND)
        throw re;
      return new int[] {0, 0, 0};
    }

    int[] rv = new int[3];
    m.readIndex += 3; // Skip option and antenna selection
    rv[0] = m.getu8();  // tags found
    m.readIndex += 2; // Skip embedded command count and opcode
    rv[1] = m.getu16(); // tags successfully operated on
    rv[2] = m.getu16(); // tags unsuccessfully operated on
    return rv;
  }

  /**
   * Search for tags for a specified amount of time and kill each one.
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for
   * tags. Valid range is 0-65535
   * @param selection the antenna or antennas to use for the search
   * @param filter a specification of the air protocol filtering to perform
   * @param accessPassword the access password to use when killing the tag
   * @param killPassword the kill password to use when killing found tags
   * @return A three-element array: {the number of tags found, the
   * number of tags successfully killed, the number of tags
   * unsuccessfully killed}
   */
  public int[] cmdReadTagAndKillMultiple(int timeout, AntennaSelection selection,
                                         TagFilter filter, int accessPassword,
                                         int killPassword)
    throws ReaderException
  {
    Message m;
    int lenByte;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    msgSetupReadTagMultiple(m, timeout, selection.value | 0x04, filter,
                            TagProtocol.GEN2,TagMetadataFlag.ALL, accessPassword, false);
    m.setu8(1); // embedded command count
    lenByte = m.writeIndex++;
    msgAddGEN2KillTag(m, 0, killPassword);
    m.data[lenByte] = (byte)(m.writeIndex - (lenByte + 2));

    return executeEmbeddedRead(timeout, m);
  }

  /**
   * Search for tags for a specified amount of time and lock each one.
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for
   * tags. Valid range is 0-65535
   * @param selection the antenna or antennas to use for the search
   * @param accessPassword the password to use when locking the tag
   * @param filter a specification of the air protocol filtering to perform
   * @param mask the Gen2 lock mask
   * @param action the Gen2 lock action
   * @return A three-element array: {the number of tags found, the
   * number of tags successfully locked, the number of tags
   * unsuccessfully locked}
   */
  public int[] cmdReadTagAndLockMultiple(int timeout, AntennaSelection selection,
                                         TagFilter filter, int accessPassword,
                                         int mask, int action)
    throws ReaderException
  {
    Message m;
    int lenByte;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    msgSetupReadTagMultiple(m, timeout, selection.value | 0x04, filter,
                            TagProtocol.GEN2,TagMetadataFlag.ALL, accessPassword, false);
    m.setu8(1); // embedded command count
    lenByte = m.writeIndex++;
    msgAddGEN2LockTag(m, 0, mask, action, 0);
    m.data[lenByte] = (byte)(m.writeIndex - (lenByte + 2));

    return executeEmbeddedRead(timeout, m);
  }

  /**
   * Search for tags for a specified amount of time and write data to each one.
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for
   * tags. Valid range is 0-65535
   * @param selection the antenna or antennas to use for the search
   * @param filter a specification of the air protocol filtering to perform
   * @param accessPassword the password to use when writing the tag
   * @param bank the Gen2 memory bank to write to
   * @param address the word address to start writing at
   * @param data the data to write
   * @return A three-element array: {the number of tags found, the
   * number of tags successfully written to, the number of tags
   * unsuccessfully written to}.
   */
  public int[] cmdReadTagAndDataWriteMultiple(int timeout, 
                                              AntennaSelection selection,
                                              TagFilter filter,
                                              int accessPassword, 
                                              Gen2.Bank bank,
                                              int address, byte[] data)
    throws ReaderException
  {
    Message m;
    int lenByte;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    msgSetupReadTagMultiple(m, timeout, selection.value | 0x04, filter,
                            TagProtocol.GEN2,TagMetadataFlag.ALL, accessPassword, false);
    m.setu8(1); // embedded command count
    lenByte = m.writeIndex++;
    msgAddGEN2DataWrite(m, 0, bank, address);
    m.setbytes(data);
    m.data[lenByte] = (byte)(m.writeIndex - (lenByte + 2));

    return executeEmbeddedRead(timeout, m);
  }

  /**
   * Search for tags for a specified amount of time and read data from each one.
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for
   * tags. Valid range is 0-65535
   * @param selection the antenna or antennas to use for the search
   * @param filter a specification of the air protocol filtering to perform
   * @param accessPassword the password to use when writing the tag
   * @param bank the Gen2 memory bank to read from
   * @param address the word address to start reading from
   * @param length the number of words to read. Only two words per tag
   * will be stored in the tag buffer.
   * @return A three-element array, containing: {the number of tags
   * found, the number of tags successfully read from, the number
   * of tags unsuccessfully read from}.
   */
  public int[] cmdReadTagAndDataReadMultiple(int timeout,
                                             AntennaSelection selection,
                                             TagFilter filter,
                                             int accessPassword, 
                                             Gen2.Bank bank,
                                             int address, int length)
    throws ReaderException
  {
    Message m;
    int lenByte;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    msgSetupReadTagMultiple(m, timeout, selection.value | 0x04, filter,
                            TagProtocol.GEN2,TagMetadataFlag.ALL, accessPassword, false);
    m.setu8(1); // embedded command count
    lenByte = m.writeIndex++;
    msgAddGEN2DataRead(m, 0, 0, bank.rep, address, length, 0);
    m.data[lenByte] = (byte)(m.writeIndex - (lenByte + 2));

    return executeEmbeddedRead(timeout, m);
  }

  /**
   * Write the EPC of a tag and update the PC bits. Behavior is
   * unspecified if more than one tag can be found.
   *
   * 
   * @param timeout the duration in milliseconds to search for a tag
   * to write. Valid range is 0-65535
   * @param epc the EPC to write to the tag
   * @param target filter to search the tag
   * @param lock whether to lock the tag (does not apply to all protocols)
   */
  private void cmdWriteTagEpc(int timeout, TagData epc, TagFilter target, boolean lock)
    throws ReaderException
  {
      Message m = new Message();
      msgAddGEN2WriteTag(m, timeout, epc, target);
      sendTimeout(timeout, m);
  }

  void checkMemParams(int address, int count)
  {
    if (count < 0 || count > 127)
    {
      throw new IllegalArgumentException("Invalid word count " + count
                                         + " (out of range)");
    }
  }

  /**
   * Write data to a Gen2 tag.
   *
   * @param timeout the duration in milliseconds to search for
   * a tag to write to. Valid range is 0-65535
   * @param bank the Gen2 memory bank to write to
   * @param address the word address to start writing at
   * @param data the data to write - must be an even number of bytes
   * @param accessPassword the password to use when writing the tag
   * @param filter a specification of the air protocol filtering to
   * perform to find the tag
   */
  private void cmdGen2WriteTagData(int timeout,
                                  Gen2.Bank bank, int address, byte[] data,
                                  int accessPassword, TagFilter filter)
    throws ReaderException
  {
    Message m;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }
    checkMemParams(address, data.length/2);

    m = new Message();
    msgAddGEN2DataWrite(m, timeout, bank, address);
    filterBytesGen2(m, 5, filter, accessPassword, true);
    m.setbytes(data);

    sendTimeout(timeout, m);
  }

  /**
   * Write tag to an ISO180006B tag
   *   
   * @param timeout the duration in milliseconds to search for
   * a tag to write to. Valid range is 0-65535
   * @param address the address to start writing at
   * @param data the data to write
   * @param filter a specification of the air protocol filtering to
   * 
   */
  private void cmdIso180006bWriteTagData(int timeout, int address, byte[] data,
                                        TagFilter filter)

    throws ReaderException
  {
    Message m;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    if (address < 0 || address > 255)
    {
      throw new IllegalArgumentException("illegal address " + address);
    }

    m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_DATA);
    m.setu16(timeout);
    if (filter != null
        && (filter instanceof TagData)
        && ((TagData)filter).epc.length == 8)
    {
      m.setu8(ISO180006B_WRITE_OPTION_READ_VERIFY_AFTER
              | ISO180006B_WRITE_OPTION_COUNT_PROVIDED);
      m.setu8(ISO180006B_COMMAND_WRITE4BYTE);
      m.setu8(ISO180006B_WRITE_LOCK_NO);
      m.setu8(address);
      m.setbytes(((TagData)filter).epc);
    }
    else
    {
      m.setu8(ISO180006B_WRITE_OPTION_GROUP_SELECT
              | ISO180006B_WRITE_OPTION_COUNT_PROVIDED);
      m.setu8(ISO180006B_COMMAND_WRITE4BYTE_MULTIPLE);
      m.setu8(ISO180006B_WRITE_LOCK_NO);
      m.setu8(address);
      filterBytesIso180006b(m, -1, filter);
    }
    m.setu16(data.length);
    m.setbytes(data);

    sendTimeout(timeout, m);
  }

  /**
   * Lock a Gen2 tag
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for
   * a tag to lock. Valid range is 0-65535
   * @param mask the Gen2 lock mask
   * @param action the Gen2 lock action
   * @param accessPassword the password to use when locking the tag
   * @param filter a specification of the air protocol filtering to perform
   */
  public void cmdGen2LockTag(int timeout, int mask, int action,
                             int accessPassword, TagFilter filter)
    throws ReaderException
  {
    Message m;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    msgAddGEN2LockTag(m, timeout, mask, action, accessPassword);
    filterBytesGen2(m, 5, filter, 0, false);

    sendTimeout(timeout, m);
  }

  /**
   * Lock an ISO180006B tag
   *   
   * @param timeout the duration in milliseconds to search for
   * a tag to lock. Valid range is 0-65535
   * @param address the part of the tag to lock. Valid range is 0-255
   * @param filter a specification of the air protocol filtering to perform
   */ 
  private void cmdIso180006bLockTag(int timeout, int address, TagFilter filter)
    throws ReaderException
  {
    Message m;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    if (address < 0 || address > 255)
    {
      throw new IllegalArgumentException("illegal address " + address);
    }

    if (filter == null
        || !(filter instanceof TagData)
        || ((TagData)filter).epc.length != 8)
    {
      throw new IllegalArgumentException("illegal filter " + filter);
    }

    m = new Message();
    m.setu8(MSG_OPCODE_LOCK_TAG);
    m.setu16(timeout);
    m.setu8(ISO180006B_LOCK_OPTION_TYPE_FOLLOWS);
    m.setu8(ISO180006B_LOCK_TYPE_QUERYLOCK_THEN_LOCK);
    m.setu8(address);
    m.setbytes(((TagData)filter).epc);

    sendTimeout(timeout, m);
  }

  /**
   * Kill a Gen2 tag.
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for
   * a tag to kill. Valid range is 0-65535
   * @param killPassword the tag kill password
   * @param filter a specification of the air protocol filtering to perform
   */
  public void cmdKillTag(int timeout, int killPassword, TagFilter filter)
    throws ReaderException
  {
    Message m;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }

    m = new Message();
    msgAddGEN2KillTag(m, timeout, killPassword);
    filterBytesGen2(m, 5, filter, 0, false);

    sendTimeout(timeout, m);
  }

  /**
   * Read the memory of a Gen2 tag.
   *
   * @deprecated
   * @param timeout the duration in milliseconds to search for the operation. Valid range is 0-65535
   * @param metadataFlags the set of metadata values to retreive and store in the returned object
   * @param bank the Gen2 memory bank to read from
   * @param address the word address to start reading from
   * @param length the number of words to read.
   * @param accessPassword the password to use when writing the tag
   * @param filter a specification of the air protocol filtering to perform
   * @return a TagReadData object containing the tag data and any
   * requested metadata (note: the tag EPC will not be present in the
   * object)
   */ 
  public TagReadData cmdGen2ReadTagData(int timeout,
                                        Set<TagMetadataFlag> metadataFlags,
                                        int bankValue, int address,
                                        int length, int accessPassword,
                                        TagFilter filter)
    throws ReaderException
  {
    Message m;
    int metadataBits;
    TagReadData tr;
    long startTime;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }
    checkMemParams(address, length);

    metadataBits = tagMetadataSetValue(metadataFlags);

    m = new Message();
    msgAddGEN2DataRead(m, timeout,metadataBits, bankValue, address, length, 0);
    filterBytesGen2(m, 5, filter, accessPassword, true);

    if (metadataBits != 0)
    {
      m.data[5] |= 0x10; 
    }
    
    startTime = System.currentTimeMillis();
    sendTimeout(timeout, m);

    tr = new TagReadData();
    tr.readBase = startTime;
    m.readIndex++;
    if (metadataBits != 0)
    {
      m.readIndex += 2;
      metadataFromMessage(tr, m, metadataFlags);
    }

    tr.data = new byte[m.writeIndex - m.readIndex];
    m.getbytes(tr.data, tr.data.length);
    tr.reader = this;
    tr.tag = new Gen2.TagData("");
    tr.reader = this;

    return tr;
  }

  /**
   * Read the memory of an ISO180006B tag.
   *   
   * @param timeout the duration in milliseconds to search for
   * a tag to read. Valid range is 0-65535
   * @param address the address to start reading from
   * @param length the number of bytes to read
   * @param filter a specification of the air protocol filtering to perform
   * @return the data read. Length is dependent on protocol.
   */ 
  private byte[] cmdIso180006bReadTagData(int timeout, int address, int length,
                               TagFilter filter)
    throws ReaderException
  {
    Message m;

    if (timeout < 0 || timeout > 65535)
    {
      throw new IllegalArgumentException("illegal timeout " + timeout);
    }
    if (address < 0 || address > 255)
    {
      throw new IllegalArgumentException("illegal address " + address);
    }
    if (length < 0 || length > 8)
    {
      throw new IllegalArgumentException("ISO180006B only supports reading 8 bytes at a time " + length);
    }

    if (filter == null
        || !(filter instanceof TagData)
        || ((TagData)filter).epc.length != 8)
    {
      throw new IllegalArgumentException("ISO180006B only supports reading from a single tag specified by 64-bit EPC " + filter);
    }

    m = new Message();
    m.setu8(MSG_OPCODE_READ_TAG_DATA);
    m.setu16(timeout);
    m.setu8(1); // Standard read operation
    m.setu8(ISO180006B_COMMAND_READ);
    m.setu8(0); // RFU
    m.setu8(length);
    m.setu8(address);
    m.setbytes(((TagData)filter).epc);

    sendTimeout(timeout, m);
    
    
    byte[] result = new byte[length];
    m.getbytes(result, length);

    return result;
  }


  /**
   * Get the number of tags stored in the tag buffer
   *
   * @deprecated
   * @return a three-element array containing: {the number of tags
   * remaining, the current read index of the tag buffer, the
   * current write index of the tag buffer}.
   */
  public int[] cmdGetTagsRemaining()
    throws ReaderException
  {
    int writeIndex, readIndex;

    Message m = sendOpcode(MSG_OPCODE_GET_TAG_BUFFER);

    readIndex = m.getu16();
    writeIndex = m.getu16();
    return new int[] {writeIndex - readIndex, readIndex, writeIndex};
  }

  /**
   * Get tag data of a number of tags from the tag buffer. This
   * command moves a read index into the tag buffer, so that repeated
   * calls will fetch all of the tags in the buffer.
   *
   * @deprecated
   * @param count the maximum of tags to get from the buffer. No more
   * than 65535 may be requested. It is an error to request more tags
   * than exist.
   * @param epc496 Whether the EPCs expected are 496 bits (true) or 96 bits (false)
   * @return the tag data. Fewer tags may be returned than were requested.
   */ 
  public TagData[] cmdGetTagBuffer(int count, boolean epc496,
                                   TagProtocol protocol)
    throws ReaderException
  {
    Message m;

    if (count < 0 || count > 65535)
    {
      throw new IllegalArgumentException("illegal count " + count);
    }

    m = new Message();
    m.setu8(MSG_OPCODE_GET_TAG_BUFFER);
    m.setu16(count);

    send(m);

    return parseTagBuffer(m, epc496, protocol);
  }
  
  /**
   * Get tag data of a tags from certain locations in the tag buffer,
   * without updating the read index.
   *
   * @deprecated
   * @param start the start index to read from
   * @param end the end index to read to
   * @param epc496 Whether the EPCs expected are 496 bits (true) or 96 bits (false)
   * @return the tag data. Fewer tags may be returned than were requested. 
   */ 
  public TagData[] cmdGetTagBuffer(int start, int end, boolean epc496,
                                   TagProtocol protocol)
    throws ReaderException
  {
    Message m;

    if (start < 0 || start > 65535)
    {
      throw new IllegalArgumentException("illegal start index " + start);
    }
    if (end < 0 || end > 65535)
    {
      throw new IllegalArgumentException("illegal end index " + end);
    }

    m = new Message();
    m.setu8(MSG_OPCODE_GET_TAG_BUFFER);
    m.setu16(start);
    m.setu16(end);

    send(m);

    return parseTagBuffer(m, epc496, protocol);
  }

  TagData[] parseTagBuffer(Message m, boolean epc496,
                           TagProtocol protocol)
  {
    TagData[] tags;
    int epcLen, recordLen, numTags;
    int i, j, off;

    recordLen = epc496 ? 68 : 18;
    numTags = (m.writeIndex - m.readIndex) / recordLen;
    tags = new TagData[numTags];
    for (i = 0, off = 0; i < numTags; i++, off += recordLen)
    {
      epcLen = m.getu16();
      tags[i] = parseTag(m, epcLen, protocol);
    }
    return tags;
  }

  TagData parseTag(Message m, int epcLen, TagProtocol protocol)
  {
    TagData tag = null;
    byte[] pcbits, epcbits, crcbits;

    switch (protocol)
    {
    case GEN2:        
        final int BUFFER_SIZE = 6;        
        ByteBuffer pcBuffer = ByteBuffer.allocate(BUFFER_SIZE);

        epcLen -=2; // removed 2 bytes of CRC

        pcBuffer.put((byte) m.getu8()); // PC Byte 1
        pcBuffer.put((byte) m.getu8()); // PC Byte 2

        epcLen -=2; // Only PC Bits
               
        if ((pcBuffer.get(0) & 0x02) == 0x02) // Bitwise AND operation on 4 bytes integer msb
        {
            pcBuffer.put((byte) m.getu8()); // PC Byte 3
            pcBuffer.put((byte) m.getu8()); // PC Byte 4

            epcLen -= 2; // XPC_W1 exists, so epcLen is decremented            

            if ((pcBuffer.get(2) & 0x80) == 0x80)
            {
                /* When MSB bit is set, the XPC_W2 word will follow the XPC_W1 word */
                pcBuffer.put((byte) m.getu8()); // PC Byte 5
                pcBuffer.put((byte) m.getu8()); // PC Byte 6

                epcLen -= 2;  // PC + XPC_W1 + XPC_W2 = 6 bytes                
            }             
        } 
        int pos = pcBuffer.position();
        pcBuffer.clear();
        pcbits = new byte[pos];
        pcBuffer.get(pcbits,0, pos);// copy contents in buffer to pcbits array.
        try
        {
            if(epcLen > -1)
            {
                epcbits = new byte[epcLen];
                m.getbytes(epcbits, epcLen);
                crcbits = new byte[2];
                    m.getbytes(crcbits, 2);
                tag = new Gen2.TagData(epcbits, crcbits, pcbits);
            }
            
            if(epcLen < -2)
            {
               m.readIndex = m.readIndex - (pos + epcLen); 
            }
        }
        catch (NegativeArraySizeException nase)
        {
            // Handling NegativeArraySizeException
            // Ignoring invalid tag response (epcLen goes to negative), example below

            //  Received: ff 1c 22 00 00 10 00 1b 01 ff 01 01 d9 22 0d c6
            //  5e 00 00 00 14 00 b1 05 00 00 00 00 20 02 18 17  ab 13 36
        }
        catch (ArrayIndexOutOfBoundsException aiobe)
        {
            // Handling ArrayIndexOutOfBoundsException
        }
        break;
    case IPX256:
      epcbits = new byte[epcLen - 2];
      m.getbytes(epcbits, epcLen - 2);
      crcbits = new byte[2];
      m.getbytes(crcbits, 2);
      tag = new Ipx256.TagData(epcbits, crcbits);
      break;
    case ISO180006B:
      epcbits = new byte[epcLen - 2];
      m.getbytes(epcbits, epcLen - 2);
      crcbits = new byte[2];
      m.getbytes(crcbits, 2);
      tag = new Iso180006b.TagData(epcbits, crcbits);
      break;
    case ISO180006B_UCODE:
      epcbits = new byte[epcLen - 2];
      m.getbytes(epcbits, epcLen - 2);
      crcbits = new byte[2];
      m.getbytes(crcbits, 2);
      tag = new Iso180006bUcode.TagData(epcbits, crcbits);
      break;
    case IPX64:
      epcbits = new byte[epcLen - 2];
      m.getbytes(epcbits, epcLen - 2);
      crcbits = new byte[2];
      m.getbytes(crcbits, 2);
      tag = new Ipx64.TagData(epcbits, crcbits);
      break;
    case ATA:
      epcbits = new byte[epcLen];
      m.getbytes(epcbits, epcLen);
      crcbits = new byte[2];
      tag = new Ata.TagData(epcbits);
      break;
    default:
      epcbits = new byte[epcLen - 2];
      m.getbytes(epcbits, epcLen - 2);
      crcbits = new byte[2];
      m.getbytes(crcbits, 2);
      tag = new TagData(epcbits, crcbits);
    }
    return tag;
  }

  /**
   * Get tag data and associated read metadata from the tag buffer.
   *
   * @deprecated
   * @param metadataFlags the set of metadata values to retrieve and store in the returned objects
   * @param resend whether to resend the same tag data sent in a previous call
   * @return an array of TagReadData objects containing the tag and requested metadata
   */
  public TagReadData[] cmdGetTagBuffer(Set<TagMetadataFlag> metadataFlags,
                                       boolean resend, TagProtocol protocol)
    throws ReaderException
  {

    return cmdGetTagBufferInternal(tagMetadataSetValue(metadataFlags),
                                   resend, protocol);
  }


  TagReadData[] cmdGetTagBufferInternal(int metadataBits,
                                        boolean resend, TagProtocol protocol)
    throws ReaderException
  {
    TagReadData[] trs;
    int  numTagsInMessage;

    Message m = new Message();
    m.setu8(MSG_OPCODE_GET_TAG_BUFFER);
    m.setu16(metadataBits);
    m.setu8(resend ? 1 : 0);

    send(m);

    // the module might not support all the bits we asked for
    Set<TagMetadataFlag> metadataFlags = tagMetadataSet(m.getu16());
    m.readIndex++; // we don't need the read options
    numTagsInMessage = m.getu8();
    trs = new TagReadData[numTagsInMessage];
    for (int i = 0 ; i < numTagsInMessage; i++)
    {
      trs[i] = new TagReadData();

      metadataFromMessage(trs[i], m, metadataFlags);
      trs[i].tag = parseTag(m, m.getu16() / 8,
              protocol==TagProtocol.NONE?trs[i].readProtocol:protocol);
    }

    return trs;
  }


  /**
   * Clear the tag buffer.
   *
   */
  private void cmdClearTagBuffer()
    throws ReaderException
  {
    sendOpcode(MSG_OPCODE_CLEAR_TAG_ID_BUFFER);
  }

  /**
   * Send the Alien Higgs2 Partial Load Image command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to write on the tag
   * @param killPassword the kill password to write on the tag
   * @param EPC the EPC to write to the tag. Maximum of 12 bytes (96 bits)
   */
  private void cmdHiggs2PartialLoadImage(int timeout, int accessPassword,
                              int killPassword, byte[] EPC)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS);
    m.setu8(ALIEN_HIGGS_CHIP_SUBCOMMAND_PARTIAL_LOAD_IMAGE);
    m.setu32(killPassword);
    m.setu32(accessPassword);
    m.setbytes(EPC);

    sendTimeout(timeout, m);
  }

  /**
   * Send the Alien Higgs2 Full Load Image command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to write on the tag
   * @param killPassword the kill password to write on the tag
   * @param lockBits the lock bits to write on the tag
   * @param pcWord the PC word to write on the tag
   * @param EPC the EPC to write to the tag. Maximum of 12 bytes (96 bits)
   */
  private void cmdHiggs2FullLoadImage(int timeout, int accessPassword,
                              int killPassword, int lockBits, 
                              int pcWord, byte[] EPC)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_ALIEN_HIGGS);
    m.setu8(ALIEN_HIGGS_CHIP_SUBCOMMAND_FULL_LOAD_IMAGE);
    m.setu32(killPassword);
    m.setu32(accessPassword);
    m.setu16(lockBits);
    m.setu16(pcWord);
    m.setbytes(EPC);

    sendTimeout(timeout, m);
  }

  /**
   * Send the Alien Higgs3 Fast Load Image command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param currentAccessPassword the access password to use to write to the tag
   * @param accessPassword the access password to write on the tag
   * @param killPassword the kill password to write on the tag
   * @param pcWord the PC word to write on the tag
   * @param EPC the EPC to write to the tag. Must be exactly 12 bytes (96 bits)
   */
  private void cmdHiggs3FastLoadImage(int timeout, int currentAccessPassword,
                                     int accessPassword, int killPassword,
                                     int pcWord, byte[] EPC, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddHiggs3FastLoadImage(m, timeout, currentAccessPassword, accessPassword, killPassword, pcWord, EPC , target);
    sendTimeout(timeout, m);
  }

  /**
   * Send the Alien Higgs3 Load Image command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param currentAccessPassword the access password to use to write to the tag
   * @param accessPassword the access password to write on the tag
   * @param killPassword the kill password to write on the tag
   * @param pcWord the PC word to write on the tag
   * @param EPCAndUserData the EPC and user data to write to the
   * tag. Must be exactly 76 bytes. The pcWord specifies which of this
   * is EPC and which is user data.
   */
  private void cmdHiggs3LoadImage(int timeout, int currentAccessPassword,
                                 int accessPassword, int killPassword,
                                 int pcWord, byte[] EPCAndUserData, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddHiggs3LoadImage(m, timeout, currentAccessPassword, accessPassword, killPassword, pcWord, EPCAndUserData, target);    
    sendTimeout(timeout, m);
  }

  /**
   * Send the Alien Higgs3 Block Read Lock command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param lockBits a bitmask of bits to lock. Valid range 0-255
   * @param target
   */
  private void cmdHiggs3BlockReadLock(int timeout, int accessPassword,
                                     int lockBits, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddHiggs3BlockReadLock(m, timeout, accessPassword, lockBits, target);
    sendTimeout(timeout, m);
  }

  /**
   * Send the NXP Set Read Protect command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   */
  private void cmdNxpSetReadProtect(int timeout, int accessPassword, int chipType, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddNxpSetReadProtect(m, timeout, accessPassword, chipType, target);
    sendTimeout(timeout, m);
  }

  /**
   * Send the NXP Reset Read Protect command.
   *
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param chipType
   * @param target
   */
  private void cmdNxpResetReadProtect(int timeout, int accessPassword, int chipType, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddNxpResetReadProtect(m, timeout, accessPassword, chipType, target);
    sendTimeout(timeout, m);
  }

  /**
   * Send the NXP Change EAS command.
   *   
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param reset true to reset the EAS, false to set it
   * @param chipType
   * @param target
   */
  private void cmdNxpChangeEas(int timeout, int accessPassword, boolean reset, int chipType, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddNxpChangeEas(m, timeout, accessPassword, reset, chipType, target);
    sendTimeout(timeout, m);
  }

  
  /**
   * Send the NXP EAS Alarm command.
   *   
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param dr Gen2 divide ratio to use
   * @param m Gen2 M parameter to use
   * @param trExt Gen2 TrExt value to use
   * @param target
   * @return 8 bytes of EAS alarm data
   */
  private byte[] cmdNxpEasAlarm(int timeout, Gen2.DivideRatio dr,
                               Gen2.TagEncoding millerM, Gen2.TrExt trExt, int chipType, TagFilter target)
    throws ReaderException
  {
      if(null!=target)
      {
        throw new FeatureNotSupportedException("NxpEasAlarm with filter is not supported");
      }
      Message m = new Message();
      int optByte;

      m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
      m.setu16(timeout);
      m.setu8(chipType);

      if (chipType == TAG_CHIP_TYPE_NXP)
      {
          // G2XL
          m.setu8(NXP_CHIP_SUBCOMMAND_EAS_ALARM);
      }
      else if (chipType == TAG_CHIP_TYPE_NXP_G2IL)
      {
          //G2IL
          optByte = m.writeIndex++;
          m.setu16(NXP_CHIP_SUBCOMMAND_EAS_ALARM);
          m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
      }
      m.setu8(dr.rep);
      m.setu8(millerM.rep);
      m.setu8(trExt.rep);
      sendTimeout(timeout, m);

      m.readIndex += 2;
      byte[] rv = new byte[8];
      m.getbytes(rv, 8);
      return rv;
  }

  /**
   * Send the NXP Calibrate command.
   *   
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param chipType
   * @param target
   * @return 64 bytes of calibration data
   */
  private byte[] cmdNxpCalibrate(int timeout, int accessPassword, int chipType, TagFilter target)
    throws ReaderException
  {
    Message m = new Message();
    msgAddNxpCalibrate(m, timeout, accessPassword, chipType, target);
    sendTimeout(timeout, m);

    m.readIndex += 2;
    byte[] rv = new byte[64];
    m.getbytes(rv, 64);
    return rv;
  }

    /**
     * NXP G2iL Change Configuration
     * @param timeout
     * @param accessPassword
     * @param configData
     * @param chipType
     * @param target
     * @return ConfigWord
     * @throws ReaderException
     */
    private Gen2.NXP.G2I.ConfigWord cmdNxpChangeConfig(int timeout, int accessPassword, int configData, int chipType, TagFilter target)
            throws ReaderException
    {
        Message m = new Message();
        msgAddNxpChangeConfig(m, timeout, accessPassword, configData, chipType, target);        
        sendTimeout(timeout, m);

        m.readIndex += 4;       
        int s = m.getu16();
        
        Gen2.NXP.G2I.ConfigWord word = new Gen2.NXP.G2I.ConfigWord();
        return word.getConfigWord(s);
    }


    /**
     * Impinj Monza4 QTReadWrite Command
     * @param timeout the timeout of the operation, in milliseconds. Valid range is 0-65535.
     * @param accessPassword
     * @param controlByte -
     * @param payload - 
     * @param qtReadWrite - The Read/Write field indicates whether the tag reads or writes QT control data
     * @param persistnce - The Persistence field indicates whether the QT control is written to nonvolatile (NVM) or volatile memory
     * @param qtSR - Bit 15 (MSB) is first transmitted bit of the payload field
     * @param qtMEM - Tag uses Public/Private Memory Map
     * @param target - target filter
     * @return instance of Gen2.QTPayload
     */
    private Gen2.Impinj.Monza4.QTPayload cmdMonza4QTReadWrite(int timeout, int accessPassword, int controlByte, int payload, TagFilter target) throws ReaderException
    {
        Message m = new Message();
        msgAddMonza4QTReadWrite(m, timeout, accessPassword, controlByte, payload, target);
        sendTimeout(timeout, m);

        m.readIndex += 4;
        int s = m.getu16();

        Gen2.Impinj.Monza4.QTPayload qtPayload = new Gen2.Impinj.Monza4.QTPayload();

        if((s&0x8000)!=0)
        {
            qtPayload.QTSR = true;
        }        
        if((s&0x4000)!=0)
        {
            qtPayload.QTMEM = true;
        }
        return qtPayload;
    }


  /**
     * IAV Denatran Activate Secure Mode
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     */
    private byte[] cmdIAVDenatranActivateSecureMode(int timeout, Gen2.Denatran.IAV.ActivateSecureMode tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 6;
        Message msg = new Message();
        msgAddIAVDenatranActivateSecureMode(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranActivateSecureMode
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranActivateSecureMode(Message msg, Gen2.Denatran.IAV.ActivateSecureMode tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
    }

    private void msgAddGen2IAVDenatran(Message m, int chiptype, int timeout, int accessPassword, TagFilter target, byte commandCode, int rfuByte)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chiptype);
        int optByte = m.writeIndex++;
        m.setu8(0x00);
        m.setu8(commandCode);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8(rfuByte);
    }
    
    private void msgAddGen2IAVDenatranSiniavMode(Message m, int chiptype, int timeout, int accessPassword, TagFilter target, byte commandCode, int rfuByte,boolean tokenDesc, byte[] token)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chiptype);
        int optByte = m.writeIndex++;
        m.setu8(0x00);
        m.setu8(commandCode);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8(rfuByte);
        if(tokenDesc)
        {
            for (int i = 0; i < token.length; i++)
            {
               m.setu8(token[i]);
            }
        }
    }
    private void msgAddGen2IAVDenatranReadFromMemMap(Message m, int chiptype, int timeout, int accessPassword, TagFilter target, byte commandCode, int rfuByte,int MMWordPtr)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chiptype);
        int optByte = m.writeIndex++;
        m.setu8(0x00);
        m.setu8(commandCode);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8(rfuByte);
        m.setu16(MMWordPtr);
    }

     private void msgAddGen2IAVDenatranWriteToMemMap(Message m, int chiptype, int timeout, int accessPassword, TagFilter target, byte commandCode, int rfuByte,int MMWordPtr,byte word,byte[] tagIdentification,byte[] writecredentials)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chiptype);
        int optByte = m.writeIndex++;
        m.setu8(0x00);
        m.setu8(commandCode);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8(rfuByte);
        m.setu16(MMWordPtr);
        m.setu16(word);
        for (int i = 0; i < tagIdentification.length; i++)
        {
            m.setu8(tagIdentification[i]);
        }
        for (int i = 0; i < writecredentials.length; i++)
        {
            m.setu8(writecredentials[i]);
        }
    }

    private void msgAddGen2IAVDenatranGetTokenId(Message m, int chiptype, int timeout, int accessPassword, TagFilter target, byte commandCode)
    {
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(chiptype);
        int optByte = m.writeIndex++;
        m.setu8(0x00);
        m.setu8(commandCode);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
    }
    /*
     * msgAddIAVDenatranAuthenticateOBU
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranAuthenticateOBU(Message msg, Gen2.Denatran.IAV.AuthenticateOBU tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
    }

    /**
     * IAV Denatran Authenticate OBU
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     */
    private byte[] cmdIAVDenatranAuthenticateOBU(int timeout, Gen2.Denatran.IAV.AuthenticateOBU tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 50;
        Message msg = new Message();
        msgAddIAVDenatranAuthenticateOBU(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranActivateSiniavMode
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranActivateSiniavMode(Message msg, Gen2.Denatran.IAV.ActivateSiniavMode tagOp, int timeout, int accessPassword, TagFilter target)
    {
        boolean tokenDesc= false;
        /**
          * Currently last two bits of the payload is used as TokenDesc
          * (Token Descriptor): 2 bits parameter indicating the presence and format of Token
          * 00 : No Token.
          * 01 : Token of 64 bits.
          */
        if(0x01 == (0x03 & tagOp.payload)){
            tokenDesc = true;
        }
        msgAddGen2IAVDenatranSiniavMode(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload, tokenDesc, tagOp.token);
    }

    /**
     * IAV Denatran ActivateSiniavMode
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 6 bytes ActivateSiniavMode data
     */
    private byte[] cmdIAVDenatranActivateSiniavMode(int timeout, Gen2.Denatran.IAV.ActivateSiniavMode tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 6;
        Message msg = new Message();
        msgAddIAVDenatranActivateSiniavMode(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranOBUAuthID
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranOBUAuthID(Message msg, Gen2.Denatran.IAV.OBUAuthID tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
    }

    /**
     * IAV Denatran OBUAuthID
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 30bytes OBUAuthID data
     */
    private byte[] cmdIAVDenatranOBUAuthID(int timeout, Gen2.Denatran.IAV.OBUAuthID tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 30;
        Message msg = new Message();
        msgAddIAVDenatranOBUAuthID(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranOBUAuthFullPass
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranOBUAuthFullPass(Message msg, Gen2.Denatran.IAV.OBUAuthFullPass tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
    }

    /**
     * IAV Denatran OBUAuthFullPass
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 28 bytes OBUAuthFullPass data
     */
    private byte[] cmdIAVDenatranOBUAuthFullPass(int timeout, Gen2.Denatran.IAV.OBUAuthFullPass tagOp, int password, TagFilter target) throws ReaderException
    {
        Message msg = new Message();
        msgAddIAVDenatranOBUAuthFullPass(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        int length=msg.data[1] - 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }
    
    /*
     * msgAddIAVDenatranOBUAuthFullPass1
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranOBUAuthFullPass1(Message msg, Gen2.Denatran.IAV.OBUAuthFullPass1 tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
    }

    /**
     * IAV Denatran OBUAuthFullPass1
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 28 bytes OBUAuthFullPass1 data
     */
    private byte[] cmdIAVDenatranOBUAuthFullPass1(int timeout, Gen2.Denatran.IAV.OBUAuthFullPass1 tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 28;
        Message msg = new Message();
        msgAddIAVDenatranOBUAuthFullPass1(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranOBUAuthFullPass2
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranOBUAuthFullPass2(Message msg, Gen2.Denatran.IAV.OBUAuthFullPass2 tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
    }

    /**
     * IAV Denatran OBUAuthFullPass2
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 28 bytes OBUAuthFullPass2 data
     */
    private byte[] cmdIAVDenatranOBUAuthFullPass2(int timeout, Gen2.Denatran.IAV.OBUAuthFullPass2 tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 28;
        Message msg = new Message();
        msgAddIAVDenatranOBUAuthFullPass2(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranOBUReadFromMemMap
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranOBUReadFromMemMap(Message msg, Gen2.Denatran.IAV.OBUReadFromMemMap tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatranReadFromMemMap(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload,tagOp.userPointer);
    }

    /**
     * IAV Denatran OBUReadFromMemMap
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 28 bytes OBUReadFromMemMap data
     */
    private byte[] cmdIAVDenatranOBUReadFromMemMap(int timeout, Gen2.Denatran.IAV.OBUReadFromMemMap tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 28;
        Message msg = new Message();
        msgAddIAVDenatranOBUReadFromMemMap(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranOBUWriteToMemMap
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranOBUWriteToMemMap(Message msg, Gen2.Denatran.IAV.OBUWriteToMemMap tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatranWriteToMemMap(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload,tagOp.userPointer,tagOp.writtenWord,tagOp.tagId,tagOp.writeCredentials);
    }

    /**
     * IAV Denatran OBUWriteToMemMap
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 6 bytes OBUWriteToMemMap data
     */
    private byte[] cmdIAVDenatranOBUWriteToFromMemMap(int timeout, Gen2.Denatran.IAV.OBUWriteToMemMap tagOp, int password, TagFilter target) throws ReaderException
    {
        int length = 6;
        Message msg = new Message();
        msgAddIAVDenatranOBUWriteToMemMap(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranReadSec
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranReadSec(Message msg, Gen2.Denatran.IAV.ReadSec tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
        msg.setu16(tagOp.writtenWord);
    }

    /**
     * IAV Denatran ReadSec
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 28 bytes ReadSec data
     */
    private byte[] cmdIAVDenatranReadSec(int timeout, Gen2.Denatran.IAV.ReadSec tagOp, int password, TagFilter target)throws ReaderException
    {
        int length = 28;
        Message msg = new Message();
        msgAddIAVDenatranReadSec(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranWriteSec
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranWriteSec(Message msg, Gen2.Denatran.IAV.WriteSec tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatran(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode, tagOp.payload);
        for (int i = 0; i < tagOp.dataWords.length; i++)
        {
            msg.setu8(tagOp.dataWords[i]);
        }
        for (int i = 0; i < tagOp.writeCredentials.length; i++)
        {
            msg.setu8(tagOp.writeCredentials[i]);
        }
    }

    /**
     * IAV Denatran WriteSec
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 6 bytes WriteSec data
     */
    private byte[] cmdIAVDenatranWriteSec(int timeout, Gen2.Denatran.IAV.WriteSec tagOp, int password, TagFilter target)throws ReaderException
    {
        int length = 6;
        Message msg = new Message();
        msgAddIAVDenatranWriteSec(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }

    /*
     * msgAddIAVDenatranGetTokenId
     * @param timeout
     * @param accessPassword
     * @param tagOp
     * @param msg
     * @param target
     */
    private void msgAddIAVDenatranGetTokenId(Message msg, Gen2.Denatran.IAV.GetTokenId tagOp, int timeout, int accessPassword, TagFilter target)
    {
        msgAddGen2IAVDenatranGetTokenId(msg, tagOp.chipType, timeout, accessPassword, target, tagOp.commandCode);
    }

    /**
     * IAV Denatran GetTokenId
     * @param timeout
     * @param password
     * @param tagOp
     * @param target
     * @throws ReaderException
     * @return 6 bytes OBUWriteToMemMap data
     */
    private byte[] cmdIAVDenatranGetTokenId(int timeout, Gen2.Denatran.IAV.GetTokenId tagOp, int password, TagFilter target) throws ReaderException
    {
        Message msg = new Message();
        msgAddIAVDenatranGetTokenId(msg, tagOp, timeout, password, target);
        sendTimeout(timeout, msg);
        msg.readIndex += 4;
        int length=msg.data[1] - 4;
        byte[] data = new byte[length];
        msg.getbytes(data, length);
        return data;
    }
  /**
   * Send the Hitachi Hibiki Read Lock command.
   *
   * @deprecated
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param mask bitmask of read lock bits to alter
   * @param action action value of read lock bits to alter
   */
  public void cmdHibikiReadLock(int timeout, int accessPassword,
                                 int mask, int action)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_HITACHI_HIBIKI);
    m.setu8(HITACHI_HIBIKI_CHIP_SUBCOMMAND_READ_LOCK);
    m.setu32(accessPassword);
    m.setu8(mask);
    m.setu8(action);
    sendTimeout(timeout, m);
  }


  public static class HibikiSystemInformation
  {
    public int infoFlags;
    public byte reservedMemory;
    public byte epcMemory;
    public byte tidMemory;
    public byte userMemory;
    public byte setAttenuate;
    public int bankLock;
    public int blockReadLock;
    public int blockRwLock;
    public int blockWriteLock;
  }

  /**
   * Send the Hitachi Hibiki Get System Information command.
   *
   * @deprecated
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @return 10-element array of integers: {info flags, reserved memory size,
   * EPC memory size, TID memory size, user memory size, set attenuate value,
   * bank lock bits, block read lock bits, block r/w lock bits, block write
   * lock bits}
   */
  public HibikiSystemInformation
  cmdHibikiGetSystemInformation(int timeout, int accessPassword)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_HITACHI_HIBIKI);
    m.setu8(HITACHI_HIBIKI_CHIP_SUBCOMMAND_GET_SYSTEM_INFORMATION);
    m.setu32(accessPassword);
    sendTimeout(timeout, m);

    HibikiSystemInformation rv = new HibikiSystemInformation();
    m.readIndex += 2;
    rv.infoFlags =      m.getu16();
    rv.reservedMemory = (byte)m.getu8();
    rv.epcMemory =      (byte)m.getu8();
    rv.tidMemory =      (byte)m.getu8();
    rv.userMemory =     (byte)m.getu8();
    rv.setAttenuate =   (byte)m.getu8();
    rv.bankLock =       m.getu16();
    rv.blockReadLock =  m.getu16();
    rv.blockRwLock =    m.getu16();
    rv.blockWriteLock = m.getu16();

    return rv;
  }

  /**
   * Send the Hitachi Hibiki Set Attenuate command.
   *
   * @deprecated
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param level the attenuation level to set
   * @param lock whether to permanently lock the attenuation level
   */
   
  public void cmdHibikiSetAttenuate(int timeout, int accessPassword, int level,
                                    boolean lock)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_HITACHI_HIBIKI);
    m.setu8(HITACHI_HIBIKI_CHIP_SUBCOMMAND_SET_ATTENUATE);
    m.setu32(accessPassword);
    m.setu8(level);
    m.setu8(lock ? 1 : 0);
    sendTimeout(timeout, m);
  }

  /**
   * Send the Hitachi Hibiki Block Lock command.
   *
   * @deprecated
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param block the block of memory to operate on
   * @param blockPassword the password for the block
   * @param mask bitmask of lock bits to alter
   * @param action value of lock bits to alter
   */
  public void cmdHibikiBlockLock(int timeout, int accessPassword, int block,
                                 int blockPassword, int mask, int action)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_HITACHI_HIBIKI);
    m.setu8(HITACHI_HIBIKI_CHIP_SUBCOMMAND_BLOCK_LOCK);
    m.setu32(accessPassword);
    m.setu8(block);
    m.setu32(blockPassword);
    m.setu8(mask);
    m.setu8(action);
    sendTimeout(timeout, m);
  }

  /**
   * Send the Hitachi Hibiki Block Read Lock command.
   *
   * @deprecated
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param block the block of memory to operate on
   * @param blockPassword the password for the block
   * @param mask bitmask of read lock bits to alter
   * @param action value of read lock bits to alter
   */
  public void cmdHibikiBlockReadLock(int timeout, int accessPassword, int block,
                                     int blockPassword, int mask, int action)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_HITACHI_HIBIKI);
    m.setu8(HITACHI_HIBIKI_CHIP_SUBCOMMAND_BLOCK_READ_LOCK);
    m.setu32(accessPassword);
    m.setu8(block);
    m.setu32(blockPassword);
    m.setu8(mask);
    m.setu8(action);
    sendTimeout(timeout, m);
  }


  /**
   * Send the Hitachi Hibiki Write Multiple Words Lock command.
   *
   * @deprecated
   * @param timeout the timeout of the operation, in milliseconds.
   * Valid range is 0-65535.
   * @param accessPassword the access password to use to write to the tag
   * @param bank the Gen2 memory bank to write to
   * @param wordOffset the word address to start writing at
   * @param data the data to write - must be an even number of bytes
   */
  public void cmdHibikiWriteMultipleWords(int timeout, int accessPassword,
                                          Gen2.Bank bank, int wordOffset,
                                          byte[] data)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(TAG_CHIP_TYPE_HITACHI_HIBIKI);
    m.setu8(HITACHI_HIBIKI_CHIP_SUBCOMMAND_WRITE_MULTIPLE_WORDS);
    m.setu32(accessPassword);
    m.setu8(bank.rep);
    m.setu32(wordOffset);
    m.setu8(data.length);
    m.setbytes(data);
    sendTimeout(timeout, m);
  }

  /**
   * Erase a range of words on a Gen2 tag that supports the 
   * optional Erase Block command.
   *
   * @deprecated
   * @param bank the Gen2 memory bank to erase words in
   * @param address the word address to start erasing at
   * @param count the number of words to erase
   */
  public void cmdEraseBlockTagSpecific(int timeout, Gen2.Bank bank,
                                       int address, int count)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_ERASE_BLOCK_TAG_SPECIFIC);
    m.setu16(timeout);
    m.setu8(0); // chip type
    m.setu8(0); // option
    m.setu32(address);
    m.setu8(bank.rep);
    m.setu8(count);
    sendTimeout(timeout, m);
  }


  /**
   * Get a block of hardware version information. This information is
   * an opaque data block.
   *
   * 
   * @param option opaque option argument
   * @param flags opaque flags argument
   * @return the version block
   */
  private byte[] cmdGetHardwareVersion(int option, int flags)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_HW_REVISION);
    m.setu8(option);
    m.setu8(flags);

    send(m);
    byte[] data = new byte[m.writeIndex - m.readIndex];
    m.getbytes(data, data.length);
    return data;
  }
  public String cmdGetSerialNumber(int option, int flags)
    throws ReaderException
  {
    byte[] serialNumber_byte = cmdGetHardwareVersion(0x00, 0x40);
    char[] serialNumber_char = new char[serialNumber_byte[3]];
    for (int i = 0; i < serialNumber_char.length; i++)
    {
      serialNumber_char[i] = (char)serialNumber_byte[i + 4];
    }
    return new String(serialNumber_char);
  }
    

  /**
   * Get the currently set Tx and Rx antenna port.
   *
   * @deprecated
   * @return a two-element array: {tx port, rx port}
   */
  public int[] cmdGetTxRxPorts()
    throws ReaderException
  
  {
    Message m;
    int tx, rx;

    m = sendOpcode(MSG_OPCODE_GET_ANTENNA_PORT);
    tx = m.getu8();
    rx = m.getu8();
    
    return new int[] {tx, rx};
  }

  /**
   * Representation of the device's antenna state.
   */
  public static class AntennaPort
  {
    /**
     * The number of physical antenna ports.
     */
    public int numPorts;
    /**
     * The current logical transmit antenna port.
     */
    public int txAntenna;
    /**
     * The current logical receive antenna port.
     */
    public int rxAntenna;
    /**
     * A list of physical antenna ports where an antenna has been detected.
     */
    public int portTerminatedList[];
  }

  /**
   * Get the current Tx and Rx antenna port, the number of physical
   * ports, and a list of ports where an antenna has been detected.
   *
   * @deprecated
   * @return an object containing the antenna port information
   */ 
  public AntennaPort cmdGetAntennaConfiguration()
    throws ReaderException
  {
    Message m = new Message();
    AntennaPort ap = new AntennaPort();
    int numTerminated;
    
    m.setu8(MSG_OPCODE_GET_ANTENNA_PORT);
    m.setu8(1);
    send(m);
    ap.txAntenna = m.getu8();
    ap.rxAntenna = m.getu8();
    ap.numPorts  = (m.writeIndex - m.readIndex);

    numTerminated = 0;
    for (int i = 0; i < ap.numPorts; i++)
    {
      if (m.data[m.readIndex + i] == 1)
      {
        numTerminated++;
      }
    }
    
    ap.portTerminatedList = new int[numTerminated];
    for (int i = 0, j = 0; i < ap.numPorts; i++)
    {
      if (m.data[m.readIndex + i] == 1)
      {
        ap.portTerminatedList[j] = i + 1;
        j++;
      }
    }

    return ap;
  }

  /**
   * Gets the search list of logical antenna ports.
   *
   * @deprecated
   * @return an array of 2-element arrays of integers interpreted as
   * (tx port, rx port) pairs. Example, representing a monostatic
   * antenna on port 1 and a bistatic antenna pair on ports 3 and 4:
   * {{1, 1}, {3, 4}}
   */
  public int[][] cmdGetAntennaSearchList()
    throws ReaderException 
  {
    Message m = new Message();
    int count;
    int[][] response;

    m.setu8(MSG_OPCODE_GET_ANTENNA_PORT);
    m.setu8(2);

    send(m);
    m.readIndex++; // Skip option byte

    count = (m.writeIndex - m.readIndex) / 2;
    response = new int[count][];
    for (int i=0, j=1; i < count; i++, j+=2)
    {
      response[i] = new int[2];
      response[i][0] = m.getu8(); // TX port
      response[i][1] = m.getu8(); // RX port
    }
        
    return response;
  }

    public int[][] getAntennaReturnLoss()
    throws ReaderException {
    Message m = new Message();
    int count;
    int[][] response;

    m.readIndex = 2; // skip option and flags
    m.setu8(MSG_OPCODE_GET_ANTENNA_PORT);
    m.setu8(6);/* antenna return loss option */
    m.data[1] = (byte) (m.readIndex - 3); /* Install length */
    send(m);

    m.readIndex++; // Skip option byte

    count = (m.writeIndex - m.readIndex) / 2;
    response = new int[count][];
    for (int i=0, j=1; i < count; i++, j+=2)
    {
      response[i] = new int[2];
      response[i][0] = m.getu8(); // TX port
      response[i][1] = m.getu8(); // RX port
    }
    return response;
  }
  /**
   * Gets the transmit powers of each antenna port.
   *
   * @deprecated
   * @return an array of 3-element arrays of integers interpreted as
   * (tx port, read power in centidbm, write power in centidbm)
   * triples. Example, with read power levels of 30 dBm and write
   * power levels of 25 dBm : {{1, 3000, 2500}, {2, 3000, 2500}}
   */
  public int[][] cmdGetAntennaPortPowers()
    throws ReaderException 
  {
    Message m = new Message();
    int[][] response;
    int count;

    m.setu8(MSG_OPCODE_GET_ANTENNA_PORT);
    m.setu8(3);

    send(m);
    m.readIndex++; // Skip option byte

    count = (m.writeIndex - m.readIndex) / 5;
    response = new int[count][];
    for (int i=0, j=1 ; i < count; i++, j+=5)
    {
      response[i] = new int[3];
      response[i][0] = m.getu8();  // Antenna number
      response[i][1] = m.getu16(); // Read power
      response[i][2] = m.getu16(); // Write power
    }

    return response;
  }


  /**
   * Gets the transmit powers and settling time of each antenna port.
   *
   * @deprecated
   * @return an array of 4-element arrays of integers interpreted as
   * (tx port, read power in centidbm, write power in centidbm,
   * settling time in microseconds) tuples.  An example with two
   * antenna ports, read power levels of 30 dBm, write power levels of
   * 25 dBm, and 500us settling times:
   * {{1, 3000, 2500, 500}, {2, 3000, 2500, 500}}
   */
  public int[][] cmdGetAntennaPortPowersAndSettlingTime()
    throws ReaderException 
  {
    Message m = new Message();
    int[][] response;
    int count;

    m.setu8(MSG_OPCODE_GET_ANTENNA_PORT);
    m.setu8(4);

    send(m);
    m.readIndex++; // Skip option byte

    count = (m.writeIndex - m.readIndex) / 7;
    response = new int[count][];
    for (int i=0; i < count; i++)
    {
      response[i] = new int[4];
      response[i][0] = m.getu8();  // Antenna number
      response[i][1] = m.gets16(); // Read power
      response[i][2] = m.gets16(); // Write power
      response[i][3] = m.getu16(); // Settling time
    }

    return response;
  }

  /**
   * Enumerate the logical antenna ports and report the antenna
   * detection status of each one.
   *
   * @deprecated
   * @return an array of 2-element arrays of integers which are
   * (module logical port, detected) pairs.
   * An example, where module's logical ports 1 and 2 have
   * detected antennas and 3 and 4 do not:
   * {{1, 1}, {2, 1}, {3, 0}, {4, 0}}
   */
  public int[][] cmdAntennaDetect()
    throws ReaderException
  {
    Message m = new Message();
    int[][] response;
    int count;

    m.setu8(MSG_OPCODE_GET_ANTENNA_PORT);
    m.setu8(5);

    send(m);
    m.readIndex++; // Skip option byte

    count = (m.writeIndex - m.readIndex)/2;
    response = new int[count][];
    for (int i=0; i < count ; i++)
    {
      response[i] = new int[2];
      response[i][0] = m.getu8();
      response[i][1] = m.getu8();
    }

    return response;
  }

  int[][] antennas;
  int[] getConnectedPorts(boolean queryModule)
    throws ReaderException
  {
    int[] connectedAntennas;
    int count;

    // First, try the new antenna-detect command (61 05)
    try
    {      
      int index, numPorts;

      // if antennas is not initialized or queryModule is true, then query the module and populate the antennas
      if(antennas.length == 0 || queryModule)
      {
      antennas = cmdAntennaDetect();
      }
      numPorts = antennas.length;
      count = 0;
      for (int i = 0; i < numPorts; i++)
      {
        // if antenna is detected, incrementing count
        if (antennas[i][1] != 0)
        {
          count++;
        }
      }
      connectedAntennas = new int[count];
      index = 0;
      for (int i = 0; i < numPorts; i++)
      {
        if (antennas[i][1] != 0)
        {
          connectedAntennas[index++] = antennas[i][0];
        }
      }
    }

    // Fall back to legacy antenna detect (61 01) if new format fails
    catch (ReaderCodeException re)
    {
      AntennaPort ap = cmdGetAntennaConfiguration();
      connectedAntennas = ap.portTerminatedList;
    }
    return connectedAntennas;
  }

  int[] getAllAntennas()
  {
      return getAntennas(null);
  }
  int[] getConnectedAntennas() throws ReaderException
  {
      Set<Integer> detected = new HashSet<Integer>();
      for (int port : getConnectedPorts(true)) {
          /**
           * Currently Antenna detection is not enabled in firmware, for m6e micro
           * in api we are making all the ports by default as, not connected in case of m6e micro
           * so, User need to set the antenna, before issuing any kind of search command, in case of m6e micro
           *
           * TO DO: remove this fix, once antenna detection is enabled in the firmware for m6e micro.
           **/
          if (TMR_SR_MODEL_M6E_MICRO != versionInfo.hardware.part1)
      {
          detected.add(port);
      }
      }
      return getAntennas(detected);
  }

  int[] getAntennas(Set<Integer> connectedPorts)
  {
      ArrayList antennaList = new ArrayList();
      for (Map.Entry<Integer,int[]> entry : antennaPortMap.entrySet())
      {
          Integer antenna = entry.getKey();
          int[] txrx = entry.getValue();
          if ((connectedPorts == null)
              || (connectedPorts.contains(txrx[0]) && connectedPorts.contains(txrx[1])))
          {
              antennaList.add(antenna);
          }
      }
      int[] antennas = new int[antennaList.size()];
      int i=0;
      for (Object x : antennaList)
      {
          Integer antenna = (Integer)x;
          antennas[i++] = antenna;
      }
      return antennas;
  }

  /**
   * Get the current global Tx power setting for read operations.
   *
   * @deprecated
   * @return the power setting, in centidbm
   */
  public int cmdGetReadTxPower()
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_TX_READ_POWER);
    m.setu8(0);
    
    send(m);
    m.readIndex++; // Skip option byte
    return m.gets16();
  }

  /**
   * Get the current global Tx power setting for read operations, and the
   * minimum and maximum power levels supported.
   *
   * @deprecated
   * @return a three-element array: {tx power setting in centidbm,
   * maximum power, minimum power}. Example: {2500, 3000, 500}
   */
  public int[] cmdGetReadTxPowerWithLimits()
    throws ReaderException
  {
    Message m = new Message();
    int[] limits = new int[3];

    m.setu8(MSG_OPCODE_GET_TX_READ_POWER);
    m.setu8(1);
    send(m);
    m.readIndex++; // Skip option byte

    limits[0] = (short)m.getu16();
    limits[1] = (short)m.getu16();
    limits[2] = (short)m.getu16();

    return limits;
  }

  /**
   * Get the current global Tx power setting for write operations.
   *
   * @deprecated
   * @return the power setting, in centidbm
   */
  public int cmdGetWriteTxPower()
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_TX_WRITE_POWER);
    m.setu8(0);
    send(m);
    m.readIndex++; // Skip option byte

    return (short)m.getu16();
  }

  /**
   * Get the current RFID protocol the device is configured to use.
   *
   * @deprecated
   * @return the current protocol
   */
  public TagProtocol cmdGetProtocol()
    throws ReaderException
  {
    Message m;

    m = sendOpcode(MSG_OPCODE_GET_TAG_PROTOCOL);
    return codeToProtocolMap.get(m.getu16());
  }

  /**
   * Get the current global Tx power setting for write operations, and the
   * minimum and maximum power levels supported.
   *
   * @deprecated
   * @return a three-element array: {tx power setting in centidbm,
   * maximum power, minimum power}. Example: {2500, 3000, 500}
   */
  public int[] cmdGetWriteTxPowerWithLimits()
    throws ReaderException
  {
    Message m = new Message();
    int[] limits = new int[3];

    m.setu8(MSG_OPCODE_GET_TX_WRITE_POWER);
    m.setu8(1);
    send(m);
    m.readIndex++; // Skip option byte

    limits[0] = m.getu16();
    limits[1] = m.getu16();
    limits[2] = m.getu16();
    return limits;
  }

  /**
   * Gets the frequencies in the current hop table
   *
   * @deprecated
   * @return an array of the frequencies in the hop table, in kHz
   */
  public int[] cmdGetFrequencyHopTable()
    throws ReaderException
  {
    Message m;
    int[] table;
    int i, off, tableLen;

    m = sendOpcode(MSG_OPCODE_GET_FREQ_HOP_TABLE);

    tableLen = (m.writeIndex - m.readIndex) / 4;
    table = new int[tableLen];
    for (i = 0, off = 0; i < tableLen; i++, off += 4)
    {
      table[i] = m.getu32();
    }
    return table;
  }

  /**
   * Gets the interval between frequency hops.
   *
   * 
   * @return the hop interval, in milliseconds
   */
  private int cmdGetFrequencyHopTime()
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_FREQ_HOP_TABLE);
    m.setu8(1);
    send(m);
    m.readIndex++;

    return m.getu32();
  }

  /**
   * Gets the state of the device's GPIO input pins.
   *
   * @return an array of GpioPin representing the state of each pin
   * (index 0 corresponding to pin 1)
   */
  private GpioPin[] cmdGetGPIO()
    throws ReaderException
  {
      Message m = new Message();
      List<GpioPin> gpioStatus = new ArrayList<GpioPin>();
      
      m.setu8(MSG_OPCODE_GET_USER_GPIO_INPUTS);// 66 opcode
      m.setu8(0x01);//option 1 to get the direction and value of all GPIO lines
      send(m);
            
      m.readIndex++; // skip option byte
                
      while(m.readIndex < m.writeIndex)
      {
          int id = m.getu8();
          boolean dir = (1 == m.getu8()) ? true : false;
          boolean value = (1 == m.getu8()) ? true : false;
          gpioStatus.add(new GpioPin(id, value, dir));        
      }
      return gpioStatus.toArray(new GpioPin[0]);
  }

  /**
   * Gets the current region the device is configured to use.
   *
   * @return the region
   */
  private Reader.Region cmdGetRegion()
    throws ReaderException
  {
    Message m;

    m = sendOpcode(MSG_OPCODE_GET_REGION);
    return codeToRegionMap.get(m.getu8());
  }

  /**
   * Region-specific parameters that are supported on the device.
   */ 
  public enum RegionConfiguration
  {

    /**
     * Whether LBT is enabled.
     * <p>
     * Type: Boolean
     */ 
    LBTENABLED (0x40);

    int value;
    RegionConfiguration(int v)
    {
      value = v;
    }
    /**
     * internal use method
     * @return value
     */
    public int getValue()
    {
      return value;
    }
  }

  /**
   * Get the value of a region-specific configuration setting.
   *
   * @deprecated
   * @param key the setting
   * @return an object with the setting value. The type of the object
   * is dependant on the key; see the RegionConfiguration class for details;
   */
  public Object cmdGetRegionConfiguration(RegionConfiguration key)
    throws ReaderException
  {
    Message m = new Message();
    Object ret = null;

    m.setu8(MSG_OPCODE_GET_REGION);
    m.setu8(1);
    m.setu8(key.getValue());
    send(m);
    m.readIndex += 3; // Skip option, region, and key

    switch (key)
    {
    case LBTENABLED:
      ret = (Boolean) (m.getu8() == 1);
    }

    return ret;
  }

  /**
   * The device power mode, for use in the /reader/powerMode
   * parameter, {@link #cmdGetPowerMode}, and {@link #cmdSetPowerMode}.
   */
  public enum PowerMode
  {
      INVALID (-1),
      FULL (0),
      MINSAVE (1),
      MEDSAVE (2),
      MAXSAVE (3),
      SLEEP (4);

    int value;
    PowerMode(int v)
    {
      value = v;
    }

    static PowerMode getPowerMode(int p)
    {
      switch (p)
      {
      case -1: return INVALID;
      case 0: return FULL;
      case 1: return MINSAVE;
      case 2: return MEDSAVE;
      case 3: return MAXSAVE;
      case 4: return SLEEP;
      default: return null;
      }
    }
  }

  /**
   * Gets the current power mode of the device.
   *
   * @deprecated
   * @return the power mode
   */
  public PowerMode cmdGetPowerMode()
    throws ReaderException
  {

    Message m = sendOpcode(MSG_OPCODE_GET_POWER_MODE);
    int mode = m.getu8();
    PowerMode p = PowerMode.getPowerMode(mode);
    if (p == null)
    {
      throw new ReaderParseException("Unknown power mode " + mode);
    }
    return p;
  }


  /**
   * The device user mode, for use in the /reader/userMode
   * parameter, {@link #cmdGetUserMode}, and {@link #cmdSetUserMode}.
   */
  public enum UserMode
  {
      NONE (0),
      PRINTER (1),
      PORTAL (3);
    int value;
    UserMode(int v)
    {
      value = v;
    }

    static UserMode getUserMode(int p)
    {
      switch (p)
      {
      case 0: return NONE;
      case 1: return PRINTER;
      case 3: return PORTAL;
      default: return null;
      }
    }
  }

  /**
   * Gets the current user mode of the device.
   *
   * @deprecated
   * @return the user mode
   */
  public UserMode cmdGetUserMode()
    throws ReaderException
  {

    Message m = sendOpcode(MSG_OPCODE_GET_USER_MODE);
    int mode = m.getu8();
    UserMode u = UserMode.getUserMode(mode);
    if (u == null)
    {
      throw new ReaderParseException("Unknown user mode " + mode);
    }
    return u;
  }
  
  /**
   * The device configuration keys for use in
   * {@link #cmdGetReaderConfiguration} and {@link #cmdSetReaderConfiguration}.
   * 
   */
  public enum Configuration
  {
    /**
     * Whether reads of the same tag EPC on distinct antennas are considered distinct tag reads.
     * <p>
     * Type: Boolean
     */
    UNIQUE_BY_ANTENNA (0),
      /**
       * Whether the reader operates in power-saving RF mode.
       * <p>
       * Type: Boolean
       */
      TRANSMIT_POWER_SAVE (1),
      /**
       * Whether the reader permits EPCs longer than 96 bits
       * <p>
       * Type: Boolean
       */
      EXTENDED_EPC (2),
      /**
       * A bitmask of the GPO pins that are used for antenna port
       * switching.
       * <p>
       * Type: Integer
       */
      ANTENNA_CONTROL_GPIO (3),
      /**
       * Whether to check for a connected antenna on each port before
       * transmitting.
       * <p>
       * Type: Boolean
       */
      SAFETY_ANTENNA_CHECK (4),
      /**
       * Whether to check for an over-temperature condition before
       * transmitting.
       * <p>
       * Type: Boolean
       */
      SAFETY_TEMPERATURE_CHECK (5),
      /**
       * In a set of reads of the same tag, whether to record the
       * metadata of the tag read with the highest RSSI value (as
       * opposed to the most recent).
       * <p> 
       * Type: Boolean
       */
      RECORD_HIGHEST_RSSI (6),
      /**
       * Whether reads of the same tag EPC with distinct tag memory
       * (in a cmdReadTagAndReadMultiple() operation) are considered
       * distinct.
       * <p>
       * Type: Boolean
       */
      UNIQUE_BY_DATA (8),
      /**
       * Whether RSSI values are reported in dBm, as opposed to
       * arbitrary uncalibrated units.
       * <p>
       * Type: Boolean
       */
      RSSI_IN_DBM (9),
      /**
       * Self jammer cancellation
       * User can enable/disable through level2 API
       */
      SELF_JAMMER_CANCELLATION (10),
      /**
       * Whether Reads of the same protocol considered same tag
       * Type : Boolean
       */
      UNIQUE_BY_PROTOCOL (0x0B),
      /**
       * General category of finished reader into which module is integrated; e.g.,
       * 0: bare module
       * 1: In-vehicle Reader (e.g., Tool Link, Vega)
       * 2: USB Reader      
       */
      PRODUCT_GROUP_ID(0x12),
      /**
       * Product ID (Group ID 0x0002 ) information
       * 0x0001 :M5e-C USB reader
       * 0x0002 :Backback NA antenna
       * 0x0003 :Backback EU antenna
       **/
      PRODUCT_ID(0x13),
      /**
       * Tag Buffer Entry Timeout
       * User can set the tag buffer timeout
       * Type: byte array
       */
      TAG_BUFFER_ENTRY_TIMEOUT (0x0d),
     /**
       * enable/disable tag filtering
       * Type: Boolean
      */
      ENABLE_FILTERING(0x0c),
      /*
       * Trasport bus type
       */
      CURRENT_MSG_TRANSPORT(0x0E),
      /*
       * Enable crc calculation
       */
      SEND_CRC(0x1b);

      int value;
      Configuration(int v)
      {
          value = v;
      }
  }

  public enum TransportType
  {
      SOURCESERIAL(0),
      SOURCEUSB(3),
      SOURCEUNKNOWN(4);
      int value;
      private TransportType(int value)
      {
          this.value = value;
      }
      private static final Map<Integer, TransportType> lookup = new HashMap<Integer, TransportType>();
      static
      {
          for (TransportType transportType : EnumSet.allOf(TransportType.class))
          {
              lookup.put(transportType.getCode(), transportType);
          }
      }
      public int getCode()
      {
          return value;
      }
      public static TransportType get(int value)
      {
          return lookup.get(value);
      }
  }

  /**
   * Gets the value of a device configuration setting.
   *
   * 
   * @param key the setting
   * @return an object with the setting value. The type of the object
   * is dependant on the key; see the Configuration class for details.
   */
  private Object cmdGetReaderConfiguration(SerialReader.Configuration key)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_READER_OPTIONAL_PARAMS);
    m.setu8(1);
    m.setu8(key.value);
    send(m);
    m.readIndex += 2;

    
    if (key == Configuration.ANTENNA_CONTROL_GPIO)
    {
      return (int)m.getu8();
    }
    else if(key == Configuration.PRODUCT_GROUP_ID)
    {
      return (int)m.getu16();
    }
    else if(key == Configuration.PRODUCT_ID)
    {
      return (int)m.getu16();
    }
    else if(key == Configuration.TAG_BUFFER_ENTRY_TIMEOUT)
    {
        return (int)m.getu32();
    }
    else if (key== Configuration.UNIQUE_BY_ANTENNA || key == Configuration.UNIQUE_BY_DATA || key == Configuration.UNIQUE_BY_PROTOCOL)
    {
      if (m.getu8() == 0)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    else if (key == Configuration.TRANSMIT_POWER_SAVE || key == Configuration.EXTENDED_EPC || key == Configuration.SAFETY_ANTENNA_CHECK
            || key == Configuration.SAFETY_TEMPERATURE_CHECK || key == Configuration.RECORD_HIGHEST_RSSI || key == Configuration.RSSI_IN_DBM
            || key == Configuration.SELF_JAMMER_CANCELLATION || key == Configuration.ENABLE_FILTERING)
    {
        if (m.getu8() == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (key == Configuration.TAG_BUFFER_ENTRY_TIMEOUT)
    {
        return (int)m.getu32();
    }
    else if(key == Configuration.CURRENT_MSG_TRANSPORT)
    {
        return (int)m.getu8();
    }
    else
    {
        throw new ReaderParseException("Unrecognized Reader Configuration Key " + key.toString());
    }
  }


  /**
   * Interface for per-protocol parameter enumerations to implement.
   */
  public interface ProtocolConfiguration 
  {

    /**
     * Internal-use method.
     */
    int getValue();
  }

  /**
   * Gen2-specific parameters that are supported on the device.
   */ 
  public enum Gen2Configuration implements ProtocolConfiguration
  {
    /**
     * The Gen2 session value.
     * <p>
     * Type: Gen2.Session
     */
    SESSION (0),
    /**
     * The Gen2 target algorithm used for inventory operations.
     * <p>
     * Type: Gen2.Target
     */
      TARGET (1),
      /**
       * The Gen2 M value.
       * <p>
       * Type: Gen2.MillerM
       */
      TAGENCODING (2),
      /**
       * The Gen2 link frequency.
       * <p>
       * Type: Gen2.LinkFrequency
       */
      LINKFREQUENCY (0x10),

      /**
       * The Gen2 Tari value.
       * Type: Gen2.Tari
       */
      TARI (0x11),

      /**
       * The Gen2 Q algorithm used for inventory operations.
       * <p>
       * Type: Gen2.Q
       */
      Q (0x12),
      /**
       * The Gen2 BAP.
       * <p>
       * Type: Gen2.BAP
       */
      BAP(0x13);

    int value;
    Gen2Configuration(int v)
    {
      value = v;
    }
    /**
     * internal use method
     * @return value
     */
    public int getValue()
    {
      return value;
    }
  }

   /**
   * Operation Options for cmdSetUserProfile 
   */ 
  public enum SetUserProfileOption 
  {
    /**  Save operation */
     SAVE (0x01), 
    /**  Restore operation */
     RESTORE (0x02) ,
    /**  Verify operation */
     VERIFY (0x03),  
    /** Clear operation  */
     CLEAR (0x04);  
     int value;
    SetUserProfileOption(int v)
    {
      value = v;
    }
    /**
     * internal use method
     * @return value
     */
    public int getValue()
    {
      return value;
    }
  }

  /**
   * User Configuration Operation
   */
    public static class UserConfigOp
    {
        SetUserProfileOption Opcode;
        
        public UserConfigOp(SetUserProfileOption setUserProfileOption)
        {
            Opcode = setUserProfileOption;
        }                
    }
  
  /**
   * Configuration key for cmdSetUserProfile
   */ 
  public enum ConfigKey 
  {
    /** All Configuration */
    ALL (0x01);  
     int value;
    ConfigKey(int v)
    {
      value = v;
    }
    /**
     * internal use method
     * @return value
     */
    public int getValue()
    {
      return value;
    }
  }
  
  /**
   * The configuration values for cmdSetUserProfile
   */ 
  public enum ConfigValue 
  {
   
    /** Firmware default configurations */
    FIRMWARE_DEFAULT(0x00),
    /** Custom configurations */
    CUSTOM_CONFIGURATION(0x01);  
     int value;
    ConfigValue(int v)
    {
      value = v;
    }
    /**
     * internal use method
     * @return value
     */
    public int getValue()
    {
      return value;
    }
  }
  /**
   * ISO18000-6B-specific parameters that are supported on the device.
   */ 
  public enum ISO180006BConfiguration implements ProtocolConfiguration
  {
    /**
     * The frequency of the tag data, in kHz.
     * <p>
     * Type: Integer
     */
    LINKFREQUENCY (0x10),
   
    /**
     * Modulation depth in transmitter
     * <p>
     * Type:Integer
     */
    MODULATIONDEPTH(0x11),

    /*
     * Delimiter in transmitter
     * <p>
     * Type:Integer
     */
    DELIMITER(0x12);

    int value;
    ISO180006BConfiguration(int v)
    {
      value = v;
    }
    /**
     * internal use method
     * @return value
     */
    public int getValue()
    {
      return value;
    }
  }
                                      
  /**
   * Gets the value of a protocol configuration setting.
   *
   * 
   * @param protocol the protocol of the setting
   * @param key the setting
   * @return an object with the setting value. The type of the object
   * is dependant on the key; see the ProtocolConfiguration class for details.
   */
  private Object cmdGetProtocolConfiguration(TagProtocol protocol,
                                     ProtocolConfiguration key)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_GET_PROTOCOL_PARAM);
    m.setu8(protocolToCodeMap.get(protocol));
    m.setu8(key.getValue());
    send(m);
    m.readIndex += 2; // Skip protocol and key

    if (protocol == TagProtocol.GEN2)
    {
      switch ((Gen2Configuration)key)
      {
      case SESSION:
        switch (m.getu8())
        {
        case 0:
          return Gen2.Session.S0;
        case 1:
          return Gen2.Session.S1;
        case 2:
          return Gen2.Session.S2;
        case 3:
          return Gen2.Session.S3;
        }
        break;
      case TARGET:
        switch(m.getu16())
        {
        case 0x0000:
          return Gen2.Target.AB;
        case 0x0001:
          return Gen2.Target.BA;
        case 0x0100:
          return Gen2.Target.A;
        case 0x0101:
          return Gen2.Target.B;
        }
        break;
      case TAGENCODING:
        switch (m.getu8())
        {
        case 0:
          return Gen2.TagEncoding.FM0;
        case 1:
          return Gen2.TagEncoding.M2;
        case 2:
          return Gen2.TagEncoding.M4;
        case 3:
          return Gen2.TagEncoding.M8;
        }
        break;
      case LINKFREQUENCY:
        switch (m.getu8())
        {
        case 0:
          return Gen2.LinkFrequency.LINK250KHZ;
        case 2:
          return Gen2.LinkFrequency.LINK320KHZ;
//        case 1:
//          return Gen2.LinkFrequency.LINK400KHZ;
//        case 3:
//          return Gen2.LinkFrequency.LINK40KHZ;
        case 4:
          return Gen2.LinkFrequency.LINK640KHZ;

        }
      case TARI:
        switch (m.getu8())
        {
        case 0:
          return Gen2.Tari.TARI_25US;
        case 1:
          return Gen2.Tari.TARI_12_5US;
        case 2:
          return Gen2.Tari.TARI_6_25US;
        }
      case Q:
        int type = m.getu8();
        if (type == 0)
        {
          return new Gen2.DynamicQ();
        }
        else if (type == 1)
        {
          return new Gen2.StaticQ(m.getu8());
        }
      case BAP:
        Gen2.Bap bap = new Gen2.Bap();
        m.readIndex = 10;
        bap.powerUpDelayUs = m.getu32();
        bap.freqHopOfftimeUs = m.getu32();
        return bap;
      default:
        throw new IllegalArgumentException("Unknown " + protocol.toString() + 
                                           " parameter " + key.toString());
      }
    }
    else if (protocol == TagProtocol.ISO180006B)
    {
      switch ((ISO180006BConfiguration)key)
      {
      case LINKFREQUENCY:
        switch (m.getu8())
        {
        case 0:
          return Iso180006b.LinkFrequency.LINK160KHZ;
        case 1:
          return Iso180006b.LinkFrequency.LINK40KHZ;
        }
        break;
       case MODULATIONDEPTH:
         switch (m.getu8())
         {
         case 0:
           return Iso180006b.ModulationDepth.MODULATION99PERCENT;
         case 1:
           return Iso180006b.ModulationDepth.MODULATION11PERCENT;
         }
         break;
       case DELIMITER:
         switch (m.getu8())
         {
         case 1:
           return Iso180006b.Delimiter.DELIMITER1;
         case 4:
           return Iso180006b.Delimiter.DELIMITER4;
         }
         break;
      default:
        throw new IllegalArgumentException("Unknown " + protocol.toString() + 
                                           " parameter " + key.toString());
      }
    }
    else
    {
      throw new IllegalArgumentException("Protocol parameters not supported for protocol " + protocol.toString());
    }

    throw new ReaderParseException("Could not interpret protocol configuration response");
  }

  /**
   * The statistics avaliable for retrieval by cmdGetReaderStatistics.
   */
  public enum ReaderStatisticsFlag
  {
    /** Total time the port has been transmitting, in milliseconds. Resettable */
      RF_ON_TIME (1),
    /** Detected noise floor with transmitter off. Recomputed when requested, not resettable.  */
        NOISE_FLOOR(2),
    /** Detected noise floor with transmitter on. Recomputed when requested, not resettable.  */
      NOISE_FLOOR_TX_ON(8);

    int value;
    ReaderStatisticsFlag(int v)
    {
      value = v;
    }
  }

    /**
     * The statistics avaliable for retrieval by cmdGetReaderStatistics.
     */
    public enum ReaderStatusFlag
    {
        /* Noise Floor */
        NOISE_FLOOR(0x0001),
        /* Frequency */
        FREQUENCY(0x00002),
        /* Temperature */
        TEMPERATURE(0x00004),
        /* Current Antenna Ports */
        CURRENT_ANTENNAS(0x0008),
        /* All */
        ALL(0x000F);
        public int value;

        ReaderStatusFlag(int v)
        {
            value = v;
        }
    }

  /**
   * Reader Statistics
   */
  public static class ReaderStatistics
  {
      public int numPorts;
      public int[] rfOnTime;
      public int[] noiseFloor;
      public int[] noiseFloorTxOn;

      @Override
      public String toString()
      {
          StringBuilder readerStats = new StringBuilder();
          readerStats.append("[numPorts : ");
          readerStats.append(numPorts);
          readerStats.append("]");
          readerStats.append("[rfOnTime : ");
          for(int i=0;i<rfOnTime.length;i++)
          {
            readerStats.append(rfOnTime[i]);
            readerStats.append(",");
          }
          readerStats.deleteCharAt(readerStats.length()-1);
          readerStats.append("]");
          readerStats.append("[noiseFloor : ");
          for(int i=0;noiseFloor!=null && i<noiseFloor.length;i++)
          {
            readerStats.append(noiseFloor[i]);
            readerStats.append(",");
          }
          readerStats.deleteCharAt(readerStats.length()-1);
          readerStats.append("]");
          readerStats.append("[noiseFloorWithTxOn : ");
          for(int i=0;noiseFloorTxOn!=null && i<noiseFloorTxOn.length;i++)
          {
            readerStats.append(noiseFloorTxOn[i]);
            readerStats.append(",");
          }
          readerStats.deleteCharAt(readerStats.length()-1);
          readerStats.append("]");
          return readerStats.toString();
      }
  }
  
  /**
     * The Stats avaliable for retrieval by cmdGetReaderStats.
     */
    public enum ReaderStatsFlag
    {
        NONE(0x00),
        /** Total time the port has been transmitting, in milliseconds. Resettable */
        RF_ON_TIME (1 << 0),
        /** Noise floor with the TX on for the antennas were last configured for searching */
        NOISE_FLOOR_SEARCH_RX_TX_WITH_TX_ON(1 << 6),
        /** Current frequency in uints of Khz */
        FREQUENCY(1 << 7),
        /** Current temperature of the device in units of Celsius */
        TEMPERATURE(1 << 8),
        /** Current antenna */
        ANTENNA(1 << 9),
        /** Current protocol */
        PROTOCOL(1 << 10),
        /** Current connected antennas */
        CONNECTED_ANTENNA_PORTS(1 << 11);
        int value;

        ReaderStatsFlag(int v)
        {
            value = v;
        }
    }

  /**
   * Reader Stats
   */
  public static class ReaderStats
  {
      public int numPorts = 0;
      public int frequency = 0;
      public int temperature = 0;
      public int antenna = 0;
      public int[] rfOnTime = new int[0];
      public int[] noiseFloor = new int[0];
      public int[] connectedAntennaPorts = new int[0];
      public byte[] noiseFloorTxOn = new byte[0];
      public TagProtocol protocol = null;
  }

/**
 * This object contains the information related to status reports
 * sent by the module during continuous reading
 */
  public static class StatusReport
  {
      
  }

  public static class AntennaStatusReport extends StatusReport
  {
      /* antenna */
    int antenna = -1;

    public int getAntenna()
    {
        return antenna;
    }

    @Override
    public String toString()
    {
        StringBuilder antReport = new StringBuilder();
        antReport.append("AntennaStatusReport : ");
        antReport.append(antenna);
        return antReport.toString();
    }
  }

  public static class FrequencyStatusReport extends StatusReport
  {
      // module actually reports a u24value, but 32-bit is the closest data type.
    int frequency = -1;

    public int getFrequency()
    {
        return frequency;
    }

    @Override
    public String toString()
    {
        StringBuilder freqReport = new StringBuilder();
        freqReport.append("FrequencyStatusReport : ");
        freqReport.append(frequency);
        return freqReport.toString();
    }

  }

  public static class TemperatureStatusReport extends StatusReport
  {
    /* Temperature */
    int temperature = -1;

    public int getTemperature()
    {
        return temperature;
    }

    @Override
    public String toString()
    {
        StringBuilder tempReport = new StringBuilder();
        tempReport.append("TemperatureStatusReport : ");
        tempReport.append(temperature);
        return tempReport.toString();
    }
  }
  

  /**
   * Get the current per-port statistics.
   *
   * @param stats the set of statistics to gather
   * @return a ReaderStatistics structure populated with the requested per-port
   * values
   */
  private ReaderStatistics cmdGetReaderStatistics(Set<ReaderStatisticsFlag> stats)
    throws ReaderException
  {
    Message m = new Message();
    ReaderStatistics ps;
    int i, len, flagBits;    
    
    flagBits = 0;
    for (ReaderStatisticsFlag f : stats)
    {
      flagBits |= f.value;
    }
    m.readIndex += 2; // skip option and flags

    m.setu8(MSG_OPCODE_GET_READER_STATS);    
    m.setu8(READER_STATS_OPTION_GET_PER_PORT); //option byte per port 0x02
    m.setu8(flagBits);

    Message m1 = new Message();
    m1 = send(m);
    ps = new ReaderStatistics();

    m1.readIndex =7; // offset

    len = ports.length;
    ps.numPorts = len;
    while (m1.readIndex < m1.writeIndex)
    {
      int statFlag = m1.getu8();
      if ((0 != statFlag) && (statFlag == ReaderStatisticsFlag.RF_ON_TIME.value))
      {
        ps.rfOnTime = new int[len];
        m1.readIndex++;
        for (i = 0; i < len; i++)
        {
          int antenna = m1.data[m1.readIndex];
          if (i == (antenna - 1))
          {
            m1.readIndex++;
            ps.rfOnTime[i] = (byte) m1.getu32(); // value is signed
          }
          else
          {
            ps.rfOnTime[i] = 0;
          }
        }
      }
      else if ((0 != statFlag) && (statFlag == ReaderStatisticsFlag.NOISE_FLOOR.value))
      {        
        ps.noiseFloor = new int[len];
        m1.readIndex++;
        for (i = 0; i < len; i++)
        {
          int antenna = m1.data[m1.readIndex];
          if (i == (antenna - 1))
          {
            m1.readIndex++;
            ps.noiseFloor[i] = (byte) m1.getu8()& 0xff; // value is signed
          }
          else
          {
            ps.noiseFloor[i] = 0;
          }
        }
      }
      else if ((0 !=statFlag) && (statFlag == ReaderStatisticsFlag.NOISE_FLOOR_TX_ON.value))
      {        
        ps.noiseFloorTxOn = new int[len];
        m1.readIndex++;
        for (i = 0; i < len; i++)
        {
          int antenna = m1.data[m1.readIndex];
          if (i == (antenna - 1))
          {
            m1.readIndex++;
            ps.noiseFloorTxOn[i] = (byte) m1.getu8()& 0xff; // value is signed
          }
          else
          {
            ps.noiseFloorTxOn[i] = 0;
          }
        }
      }              
    }//end of while loop
    return ps;
  }

  /**
   * Get the current per-port stats.
   *
   * @param stats the set of stats to gather
   * @return a ReaderStats structure populated with the requested per-port
   * values
   */
  private ReaderStats cmdGetReaderStats(Set<ReaderStatsFlag> stats)
    throws ReaderException
  {
    ReaderStats ps = null;
    try
    {
      Message m = new Message();
      int flagBits = 0;
      for (ReaderStatsFlag f : stats)
      {
        flagBits |= f.value;
      }
      m.readIndex = 2; // skip option and flags
      m.setu8(MSG_OPCODE_GET_READER_STATS);
      m.setu8(READER_STATS_OPTION_GET_PER_PORT); //option byte per port 0x02
      if ((0x80) > flagBits)
      {
        m.setu8(flagBits);
      }
      else
      {
        int temp = flagBits & 0x7f;
        flagBits &= 0xFFFE;
        flagBits = ((flagBits << 1) | temp);
        flagBits &= 0xFF7F;
        m.setu16((0x8000) | flagBits);
      }
      m.data[1] = (byte) (m.readIndex - 3); /* Install length */
      Message m1 = new Message();
      m1 = send(m);
      if ((0x80) > flagBits)
      {
        m1.readIndex = 7; // offset
      }
      else
      {
        m1.readIndex = 8; // offset
      }
      ps = fillReaderStats(m1);
    }
    catch (Exception ex)
    {
      ex.printStackTrace();
    }
    return ps;
  }

  private ReaderStats fillReaderStats(Message m1)
  {
    ReaderStats ps = new ReaderStats();
    try
    {
        int len = ports.length;
        ps.numPorts = len;
        while (m1.readIndex < m1.writeIndex)
        {
          int statFlag =0;
          if ((0x80) > m1.getu8at(m1.readIndex))
          {
            statFlag = m1.getu8at(m1.readIndex);
          }
          else
          {
            /**
            * the response flag will be in EBV format,
            * convert that to the api enum values
            */
            statFlag = (( m1.getu8at(m1.readIndex) << 8) | (m1.getu8at(m1.readIndex + 1)));
            statFlag &= 0x7fff;
            statFlag = ((statFlag >> 1) | (statFlag & 0x7f));
          }

          if ((0 != statFlag) && (statFlag == ReaderStatsFlag.RF_ON_TIME.value))
          {
            ps.rfOnTime = new int[len];
            m1.readIndex +=2;
            for ( int i = 0; i < len; i++)
            {
              int antenna = m1.getu8at(m1.readIndex);
              if (i == (antenna - 1))
              {
                m1.readIndex++;
                ps.rfOnTime[i] = m1.getu32();
                ps.numPorts = len;
              }
            }
          }
          else if ((0 !=statFlag) && (statFlag == ReaderStatsFlag.NOISE_FLOOR_SEARCH_RX_TX_WITH_TX_ON.value))
          {
            m1.readIndex += 2;
            ps.noiseFloorTxOn = new byte[len];
            Iterator iter = antennaPortMap.keySet().iterator();
            Iterator valueIter = antennaPortMap.values().iterator();

            while(valueIter.hasNext())
            {
              int[] txrx = (int[])valueIter.next();
              int logicalPort = (Integer)iter.next();
              int tx = Integer.parseInt(Integer.toString(m1.getu8at(m1.readIndex), 10));
              int rx = Integer.parseInt(Integer.toString(m1.getu8at(m1.readIndex + 1), 10));
              if(txrx[0] == tx && txrx[1] == rx)
              {
                m1.readIndex += 2;
                ps.noiseFloorTxOn[logicalPort-1]= (byte)(m1.getu8() & 0xff);
              }
            }
          }
          else if ((0 !=statFlag) && (statFlag == ReaderStatsFlag.FREQUENCY.value))
          {
            m1.readIndex += 3;
            ps.frequency = m1.getu24();
          }
          else if ((0 !=statFlag) && (statFlag == ReaderStatsFlag.TEMPERATURE.value))
          {
            m1.readIndex += 3;
            ps.temperature = (byte)m1.getu8();
          }
          else if ((0 !=statFlag) && (statFlag == ReaderStatsFlag.PROTOCOL.value))
          {
            m1.readIndex += 3;
            ps.protocol =codeToProtocolMap.get(m1.getu8());
          }
           else if ((0 !=statFlag) && (statFlag == ReaderStatsFlag.ANTENNA.value))
          {
            m1.readIndex += 3;            
            ps.antenna = antennaPortReverseMap.get((((m1.getu8() & 0xF) << 4) | (m1.getu8() & 0xF)));
          }
          else if ((0 !=statFlag) && (statFlag == ReaderStatsFlag.CONNECTED_ANTENNA_PORTS.value))
          {
            m1.readIndex += 3;
            antennas = new int[len][];
            for (int i=0; i < len ; i++)
            {
              antennas[i] = new int[2];
              antennas[i][0] = m1.getu8();
              antennas[i][1] = m1.getu8();
            }
            ps.connectedAntennaPorts=getConnectedPorts(false);
            }
        }//end of while loop
      }
      catch(Exception ex)
      {
        ex.printStackTrace();
      }
    return ps;
  }

  /**
   * Get the list of RFID protocols supported by the device.
   *
   * @deprecated
   * @return an array of supported protocols
   */
  public TagProtocol[] cmdGetAvailableProtocols()
    throws ReaderException
  {
    Message m;
    TagProtocol[] protocols;
    TagProtocol p;
    int numProtocols, numKnownProtocols;
    int i, j, index;

    m = sendOpcode(MSG_OPCODE_GET_AVAILABLE_PROTOCOLS);
    numProtocols = (m.writeIndex - m.readIndex) / 2;
    numKnownProtocols = 0;
    index = m.readIndex;
    for (i = 0; i < numProtocols; i++)
    {
      p = codeToProtocolMap.get(m.getu16());
      if (p != TagProtocol.NONE)
      {
        numKnownProtocols++;
      }
    }
    protocols = new TagProtocol[numKnownProtocols];
    j = 0;
    m.readIndex = index;
    for (i = 0; i < numProtocols; i++)
    {
      p = codeToProtocolMap.get(m.getu16());
      if (p != TagProtocol.NONE)
      {
        protocols[j++] = p;
      }
    }

    return protocols;
  }


  /**
   * Get the list of regulatory regions supported by the device.
   *
   * @deprecated
   * @return an array of supported regions
   */
  public Reader.Region[] cmdGetAvailableRegions()
    throws ReaderException
  {
    Message m;
    Reader.Region[] regions;
    Reader.Region r;
    int numRegions, numKnownRegions;
    int i, j;

    m = sendOpcode(MSG_OPCODE_GET_AVAILABLE_REGIONS);
    numRegions = (m.writeIndex - m.readIndex);
    numKnownRegions = 0;
    int index = m.readIndex;
    for (i = 0; i < numRegions; i++)
    {
      r = codeToRegionMap.get(m.getu8());
      if (r != Reader.Region.UNSPEC)
      {
        numKnownRegions++;
      }
    }
    regions = new Reader.Region[numKnownRegions];
    j = 0;
    m.readIndex = index;
    for (i = 0; i < numRegions; i++)
    {
      r = codeToRegionMap.get(m.getu8());
      if (r != Reader.Region.UNSPEC)
      {
        regions[j++] = r;
      }
    }

    return regions;
  }

  /**
   * Get the current temperature of the device.
   *
   * @deprecated
   * @return the temperature, in degrees C
   */
  public int cmdGetTemperature()
    throws ReaderException
  {
    Message m;

    m = sendOpcode(MSG_OPCODE_GET_TEMPERATURE);
    return (byte)m.getu8(); // returned value is signed
  }

  /**
   * Sets the Tx and Rx antenna port. Port numbers range from 1-255.
   *
   * @deprecated
   * @param txPort the logical antenna port to use for transmitting
   * @param rxPort the logical antenna port to use for receiving
   */
  public void cmdSetTxRxPorts(int txPort, int rxPort)
    throws ReaderException 
  {
    if (txPort < 0 || txPort > 255)
    {
      throw new IllegalArgumentException("illegal txAnt " + txPort);
    }
    if (rxPort < 0 || rxPort > 255)
    {
      throw new IllegalArgumentException("illegal rxAnt " + rxPort);
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_SET_ANTENNA_PORT);
    m.setu8(txPort);
    m.setu8(rxPort);
    send(m);
  }

  /**
   * Sets the search list of logical antenna ports. Port numbers range
   * from 1-255.
   *
   * @deprecated
   * @param list the ordered search list. An array of 2-element arrays
   * of integers interpreted as (tx port, rx port) pairs. Example,
   * representing a monostatic antenna on port 1 and a bistatic
   * antenna pair on ports 3 and 4: {{1, 1}, {3, 4}}
   */
  public void cmdSetAntennaSearchList(int[][] list)
    throws ReaderException 
  {

    for (int i = 0 ; i < list.length; i++) 
    {
      if (list[i][0] < 0 || list[i][0] > 255)
      {
        throw new IllegalArgumentException("illegal tx port " + list[i][0]);
      }
      if (list[i][1] < 0 || list[i][1] > 255)
      {
        throw new IllegalArgumentException("illegal rx port " + list[i][1]);
      }
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_SET_ANTENNA_PORT);
    m.setu8(2);
    for (int i=0; i < list.length; i++)
    {
      m.setu8(list[i][0]);
      m.setu8(list[i][1]);
    }
    send(m);
  }

  /**
   * Sets the transmit powers of each antenna port. Note that setting
   * a power level to 0 will cause the corresponding global power
   * level to be used. Port numbers range from 1-255; power levels
   * range from 0-65535.
   *
   * @deprecated
   * @param list an array of 3-element arrays of integers interpreted as
   * (tx port, read power in centidbm, write power in centidbm)
   * triples. Example, with read power levels of 30 dBm and write
   * power levels of 25 dBm : {{1, 3000, 2500}, {2, 3000, 2500}}
   */ 
  public void cmdSetAntennaPortPowers(int[][] list)
    throws ReaderException
  {
    for (int i = 0 ; i < list.length; i++) 
    {
      if (list[i][0] < 0 || list[i][0] > 255)
      {
        throw new IllegalArgumentException("illegal tx port " + list[i][0]);
      }
      if (list[i][1] < 0 || list[i][1] > 65535)
      {
        throw new IllegalArgumentException("illegal read tx power " + list[i][1]);
      }
      if (list[i][2] < 0 || list[i][2] > 65535)
      {
        throw new IllegalArgumentException("illegal write tx power " + list[i][2]);
      }
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_SET_ANTENNA_PORT);
    m.setu8(3);
    for (int i=0, j=1; i < list.length; i++, j+=5)
    {
      m.setu8(list[i][0]);
      m.setu16(list[i][1]);
      m.setu16(list[i][2]);
    }
    send(m);
  }

  /**
   * Sets the transmit powers and settling times of each antenna
   * port. Note that setting a power level to 0 will cause the
   * corresponding global power level to be used. Port numbers range
   * from 1-255; power levels range from 0-65535; settling time ranges
   * from 0-65535.
   *
   * @deprecated
   * @param list an array of 4-element arrays of integers interpreted as
   * (tx port, read power in centidbm, write power in centidbm,
   * settling time in microseconds) tuples.  An example with two
   * antenna ports, read power levels of 30 dBm, write power levels of
   * 25 dBm, and 500us settling times:
   * {{1, 3000, 2500, 500}, {2, 3000, 2500, 500}}
   */ 
  public void cmdSetAntennaPortPowersAndSettlingTime(int[][] list)
    throws ReaderException
  {
    for (int i = 0 ; i < list.length; i++) 
    {
      if (list[i][0] < 0 || list[i][0] > 255)
      {
        throw new IllegalArgumentException("illegal tx port " + list[i][0]);
      }
      if (list[i][1] < -32768 || list[i][1] > 32767)
      {
        throw new IllegalArgumentException("illegal read tx power " + list[i][1]);
      }
      if (list[i][2] < -32768 || list[i][2] > 32767)
      {
        throw new IllegalArgumentException("illegal write tx power " + list[i][2]);
      }
      if (list[i][3] < 0 || list[i][3] > 65535)
      {
        throw new IllegalArgumentException("illegal settling time " + list[i][3]);
      }
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_SET_ANTENNA_PORT);
    m.setu8(4);
    for (int i=0; i < list.length; i++)
    {
      m.setu8(list[i][0]);
      m.setu16(list[i][1]);
      m.setu16(list[i][2]);
      m.setu16(list[i][3]);
    }
    send(m);
  }

  /**
   * Set the current global Tx power setting for read operations.
   *
   * @deprecated
   * @param centidbm the power level
   */
  public void cmdSetReadTxPower(int centidbm)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_TX_READ_POWER);
    if ((centidbm < -32768 || centidbm > 32767))
    {
       throw new IllegalArgumentException("illegal read tx power " + centidbm);
    }
    m.setu16(centidbm);
    send(m);
  }

  static int tagMetadataSetValue(Set<TagMetadataFlag> flags)
  {
    int value = 0;
    for (TagMetadataFlag flag : flags)
    {
      value |= tagMetadataFlagValues.get(flag);
    }
    return value;
  }


  // Cache the most recent bits->set mapping - a connection
  // to one module will almost always return the same bits.
  private int lastMetadataBits;
  private EnumSet<TagMetadataFlag> lastMetadataFlags;

  Set<TagMetadataFlag> tagMetadataSet(int bits)
  {

    if (bits == lastMetadataBits)
    {
      return lastMetadataFlags.clone();
    }

    EnumSet<TagMetadataFlag> metadataFlags = 
      EnumSet.noneOf(TagMetadataFlag.class);

    for (TagMetadataFlag f : TagMetadataFlag.values())
    {
      if (0 != (tagMetadataFlagValues.get(f) & bits))
      {
        metadataFlags.add(f);
      }
    }

    lastMetadataBits = bits;
    lastMetadataFlags = metadataFlags;

    return metadataFlags;
  }

  /**
   * Set the current RFID protocol for the device to use.
   *
   * 
   * @param protocol the protocol to use
   */ 
  private void cmdSetProtocol(TagProtocol protocol)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_TAG_PROTOCOL);
    m.setu16(protocolToCodeMap.get(protocol));
    send(m);
  }

  /**
   * Set the current global Tx power setting for write operations.
   *
   * @deprecated
   * @param centidbm the power level.
   */
  public void cmdSetWriteTxPower(int centidbm)
    throws ReaderException
  {        
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_TX_WRITE_POWER);
    if (((short)centidbm < -32768 || (short)centidbm > 32767))
    {
       throw new IllegalArgumentException("illegal write tx power " + centidbm);
    }
    m.setu16(centidbm);
    send(m);
  }
  
  /**
   * Set the frequency hop table.
   *
   * @deprecated
   * @param table A list of frequencies, in kHz. The list may be at
   * most 62 elements.
   */
  public void cmdSetFrequencyHopTable(int[] table)
    throws ReaderException
  {
    Message m = new Message();
    int i, off;

    m.setu8(MSG_OPCODE_SET_FREQ_HOP_TABLE);
    for (i = 0, off = 0; i < table.length; i++, off += 4)
    {
      m.setu32(table[i]);
    }
    send(m);
  }

  /**
   * Set the interval between frequency hops. The valid range for this
   * interval is region-dependent.
   *
   * 
   * @param hopTime the hop interval, in milliseconds
   */
  private void cmdSetFrequencyHopTime(int hopTime)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_FREQ_HOP_TABLE);
    m.setu8(1);
    m.setu32(hopTime);
    send(m);
  }
  /**
   * set protocol license key
   * @param key  license key
   * @return supported protocol bit mask
   * @throws ReaderException
   */
  public List cmdSetProtocolLicenseKey(byte[] key) throws ReaderException
  {
    List list = new Vector<TagProtocol>();
    Message m=new Message();
    m.setu8(0x9E);
    m.setu8(0x01);
    m.setbytes(key);
    Message msg = send(m);
    msg.getu8();
    int numProtocol=(msg.writeIndex-msg.readIndex)/2;
    for(int i=0;i<numProtocol ;i++)
    {
      list.add(codeToProtocolMap.get(msg.getu16()));
    }
    return list;
   }

  /**
   * Set the state of a single GPIO pin
   *  
   * @param gpio the gpio pin number
   * @param high whether to set the pin high
   */ 
  private void cmdSetGPIO(int gpio, boolean high)
    throws ReaderException
  {
    Message m = new Message();
    m.setu8(MSG_OPCODE_SET_USER_GPIO_OUTPUTS);
    m.setu8(gpio);
    m.setu8(high ? 1 : 0);
    send(m);
  }

  /**
   * Get direction of a single GPIO pin
   *
   * @deprecated
   * @param pin  GPIO pin number
   * @return true if output pin, false if input pin
   */
  public boolean cmdGetGPIODirection(int pin)
    throws ReaderException
  {
    boolean out;

    Message m = new Message();           
    m.setu8(MSG_OPCODE_SET_USER_GPIO_OUTPUTS);
    m.setu8(pin);
    send(m);
    out = (m.data[6] == 1);
    return out;
  }

  /**
   * Set direction of a single GPIO pin
   *
   * @deprecated
   * @param pin  GPIO pin number
   * @param out  true for output, false for input
   */
  public void cmdSetGPIODirection(int pin, boolean out)
    throws ReaderException
  {
    Message m = new Message();
    m.setu8(MSG_OPCODE_SET_USER_GPIO_OUTPUTS);
    m.setu8(1); // Option flag
    m.setu8(pin);
    m.setu8(out ? 1: 0);
    m.setu8(0); // New value if output
    send(m);
  }

  /** 
   * Get directions of all GPIO pins
   *
   * @deprecated
   * @param wantOut  false = get inputs, true = get outputs 
   * @return list of pins that are set in the requested direction
   */ 
  public int[] getGPIODirection(boolean wantOut)
    throws ReaderException
  {
    int[] retval;
    int gpioPinsLength = 0;
    ArrayList pinList = new ArrayList();
    if (M6E_FAMILY_LIST.contains(model.toUpperCase()))
    {
      gpioPinsLength = cmdGetGPIO().length;
      if ((byte)0xFF == gpioDirections)
      {
        /* Cache the current state */
        gpioDirections = 0;     
        for (int pin = 1; pin <= gpioPinsLength ; pin++)
        {
          if (cmdGetGPIODirection(pin))
          {
            gpioDirections =(byte)( gpioDirections | (1 << pin));
          }
        }
      }
      for (int pin = 1; pin <= gpioPinsLength ; pin++)
      {
        boolean bitTest = ((gpioDirections >> pin & 1) == 1);
        if (wantOut == bitTest) 
        {
          pinList.add(new Integer(pin));
        }
      }
      retval = new int[pinList.size()];
      for (int i=0; i<pinList.size(); i++)
      {
        retval[i] = (Integer)pinList.get(i);
      }
    }
    else
    {
      retval = new int[] {1,2};
    }
    return retval;
  }

  /** 
   * Set directions of all GPIO pins
   *
   * @deprecated
   * @param wantOut  false = input, true = output 
   * @param pins GPIO pins to set to the desired direction.  All other pins implicitly set the other way.
  */ 
  public void setGPIODirection(boolean wantOut, int[] pins)
    throws ReaderException
  {
    byte newDirections;
    if (M6E_FAMILY_LIST.contains(model.toUpperCase()))
    {
      if (wantOut)
      {
        newDirections = 0;
      }
      else
      {
        newDirections = 0x1e;
      }

      for (int i = 0 ; i < pins.length ; i++)
      {
        int bit = 1 << pins[i];
        newDirections = (byte) (newDirections ^ bit); 
      }

      for (int pin = 0 ; pin < pins.length ; pin++)
      {
        int bit = 1 << pins[pin];
        boolean out = (newDirections & bit) != 0;
        cmdSetGPIODirection(pins[pin], out);
      }
      gpioDirections = newDirections;
    }
    else
    {
        throw new ReaderException("Parameter is read only.");
    }
  }

  /**
   * Set the current regulatory region for the device. Resets region-specific 
   * configuration, such as the frequency hop table.
   *
   * @param region the region to set
   */ 
  private void cmdSetRegion(Reader.Region region)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_REGION);
    m.setu8(regionToCodeMap.get(region));
    send(m);
  }

  public void cmdSetRegionLbt(Reader.Region region, boolean lbt)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_REGION);
    m.setu8(regionToCodeMap.get(region));
    m.setu8(lbt ? 1 : 0);
    send(m);
  }

  /**
   * Set the current power mode of the device.
   *
   * @deprecated
   * @param mode the mode to set
   */
  public void cmdSetPowerMode(PowerMode mode)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_POWER_MODE);
    m.setu8(mode.value);
    send(m);
  }

  /**
   * Set the current user mode of the device.
   *
   * @deprecated
   * @param mode the mode to set
   */
  public void cmdSetUserMode(UserMode mode)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_USER_MODE);
    m.setu8(mode.value);
    send(m);
  }

  /**
   * Sets the value of a device configuration setting.
   *
   * 
   * @param key the setting
   * @param value an object with the setting value. The type of the object
   * is dependant on the key; see the Configuration class for details.
   */
  private void cmdSetReaderConfiguration(SerialReader.Configuration key,
                                  Object value)
    throws ReaderException
  {
    Message m = new Message();
    int data;

    m.setu8(MSG_OPCODE_SET_READER_OPTIONAL_PARAMS);
    m.setu8(1);
    m.setu8(key.value);

    if (key == Configuration.ANTENNA_CONTROL_GPIO)
    {
        data = (Integer)value;
        m.setu8(data);
    }
    else if (key==Configuration.UNIQUE_BY_ANTENNA)
    {
      data = ((Boolean)value) ? 0 : 1;
      uniqueByAntenna = (Boolean)value;
      m.setu8(data);
    }
    else if (key == Configuration.UNIQUE_BY_DATA)
    {
      data = ((Boolean)value) ? 0 : 1;
      uniqueByData = (Boolean)value;
      m.setu8(data);
    }
    else if (key == Configuration.UNIQUE_BY_PROTOCOL)
    {
      data = ((Boolean)value) ? 0 : 1;
      uniqueByProtocol = (Boolean)value;
      m.setu8(data);
    }
    else if (key == Configuration.TAG_BUFFER_ENTRY_TIMEOUT)
    {
        data = (Integer)value;
        m.setu32(data);
    }
    else if (key == Configuration.ENABLE_FILTERING)
    {
        data = ((Boolean)value) ? 1 : 0;
        m.setu8(data);
    }
    else
    {
      data = ((Boolean)value) ? 1 : 0;
      m.setu8(data);
    }
    send(m);
  }

  /**
   * Sets the value of a protocol configuration setting.
   *
   * 
   * @param protocol the protocol of the setting
   * @param key the setting
   * @param value an object with the setting value. The type of the object
   * is dependant on the key; see the ProtocolConfiguration class for details.
   */
  private void cmdSetProtocolConfiguration(TagProtocol protocol,
                                   ProtocolConfiguration key,
                                   Object value)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_PROTOCOL_PARAM);
    m.setu8(protocolToCodeMap.get(protocol));
    m.setu8(key.getValue());

    if (protocol == TagProtocol.GEN2)
    {
      switch ((Gen2Configuration)key)
      {
      case SESSION:
        m.setu8(((Gen2.Session)value).rep);
        break;
      case TARGET:
        switch ((Gen2.Target)value)
        {
        case A:
          m.setu16(0x0100);
          break;
        case B:
          m.setu16(0x0101);
          break;
        case AB:
          m.setu16(0x0000);
          break;
        case BA:
          m.setu16(0x0001);
          break;
        }
        break;
      case TAGENCODING:
        m.setu8(((Gen2.TagEncoding)value).rep);
        break;
      case LINKFREQUENCY:
        switch (((Gen2.LinkFrequency)value).rep)
        {
        case 250:
          m.setu8(0);
          break;
        case 640:
          m.setu8(4);
          break;
        case 320:
          m.setu8(2);
          break;
        default:
            throw new IllegalArgumentException("Unsupported BLF " + value.toString());
        }
        break;
       case TARI:
        switch ((Gen2.Tari)value)
        {
        case TARI_25US:
          m.setu8(0);
          break;
        case TARI_12_5US:
          m.setu8(1);
          break;
        case TARI_6_25US:
          m.setu8(2);
          break;
        }
        break;
      case Q:
        if (value instanceof Gen2.DynamicQ)
        {
          m.setu8(0);
        }
        else if (value instanceof Gen2.StaticQ)
        {
          m.setu8(1);
          m.setu8(((Gen2.StaticQ)value).initialQ);
        }
        else
          throw new IllegalArgumentException("Unknown Q algorithm " + value.toString());
        break;
      case BAP:
          Gen2.Bap bap=(Gen2.Bap)value;
          m.setu8(0x01); //version
          m.setu16(0x03);// currently we supports only this enable bits.
          m.setu32(bap.powerUpDelayUs);
          m.setu32(bap.freqHopOfftimeUs);
          break;
      default:
        throw new IllegalArgumentException("Unknown " + protocol.toString() + 
                                           " parameter " + key.toString());
      }
    }
    else if (protocol == TagProtocol.ISO180006B)
    {
      switch ((ISO180006BConfiguration)key)
      {
      case LINKFREQUENCY:
        switch (((Iso180006b.LinkFrequency)value).rep)
        {
        case 160:
          m.setu8(0);
          break;
        case 40:
          m.setu8(1);
          break;
        default:
            throw new IllegalArgumentException("Unsupported BLF " + value.toString());

        }
        break;
      case MODULATIONDEPTH:
        switch (((Iso180006b.ModulationDepth)value))
        {
         case MODULATION99PERCENT:
          m.setu8(0);
          break;
         case MODULATION11PERCENT:
          m.setu8(1);
          break;
      default:
            throw new IllegalArgumentException("Unsupported ModulationDepth " + value.toString());
        }
        break;
      case DELIMITER:
         switch ((Iso180006b.Delimiter)value)
         {
         case DELIMITER1:
          m.setu8(1);
          break;
         case DELIMITER4:
          m.setu8(4);
          break;
         default:
            throw new IllegalArgumentException("Unsupported Delimiter " + value.toString());
        }
         break;
      default:
        throw new IllegalArgumentException("Unknown " + protocol.toString() + 
                                           " parameter " + key.toString());
      }

    }
    else
    {
      throw new IllegalArgumentException("Protocol parameters not supported for protocol " + protocol.toString());
    }

    send(m);
  }

    private void cmdSetGen2WriteResponseWaitTime(Object waitTime , Object WriteEarlyExit) throws ReaderException
    {
        Message m = new Message();

        //Form a message
        m.setu8(MSG_OPCODE_SET_PROTOCOL_PARAM);
        m.setu8(PROT_GEN2);
        m.setu8(MSG_OPCODE_WRITE_RESPONSE);

        //Get the value on the module before setting
        ArrayList response = cmdGetGen2WriteResponseWaitTime();

        /*
         * Add WriteEarlyExit value to message.
         * If WriteEarlyExit is null then get the value on the reader and add it.
         */
        if(WriteEarlyExit != null)
        {
            m.setu8((Boolean)WriteEarlyExit ? 0 : 1);
        }
        else
        {
            m.setu8((Boolean)response.get(0) ? 0 :1);
        }

        /*
         * Add waitTime value to message.
         * If waitTime is null then get the value on the reader and add it.
         */
        if(waitTime != null)
        {
            m.setu16((Integer)waitTime);
        }
        else
        {
            m.setu16((Integer)response.get(1));
        }

        //Send the message to the Reader
        send(m);
    }

    private ArrayList cmdGetGen2WriteResponseWaitTime() throws ReaderException
    {
        List aList = new ArrayList();
        Message m = new Message();

        //Form a message
        m.setu8(MSG_OPCODE_GET_PROTOCOL_PARAM);
        m.setu8(PROT_GEN2);
        m.setu8(MSG_OPCODE_WRITE_RESPONSE);

        //Send the message to the Reader
        send(m);
        m.readIndex += 2; // Skip protocol and key

        //Get the WriteEarlyExit value and add to the list
        int WriteEarlyExit = m.getu8();

        //Parse the WriteEarlyExit value
        boolean writeExit = (WriteEarlyExit == 1) ? false : true ;
        aList.add(writeExit);

        //Get the writeReplyTimeOut value and add to the list
        int writeReplyTimeOut = m.getu16();
        aList.add(writeReplyTimeOut);

        return (ArrayList) aList;
    }
  /**
   * Setting user profile on the basis of option,key and value parameter
   * @param option Save,restore,verify and reset configuration
   * @param key  Which part of configuration to operate on
   * @param val Type of configuration value to use (default, custom...)
   */
  
  public void cmdSetUserProfile(SetUserProfileOption option, ConfigKey key, ConfigValue val)
  {
      try{

      Message m = new Message();
      m.setu8(MSG_OPCODE_SET_USER_PROFILE);
      m.setu8(option.getValue());
      m.setu8(key.getValue());
      m.setu8(val.getValue());      
      send(m);
      if (option == SetUserProfileOption.RESTORE || option == SetUserProfileOption.CLEAR)
      {
        openPort();        
        currentProtocol = cmdGetProtocol();                
      }
      }catch(Exception e){
          e.printStackTrace();
      }
  }
  /**
   * get save/restore configuration
   * @param data Byte array consisting of opcode option
   * @return  Byte array
   */
   public byte[] cmdGetUserProfile(byte data[])
        {
            try{
        Message m=new Message();

        m.setu8(MSG_OPCODE_GET_USER_PROFILE);
        for(int i=0;i<data.length;i++){
            m.setu8(data[i]);
        }
        
        Message msg;
        msg=send(m);
        byte[] response;
        int resLen=(msg.writeIndex-msg.readIndex);
        response = new byte[resLen];
        for (int i = 0; i < resLen; i++)
        {
         response[i] = (byte) msg.getu8();
        }
        String str=ReaderUtil.byteArrayToHexString(response);
        
        return response;
        }catch(Exception e){
            e.printStackTrace();
            return null;
        }

        }


  /**
   * Reset the per-port statistics.
   *
   * @param stats the set of statistics to reset. Only the RF on time statistic
   * may be reset.
   */
  public void cmdResetReaderStatistics(Set<ReaderStatisticsFlag> stats)
    throws ReaderException
  {
    Message m = new Message();
    int flagBits;

    flagBits = 0;
    for (ReaderStatisticsFlag f : stats)
    {
      flagBits |= f.value;
    }

    m.setu8(MSG_OPCODE_GET_READER_STATS);
    m.setu8(READER_STATS_OPTION_RESET);
    m.setu8(flagBits);
    send(m);
  }
  
  /**
   * Reset the per-port stats.
   *
   * @param stats the set of stats to reset. Only the RF on time stats
   * may be reset.
   */
  public void cmdResetReaderStats(Set<ReaderStatsFlag> stats)
    throws ReaderException
  {
    Message m = new Message();
    int flagBits =0 ;

    flagBits = 0;
    for (ReaderStatsFlag f : stats)
    {
      flagBits |= f.value;
    }
    m.writeIndex  = 2; // skip option and flags

    m.setu8(MSG_OPCODE_GET_READER_STATS);
    m.setu8(READER_STATS_OPTION_RESET);
    
    if ((0x80) > flagBits)
    {
      m.setu8(flagBits);
    }
    else
    {
      int temp=flagBits & 0x7f;
      flagBits &= 0xFFFE;
      flagBits = ((flagBits << 1) | temp);
      flagBits &= 0xFF7F;
      m.setu16((0x8000) | flagBits);
    }
    m.data[1] =(byte) (m.writeIndex - 3);
    send(m);
  }

  /**
   * Set the operating frequency of the device.
   * Testing command.
   *
   * @param frequency the frequency to set, in kHz
   */ 
  public void cmdTestSetFrequency(int frequency)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_SET_OPERATING_FREQ);
    m.setu32(frequency);
    send(m);
  }


  /**
   * Turn CW transmission on or off.
   * Testing command.
   *
   * @param on whether to turn CW on or off
   */ 
  public void cmdTestSendCw(boolean on)
    throws ReaderException
  {
    Message m = new Message();

    m.setu8(MSG_OPCODE_TX_CW_SIGNAL);
    m.setu8(on ? 1 : 0);
    send(m);
  }


  /**
   * Turn on pseudo-random bit stream transmission for a particular
   * duration.  
   * Testing command.
   *
   * @param duration the duration to transmit the PRBS signal. Valid
   * range is 0-65535
   */ 
  public void cmdTestSendPrbs(int duration)
    throws ReaderException
  {
    if (duration < 0 || duration > 65535)
    {
      throw new IllegalArgumentException("illegal PRBS duration " + duration);
    }

    Message m = new Message();
    m.setu8(MSG_OPCODE_TX_CW_SIGNAL);
    m.setu8(2);
    m.setu16(duration);
    send(m);
  }

  /**
   * Send new message to tag with secure accesspassword corresponding with tag epc 
   * @param accessPassword secure accesspassword to read tag data
   * @throws ReaderException
   */
  private void cmdAuthReqResponse(Gen2.Password accessPassword ) throws ReaderException
    {
        Message m = new Message();
        //Form a message
        m.setu8(MSG_MAX_PACKET_LEN);
        m.writeIndex = 2;
        m.setu8(MSG_OPCODE_MULTI_PROTOCOL_TAG_OP);
        m.setu8(0x00);
        m.setu8(0x00);
        m.setu8(0x03);
        m.setu8(0x00);
        m.setu8(0x01);
        m.setu8(0x00);
       
        int password =  accessPassword.value;
        m.setu8(0x20);
        m.setu8((password >> 24));
        m.setu8((password >> 16));
        m.setu8((password >> 8));
        m.setu8((password >> 0));
        m.data[1] = (byte) (m.writeIndex - 3);/* Install length */
        sendMessage(0, m);
    }
  
  // package-visible (non-public) constructor
  SerialReader(String serialDevice, SerialTransport transport)
    throws ReaderException
  {
    this.serialDevice = serialDevice;    
    serialListeners = new Vector<TransportListener>();
    this.st = transport;
    initParams();
  }
  
SerialReader(String serialDevice) throws ReaderException
{
    this(serialDevice, new SerialTransportNative());
}



  public SerialReader(SerialTransport st)
    throws ReaderException
  {
    this.st = st;
    serialListeners = new Vector<TransportListener>();
    initParams();
  }

  public SerialTransport getSerialTransport()
  {
    return st;
  }

  void initParams()
  {
    addParam(TMR_PARAM_BAUDRATE,
             Integer.class, baudRate, true,
             new SettingAction()
             {
               public Object set(Object value)
                 throws ReaderException
               {
                 baudRate = (Integer)value;
                 /** 
                  *  some transport layer does not support baud rate settings.
                  *  for ex: TCP transport. In that case skip the baud rate settings.
                  **/
                 if (connected && (st.getBaudRate() != baudRate && st.getBaudRate()!=0))
                 {
                   cmdSetBaudRate(baudRate);
                   st.setBaudRate(baudRate);
                 }
                 return value;
                }
               public Object get(Object value)
               {
                 return baudRate;
               }
             });
        
    addParam(
      TMR_PARAM_POWERMODE,
               PowerMode.class, null, true,
               new SettingAction()
               {
                 public Object set(Object value)
                   throws ReaderException
                 {
                   cmdSetPowerMode((PowerMode)value);
                   powerMode = (PowerMode)value;
                   return value;
                 }
                 public Object get(Object value)
                   throws ReaderException
                 {
                   return cmdGetPowerMode();
                 }
               });
    addParam(TMR_PARAM_REGION_ID,
                Reader.Region.class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        if(connected)
                        {
                            cmdSetRegion((Reader.Region) value);
                        }
                        region = (Reader.Region)value;
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetRegion();
                    }
                });
  }

    public void connect() throws ReaderException
    {
        int tagopAntenna;
        if (connected)
        {
            return;
        }
        openPort();
        boot(region);

        addParam(TMR_PARAM_ANTENNA_PORTLIST,
                int[].class, null, false,
                new ReadOnlyAction()
                {

                    public Object get(Object value)
                    {
                        return getAllAntennas();
                    }
                });

        addParam(TMR_PARAM_GPIO_INPUTLIST,
                int[].class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        setGPIODirection(false, (int[]) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return getGPIODirection(false);
                    }
                });

        addParam(TMR_PARAM_GPIO_OUTPUTLIST,
                int[].class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        setGPIODirection(true, (int[]) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return getGPIODirection(true);
                    }
                });

        addParam(TMR_PARAM_ANTENNA_CONNECTEDPORTLIST,
                int[].class, null, false,
                new ReadOnlyAction()
                {

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return getConnectedAntennas();
                    }
                });

                addParam(TMR_PARAM_ANTENNA_RETURNLOSS,
                int[].class, null, false,
                new ReadOnlyAction()
                {

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return getAntennaReturnLoss();
                    }
                });

        addParam(TMR_PARAM_GEN2_WRITEMODE,
                Gen2.WriteMode.class, Gen2.WriteMode.WORD_ONLY, true, null);

        addParam(TMR_PARAM_RADIO_READPOWER,
                Integer.class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetReadTxPower((Integer) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetReadTxPower();
                    }
                });

        addParam(TMR_PARAM_RADIO_WRITEPOWER,
                Integer.class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetWriteTxPower((Integer) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetWriteTxPower();
                    }
                });

        addParam(TMR_PARAM_RADIO_ENABLEPOWERSAVE,
                Boolean.class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetReaderConfiguration(Configuration.TRANSMIT_POWER_SAVE, value);
                        return value;

                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        value = cmdGetReaderConfiguration(Configuration.TRANSMIT_POWER_SAVE);
                        return value;
                    }
                });

        addParam(TMR_PARAM_VERSION_SERIAL,
                String.class, null, true,
                new ReadOnlyAction()
                {

                    public Object get(Object value) throws ReaderException
                    {
                        try
                        {
                            return cmdGetSerialNumber(0x00, 0x40);
                        }
                        catch(ReaderException re)
                        {
                            return null;
                        }
                    }
                });
                
                
        class ProtocolConfigurationKeySettingAction implements SettingAction
        {

            TagProtocol protocol;
            ProtocolConfiguration key;

            ProtocolConfigurationKeySettingAction(TagProtocol p, ProtocolConfiguration k)
            {
                protocol = p;
                key = k;
            }

            public Object set(Object value)
                    throws ReaderException
            {
                cmdSetProtocolConfiguration(protocol, key, value);
                return value;
            }

            public Object get(Object value)
                    throws ReaderException {
                return cmdGetProtocolConfiguration(protocol, key);
            }
        }

        addParam(TMR_PARAM_GEN2_SESSION,
                Gen2.Session.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.GEN2, Gen2Configuration.SESSION));

        addUnconfirmedParam(TMR_PARAM_GEN2_TAGENCODING,
                Gen2.TagEncoding.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.GEN2, Gen2Configuration.TAGENCODING));

        addUnconfirmedParam(TMR_PARAM_GEN2_Q,
                Gen2.Q.class, null, true,
                new SettingAction()
                {
                    public Object set(Object value)
                            throws ReaderException
                    {
                        if(value instanceof Gen2.StaticQ)
                        {
                            Gen2.StaticQ q = ((Gen2.StaticQ) value);
                            if (q.initialQ < 0 || q.initialQ > 15) {
                                throw new IllegalArgumentException("Value of /reader/gen2/q is out of range. Should be between 0 and 15");
                            }
                        }
                        cmdSetProtocolConfiguration(TagProtocol.GEN2, Gen2Configuration.Q, value);
                        return value;
                    }
                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetProtocolConfiguration(TagProtocol.GEN2, Gen2Configuration.Q);
                    }
                });

                addParam(TMR_PARAM_GEN2_BAP,
                Gen2.Bap.class, null, true,
                new SettingAction()
                {
                    public Object set(Object value)
                            throws ReaderException
                    {
                        if(value instanceof Gen2.Bap)
                        {
                            Gen2.Bap bap = ((Gen2.Bap) value);
                            if(bap == null)
                            {
                                /** It means user is disabling the bap support, make the flag down 
                                 * and skip the command sending
                                 **/ 
                                isBapEnabled = false;
                                return value;
                            }
                            else if( bap.powerUpDelayUs < -1 || bap.freqHopOfftimeUs <  -1)
                            {
                                /*
                                 * Invalid values for BAP parameters,
                                 * Accepts only positive values or -1 for NULL
                                 */
                                isBapEnabled = false;
                                throw new ReaderException("Invalid values for BAP parameters");
                            }
                            else
                            {
                                /*
                                 * do a paramGet of BAP parameters. This serves two purposes.
                                 * 1. API knows wheather module supports the BAP options or not.
                                 * 2. API can assign default values to the fields, are not set by the user.
                                 */
                                Gen2.Bap getBapParams=(Gen2.Bap) cmdGetProtocolConfiguration(TagProtocol.GEN2, Gen2Configuration.BAP);
                                if(bap.powerUpDelayUs == -1)
                                {
                                    bap.powerUpDelayUs = getBapParams.powerUpDelayUs;
                                }
                                if(bap.freqHopOfftimeUs == -1)
                                {
                                    bap.freqHopOfftimeUs = getBapParams.freqHopOfftimeUs;
                                }
                               isBapEnabled = true;
                            }
                            cmdSetProtocolConfiguration(TagProtocol.GEN2, Gen2Configuration.BAP, bap);
                        }
                         return value;
                    }
                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetProtocolConfiguration(TagProtocol.GEN2, Gen2Configuration.BAP);
                    }
                });


        addUnconfirmedParam(TMR_PARAM_GEN2_BLF,
                Gen2.LinkFrequency.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.GEN2, Gen2Configuration.LINKFREQUENCY));

        addUnconfirmedParam(TMR_PARAM_GEN2_TARI,
                Gen2.Tari.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.GEN2, Gen2Configuration.TARI));


        addUnconfirmedParam(TMR_PARAM_ISO180006B_BLF,
                Iso180006b.LinkFrequency.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.ISO180006B,
                ISO180006BConfiguration.LINKFREQUENCY));

        addUnconfirmedParam(TMR_PARAM_ISO180006B_MODULATION_DEPTH,
                Iso180006b.ModulationDepth.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.ISO180006B,
                ISO180006BConfiguration.MODULATIONDEPTH));

        addUnconfirmedParam(TMR_PARAM_ISO180006B_DELIMITER,
                Iso180006b.Delimiter.class, null, true,
                new ProtocolConfigurationKeySettingAction(
                TagProtocol.ISO180006B,
                ISO180006BConfiguration.DELIMITER));

        addParam(TMR_PARAM_REGION_LBT_ENABLE,
                Boolean.class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        int[] hopTable = (int[]) paramGet(TMR_PARAM_REGION_HOPTABLE);
                        int hopTime = (Integer) paramGet(TMR_PARAM_REGION_HOPTIME);
                        try
                        {
                            cmdSetRegionLbt(region, (Boolean) value);
                        }
                        catch (ReaderCodeException re)
                        {
                            throw new IllegalArgumentException(
                                    "LBT may not be set in this region");
                        }
                        paramSet(TMR_PARAM_REGION_HOPTABLE, hopTable);
                        paramSet(TMR_PARAM_REGION_HOPTIME, hopTime);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        try
                        {
                            return cmdGetRegionConfiguration(
                                    RegionConfiguration.LBTENABLED);
                        }
                        catch (ReaderCodeException re)
                        {
                            // In regions that don't support LBT, the command doesn't even work.
                            return false;
                        }
                    }
                });

        addParam(TMR_PARAM_REGION_SUPPORTEDREGIONS,
                Reader.Region[].class, null, false,
                new ReadOnlyAction()
                {

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetAvailableRegions();
                    }
                });
        
        addParam(TMR_PARAM_REGION_HOPTABLE,
                int[].class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetFrequencyHopTable((int[]) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetFrequencyHopTable();
                    }
                });

        addParam(TMR_PARAM_REGION_HOPTIME,
                Integer.class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetFrequencyHopTime((Integer) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetFrequencyHopTime();
                    }
                });

        addParam(TMR_PARAM_READER_STATISTICS,
                ReaderStatisticsFlag.class, null, true,
                new SettingAction()
                {
                    public Object set(Object value)
                            throws ReaderException
                    {
                        HashSet<ReaderStatisticsFlag> flagSet = new HashSet<ReaderStatisticsFlag>();
                        flagSet.add(ReaderStatisticsFlag.RF_ON_TIME);                        
                        cmdResetReaderStatistics(flagSet);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        HashSet<ReaderStatisticsFlag> flagSet = new HashSet<ReaderStatisticsFlag>();
                        flagSet.add(ReaderStatisticsFlag.RF_ON_TIME);
                        flagSet.add(ReaderStatisticsFlag.NOISE_FLOOR_TX_ON);
                        flagSet.add(ReaderStatisticsFlag.NOISE_FLOOR);
                        return cmdGetReaderStatistics(flagSet);
                    }
                });
				
		addParam(TMR_PARAM_READER_STATS_ENABLE,
                ReaderStatsFlag[].class, null, true,
                new SettingAction()
                {                   
                    public Object set(Object value)
                            throws ReaderException
                    {
                        if(!statsSupportedFlags)
                        {
                             throw new ReaderException("Operation not supported");
                        }
                        statsEnabledFlags=(ReaderStatsFlag[])value;
                        for (ReaderStatsFlag rs : statsEnabledFlags) {
                           statsFlags |= rs.value;
                        }
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        if(!statsSupportedFlags)
                        {
                             throw new ReaderException("Operation not supported");
                        }
                        return statsEnabledFlags;
                    }
                });

        addParam(TMR_PARAM_READER_STATS,
                ReaderStatsFlag[].class, null, true,
                new SettingAction()
                {
                    public Object set(Object value)
                            throws ReaderException
                    {
                       throw new ReaderException("Operation not supported");
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        if(!statsSupportedFlags)
                        {
                             throw new ReaderException("Operation not supported");
                        }
                        HashSet<ReaderStatsFlag> flagSet = new HashSet<ReaderStatsFlag>();
                        flagSet.addAll(Arrays.asList(ReaderStatsFlag.values()));
                        return cmdGetReaderStats(flagSet);
                    }
                });		

        addParam(TMR_PARAM_READ_PLAN,
                ReadPlan.class, new SimpleReadPlan(), true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        if (value instanceof SimpleReadPlan)
                        {
                            SimpleReadPlan srp = (SimpleReadPlan) value;
                            checkStopTriggerValue(srp);
                        }
                        else if (value instanceof MultiReadPlan)
                        {
                            MultiReadPlan mrp = (MultiReadPlan) value;
                            for (ReadPlan plan : mrp.plans)
                            {
                                if (plan instanceof SimpleReadPlan) 
                                {
                                    SimpleReadPlan srp = (SimpleReadPlan) plan;
                                    checkStopTriggerValue(srp);
                                }
                            }
                        }
                        return value;
                    }
                    public void checkStopTriggerValue(SimpleReadPlan srp)
                    {
                        if (srp instanceof StopTriggerReadPlan) 
                        {
                            StopTriggerReadPlan strp = (StopTriggerReadPlan) srp;
                            if (strp.stopOnCount instanceof StopOnTagCount)
                            {
                                StopOnTagCount sotc = (StopOnTagCount) strp.stopOnCount;
                                if (sotc.N < 0)
                                {
                                    throw new IllegalArgumentException(
                                            "Value of N is out of range. Should be > 0");
                                }
                            }
                        }
                        if (!protocolSet.contains(srp.protocol))
                        {
                            throw new IllegalArgumentException(
                                    "Unsupported protocol " + srp.protocol + ".");
                        }                  
                    }
                    public Object get(Object value)
                            throws ReaderException
                    {
                        return value;
                    }
                });

        addParam(TMR_PARAM_TAGOP_PROTOCOL,
                TagProtocol.class, TagProtocol.GEN2, true,
                new SettingAction()
                {

                    public Object set(Object value) throws ReaderException
                    {
                        TagProtocol p = (TagProtocol) value;

                        if (!protocolSet.contains(p))
                        {
                            throw new IllegalArgumentException(
                                    "Unsupported protocol " + p + ".");
                        }
                        setProtocol(p);
                        return value;
                    }

                    public Object get(Object value)
                    {
                        return currentProtocol;
                    }
                });
        
        int[] connectedPorts = getConnectedPorts(false);
        if (connectedPorts.length > 0)
        {
            tagopAntenna = connectedPorts[0];
        }
        else
        {
            tagopAntenna = 0;
        }
        addParam(TMR_PARAM_TAGOP_ANTENNA,
                Integer.class, tagopAntenna, true,
                new SettingAction()
                {

                    public Object set(Object value) throws ReaderException
                    {
                        int i, a = (Integer) value;
                        for (i = 0; i < txrxPorts.length; i++)
                        {
                            if (a == txrxPorts[i][0])
                            {
                                break;
                            }
                        }
                        if (i == txrxPorts.length)
                        {
                            throw new IllegalArgumentException(
                                    "Invalid antenna " + a + ".");
                        }
                      setCurrentAntenna(a);
                        return value;
                    }

                    public Object get(Object value)
                    {
                        return value;
                    }
                });

        /**
         * Inner Class ConfigurationKeySettingAction
         */
        class ConfigurationKeySettingAction implements SettingAction
        {

            Configuration key;

            ConfigurationKeySettingAction(Configuration k)
            {
                key = k;
            }

            public Object set(Object value)
                    throws ReaderException
            {
                cmdSetReaderConfiguration(key, value);
                return value;
            }

            public Object get(Object value)
                    throws ReaderException
            {
                return cmdGetReaderConfiguration(key);
            }
        }

        addUnconfirmedParam(TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA,
                Boolean.class, false, true,
                new ConfigurationKeySettingAction(
                Configuration.UNIQUE_BY_ANTENNA));

        addUnconfirmedParam(TMR_PARAM_TAGREADDATA_UNIQUEBYDATA,
                Boolean.class, false, true,
                new ConfigurationKeySettingAction(
                Configuration.UNIQUE_BY_DATA));

        addUnconfirmedParam(TMR_PARAM_ANTENNA_CHECKPORT,
                Boolean.class, false, true,
                new ConfigurationKeySettingAction(
                Configuration.SAFETY_ANTENNA_CHECK));
        addUnconfirmedParam(TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI,
                Boolean.class, false, true,
                new ConfigurationKeySettingAction(
                Configuration.RECORD_HIGHEST_RSSI));
        addUnconfirmedParam(TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM,
                Boolean.class, true, true,
                new ConfigurationKeySettingAction(
                Configuration.RSSI_IN_DBM));                               

        class PortParamSettingAction implements SettingAction
        {

            int paramColumn;

            PortParamSettingAction(int paramColumn)
            {
                this.paramColumn = paramColumn;
            }

            public Object set(Object value)
                    throws ReaderException
            {
                int[][] rpList = (int[][]) value;
                setArrayColumn(portParamList, paramColumn, 0);
                setArrayColumnByPort(portParamList, paramColumn, rpList);
                cmdSetAntennaPortPowersAndSettlingTime(portParamList);
                return value;
            }

            public Object get(Object value)
                    throws ReaderException
            {
                portParamList = cmdGetAntennaPortPowersAndSettlingTime();
                return extractNonzeroArrayColumn(portParamList, paramColumn);
            }

            void setArrayColumn(int[][] array, int column, int value)
            {
                for (int[] row : array)
                {
                    row[column] = value;
                }
            }

            void setArrayColumnByPort(int[][] array, int column, int[][] pairs)
            {
                boolean found;
                for (int[] pair : pairs)
                {
                    found = false;
                    for (int[] row : array)
                    {
                        if (pair[0] == row[0])
                        {
                            row[column] = pair[1];
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        throw new IllegalArgumentException(
                                "Invalid port number " + pair[0]);
                    }
                }
            }

            int[][] extractNonzeroArrayColumn(int[][] array, int column)
            {
                List<int[]> returnList = new Vector<int[]>();
                for (int[] row : array)
                {                   
                    if (model.equalsIgnoreCase(TMR_READER_M6E_MICRO))
                {
                        if (row[column] != -32768)
                    {
                        returnList.add(new int[]{row[0], row[column]});
                    }
                }
                    else if (row[column] != 0)
                    {
                        returnList.add(new int[]{row[0], row[column]});
                    }
                }
                return returnList.toArray(new int[returnList.size()][]);
            }
        }

        addUnconfirmedParam(TMR_PARAM_RADIO_PORTREADPOWERLIST,
                int[][].class, null, true,
                new PortParamSettingAction(1));
        addUnconfirmedParam(TMR_PARAM_RADIO_PORTWRITEPOWERLIST,
                int[][].class, null, true,
                new PortParamSettingAction(2));
        addUnconfirmedParam(TMR_PARAM_ANTENNA_SETTLINGTIMELIST,
                int[][].class, null, true,
                new PortParamSettingAction(3));

        addUnconfirmedParam(
                TMR_PARAM_GEN2_TARGET,
                Gen2.Target.class, null, true,
                new SettingAction() {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetProtocolConfiguration(TagProtocol.GEN2,
                                Gen2Configuration.TARGET,
                                value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetProtocolConfiguration(TagProtocol.GEN2,
                                Gen2Configuration.TARGET);

                    }
                });


        addUnconfirmedParam(
                TMR_PARAM_RADIO_TEMPERATURE,
                Integer.class, null, false,
                new ReadOnlyAction()
                {

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetTemperature();
                    }
                });

        addParam(TMR_PARAM_READER_TAGOP_SUCCESSES, Integer.class, 0, false,
                new ReadOnlyAction()
                {
                    @Override
                    public Object get(Object val) throws ReaderException
                    {
                        return tagOpSuccessCount;
                    }
                });

       addParam(TMR_PARAM_READER_TAGOP_FAILURES, Integer.class, 0, false,
                new ReadOnlyAction()
                {
                    @Override
                    public Object get(Object val) throws ReaderException
                    {
                        return tagOpFailuresCount;
                    }
                });
        
      addParam(TMR_PARAM_READER_WRITE_EARLY_EXIT, Boolean.class , null ,true,
                    new SettingAction() {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetGen2WriteResponseWaitTime(null,value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                       ArrayList response = cmdGetGen2WriteResponseWaitTime();
                       return response.get(0);
                    }
                 });
            addParam(TMR_PARAM_READER_WRITE_REPLY_TIMEOUT,Integer.class , null,true,
                    new SettingAction() {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        int waitTime = (Integer)value;
                        if(waitTime <= 21000 &&  waitTime >= 1000)
                        {
                            cmdSetGen2WriteResponseWaitTime(value,null);
                        }
                        else
                        {
                            throw new IllegalArgumentException("Value out of range :" + waitTime);
                        }
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                      ArrayList response = cmdGetGen2WriteResponseWaitTime();
                      return response.get(1);
                    }
                 });
            
        // M6e module specific params
        if (M6E_FAMILY_LIST.contains(model.toUpperCase()))
        {
            addParam(TMR_PARAM_LICENSE_KEY, byte[].class, null, true,
                    new SettingAction()
                    {

                        public Object set(Object value) throws ReaderException
                        {
                            cmdSetProtocolLicenseKey((byte[]) value);
                            return value;
                        }

                        public Object get(Object value)
                        {
                            return null;
                        }
                    });

            addParam(TMR_PARAM_USER_CONFIG, UserConfigOp.class, null, true,
                    new SettingAction() {

                        public Object set(Object value) throws ReaderException
                        {
                            cmdSetUserProfile(((SerialReader.UserConfigOp) value).Opcode, ConfigKey.ALL, ConfigValue.CUSTOM_CONFIGURATION);
                            return value;
                        }

                        public Object get(Object value)
                        {
                            return null;
                        }
                    });

            addParam(TMR_PARAM_ENABLE_SJC, Boolean.class, true, true,
                    new SettingAction()
                    {

                        public Object set(Object value) throws ReaderException
                        {
                            cmdSetReaderConfiguration(Configuration.SELF_JAMMER_CANCELLATION, value);
                            return value;
                        }

                        public Object get(Object value) throws ReaderException
                        {
                            return cmdGetReaderConfiguration(Configuration.SELF_JAMMER_CANCELLATION);
                        }
                    });

            addParam(TMR_PARAM_READ_FILTER_TIMEOUT, Integer.class, 0, true,
                    new SettingAction()
                    {
                        public Object set(Object value) throws ReaderException
                        {
                            int moduleValue = (Integer)((DEFAULT_READ_FILTER_TIMEOUT == (Integer)value) ? 0 : value);
                            cmdSetReaderConfiguration(Configuration.TAG_BUFFER_ENTRY_TIMEOUT, moduleValue);
                            _readFilterTimeout = moduleValue;
                            return value;
                        }

                        public Object get(Object value) throws ReaderException
                        {
                            return _readFilterTimeout;                            
                        }
                    });

            addParam(TMR_PARAM_ENABLE_READ_FILTERING, Boolean.class, true, true,
                    new SettingAction()
                    {
                        public Object set(Object value) throws ReaderException
                        {
                            cmdSetReaderConfiguration(Configuration.ENABLE_FILTERING, value);
                            _enableFiltering = (Boolean)value;
                            return value;
                        }

                        public Object get(Object value) throws ReaderException
                        {
                            return _enableFiltering;
                        }
                    });

            addParam(TMR_PARAM_READER_STATUS_ANTENNA, Boolean.class, false, true,
                    new SettingAction()
                    {
                        public Object set(Object value) throws ReaderException
                        {
                            antennaStatusEnable = (Boolean) value;
                            return value;
                        }

                        public Object get(Object value) throws ReaderException
                        {
                            return antennaStatusEnable;
                        }
                    });

            addParam(TMR_PARAM_READER_STATUS_FREQUENCY, Boolean.class, false, true,
                    new SettingAction()
                    {
                        public Object set(Object value) throws ReaderException
                        {
                            frequencyStatusEnable = (Boolean) value;
                            return value;
                        }

                        public Object get(Object value) throws ReaderException
                        {
                            return frequencyStatusEnable;
                        }
                    });

            addParam(TMR_PARAM_READER_STATUS_TEMPERATURE, Boolean.class, false, true,
                    new SettingAction() {

                        public Object set(Object value) throws ReaderException
                        {
                            temperatureStatusEnable = (Boolean) value;
                            return value;
                        }

                        public Object get(Object value) throws ReaderException
                        {
                            return temperatureStatusEnable;
                        }
                    });
                    
            addUnconfirmedParam(TMR_PARAM_TAGREADDATA_UNIQUEBYPROTOCOL,
                Boolean.class, false, true,
                new ConfigurationKeySettingAction(
                Configuration.UNIQUE_BY_PROTOCOL));

             // initializing enableFiltering at the boot time
             _enableFiltering = (Boolean)cmdGetReaderConfiguration(Configuration.ENABLE_FILTERING);
             uniqueByProtocol = (Boolean)cmdGetReaderConfiguration(Configuration.UNIQUE_BY_PROTOCOL);
             // initializing readFilterTimeout
             int timeout = (Integer)cmdGetReaderConfiguration(Configuration.TAG_BUFFER_ENTRY_TIMEOUT);
             _readFilterTimeout = (timeout == 0) ? DEFAULT_READ_FILTER_TIMEOUT : timeout;
        }
        else if (model.equals(TMR_READER_M5E)) //model is M5e
        {
            addParam(TMR_PARAM_READER_EXTENDED_EPC, Boolean.class, null, true,
                    new SettingAction()
                    {
                        public Object get(Object val) throws ReaderException
                        {
                            return cmdGetReaderConfiguration(Configuration.EXTENDED_EPC);
                        }

                        public Object set(Object val) throws ReaderException
                        {
                            extendedEPC = true;
                            cmdSetReaderConfiguration(Configuration.EXTENDED_EPC, val);
                            return val;
                        }
                    });

             paramSet(TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM, true);

             addParam(TMR_PARAM_ENABLE_READ_FILTERING, Boolean.class, true, false,
                    new ReadOnlyAction()
                    {                        
                        @Override
                        public Object get(Object value) throws ReaderException
                        {
                            // filtering is always enabled on moduel for M5e, so returning true.
                            return true;
                        }
                    });
              /*
               * Setting extended epc flag as true, to allow the M5e to read tags with EPC > 96 bits
               */
              cmdSetReaderConfiguration(Configuration.EXTENDED_EPC, true);
              extendedEPC = true;
        }
        uniqueByAntenna = (Boolean)cmdGetReaderConfiguration(Configuration.UNIQUE_BY_ANTENNA);
        uniqueByData = (Boolean)cmdGetReaderConfiguration(Configuration.UNIQUE_BY_DATA);
        connected = true;
    }//end of connect() method

 
   public void reboot() throws ReaderException
   {
         /** XXX drop down to 9600 before going into the bootloader. Some
          *  XXX versions of M5e preserve the baud rate, and some go back to 9600;
          *  XXX this way we know what the speed is either way.
          *  some transport layer does not support baud rate settings.
          *  for ex: TCP transport. In that case skip the baud rate settings.
          **/
       if(st.getBaudRate() != 0)
       {
          cmdSetBaudRate(9600);
          st.setBaudRate(9600);
       }
       
        try
        {
            cmdBootBootloader();
        }
        catch (ReaderCodeException ex)
        {
            // Invalid Opcode (101h) okay -- means "already in bootloader"
            if (0x101 != ex.getCode())
            {
                // Other errors are real
                throw ex;
            }
        }
   }
    
    private int[][] makeDefaultTxRxMap() throws ReaderException
    {
        ArrayList<int[]> map = new ArrayList<int[]>();
        // Modify TxRxMap according to reader product
        Integer pgid = (Integer) paramGet(TMR_PARAM_READER_PRODUCTGROUPID);
        switch (pgid)
        {
            default:
                // Module
            case 0x0002:
                // USB Reader
                // Default map okay -- M5e-C only has 1 antenna port, anyway
                for (int port : ports)
                {
                    map.add(new int[] {port, port, port});
                }
                break;

            case 0x0001:
                // Ruggedized Reader (Tool Link, Vega)
                map.add(new int[] {1,2,2});
                map.add(new int[] {2,5,5});
                map.add(new int[] {3,1,1});
                break;
        }
        return map.toArray(new int[][]{{1,1,1}});
    }

    private void setTxRxMap(int[][] map)
    {
        _txrxMap  = map;
        setPortMaps(_txrxMap);
    }
    private void setPortMaps(int[][] value)
    {
        Map<Integer,int[]> new_antennaPortMap = new HashMap<Integer, int[]>();
        Map<Integer,Integer> new_antennaPortReverseMap = new HashMap<Integer,Integer>();
        Map<Integer,Integer> new_antennaPortTransmitMap = new HashMap<Integer,Integer>();
        
        for (int[] triple : value)
        {
            if (!portSet.contains(triple[1]))
            {
                throw new IllegalArgumentException(
                        "Invalid port number " + triple[1]);
            }
            if (!portSet.contains(triple[2]))
            {
                throw new IllegalArgumentException(
                        "Invalid port number " + triple[2]);
            }
            if (new_antennaPortMap.containsKey(triple[0]))
            {
                throw new IllegalArgumentException(
                        "Duplicate entries for antenna " + triple[0]);
            }
            new_antennaPortMap.put(triple[0],
                    new int[]{triple[1], triple[2]});
            new_antennaPortReverseMap.put(((triple[1] & 0xF) << 4) | (triple[2] & 0xF),
                    triple[0]);
            new_antennaPortTransmitMap.put(triple[1], triple[0]);

            // Mapping may have changed, invalidate current-antenna cache
            currentAntenna = 0;
            searchList = null;
        }

        antennaPortMap.clear();
        antennaPortReverseMap.clear();
        antennaPortTransmitMap.clear();

        // updating port map
        antennaPortMap.putAll(new_antennaPortMap);
        antennaPortReverseMap.putAll(new_antennaPortReverseMap);
        antennaPortTransmitMap.putAll(new_antennaPortTransmitMap);
    }
    private void configureForProductGroup() throws ReaderException
    {
        ports = getAntennaPorts();
        portSet = new HashSet<Integer>();
        for (int p : ports)
        {
            portSet.add(p);
        }
        txrxPorts = makeDefaultTxRxMap();
        addUnconfirmedParam(
                TMR_PARAM_ANTENNA_PORTSWITCHGPOS,
                int[].class, null, true,
                new SettingAction()
                {

                    public Object set(Object value)
                            throws ReaderException
                    {
                        int val;
                        int[] list = (int[]) value;
                        if (list.length == 0)
                        {
                            val = 0;
                            return val;
                        }
                        else if (list.length == 1 && list[0] == 1)
                        {
                            val = 1;
                        }
                        else if (list.length == 1 && list[0] == 2)
                        {
                            val = 2;
                        }
                        else if (list.length == 2 && list[0] == 1
                                && list[1] == 2)
                        {
                            val = 3;
                        }
                        else
                        {
                            throw new IllegalArgumentException(
                                    "Illegal set of GPOs for antenna switch control");
                        }
                        cmdSetReaderConfiguration(
                                Configuration.ANTENNA_CONTROL_GPIO,
                                (Integer) val);
                        ports = getAntennaPorts();
                        portSet = new HashSet<Integer>();
                        for (int p : ports)
                        {
                            portSet.add(p);
                        }
                        // Update txRxMap
                        int[][] newmap = new int[ports.length][3];
                        for (int p = 0; p < ports.length; p++)
                        {
                            for (int i = 0; i < 3; i++)
                            {
                                newmap[p][i] = ports[p];
                            }
                        }
                        // XXX check/invalidate current readPlan, tagopAntenna?                     
                        setTxRxMap(newmap);
                        
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        int val;
                        int[] rv;
                        val = (Integer) cmdGetReaderConfiguration(
                                Configuration.ANTENNA_CONTROL_GPIO);
                        if (val == 0)
                        {
                            rv = new int[]{};
                        }
                        else if (val == 1)
                        {
                            rv = new int[]{1};
                        }
                        else if (val == 2)
                        {
                            rv = new int[]{2};
                        }
                        else if (val == 3)
                        {
                            rv = new int[]{1, 2};
                        }
                        else
                        {
                            throw new ReaderException("Unknown response "
                                    + "to config request");
                        }
                        return rv;
                    }
                });

        switch ((Integer)paramGet(TMR_PARAM_READER_PRODUCTGROUPID))
        {
            case 1:
                //ruggedized reader
                paramSet(TMR_PARAM_ANTENNA_PORTSWITCHGPOS, new int[]{1});
                break;
        }

        setTxRxMap(makeDefaultTxRxMap());
        addParam(TMR_PARAM_ANTENNA_TXRXMAP,
                int[][].class, null, true,
                new SettingAction()
                {
                    public Object set(Object value)
                            throws ReaderException
                    {
                        setTxRxMap((int[][])value);
                        return antennaPortMap;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        List<int[]> rvList = new Vector<int[]>();
                        for (int p : antennaPortMap.keySet())
                        {
                            int[] pair = antennaPortMap.get(p);
                            rvList.add(new int[]{p, pair[0], pair[1]});
                        }

                        return rvList.toArray(new int[rvList.size()][]);
                    }
                });
    }//end of method configureForProductGroup()

    /**
     * Configure boot level params.
     * @param region
     * @throws ReaderException
     */
    void boot(Region region)
            throws ReaderException
    {
        int program;
        powerMode = PowerMode.INVALID;

        try
        {
            program = cmdGetCurrentProgram();
            if ((program & 0x3) == 1)
            {
                versionInfo = cmdBootFirmware();
                protocolSet.clear();
                protocolSet.addAll(Arrays.asList(versionInfo.protocols));
            }            
            else if ((program & 0x3) != 2)
            {
                throw new ReaderException("Unknown current program 0x"
                        + Integer.toHexString(program));
            }
        }
        catch (ReaderCodeException re)
        {
            // The reader is probably in the M4e bootloader.
            try
            {
                cmdBootFirmware();
            }
            catch (ReaderCodeException re2)
            {
                throw  new ReaderException("Application image failed CRC check");
            }
        }
        // Read and set the power mode ASAP
        // so that it can be referenced by sendMessage
        powerMode = cmdGetPowerMode();

        paramSet(TMR_PARAM_BAUDRATE, baudRate);

        int type = (Integer) cmdGetReaderConfiguration(Configuration.CURRENT_MSG_TRANSPORT);
        TransportType transportValue = TransportType.get(type);
        if(M6E_FAMILY_LIST.contains(model.toUpperCase()))
        {
            if (transportValue == TransportType.SOURCEUSB)
            {
                try
                {
                    cmdSetReaderConfiguration(Configuration.SEND_CRC, false);
                    isCRCEnabled = false;
                }
                catch(ReaderException re)
                {
                    //Ignoring unsupported exception with old firmware and latest API
                }
            }
        }

        if (region != null)
        {
            cmdSetRegion(region);
        }        
        currentProtocol = cmdGetProtocol();
        if (currentProtocol == null)
        {
            cmdSetProtocol(TagProtocol.GEN2);
            currentProtocol = TagProtocol.GEN2; 
        }
       /**
       * This version check is required for the new reader stats.
       * Older firmwares does not support this. Currently, firmware version
       * 1.21.1.2 has support for new reader stats.
       **/
       if(versionInfo.fwVersion.part1 >= 1 && versionInfo.fwVersion.part2 >= 21 &&
                versionInfo.fwVersion.part3 >= 1 && versionInfo.fwVersion.part4 >= 2)
       {
            statsSupportedFlags = true;
       }

        addParam(TMR_PARAM_VERSION_MODEL,
                String.class, model, false, null);


        addParam(TMR_PARAM_VERSION_HARDWARE,
                String.class, null, false,
                new ReadOnlyAction()
        {

                    public Object get(Object value)
                            throws ReaderException
                    {
                        // This value doesn't change during runtime, so let
                        // the param system cache it.
                        if (value == null)
                        {
                            StringBuilder sb = new StringBuilder();
                            sb.append(versionInfo.hardware.toString());
                            try
                            {
                                byte[] hwinfo = cmdGetHardwareVersion(0, 0);
                                // commented below code to fix bug#2117
//                                sb.append("-");
//                                for (byte b : hwinfo)
//                                {
//                                    sb.append(String.format("%02x", b));
//                                }
                            }
                            catch (ReaderCodeException re)
                            {
                                // The module throws an exception if there's no HW info programmed in.
                                // Not really an error here, just a lack of data.
                            }
                            value = sb.toString();
                        }
                        return value;
                    }
                });

        addParam(TMR_PARAM_VERSION_SOFTWARE,
                String.class,
                String.format("%s-%s-BL%s",
                versionInfo.fwVersion.toString(),
                versionInfo.fwDate.toString(),
                versionInfo.bootloader.toString()),
                false, null);

        addParam(TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS,
                TagProtocol[].class, versionInfo.protocols, false,
                new ReadOnlyAction()
                {
                    public Object get(Object value)
                    {
                        TagProtocol[] protos = (TagProtocol[]) value;
                        TagProtocol[] protosCopy;
                        protosCopy = new TagProtocol[protos.length];
                        System.arraycopy(protos, 0, protosCopy, 0,
                                protos.length);
                        return protosCopy;
                    }
                });

        addParam(TMR_PARAM_RADIO_POWERMIN,
                Integer.class, null, false,
                new ReadOnlyAction()
                {
                    public Object get(Object value)
                            throws ReaderException
                    {
                        if (powerLimits == null)
                        {
                            powerLimits = cmdGetReadTxPowerWithLimits();
                        }
                        return powerLimits[2];
                    }
                });

        addParam(TMR_PARAM_RADIO_POWERMAX,
                Integer.class, null, false,
                new ReadOnlyAction()
                {
                    public Object get(Object value)
                            throws ReaderException
                    {
                        if (powerLimits == null)
                        {
                            powerLimits = cmdGetReadTxPowerWithLimits();
                        }
                        return powerLimits[1];
                    }
                });

        addUnconfirmedParam(
                TMR_PARAM_USERMODE,
                UserMode.class, null, true,
                new SettingAction() 
                {
                    public Object set(Object value)
                            throws ReaderException
                    {
                        cmdSetUserMode((UserMode) value);
                        return value;
                    }

                    public Object get(Object value)
                            throws ReaderException
                    {
                        return cmdGetUserMode();
                    }
                });


        addParam(TMR_PARAM_READER_PRODUCTGROUPID, Integer.class, null, false,
                new ReadOnlyAction()
                {
                    @Override
                    public Object get(Object val) throws ReaderException
                    {
                        if (productGroupID == -1)
                        {
                       try
                       {
                           productGroupID = (Integer)cmdGetReaderConfiguration(Configuration.PRODUCT_GROUP_ID);
                       }
                       catch(ReaderException re)
                       {
                           if(re instanceof ReaderCodeException && ((ReaderCodeException)re).getCode()==FAULT_MSG_INVALID_PARAMETER_VALUE)
                           {
                               productGroupID = -1;
                           }
                       }
                        }
                       return productGroupID;
                    }
                });

        addParam(TMR_PARAM_READER_PRODUCTGROUP, String.class, null, false,
                new ReadOnlyAction()
                {
                    @Override
                    public Object get(Object val) throws ReaderException
                    {
                        if(productGroupID == -1)
                        {
                            productGroupID = (Integer)paramGet(TMR_PARAM_READER_PRODUCTGROUPID);
                        }
                        switch (productGroupID)
                        {
                            case 0:
                            case 0xFFFF:
                                return "Embedded Reader";
                            case 1:
                                return "Ruggedized Reader";
                            case 2:
                                return "USB Reader";
                            default:
                                return "Unknown";
                        }
                    }
                });
                
        addParam(TMR_PARAM_READER_PRODUCTID, Integer.class, null, false,
                new ReadOnlyAction()
                {
                    @Override
                    public Object get(Object val) throws ReaderException
                    {
                        if (productID == -1)
                        {
                       try
                       {
                           productID = (Integer)cmdGetReaderConfiguration(Configuration.PRODUCT_ID);
                       }
                       catch(ReaderException re)
                       {
                           if(re instanceof ReaderCodeException && ((ReaderCodeException)re).getCode()==FAULT_MSG_INVALID_PARAMETER_VALUE)
                           {
                               productID = -1;
                           }
                       }
                        }
                       return productID;
                    }
                });
        // This method should configure the product group information
        configureForProductGroup();        
    }

    public void destroy()
    {
        connected = false;

        try
        {
            //If crc is not enabled, enable the crc
            if (!isCRCEnabled)
            {
                cmdSetReaderConfiguration(Configuration.SEND_CRC, true);
            }
            st.shutdown();
        }
        catch (ReaderException re)
        {
            // Nothing to do here; we're trying to shut down.
        }
        st = null;
    }

    int[] getAntennaPorts()
            throws ReaderException
    {
        int[] newPorts;
        int numPorts;
        try
        {
            // Try the new antenna-detect command, which as a side effect
            // enumerates all valid antennas for us.            
            antennas = cmdAntennaDetect();
            numPorts = antennas.length;
            newPorts = new int[numPorts];
            for (int i = 0; i < numPorts; i++)
            {
                newPorts[i] = antennas[i][0];
            }
        }
        catch (ReaderCodeException re)
        {
            AntennaPort ap = cmdGetAntennaConfiguration();
            numPorts = ap.numPorts;
            newPorts = new int[numPorts];
            for (int i = 0; i < numPorts; i++)
            {
                newPorts[i] = i + 1;
            }
        }
        return newPorts;
    }

    protected void setCurrentAntenna(int a)
            throws ReaderException
    {
        if (currentAntenna != a)
        {
            int[] ports = antennaPortMap.get(a);
            if (ports == null)
            {
                throw new IllegalArgumentException(
                        String.format("No antenna number %d in /reader/antenna/txRxMap", a));
            }
            cmdSetTxRxPorts(ports[0], ports[1]);
            currentAntenna = a;
        }
    }

    protected void setSearchAntennaList(int[] list)
            throws ReaderException
    {
        if (!Arrays.equals(searchList, list))
        {
            int[][] pairList = new int[list.length][];
            int[] pair;
            for (int i = 0; i < list.length; i++) {
                pair = antennaPortMap.get(list[i]);
                if (pair == null)
                {
                    throw new IllegalArgumentException(
                            String.format("No antenna number %d in /reader/antenna/txRxMap",
                            list[i]));
                }
                pairList[i] = pair;
            }
            cmdSetAntennaSearchList(pairList);
            searchList = list.clone();
        }
    }

    protected void setProtocol(TagProtocol protocol)
            throws ReaderException
    {
        if (protocol != currentProtocol)
        {
            cmdSetProtocol(protocol);
            currentProtocol = protocol;
            if (extendedEPC)
            {
                cmdSetReaderConfiguration(Configuration.EXTENDED_EPC, true);
            }
            cmdSetReaderConfiguration(Configuration.ENABLE_FILTERING, _enableFiltering);
            int moduleValue = (DEFAULT_READ_FILTER_TIMEOUT == _readFilterTimeout)? 0: _readFilterTimeout;
            cmdSetReaderConfiguration(Configuration.TAG_BUFFER_ENTRY_TIMEOUT, moduleValue);
        }
    }

    void checkOpAntenna()
            throws ReaderException
    {
        int a;
        a = (Integer) paramGet(TMR_PARAM_TAGOP_ANTENNA);
        if (a == 0)
        {
            throw new ReaderException("No antenna detected or configured "
                    + "for tag operations");
        }
        setCurrentAntenna(a);
    }

    void checkRegion()
            throws ReaderException
    {
        if (region == null)
        {
            throw new ReaderException("Region must be set before RF operation");
        }
    }

    // Fill in a new TagData object by parsing module data
    void metadataFromMessage(TagReadData t, Message m,
            Set<TagMetadataFlag> meta)
    {
        t.metadataFlags = meta;

        if (meta.contains(TagMetadataFlag.READCOUNT))
        {
            t.readCount = m.getu8();
        }
        if (meta.contains(TagMetadataFlag.RSSI))
        {
            t.rssi = (byte) m.getu8(); // keep the sign here
        }
        if (meta.contains(TagMetadataFlag.ANTENNAID))
        {
            t.antenna = m.getu8();
        }
        if (meta.contains(TagMetadataFlag.FREQUENCY))
        {
            t.frequency = m.getu24();
        }
        if (meta.contains(TagMetadataFlag.TIMESTAMP))
        {
            t.readOffset = m.getu32();
        }
        if (meta.contains(TagMetadataFlag.PHASE))
        {
            t.phase = m.getu16();
        }
        if (meta.contains(TagMetadataFlag.PROTOCOL))
        {
            t.readProtocol = codeToProtocolMap.get(m.getu8());
        }
        if (meta.contains(TagMetadataFlag.DATA))
        {
            int dataBits = m.getu16();
            t.data = new byte[(dataBits + 7) / 8];
            m.getbytes(t.data, t.data.length);
            if (isGen2AllMemoryBankEnabled)
            {
                parseTagMemBankdata(t, t.data, 0);
                isGen2AllMemoryBankEnabled = false;
        }
        }
        if (meta.contains(TagMetadataFlag.GPIO_STATUS))
        {
            byte gpioByte = (byte) m.getu8();
            int gpioNumber;
            switch (versionInfo.hardware.part1)
            {
                case TMR_SR_MODEL_M6E:
                    gpioNumber = 4;
                    break;
                case TMR_SR_MODEL_M5E:
                    gpioNumber = 2;
                    break;
                default:
                    gpioNumber = 4;
                    break;
            }

            t.gpio = new GpioPin[gpioNumber];
            for (int i = 0; i < gpioNumber; i++)
            {
                (t.gpio)[i] = new GpioPin((i + 1), (((gpioByte >> i) & 1) == 1));
            }
        }
    }
    static Set<TagMetadataFlag> allMeta = EnumSet.of(
            TagMetadataFlag.READCOUNT,
            TagMetadataFlag.RSSI,
            TagMetadataFlag.ANTENNAID,
            TagMetadataFlag.FREQUENCY,
            TagMetadataFlag.PHASE,
            TagMetadataFlag.TIMESTAMP,
            TagMetadataFlag.DATA,
            TagMetadataFlag.PROTOCOL,
            TagMetadataFlag.GPIO_STATUS);
    static int allMetaBits = tagMetadataSetValue(allMeta);

    private void parseTagMemBankdata(TagReadData t, byte[] response, int readOffset)
    {
        int length = t.getData().length;
        while (length != 0)
        {
            if (readOffset == length) 
            {
                break;
            }
            int bank = ((response[readOffset] >> 4) & 0x1F);
            int epcdataLength = response[readOffset + 1] * 2;
            Gen2.Bank memBank = Gen2.Bank.getBank(bank);
            switch (memBank)
            {
                case EPC:
                    t.dataEpcMem = new byte[epcdataLength];
                    System.arraycopy(response, (readOffset + 2), t.dataEpcMem, 0, epcdataLength);
                    readOffset += (2 + epcdataLength);
                    break;
                case RESERVED:
                    t.dataReservedMem = new byte[epcdataLength];
                    System.arraycopy(response, (readOffset + 2), t.dataReservedMem, 0, epcdataLength);
                    readOffset += (2 + epcdataLength);
                    break;
                case TID:
                    t.dataTidMem = new byte[epcdataLength];
                    System.arraycopy(response, (readOffset + 2), t.dataTidMem, 0, epcdataLength);
                    readOffset += (2 + epcdataLength);
                    break;
                case USER:
                    t.dataUserMem = new byte[epcdataLength];
                    System.arraycopy(response, (readOffset + 2), t.dataUserMem, 0, epcdataLength);
                    readOffset += (2 + epcdataLength);
                    break;
                default:
                    break;
            }
        }
    }

    /**
     * reading tags for specified amount of time
     * @param timeout
     * @return array of tag data
     * @throws ReaderException
     */
    public TagReadData[] read(long timeout)
            throws ReaderException
    {
        List<TagReadData> tagvec;
        checkConnection();
        //checkRegion();
		/**
         * Reset the reader statistics at the beginning of every search,
         * currently reader statistics only supports in m6e..
         * TO DO: Enable this for other readers in future.
         */
         HashSet<ReaderStatsFlag> flagSet = new HashSet<ReaderStatsFlag>();
         flagSet.addAll(Arrays.asList(ReaderStatsFlag.values()));
         if(statsSupportedFlags)
         {
            cmdResetReaderStats(flagSet);
         }

        if(!continuousReading)
        {
            tagOpSuccessCount = 0;
            tagOpFailuresCount = 0;
            cmdClearTagBuffer();
        }
        if(!useStreaming)
        {
            // This is not true continuous read. Send 22 to start read and then use 29 to Get the tag buffer
                cmdClearTagBuffer();
        }
        tagvec = new Vector<TagReadData>();
        readInternal(timeout, (ReadPlan) paramGet(TMR_PARAM_READ_PLAN), tagvec);
        return tagvec.toArray(new TagReadData[tagvec.size()]);
    }

    /**
     * read tags based on read plan and update tag data list
     * @param timeout
     * @param rp
     * @param tagvec
     * @throws ReaderException
     */
    void readInternal(long timeout, ReadPlan rp, List<TagReadData> tagvec)
            throws ReaderException
    {        
        int readTimeout;
        int numTags = 0, count;
        long baseTime, now, endTime;        
        TagFilter readFilter;
        isStopNTags = false;
        numberOfTagsToRead = 0;

        if (timeout < 0)
        {
            throw new IllegalArgumentException("timeout " + timeout
                    + "ms out of range");
        }
        // HashMap<TagProtocol,TagFilter> protocolFilterPair=new HashMap<TagProtocol,TagFilter>();        

        List<SimpleReadPlan> planList = new ArrayList<SimpleReadPlan>();

        if (rp instanceof MultiReadPlan)
        {
            MultiReadPlan mrp = (MultiReadPlan) rp;
            if ((useStreaming || M6E_FAMILY_LIST.contains(model.toUpperCase()))&& compareAntennas(mrp))
            {
                for (ReadPlan r : mrp.plans)
                {
                    SimpleReadPlan srp = (SimpleReadPlan) r;
                    //protocolFilterPair.put(srp.protocol, srp.filter);
                    planList.add(srp);
                }
                setSearchAntennaList(prepForSearch((SimpleReadPlan) mrp.plans[0]));
                cmdMultiProtocolSearch((int) MSG_OPCODE_READ_TAG_ID_MULTIPLE, planList, TagMetadataFlag.ALL,
                        ((useStreaming ? READ_MULTIPLE_SEARCH_FLAGS_TAG_STREAMING : 0) | READ_MULTIPLE_SEARCH_FLAGS_SEARCH_LIST), (short) timeout, tagvec);
            } 
            else
            {
                for (ReadPlan r : mrp.plans)
                {
                    if(mrp.totalWeight!=0)
                    {
                        readInternal(timeout * r.weight / mrp.totalWeight, r, tagvec);
                    }
                    else
                    {
                        readInternal(timeout/mrp.plans.length, r, tagvec);
                    }                    
                }
            }
            return;
        }// end of multi read plan logic

        readFilter = null;
        if (!(rp instanceof SimpleReadPlan))
        {
            throw new UnsupportedOperationException("Unsupported read plan "
                    + rp.getClass().getName());
        }        
        if (rp instanceof StopTriggerReadPlan)
        {
            StopTriggerReadPlan strp = (StopTriggerReadPlan) rp;
            if(strp.stopOnCount instanceof StopOnTagCount)
            {
                StopOnTagCount sotc=(StopOnTagCount) strp.stopOnCount;
                isStopNTags = true;            
                numberOfTagsToRead = sotc.N;   
            }            
        }

        SimpleReadPlan sp = (SimpleReadPlan) rp;

        setProtocol(sp.protocol);
        if(useStreaming)
        {
            //Disable filtering incase of streaming
           cmdSetReaderConfiguration(Configuration.ENABLE_FILTERING, false);
        }
        readFilter = sp.filter;
        setSearchAntennaList(prepForSearch(sp));

        now = System.currentTimeMillis();
        endTime = now + timeout;
        int tm = 0;
        while (now < endTime)
        {
            readTimeout = ((endTime - now) < 65535) ? (int) (endTime - now) : 65535;            
            baseTime = System.currentTimeMillis();
            Message m = new Message();
            boolean isFastSearch = sp.useFastSearch;
            int searchflag = AntennaSelection.CONFIGURED_LIST.value;

            if (sp.Op == null) // stand alone tag operations
            {
                if (useStreaming)
                {
                    if(planList.isEmpty())
                    {
                        planList.add((SimpleReadPlan)paramGet(TMR_PARAM_READ_PLAN));
                    }                    
                    cmdMultiProtocolSearch((int) MSG_OPCODE_READ_TAG_ID_MULTIPLE, planList, null,
                        ((useStreaming ? READ_MULTIPLE_SEARCH_FLAGS_TAG_STREAMING : 0) | READ_MULTIPLE_SEARCH_FLAGS_SEARCH_LIST), (short) timeout, tagvec);
                    return;
                }
                else // no streaming option
                {
                    try
                    {
                        int tagCount = cmdReadTagMultiple(readTimeout, AntennaSelection.CONFIGURED_LIST, sp.protocol, readFilter, isFastSearch);
                        List<TagReadData>  tagData = getAllTagReads(baseTime, tagCount, sp.protocol);
                        tagvec.addAll(tagData);
                    } 
                    catch (ReaderException re)
                    {
                        if (re instanceof ReaderCodeException && ((ReaderCodeException)re).getCode() == FAULT_NO_TAGS_FOUND)
                        {
                            // just ignore no tags found response                            
                        }
                        else if (re instanceof ReaderFatalException)
                        {
                            throw re;
                        }
                        else if (re instanceof ReaderCommException || (re instanceof ReaderCodeException &&(
                                 (((ReaderCodeException) re).getCode() == FAULT_SYSTEM_UNKNOWN_ERROR) ||
                                 (((ReaderCodeException) re).getCode() == FAULT_TM_ASSERT_FAILED) ||
                                ((ReaderCodeException) re).getCode() == FAULT_MSG_WRONG_NUMBER_OF_DATA)))
                        {
                            // exception handled and come out of the thread                            
                            throw re;
                        }
                        else if(re instanceof ReaderCodeException)
                        {
                            //any other reader code exception like 0x504, 0x505
                            int tagCount = cmdGetTagsRemaining()[0];
                            List<TagReadData> tagData = getAllTagReads(baseTime, tagCount, sp.protocol);
                            re.setTagReads(tagData);
                            tagvec.addAll(tagData); 
                            /**
                             * M5e and its variants hardware does not have a real PA protection.So, doing the read with out
                             * antenna may cause the damage to the reader.
                             *
                             * it's okay to let M6e and its variants continue to operate because it has a PA protection mechanism.
                             **/
                            if(!M6E_FAMILY_LIST.contains(model.toUpperCase()) && re.getMessage().contains("high return loss"))
                            {
                                // exception handled and come out of the thread
                                throw re;
                            }
                            else if (((ReaderCodeException) re).getCode() == FAULT_MSG_INVALID_PARAMETER_VALUE)
                            {
                                throw re;
                            }
                            else
                            {
                               notifyExceptionListeners(re);
                            }
                        }
                        else
                        {
                            throw re;
                    }
                    }
                }//end of else
            }
            else //embedded operation
            {
                if (sp.protocol != TagProtocol.GEN2)
                {
                    throw new ReaderException("The embedded command only supports Gen2");
                }
                if (useStreaming) //using streaming
                {
                    if(planList.isEmpty())
                    {
                        planList.add((SimpleReadPlan)paramGet(TMR_PARAM_READ_PLAN));
                    }
                    cmdMultiProtocolSearch((int) MSG_OPCODE_READ_TAG_ID_MULTIPLE, planList, null,
                        ((useStreaming ? READ_MULTIPLE_SEARCH_FLAGS_TAG_STREAMING : 0) | READ_MULTIPLE_SEARCH_FLAGS_SEARCH_LIST | READ_MULTIPLE_SEARCH_FLAGS_EMBEDDED_OP), (short) timeout, tagvec);
                
                    searchflag |= READ_MULTIPLE_SEARCH_FLAGS_EMBEDDED_OP;
                    //tm = msgEmbedded(m, sp, readTimeout, searchflag, null);
    
                    m.data[tm] = (byte) (m.writeIndex - tm - 2);
                                
                   // sendMessage(readTimeout, m);
                    //receiveResponseStream(readTimeout, m);
                }
                else //without streaming (for M5e and M6e with asyncofftime non-zero)
                {
                    count = 0;

                    searchflag |= READ_MULTIPLE_SEARCH_FLAGS_EMBEDDED_OP;
                    tm = msgEmbedded(m, sp, readTimeout, searchflag, sp.filter, isFastSearch);
                    m.data[tm] = (byte) (m.writeIndex - tm - 2);
                    try
                    {
                        sendTimeout(readTimeout, m);
                    }
                    catch (ReaderCodeException re)
                    {
                        if (re.getCode() == FAULT_NO_TAGS_FOUND)
                        {
                            return;
                        }
                        else if (re.getCode() == FAULT_MSG_INVALID_PARAMETER_VALUE)
                        {
                            throw re;
                        }
                    }
                    int sflags = m.getu16at(6);
                    if ((sflags & AntennaSelection.LARGE_TAG_POPULATION_SUPPORT.value) != 0 && M6E_FAMILY_LIST.contains(model.toUpperCase()))
                    {
                        numTags = m.getu32at(8);
                        tagOpSuccessCount += m.getu16at(14);
                        tagOpFailuresCount += m.getu16at(16);
                    }
                    else
                    {
                        numTags = m.getu8at(8);
                        tagOpSuccessCount += m.getu16at(11);
                        tagOpFailuresCount += m.getu16at(13);
                    }
                    List<TagReadData> tagData = getAllTagReads(baseTime, numTags, sp.protocol);
                    tagvec.addAll(tagData);
                }//end of else (without streaming)
            }
            if (isStopNTags) 
            {
                break;//breaking while, on finding tags no need of sending 22 again
            }
            now = System.currentTimeMillis();
        }
        /*deduplication*/
        if(_enableFiltering)
        {
            HashMap<String, TagReadData> map = new HashMap<String, TagReadData>();            
            String key = null;



            for (TagReadData tag : tagvec)
            {
                if (null == tag)
                {
                    continue;
                }
                key = tag.epcString();
                if (uniqueByAntenna)
                {
                    key += tag.epcString() + ";" + tag.getAntenna();
                }
                if (uniqueByData)
                {
                    key += tag.epcString() + ";" + ReaderUtil.byteArrayToHexString(tag.data);
                }
                if (uniqueByProtocol)
                {
                    key += tag.epcString() + ";" + tag.getTag().getProtocol();
                }                    

                if (!map.containsKey(key))
                {
                    map.put(key, tag);
                }
                else //see the tag again
                {
                    map.get(key).readCount = map.get(key).getReadCount() + tag.getReadCount();
                    if ((Boolean) paramGet(TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI)) {
                        if (tag.getRssi() > map.get(key).getRssi()) {
                            int tmp = map.get(key).getReadCount();
                            map.put(key, tag);
                            map.get(key).readCount = tmp;
                        }
                    }
                }//end of else
            }//end of for
            tagvec.clear();
            tagvec.addAll(map.values());
        }//end of de-duplication        
    }

    private int msgEmbedded(Message m, SimpleReadPlan sp, int readTimeout, int searchflag, TagFilter readFilter, boolean fastSearch) throws ReaderException
    {
        int tm;
        int accPword = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
        if(sp.Op instanceof Gen2.SecureReadData)
        {
            isSecureAccessEnabled = true;
            int accessPassword = 0;
            if (((Gen2.SecureReadData) sp.Op).password instanceof Gen2.SecurePasswordLookup)
            {
                isSecurePasswordLookup = true;
                //length of the data is of 4bytes i.e flashOffset of 2bytes,addressOffset of 1byte ,addressLength of 1 byte
                byte[] data = new byte[4];
                byte[] fOffset = ((Gen2.SecurePasswordLookup) ((Gen2.SecureReadData) sp.Op).password).flashOffset;
                data[0] = ((Gen2.SecurePasswordLookup) ((Gen2.SecureReadData) sp.Op).password).addressOffset;
                data[1] = ((Gen2.SecurePasswordLookup) ((Gen2.SecureReadData) sp.Op).password).addressLength;
                data[2] = fOffset[0];
                data[3] = fOffset[1];
                accessPassword = ReaderUtil.byteArrayToInt(data, 0);
            } 
            else if (((Gen2.SecureReadData) sp.Op).password instanceof Gen2.Password)
            {
                accessPassword = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
            }

            //Get the tag type;
            int tagType = (Enum.valueOf(Gen2.SecureTagType.class, ((Gen2.SecureReadData)sp.Op).type.toString())).type;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accessPassword, fastSearch);
            msgAddGEN2DataRead(m, readTimeout, 0, ((Gen2.SecureReadData) sp.Op).Bank.rep, ((Gen2.SecureReadData) sp.Op).WordAddress, ((Gen2.SecureReadData) sp.Op).Len, tagType);
        }
        else if(sp.Op instanceof Gen2.ReadData)
        {
            Gen2.ReadData rData = (ReadData) sp.Op;
            int value = 0x0;
            if(rData.banks != null)
            {
                EnumSet<Gen2.Bank> banks = rData.banks;
                Iterator<Bank> iterator = banks.iterator();
                while(iterator.hasNext())
                {
                    value |= iterator.next().rep;
                }
                if(value > 3)
                {
                    isGen2AllMemoryBankEnabled = true;
                }
            }
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            if(!isGen2AllMemoryBankEnabled)
            {
                msgAddGEN2DataRead(m, readTimeout, 0,((Gen2.ReadData) sp.Op).Bank.rep, ((Gen2.ReadData) sp.Op).WordAddress, ((Gen2.ReadData) sp.Op).Len, 0);
        }
            else{
                msgAddGEN2DataRead(m, readTimeout, 0, value, ((Gen2.ReadData) sp.Op).WordAddress, ((Gen2.ReadData) sp.Op).Len, 0);
            }
        }
        else if (sp.Op instanceof Gen2.WriteData)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2DataWrite(m, readTimeout, ((Gen2.WriteData) sp.Op).Bank, ((Gen2.WriteData) sp.Op).WordAddress, ((Gen2.WriteData) sp.Op).Data);
        } 
        else if (sp.Op instanceof Gen2.WriteTag)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2WriteTag(m, readTimeout, ((Gen2.WriteTag) sp.Op).Epc, null);
        } 
        else if (sp.Op instanceof Gen2.Lock)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2LockTag(m, readTimeout, ((Gen2.Lock) sp.Op).Action.mask, ((Gen2.Lock) sp.Op).Action.action, ((Gen2.Lock) sp.Op).AccessPassword);
        } 
        else if (sp.Op instanceof Gen2.Kill)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2KillTag(m, readTimeout, ((Gen2.Kill) sp.Op).KillPassword);
        } 
        else if (sp.Op instanceof Gen2.BlockWrite)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2BlockWrite(m, readTimeout, ((Gen2.BlockWrite) sp.Op).Bank, ((Gen2.BlockWrite) sp.Op).WordPtr, ((Gen2.BlockWrite) sp.Op).WordCount, ((Gen2.BlockWrite) sp.Op).Data, 0, null);
        } 
        else if (sp.Op instanceof Gen2.BlockPermaLock)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2BlockPermaLock(m, readTimeout, ((Gen2.BlockPermaLock) sp.Op).ReadLock, ((Gen2.BlockPermaLock) sp.Op).Bank, ((Gen2.BlockPermaLock) sp.Op).BlockPtr, ((Gen2.BlockPermaLock) sp.Op).BlockRange, ((Gen2.BlockPermaLock) sp.Op).Mask, 0, null);
        }
        else if (sp.Op instanceof Gen2.BlockErase)
        {
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddGEN2BlockErase(m, readTimeout, ((Gen2.BlockErase) sp.Op).Bank, ((Gen2.BlockErase) sp.Op).WordPtr, ((Gen2.BlockErase) sp.Op).WordCount, 0, null);
        }
        else if (sp.Op instanceof Gen2.Alien.Higgs2.PartialLoadImage)
        {
            Gen2.Alien.Higgs2.PartialLoadImage higgsTagop = (Gen2.Alien.Higgs2.PartialLoadImage) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddHiggs2PartialLoadImage(m, readTimeout, higgsTagop.accessPassword, higgsTagop.killPassword, higgsTagop.epc);
        } 
        else if (sp.Op instanceof Gen2.Alien.Higgs2.FullLoadImage)
        {
            Gen2.Alien.Higgs2.FullLoadImage higgs2Tagop = (Gen2.Alien.Higgs2.FullLoadImage) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddHiggs2FullLoadImage(m, readTimeout, higgs2Tagop.accessPassword, higgs2Tagop.killPassword, higgs2Tagop.lockBits, higgs2Tagop.pcWord, higgs2Tagop.epc);
        } 
        else if (sp.Op instanceof Gen2.Alien.Higgs3.BlockReadLock)
        {
            Gen2.Alien.Higgs3.BlockReadLock higgs3Tagop = (Gen2.Alien.Higgs3.BlockReadLock) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, higgs3Tagop.accessPassword, fastSearch);
            msgAddHiggs3BlockReadLock(m, readTimeout, higgs3Tagop.accessPassword, higgs3Tagop.lockBits, null);
        } 
        else if (sp.Op instanceof Gen2.Alien.Higgs3.FastLoadImage)
        {
            Gen2.Alien.Higgs3.FastLoadImage higgs3Tagop = (Gen2.Alien.Higgs3.FastLoadImage) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, higgs3Tagop.currentAccessPassword, fastSearch);
            msgAddHiggs3FastLoadImage(m, readTimeout, higgs3Tagop.currentAccessPassword, higgs3Tagop.accessPassword, higgs3Tagop.killPassword, higgs3Tagop.pcWord, higgs3Tagop.epc, null);
        } 
        else if (sp.Op instanceof Gen2.Alien.Higgs3.LoadImage)
        {
            Gen2.Alien.Higgs3.LoadImage higgs3Tagop = (Gen2.Alien.Higgs3.LoadImage) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, higgs3Tagop.currentAccessPassword, fastSearch);
            msgAddHiggs3LoadImage(m, readTimeout, higgs3Tagop.currentAccessPassword, higgs3Tagop.accessPassword, higgs3Tagop.killPassword, higgs3Tagop.pcWord, higgs3Tagop.EPCAndUserData, null);
        }
        else if (sp.Op instanceof Gen2.IDS.SL900A)
        {
            Gen2.IDS.SL900A op = (Gen2.IDS.SL900A) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, op.password, fastSearch);
            if(op instanceof Gen2.IDS.SL900A.EndLog)
            {
               Gen2.IDS.SL900A.EndLog tagOp = (Gen2.IDS.SL900A.EndLog)op;
                msgAddIdsSL900aEndLog(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.StartLog)
            {
                Gen2.IDS.SL900A.StartLog tagOp = (Gen2.IDS.SL900A.StartLog)op;
                msgAddIdsSL900aStartLog(m, tm,tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.Initialize)
            {
                Gen2.IDS.SL900A.Initialize tagOp = (Gen2.IDS.SL900A.Initialize)op;
                msgAddIdsSL900aInitialize(m, tm,tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.GetLogState)
            {
                Gen2.IDS.SL900A.GetLogState tagOp = (Gen2.IDS.SL900A.GetLogState)op;
                msgAddIdsSL900aGetLogState(m, tm,tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.GetSensorValue)
            {
                Gen2.IDS.SL900A.GetSensorValue tagOp = (Gen2.IDS.SL900A.GetSensorValue)op;
                msgAddIdsSL900aGetSensorValue(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.SetLogMode)
            {
                Gen2.IDS.SL900A.SetLogMode tagOp = (Gen2.IDS.SL900A.SetLogMode)op;
                msgAddIdsSL900aSetLogMode(m, tm,tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.AccessFifo)
            {
                Gen2.IDS.SL900A.AccessFifo tagOp = (Gen2.IDS.SL900A.AccessFifo)op;
                msgAddIdsSL900aAccessFifo(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.SetCalibrationData)
            {
                Gen2.IDS.SL900A.SetCalibrationData tagOp = (Gen2.IDS.SL900A.SetCalibrationData)op;
                msgAddIdsSL900aSetCalibrationData(m, readTimeout, tagOp, null);
        }
            else if(op instanceof Gen2.IDS.SL900A.GetCalibrationData)
            {
                Gen2.IDS.SL900A.GetCalibrationData tagOp = (Gen2.IDS.SL900A.GetCalibrationData)op;
                msgAddIdsSL900aGetCalibrationData(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.SetSfeParameters)
            {
                Gen2.IDS.SL900A.SetSfeParameters tagOp = (Gen2.IDS.SL900A.SetSfeParameters)op;
                msgAddIdsSL900aSetSfeParameters(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.GetMeasurementSetup)
            {
                Gen2.IDS.SL900A.GetMeasurementSetup tagOp = (Gen2.IDS.SL900A.GetMeasurementSetup)op;
                msgAddIdsSL900aGetMeasurementSetup(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.GetBatteryLevel)
            {
                Gen2.IDS.SL900A.GetBatteryLevel tagOp = (Gen2.IDS.SL900A.GetBatteryLevel)op;
                msgAddIdsSL900aGetBatteryLevel(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.SetLogLimit)
            {
                Gen2.IDS.SL900A.SetLogLimit tagOp = (Gen2.IDS.SL900A.SetLogLimit)op;
                msgAddIdsSL900aSetLogLimit(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.SetPassword)
            {
                Gen2.IDS.SL900A.SetPassword tagOp = (Gen2.IDS.SL900A.SetPassword)op;
                msgAddIdsSL900ASetPassword(m, readTimeout, tagOp, null);
            }
            else if(op instanceof Gen2.IDS.SL900A.SetShelfLife)
            {
                Gen2.IDS.SL900A.SetShelfLife tagOp = (Gen2.IDS.SL900A.SetShelfLife)op;
                msgAddIdsSL900aSetShelfLife(m, readTimeout, tagOp, null);
            }
        }
        else if (sp.Op instanceof Gen2.NxpGen2TagOp.ResetReadProtect)
        {
            Gen2.NxpGen2TagOp.ResetReadProtect resetProtect = (Gen2.NxpGen2TagOp.ResetReadProtect) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, resetProtect.accessPassword, fastSearch);
            msgAddNxpResetReadProtect(m, readTimeout, resetProtect.accessPassword, resetProtect.chipType, null);
        }
        else if (sp.Op instanceof Gen2.NxpGen2TagOp.SetReadProtect)
        {
            Gen2.NxpGen2TagOp.SetReadProtect setProtect = (Gen2.NxpGen2TagOp.SetReadProtect) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, setProtect.accessPassword, fastSearch);
            msgAddNxpSetReadProtect(m, readTimeout, setProtect.accessPassword, setProtect.chipType, null);
        }
        else if (sp.Op instanceof Gen2.NxpGen2TagOp.Calibrate)
        {
            Gen2.NxpGen2TagOp.Calibrate calibrate = (Gen2.NxpGen2TagOp.Calibrate) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, calibrate.accessPassword, fastSearch);
            msgAddNxpCalibrate(m, readTimeout, 0, calibrate.chipType, null);
        }
        else if (sp.Op instanceof Gen2.NxpGen2TagOp.ChangeEas)
        {
            Gen2.NxpGen2TagOp.ChangeEas changeEas = (Gen2.NxpGen2TagOp.ChangeEas) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, changeEas.accessPassword, fastSearch);
            msgAddNxpChangeEas(m, readTimeout, changeEas.accessPassword, changeEas.reset, changeEas.chipType, null);
        }
        else if (sp.Op instanceof Gen2.NXP.G2I.ChangeConfig)
        {
            Gen2.NXP.G2I.ChangeConfig changeConfig = (Gen2.NXP.G2I.ChangeConfig) sp.Op;
            if (changeConfig.chipType != TAG_CHIP_TYPE_NXP_G2IL)
            {
                throw new FeatureNotSupportedException("ChangeConfig is supported only for NXP G2iL Tags");
            }
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, changeConfig.accessPassword, fastSearch);
            msgAddNxpChangeConfig(m, readTimeout, 0, changeConfig.configWord, changeConfig.chipType, null);
        }
        else if (sp.Op instanceof Gen2.Impinj.Monza4.QTReadWrite)
        {
            Gen2.Impinj.Monza4.QTReadWrite readWriteOp = (Gen2.Impinj.Monza4.QTReadWrite) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, readWriteOp.accessPassword, fastSearch);
            msgAddMonza4QTReadWrite(m, readTimeout, 0, readWriteOp.controlByte, readWriteOp.payloadWord, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.ActivateSecureMode)
        {
            Gen2.Denatran.IAV.ActivateSecureMode tagOp = (Gen2.Denatran.IAV.ActivateSecureMode) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranActivateSecureMode(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.AuthenticateOBU)
        {
            Gen2.Denatran.IAV.AuthenticateOBU tagOp = (Gen2.Denatran.IAV.AuthenticateOBU) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranAuthenticateOBU(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.ActivateSiniavMode)
        {
            Gen2.Denatran.IAV.ActivateSiniavMode tagOp = (Gen2.Denatran.IAV.ActivateSiniavMode) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranActivateSiniavMode(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.OBUAuthID)
        {
            Gen2.Denatran.IAV.OBUAuthID tagOp = (Gen2.Denatran.IAV.OBUAuthID) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranOBUAuthID(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.OBUAuthFullPass)
        {
            Gen2.Denatran.IAV.OBUAuthFullPass tagOp = (Gen2.Denatran.IAV.OBUAuthFullPass) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranOBUAuthFullPass(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.OBUAuthFullPass1)
        {
            Gen2.Denatran.IAV.OBUAuthFullPass1 tagOp = (Gen2.Denatran.IAV.OBUAuthFullPass1) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranOBUAuthFullPass1(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.OBUAuthFullPass2)
        {
            Gen2.Denatran.IAV.OBUAuthFullPass2 tagOp = (Gen2.Denatran.IAV.OBUAuthFullPass2) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranOBUAuthFullPass2(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.OBUReadFromMemMap)
        {
            Gen2.Denatran.IAV.OBUReadFromMemMap tagOp = (Gen2.Denatran.IAV.OBUReadFromMemMap) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranOBUReadFromMemMap(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.OBUWriteToMemMap)
        {
            Gen2.Denatran.IAV.OBUWriteToMemMap tagOp = (Gen2.Denatran.IAV.OBUWriteToMemMap) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranOBUWriteToMemMap(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.ReadSec)
        {
            Gen2.Denatran.IAV.ReadSec tagOp = (Gen2.Denatran.IAV.ReadSec)sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranReadSec(m, tagOp, 0,accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.WriteSec)
        {
            Gen2.Denatran.IAV.WriteSec tagOp = (Gen2.Denatran.IAV.WriteSec)sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranWriteSec(m, tagOp, 0, accPword, null);
        }
        else if(sp.Op instanceof Gen2.Denatran.IAV.GetTokenId)
        {
            Gen2.Denatran.IAV.GetTokenId tagOp = (Gen2.Denatran.IAV.GetTokenId) sp.Op;
            tm = prepEmbReadTagMultiple(m, readTimeout, searchflag, readFilter, sp.protocol, TagMetadataFlag.ALL, accPword, fastSearch);
            msgAddIAVDenatranGetTokenId(m, tagOp, 0, accPword, null);
        }
        else
        {
            throw new ReaderException("Received opcode is either invalid or not supported");
        }
        return tm;
    }
    /**
     * prepEmbReadTagMultiple
     * @param m
     * @param readTimeout
     * @param searchflag
     * @param readFilter
     * @param protocol
     * @param metadataFlags
     * @param accessPassword
     * @return index
     */
    private int prepEmbReadTagMultiple(Message m, int readTimeout, int searchflag, TagFilter readFilter, TagProtocol protocol,
                                       TagMetadataFlag metadataFlags,
                                       int accessPassword, boolean fastSearch  )
    {
        msgSetupReadTagMultiple(m, readTimeout, searchflag, readFilter, protocol, metadataFlags, accessPassword, fastSearch);
        m.setu8(0x01); //embedded command count
        return m.writeIndex++; // record the index of the embedded command length
    }
    private int[] prepForSearch(SimpleReadPlan sp) throws ReaderException
    {
        int[] antennaList;

        // If no antennas specified in read plan, try to autodetect
        if (sp.antennas.length == 0)
        {            
            antennaList = getConnectedAntennas();

            if(antennaList.length == 0)
            {
                throw new ReaderException("No connected antennas found");
            }
        }
        else
        {
            antennaList = sp.antennas;
        }
        return antennaList;
    }

    public void writeTag(TagFilter filter, TagData newID)
            throws ReaderException
    {
        if (filter != null)
        {
            cmdWriteTagEpc(commandTimeout, newID, filter, false);
        }
        else
        {
            checkConnection();
            //checkRegion();
            checkOpAntenna();
            cmdWriteTagEpc(commandTimeout, newID, null, false);
        }
    }

    // kill tag
    public void killTag(TagFilter target, TagAuthentication auth)
            throws ReaderException
    {
        TagProtocol protocol;
        protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);

        if (TagProtocol.GEN2 != protocol)
        {
            throw new UnsupportedOperationException("Tag killing only supported for Gen2");
        }

        if (auth == null)
        {
            throw new IllegalArgumentException("killTag requires tag authentication");
        }
        if (auth instanceof Gen2.Password)
        {
            Gen2.Password pw = (Gen2.Password) auth;
            checkConnection();
            //checkRegion();
            checkOpAntenna();
            setProtocol(protocol);
            cmdKillTag(commandTimeout, pw.value, target);
        } 
        else
        {
            throw new UnsupportedOperationException("Unsupported authentication "
                    + auth.getClass().getName());
        }
    }

    /**
     * read tag memory
     * @param target
     * @param bank
     * @param address
     * @param count
     * @return byte array data
     * @throws ReaderException
     */
    public byte[] readTagMemBytes(TagFilter target,
            int bankValue, int address, int count)
            throws ReaderException
    {
        TagProtocol protocol;
        TagReadData tr;
        byte[] bytes;

        checkConnection();
        //checkRegion();
        checkOpAntenna();
        protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        int accessPassword = ((Gen2.Password)paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
        setProtocol(protocol);

        if (TagProtocol.GEN2 == protocol)
        {
            // gen2 devices address and read in words - round address down and
            // length up if necessary
            int wordCount, wordAddress;
            wordAddress = address / 2;
            wordCount = (count + 1 + (address % 2)) / 2;

            tr = cmdGen2ReadTagData(commandTimeout, TagMetadataFlag.emptyMetadata,
                    bankValue,
                    wordAddress, wordCount,
                    accessPassword, target);

            bytes = tr.data;

            if ((wordAddress * 2 == address) && (wordCount * 2 == count))
            {
                return bytes;
            }
            else
            {
                byte[] adjustBytes = new byte[count];
                System.arraycopy(bytes, address % 2, adjustBytes, 0, count);
                return adjustBytes;
            }
        }
        else if (TagProtocol.ISO180006B == protocol)
        {
            byte[] result = new byte[count];
            int offset = 0;
            //return cmdIso180006bReadTagData(commandTimeout, address, count, target);
            while (count > 0)
            {
                int readSize = 8;
                if (readSize > count)
                {
                    readSize = count;
                }
                byte[] readData = cmdIso180006bReadTagData(commandTimeout, address, count, target);
                System.arraycopy(readData, 0, result, offset, readSize);
                count -= readSize;
                offset += readSize;
            }
            return result;
        } 
        else
        {
            throw new UnsupportedOperationException("Protocol " + protocol
                    + " not supported for data reading");
        }
    }

    /**
     * read tag memory in word format
     * @param target
     * @param bank
     * @param address
     * @param count
     * @return short array data
     * @throws ReaderException
     */
    public short[] readTagMemWords(TagFilter target,
            int bank, int address, int count)
            throws ReaderException
    {
        TagReadData tr;
        byte[] bytes;
        short[] words;

        bytes = readTagMemBytes(target, bank, 2 * address, 2 * count);
        words = new short[bytes.length / 2];
        for (int i = 0; i < words.length; i++)
        {
            words[i] = (short) ((bytes[2 * i] << 8) | (bytes[2 * i + 1] & 0xff));
        }
        return words;
    }

    /**
     * write tag memory
     * @param target
     * @param bank
     * @param address
     * @param data
     * @throws ReaderException
     */
    public void writeTagMemBytes(TagFilter target,
            int bank, int address, byte[] data)
            throws ReaderException
    {
        TagProtocol protocol;
        checkConnection();
        //checkRegion();
        checkOpAntenna();
        protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        int accessPassword = ((Gen2.Password)paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
        setProtocol(protocol);
        if (TagProtocol.GEN2 == protocol)
        {
            // Unlike a read operation, this pretty much has to have even parameters.
            if ((address % 2) != 0)
            {
                throw new IllegalArgumentException("Byte write address must be even");
            }
            if ((data.length % 2) != 0)
            {
                throw new IllegalArgumentException("Byte write length must be even");
            }

            switch ((Gen2.WriteMode) paramGet(TMR_PARAM_GEN2_WRITEMODE))
            {
                case WORD_ONLY:
                    cmdGen2WriteTagData(commandTimeout, Gen2.Bank.getBank(bank), address / 2,
                            data, accessPassword, target);
                    break;
                case BLOCK_ONLY:
                    blockWrite(target, Gen2.Bank.getBank(bank), address, (byte) (data.length / 2), ReaderUtil.convertByteArrayToShortArray(data));
                    break;
                case BLOCK_FALLBACK:
                    try
                    {
                        blockWrite(target, Gen2.Bank.getBank(bank), address, (byte) (data.length / 2), ReaderUtil.convertByteArrayToShortArray(data));
                    }
                    catch (ReaderCodeException e)
                    {
                        if ((e.getCode() == 0x406))
                        {
                            cmdGen2WriteTagData(commandTimeout, Gen2.Bank.getBank(bank), address / 2,
                                    data, accessPassword, target);
                        }
                        else
                        {
                            throw e;
                        }
                    }
                    break;
                default:
                    break;
            }
        } 
        else if (TagProtocol.ISO180006B == protocol)
        {
            cmdIso180006bWriteTagData(commandTimeout, address, data, target);
        } 
        else
        {
            throw new UnsupportedOperationException("Protocol " + protocol
                    + " not supported for data writing");
        }
    }

    /**
     * write tag memory in terms of word data
     * @param target
     * @param bank
     * @param address
     * @param data
     * @throws ReaderException
     */
    public void writeTagMemWords(TagFilter target,
            int bank, int address, short[] data)
            throws ReaderException
    {
        byte[] bytes;

        bytes = new byte[data.length * 2];
        for (int i = 0; i < data.length; i++)
        {
            bytes[i * 2] = (byte) ((data[i] >> 8) & 0xff);
            bytes[i * 2 + 1] = (byte) ((data[i] >> 0) & 0xff);
        }
        writeTagMemBytes(target, bank, 2 * address, bytes);
    }

    public void lockTag(TagFilter target, TagLockAction lock)
            throws ReaderException
    {
        lockTag(target, lock, -1);
    }


    /**
     * lock tag operation
     * @param target
     * @param lock
     * @param accessPassword
     * @throws ReaderException
     */
    public void lockTag(TagFilter target, TagLockAction lock, int accessPassword)
            throws ReaderException
    {
        TagProtocol protocol;

        checkConnection();
        //checkRegion();
        checkOpAntenna();
        protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        setProtocol(protocol);

        if (TagProtocol.GEN2 == protocol)
        {
            if (!(lock instanceof Gen2.LockAction))
            {
                throw new UnsupportedOperationException("Unsupported lock action "
                        + lock.getClass().getName());
            }

            Gen2.LockAction g2l = (Gen2.LockAction) lock;
            if(accessPassword == -1)
            {
                Gen2.Password accPw = (Gen2.Password)paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
                accessPassword = accPw.value;
            }
            cmdGen2LockTag(commandTimeout, g2l.mask, g2l.action, accessPassword,
                    target);
        } 
        else if (TagProtocol.ISO180006B == protocol)
        {
            if (!(lock instanceof Iso180006b.LockAction))
            {
                throw new UnsupportedOperationException("Unsupported lock action "
                        + lock.getClass().getName());
            }

            Iso180006b.LockAction i18kl = (Iso180006b.LockAction) lock;
            cmdIso180006bLockTag(commandTimeout, i18kl.address, target);
        } 
        else
        {
            throw new UnsupportedOperationException("Protocol " + protocol
                    + " not supported for tag locking");
        }
    }

    public void blockWrite(TagFilter target, Gen2.Bank bank, int wordPtr, byte wordCount, short[] data)
            throws ReaderException
    {
        int timeout = (Integer) paramGet(TMR_PARAM_COMMANDTIMEOUT);
        Gen2.Bank bankobj = bank;
        Gen2.Password pwobj = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);        
        int password = pwobj.value;
        cmdBlockWrite(timeout, bankobj, wordPtr, wordCount, data, password, target);
    }

    public byte[] blockPermaLock(TagFilter target, byte readLock, Gen2.Bank bank, int blockPtr, byte blockRange, short[] mask)
            throws ReaderException
    {
        int timeout = (Integer) paramGet(TMR_PARAM_COMMANDTIMEOUT);
        Gen2.Bank bankobj = bank;
        Gen2.Password pwobj = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
        int password = pwobj.value;
        return (cmdBlockPermaLock(timeout, readLock, bankobj, blockPtr, blockRange, mask, password, target));
    }

    public void blockErase(TagFilter target, Gen2.Bank bank, int wordPtr, byte wordCount)
            throws ReaderException
    {
        int timeout = (Integer) paramGet(TMR_PARAM_COMMANDTIMEOUT);
        Gen2.Bank bankobj = bank;
        Gen2.Password pwobj = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
        int password = pwobj.value;
        cmdBlockErase(timeout, bankobj, wordPtr, wordCount, password, target);
    }

    public void cmdBlockWrite(int timeout, Gen2.Bank memBank, int wordPtr, byte wordCount, short[] data, int accessPassword, TagFilter target)
            throws ReaderException
    {
        Message m = new Message();
        int optByte;
        m.setu8(MSG_OPCODE_WRITE_TAG_SPECIFIC); //opcode
        m.setu16(timeout);
        m.setu8(0x00);//chip type
        optByte = m.writeIndex++;
        m.setu8(0x00);//block write opcode
        m.setu8(0xC7);//block write opcode
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8(0x00);//Write Flags
        m.setu8(memBank.rep);
        m.setu32(wordPtr);
        m.setu8(wordCount);
        byte[] blockWrite = ReaderUtil.convertShortArraytoByteArray(data);
        m.setbytes(blockWrite, 0, blockWrite.length);
        send(m);
    }

    public byte[] cmdBlockPermaLock(int timeout, byte readLock, Gen2.Bank memBank, int blockPtr, byte blockRange, short[] mask, int accessPassword, TagFilter target)
            throws ReaderException
    {
        Message m = new Message();
        int optByte;
        m.setu8(MSG_OPCODE_ERASE_BLOCK_TAG_SPECIFIC);
        m.setu16(timeout);
        m.setu8(0x00);//chip type
        optByte = m.writeIndex++;
        m.setu8(0x01);
        filterBytes(TagProtocol.GEN2, m, optByte, target, accessPassword, true);
        m.data[optByte] = (byte) (0x40 | (m.data[optByte]));//option
        m.setu8(0x00);//RFU
        m.setu8(readLock);
        m.setu8(memBank.rep);
        m.setu32(blockPtr);
        m.setu8(blockRange);
        if (readLock == 0x01)
        {
            m.setbytes(ReaderUtil.convertShortArraytoByteArray(mask));
        }
        Message msg = send(m);
        if (readLock == 0)
        {
            byte[] returnData = new byte[(msg.data[1] - 2)];
            System.arraycopy(msg.data, 7, returnData, 0, (msg.data[1] - 2));
            return returnData;
        }
        else
        {
            return null;
        }
    }

    public void cmdBlockErase(int timeout, Gen2.Bank memBank, int wordPtr, byte wordCount, int accessPassword, TagFilter target)
            throws ReaderException
    {
        Message m = new Message();
        msgAddGEN2BlockErase(m, timeout, memBank, wordPtr, wordCount, accessPassword, target);
        send(m);
    }

    /**
     * gpiGet
     * @return array of GpioPin
     * @throws ReaderException
     */
    public GpioPin[] gpiGet()
            throws ReaderException
    {        
        checkConnection();
        List<GpioPin> pinvals = new ArrayList<GpioPin>();
        GpioPin[] states = cmdGetGPIO();
        for (GpioPin gpioState : states)
        {
            // blocking output pins using directionality
            if(!gpioState.output)
            {
                pinvals.add(gpioState);
            }
        }
        return pinvals.toArray(new GpioPin[pinvals.size()]) ;
    }

    /**
     * gpoSet
     * @param state array of GpioPin (id, high) settings (output field is ignored)
     * @throws ReaderException
     */
    public void gpoSet(GpioPin[] state)
            throws ReaderException
    {
        checkConnection();

        for (GpioPin gp : state)
        {
            cmdSetGPIO(gp.id, gp.high);
        }
    }

    int readInt(InputStream fwStr)
            throws IOException
    {
        byte[] intbuf = new byte[4];
        int r, ret;

        r = 0;
        while (r < 4)
        {
            ret = fwStr.read(intbuf, r, 4 - r);
            r += ret;
        }

        return ((intbuf[0] & 0xff) << 24)
                | ((intbuf[1] & 0xff) << 16)
                | ((intbuf[2] & 0xff) << 8)
                | (intbuf[3] & 0xff);
    }

    public synchronized void firmwareLoad(InputStream fwStr)
            throws ReaderException, IOException
    {
        int header1, header2;
        int sector, len, address, ret;
        byte[] buf;
        int result = 0;
        try
        {
            checkConnection();
        } 
        catch (ReaderException ex)
        {
            // open serial port.
            // Serial baud rates to try, in the order we're most likely to find them
            openSerialPort();
        }
        
       header1 = readInt(fwStr);
       header2 = readInt(fwStr);

        // Check the magic numbers for correct magic
        if ((header1 != 0x544D2D53) || (header2 != 0x5061696B))
        {
            throw new IllegalArgumentException("Stream does not contain reader firmware");
        }

        sector = readInt(fwStr);
        len = readInt(fwStr);

        if (sector != 2)
        {
            throw new IllegalArgumentException("Only application firmware can be loaded");
        }

        // Move the reader into the bootloader so flash operations work
        // XXX drop down to 9600 before going into the bootloader. Some
        // XXX versions of M5e preserve the baud rate, and some go back to 9600;
        // XXX this way we know what the speed is either way.
        if(st.getBaudRate() != 0)
        {
           cmdSetBaudRate(9600);
           st.setBaudRate(9600);
        }
        
        try
        {
            cmdBootBootloader();
        }
        catch (ReaderCodeException ex)
        {
            // Invalid Opcode (101h) okay -- means "already in bootloader"
            if (0x101 != ex.getCode())
            {
                // Other errors are real
                throw ex;
            }
        }

        // Wait a moment for the bootloader to come back up. This seems to
        // take longer on M5e firmware versions that reset themselves
        // more thoroughly.
        try
        {
            Thread.sleep(200);
        }
        catch (InterruptedException ie)
        {
        }
        
        /** 
         *  some transport layer does not support baud rate settings.
         *  for ex: TCP transport. In that case skip the baud rate settings.
        **/
        if(st.getBaudRate() != 0)
        {
            cmdSetBaudRate(115200);
            st.setBaudRate(115200);
        }
        
        cmdEraseFlash(2, 0x08959121);

        address = 0;
        buf = new byte[240];
        while (len > 0)
        {
            ret = fwStr.read(buf, 0, 240);
            if (ret == -1)
            {
                throw new IllegalArgumentException(
                        "Stream did not contain full length of firmware");
            }
            cmdWriteFlash(2, address, 0x02254410, buf, 0);
            address += ret;
            len -= ret;
        }
        if(!connected)
        {
            connect();
        }

        // Boot the application and re-initialize parameters that may be changed
        // by the new firmware
        else
        {
            boot(region);
        }
    }

    void filterBytes(TagProtocol t, Message m, int optIndex, TagFilter target,
            int password, boolean usePassword)
    {
        if(isSecureAccessEnabled)
        {
            m.data[optIndex] = EmbeddedReaderMessage.SINGULATION_OPTION_SECURE_READ_DATA;
        }
        else
        {
             m.data[optIndex] = EmbeddedReaderMessage.SINGULATION_OPTION_SELECT_DISABLED;
        }
        
        if (TagProtocol.GEN2 == t)
        {
            filterBytesGen2(m, optIndex, target, password, usePassword);
        }
        else if (TagProtocol.ISO180006B == t)
        {
            filterBytesIso180006b(m, optIndex, target);
        }
    }

    void filterBytesGen2(Message m, int optIndex, TagFilter target,
            int password, boolean usePassword)
    {
        if (target == null)
        {
            if(password==0)
            {
                if(!isSecureAccessEnabled)
                {
                   m.data[optIndex] = 0;
                }
                return;
            }
            if(usePassword)
            {
                m.data[optIndex] = 0x05; // Password only
                m.setu32(password);
                return;
            }
            else // target is null and usePassword false
            {
                m.data[optIndex] = 0;
                return;
            }
        }
        if(target!=null && usePassword)
        {
            m.data[optIndex] = 0x05; // Password only
            m.setu32(password);
        }
        if(target instanceof TagData)
        {
            TagData t = (TagData) target;
            m.data[optIndex] = 1; 
            int bitCount=t.epc.length * 8;
            if(bitCount < 255)
            {
                m.setu8(bitCount);
            }
            else
            {
               m.data[optIndex] |= 0x20; //OPTION for extended length  is 0x20
               m.setu16(bitCount);
            }
            m.setbytes(t.epc);
        }
        else if (target instanceof Gen2.Select)
        {
            Gen2.Select s = (Gen2.Select) target;
            if (s.bank == Gen2.Bank.EPC)
            {
                if(isSecurePasswordLookup)
                {
                    m.data[optIndex] = 0x44;
                    isSecurePasswordLookup = false;
                }
                else
                {
                m.data[optIndex] = 0x04;
            }
            }
            else
            {
                m.data[optIndex] = (byte) s.bank.rep;
            }
            if (s.invert == true)
            {
                m.data[optIndex] |= 0x08;
            }

            m.setu32(s.bitPointer);
            if (s.bitLength > 255)
            {
                m.data[optIndex] |= 0x20;
                m.setu16(s.bitLength);
            }
            else
            {
                m.setu8(s.bitLength);
            }
            m.setbytes(s.mask);
        }
        else
        {
            throw new UnsupportedOperationException("Unknown select type "
                    + target.getClass().getName());
        }
    }

    void filterBytesIso180006b(Message m, int optIndex, TagFilter target)
    {
       
        if (optIndex != -1)
        {
            m.data[optIndex] = 1;
        }

        if (null == target)
        {
            // Set up a match-anything filter, since it isn't the default.
            m.setu8(ISO180006B_SELECT_OP_EQUALS);
            m.setu8(0);  // address
            m.setu8(0);  // mask - don't compare anything
            m.setu32(0); // dummy tag ID bytes 0-3, not compared
            m.setu32(0); // dummy tag ID bytes 4-7, not compared
        }
        else if (target instanceof Iso180006b.Select)
        {
            Iso180006b.Select sel = (Iso180006b.Select) target;

            if (false == sel.invert)
            {
                m.setu8(sel.op.rep);
            }
            else
            {
                m.setu8(sel.op.rep | ISO180006B_SELECT_OP_INVERT);
            }
            m.setu8(sel.address);
            m.setu8(sel.mask);
            m.setbytes(sel.data);
        }
        else if (target instanceof TagData)
        {
            TagData t = (TagData) target;

            if (t.epc.length > 8)
            {
                throw new IllegalArgumentException("Can't select on more than 8 bytes");
            }

            // Convert the byte count to a MSB-based bit mask
            int mask = (0xff00 >> t.epc.length) & 0xff;

            m.setu8(ISO180006B_SELECT_OP_EQUALS);
            m.setu8(0); // Address - EPC is at the start of memory
            m.setu8(mask);
            m.setbytes(t.epc);
            // Pad EPC data to 8 bytes
            for (int i = t.epc.length; i < 8; i++)
            {
                m.setu8(0);
            }
        }
        else
        {
            throw new UnsupportedOperationException("Unknown select type "
                    + target.getClass().getName());
        }
    }
   
   /**
    * Handles tag operations
    * @param tagOP
    * @param target
    * @return Object
    * @throws ReaderException
    */
   public Object executeTagOp(TagOp tagOP,TagFilter target) throws ReaderException
   {
        TagProtocol protocolID = (TagProtocol)paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        try
        {
            if (tagOP instanceof Gen2.Kill)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                killTag(target, new Gen2.Password(((Gen2.Kill)tagOP).KillPassword));
                return null;
            }
            else if ( tagOP instanceof Gen2.Lock)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                lockTag(target, new Gen2.LockAction(((Gen2.Lock)tagOP).Action), ((Gen2.Lock)tagOP).AccessPassword);
                return null;
            }
            else if (tagOP instanceof Gen2.WriteTag)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                writeTag(target, ((Gen2.WriteTag)tagOP).Epc);
                return null;
            }
            else if (tagOP instanceof Gen2.SecureReadData)
            {
                throw new ReaderException("Operation not supported");
            }
            else if (tagOP instanceof Gen2.ReadData)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.ReadData rData = (Gen2.ReadData)tagOP;
                int value = 0;
                if(rData.banks != null)
                {
                    EnumSet<Gen2.Bank> banks = rData.banks;
                    Iterator<Bank> iterator = banks.iterator();
                    while(iterator.hasNext())
                    {
                        value |= iterator.next().rep;
            }
                }
                else{
                    value = ((Gen2.ReadData)tagOP).Bank.rep;
                }
                return readTagMemWords(target, value, ((Gen2.ReadData)tagOP).WordAddress, ((Gen2.ReadData)tagOP).Len);
            }
           
            else if (tagOP instanceof Gen2.WriteData)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                writeTagMemWords(target, ((Gen2.WriteData)tagOP).Bank.rep, ((Gen2.WriteData)tagOP).WordAddress,((Gen2.WriteData)tagOP).Data) ;
                return null;
            }
            else if (tagOP instanceof Gen2.BlockWrite)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                blockWrite(target, ((Gen2.BlockWrite)tagOP).Bank, ((Gen2.BlockWrite)tagOP).WordPtr, ((Gen2.BlockWrite)tagOP).WordCount, ((Gen2.BlockWrite)tagOP).Data);
                return null;
            }
            else if (tagOP instanceof Gen2.BlockPermaLock)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return blockPermaLock(target, ((Gen2.BlockPermaLock)tagOP).ReadLock, ((Gen2.BlockPermaLock)tagOP).Bank, ((Gen2.BlockPermaLock)tagOP).BlockPtr, ((Gen2.BlockPermaLock)tagOP).BlockRange, ((Gen2.BlockPermaLock)tagOP).Mask);
            }
            else if (tagOP instanceof Gen2.BlockErase)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                blockErase(target, ((Gen2.BlockErase)tagOP).Bank, ((Gen2.BlockErase)tagOP).WordPtr, ((Gen2.BlockErase)tagOP).WordCount);
                return null;
            }
            else if(tagOP instanceof Gen2.Alien.Higgs2.PartialLoadImage)
            {
                if(null != target)
                {
                    throw new FeatureNotSupportedException("Method or Operation not suppported");
                }
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Alien.Higgs2.PartialLoadImage higgsTagop = (Gen2.Alien.Higgs2.PartialLoadImage)tagOP;
                cmdHiggs2PartialLoadImage(commandTimeout, higgsTagop.accessPassword, higgsTagop.killPassword, higgsTagop.epc);
                return null;
            }
            else if(tagOP instanceof Gen2.Alien.Higgs2.FullLoadImage)
            {
                if(null != target)
                {
                    throw new FeatureNotSupportedException("Method or Operation not suppported");
                }
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Alien.Higgs2.FullLoadImage higgs2Tagop = (Gen2.Alien.Higgs2.FullLoadImage)tagOP;
                cmdHiggs2FullLoadImage(commandTimeout, higgs2Tagop.accessPassword, higgs2Tagop.killPassword, higgs2Tagop.lockBits, higgs2Tagop.pcWord, higgs2Tagop.epc);
                return null;
            }
            else if(tagOP instanceof Gen2.Alien.Higgs3.BlockReadLock)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Alien.Higgs3.BlockReadLock higgs3Tagop = (Gen2.Alien.Higgs3.BlockReadLock)tagOP;
                cmdHiggs3BlockReadLock(commandTimeout, higgs3Tagop.accessPassword, higgs3Tagop.lockBits, target);
                return null;
            }
            else if(tagOP instanceof Gen2.Alien.Higgs3.FastLoadImage)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Alien.Higgs3.FastLoadImage higgs3Tagop = (Gen2.Alien.Higgs3.FastLoadImage)tagOP;
                cmdHiggs3FastLoadImage(commandTimeout, higgs3Tagop.currentAccessPassword, higgs3Tagop.accessPassword, higgs3Tagop.killPassword, higgs3Tagop.pcWord, higgs3Tagop.epc, target);
                return null;
            }
            else if(tagOP instanceof Gen2.Alien.Higgs3.LoadImage)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Alien.Higgs3.LoadImage higgs3Tagop = (Gen2.Alien.Higgs3.LoadImage)tagOP;
                cmdHiggs3LoadImage(commandTimeout, higgs3Tagop.currentAccessPassword, higgs3Tagop.accessPassword, higgs3Tagop.killPassword, higgs3Tagop.pcWord, higgs3Tagop.EPCAndUserData, target);
                return null;
            }
            else if(tagOP instanceof Gen2.IDS.SL900A.StartLog)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aStartLog(commandTimeout, (Gen2.IDS.SL900A.StartLog)tagOP, target);
                return null;
            }
            else if(tagOP instanceof Gen2.IDS.SL900A.EndLog)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aEndLog(commandTimeout, (Gen2.IDS.SL900A.EndLog)tagOP, target);
               return null;
            }
            else if(tagOP instanceof Gen2.NxpGen2TagOp.Calibrate)
            {                
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.NxpGen2TagOp.Calibrate calibrate = (Gen2.NxpGen2TagOp.Calibrate)tagOP;
                return cmdNxpCalibrate(commandTimeout, calibrate.accessPassword,calibrate.chipType , target);
            }
            else if(tagOP instanceof Gen2.NxpGen2TagOp.ResetReadProtect)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.NxpGen2TagOp.ResetReadProtect resetProtect = (Gen2.NxpGen2TagOp.ResetReadProtect)tagOP;
                cmdNxpResetReadProtect(commandTimeout, resetProtect.accessPassword, resetProtect.chipType, target);
                return null;
            }
            else if(tagOP instanceof Gen2.NxpGen2TagOp.SetReadProtect)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.NxpGen2TagOp.SetReadProtect setProtect = (Gen2.NxpGen2TagOp.SetReadProtect)tagOP;
                cmdNxpSetReadProtect(commandTimeout, setProtect.accessPassword, setProtect.chipType, target);
                return null;
            }
            else if(tagOP instanceof Gen2.NxpGen2TagOp.ChangeEas)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.NxpGen2TagOp.ChangeEas changeEas = (Gen2.NxpGen2TagOp.ChangeEas)tagOP;
                cmdNxpChangeEas(commandTimeout, changeEas.accessPassword, changeEas.reset, changeEas.chipType, target);
                return null;
            }
            else if(tagOP instanceof Gen2.NxpGen2TagOp.EasAlarm)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.NxpGen2TagOp.EasAlarm nxpTagOp = (Gen2.NxpGen2TagOp.EasAlarm)tagOP;
                return cmdNxpEasAlarm(commandTimeout, nxpTagOp.divideRatio, nxpTagOp.tagEncoding, nxpTagOp.trExt, nxpTagOp.chipType, target);
            }
            else if (tagOP instanceof Gen2.Impinj.Monza4.QTReadWrite)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Impinj.Monza4.QTReadWrite qtReadWriteOp = (Gen2.Impinj.Monza4.QTReadWrite)tagOP;
                return cmdMonza4QTReadWrite(commandTimeout, qtReadWriteOp.accessPassword, qtReadWriteOp.controlByte, qtReadWriteOp.payloadWord, target);
            }
            else if(tagOP instanceof Gen2.NXP.G2I.ChangeConfig)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.NXP.G2I.ChangeConfig nxpConfig = (Gen2.NXP.G2I.ChangeConfig)tagOP;
                if(nxpConfig.chipType != TAG_CHIP_TYPE_NXP_G2IL)
                {
                    throw new FeatureNotSupportedException("ChangeConfig is supported only for NXP G2iL Tags");
                }
                return cmdNxpChangeConfig(commandTimeout, nxpConfig.accessPassword, nxpConfig.configWord, nxpConfig.chipType, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.ActivateSecureMode)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.ActivateSecureMode secureMode = (Gen2.Denatran.IAV.ActivateSecureMode)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranActivateSecureMode(commandTimeout, secureMode, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.AuthenticateOBU)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.AuthenticateOBU tagOp = (Gen2.Denatran.IAV.AuthenticateOBU)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranAuthenticateOBU(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.ActivateSiniavMode)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.ActivateSiniavMode tagOp = (Gen2.Denatran.IAV.ActivateSiniavMode)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranActivateSiniavMode(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.OBUAuthID)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.OBUAuthID tagOp = (Gen2.Denatran.IAV.OBUAuthID)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranOBUAuthID(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.OBUAuthFullPass)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.OBUAuthFullPass tagOp = (Gen2.Denatran.IAV.OBUAuthFullPass)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranOBUAuthFullPass(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.OBUAuthFullPass1)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.OBUAuthFullPass1 tagOp = (Gen2.Denatran.IAV.OBUAuthFullPass1)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranOBUAuthFullPass1(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.OBUAuthFullPass2)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.OBUAuthFullPass2 tagOp = (Gen2.Denatran.IAV.OBUAuthFullPass2)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranOBUAuthFullPass2(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.OBUReadFromMemMap)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.OBUReadFromMemMap tagOp = (Gen2.Denatran.IAV.OBUReadFromMemMap)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranOBUReadFromMemMap(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.OBUWriteToMemMap)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.OBUWriteToMemMap tagOp = (Gen2.Denatran.IAV.OBUWriteToMemMap)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranOBUWriteToFromMemMap(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.ReadSec)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.ReadSec tagOp = (Gen2.Denatran.IAV.ReadSec)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranReadSec(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.WriteSec)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.WriteSec tagOp = (Gen2.Denatran.IAV.WriteSec)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranWriteSec(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.Denatran.IAV.GetTokenId)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                Gen2.Denatran.IAV.GetTokenId tagOp = (Gen2.Denatran.IAV.GetTokenId)tagOP;
                int password = ((Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD)).value;
                return cmdIAVDenatranGetTokenId(commandTimeout, tagOp, password, target);
            }
            else if(tagOP instanceof Gen2.IDS.SL900A.Initialize)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aInitialize(commandTimeout, (Gen2.IDS.SL900A.Initialize)tagOP, target);
                return null;
            }
            else if(tagOP instanceof Gen2.IDS.SL900A.GetLogState)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return cmdIdsSL900aGetLogState(commandTimeout, (Gen2.IDS.SL900A.GetLogState)tagOP, target);
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.GetSensorValue)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return cmdIdsSL900aGetSensorValue(commandTimeout, (Gen2.IDS.SL900A.GetSensorValue)tagOP, target);
            }            
            else if(tagOP instanceof Gen2.IDS.SL900A.SetLogMode)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aSetLogMode(commandTimeout, (Gen2.IDS.SL900A.SetLogMode)tagOP, target);
                return null;
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.AccessFifo)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return cmdIdsSL900aAccessFifo(commandTimeout, (Gen2.IDS.SL900A.AccessFifo)tagOP, target);
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.SetCalibrationData)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aSetCalibrationData(commandTimeout, (Gen2.IDS.SL900A.SetCalibrationData)tagOP, target);
                return null;
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.GetCalibrationData)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return cmdIdsSL900aGetCalibrationData(commandTimeout, (Gen2.IDS.SL900A.GetCalibrationData)tagOP, target);
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.SetSfeParameters)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aSetSfeParameters(commandTimeout, (Gen2.IDS.SL900A.SetSfeParameters)tagOP, target);
                return null;
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.GetMeasurementSetup)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return cmdIdsSL900aGetMeasurementSetup(commandTimeout, (Gen2.IDS.SL900A.GetMeasurementSetup)tagOP, target);
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.GetBatteryLevel)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                return cmdIdsSL900aGetBatteryLevel(commandTimeout, (Gen2.IDS.SL900A.GetBatteryLevel)tagOP, target);
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.SetLogLimit)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aSetLogLimit(commandTimeout, (Gen2.IDS.SL900A.SetLogLimit) tagOP, target);
                return null;
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.SetPassword)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900ASetPassword(commandTimeout, (Gen2.IDS.SL900A.SetPassword) tagOP, target);
                return null;
            }
            else if (tagOP instanceof Gen2.IDS.SL900A.SetShelfLife)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
                cmdIdsSL900aSetShelfLife(commandTimeout, (Gen2.IDS.SL900A.SetShelfLife) tagOP, target);
                return null;
            }
            else if (tagOP instanceof Iso180006b.ReadData)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.ISO180006B);
                return cmdIso180006bReadTagData(commandTimeout, ((Iso180006b.ReadData)tagOP).ByteAddress, ((Iso180006b.ReadData)tagOP).Len, target);
            }
            else if (tagOP instanceof Iso180006b.WriteData)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.ISO180006B);
                cmdIso180006bWriteTagData(commandTimeout, ((Iso180006b.WriteData)tagOP).ByteAddress, (byte[])((Iso180006b.WriteData)tagOP).Data, target);
                return null;
            }
            else if (tagOP instanceof Iso180006b.Lock)
            {
                paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.ISO180006B);
                cmdIso180006bLockTag(commandTimeout, ((Iso180006b.Lock)tagOP).ByteAddress, target);
                return null;
            }
            else
            {
                return null;
            }
       }
       finally
       {
            // restoring the old protocol
           paramSet(TMR_PARAM_TAGOP_PROTOCOL, protocolID);
       }
   }

   /**
    * multi protocol search
    * @param opcode
    * @param plans
    * @param metadataFlags
    * @param antennas
    * @param timeout
    * @param collectedTags
    * @throws ReaderException
    */
    public void cmdMultiProtocolSearch(int opcode, List<SimpleReadPlan> plans, TagMetadataFlag metadataFlags, int antennas, int timeout, List<TagReadData> collectedTags)
            throws ReaderException
    {
        Message m = new Message();
        m.setu8(MSG_OPCODE_MULTI_PROTOCOL_TAG_OP);

        if(useStreaming)
        {
            m.setu16(0x00); // timeout
            m.setu8(0x01);  // TM option 1 for continuous reading
        }
        else
        {
            m.setu16(timeout);
            m.setu8(0x11); // TM option for metadata
            int metadataBits = allMetaBits;
            m.setu16(metadataBits);
        }
        m.setu8(opcode); // sub-command opcode

        TagProtocol tagProtocol = TagProtocol.NONE;
        byte[] data;

        int totalWeight =0;
        int totalTagsToRead = 0;
        for (SimpleReadPlan readPlan : plans)
        {
            totalWeight = totalWeight+readPlan.weight;
            if (readPlan instanceof StopTriggerReadPlan)
            {
                StopTriggerReadPlan strp = (StopTriggerReadPlan) readPlan;
                if (strp.stopOnCount instanceof StopOnTagCount)
                {
                    isStopNTags = true;
                    StopOnTagCount sotc = (StopOnTagCount) strp.stopOnCount;
                    totalTagsToRead += sotc.N;
                }
            }
        }
        if (isStopNTags & !useStreaming)
        {
            m.setu16(READ_MULTIPLE_RETURN_ON_N_TAGS);
            m.setu32(totalTagsToRead);
        }
        else
        {
            m.setu16(0x0000); // search flags   
        }

        for (SimpleReadPlan plan : plans)
        {
            isStopNTags = false;
            short subTimeout =(short)0;
            if(totalWeight==0)
            {
                subTimeout = (short)(timeout / plans.size());
            }else
            {
                subTimeout = (short)(plan.weight *timeout /totalWeight);
            }
            if (plan.protocol.equals(TagProtocol.GEN2))
            {
                m.setu8(PROT_GEN2);
            }
            else if (plan.protocol.equals(TagProtocol.ISO180006B))
            {
                m.setu8(PROT_ISO180006B);
            }
            else if (plan.protocol.equals(TagProtocol.IPX64))
            {
                m.setu8(PROT_IPX64);
            }
            else if (plan.protocol.equals(TagProtocol.IPX256))
            {
                m.setu8(PROT_IPX256);
            }
            else if (plan.protocol.equals(TagProtocol.ISO180006B_UCODE))
            {
                m.setu8(PROT_UCODE);
            }
            else if (plan.protocol.equals(TagProtocol.ATA))
            {
                m.setu8(PROT_ATA);
            }
            if (plan instanceof StopTriggerReadPlan)
            {  
                StopTriggerReadPlan strp = (StopTriggerReadPlan) plan;
                if (strp.stopOnCount instanceof StopOnTagCount)
                {
                    isStopNTags = true;
                    StopOnTagCount sotc = (StopOnTagCount) strp.stopOnCount; 
                    numberOfTagsToRead = sotc.N;                    
                }                
            }
            boolean isFastSearch = plan.useFastSearch;
            int subLen = m.writeIndex++;
            int opCodeLen = 0;
            switch (opcode)
            {
                case MSG_OPCODE_READ_TAG_ID_SINGLE:
                    data = msgSetupReadTagSingle(metadataFlags, plan.filter, plan.protocol, (short) subTimeout);
                    break;
                case MSG_OPCODE_READ_TAG_ID_MULTIPLE:
                    if(null == plan.Op)
                    {
                        msgSetupReadTagMultiple(m, subTimeout, antennas, plan.filter, plan.protocol, metadataFlags, 0, isFastSearch);
                    }
                    else
                    {
                     opCodeLen= msgEmbedded(m, plan, subTimeout, READ_MULTIPLE_SEARCH_FLAGS_EMBEDDED_OP, plan.filter, isFastSearch);
                     m.data[opCodeLen] = (byte) (m.writeIndex - opCodeLen - 2);
                    }
                    break;
                default:
                    throw new ReaderException("Operation not supported" + opcode);
            }
            m.data[subLen] = (byte) (m.writeIndex - subLen - 2);

        }

        Message response;
        int tagsFound;
        if (opcode == MSG_OPCODE_READ_TAG_ID_SINGLE)
        {
            response = sendTimeout(timeout, m);
            tagsFound = response.getu8at(9);
            m.readIndex = 13; // the start of the tag responses
            for (int i = 0; i < m.readIndex; i++)
            {
                int subBegin = m.readIndex;
                if (m.readIndex == response.data.length - 2) // reach the crc
                {
                    break;
                }
                int subResponseLen = response.data[m.readIndex + 1];
                m.readIndex = m.readIndex + 4 + 3; // points to the start of metadata , ignore option and metaflags for read tag single
                TagReadData read = new TagReadData();
                metadataFromMessage(read, m, tagMetadataFlagValues.keySet());
                read.antenna = antennaPortReverseMap.get(read.antenna);
                int crcLen = 2;
                int epcLen = subResponseLen + 4 - (m.readIndex - subBegin) - crcLen;
                read.tag = parseTag(m, epcLen, tagProtocol);
                byte epc[] = new byte[epcLen];
                byte crc[] = new byte[crcLen];
                System.arraycopy(response.data, m.readIndex, epc, 0, epcLen);
                System.arraycopy(response.data, m.readIndex + epcLen, crc, 0, crcLen);
                TagData tag = null;
                switch (read.readProtocol)
                {
                    default:
                        tag = new TagData(epc, crc);
                        break;
                    case GEN2:
                        tag = new Gen2.TagData(epc, crc);
                        break;
                    case ISO180006B:
                    case ISO180006B_UCODE:
                        tag = new Iso180006b.TagData(epc, crc);
                        break;
                }
                read.tag = tag;
                collectedTags.add(read);
                m.readIndex += epcLen + crcLen;  // point to the next protocol tag response
            }
        }

        if (opcode == MSG_OPCODE_READ_TAG_ID_MULTIPLE)
        {
            if (useStreaming)
            {
                sendTimeout(timeout, m);
                opCode = opcode;  // Change what receiveMessage expects to see
                while(continuousReader.enabled)
                {
                    try
                    {
                        receiveResponseStream(timeout, m);
                    }
                    catch (ReaderException re)
                    {
                        if (re instanceof ReaderCodeException && ((ReaderCodeException)re).getCode() == FAULT_NO_TAGS_FOUND)
                        {
                            // just ignore no tags found response
                        }
                        else
                        {
                            throw re;
                        }
                    }
                }                                
            } 
            else
            {
                int count = 0;
                try
                {
                    response = sendTimeout(timeout + transportTimeout, m);
                }
                catch (ReaderCodeException re)
                {
                    if (re.getCode() == FAULT_NO_TAGS_FOUND)
                    {
                        return;
                    }
                    else if(re.getCode() == FAULT_MSG_INVALID_PARAMETER_VALUE)
                    {
                        throw re;
                    }
                }
                int numTags = m.getu32at(9);
                while (count < numTags)
                {
                    TagReadData tr[];
                    tr = cmdGetTagBuffer(tagMetadataFlagValues.keySet(), false, tagProtocol);
                    for (TagReadData t : tr)
                    {
                        t.readBase = System.currentTimeMillis();
                        t.antenna = antennaPortReverseMap.get(t.antenna);
                        if (null != t)
                        {
                        collectedTags.add(t);
                        }                        
                        count++;
                    }
                }//end of while
            }//end of else
        }//end of if
    }
    
    public byte[] msgSetupReadTagSingle(TagMetadataFlag metadataFlags, TagFilter filter, TagProtocol protocol, short timeout)
    {
        Message m = new Message();
        int optByte;
        m.setu8(MSG_OPCODE_READ_TAG_ID_SINGLE);
        m.setu16(timeout);
        optByte = m.writeIndex++;
        filterBytes(protocol, m, optByte, filter, 0, true);
        m.data[optByte] |= (byte) (SINGULATION_FLAG_METADATA_ENABLED);
        int metadatabits = tagMetadataFlagValues.get(metadataFlags);
        m.setu16(metadatabits);
        return m.data;
    }

    private void openSerialPort() throws ReaderException
    {
      if (!connected && serialDevice != null)
      {
             String javaRunTime = System.getProperty("java.runtime.name");
              if(javaRunTime.equalsIgnoreCase("Android Runtime"))
              {                  
                 if(serialDevice.startsWith("/dev")) //For android USB serial port communication 
                  {
                     
                      int deviceClass = AndroidUsbReflection.getDeviceClass();
                      if (deviceClass == 0)
                      {
                          st = new AndroidUSBTransport();
                      }
                      else
                      {
                          st = new AndroidUsbCdcAcmTransport();                          
                      }
                  }
                  else //For Android bluetooth communication
                  { 
                      st= new BluetoothTransportAndroid(serialDevice);
                  }
              }
          st.open();
      }
    }

   /**
    * Opens Serial Port and probes the module
    * @throws ReaderException
    */
    private void openPort() throws ReaderException
    {
       
      // Serial baud rates to try, in the order we're most likely to find them
      int[] bps = {9600, 115200, 921600, 19200, 38400, 57600, 230400, 460800};
      int bitRate=0;
      openSerialPort();
      versionInfo = null;
      boolean success = false;
      boolean isBaudRateOk = false;
      int index = 0;
      boolean isVersionError = true;
      for (int count=0;count<bps.length;count++)
      {
          isBaudRateOk = false;
          if(index < 2)
          {
               /* Try this first
              Module might be in deep sleep mode, if there is no response for the
              first attempt, Try the same baudrate again. count = 0 and count = 1 */
              bitRate = baudRate;
          }
          else
          {
              if(count == index)
              {
                  count = 0;
              }
              bitRate = bps[count];
              if (baudRate == bitRate)
              {
                  continue;//We already tried this one
              }
          }              
          st.setBaudRate(bitRate);
          index++;
          while(true)
          {
          try
          {
              // Reader, are you there?
              st.flush();
              versionInfo = cmdVersion();
              protocolSet = EnumSet.noneOf(TagProtocol.class);
              protocolSet.addAll(Arrays.asList(versionInfo.protocols));
              if (versionInfo.hardware.part1 == TMR_SR_MODEL_M6E||versionInfo.hardware.part1 == TMR_SR_MODEL_M6E_MICRO||versionInfo.hardware.part1 == TMR_SR_MODEL_M6E_PRC)
              {
                supportsPreamble = true;
              }
              baudRate = bitRate;
              break;
          }
          catch (ReaderCommException ex)
          {
                  if (ex.getMessage().contains("Device was reset externally") || ex.getMessage().contains("Reader failed crc check"))
                  {
                      if(isVersionError)
                      {
                      index = 0;
                          if(ex.getMessage().contains("Reader failed crc check"))
                          {
                            isCRCEnabled = false;
                  }
                          try
                          {
                              cmdStopContinuousRead(this, STR_FLUSH_READS);
                          }
                          catch (ReaderException re)
                          {
                              // Ignoring BUFFER FULL and DEVICE RESET
                          }
                          isVersionError = false;
                      }
                  }                  
                  if(ex.getMessage().startsWith("Timeout"))
                  {
                      isBaudRateOk = true;
                      break;
                  }
                  if(ex.getMessage().equals("Invalid M6e response header, SOH not found in response"))
                  {
                      //"Invalid M6e response header, SOH not found in response"
                  }
              // That didn't work.  Try the next speed, but remember
              // this exception, in case there's nothing left to try.
              continue;
          }
          catch (ReaderException re)
          {
              st.shutdown();
              throw re; // A error response to a version command is bad news
              }
          }
          if(isBaudRateOk)
          {
              continue;
          }
          success = true;
          break;
      }
      if (success == false)
      {
          st.shutdown();
          throw new ReaderCommException("No response from reader at any baud rate.");
      }
      if (versionInfo.hardware.equals(m4eType1) || versionInfo.hardware.equals(m4eType2))
      {
          model = TMR_READER_M4E;
      }
      else
      {
          switch (versionInfo.hardware.part1)
          {
              case TMR_SR_MODEL_M5E:
                  model = TMR_READER_M5E;
                  break;
              case TMR_SR_MODEL_M5E_COMPACT:
                  model = TMR_READER_M5E_COMPACT;
                  break;
              case TMR_SR_MODEL_M5E_I:
                  switch (versionInfo.hardware.part4)
                  {
                      case TMR_SR_MODEL_M5E_I_REV_EU:
                          model = TMR_READER_M5E_EU;
                          break;
                      case TMR_SR_MODEL_M5E_I_REV_NA:
                          model = TMR_READER_M5E_NA;
                          break;
                      case TMR_SR_MODEL_M5E_I_REV_JP:
                          model = TMR_READER_M5E_JP;
                          break;
                      case TMR_SR_MODEL_M5E_I_REV_PRC:
                          model = TMR_READER_M5E_PRC;
                          break;
                      default:
                          model = "Unknown";
                          break;
                  }
                  break;
              case TMR_SR_MODEL_M4E:
                  model = TMR_READER_M4E;
                  break;
              case TMR_SR_MODEL_M6E:
                  model = TMR_READER_M6E;
                  supportsPreamble = true;
                  break;
              case TMR_SR_MODEL_M6E_PRC:
                  model = TMR_READER_M6E_PRC;
                  supportsPreamble = true;
                  break;
              case TMR_SR_MODEL_M6E_MICRO:
                  model = TMR_READER_M6E_MICRO;
                  supportsPreamble = true;
                  break;
              default:
                  model = "Unknown";
                  break;
          }//END OF SWITCH
      }// end of else

    }//end of method
}//end of class

