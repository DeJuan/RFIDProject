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


import java.util.StringTokenizer;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.EnumSet;
import java.util.List;
import java.util.ArrayList;
import java.util.Set;
import java.util.Vector;
import org.apache.log4j.PropertyConfigurator;
import org.slf4j.LoggerFactory;
import org.slf4j.impl.Log4jLoggerAdapter;
import static com.thingmagic.TMConstants.*;




/**
 * The RqlReader class is an implementation of a Reader object that
 * communicates with a ThingMagic fixed RFID reader via the RQL
 * network protocol.
 * <p>
 * Instances of the RQL class are created with the Reader.create()
 * method with a 'rql' URI or a generic 'tmr' URI that references a
 * network device.
 */
public class RqlReader extends com.thingmagic.Reader
{
    private final static int[] _gpioBits = {0x04, 0x08, 0x10, 0x02, 0x20, 0x40, 0x80, 0x100};
    private static String[] _readFieldNames = null;    
    private final static String[] _astraReadFieldNames = "antenna_id read_count id frequency dspmicros protocol_id lqi".split(" ");
    private final static String[] _m5ReadFieldNames = "antenna_id read_count id frequency dspmicros protocol_id".split(" ");
    private final static String[] _readMetaData = "antenna_id read_count id metadata data protocol_id phase ".split(" ");

    private final int ANTENNA_ID = 0;
    private final int READ_COUNT = 1;
    private final int ID = 2;
    private final int FREQUENCY = 3;
    private final int DSPMICROS = 4;
    private final int PROTOCOL_ID = 5;
    private final int LQI = 6;
    private final int DATA = 4;
    private final int METADATA = 3;
    private final int PHASE = 6;

    private int _rfPowerMax;
    private int _rfPowerMin;


  // Connection state and fixed values
  int maxAntennas;
  boolean antennas[];
  Set<TagProtocol> protocolSet;
  String host;
  int port;
  List<TransportListener> listeners;
  boolean hasListeners;
  Socket rqlSock;
  BufferedReader rqlIn;
  BufferedWriter rqlOut;
  boolean isAstra; // for workarounds
  int[] gpiList, gpoList;
  static boolean _stopRequested;
  private ManualResetEvent _stopCompleted;
  private int _maxCursorTimeout;
  PowerMode powerMode = PowerMode.INVALID;
  String model;  

  private static Log4jLoggerAdapter rqlLogger;   
  static
  {
      rqlLogger = (Log4jLoggerAdapter) LoggerFactory.getLogger(LLRPReader.class);
      PropertyConfigurator.configure(LLRPReader.class.getClassLoader().getResource("log4j.properties"));
  }
  // Values affected by parameter operations
  int txPower;

    @Override
    public void reboot() 
    {
        throw new UnsupportedOperationException("Unsupported operation");
    }

  static class SimpleTransportListener implements TransportListener
  {
    public void message(boolean tx, byte[] data, int timeout)
    {
        System.out.println((tx ? "Sending:\n" : "Receiving:\n") + new String(data));
    }
  }
  static
  {
    simpleTransportListener = new SimpleTransportListener();
  }

  RqlReader(String host)
    throws ReaderException
  {
    this(host, 8080);
  }

  RqlReader(String host, int port)
    throws ReaderException
  {
    // Uncomment following to turn on debugging output
    //rqlLogger.setLevel(Level.FINE);
    //for (Handler handler : rqlLogger.getLogger("").getHandlers())
    //{
    //  handler.setLevel(rqlLogger.getLevel());
    //}

    this.host = host;
    this.port = port;

    listeners = new Vector<TransportListener>();
    rqlSock = new Socket();            
  }

    @Override
  public void connect()
    throws ReaderException
  {
    String version, serial;
    TagProtocol[] protocols=null;
    Setting s;
    SettingAction saCopy;
    int session;
    model=null;

    try
    {
      rqlSock.connect(new InetSocketAddress(host, port),this.transportTimeout);
      rqlIn = new BufferedReader(
      new InputStreamReader(rqlSock.getInputStream()));
      rqlOut = new BufferedWriter(
        new OutputStreamWriter(rqlSock.getOutputStream()));
      fullyResetRql();
      maxAntennas = Integer.parseInt(getField(TMR_RQL_RDR_AVAIL_ANTENNAS,
                                           TMR_RQL_PARAMS));
    }
    catch (java.net.UnknownHostException e)
    {
      throw new ReaderCommException(e.getMessage());
    }
    catch (IOException ioe)
    {
      throw new ReaderCommException(ioe.getMessage());
    }
    catch(Exception e)
    {
        throw new ReaderException(e.getMessage());
    }

    
    serial = getField(TMR_RQL_RDR_SERIAL, TMR_RQL_PARAMS);
    version = getField(TMR_RQL_VERSION, TMR_RQL_SETTINGS);
    try
    {
        //model = null;
        model = getField(TMR_RQL_MODEL, TMR_RQL_PARAMS);
    }
    catch(ReaderParseException re)
    {
      // Do nothing
        model = "Astra";
    }
    if (model.equals("Mercury6") || model.equals("Astra"))
    {
        gpiList = new int[]{3, 4, 6, 7};
        gpoList = new int[]{0, 1, 2, 5};
        _readFieldNames = _astraReadFieldNames;
        _rfPowerMax = 3150;
        _rfPowerMin = 500;
        if(model.equals("Astra"))
        {
          isAstra = true;
          _rfPowerMax = 3000;
          _rfPowerMin = 500;
        }
    }
    else if (model.equals("Mercury5"))
    {
        gpiList = new int[]{3, 4};
        gpoList = new int[]{0, 1, 2, 5};
        _readFieldNames = _m5ReadFieldNames;
        _rfPowerMax = 3250;
        _rfPowerMin = 500;
    }
    if (model.equals("Astra"))
    {
        protocolSet = EnumSet.noneOf(TagProtocol.class);
        protocolSet.add(TagProtocol.GEN2);
    }
    else
    {
        int pcount;
        String[] protocolIds;
        protocolIds = getField(TMR_RQL_SUPP_PROTOCOLS, TMR_RQL_SETTINGS).split(" ");
        pcount = 0;
        for (String id : protocolIds)
        {
            if (TagProtocol.getProtocol(id) != null)
            {
                pcount++;
            }
        }
        protocols = new TagProtocol[pcount];
        protocolSet = EnumSet.noneOf(TagProtocol.class);
        for (String id : protocolIds)
        {
            TagProtocol p;
            p = TagProtocol.getProtocol(id);
            if (p != null)
            {
                protocols[--pcount] = p;
                protocolSet.add(p);
            }
       }
    }
    txPower = Integer.parseInt(getField(TMR_RQL_TX_POWER, TMR_RQL_SAVED_SETTINGS));
    addParam(TMR_PARAM_VERSION_MODEL,
    String.class, model, false, null);
    if(!isAstra)
    {
        addParam(TMR_PARAM_VERSION_HARDWARE,
        String.class, null, false, new ReadOnlyAction()              
        {
            public Object get(Object value)
            throws ReaderException
            {
                      return getField(TMR_RQL_HARDWARE, TMR_RQL_PARAMS);
            }                 
         });
     }
    addParam(TMR_PARAM_VERSION_SOFTWARE,
                String.class, version, false, null);
    addParam(TMR_PARAM_LICENSE_KEY, byte[].class, null, true,
              new SettingAction()
      {
            public Object set(Object value) throws ReaderException
            {
                setField(TMR_RQL_LICENSEKEY, ReaderUtil.byteArrayToHexString((byte[])value), TMR_RQL_PARAMS);
                String response = getField(TMR_RQL_LICENSEKEY, TMR_RQL_PARAMS);
                if(response.contains("Success"))
                {
                    return null;
                }
                throw new ReaderException(response);
             }
             public Object get(Object value) throws ReaderException
             {
                return null;
             }
      });
    saCopy = new SettingAction()
    {
        public Object set(Object value)
        {
            return value;
        }
        public Object get(Object value)
        {
            return copyIntArray(value);
        }
    };
    addParam(TMR_PARAM_ANTENNA_PORTLIST,
              int[].class, null, false,
              new ReadOnlyAction() 
            {
                public Object get(Object value)
                {
                int[] antennaPorts = new int[maxAntennas];
                for (int i = 0; i < maxAntennas; i++)
                {
                          antennaPorts[i] = i + 1;
                }
                return antennaPorts;
                }                 
            });

    addParam(TMR_PARAM_GPIO_INPUTLIST,
                int[].class, gpiList, false, saCopy);

    addParam(TMR_PARAM_GPIO_OUTPUTLIST,
                int[].class, gpoList, false, saCopy);

    addParam(TMR_PARAM_VERSION_SUPPORTEDPROTOCOLS,
                TagProtocol[].class, protocols, false,
                    new SettingAction()
                    {
                      public Object set(Object value)
                      {
                        return value;
                      }
                      public Object get(Object value)
                      {
                        TagProtocol[] protos = (TagProtocol[])value;
                        TagProtocol[] protosCopy;
                        protosCopy = new TagProtocol[protos.length];
                        System.arraycopy(protos, 0, protosCopy, 0, protos.length);
                        return protosCopy;

                      }
    });

      addParam(TMR_PARAM_RADIO_POWERMIN,
              Integer.class, 0, false,
              new ReadOnlyAction()
              {
                  public Object get(Object value)
                  {
                      return _rfPowerMin;
                  }
              });

      addParam(TMR_PARAM_RADIO_POWERMAX,
              Integer.class, 0, false,
              new ReadOnlyAction()
              {
                  public Object get(Object value)
                  {
                      return _rfPowerMax;
                  }
              });
      addParam(TMR_PARAM_ANTENNA_CONNECTEDPORTLIST,
               int[].class, null, false,
               new SettingAction()
               {
                   public Object set(Object value)
                   {
                       return value;
                   }
                   public Object get(Object value)
                   throws ReaderException
                   {
                        // RQL doesn't have an antenna-detection
                        // mechanism. The reader knows, and will use
                        // those antennas if we don't specify
                        // anything, but the user can't do the same
                        // thing manually.
                        String antennas=null;
                        try
                        {
                            antennas = getField(TMR_RQL_RDR_CONN_ANTENNAS,TMR_RQL_PARAMS);
                        }
                        catch(ReaderParseException re)
                        {
                            if(model.equalsIgnoreCase("Astra"))
                            {
                                antennas="1";
                            }
                        }
                        int count=0;
                        String[] split = antennas.split(" ");
                        for (int i = 0; i < split.length; i++)
                        {
                            if(!split[i].startsWith("X"))
                            {
                                count++;
                            }
                        }
                        int[] list=new int[count];
                        int count1=0;
                        for (int i = 0; i < split.length; i++)
                        {
                            if(!split[i].startsWith("X"))
                            {
                                list[count1++] = Integer.parseInt(split[i]);
                            }
                        }
                      return list;
                   }
                });


    addParam(TMR_PARAM_RADIO_READPOWER,
                Integer.class, txPower, true,
                    new SettingAction()
                    {
                      public Object set(Object value)
                        throws ReaderException
                      {
                        int power = (Integer)value;
                        if (power < (Integer)paramGet(TMR_PARAM_RADIO_POWERMIN) ||
                            power > (Integer)paramGet(TMR_PARAM_RADIO_POWERMAX))
                        {
                          throw new IllegalArgumentException(
                            "Invalid power level " + value);
                        }
                        setTxPower(power);
                        return value;
                      }
                      public Object get(Object value)
                      {
                        return txPower;
                      }
    });

    addParam(TMR_PARAM_RADIO_WRITEPOWER,
                Integer.class, txPower, true,
                    new SettingAction()
                    {
                      public Object set(Object value)
                        throws ReaderException
                      {
                        int power = (Integer)value;
                        if (power < (Integer)paramGet(TMR_PARAM_RADIO_POWERMIN) ||
                            power > (Integer)paramGet(TMR_PARAM_RADIO_POWERMAX))
                        {
                          throw new IllegalArgumentException(
                            "Invalid power level " + value);
                        }
                        setTxPower(power);
                        return value;
                      }
                      public Object get(Object value)
                      {
                        return txPower;
                      }
    });

    addParam(TMR_PARAM_READ_PLAN,
                ReadPlan.class, new SimpleReadPlan(), true, null);

    addParam(TMR_PARAM_TAGOP_PROTOCOL,
                TagProtocol.class, TagProtocol.GEN2, true,
                    new SettingAction()
                    {
                      public Object set(Object value)
                      {
                        TagProtocol p = (TagProtocol)value;

                        if (!protocolSet.contains(p))
                          throw new IllegalArgumentException(
                            "Unsupported protocol " + p + ".");
                        return value;
                      }
                      public Object get(Object value)
                      {
                        return value;
                      }
    });

      addParam(TMR_PARAM_TAGOP_ANTENNA,
              Integer.class, getFirstConnectedAntenna(), true,
              new SettingAction()
               {
                   public Object set(Object value)
                   {
                       int a = (Integer) value;
                       if (a < 1 || a > maxAntennas)
                       {
                           throw new IllegalArgumentException(
                                   "Invalid antenna " + a + ".");
                       }
                       return value;
                  }

                  public Object get(Object value)
                  {
                      return value;
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
                      powerMode = (PowerMode) value;
                      setField(TMR_RQL_POWERMODE, Integer.toString(powerMode.value), TMR_RQL_PARAMS);
                      return powerMode;
                  }
                  public Object get(Object value)
                          throws ReaderException {
                      return getField(TMR_RQL_POWERMODE, TMR_RQL_PARAMS);
                  }
              });

    addParam(TMR_PARAM_CURRENTTIME,
              Date.class, null, false,
              new SettingAction() 
              {
                    public Object set(Object value) throws ReaderException
                    {
                    throw new UnsupportedOperationException("Not supported yet.");
                    }
                    public Object get(Object value)throws ReaderException
                    {
                        try
                        {
                          String rawDate = getField(TMR_RQL_CURRENTTIME, TMR_RQL_SETTINGS);
                          DateFormat dbFormatter = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSSSS");
                          Date scheduledDate = dbFormatter.parse(rawDate);
                          return scheduledDate;
                        }
                        catch (ParseException ex)
                        {
                            rqlLogger.error(ex.getMessage());
                            throw new ReaderException("Parse Exception occurred");
                        }
               }
              });


    addParam(TMR_PARAM_HOSTNAME, String.class, null, true,
              new SettingAction()
             {
                  public Object set(Object value)throws ReaderException
                  {
                      if (!value.equals(host))
                      {
                          setField(TMR_RQL_HOSTNAME, value.toString(), TMR_RQL_SAVED_SETTINGS);
                          host = value.toString();
                          return value;
                      }
                      else
                      {
                          return host;
                      }
                  }
                  public Object get(Object value)
                  {
                      String hostname="";
                      try
                      {
                        hostname = getField(TMR_RQL_HOSTNAME, TMR_RQL_SAVED_SETTINGS);
                      }
                      catch(ReaderException re)
                      {
                        if(re instanceof ReaderParseException)
                        {
                            hostname =  "";
                        }                          
                      }
                      return hostname;
                  }
              });

    addParam(TMR_PARAM_ANTENNAMODE, String.class, null, true,
              new SettingAction()
            {
                public Object set(Object value)throws ReaderException
                {
                      //setting is allowed for Astra not for M6, need to block M6 here
                      if (isAstra)
                      {
                          setField(TMR_RQL_ANTENNAMODE, value.toString(), TMR_RQL_SAVED_SETTINGS);
                          return value;
                      } 
                      else
                      {
                          throw new ReaderException("antenna mode can be set only for Astra");
                      }
                  }

                  public Object get(Object value)throws ReaderException
                  {
                       return getField(TMR_RQL_ANTENNAMODE, TMR_RQL_SAVED_SETTINGS);
                  }
              });
             isAstra = model.equalsIgnoreCase("Astra")? true: false;
              if(!isAstra)
              {
              addParam(TMR_PARAM_ANTENNA_CHECKPORT, Boolean.class, null, true,
              new SettingAction()
              {

                  public Object set(Object value)
                          throws ReaderException
                  {
                      setField(TMR_RQL_ANTENNA_SAFETY, value.toString(), TMR_RQL_PARAMS);
                      return value;
                  }

                  public Object get(Object value)
                          throws ReaderException
                  {
                      return getField(TMR_RQL_ANTENNA_SAFETY, TMR_RQL_PARAMS);
                  }
                });
               addParam(TMR_PARAM_ENABLE_SJC, Boolean.class, null, true,
               new SettingAction()
               {

                  public Object set(Object value)
                          throws ReaderException
                  {
                      if (!(model.equals("Mercury6")))
                      {
                          setField(TMR_RQL_SJC, value.toString(), TMR_RQL_PARAMS);
                      }
                      else
                          throw new ReaderException("Unsupported Operation for Mercury6");
                      return value;
                  }

                  public Object get(Object value)
                          throws ReaderException
                  {
                      return getField(TMR_RQL_SJC, TMR_RQL_PARAMS);
                  }
               });
               addParam(TMR_PARAM_GEN2_BLF,
               Gen2.LinkFrequency.class, null, true ,
               new SettingAction()
               {
                   public Object set(Object value) throws ReaderException
                   {
                   Gen2.LinkFrequency lf = (Gen2.LinkFrequency)value;
                   Integer freq = Gen2.LinkFrequency.get(lf);
                   if(model.equals("Mercury6") && lf == Gen2.LinkFrequency.LINK320KHZ)
                   {
                        throw new ReaderException("Link Frequency not supported");
                   }
                   setField(TMR_RQL_GEN2_BLF, freq.toString(), TMR_RQL_PARAMS);
                   return value;
                   }
                   public Object get(Object value) throws ReaderException
                   {
                        int frequency = Integer.parseInt(getField(TMR_RQL_GEN2_BLF, TMR_RQL_PARAMS));
                        return Gen2.LinkFrequency.getFrequency(frequency);
                   }
              });
              addParam(TMR_PARAM_ISO180006B_BLF,
              Iso180006b.LinkFrequency.class,Iso180006b.LinkFrequency.LINK160KHZ, true,
              new SettingAction()
               {
                    public Object set(Object value) throws ReaderException
                    {
                        Integer lf = Iso180006b.LinkFrequency.get((Iso180006b.LinkFrequency)value);
                        setField(TMR_RQL_ISO_BLF, lf.toString(), TMR_RQL_PARAMS);
                        return value;
                    }
                    public Object get(Object value) throws ReaderException
                    {
                        int frequency = Integer.parseInt(getField(TMR_RQL_ISO_BLF, TMR_RQL_PARAMS));
                        return Iso180006b.LinkFrequency.getFrequency(frequency);
                    }
                });
                 addParam(TMR_PARAM_GEN2_TAGENCODING,
            Gen2.TagEncoding.class, null, true,
              new SettingAction() {
                  public Object get(Object value)
                          throws ReaderException
                  {
                      return Integer.parseInt(getField(TMR_RQL_GEN2_TAGENCODING, TMR_RQL_PARAMS));
                  }

                  public Object set(Object value)
                          throws ReaderException
                  {
                      Integer val = Gen2.TagEncoding.get((Gen2.TagEncoding)value);
                      setField(TMR_RQL_GEN2_TAGENCODING, val.toString(), TMR_RQL_PARAMS);
                      return value;
                  }
              });
              addParam(TMR_PARAM_RADIO_TEMPERATURE,
                Integer.class, null, false,
                new ReadOnlyAction()
                {
                  public Object get(Object value)
                          throws ReaderException
                  {
                      return Integer.parseInt(getField(TMR_RQL_TEMPERATURE, TMR_RQL_PARAMS));
                  }
                });
                addParam(TMR_PARAM_GEN2_TARI,
                Gen2.Tari.class, null, true,
                new SettingAction()
                {
                  public Object get(Object value)
                          throws ReaderException
                  {
                      return Integer.parseInt(getField(TMR_RQL_GEN2_TARI, TMR_RQL_PARAMS));
                  }

                  public Object set(Object value)
                          throws ReaderException
                  {
                      Integer val = Gen2.Tari.getTari((Gen2.Tari)value);
                      setField(TMR_RQL_GEN2_TARI,val.toString(), TMR_RQL_PARAMS);
                      return value;
                  }
                });
               addParam(TMR_PARAM_RADIO_PORTREADPOWERLIST,
                int[][].class, null, true,
                new SettingAction() {
                  public Object get(Object value)
                          throws ReaderException
                  {
                        return getPortPowerArray(value,"read");
                  }

                  public Object set(Object value)
                          throws ReaderException
                  {
                      runQuery(setPortPowerArray(value, "read"));
                      return value;
                  }
                });

                addParam(TMR_PARAM_RADIO_PORTWRITEPOWERLIST,
                int[][].class, null, true,
                new SettingAction() {
                  public Object get(Object value)
                          throws ReaderException
                  {
                        return getPortPowerArray(value,"write");
                  }

                  public Object set(Object value)
                          throws ReaderException
                  {
                      runQuery(setPortPowerArray(value, "write"));
                      return value;
                  }
                });
              }
            addParam(TMR_PARAM_GEN2_SESSION,
            Gen2.Session.class, null, true,
            new SettingAction()
            {
                public Object set(Object value)
                        throws ReaderException
                      {
                        runQuery(String.format(
                                   "UPDATE params SET gen2Session='%d';",
                                   ((Gen2.Session)value).rep));
                        return value;
                      }
                      public Object get(Object value)
                        throws ReaderException
                      {
                        int s = Integer.parseInt(getField(TMR_RQL_GEN2_SESSION, TMR_RQL_PARAMS));
                        switch (s)
                        {
                          // -1 is Astra's way of saying "Use the module's value".
                          // That value is dependant on the user mode, so check it.
                        case -1: 
                          int mode = Integer.parseInt(getField(TMR_RQL_USERMODE, TMR_RQL_PARAMS));
                          if (mode == 3) // portal mode
                          {
                            return Gen2.Session.S1;
                          }
                          else
                          {
                            return Gen2.Session.S0; 
                          }
                        case 0: return Gen2.Session.S0;
                        case 1: return Gen2.Session.S1;
                        case 2: return Gen2.Session.S2;
                        case 3: return Gen2.Session.S3;
                        }
                        throw new ReaderParseException("Unknown Gen2 session value " + s);
                      }
            });
    addUnconfirmedParam(TMR_PARAM_GEN2_Q,
               Gen2.Q.class, null, true,
               new SettingAction()
               {
                 public Object set(Object value)
                   throws ReaderException
                 {
                   if (value instanceof Gen2.StaticQ)
                   { 
                       Gen2.StaticQ q = ((Gen2.StaticQ) value);
                       if (q.initialQ < 0 || q.initialQ > 15)
                       {
                           throw new IllegalArgumentException("Value of /reader/gen2/q is out of range. Should be between 0 and 15");
                       }
                       String qval = Integer.toString(q.initialQ);
                       setField(TMR_RQL_GEN2INITQ, qval, TMR_RQL_PARAMS);
                       setField(TMR_RQL_GEN2MINQ, qval, TMR_RQL_PARAMS);
                       setField(TMR_RQL_GEN2MAXQ, qval, TMR_RQL_PARAMS);
                   }
                   else
                   {
                     setField(TMR_RQL_GEN2MINQ, "2", TMR_RQL_PARAMS);
                     setField(TMR_RQL_GEN2MAXQ, "6", TMR_RQL_PARAMS);
                   }
                   return value;
                 }
                 public Object get(Object value)
                   throws ReaderException
                 {
                   int q = Integer.parseInt(
                     getField(TMR_RQL_GEN2INITQ, TMR_RQL_PARAMS));
                   int min = Integer.parseInt(
                     getField(TMR_RQL_GEN2MINQ, TMR_RQL_PARAMS));
                   int max = Integer.parseInt(
                     getField(TMR_RQL_GEN2MAXQ, TMR_RQL_PARAMS));
                   if (q == min && q == max)
                   {
                        if(q==-1)
                        {
                            return new Gen2.DynamicQ();
                        }
                        else
                        {
                            return new Gen2.StaticQ(q);
                        }
                   }
                   else
                   {
                        return new Gen2.DynamicQ();
                   }
                 }
               });
            
      addUnconfirmedParam(TMR_PARAM_GEN2_TARGET,
               Gen2.Target.class, null, true,
               new SettingAction()
               {
                 public Object set(Object value)
                   throws ReaderException
                 {
                   Gen2.Target t = (Gen2.Target)value;
                   int val;
                   switch (t)
                   {
                   case A:
                     val = 2;
                     break;
                   case B:
                     val = 3;
                     break;
                   case AB:
                     val = 0;
                     break;
                   case BA:
                     val = 1;
                     break;
                   default:
                     throw new IllegalArgumentException("Unknown target enum "
                                                        + t);
                   }
                   setField(TMR_RQL_GEN2TARGET, Integer.toString(val), TMR_RQL_PARAMS);
                   return value;
                 }
                 public Object get(Object value)
                   throws ReaderException
                 {
                   int val = Integer.parseInt(
                     getField(TMR_RQL_GEN2TARGET, TMR_RQL_PARAMS));
                   switch (val)
                   {
                     // -1 is Astra's way of saying "Use the module default".
                     // Take advantage of knowing that default.
                   case -1:
                     return Gen2.Target.A;
                   case 0:
                     return Gen2.Target.AB;
                   case 1:
                     return Gen2.Target.BA;
                   case 2:
                     return Gen2.Target.A;
                   case 3:
                     return Gen2.Target.B;
                   }
                   throw new ReaderParseException("Unknown target value " + 
                                                  val);
                 }
               });    

    // It's okay if this doesn't get set to anything other than NONE;
    // since the user doesn't need to set the region, the connection
    // shouldn't fail just because the region is unknown.
    Reader.Region region = Reader.Region.UNSPEC;

    String regionName = getField(TMR_RQL_REGIONNAME, TMR_RQL_PARAMS).toUpperCase();
    if (regionName.equals("US"))
    {
      region = Reader.Region.NA;
    }
    else if (regionName.equals("JP"))
    {
      region = Reader.Region.JP;
    }
    else if (regionName.equals("CN"))
    {
      region = Reader.Region.PRC;
    }
    else if (regionName.equals("IN"))
    {
      region = Reader.Region.IN;
    }
    else if (regionName.equals("AU"))
    {
      region = Reader.Region.AU;
    }
    else if (regionName.equals("NZ"))
    {
      region = Reader.Region.NZ;
    }
    else if (regionName.equals("KR"))
    {
      String regionVersion = getField(TMR_RQL_REGIONVERSION, TMR_RQL_PARAMS);
      if (regionVersion.equals("1") || regionVersion.equals(""))
      {
        region = Reader.Region.KR;
      }
      else if (regionVersion.equals("2"))
      {
        region = Reader.Region.KR2;
      }
    }
    else if (regionName.equals("EU"))
    {
      String regionVersion = getField(TMR_RQL_REGIONVERSION, TMR_RQL_PARAMS);
      if (regionVersion.equals("1") || regionVersion.equals(""))
      {
        region = Reader.Region.EU;
      }
      else if (regionVersion.equals("2"))
      {
        region = Reader.Region.EU2;
      }
      else if (regionVersion.equals("3"))
      {
        region = Reader.Region.EU3;
      }
    }

    addParam(TMR_PARAM_REGION_SUPPORTEDREGIONS,
             Reader.Region[].class, new Reader.Region[] {region}, true, null);

    addParam(TMR_PARAM_REGION_ID,
             Reader.Region.class, region, true,
             new ReadOnlyAction()
             {
               public Object get(Object value)
                 throws ReaderException
               {
                 return value;
               }
             });

        addParam(TMR_PARAM_READER_DESCRIPTION, String.class, null, true,
              new SettingAction() {

                  public Object set(Object value)
                          throws ReaderException {
                      setField(TMR_RQL_READER_DESC, value.toString(), TMR_RQL_PARAMS);
                      return value.toString();
                  }

                  public Object get(Object value)
                  {
                    String description="";
                    try
                    {
                        description = getField(TMR_RQL_READER_DESC, TMR_RQL_PARAMS);
                    }
                    catch(ReaderException re)
                    {
                        if(re instanceof ReaderParseException)
                        {
                            description="";
                        }
                    }
                    return description;
                  }
              });

            addParam(TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA, Boolean.class, false, true,
              new SettingAction()
              {

                  public Object set(Object value)
                          throws ReaderException
                  {                      
                      return value;
                  }

                  public Object get(Object value)
                          throws ReaderException
                  {
                      return value;
                  }
              });

              addParam(TMR_PARAM_TAGREADDATA_UNIQUEBYDATA, Boolean.class, false, true,
              new SettingAction()
              {

                  public Object set(Object value)
                          throws ReaderException
                  {                      
                      return value;
                  }

                  public Object get(Object value)
                          throws ReaderException
                  {
                      return value;
                  }
              });
              addParam(TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI, Boolean.class, false, true,
              new SettingAction()
              {

                  public Object set(Object value)
                          throws ReaderException
                  {                      
                      return value;
                  }

                  public Object get(Object value)
                          throws ReaderException
                  {
                      return value;
                  }
              });
            
              addParam(TMR_PARAM_ENABLE_READ_FILTERING, Boolean.class, true, false,
                    new ReadOnlyAction()
                    {
                        @Override
                        public Object get(Object value) throws ReaderException
                        {
                            return true;
                        }
                    });
              addParam(TMR_PARAM_TAGREADDATA_UNIQUEBYPROTOCOL, Boolean.class, true, false,
                  new ReadOnlyAction()
                  {
                      public Object get(Object value)
                              throws ReaderException
                      {
                          return true;
                      }
                  });

        connected = true;
}

  
  
  /**
   * Start reading RFID tags in the background. The tags found will be passed 
   * to the registered read listeners, and any exceptions that occur during reading 
   * will be passed to the registered exception listeners. Reading will continue 
   * until stopReading() is called. 
   */

    

    @Override
    public void startReading() {
        try
        {
            setTxPower((Integer) paramGet(TMR_PARAM_RADIO_READPOWER));
            ReadPlan rp = (ReadPlan) paramGet(TMR_PARAM_READ_PLAN);
            int ontime = (Integer) paramGet(TMR_PARAM_READ_ASYNCONTIME);
            int offtime = (Integer) paramGet(TMR_PARAM_READ_ASYNCOFFTIME);

            resetRql();
            ArrayList<Integer> ctimes = new ArrayList<Integer>();
            List<String> cnames = setUpCursors(rp, ontime, ctimes);

            startAutoMode(cnames, ontime, offtime);
            startReceiver(ctimes);
        } 
        catch (ReaderException ex)
        {
            rqlLogger.error(ex.getMessage());
        }        
    }

    @Override
    public boolean stopReading() {
         _stopRequested = true;
           if (null != _stopCompleted)
           {
            try
            {
                _stopCompleted.waitOne();
                _stopCompleted = null;
            } 
            catch (InterruptedException ex)
            {
                rqlLogger.error(ex.getMessage());
                return false;
            }
        }
        return true;
    }


    void resetRql() throws ReaderException
    {
        runQuery("RESET", false);
    }

    /*
     * Reset RQL, even if it's already in auto mode.
     * In addition to sending the RESET command, keep watching
     * until auto mode has come to a full stop.
     */
    void fullyResetRql() throws ReaderException
    {
        // Reset RQL and ask for a known response to confirm stoppage
        // SET AUTO=OFF is required before RESET to prevent RQL from spitting up
        // extra empty lines after responding to SELECT rql_version.
        //
        // DO NOT split these into separate queries.  We want to stop RQL, no
        // matter what -- trying to receive responses in between commands allows
        // interruptions from failed receives.
        sendQuery("SET AUTO=OFF; RESET; SELECT rql_version FROM firmware");

        // Wait for response of the form "rql Built 2011-02-16T16:54:39-0500\n\n"
        // Don't have to worry about cursor timeouts, because we sent AUTO=OFF
        // a couple of command cycles ago, so all tag reads should have flushed
        // out by now.
        //
        // Need to receive a few response packets to get command-response sequence
        // back in sync, but shouldn't try longer than a single transport timeout,
        // since the stream is flowing pretty steadily.
        // (NOTE: In non-continuous mode, tag read responses are delayed until
        //  the current cursor finishes, but there's no way to know that timeout
        //  so we just let it fail.)
        long startTime = System.currentTimeMillis();
        long timeout = (Integer)paramGet("/reader/transportTimeout");
        long endTime = startTime + timeout;
        while (System.currentTimeMillis() <= endTime)
        {
            String[] lines = receiveBatch(commandTimeout, false);
            if (lines.length > 0)
            {
                if ((lines[lines.length-1]).startsWith("rql Built"))
                {
                    break;
                }
            }
        }
    }

    /*
     * @param rp  Read plan to create cursors from
     * @param timeout  Overall timeout for entire read plan
     * @param ctimesOut  Output list of individual cursor timeouts.  Pass in an existing list to be appended to.
     * @return List of cursor names
     */
    private List<String> setUpCursors(ReadPlan rp, int timeout, List<Integer>ctimesOut) throws ReaderException
    {
        List<String> cnames = new ArrayList<String>();

        List<String> rql = GenerateRql(rp, timeout, ctimesOut);

        int cnum = 0;
        for (int i = 0; i < rql.size(); i++)
        {
            String line = rql.get(i);
            cnum++;
            String cname = String.format("mapic%d", cnum);
            cnames.add(cname);
            String decl = String.format(
                    "DECLARE %s CURSOR FOR %s", cname, line);
            runQuery(decl);
        }        
        return cnames;
    }

    /*
     * @param rp  Read plan to create RQL from
     * @param milliseconds  Overall timeout for entire read plan
     * @param ctimesOut  Output list of individual cursor timeouts.  Pass in an existing list to be appended to.
     * @return  List of RQL subqueries
     */
    private List<String> GenerateRql(ReadPlan rp, int milliseconds, List<Integer>ctimesOut) {

        List<String> queries = new ArrayList<String>();

        if (rp instanceof MultiReadPlan)
        {
            MultiReadPlan mrp = (MultiReadPlan) rp;
            for (ReadPlan r : mrp.plans)
            {
                // Ideally, totalWeight=0 would allow reader to
                // dynamically adjust timing based on tags observed.
                // For now, just divide equally.
                int subtimeout =
                    (0 != mrp.totalWeight) ? (int) milliseconds * r.weight / mrp.totalWeight
                    : milliseconds / mrp.plans.length;
                subtimeout = Math.min(subtimeout, Integer.MAX_VALUE);
                queries.addAll(GenerateRql(r, subtimeout, ctimesOut));
            }
        }
        else if (rp instanceof SimpleReadPlan)
        {
            List<String> wheres = new ArrayList<String>();
            wheres.addAll(readPlanToWhereClause(rp));
            if(((SimpleReadPlan)rp).filter!=null)
            {
                wheres.addAll(tagFilterToWhereClause(((SimpleReadPlan) rp).filter));
            }
            TagOp op = ((SimpleReadPlan)rp).Op;
            String query;
            if(op!=null && op instanceof Gen2.ReadData)
            {
                query = makeSelect(_readMetaData, TMR_RQL_TAG_ID, wheres, milliseconds);                
            }
            else
            {
                query = makeSelect(_readFieldNames, TMR_RQL_TAG_ID, wheres, milliseconds);                
            }
            queries.add(query);
            ctimesOut.add(milliseconds);
        } 
        else
        {
            throw new IllegalArgumentException("Unrecognized /reader/read/plan type ");
        }
        return queries;
    }


    private String makeSelect(String[] fields, String table, List<String> wheres, int timeout) {
        List<String> words = new ArrayList<String>();

        words.add("SELECT ");
        words.add(ReaderUtil.arrayToString(fields,","));
        words.add(" FROM ");
        words.add(table);
        if(wheres!=null)
        {
            words.add(wheresToRql(wheres));
        }

        if (-1 < timeout) {
            words.add(" SET time_out=" + timeout + ";");
        }

        //rqlLogger.log(Level.INFO, ReaderUtil.convertListToString(words,null));
        return ReaderUtil.convertListToString(words,null);
    }

    private String makeUpdate(String field, String table, Object value,List<String> wheres, int timeout) {
        List<String> words = new ArrayList<String>();

        words.add("UPDATE ");
        words.add(table);
        words.add(" SET ");
        if(value instanceof String)
        {
            words.add(field + " =0x" +  value);
        }
        else
        {
            words.add(field + " =" + value);
        }
        
        words.add(wheresToRql(wheres));       

        //rqlLogger.log(Level.INFO, ReaderUtil.convertListToString(words,null));
        return ReaderUtil.convertListToString(words,null);
    }

    private String wheresToRql(List<String> wheres) {
        List<String> words = new ArrayList<String>();

        if ((null != wheres) && (0 < wheres.size())) {
            words.add(String.format(" WHERE %s ", ((ReaderUtil.convertListToString(wheres," AND ")) )));
        }
        else
        {
            return null;
        }

        return ReaderUtil.convertListToString(words,null);
    }

    private List<String> tagFilterToWhereClause(TagFilter tagFilter)
    {
        List<String> wheres = new ArrayList<String>();

        if(tagFilter == null)
        {
            return null;
        }
        else if(tagFilter instanceof TagData)
        {
            //works for any TagData say Gen2 TagData and ISO TagData
            TagData td = (TagData) tagFilter;
            wheres.add(String.format("id=0x%s", ReaderUtil.byteArrayToHexString(td.epcBytes())));
        }        
        else if(tagFilter instanceof Gen2.Select)
        {
            Gen2.Select sel = (Gen2.Select) tagFilter;
            wheres.add("filter_subtype=0");
            wheres.add(String.format("filter_mod_flags=0x%s", (sel.invert) ? "00000001" : "00000000"));
            wheres.add(String.format("filter_bank=%s", sel.bank.rep));
            wheres.add("filter_bit_address=" + sel.bitPointer);
            wheres.add("filter_bit_length=" + sel.bitLength);
            if(sel.mask!=null)
            {
                wheres.add("filter_data=0x" + ReaderUtil.byteArrayToHexString(sel.mask));
            }

        }
        else if(tagFilter instanceof Iso180006b.Select)
        {
            Iso180006b.Select sel = (Iso180006b.Select) tagFilter;
            wheres.add("filter_subtype=0");
            String operation = "";

            if(sel.op.equals(Iso180006b.SelectOp.EQUALS))
            {
                operation = "00";
            }
            else if(sel.op.equals(Iso180006b.SelectOp.NOTEQUALS))
            {
                operation = "01";
            }
            else if(sel.op.equals(Iso180006b.SelectOp.GREATERTHAN))
            {
                operation = "02";
            }
            else if(sel.op.equals(Iso180006b.SelectOp.LESSTHAN))
            {
                operation = "03";
            }
            
            wheres.add(String.format("filter_command=%s", operation));
            wheres.add(String.format("filter_mod_flags=0x%s", (sel.invert) ? "00000001" : "00000000"));
            wheres.add("filter_bit_address=" + sel.address);
            wheres.add("filter_bit_length=64");
            wheres.add("filter_mask=0xff");
            if(sel.data!=null)
            {
                wheres.add("filter_data=0x" + ReaderUtil.byteArrayToHexString(sel.data));
            }
        }
        else
        {
            throw new IllegalArgumentException("RQL only supports singulation by EPC. " + tagFilter.toString() + " is not supported.");
        }        
        return wheres;
    }

    /*
     * Form the where clause with tag protocol and antennas
     * @param ReadPlan
     *
     */

    private List<String> readPlanToWhereClause(ReadPlan readPlan)
    {
        List<String> wheres = new ArrayList<String>();

        if (readPlan instanceof SimpleReadPlan)
        {
            SimpleReadPlan srp = (SimpleReadPlan) readPlan;
            wheres.addAll(tagProtocolToWhereClause(srp.protocol));
            wheres.addAll(antennasToWhereClause(srp.antennas));
            if (null != srp.Op && srp.Op  instanceof  Gen2.ReadData)
            {
                wheres.add(String.format("mem_bank=%d", ((Gen2.ReadData)srp.Op).Bank.rep));
                wheres.add(String.format("block_count=%d", ((Gen2.ReadData) srp.Op).Len));
                wheres.add(String.format("block_number=%d", ((Gen2.ReadData) srp.Op).WordAddress).toString());
            }
        }
        else
        {
            throw new IllegalArgumentException("Unrecognized /reader/read/plan type ");
        }
        return wheres;
    }

    private List<String> tagProtocolToWhereClause(TagProtocol tp)
    {
        List<String> wheres = new ArrayList<String>();
        String clause;

        switch (tp)
        {
            case GEN2:
                clause = "GEN2";
                break;
            case ISO180006B:
                clause = "ISO18000-6B";
                break;
            case IPX64:
                clause = "IPX64";
                break;
            case IPX256:
                clause = "IPX256";
                break;
            default:
                throw new IllegalArgumentException("Unrecognized protocol " + tp.toString());
        }
        wheres.add(String.format("protocol_id='%s'", clause));
        return wheres;
    }


    private List<String> antennasToWhereClause(int[] ants)
    {
        List<String> wheres = new ArrayList<String>();

        if ((null != ants) && (0 < ants.length))
        {
            StringBuilder sb = new StringBuilder();
            //sb.append(" AND ");
            List<String> antwords = new ArrayList<String>();

            for (int ant : ants) {
                antwords.add(antennaToRql(ant));
            }
            if(ants.length == 1){
                sb.append(ReaderUtil.convertListToString(antwords,null));
            }else{
                sb.append("(");
                sb.append(ReaderUtil.convertListToString(antwords," OR "));
                sb.append(")");
            }
            
            wheres.add(sb.toString());
        }
        return wheres;
    }

    private String antennaToRql(int ant)
    {
        return String.format("antenna_id=%d", ant);
    }

    private void startAutoMode(List<String> cnames, int ontime, int offtime) throws ReaderException {
       
        String cmd;

        int repeatTime = 0;
        if (0 < offtime) {
            repeatTime = ontime + offtime;
        }
        cmd = String.format("SET repeat=%d", repeatTime);
        runQuery(cmd, false);

        cmd = String.format("SET AUTO %s=ON", (ReaderUtil.convertListToString(cnames,",")));
        runQuery(cmd, false);


    }

    /**
     *  Receiver Thread
     * @param ctimes
     * @throws ReaderException
     */
    private void startReceiver(List<Integer> ctimes) throws ReaderException
    {
        _stopRequested = false;
        _stopCompleted = new ManualResetEvent(false);
        _maxCursorTimeout = 0;
        for(int ctime : ctimes)
        {
            _maxCursorTimeout = Math.max(_maxCursorTimeout, ctime);
        }
        Thread recvThread = new Thread() {

            @Override
            public void run() 
            {
                try
                {
                    int offTimeout;
                    String[] rows;
                    Date baseTime;

                    while (false == _stopRequested)                    
                    {
                        baseTime = new Date();                        
                        rows = receiveBatch(_maxCursorTimeout);
                        notifyBatchListeners(rows, baseTime);
                    }
                    
                    sendQuery("SET AUTO=OFF");
                    baseTime = new Date();

                    int zeroCount;
                    int safetyCount;

                    // 1 zeroCount response is for the AUTO=OFF response.
                    // 1 zeroCount response is for the final tag bundle.
                    zeroCount = 2;
                    offTimeout = _maxCursorTimeout;

                    /* safetyCount keeps us from spinning indefinitely,but we don't want it to interfere with
                    normal termination, so put it safely out of the way */
                    safetyCount = 10;
                    while ((0 < zeroCount) && (0 < safetyCount))
                    {
                        try
                        {
                            rows = receiveBatch(offTimeout, false);
                            if (0 == rows.length)
                            {
                                --zeroCount;
                            }
                            notifyBatchListeners(rows, baseTime);
                            --safetyCount;
                            offTimeout =  0;
                        }
                        catch (ReaderCommException e)
                        {
                            if (e.getMessage().equals("Read timed out"))
                            {
                                break;
                            }
                            else
                            {
                                throw e;
                            }
                        }
                    }//end while
                                        
                }
                catch (ReaderException re)
                {                    
                    rqlLogger.error(re.getMessage());
                    notifyExceptionListeners(re);                    
                }
                finally
                {
                    try
                    {
                        resetRql();
                    } 
                    catch (ReaderException ex)
                    {
                        rqlLogger.error(ex.getMessage());
                    }
                    _stopCompleted.set();
                }
            } //end thread run method
        };
        recvThread.start();
    }

    /**
     * notify listeners
     * @param rows
     * @param baseTime
     * @throws ReaderException
     */
    private void notifyBatchListeners(String[] rows, Date baseTime) throws ReaderException
    {
        for (String row : rows)
        {
            if (0 < row.length())
            {
                try
                {
                    TagReadData trd = parseRqlResponse(row, baseTime);
                    notifyReadListeners(trd);
                }
                catch (Exception ex)
                {
                    rqlLogger.error("Tag Read parse failed on row \""+row+"\"");
                    throw new ReaderCommException("Error \""+ex.getMessage()+"\" parsing tag read row \""+row+"\"");
                }
            }
        }
    }



    private TagReadData parseRqlResponse(String row, Date baseTime) throws ReaderException
    {
        StringTokenizer sr = new StringTokenizer(row, "|");
        String[] fields = new String[sr.countTokens()];

        int i = 0;
        while (sr.hasMoreTokens())
        {
            fields[i++] = sr.nextToken();
        }

        if (_readFieldNames.length != fields.length)
        {
            throw new ReaderParseException(String.format("Unrecognized format."
                    + "  Got %d fields in RQL response, expected %d.",
                    fields.length, _readFieldNames.length));
        }
       
        TagProtocol proto = TagProtocol.getProtocol(fields[PROTOCOL_ID]);
        byte[] epccrc = ReaderUtil.hexStringToByteArray(fields[ID].substring(2));
        String epcString = ReaderUtil.byteArrayToHexString(epccrc);

        //System.out.println("@@@@@@@ EPC STRING FOUND " + epcString);
        
        byte[] epc = new byte[epccrc.length - 2];
        System.arraycopy(epccrc, 0, epc, 0, epc.length);

        byte[] crc = new byte[2];
        System.arraycopy(epccrc, epc.length, crc, 0, 2);

                
        TagData tag = null;

        switch (proto)
        {
            case GEN2:                
                byte[] pcbits = null;
                if(fields[DATA].startsWith("0x"))
                {
                    pcbits = ReaderUtil.hexStringToByteArray(fields[METADATA]);
                    tag = new Gen2.TagData(epc,crc,pcbits);
                }
                else
                {
                    tag = new Gen2.TagData(epc, crc);
                }
                break;
            case ISO180006B:
                tag = new Iso180006b.TagData(epc, crc);
                break;
            case IPX64:
                tag = new Ipx64.TagData(epc, crc);
                break;
            case IPX256:
                tag = new Ipx256.TagData(epc, crc);
                break;
            default:
                throw new ReaderParseException("Unknown protocol code " + fields[PROTOCOL_ID]);
        }

        int antenna = Integer.parseInt(fields[ANTENNA_ID]);
        TagReadData tr = new TagReadData();
        tr.tag = tag;
        tr.antenna = antenna;
        //tr.df = baseTime;        
        tr.readCount = Integer.parseInt(fields[READ_COUNT]);
        tr.reader = this;
        tr.readBase = System.currentTimeMillis();
        if(fields[DATA].startsWith("0x"))
        {
            tr.phase = Integer.parseInt(fields[PHASE]);
            tr.data = ReaderUtil.hexStringToByteArray(fields[DATA].substring(2));
            //tr.metadataFlags.
        }
        else
        {
            tr.frequency = Integer.parseInt(fields[FREQUENCY]);
            tr.readOffset = Integer.parseInt(fields[DSPMICROS]) / 1000;

            if (!model.equals(TMR_READER_M5))
            {
                tr.rssi = Integer.parseInt(fields[LQI]);
            }

        }
       
        return tr;
    }

    /**
     * returns first connected antenna
     * @return 
     */
    private int getFirstConnectedAntenna() throws ReaderException
    {
        int[] validAnts = (int[]) paramGet(TMR_PARAM_ANTENNA_CONNECTEDPORTLIST);

        if (0 < validAnts.length)
        {
            return validAnts[0];
        }
        return 0;

    }          

  /**
   * The device power mode, for use in the /reader/powerMode
   * parameter.
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
      default: return null;
      }
    }
  }



   /**
   * The device antenna mode, for use in the /reader/antennaMode
   *
   */
  public enum AntennaMode
  {
      INVALID(-1),
      MONOSTATIC(0),
      BISTATIC(1);
      
      int value;

      AntennaMode(int v)
      {
          value = v;
      }

      static AntennaMode getAntennaMode(int p)
      {
          switch (p)
          {
              case -1:
                  return INVALID;
              case 0:
                  return MONOSTATIC;
              case 1:
                  return BISTATIC;
              default:
                  return null;
          }
    }
  }


  void setSoTimeout(int timeout)
  {
    try
    {
      rqlSock.setSoTimeout(timeout);
    }
    catch (java.net.SocketException se)
    {
      // this is a "shouldn't happen" - convert to a major error
      throw new java.io.IOError(se);
    }
  }


  String singulationString(TagFilter t)
  {
    String s;
    byte[] data;

    if (t == null)
      s = "";
    else
    {
      if (!(t instanceof TagData))
      {
        throw new IllegalArgumentException("RQL only supports singulation by EPC");
      }
      s = String.format("AND id=0x%s", ((TagData)t).epcString());
    }

    return s;
  }

  int[] copyIntArray(Object value)
  {
    int[] original = (int[])value;
    int[] copy = new int[original.length];
    System.arraycopy(original, 0, copy, 0, original.length);
    return copy;
  }

  public void addTransportListener(TransportListener listener)
  {
    if(null != listener)
    {
        listeners.add(listener);
        hasListeners = true;
    }
  }


  public void removeTransportListener(TransportListener l)
  {
    listeners.remove(l);
    if (listeners.isEmpty())
      hasListeners = false;
  }

    @Override
    public void addStatusListener(StatusListener listener)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void removeStatusListener(StatusListener listener)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void addStatsListener(StatsListener listener)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public void removeStatsListener(StatsListener listener)
    {
        throw new UnsupportedOperationException("Not supported yet.");
    }

  void notifyListeners(String s, boolean tx, int timeout)
  {
    byte[] msg = s.getBytes();
    for (TransportListener l : listeners)
    {
      l.message(tx, msg, timeout);
    }
  }

  void notifyListeners(List<String> strings, boolean tx, int timeout)
  {
    StringBuilder sb = new StringBuilder();

    for (String s : strings)
    {
      sb.append(s);
      sb.append("\n");
    }
    sb.append("\n");
    
    notifyListeners(sb.toString(), tx, timeout);
  }


  String[] runQuery(String q)
    throws ReaderException
  {
    return runQuery(q, false);
  }

  synchronized String[] runQuery(String query, boolean permitEmptyResponse)
    throws ReaderException
  {
    sendQuery(query);
    return receiveBatch(commandTimeout, permitEmptyResponse);
  }

  synchronized void sendQuery(String query)
    throws ReaderException
  {
    setSoTimeout((Integer)paramGet(TMR_PARAM_TRANSPORTTIMEOUT));
    if (!query.endsWith(";"))
    {
      query += ";\n";
    }
    if (hasListeners)
    {
      notifyListeners(query, true, 0);
    }
    try
    {
      if(rqlOut!=null)
      {
        rqlOut.write(query);
        rqlOut.write('\n');
        rqlOut.flush();
        rqlLogger.info("rqlOut wrote \""+query+"\"");
      }
      else
      {
        throw new ReaderCommException("rqlOut is null");
      }
    }
    catch (SocketException se)
    {
        //Socket Communication Error
        throw new ReaderCommException(se.getMessage());
    }
    catch (IOException e)
    {
      throw new ReaderCommException(e.getMessage());
    }
  }

    private String[] receiveBatch(int cmdTimeout) throws ReaderException
    {
        return receiveBatch(cmdTimeout, false);
    }

    /// <summary>
    /// Receive multi-line response (terminated by empty line)
    /// </summary>
    /// <param name="cmdTimeout">Milliseconds of inactivity allowed before timing out receive</param>
    /// <param name="permitEmptyResponse"></param>
    /// <returns></returns>
    private String[] receiveBatch(int cmdTimeout, boolean permitEmptyResponse) throws ReaderException
    {

        int transTimeout = (Integer) paramGet(TMR_PARAM_TRANSPORTTIMEOUT);
        int recvTimeout = transTimeout + cmdTimeout;
        //int recvTimeout = 500;
        List<String> response = new ArrayList<String>();

        try
        {
            if (!rqlSock.isClosed())
            {
                rqlSock.setSoTimeout(recvTimeout);
            }            

            String line = null;
            boolean done = false;
            while (!done)
            {
                line = rqlIn.readLine();

                if (line != null && line.startsWith("Error"))
                {                    
                    try
                    {
                        rqlIn.readLine();
                        throw new ReaderException(line);
                    }
                    catch(ReaderException re)
                    {
                        throw new ReaderException(line);
                    }
                }
                if (line != null && (permitEmptyResponse || !line.equals("")))
                {
                    response.add(line);
                    permitEmptyResponse = false;
                }
                else
                {
                    done = true;
                }
            }
        } catch (SocketException ex)
        {
            throw new ReaderCommException(ex.getMessage(), ReaderUtil.convertListToBytes(response));
        } catch (IOException ioe)
        {
            throw new ReaderCommException(ioe.getMessage(), ReaderUtil.convertListToBytes(response));
        }
    notifyListeners(response, false, recvTimeout);
    cleanLeadingNewline(response);
    return response.toArray(new String[0]);
}

    /*
     * strip off new line
     * @param response
     */
    private void cleanLeadingNewline(List<String> response)
    {
        // If response has at least two lines (leading newline, plus some content)
        String[] strResponse = response.toArray(new String[0]);
        if (2 <= response.size()) {
            // If first line is leading newline and second isn't a terminating newline

            if (0 == strResponse[0].length()
                    && 0 <= strResponse[1].length()) {
                // Strip off the spurious leading newline
                response.remove(0);
            }
        }
    }

  String getTable(String field)
  {
    if (field.equals(TMR_RQL_UHF_POWER) ||
        field.equals(TMR_RQL_TX_POWER) ||
        field.equals(TMR_RQL_HOSTNAME) ||
        field.equals(TMR_RQL_IFACE) ||
        field.equals(TMR_RQL_DHCPCD) ||
        field.equals(TMR_RQL_IP_ADDRESS) ||
        field.equals(TMR_RQL_NETMASK) ||
        field.equals(TMR_RQL_GATEWAY) ||
        field.equals(TMR_RQL_NTP_SERVERS) ||
        field.equals(TMR_RQL_EPC_ID_LEN) ||
        field.equals(TMR_RQL_PRIMARY_DNS) ||
        field.equals(TMR_RQL_SECONDARY_DNS) ||
        field.equals(TMR_RQL_DOMAIN_NAME) ||
        field.equals(TMR_RQL_READER_DESC) ||
        field.equals(TMR_RQL_READER_ROLE) ||
        field.equals(TMR_RQL_ANT1_READPOINT) ||
        field.equals(TMR_RQL_ANT2_READPOINT) ||
        field.equals(TMR_RQL_ANT3_READPOINT) ||
        field.equals(TMR_RQL_ANT4_READPOINT))
    {
      return TMR_RQL_SAVED_SETTINGS;
    }
    else
    {
      return TMR_RQL_PARAMS;
    }
  }
  /**
   * Get a value from the underlying RQL/device configuration table.
   *
   * @param field the name of configuration field to look up
   * @return the value of the field as a string
   */
  public String cmdGetParam(String field)
    throws ReaderException
  {
    return getFieldInternal(field, getTable(field));
  }

  /**
   * Set a value in the underlying RQL/device configuration table.
   *
   * @param field The name of configuration field to set
   * @param value The value of the configuration parameter.
   */
  public void cmdSetParam(String field, String value)
    throws ReaderException
  {
    setField(field, value, getTable(field));
  }

  // Possible "RQL injection attack" - don't make this public
  // without sanitizing params.
  String getFieldInternal(String field, String table)
    throws ReaderException
  {
    String[] ret;

    ret = runQuery("SELECT " + field + " from " + table + ";", true);

    return ret[0];
  }
  public  Object executeTagOp(TagOp tagOP, TagFilter target) throws ReaderException
  {

       TagProtocol protocolID = (TagProtocol)paramGet(TMR_PARAM_TAGOP_PROTOCOL);
      try{
      if (tagOP instanceof Gen2.ReadData) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
          return readTagMemWords(target, ((Gen2.ReadData) tagOP).Bank.rep, ((Gen2.ReadData) tagOP).WordAddress, ((Gen2.ReadData) tagOP).Len);
      } else if (tagOP instanceof Gen2.WriteData) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
          writeTagMemWords(target, ((Gen2.WriteData) tagOP).Bank.rep, ((Gen2.WriteData) tagOP).WordAddress, ((Gen2.WriteData) tagOP).Data);
          return null;
      } else if(tagOP instanceof Gen2.WriteTag){
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
          writeTag(target, ((Gen2.WriteTag)tagOP).Epc);
          return null;
      }else if (tagOP instanceof Gen2.Lock) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
          Gen2.Password oldPassword = (Gen2.Password)(paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD));
          boolean needRestorePassword = false;
          try
          {

              if (((Gen2.Lock) tagOP).AccessPassword != 0)
              {
                  needRestorePassword = true;
                  paramSet(TMR_PARAM_GEN2_ACCESSPASSWORD, new Gen2.Password(((Gen2.Lock) tagOP).AccessPassword));
              }
              lockTag(target, new Gen2.LockAction(((Gen2.Lock) tagOP).Action));
          } 
          finally
          {
              if(needRestorePassword)
              {
                paramSet(TMR_PARAM_GEN2_ACCESSPASSWORD, oldPassword);
              }              
          }                  
          return null;
      } else if (tagOP instanceof Gen2.Kill) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.GEN2);
          killTag(target, new Gen2.Password(((Gen2.Kill) tagOP).KillPassword));
          return null;
      } else if (tagOP instanceof Iso180006b.ReadData) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.ISO180006B);
          //System.out.println(cmdGetParam(TMR_RQL_PROTOCOL_ID));
          return readTagMemBytes(target, 0, ((Iso180006b.ReadData) tagOP).ByteAddress, ((Iso180006b.ReadData) tagOP).Len);
      } else if (tagOP instanceof Iso180006b.WriteData) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.ISO180006B);
          //System.out.println(cmdGetParam(TMR_RQL_PROTOCOL_ID));
          writeTagMemBytes(target, 0, ((Iso180006b.WriteData) tagOP).ByteAddress, ((Iso180006b.WriteData) tagOP).Data);
          return null;
      }else if (tagOP instanceof Iso180006b.Lock) {
          paramSet(TMR_PARAM_TAGOP_PROTOCOL, TagProtocol.ISO180006B);
          //System.out.println(cmdGetParam(TMR_RQL_PROTOCOL_ID));
          lockTag(target, new Iso180006b.LockAction(((Iso180006b.Lock) tagOP).ByteAddress));
          return null;
      }else if (tagOP instanceof Gen2.BlockWrite) {
          throw new FeatureNotSupportedException("Gen2.BlockWrite not supported");
      }else if (tagOP instanceof Gen2.BlockPermaLock){
          throw new FeatureNotSupportedException("Gen2.BlockPermaLock not supported");
      }else {
        return null;
      }
      }finally{
          // restoring the old protocol.
        paramSet(TMR_PARAM_TAGOP_PROTOCOL, protocolID);
      }
      
  }
  // For internal use - throw an exception on blank responses.
  // Nothing we're asking about has a legitimate reason to be blank.
  String getField(String field, String table)
    throws ReaderException
  {
    String ret;
    ret = getFieldInternal(field, table);
    if (ret.length() == 0)
    {
        throw new ReaderParseException("No field " + field + " in table " + table);
    }
    return ret;
  }


  void setField(String field, String value, String table)
    throws ReaderException
  {
    runQuery(String.format("UPDATE %s SET %s='%s';",
                           table, field, value));
  }

  /**
   * Retrieve port power for each antenna
   * @param val
   * @param op
   * @return
   * @throws ReaderException
   */
    private int[][] getPortPowerArray(Object val, String op) throws ReaderException
    {

        List<String> fields = new ArrayList<String>();        

        for (int count = 1; count <= maxAntennas; count++)
        {
            fields.add("ant" + count + "_" + op + "_tx_power_centidbm");
        }

        String[] response = runQuery(makeSelect(fields.toArray(new String[0]), TMR_RQL_PARAMS, null, commandTimeout));

        StringTokenizer st = new StringTokenizer(response[0],"|");
        String[] tokens = new String[st.countTokens()];
        int i=0;
        while(st.hasMoreTokens())
        {
            tokens[i++] = st.nextToken();
        }

        int[][] portPowerArray = new int[maxAntennas][];
        
        for (int count = 0; count < maxAntennas; count++)
        {            
            portPowerArray[count] = new int[]{count + 1, Integer.parseInt(tokens[count])};

        }                      
        return portPowerArray;
    }

   

    /**
     * Set the port power for all the antennas
     * @param val
     * @param op
     * @return
     */
    private String setPortPowerArray(Object val, String op)
    {
        int[][] prpListValues = (int[][]) val;
        StringBuilder portCmd = new StringBuilder();
        portCmd.append("UPDATE "+ TMR_RQL_SAVED_SETTINGS +" SET ");
        int portCount=0;

        for (int[] row : prpListValues)
        {
            portCount++;
            if ((row[0] > 0) && (row[0] <= maxAntennas))
            {
                int power = row[1];
                if (power < _rfPowerMin)
                {
                    throw new IllegalArgumentException(String.format("Requested power (%d) too low (RFPowerMin=%d cdBm)", power, _rfPowerMin));
                }
                if (power > _rfPowerMax)
                {
                    throw new IllegalArgumentException(String.format("Requested power (%d) too high (RFPowerMax=%d cdBm)", power, _rfPowerMax));
                }

                portCmd.append("ant");
                portCmd.append(String.valueOf(row[0]));
                portCmd.append("_");
                portCmd.append(op);
                portCmd.append("_tx_power_centidbm=");
                portCmd.append(String.valueOf(row[1]));
                if(portCount < prpListValues.length)
                    portCmd.append(",");                
            } 
            else
            {
                throw new IllegalArgumentException("Antenna id is invalid");
            }
        }

        return portCmd.toString();
    }

  

  public void destroy()
  {
    try
    {
      rqlSock.close();
    }
    catch (java.io.IOException e)
    {
      // How can we fail to be closed?
    }

    rqlIn = null;
    rqlOut = null;

    connected = false;
  }

  void setTxPower(int power)
    throws ReaderException
  {
    // We cache the value we last set the power to. This could be
    // changed out from under us my, for example, a m4api call.
    if (power != txPower)
    {
      runQuery(String.format("UPDATE saved_settings SET tx_power='%d';",
                             power));
      txPower = power;
    }
  }

  String protocolStr(TagProtocol protocol)
  {
    switch (protocol)
    {
    case GEN2: return TMR_TAGPROTOCOL_GEN2;
    case ISO180006B: return TMR_TAGPROTOCOL_ISO180006B;
    case ISO180006B_UCODE: return TMR_TAGPROTOCOL_ISO180006B;
    case IPX256: return TMR_TAGPROTOCOL_IPX256;
    case IPX64: return TMR_TAGPROTOCOL_IPX64;

    }
    throw new IllegalArgumentException("RQL does not support protocol " + 
                                       protocol);
  }

    TagProtocol codeToProtocol(int code) {
        switch (code) {
            case 8:
                return TagProtocol.ISO180006B;
            case 12:
                return TagProtocol.GEN2;
            case 13:
                return TagProtocol.IPX64;
            case 14:
                return TagProtocol.IPX256;

        }
        return null;
    }

  String readPlanWhereClause(ReadPlan rp)
  {
    String clause;

    if (rp instanceof SimpleReadPlan)
    {
      SimpleReadPlan srp = (SimpleReadPlan)rp;
      StringBuilder sb = new StringBuilder();

      sb.append(String.format("protocol_id='%s'",
                              protocolStr(srp.protocol)));
      if (srp.antennas.length > 0)
      {
        sb.append(" AND (");
        sb.append(String.format("antenna_id=%d", srp.antennas[0]));
        for (int i = 1; i < srp.antennas.length; i++)
        {
          sb.append(String.format(" OR antenna_id=%d", srp.antennas[i]));
        }
        sb.append(")");
      }
      clause = sb.toString();
    }
    else
    {
      throw new RuntimeException("Unknown ReadPlan passed to readPlanWhereClause" + rp.getClass());
    }
    return clause;
  }

  void setUcodeMode(SimpleReadPlan srp)
    throws ReaderException
  {
    if (srp.protocol == TagProtocol.ISO180006B)
    {
      setField(TMR_RQL_UCODEEPC, "no", TMR_RQL_PARAMS);
    }
    else if (srp.protocol == TagProtocol.ISO180006B_UCODE)
    {
      setField(TMR_RQL_UCODEEPC, "yes", TMR_RQL_PARAMS);
    }
  }
  
  public TagReadData[] read(long duration)
    throws ReaderException
  {
    List<TagReadData> tagvec;

    setTxPower((Integer)paramGet(TMR_PARAM_RADIO_READPOWER));
    tagvec = new ArrayList<TagReadData>();

    try
    {
      setSoTimeout((int)duration + transportTimeout);
      readInternal((int)duration, (ReadPlan)paramGet(TMR_PARAM_READ_PLAN), tagvec);
    }
    finally
    {
      setSoTimeout(transportTimeout);
    }
    return tagvec.toArray(new TagReadData[tagvec.size()]);
  }




  private void readInternal(int millis, ReadPlan rp, List<TagReadData> reads) throws ReaderException
    {
        resetRql();        
        List<Integer> timeouts = new ArrayList<Integer>();
        List<String> cnames = setUpCursors(rp, millis, timeouts);

        String cmd = String.format("FETCH %s", ReaderUtil.convertListToString(cnames, ","));
        sendQuery(cmd);

        for (int i = 0; i < cnames.size(); i++)
        {
            Date baseTime = new Date();            
            String[] rows = receiveBatch(timeouts.get(i));
            for (String row : rows)
            {
                if (0 < row.length())
                {
                    reads.add(parseRqlResponse(row, baseTime));
                }
            }            
        }
        // DeDuplication Logic        
        ReaderUtil.removeDuplicates(reads,paramGet(TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA),paramGet(TMR_PARAM_TAGREADDATA_UNIQUEBYDATA),paramGet(TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI));
        resetRql();
    }

  
    String tagopWhereClause(TagFilter target) throws ReaderException {
        int antenna;
        TagProtocol protocol;

        try
        {
            antenna = (Integer) paramGet(TMR_PARAM_TAGOP_ANTENNA);
            protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        } 
        catch (ReaderException re)
        {
            // If these parameters aren't set, something very bad has happened
            // to the internal state of this module; fail.
            throw new ReaderException("tagop parameters not set " + re.getMessage());
        }

        List<String> wheres = new ArrayList<String>();
	wheres.add(String.format("protocol_id='%s'", protocolStr(protocol)));
	wheres.add(String.format("antenna_id=%d", antenna));
        if (target != null)
        {
	    wheres.addAll(tagFilterToWhereClause(target));
        }
	return ReaderUtil.convertListToString(wheres, " AND ");
    }


    /**
     * returns password clause for use in where clause
     * @return String
     */
    String whereAsPasswordClause() throws ReaderException
    {
        Gen2.Password password;
        password = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
        int value = (int) password.value;
        String toHexString = Integer.toHexString(value);
        int len = toHexString.length();
        if (8 > len) {
            for (int i = 0; i < 8 - len; i++) {
                toHexString = "0" + toHexString;
            }
        }
        TagProtocol protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        if (protocol.equals(TagProtocol.GEN2) && password != null && password.value != 0) {
            return String.format(" password=0x%s", toHexString);
        } else {
            return "";
        }

    }

    String tagopSetClause()
            throws ReaderException
    {
        Gen2.Password password;
        password = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
        TagProtocol protocol = (TagProtocol)paramGet(TMR_PARAM_TAGOP_PROTOCOL);
        if(protocol.equals(TagProtocol.GEN2) && password!=null && password.value!=0)
        {
            return String.format(",password=0x%x", password.value);
        }
        else
        {
            return "";
        }
    }

  public void writeTag(TagFilter oldID, TagData newID)
    throws ReaderException
  {      
      // Validate parameters
      TagProtocol protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);
      if ((TagProtocol.ISO180006B == protocol) ||
              (TagProtocol.ISO180006B_UCODE == protocol))
      {
          if (null == oldID)
          {
              throw new ReaderException("ISO18000-6B does not yet support writing ID to unspecified tags.  Please provide a TagFilter.");
          }
      }

      List<String> wheres = new ArrayList<String>();
      setTxPower((Integer) paramGet(TMR_PARAM_RADIO_WRITEPOWER));
      
      wheres.add(tagopWhereClause(oldID));
      String pwClause = whereAsPasswordClause();
      if (!"".equals(pwClause))
      {
        wheres.add(pwClause);
      }
      

      String updateQuery = makeUpdate(TMR_RQL_ID,TMR_RQL_TAG_ID,newID.epcString(), wheres, 0);
      runQuery(updateQuery);

//    if (isAstra)
//    {
//      // The regular UPDATE command below won't work on current Astra
//      // with respect to singulation. Outsource this work to the
//      // memory-write.
//      writeTagMemBytes(oldID,
//                       1, // EPC bank
//                       4, // word address 2 - skip PC and CRC
//                       newID.epcBytes());
//    }
  }

  public void killTag(TagFilter target, TagAuthentication auth)
    throws ReaderException
  {    
      int killPassword;

      if (auth == null)
      {
          killPassword = 0;
      }
      else if (auth instanceof Gen2.Password)
      {
          killPassword = ((Gen2.Password) auth).value;
      }
      else
      {
          throw new IllegalArgumentException("Unknown kill authentication type.");
      }

      List<String> wheres = new ArrayList<String>();
      wheres.add(tagopWhereClause(target));

      if (auth != null && killPassword != 0)
      {
          wheres.add(String.format(" password=0x%x", killPassword));
      }

      String killQuery = makeUpdate("killed", TMR_RQL_TAG_ID, (int) 1, wheres, commandTimeout);
      runQuery(killQuery);
  }

  void checkMemParams(int bank, int address, int count)
  {
    if (bank < 0 || bank > 3)
    {
      throw new IllegalArgumentException("Invalid memory bank " + bank);
    }
    if (count < 0 || count > 8)
    {
      throw new IllegalArgumentException("Invalid word count " + count
                                         + " (out of range)");
    }
  }

  public byte[] readTagMemBytes(TagFilter target,
                                int bank, int address, int count)
    throws ReaderException
  {    
    List<String> wheres = new ArrayList<String>();

    String[] ret;
    byte[] bytes;
    int wordAddress = 0, wordCount = 0;
    int start;

    TagProtocol protocol =  (TagProtocol)paramGet(TMR_PARAM_TAGOP_PROTOCOL);

    if(protocol.equals(TagProtocol.GEN2))
    {
        wordAddress = address / 2;
        wordCount = (count + 1 + (address % 2) ) / 2;
    }
    else if(protocol.equals(TagProtocol.ISO180006B))
    {
        wordAddress = address;
        wordCount = count;
    }
    else
    {
        throw new FeatureNotSupportedException("read operation not implemented for " + protocol);
    }

    
    checkMemParams(bank, wordAddress, wordCount);
    setTxPower((Integer)paramGet(TMR_PARAM_RADIO_READPOWER));
   


      wheres.add(String.format("block_number=%d", wordAddress));
      wheres.add(String.format("block_count=%d", wordCount));
      wheres.add(tagopWhereClause(target));
      if (protocol.equals(TagProtocol.GEN2))
      {
          wheres.add(String.format("mem_bank=%d", bank));
      }


    String selectQuery = makeSelect(new String[]{"data"}, "tag_data", wheres, commandTimeout);
             
    ret = runQuery(selectQuery);

    start = 2;
    if (wordAddress * 2 != address)
    {
      // Only for ISO tags
      start = 4;
    }
    bytes = new byte[count];
    for (int i = 0; i < count; i++)
    {
        if(start + 2 + i*2<ret[0].length()){
            String byteStr = ret[0].substring(start + i*2, start + 2 + i*2);
            bytes[i] = (byte)Integer.parseInt(byteStr, 16);
        }

    }
    return bytes;
  }

  public short[] readTagMemWords(TagFilter target,
                                 int bank, int address, int count)
    throws ReaderException
  {

      List<String> wheres = new ArrayList<String>();
      //StringBuilder query = new StringBuilder();
      String[] ret;
      short[] words;
      TagProtocol protocol;

      checkMemParams(bank, address, count);

      setTxPower((Integer) paramGet(TMR_PARAM_RADIO_READPOWER));
      protocol = (TagProtocol)paramGet(TMR_PARAM_TAGOP_PROTOCOL);

      wheres.add(String.format("block_number=%d", address));
      wheres.add(String.format("block_count=%d", count));
      wheres.add(tagopWhereClause(target));

      if (protocol.equals(TagProtocol.GEN2))
      {
          wheres.add(String.format("mem_bank=%d", bank));
      }


      String selectQuery = makeSelect(new String[]{"data"}, "tag_data", wheres, commandTimeout);
      //query.append(";");
      ret = runQuery(selectQuery);

      words = new short[count];
      for (int i = 0; i < count; i++)
      {
              String wordStr = ret[0].substring(2 + i * 4, 6 + i * 4);
              words[i] = (short) Integer.parseInt(wordStr, 16);
      }
      return words;
  }

  public void writeTagMemBytes(TagFilter target,
                               int bank, int address, byte[] data)
    throws ReaderException
  {
      int wordAddress=0;
      StringBuilder dataStr;      
      
      List<String> wheres = new ArrayList<String>();

      if ((address % 2) != 0)
      {
          throw new IllegalArgumentException("Byte write address must be even");
      }
      if ((data.length % 2) != 0)
      {
          throw new IllegalArgumentException("Byte write length must be even");
      }

      TagProtocol protocol =  (TagProtocol)paramGet(TMR_PARAM_TAGOP_PROTOCOL);

      if (protocol.equals(TagProtocol.GEN2))
      {
          wordAddress = address / 2;
      }
      else if (protocol.equals(TagProtocol.ISO180006B))
      {
          wordAddress = address;
      }
      else
      {
         throw new FeatureNotSupportedException("write operation not implemented for " + protocol);
      }
      
      checkMemParams(bank, wordAddress, data.length / 2);
      setTxPower((Integer) paramGet(TMR_PARAM_RADIO_WRITEPOWER));
      

      dataStr = new StringBuilder();
      for (byte b : data)
      {
          dataStr.append(String.format("%02x", b));
      }


      wheres.add(String.format("block_number=%d", wordAddress));
      wheres.add(tagopWhereClause(target));

      if (protocol.equals(TagProtocol.GEN2))
      {
          wheres.add(String.format(" mem_bank=%d", bank));
      }

      String updateQuery = makeUpdate("data", "tag_data",dataStr.toString(), wheres, commandTimeout);
    // removed password and membank for ISO tags
     // query.append(S tring.format("UPDATE tag_data"
//              + " SET data=0x%s"
//              + " WHERE block_number=%d"
//              + " AND %s",
//              dataStr,
//              wordAddress, tagopWhereClause(target)))
      
      runQuery(updateQuery);
  }

  public void writeTagMemWords(TagFilter target,
                               int bank, int address, short[] data)
    throws ReaderException
  {
      StringBuilder dataStr;
      StringBuilder query = new StringBuilder();
      TagProtocol protocol;
      List<String> wheres = new ArrayList<String>();

      checkMemParams(bank, address, data.length);
      setTxPower((Integer) paramGet(TMR_PARAM_RADIO_WRITEPOWER));
      protocol = (TagProtocol) paramGet(TMR_PARAM_TAGOP_PROTOCOL);


      wheres.add(String.format("block_number=%d", address));
      wheres.add(tagopWhereClause(target));
      if (protocol.equals(TagProtocol.GEN2))
      {
          wheres.add(String.format(" mem_bank=%d", bank));
          //query.append(String.format(" AND mem_bank=%d", bank));
      }
      Gen2.Password accessPassword = (Gen2.Password)paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
      if(accessPassword!=null && accessPassword.value!=0)
      {
        wheres.add(whereAsPasswordClause());
      }

      dataStr = new StringBuilder();
      for (short s : data)
      {
          dataStr.append(String.format("%04x", s));
      }

      String updateQuery = makeUpdate("data", "tag_data",dataStr.toString(), wheres, 0);

//    query = String.format("UPDATE tag_data"
//                          + " SET data=0x%s"
//                          + " WHERE block_number=%d"
//                          + " AND %s"
//                          + " %s",
//                          dataStr,
//                          address,
//                          tagopWhereClause(target),tagOpAppendPassword() +";");

    runQuery(updateQuery);
  }

  public void lockTag(TagFilter target, TagLockAction lock)
    throws ReaderException
  {    
    List<String> wheres = new ArrayList<String>();

    if (lock instanceof Gen2.LockAction)
    {
        Gen2.LockAction la = (Gen2.LockAction) lock;
        wheres.add(tagopWhereClause(target));
        Gen2.Password accessPassword = (Gen2.Password) paramGet(TMR_PARAM_GEN2_ACCESSPASSWORD);
        if ((accessPassword.value != 0) && accessPassword != null)
        {
            wheres.add(whereAsPasswordClause());
        }                

      // RQL splits the Gen2 memory bank lock bits into "ID" and
      // "Data" pieces, such that you can't update both at once.
      if ((la.mask & 0x3FC) != 0)
      {          
          wheres.add(String.format("type=%d", la.mask));
          
          String gen2LockQuery = makeUpdate(TMR_RQL_TAG_LOCKED, TMR_RQL_TAG_ID,la.action, wheres, commandTimeout);
//        query = String.format("UPDATE tag_id"
//                              + " SET locked=%d"
//                              + " %s"
//                              + " WHERE %s AND type=%d;",
//                              la.action,
//                              tagopSetClause(),
//                              tagopWhereClause(target),
//                              la.mask);
        runQuery(gen2LockQuery);
      }
      if ((la.mask & 0x3) != 0)
      {
          
          wheres.add(String.format("type=%d", la.mask));

          String gen2LockQuery = makeUpdate(TMR_RQL_TAG_LOCKED, TMR_RQL_TAG_DATA,la.action, wheres, commandTimeout);
//        query = String.format("UPDATE tag_data"
//                              + " SET locked=%d"
//                              + " %s"
//                              + " WHERE %s AND type=%d;",
//                              la.action,
//                              tagopSetClause(),
//                              tagopWhereClause(target),
//                              la.mask);
        runQuery(gen2LockQuery);
      }
    }
    else if(lock instanceof Iso180006b.LockAction){

        Iso180006b.LockAction la = (Iso180006b.LockAction) lock;

        wheres.add(tagopWhereClause(target));


        wheres.add(String.format("block_number=%s", la.address));
        String isoLockQuery = makeUpdate(TMR_RQL_TAG_LOCKED, TMR_RQL_TAG_ID, (int) 1, wheres, commandTimeout);

//        query = String.format("UPDATE tag_id"
//                + " SET locked=%d"
//                + " WHERE %s "
//                + "AND block_number=%s;",
//                (int) 1,
//                tagopWhereClause(target),
//                la.address);
        runQuery(isoLockQuery);
      }
  }
 
    @Override
    public void firmwareLoad(InputStream firmware, FirmwareLoadOptions loadOptions) 
            throws ReaderException, IOException
    {
        webRequest(firmware, loadOptions);
    }

    @Override
    public synchronized void firmwareLoad(InputStream firmware)
            throws IOException, ReaderException
    {
        webRequest(firmware, null);
    }

    public void webRequest(InputStream fwStr,FirmwareLoadOptions loadOptions)
            throws ReaderException, IOException
    {
        // These are about to stop working
        rqlSock.close();
        rqlSock = null;
        rqlIn = null;
        rqlOut = null;
        
        // Assume that a system with an RQL interpreter has the standard
        // web interface and password. This isn't really an RQL operation,
        // but it will work most of the time.

        try
        {
            ReaderUtil.firmwareLoadUtil(fwStr, this, loadOptions);
        }
        finally
        {
            // Reconnect to the reader
            rqlSock = new Socket();
            setSoTimeout(transportTimeout);
            Reader r = create(uri.toString());
            if(r instanceof LLRPReader)
            {
                throw new ReaderException("Reader Type changed...Please reconnect");
            }
            r.connect();
        }        
  }

  public GpioPin[] gpiGet()
    throws ReaderException
  {
    String[] ret;
    int gpioVal;
    GpioPin[] state;

    ret = runQuery("SELECT data FROM io WHERE mask=0xffffffff;");
    gpioVal = Integer.decode(ret[0]);

    state = new GpioPin[gpiList.length];
    for (int i = 0; i < gpiList.length; i++)
    {
      state[i] = new GpioPin(gpiList[i], ((gpioVal & _gpioBits[gpiList[i]]) != 0));
    }

    return state;
  }

  public void gpoSet(GpioPin[] state)
    throws ReaderException
  {
    String query;
    int mask, data;

    mask = 0;
    data = 0;
    for (GpioPin gp : state)
    {
      mask |= _gpioBits[gp.id];
      if (gp.high)
      {
        data |= _gpioBits[gp.id];
      }
    }

    query = String.format("UPDATE io SET data=0x%x WHERE mask=0x%x;",
                          data, mask);

    runQuery(query);
  }
}
/*
 * synchronization mechanism for async reading
 */
class ManualResetEvent
{    
    private volatile boolean open = false;

    public ManualResetEvent(boolean open)
    {
        this.open = open;
    }

    public synchronized void waitOne() throws InterruptedException
    {
        //System.out.println("Waiting .....");
            while (open == false)
            {
                
                this.wait();
            }        
    }

    public synchronized void set()
    {
        //System.out.println("################ ENTERED TO BREAK THE LOCK");
        open = true;
        //RqlReader._stopRequested = true;
        this.notifyAll();
    }

    public void reset()
    {
        open = false;
    }
}
