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
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;

namespace ThingMagic
{
    /// <summary>
    /// The RqlReader class is an implementation of a Reader object that 
    /// communicates with a ThingMagic fixed RFID reader via the RQL network protocol.
    /// 
    /// Instances of the RQL class are created with the Reader.create()method with a 
    /// "rql" URI or a generic "tmr" URI that references a network device.
    /// </summary>
    public class RqlReader : Reader
    {
        #region Constants

        private const int ANTENNA_ID  = 0;
        private const int READ_COUNT  = 1;
        private const int ID          = 2;
        private const int FREQUENCY   = 3;
        private const int DSPMICROS   = 4;
        private const int PROTOCOL_ID = 5;
        private const int LQI         = 6;
        private const int DATA        = 7;
        private const int METADATA    = 8;
        private const int PHASE       = 9;

	    #endregion

        #region Static Fields

        private static string[]          _readFieldNames      = null;
        private static readonly char[]   _selFieldSeps        = new char[] { '|' };
        private static readonly string[] _astraReadFieldNames = "antenna_id read_count id frequency dspmicros protocol_id lqi".Split(new char[] { ' ' });
        private static readonly string[] _m5ReadFieldNames    = "antenna_id read_count id frequency dspmicros protocol_id".Split(new char[] { ' ' });
        private static readonly int[]    _gpioMapping         = new int[] { 0x04, 0x08, 0x10, 0x02, 0x20, 0x40, 0x80, 0x100 };
        private static readonly string[] _readMetaData        = "antenna_id read_count id frequency dspmicros protocol_id lqi data metadata phase".Split(new char[] { ' ' });
        
        #endregion

        #region Fields

        private string _hostname;
        private int _portNumber;
        private Socket _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        private NetworkStream _stream;
        private StreamReader _streamReader;
        private int _antennaMax;
        private int[] _antennaPorts;
        private int[] _gpiList;
        private int[] _gpoList;
        private string _model;
        private int _rfPowerMax;
        private int _rfPowerMin;
        private string _serialNumber;
        private string _softwareVersion;
        private TagProtocol[] _supportedProtocols;
        private ManualResetEvent _stopRequested;
        private int _maxCursorTimeout;
        private PowerMode powerMode = PowerMode.INVALID;

        /// <summary>
        /// Reader's native TX power setting
        /// </summary>
        private int _txPower;

        #endregion

        #region Nested Classes

        #region MaskedBits

        /// <summary>
        /// Value plus mask; i.e., selectively modify bits within a register, arbitrarily setting them to 0 or 1
        /// </summary>
        private sealed class MaskedBits
        {
            #region Fields

            public int Value;
            public int Mask; 

            #endregion

            #region Construction

            public MaskedBits()
            {
                Value = 0;
                Mask = 0;
            }

            #endregion            
        }

        #endregion 

        #region Gen2KillArgs

        private sealed class Gen2KillArgs
        {
            #region Fields

            public TagFilter Target;
            public UInt32     Password; 

            #endregion

            #region Construction

            public Gen2KillArgs(TagFilter target, TagAuthentication password)
            {
                if (null == password)
                    Password = 0;

                else if (password is Gen2.Password)
                {
                    Target = target;
                    Password = ((Gen2.Password) password).Value;
                }

                else
                    throw new ArgumentException("Unsupported TagAuthentication: " + password.GetType().ToString());
            }

            #endregion
        }

        #endregion

        #region Gen2LockArgs

        private sealed class Gen2LockArgs
        {
            #region Fields

            public TagFilter      Target;
            public Gen2.LockAction Action;

            #endregion

            #region Construction

            public Gen2LockArgs(TagFilter target, Gen2.LockAction action)
            {
                Target = target;
                Action = action;
            }

            #endregion
        }

        #endregion

        #region Iso180006bLockArgs

        private sealed class Iso180006bLockArgs
        {
            #region Fields

            public TagFilter Target;
            public byte Address;
            public int Locked;

            #endregion

            #region Construction

            public Iso180006bLockArgs(TagFilter target, byte address,int locked)
            {
                Target = target;
                Address = address;
                Locked = locked;
            }

            #endregion
        }

        #endregion

        #region WriteTagArgs

        private sealed class WriteTagArgs
        {
            #region Fields

            public TagFilter Target;
            public TagData    Epc;

            #endregion

            #region Construction

            public WriteTagArgs(TagFilter target, TagData epc)
            {
                Target = target;
                Epc = epc;
            }

            #endregion
        }
        
        #endregion

        #endregion



        #region Construction

        /// <summary>
        /// Connect to RQL reader on default port (8080)
        /// </summary>
        /// <param name="host">_hostname of RQL reader</param>
        /// <param name="port">Port Number of RQL reader</param>        
        public RqlReader(string host, int port)
        {
            if (port <= 0)
            {
                port = 8080;
            }

            _hostname   = host;
            _portNumber = port;
        }

        #endregion

        #region Initialization Methods

        #region GetHostAddress

        private IPAddress GetHostAddress(string host)
        {
            try
            {
                return IPAddress.Parse(host);
            }
            catch (FormatException)
            {
                IPHostEntry he = Dns.GetHostEntry(host);
                return he.AddressList[0];
            }
        }

        #endregion

        #region InitParams

        private void InitParams()
        {
            // TODO: ParamClear breaks pre-connect params.
            // ParamClear was orginally intended to reset the parameter table
            // after a firmware update, removing parameters that no longer exist
            // in the new firmware.  Deprioritize post-upgrade behavior for now.
            // --Harry Tsai 2009 Jun 26
            //ParamClear();
            // It's okay if this doesn't get set to anything other than NONE;
            // since the user doesn't need to set the region, the connection
            // shouldn't fail just because the region is unknown.
            Reader.Region region = Reader.Region.UNSPEC;

            String regionName = GetField("regionName", "params").ToUpper();
            if (regionName.Equals("US"))
                region = Reader.Region.NA;
            else if (regionName.Equals("KR"))
            {
                String regionVersion = (string)GetField("region_version", "params");
                if (regionVersion.Equals("1") || regionVersion.Equals(""))
                    region = Reader.Region.KR;
                else if (regionVersion.Equals("2"))
                    region = Reader.Region.KR2;
            }
            else if (regionName.Equals("JP"))
                region = Reader.Region.JP;
            else if (regionName.Equals("AU"))
                region = Reader.Region.AU;
            else if (regionName.Equals("NZ"))
                region = Reader.Region.NZ;
            else if (regionName.Equals("CN"))
                region = Reader.Region.PRC;
            else if (regionName.Equals("IN"))
                region = Reader.Region.IN;
            else if (regionName.Equals("EU"))
            {
                String regionVersion = (string) GetField("region_version", "params");
                if (regionVersion.Equals("1") || regionVersion.Equals(""))
                    region = Reader.Region.EU;
                else if (regionVersion.Equals("2"))
                    region = Reader.Region.EU2;
                else if (regionVersion.Equals("3"))
                    region = Reader.Region.EU3;
            }
            ParamAdd(new Setting("/reader/region/supportedRegions", typeof(Region[]), new Region[] { region }, false));
            ParamAdd(new Setting("/reader/region/id", typeof(Region), region, false,
                delegate(Object val)
                {
                    return val;
                },null));
            ParamAdd(new Setting("/reader/antenna/portList", typeof(int[]), null, false,
            delegate(Object val) { val = _antennaPorts; return val; },
            delegate(Object val) { throw new ArgumentException("Antenna PortList is a Read Only Param"); }));
            ParamAdd(new Setting("/reader/antenna/connectedPortList", typeof(int[]), null, false,
              delegate(Object val)
              {
                  String portList = "";
                  try
                  {
                      portList = GetField("reader_connected_antennas", "params");
                  }
                  catch (FeatureNotSupportedException)
                  {
                      if ("Astra" == _model)
                      {
                          portList = "1";
                      }
                  }
                  String[] ports = portList.Split(' ');
                  List<int> list = new List<int>();
                  for (int i = 0; i < ports.Length; i++)
                  {
                      if (!ports[i].StartsWith("X"))
                      {
                          try
                          {
                              int value = Convert.ToInt32(ports[i]);
                              list.Add(value);
                          }
                          catch (FormatException)
                          {
                          }
                      }
                  }
                  return list.ToArray();
              },
              null));
            ParamAdd(new Setting("/reader/gen2/q", typeof(Gen2.Q), null, true,
                delegate(Object val)
                {
                    int initq = int.Parse(GetField("gen2InitQ", "params"));
                    int minq = int.Parse(GetField("gen2MinQ", "params"));
                    int maxq = int.Parse(GetField("gen2MaxQ", "params"));
                    if ((minq == initq) && (initq == maxq) )
                    {
                        if (initq >= 0)
                        {
                            return new Gen2.StaticQ((byte)initq);
                        }
                        else
                        {
                            return new Gen2.DynamicQ();
                        }
                    }
                    else
                        return new Gen2.DynamicQ();
                },
                delegate(Object val)
                {
                    if (val is Gen2.StaticQ)
                    {
                        Gen2.StaticQ q = (Gen2.StaticQ) val;
                        if (q.InitialQ < 0 || q.InitialQ > 15)
                        {
                          throw new ArgumentOutOfRangeException("Value of /reader/gen2/q is out of range. Should be between 0 to 15");
                        }
                        string qval = q.InitialQ.ToString("D");
                        CmdSetParam("gen2InitQ", qval);
                        CmdSetParam("gen2MinQ", qval);
                        CmdSetParam("gen2MaxQ", qval);
                    }
                    else
                    {
                        CmdSetParam("gen2MinQ", "2");
                        CmdSetParam("gen2MaxQ", "6");
                    }

                    int initq = int.Parse(GetField("gen2InitQ", "params"));
                    int minq = int.Parse(GetField("gen2MinQ", "params"));
                    int maxq = int.Parse(GetField("gen2MaxQ", "params"));
                    if ((minq == initq) && (initq == maxq))
                        return new Gen2.StaticQ((byte) initq);
                    else
                        return new Gen2.DynamicQ();
                }));
            ParamAdd(new Setting("/reader/gen2/session", typeof(Gen2.Session), null, true,
                delegate(Object val)
                {
                    string rsp = GetField("gen2Session", "params");
                    int s = int.Parse(rsp);
                    switch (s)
                    {
                        // -1 is Astra's way of saying "Use the module's value."
                        // That value is dependent on the user mode, so check it.
                        case -1:
                            int mode = int.Parse(GetField("userMode", "params"));
                            if (mode == 3) // portal mode
                                return Gen2.Session.S1;
                            else
                                return Gen2.Session.S0;
                        case 0: return Gen2.Session.S0;
                        case 1: return Gen2.Session.S1;
                        case 2: return Gen2.Session.S2;
                        case 3: return Gen2.Session.S3;
                    }
                    throw new ReaderParseException("Unknown Gen2 session value " + s.ToString());
                },
                delegate(Object val) { SetField("params.gen2Session", (int) val); return val; }));
            ParamAdd(new Setting("/reader/gen2/target", typeof(Gen2.Target), null, true,
                delegate(Object val)
                {
                    string rsp = GetField("gen2Target", "params");
                    int rval = int.Parse(rsp);
                    Gen2.Target tgt;
                    switch (rval)
                    {
                        case 0: tgt = Gen2.Target.AB; break;
                        case 1: tgt = Gen2.Target.BA; break;
                        case -1:
                        case 2: tgt = Gen2.Target.A; break;
                        case 3: tgt = Gen2.Target.B; break;
                        default:
                            throw new ReaderParseException("Unrecognized Gen2 target: " + rval.ToString());
                    }
                    return tgt;
                },
                delegate(Object val)
                {
                    Gen2.Target tgt = (Gen2.Target) val;
                    int tnum;
                    switch (tgt)
                    {
                        case Gen2.Target.AB: tnum = 0; break;
                        case Gen2.Target.BA: tnum = 1; break;
                        case Gen2.Target.A: tnum = 2; break;
                        case Gen2.Target.B: tnum = 3; break;
                        default:
                            throw new ArgumentException("Unrecognized Gen2.Target " + val.ToString());
                    }
                    CmdSetParam("gen2Target", String.Format("{0:D}", tnum)); return tgt;
                }));
            ParamAdd(new Setting("/reader/gpio/inputList", typeof(int[]), _gpiList, false));
            ParamAdd(new Setting("/reader/gpio/outputList", typeof(int[]), _gpoList, false));
            ParamAdd(new Setting("/reader/version/model", typeof(string), _model, false));
            SettingFilter powerSetFilter = delegate(Object val)
            {
                int power = (int) val;
                if (power < _rfPowerMin) { throw new ArgumentException(String.Format("Requested power ({0:D}) too low (RFPowerMin={1:D}cdBm)", power, _rfPowerMin)); }
                if (power > _rfPowerMax) { throw new ArgumentException(String.Format("Requested power ({0:D}) too high (RFPowerMax={1:D}cdBm)", power, _rfPowerMax)); }
                SetField("saved_settings.tx_power", power);
                _txPower = power;
                return power;
            };
            ParamAdd(new Setting("/reader/radio/readPower", typeof(int), _txPower, true, null, powerSetFilter));
            ParamAdd(new Setting("/reader/read/plan", typeof(ReadPlan), new SimpleReadPlan(), true, null,
                delegate(Object val)
                {
                    if ((val is SimpleReadPlan) || (val is MultiReadPlan)) { return val; }
                    else { throw new ArgumentException("Unsupported /reader/read/plan type: " + val.GetType().ToString() + "."); }
                }));
            ParamAdd(new Setting("/reader/radio/powerMax", typeof(int), _rfPowerMax, false));
            ParamAdd(new Setting("/reader/radio/powerMin", typeof(int), _rfPowerMin, false));
            ParamAdd(new Setting("/reader/version/serial", typeof(string), _serialNumber, false));
            ParamAdd(new Setting("/reader/version/software", typeof(string), _softwareVersion, false));
            ParamAdd(new Setting("/reader/version/supportedProtocols", typeof(TagProtocol[]), _supportedProtocols, false));
            ParamAdd(new Setting("/reader/tagop/antenna", typeof(int), GetFirstConnectedAntenna(), true,
                null,
                delegate(Object val) { return ValidateAntenna((int) val); }));
            ParamAdd(new Setting("/reader/tagop/protocol", typeof(TagProtocol), TagProtocol.GEN2, true,
                null,
                delegate(Object val) { return ValidateProtocol((TagProtocol) val); }));
            ParamAdd(new Setting("/reader/radio/writePower", typeof(int), _txPower, true, null, powerSetFilter));
            ParamAdd(new Setting("/reader/powerMode", typeof(PowerMode), null, true,
                delegate(Object val)
                {
                    if (_socket.Connected)
                        return Convert.ToInt32(GetField("powerMode","params"));
                    else
                        return val;
                },
                delegate(Object val)
                {
                    if (_socket.Connected)
                        CmdSetPowerMode((PowerMode)val);

                    powerMode = (PowerMode)val;
                    return powerMode;
                }));
            ParamAdd(new Setting("/reader/hostname", typeof(string), null, true,
                delegate(Object val) 
                {
                    string hostName = null;
                    try
                    {
                        hostName = GetField("hostname", "saved_settings");
                    }
                    catch (Exception ex)
                    {
                        if (ex is FeatureNotSupportedException)
                        {
                            hostName = string.Empty;
                        }
                    }
                    return hostName;
                },
                delegate(Object val)
                {
                    if (!val.Equals(_hostname))
                    {
                        SetField("saved_settings.hostname", val);
                        _hostname = val.ToString();
                        return _hostname;
                    }
                    else
                    {
                        return _hostname;
                    }
                }));
            ParamAdd(new Setting("/reader/currentTime", typeof(DateTime), null, false,
              delegate(Object val) { return Convert.ToDateTime( GetField("current_time", "settings")); },
              null));
            ParamAdd(new Setting("/reader/antennaMode", typeof(int), null, false,
              delegate(Object val) { return Convert.ToInt32( GetField("antenna_mode", "saved_settings")); },
              null));
            ParamAdd(new Setting("/reader/antenna/checkPort", typeof(bool), null, true,
                delegate(Object val) {
                    return ConvertValueToBool(GetField("antenna_safety", "params"));
                },
                delegate(Object val) {
                    SetParamBoolValue("antenna_safety", (bool)val);
                    return val; 
                }));
            
                if (!(_model.Equals("Astra")))
                {
                    ParamAdd(new Setting("/reader/version/hardware", typeof(string), null, false,
                    delegate(Object val)
                    {
                        val = GetField("reader_hwverdata", "params");
                        return val;
                    },
                null));
                    ParamAdd(new Setting("/reader/radio/temperature", typeof(int), null, false,
                        delegate(Object val)
                        {
                            val = Convert.ToInt32(GetField("reader_temperature", "params"));
                            return val;
                        },
                    null));
                    ParamAdd(new Setting("/reader/radio/enableSJC", typeof(bool), false, false,
                         delegate(Object val)
                         {
                             return ConvertValueToBool(GetField("enableSJC", "params"));
                         }, null));
                    ParamAdd(new Setting("/reader/gen2/BLF", typeof(Gen2.LinkFrequency), null, true,
                        delegate(Object val)
                        {
                            int frequency = Convert.ToInt16(GetField("gen2BLF", "params"));
                            frequency = (frequency == -1) ? frequency : gen2BLFIntToValue(frequency);
                            return Enum.Parse(typeof(Gen2.LinkFrequency), frequency.ToString(), true);
                        },
                        delegate(Object val)
                        {
                            int blf = gen2BLFObjectToInt(val);
                            if (_model.Equals("Mercury6") && (blf == 2) || (blf == 3))
                            {
                                throw new ReaderException("Link Frequency not supported on M6");
                            }
                            SetField("params.gen2BLF", blf);
                            return val;
                        }
                        , false)
                        );
                    ParamAdd(new Setting("/reader/iso180006b/BLF", typeof(Iso180006b.LinkFrequency), Iso180006b.LinkFrequency.LINK160KHZ, true,
                        delegate(Object val)
                        {
                            int frequency = Convert.ToInt16(GetField("i186bBLF", "params"));
                            frequency = (frequency == -1) ? iso18000BBLFIntToValue(0) : iso18000BBLFIntToValue(frequency);
                            return Enum.Parse(typeof(Iso180006b.LinkFrequency), frequency.ToString(), true);
                        },
                        delegate(Object val)
                        {
                            SetField("params.i186bBLF", iso18000BBLFObjectToInt(val));
                            return val;
                        }, false));
                    ParamAdd(new Setting("/reader/gen2/tagEncoding", typeof(Gen2.TagEncoding), null, true,
                        delegate(Object val)
                        {
                            return Enum.Parse(typeof(Gen2.TagEncoding), GetField("gen2TagEncoding", "params"), true);
                        },
                        delegate(object val)
                        {
                            SetField("params.gen2TagEncoding", (int)(val));
                            return val;
                        }, false));
                    ParamAdd(new Setting("/reader/gen2/tari", typeof(Gen2.Tari), null, true,
                        delegate(Object val)
                        {
                            return Enum.Parse(typeof(Gen2.Tari), GetField("gen2Tari", "params"), true);
                        },
                        delegate(object val)
                        {
                            SetField("params.gen2Tari", (int)(val));
                            return val;
                        }, false));
                }
                ParamAdd(new Setting("/reader/description", typeof(string), null, true,
                delegate(Object val)
                {
                    string readerDesc = null;
                    try
                    {
                        readerDesc = GetField("reader_description", "params");
                    }
                    catch(Exception ex)
                    {
                        if (ex is FeatureNotSupportedException)
                        {
                            readerDesc = string.Empty;
                        }
                    }
                    return readerDesc;
                },
                delegate(Object val)
                {
                    SetField("params.reader_description", val);
                    return val;
                }, false));
           ParamAdd(new Setting("/reader/radio/portReadPowerList", typeof(int[][]), null, true,
                delegate(Object val)
                {
                    return GetPortPowerList(val,"read");
                },
                delegate(Object val)
                {
                    Query(SetPowerList(val, "read"));
                    return val;
                }, false));
           ParamAdd(new Setting("/reader/radio/portWritePowerList", typeof(int[][]), null, true,
                delegate(Object val)
                {
                    return GetPortPowerList(val,"write");
                },
                delegate(Object val)
                {
                    Query(SetPowerList(val, "write"));
                    return val;
                }, false));
            ParamAdd(new Setting("/reader/tagReadData/uniqueByAntenna", typeof(bool),false,true,
                delegate(Object val)
                {
                    return val;
                },
                delegate(Object val)
                {
                    return val;
                }, false));
            ParamAdd(new Setting("/reader/tagReadData/uniqueByData", typeof(bool), false, true,
                delegate(Object val)
                {
                    return val;
                },
                delegate(Object val)
                {
                    return val;
                }, false));
            ParamAdd(new Setting("/reader/tagReadData/recordHighestRssi", typeof(bool), false, true,
                delegate(Object val)
                {
                    return val;
                },
                delegate(Object val)
                {
                    return val;
               }, false));
            ParamAdd(new Setting("/reader/tagReadData/enableReadFilter", typeof(bool), true, false,
              delegate(Object val) { return true; },
              null));
            ParamAdd(new Setting("/reader/tagReadData/uniqueByProtocol", typeof(bool), true, false,
              delegate(Object val) { return true; },
              null));
            ParamAdd(new Setting("/reader/licenseKey", typeof(ICollection<byte>), null, true,
                 null,
                delegate(Object val)
                {
                    string[] key = (ByteFormat.ToHex((byte[])val).Split('x'));
                    SetField("params.reader_licenseKey", key[1].ToString());
                    string res = GetField("reader_licenseKey", "params");
                    
                    if(res.StartsWith("0000"))
                    {
                        return null;
                    }
                    throw new ReaderException(res.ToString());                  
                }));
        }

        #endregion

        private int[][] GetPortPowerList(object val,string op)
        {
            List<string> fields = new List<string>();
            List<int[]> prpList = new List<int[]>();

            for(int count=1; count<=_antennaMax;count++)
            {
                fields.Add("ant" + count + "_" +op + "_tx_power_centidbm");
            }

            string[] response = Query(MakeSelect(fields.ToArray(), "params"))[0].Split('|');

            for (int count = 0; count < _antennaMax; count++)
            {
               prpList.Add(new int[] { count + 1, Convert.ToInt32(response[count]) });
            }

            return prpList.ToArray();
        }

        private string SetPowerList(object val,string op)
        {
            int[][] prpListValues = (int[][])val;
            string cmd = "UPDATE SAVED_SETTINGS SET ";

            foreach(int[] row in prpListValues)
            {
                if ((row[0] > 0 )&&(row[0] <= _antennaMax))
                {
                    int power = row[1];
                    if (power < _rfPowerMin) { throw new ArgumentOutOfRangeException(String.Format("Requested power ({0:D}) too low (RFPowerMin={1:D}cdBm)", power, _rfPowerMin)); }
                    if (power > _rfPowerMax) { throw new ArgumentOutOfRangeException(String.Format("Requested power ({0:D}) too high (RFPowerMax={1:D}cdBm)", power, _rfPowerMax)); }

                    cmd += "ant" + row[0].ToString() + "_" + op + "_tx_power_centidbm=" + row[1].ToString() + ',';
                }
                else
                {
                    throw new ArgumentOutOfRangeException("Antenna id is invalid");
                }
            }

            return cmd.TrimEnd(',');
        }

        #region CmdSetPowerMode

        /// <summary>
        /// Set the current power mode of the device.
        /// </summary>
        /// <param name="powermode">the mode to set</param>
        public void CmdSetPowerMode(PowerMode powermode)
        {
            if (powerMode != powermode)
            {
                SetField("params.powerMode", powermode.GetHashCode());
                powerMode = powermode;
            }
        }

        #endregion

        #region CmdGetAvailableProtocols

        /// <summary>
        /// Get the list of RFID protocols supported by the device.
        /// </summary>
        /// <returns>an array of supported protocols</returns>
        public TagProtocol[] CmdGetAvailableProtocols()
        {
            return ParseProtocolList(GetField("supported_protocols", "settings"));           
        }

        #endregion
        
        #region ProbeHardware

        private void ProbeHardware()
        {
            _antennaMax = int.Parse(GetField("reader_available_antennas", "params"));
            _antennaPorts = new int[_antennaMax];

            for (int i = 0 ; i < _antennaPorts.Length ; i++)
                _antennaPorts[i] = i + 1;

            _serialNumber = GetField("reader_serial", "params");
            _softwareVersion = GetField("version", "settings");
            _model = null;
            try
            {
                _model = GetField("pib_model", "params");
            }
            catch (FeatureNotSupportedException)
            {
                // Do nothing, just leave default _model
            }

            switch (_softwareVersion.Substring(0, 2))
            {
                case "2.":
                    _gpiList = new int[] { 3, 4 };
                    _gpoList = new int[] { 0, 1, 2, 5 };
                    _readFieldNames = _m5ReadFieldNames;
                    _rfPowerMax = 3250;
                    _rfPowerMin = 500;
                    break;
                case "4.":
                    // 4. -> Astra or M6.  Older Astra firmware (before Pinwheel) doesn't support pib_model,
                    // but all M6s do, so (null == _model) implies Astra.
                    if (null == _model)
                    {
                        _model = "Astra";
                    }
                    _gpiList = new int[] { 3, 4, 6, 7 };
                    _gpoList = new int[] { 0, 1, 2, 5 };
                    _readFieldNames = _astraReadFieldNames;
                    _rfPowerMax = 3000;
                    _rfPowerMin = 500;
                    break;
                default:
                    break;
            }
            if (null == _model)
            {
              _model = "<unknown>";
            }

            if (_model == "Mercury6")
            {
                _rfPowerMax = 3150;
                _rfPowerMin = 500;
            }
            else if (_model == "Astra")
            {
                _rfPowerMax = 3000;
                _rfPowerMin = 500;
            }

            if ("Astra" == _model)
            {
                // Some versions of Astra firmware give an incorrect supported_protocols value,
                // so just hard-code to {GEN2}
                _supportedProtocols = new TagProtocol[] { TagProtocol.GEN2 };
            }
            else
            {
                _supportedProtocols = ParseProtocolList(GetField("supported_protocols", "settings"));
            }

            _txPower = int.Parse(GetField("tx_power", "saved_settings"));

        }
        #endregion

        #endregion

        #region Connect


        /// <summary>
        /// Connect reader object to device.
        /// If object already connected, then do nothing.
        /// </summary>

        public override void Connect()
        {
            if( !_socket.Connected )
            {
                try
                {
                    _socket.Connect(new IPEndPoint(GetHostAddress(_hostname), _portNumber));

                    _stream       = new NetworkStream(_socket, FileAccess.ReadWrite, true);
                    _streamReader = new StreamReader(_stream, Encoding.ASCII);
                }
                catch (SocketException ex)
                {
                    throw new ReaderCommException(ex.Message);
                }
                catch (IOException ex)
                {
                    throw new ReaderCommException(ex.Message);
                }

                FullyResetRql();
                ProbeHardware();
                InitParams();
            }
        }

        #endregion

        #region Reboot
        /// <summary>
        /// Reboots the reader device
        /// </summary>
        public override void Reboot()
        {
            throw new FeatureNotSupportedException("Unsupported operation");
        }
        #endregion

        #region Destroy

        /// <summary>
        /// Shuts down the connection with the reader device.
        /// </summary>

        public override void Destroy()
        {
            DestroyGivenRead();

            if (null != _streamReader)
            {
                _streamReader.Close();
            }
            if (null != _stream)
            {
                _stream.Close();
            }
            _socket.Close();
        }

        #endregion

        #region SimpleTransportListener

        /// <summary>
        /// Simple console-output transport listener
        /// </summary>
        public override void SimpleTransportListener(Object sender, TransportListenerEventArgs e)
        {
            string msg;
            msg = Encoding.ASCII.GetString(e.Data, 0, e.Data.Length);
            // Replace the newline(\n)from the message with Null string
            msg = msg.Replace(";\n", "").Replace("\n", "");
            msg = "\"" + msg.Replace("\r", "\\r").Replace("\n", "\\n") + "\";";

            Console.WriteLine(String.Format(
                "{0}: {1} (timeout={2:D}ms)",
                e.Tx ? "TX" : "RX",
                msg,
                e.Timeout
                ));
        }

        #endregion

        #region Parameter Methods

        #region CmdGetParam

        /// <summary>
        /// Get RQL parameter
        /// </summary>
        /// <param name="field">RQL parameter name</param>
        /// <returns>RQL parameter value</returns>
        public string CmdGetParam(string field)
        {
            return GetFieldInternal(field, ParamTable(field));
        }

        #endregion

        #region CmdSetParam

        /// <summary>
        /// Set RQL parameter
        /// </summary>
        /// <param name="field">RQL parameter name</param>
        /// <param name="value">RQL parameter value</param>
        public void CmdSetParam(string field, string value)
        {
            string table = ParamTable(field);
            SetField(String.Format("{0}.{1}", table, field), value);
        }

        #endregion

        #endregion

        #region Read Methods

        #region StartReading

        /// <summary>
        /// Start reading RFID tags in the background. The tags found will be
        /// passed to the registered read listeners, and any exceptions that
        /// occur during reading will be passed to the registered exception
        /// listeners. Reading will continue until stopReading() is called.
        /// </summary>

        public override void StartReading()
        {
            PrepForSearch();
            ReadPlan rp = (ReadPlan)ParamGet("/reader/read/plan");
            int ontime = (int)ParamGet("/reader/read/asyncOnTime");
            int offtime = (int)ParamGet("/reader/read/asyncOffTime");

            ResetRql();
            List<int> ctimes = null;
            List<string> cnames = SetupCursors(rp, ontime, out ctimes);

            StartAutoMode(cnames, ontime, offtime);
            StartReceiver(ctimes);
        }

        private void ResetRql()
        {
            Query("RESET");
        }

        private void FullyResetRql()
        {
            // Reset RQL and ask for a known response to confirm stoppage
            // SET AUTO=OFF is required before RESET to prevent RQL from spitting up
            // extra empty lines after responding to SELECT rql_version.
            //
            // DO NOT split these into separate queries.  We want to stop RQL, no
            // matter what -- trying to receive responses in between commands allows
            // interruptions from failed receives.
            SendQuery("SET AUTO=OFF; RESET; SELECT rql_version FROM firmware", 0);

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
            long startTime = DateTime.Now.Ticks;
            long timeout = 10000 * (int)ParamGet("/reader/transportTimeout");
            long endTime = startTime + timeout;
            int commandTimeout = (int)ParamGet("/reader/commandTimeout");
            while (DateTime.Now.Ticks <= endTime)
            {
                String[] lines = ReceiveBatch(commandTimeout, false);
                if (lines.Length > 0)
                {
                    if ((lines[lines.Length-1]).StartsWith("rql Built"))
                    {
                        break;
                    }
                }
            }
            }
        Thread recvThread;
        private void StartReceiver(ICollection<int> ctimes)
        {
            _stopRequested = new  ManualResetEvent(false);
            _maxCursorTimeout = 0;
            foreach (int ctime in ctimes)
            {
                _maxCursorTimeout = Math.Max(_maxCursorTimeout, ctime);
            }           
            recvThread = new Thread(ReceiverBody);
            recvThread.Start();
        }

        private void ReceiverBody()
        {
            try
            {
                try
                {
                    DateTime baseTime = DateTime.Now.ToUniversalTime();
                string[] rows;

                    // Check on each iteration if event was raised (signaled)
                    // that implicitly means if user requested to 'Stopreading'
                    while (false == _stopRequested.WaitOne(0, false))
                {
                    rows = ReceiveBatch(_maxCursorTimeout);
                    notifyBatchListeners(rows, baseTime);
                }
                SendQuery("SET AUTO=OFF", 0);
                int offTimeout;
                int zeroCount;
                int safetyCount;

                // 1 zeroCount response is for the AUTO=OFF response.
                // 1 zeroCount response is for the final tag bundle.
                    //   Sometimes there is no second response.  In this case, we should swallow
                    //   up the timeout exception to prevent it from causing trouble elsewhere.
                zeroCount = 2;
                offTimeout = _maxCursorTimeout;

                /* safetyCount keeps us from spinning indefinitely,but we don't want it to interfere with
                 normal termination, so put it safely out of the way */
                safetyCount = 10;
                while ((0 < zeroCount) && (0 < safetyCount))
                {
                    try
                    {
                        rows = ReceiveBatch(offTimeout, false);
                        notifyBatchListeners(rows, baseTime);

                        if (0 == rows.Length)
                        {
                            --zeroCount;
                        }
                        }
                        catch (ReaderCommException)
                        {
                            /* Assume timeout -- we don't have a good way
                             * to distinguish other comm exceptions */
                            // Clear socket timeout flag
                            _socket.Blocking = true;
                            // Treat as if it were the second empty response
                            --zeroCount;
                        }

                        --safetyCount;
                        offTimeout = 0;
                    }
                }
                finally
                    {
                    ResetRql();
                }
            }
            catch (ReaderException re)
                        {
                if ((re is ReaderCommException) &&
                    (-1 != re.Message.IndexOf("did not properly respond after a period of time")))
                {
                            // Timeout: RQL didn't send as many responses as we expected

                            // Clear socket timeout error condition (as suggested in
                            // http://www.codeguru.com/forum/showthread.php?threadid=469679)
                            // Otherwise, subsequent receives all fail with the same error
                            _socket.Blocking = true;
                }

                ReadExceptionPublisher expub = new ReadExceptionPublisher(this, re);
                Thread trd = new Thread(expub.OnReadException);
                trd.Start();
            }
            }

        private void notifyBatchListeners(ICollection<String> rows, DateTime baseTime)
        {
            foreach (string row in rows)
            {
                if (0 < row.Length)
                {
                    TagReadData trd = ParseRqlResponse(row, baseTime);
                    OnTagRead(trd);
                }
            }
        }


        private void StartAutoMode(List<string> cnames, int ontime, int offtime)
        {
            string cmd;

            int repeatTime = 0;
            if (0 < offtime)
            {
                repeatTime = ontime + offtime;
            }
            cmd = String.Format(
                "SET repeat={0}", repeatTime);
            SendQuery(cmd, 0);

            cmd = String.Format(
                "SET AUTO {0}=ON", String.Join(",", cnames.ToArray()));
            SendQuery(cmd, 0);
        }

        #endregion

        #region StopReading

        /// <summary>
        /// Stop reading RFID tags in the background.
        /// </summary>
        public override void StopReading()
        {
            if (null != _stopRequested)
            {
                //Signal the recievebody thread to stop i.e // Raise (signal) the event
                _stopRequested.Set();
                //Wait for the reciebebody thread to finish 
                // (i.e. let it process the signal request, and return)
                recvThread.Join();
            }
        }

        #endregion

        #region Read

        /// <summary>
        /// Read RFID tags for a fixed duration.
        /// </summary>
        /// <param name="milliseconds">the read timeout</param>
        /// <returns>the read tag data collection</returns>

        public override TagReadData[] Read(int milliseconds)
        {
            ReadPlan          rp    = (ReadPlan) ParamGet("/reader/read/plan");
            List<TagReadData> reads = new List<TagReadData>();

            PrepForSearch();

            ReadInternal(rp, milliseconds,ref reads);

            return reads.ToArray();
        }

        #endregion

        #region ReadInternal

        private void ReadInternal(ReadPlan rp, int milliseconds, ref List<TagReadData> reads)
        {
            ResetRql();

            List<int> timeouts;
            List<string> cnames = SetupCursors(rp, milliseconds, out timeouts);

            string cmd = String.Format(
                "FETCH {0}", String.Join(",", cnames.ToArray()));
            SendQuery(cmd, 0);

            for (int i=0; i<cnames.Count; i++)
            {
                DateTime baseTime = DateTime.Now;
                String[] rows = ReceiveBatch(timeouts[i]);
                foreach (string row in rows)
                {
                    if (0 < row.Length)
                    {
                        reads.Add(ParseRqlResponse(row, baseTime));
                    }
                }
            }
            reads = RemoveDuplicates(reads);
            ResetRql();
        }

        private List<TagReadData> RemoveDuplicates(List<TagReadData> reads)
        {
            Dictionary<string, TagReadData> dic = new Dictionary<string, TagReadData>();

            List<TagReadData> _tagReads = new List<TagReadData>();
            string key;
            byte b = (byte)(((bool)ParamGet("/reader/tagReadData/uniqueByAntenna") ? 0x10 : 0x00) + ((bool)ParamGet("/reader/tagReadData/uniqueByData") ? 0x01 : 0x00));


            foreach (TagReadData tag in reads)
            {
                switch (b)
                {
                    case 0x00:
                        key = tag.EpcString;
                        break;
                    case 0x01:
                        key = tag.EpcString + ";" + ByteFormat.ToHex(tag.Data, "", "");
                        break;
                    case 0x10:
                        key = tag.EpcString + ";" + tag.Antenna.ToString();
                        break;
                    default:
                        key = tag.EpcString + ";" + tag.Antenna.ToString() + ";" + ByteFormat.ToHex(tag.Data, "", "");
                        break;
                }

                if (!dic.ContainsKey(key))
                {
                    dic.Add(key, tag);

                }
                else  //see the tag again
                {
                    dic[key].ReadCount += tag.ReadCount;
                    if ((bool)ParamGet("/reader/tagReadData/recordHighestRssi"))
                    {
                        if (tag.Rssi > dic[key].Rssi)
                        {
                            int tmp = dic[key].ReadCount;
                            dic[key] = tag;
                            dic[key].ReadCount = tmp;
                        }
                    }

                }
            }
            reads.Clear();
            reads.AddRange(dic.Values);

            return reads;
        }


        private List<string> SetupCursors(ReadPlan rp, int timeout, out List<int> ctimesOut)
        {
            List<int> ctimes = null;
            List<string> rql = GenerateRql(rp, timeout, out ctimes);
            int cnum = 0;
            List<string> cnames = new List<string>();

            for (int i = 0; i < rql.Count; i++)
            {
                string line = rql[i];
                int ctime = ctimes[i];

                cnum++;
                string cname = String.Format("mapic{0}", cnum);
                cnames.Add(cname);
                string decl = String.Format(
                    "DECLARE {0} CURSOR FOR {1}", cname, line);
                Query(decl);
            }
            ctimesOut = ctimes;
            return cnames;
        }


        private TagReadData ParseRqlResponse(string row, DateTime baseTime)
        {
            String[] fields = row.Split(_selFieldSeps);

            if (_readFieldNames.Length != fields.Length)
            {
                throw new ReaderParseException(String.Format("Unrecognized format."
                    + "  Got {0} fields in RQL response, expected {1}.",
                    fields.Length, _readFieldNames.Length));
            }
            byte[] epccrc = ByteFormat.FromHex(fields[ID].Substring(2));
            byte[] epc = new byte[epccrc.Length - 2];
            Array.Copy(epccrc, epc, epc.Length);
            byte[] crc = new byte[2];
            Array.Copy(epccrc, epc.Length, crc, 0, 2);

            TagProtocol proto = CodeToProtocol(fields[PROTOCOL_ID]);
            string idfield = fields[ID];
            TagData tag = null;

            switch (proto)
            {
                case TagProtocol.GEN2:
                    byte[] pcbits = null;
                    if (_readFieldNames.Length > 7)
                    {
                        pcbits = ByteFormat.FromHex(fields[METADATA].ToString());
                    }
                    tag = new Gen2.TagData(epc, crc,pcbits);
                    break;
                case TagProtocol.ISO180006B:
                    tag = new Iso180006b.TagData(epc, crc);
                    break;
                case TagProtocol.IPX64:
                    tag = new Ipx64.TagData(epc, crc);
                    break;
                case TagProtocol.IPX256:
                    tag = new Ipx256.TagData(epc, crc);
                    break;
                default:
                    throw new ReaderParseException("Unknown protocol code " + fields[PROTOCOL_ID]);
            }

            int antenna = int.Parse(fields[ANTENNA_ID]);
            TagReadData tr = new TagReadData();
            tr.Reader = this;
            tr._tagData = tag;
            tr._antenna = antenna;
            tr._baseTime = baseTime;
            tr.ReadCount = int.Parse(fields[READ_COUNT]);
            if (_readFieldNames.Length > 7)
            {
                tr._data = ByteFormat.FromHex(fields[DATA]);
                tr._phase = int.Parse(fields[PHASE]);
            }
            tr._readOffset = int.Parse(fields[DSPMICROS]) / 1000;
            tr._frequency = int.Parse(fields[FREQUENCY]);
            

            if ("Mercury5" != _model)
            {
                tr._lqi = int.Parse(fields[LQI]);
            }

            return tr;
        }

        #endregion

        private List<string> GenerateRql(ReadPlan rp, int milliseconds)
        {
            List<int> timeoutList = null;
            return GenerateRql(rp, milliseconds, out timeoutList);
        }

        /// <summary>
        /// Convert ReadPlan to RQL SELECT statements
        /// </summary>
        /// <param name="rp">ReadPlan to convert</param>
        /// <param name="milliseconds">Total number of milliseconds to allocate to ReadPlan</param>
        /// <param name="subTimeouts">Optional output of milliseconds allocated to each RQL subquery</param>
        /// <returns>List of RQL subqueries</returns>
        private List<string> GenerateRql(ReadPlan rp, int milliseconds, out List<int> subTimeouts)
        {
            List<string> queries = new List<String>();
            List<int> timeoutList = new List<int>();

            if (rp is MultiReadPlan)
            {
                MultiReadPlan mrp = (MultiReadPlan)rp;
                foreach (ReadPlan r in mrp.Plans)
                {
		    // Ideally, totalWeight=0 would allow reader to
		    // dynamically adjust timing based on tags observed.
		    // For now, just divide equally.
                    int subtimeout = 
			(mrp.TotalWeight != 0) ? (int)milliseconds * r.Weight / mrp.TotalWeight
			: milliseconds / mrp.Plans.Length;
                    subtimeout = Math.Min(subtimeout, UInt16.MaxValue);
                    List<int> subTimeoutList = new List<int>();
                    queries.AddRange(GenerateRql(r, subtimeout, out subTimeoutList));
                    timeoutList.AddRange(subTimeoutList);
                }
            }
            else if (rp is SimpleReadPlan)
            {
                List<string> wheres = new List<string>();
                wheres.AddRange(ReadPlanToWhereClause(rp));
                wheres.AddRange(TagFilterToWhereClause(((SimpleReadPlan)rp).Filter));
                DateTime baseTime = DateTime.Now;
                TagOp op = ((SimpleReadPlan)rp).Op;
                String query;
                if(op!=null && op is Gen2.ReadData)
                {
                    _readFieldNames = _readMetaData;                
                }
               
                query = MakeSelect(_readFieldNames, "tag_id", wheres, milliseconds);
                queries.Add(query);
                timeoutList.Add(milliseconds);
            }
            else
            {
                throw new ArgumentException("Unrecognized /reader/read/plan type " + typeof(ReadPlan).ToString());
            }
            subTimeouts = timeoutList;
            return queries;
        }

        #region PrepForSearch

        private void PrepForSearch()
        {
            SetTxPower((int) ParamGet("/reader/radio/readPower"));
        }

        #endregion

        #region SetTxPower

        private void SetTxPower(int power)
        {
            if (power != _txPower)
            {
                Query(String.Format("UPDATE saved_settings SET tx_power={0:D}", power));
            }
        }

        #endregion

        #endregion

        #region GPIO Methods

        #region GpiGet

        /// <summary>
        /// Get the state of all of the reader's GPI pins. 
        /// </summary>
        /// <returns>array of GpioPin objects representing the state of all input pins</returns>
        public override GpioPin[] GpiGet()
        {
            List<String> wheres = new List<String>();
            wheres.Add("mask=0xffffffff");
            int inputState = (int)IntUtil.StrToLong(GetField("data", "io", wheres));

            int[] pins = (int[])ParamGet("/reader/gpio/inputList");
            List<GpioPin> pinvals = new List<GpioPin>();
            foreach (int pin in pins)
            {
                bool state = (0 != (inputState & GetPinMask(pin)));
                pinvals.Add(new GpioPin(pin, state));
            }
            return pinvals.ToArray();
        }

        #endregion

        #region GpoSet

        /// <summary>
        /// Set the state of some GPO pins.
        /// </summary>
        /// <param name="state">array of GpioPin objects</param>
        public override void GpoSet(ICollection<GpioPin> state)
        {
            MaskedBits mb = new MaskedBits();
            int[] valids = (int[])ParamGet("/reader/gpio/outputList");
            foreach (GpioPin gp in state)
            {
                int pin = gp.Id;
                if (IsMember<int>(pin, valids))
                {
                    int pinmask = GetPinMask(pin);
                    mb.Mask |= pinmask;
                    if (gp.High) { mb.Value |= mb.Mask; }
                }
            }
            SetField("io.data", mb);
        }

        #endregion

        #region GetPinMask

        private static int GetPinMask(int gpioPin)
        {
            return _gpioMapping[gpioPin];
        }

        #endregion

        #endregion

        #region Tag Operation Methods

        #region ExecuteTagOp
        /// <summary>
        /// execute a TagOp
        /// </summary>
        /// <param name="tagOP">Tag Operation</param>
        /// <param name="target">Tag filter</param>
        ///<returns>the return value of the tagOp method if available</returns>

        public override Object ExecuteTagOp(TagOp tagOP, TagFilter target)
        {
            TagProtocol oldProtocol = (TagProtocol)ParamGet("/reader/tagop/protocol");
            try
            {
                if (tagOP is Gen2.ReadData)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                    return ReadTagMemWords(target, (int)(((Gen2.ReadData)tagOP).Bank), (int)((Gen2.ReadData)tagOP).WordAddress, ((Gen2.ReadData)tagOP).Len);
                }
                if (tagOP is Gen2.WriteData)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                    WriteTagMemWords(target, (int)(((Gen2.WriteData)tagOP).Bank), (int)((Gen2.WriteData)tagOP).WordAddress, (ushort[])((Gen2.WriteData)tagOP).Data);
                    return null;
                }
                if (tagOP is Gen2.Lock)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                    Gen2.Password oldPassword = (Gen2.Password)(ParamGet("/reader/gen2/accessPassword"));
                    if (((Gen2.Lock)tagOP).AccessPassword != 0)
                    {
                        ParamSet("/reader/gen2/accessPassword", new Gen2.Password(((Gen2.Lock)tagOP).AccessPassword));
                    }
                    try
                    {
                        LockTag(target, ((Gen2.Lock)tagOP).LockAction);
                    }
                    finally
                    {
                        ParamSet("/reader/gen2/accessPassword", oldPassword);
                    }
                    return null;
                }
                if (tagOP is Gen2.Kill)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                    Gen2.Password auth = new Gen2.Password(((Gen2.Kill)tagOP).KillPassword);
                    KillTag(target, auth);
                    return null;
                }
                else if (tagOP is Gen2.WriteTag)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.GEN2);
                    WriteTag(target, ((Gen2.WriteTag)tagOP).Epc);
                    return null;
                }
                else if (tagOP is Gen2.BlockWrite)
                {
                    throw new FeatureNotSupportedException("Gen2.BlockWrite not supported");
                }
                else if (tagOP is Gen2.BlockPermaLock)
                {
                    throw new FeatureNotSupportedException("Gen2.BlockPermaLock not supported");
                }
                else if (tagOP is Iso180006b.ReadData)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.ISO180006B);
                    return ReadTagMemBytes(target, 0, (int)(((Iso180006b.ReadData)tagOP).byteAddress), ((Iso180006b.ReadData)tagOP).length);
                }
                else if (tagOP is Iso180006b.WriteData)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.ISO180006B);
                    WriteTagMemBytes(target, 0, (int)(((Iso180006b.WriteData)tagOP).Address), ((Iso180006b.WriteData)tagOP).Data);
                    return null;
                }
                else if (tagOP is Iso180006b.LockTag)
                {
                    ParamSet("/reader/tagop/protocol", TagProtocol.ISO180006B);
                    Iso180006bLockTag(target, ((Iso180006b.LockTag)tagOP).Address);
                    return null;
                }
                else
                {
                    throw new Exception("Unsupported tagop.");
                }
            }
            finally
            {
                ParamSet("/reader/tagop/protocol", oldProtocol);
            }
        }

        #endregion

        #region KillTag

        /// <summary>
        /// Kill a tag. The first tag seen is killed.
        /// </summary>
        /// <param name="target">the tag target</param>
        /// <param name="password">the kill password</param>
        public override void KillTag(TagFilter target, TagAuthentication password)
        {
            PrepForTagop();
            SetField("tag_id.killed", new Gen2KillArgs(target, password));
        }

        #endregion

        #region LockTag

        /// <summary>
        /// Perform a lock or unlock operation on a tag. The first tag seen
        /// is operated on - the singulation parameter may be used to control
        /// this. Note that a tag without an access password set may not
        /// accept a lock operation or remain locked.
        /// </summary>
        /// <param name="target">the tag target to operate on</param>
        /// <param name="action">the tag lock action</param>
        public override void LockTag(TagFilter target, TagLockAction action)
        {
            PrepForTagop();
            TagProtocol protocol = (TagProtocol) ParamGet("/reader/tagop/protocol");

            if (action is Gen2.LockAction)
            {
                if (TagProtocol.GEN2 != protocol)
                {
                    throw new ArgumentException(String.Format(
                        "Gen2.LockAction not compatible with protocol {0}",
                        protocol.ToString()));
                }

                Gen2.LockAction la = (Gen2.LockAction) action;

                // TODO: M4API makes a distinction between locking tag ID and data.
                // Locking tag_id allows access to only the EPC, TID and password banks.
                // Locking tag_data allows access to only the User bank.
                // Ideally, M4API would stop making the distinction, since Gen2 has a unified locking model.
                // In the meantime, just lock tag_id, since it covers more and tags with user memory are still uncommon.

                if ((la.Mask & 0x3FC) != 0)
                    SetField("tag_id.locked", new Gen2LockArgs(target, (Gen2.LockAction) action));

                else if ((la.Mask & 0x3) != 0)
                    SetField("tag_data.locked", new Gen2LockArgs(target, (Gen2.LockAction) action));
            }
            else
                throw new ArgumentException("LockTag does not support this type of TagLockAction: " + action.ToString());
        }

        #endregion

        #region Iso180006bLockTag
        /// <summary>
        /// Lock a byte of memory on an ISO180006B tag 
        /// </summary>
        /// <param name="filter">a specification of the air protocol filtering to perform</param> 
        /// <param name="address">Indicates the address of tag memory to be (un)locked.</param>  
        [Obsolete()]
        public void Iso180006bLockTag(TagFilter filter,byte address)
        {
            PrepForTagop();
            TagProtocol protocol = (TagProtocol) ParamGet("/reader/tagop/protocol");

            if (null != filter )
            {
                Iso180006bLockArgs lockArgs = new Iso180006bLockArgs(filter, address, 1);
                SetField("tag_data.locked", lockArgs);
            }
            else
                throw new ArgumentException("ISO180006B only supports locking a single tag specified by 64-bit EPC");
        }
        #endregion

        #region ReadTagMemBytes

        /// <summary>
        /// Read data from the memory bank of a tag.
        /// </summary>
        /// <param name="target">the tag target to operate on</param>
        /// <param name="bank">the tag memory bank</param>
        /// <param name="address">the reading starting byte address</param>
        /// <param name="byteCount">the bytes to read</param>
        /// <returns>the bytes read</returns>
        public override byte[] ReadTagMemBytes(TagFilter target, int bank, int address, int byteCount)
        {
            PrepForTagop();
            int wordAddress = 0, wordCount = 0;

          
            List<string> wheres = new List<string>();
            if (TagProtocol.GEN2 == (TagProtocol)ParamGet("/reader/tagop/protocol"))
            {
                wheres.Add(String.Format("mem_bank={0:D}", bank));
                wordAddress = address / 2;
                wordCount = ByteConv.WordsPerBytes(byteCount);
            }
            else
            {
                wordAddress = address;
                wordCount = byteCount;
            }
            wheres.Add(String.Format("block_count={0:D}", wordCount));
            wheres.Add(String.Format("block_number={0:D}", wordAddress));
            
            Gen2.Password password = (Gen2.Password)(ParamGet("/reader/gen2/accessPassword"));
            if ((TagProtocol.GEN2 == (TagProtocol)ParamGet("/reader/tagop/protocol")) && (password.Value != 0))
            {
                wheres.Add(String.Format("password=0x{0:X}", password));
            }
            wheres.AddRange(MakeTagopWheres(target));
            string response = Select(new string[] { "data" }, "tag_data", wheres, -1)[0];
            if (TagProtocol.GEN2 == (TagProtocol)ParamGet("/reader/tagop/protocol"))
            {
                // Skip "0x" prefix
                int charOffset = 2;
                // Correct for word start boundary
                int byteOffset = address - (wordAddress * 2);
                charOffset += byteOffset * 2;
                int charLength = byteCount * 2;
                string byteStr = response.Substring(charOffset, charLength);

                return ByteFormat.FromHex(byteStr);
            }
            else
            {
                return ByteFormat.FromHex(response);
            }
        }

        /// <summary>
        /// Read data from the memory bank of a tag.
        /// </summary>
        /// <param name="target">the tag target to operate on</param>
        /// <param name="bank">the tag memory bank</param>
        /// <param name="wordAddress">the read starting word address</param>
        /// <param name="wordCount">the number of words to read</param>
        /// <returns>the read words</returns>

        public override ushort[] ReadTagMemWords(TagFilter target, int bank, int wordAddress, int wordCount)
        {
            return ReadTagMemWordsGivenReadTagMemBytes(target, bank, wordAddress, wordCount);
        }

        #endregion

        #region WriteTag

        /// <summary>
        /// Write a new ID to a tag.
        /// </summary>
        /// <param name="target">the tag target to operate on</param>
        /// <param name="epc">the tag ID to write</param>

        public override void WriteTag(TagFilter target, TagData epc)
        {
            // Validate parameters
            TagProtocol protocol = (TagProtocol)ParamGet("/reader/tagop/protocol");
            switch (protocol)
            {
                case TagProtocol.ISO180006B:
                case TagProtocol.ISO180006B_UCODE:
                    if (null == target)
                    {
                        throw new ReaderException("ISO18000-6B does not yet support writing ID to unspecified tags.  Please provide a TagFilter.");
                    }
                    break;
            }

            PrepForTagop();
            SetField("tag_id.id", new WriteTagArgs(target, epc));
        }

        #endregion

        #region WriteTagMemBytes

        /// <summary>
        /// Write data to the memory bank of a tag.
        /// </summary>
        /// <param name="target">the tag target to operate on</param>
        /// <param name="bank">the tag memory bank</param>
        /// <param name="byteAddress">the starting memory address to write</param>
        /// <param name="data">the data to write</param>
        public override void WriteTagMemBytes(TagFilter target, int bank, int byteAddress, ICollection<byte> data)
        {
            PrepForTagop();
            int address = 0, byteCount = 0;
            TagProtocol proto = (TagProtocol)ParamGet("/reader/tagop/protocol");
            if (TagProtocol.GEN2 == proto)
            {
                address = byteAddress / 2;

                if (address * 2 != byteAddress)
                    throw new ArgumentException("Byte write address must be even");

                byteCount = data.Count;
                int count = ByteConv.WordsPerBytes(byteCount);

                if (count * 2 != byteCount)
                    throw new ArgumentException("Byte write length must be even");
            }
            else if (TagProtocol.ISO180006B == proto)
            {
                address = byteAddress;
            }
            else
            {
                throw new ReaderException("Writing tag data not supported for protocol  " + proto.ToString());
            }

            List<string> wheres = new List<string>();
            wheres.Add(String.Format("mem_bank={0:D}", bank));
            wheres.Add(String.Format("block_number={0:D}", address));
            wheres.AddRange(MakeTagopWheres(target));
            wheres.AddRange(MakeAccessPasswordWheres());

            SetField("tag_data.data", CollUtil.ToArray(data), wheres);   
        }

        #endregion

        #region WriteTagMemWords

        /// <summary>
        /// Write data to the memory bank of a tag.
        /// </summary>
        /// <param name="target">the tag target to operate on</param>
        /// <param name="bank">the tag memory bank</param>
        /// <param name="address">the memory address to write</param>
        /// <param name="data">the data to write</param>
        public override void WriteTagMemWords(TagFilter target, int bank, int address, ICollection<ushort> data)
        {
            ushort[] dataArray = CollUtil.ToArray(data);
            byte[] dataBytes = new byte[dataArray.Length * 2];

            for (int i = 0; i < dataArray.Length; i++)
            {
                dataBytes[2 * i] = (byte)((dataArray[i] >> 8) & 0xFF);
                dataBytes[2 * i + 1] = (byte)((dataArray[i]) & 0xFF);
            }           
            
            WriteTagMemBytes(target, bank, address*2, dataBytes);
        }

        #endregion

        #region PrepForTagop

        private void PrepForTagop()
        {
            SetTxPower((int) ParamGet("/reader/radio/writePower"));
            // Antenna and protocol are handled in the tagop WHERE clause generator
        }

        #endregion

        #endregion

        #region Firmware Methods

        #region FirmwareLoad
        /// <summary>
        /// Loads firmware on the Reader.
        /// </summary>
        /// <param name="firmware">Firmware IO stream</param>
        [MethodImpl(MethodImplOptions.Synchronized)]
        public override void FirmwareLoad(Stream firmware)
        {
            FirmwareLoad(firmware, null);
        }

        /// <summary>
        /// Loads firmware on the Reader.
        /// </summary>
        /// <param name="firmware">Firmware IO stream</param>
        /// <param name="rflOptions">firmware load options</param>
        [MethodImpl(MethodImplOptions.Synchronized)]
        public override void FirmwareLoad(Stream firmware, FirmwareLoadOptions rflOptions)
        {
            try
            {
                ReaderUtil.FirmwareLoadUtil(this, firmware, (FixedReaderFirmwareLoadOptions)rflOptions, _hostname, ref _socket);
            }
            finally
            {
                // Reconnect to the reader
                this.Destroy();
                _socket = null;
                _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                Reader currentReader = this;
                Reader changedReader = Reader.Create((string)currentReader.ParamGet("/reader/uri"));
                if (!(currentReader.GetType().Equals(changedReader.GetType())))
                {
                    throw new ReaderException("Reader type has been changed!. Please re-connect");
                }
                this.Connect();
                }
                }

        #endregion

        #endregion

        #region RQL Methods

        #region Query

        private string[] Query(string cmd)
        {
            return Query(cmd, 0, false);
        }

        private string[] Query(string cmd, bool permitEmptyResponse)
        {
            return Query(cmd, 0, permitEmptyResponse);
        }

        /// <summary>
        /// Perform RQL query
        /// </summary>
        /// <param name="cmd">Text of query to send</param>
        /// <param name="cmdTimeout">Number milliseconds to allow query to execute.
        /// The ultimate comm timeout will be this number plus the transportTimeout.</param>
        /// <param name="permitEmptyResponse">If true, then first line of RQL response may be empty -- keep looking for response terminator (which is also an empty line.)
        /// If false, stop receiving response at first empty line.</param>
        /// <returns>Query response, parsed into individual lines.
        /// Includes terminating empty line.</returns>
        [MethodImpl(MethodImplOptions.Synchronized)]
        private string[] Query(string cmd, int cmdTimeout, bool permitEmptyResponse)
        {
            SendQuery(cmd, cmdTimeout);
            return ReceiveBatch(cmdTimeout, permitEmptyResponse);
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        private void SendQuery(string cmd, int cmdTimeout)
        {
            if (!cmd.EndsWith(";\n"))
                cmd += ";\n";

            byte[] bytesToSend = Encoding.ASCII.GetBytes(cmd);
#if WindowsCE
            int sendTimeout = 0;
#else
            int transportTimeout = (int)ParamGet("/reader/transportTimeout");
            int sendTimeout = _socket.SendTimeout = transportTimeout;
#endif

            OnTransport(true, bytesToSend, sendTimeout);

            try
            {
                _socket.Send(bytesToSend);
            }
            catch (SocketException ex)
            {
                throw new ReaderCommException(ex.Message, bytesToSend);
            }
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        private string[] ReceiveBatch(int cmdTimeout)
        {
            return ReceiveBatch(cmdTimeout, false);
        }

        /// <summary>
        /// Receive multi-line response (terminated by empty line)
        /// </summary>
        /// <param name="cmdTimeout">Milliseconds of inactivity allowed before timing out receive</param>
        /// <param name="permitEmptyResponse"></param>
        /// <returns></returns>
        private string[] ReceiveBatch(int cmdTimeout, bool permitEmptyResponse)
        {
#if WindowsCE
            int recvTimeout = 0;
#else
            int transportTimeout = (int)ParamGet("/reader/transportTimeout");
            int recvTimeout = _socket.ReceiveTimeout = transportTimeout + cmdTimeout;
#endif
            List<string> response = new List<string>();

            try
            {
                string line = null;
                bool done = false;
                while (!done)
                {
                    line = _streamReader.ReadLine();

                    if (line != null && line.StartsWith("Error"))
                    {
                        try
                        {
                            //Remove extra new line after reciving the error
                            _streamReader.ReadLine();
                            throw new ReaderException(line);
                        }
                        catch
                        {
                            throw new ReaderException(line);
                        }
                    }
                    if (line != null && (permitEmptyResponse || !line.Equals("")))
                    {
                        response.Add(line);
                        permitEmptyResponse = false;
                    }
                    else
                    {
                        done = true;
                    }
                }
            }
            catch (SocketException ex)
            {
                throw new ReaderCommException(ex.Message, StringsToBytes(response));
            }
            catch (IOException ex)
            {
                throw new ReaderCommException(ex.Message, StringsToBytes(response));
            }

            OnTransport(false, StringsToBytes(response), recvTimeout);

            CleanLeadingNewline(response);

            return response.ToArray();
        }

        /// <summary>
        /// RQL quirkiness can result in an extra newline being prepended to a response.
        /// 
        /// If auto mode is active and we issue RESET, the currently-running cursor remains active
        /// and emits a newline when it's done.  Our receive code ends up tacking this asynchronous newline
        /// onto the beginning of the next response.
        /// 
        /// Strip off the leading newline, if present, but not in the case of a legitimate "empty response".
        /// An empty response occurs when trying to fetch a non-existent field from the params table.
        /// This response consists of two consecutive newlines: the first represents an empty (i.e., null) value,
        /// and the second is the response terminator.  This should have been better designed in RQL, but now
        /// we're stuck with it.
        /// </summary>
        /// <param name="response">List of response lines (not including newlines)</param>
        private void CleanLeadingNewline(List<string> response)
        {
            // If response has at least two lines (leading newline, plus some content)
            if (2 <= response.Count)
            {
                // If first line is leading newline and second isn't a terminating newline
                if ((0 == response[0].Length) &&
                    (0 <= response[1].Length))
                {
                    // Strip off the spurious leading newline
                    response.RemoveAt(0);
                }
            }
        }

        #endregion

        #region Select

        private string[] Select(string[] fields, string table, List<string> wheres, int timeout)
        {
            return Query(MakeSelect(fields, table, wheres, timeout), timeout, false);
        }

        #endregion

        #region MakeSelect

        private string MakeSelect(string field, string table)
        {
            return MakeSelect(new string[] { field }, table, null, -1);
        }

        private string MakeSelect(string[] fields, string table)
        {
            return MakeSelect(fields, table, null, -1);
        }

        private string MakeSelect(string[] fields, string table, List<string> wheres, int timeout)
        {
            List<string> words = new List<string>();

            words.Add("SELECT");
            words.Add(String.Join(",", fields));
            words.Add("FROM");
            words.Add(table);
            words.Add(WheresToRql(wheres));

            if (-1 < timeout)
                words.Add("SET time_out=" + timeout.ToString());

            return String.Join(" ", words.ToArray());
        }

        #endregion

        #region GetField

        /// <summary>
        /// Retrieve an RQL parameter (row from a single-field table).
        /// Throws exception on empty values -- use only for parameters that aren't allowed to be empty
        /// (e.g., Reader parameters)
        /// </summary>
        /// <param name="name">Parameter name</param>
        /// <param name="table">Table name</param>
        /// <returns>Parameter value, or throws ReaderParseException if field has empty value.</returns>
        private string GetField(string name, string table)
        {
            string value = GetFieldInternal(name, table);

            if (0 == value.Length)
                throw new FeatureNotSupportedException("No field " + name + " in table " + table);

            return value;
        }

        #endregion

        #region GetFieldInternal

        private string GetFieldInternal(string name, string table)
        {
            return Query(MakeSelect(name, table), true)[0];
        }

        #endregion

        #region GetField

        private string GetField(string name, string table, List<string> wheres)
        {
            return Select(new string[] { name }, table, wheres, -1)[0];
        }

        #endregion

        #region SetField

        private string[] SetField(string tabledotname, Object value)
        {
            return SetField(tabledotname, value, null);
        }

        private string[] SetField(string tabledotname, Object value, List<string> wheres)
        {
            string[] fields = tabledotname.Split(new char[] { '.' });
            string table = fields[0];
            string name = fields[1];
            string valstr = SetFieldValueFilter(value);
            List<string> phrases = new List<string>();

            phrases.Add(String.Format("UPDATE {0} SET {1}={2}", table, name, valstr));

            if ((null != wheres) && (0 < wheres.Count))
                phrases.Add(WheresToRql(wheres));

            return Query(String.Join(" ", phrases.ToArray()));
        }

        #endregion

        #region SetFieldValueFilter

        private string SetFieldValueFilter(Object value)
        {
            if (value is int)
                return String.Format("'{0:D}'", (int) value);

            else if (value is byte[])
            {
                // No quotes around hex strings
                return String.Format("{0}", ByteFormat.ToHex((byte[]) value));
            }
            else if ((value is UInt16[]) || (value is ushort[]))
            {
                // No quotes around hex strings
                return String.Format("0x{0}", ByteFormat.ToHex((UInt16[]) value));
            }
            else if (value is Gen2KillArgs)
            {
                Gen2KillArgs args = (Gen2KillArgs) value;
                if (!(args.Target is TagData))
                    throw new ArgumentException("Kill only supports targets of type TagData, not " + args.Target.ToString());

                string epcHex = ((TagData) args.Target).EpcString;
                UInt32 password = args.Password;
                List<string> wheres = new List<string>();
                wheres.Add(String.Format("id=0x{0:X}", epcHex));
                wheres.Add(String.Format("password=0x{0:X}", password));
                wheres.AddRange(MakeTagopWheres());
               
                return String.Format("1 {0}", WheresToRql(wheres));
            }
            else if (value is Gen2LockArgs)
            {
                Gen2LockArgs args = (Gen2LockArgs) value;
                Gen2.LockAction act = (Gen2.LockAction) args.Action;
                List<string> wheres = new List<string>();
                wheres.Add(String.Format("type={0:D}", act.Mask));
                wheres.AddRange(MakeTagopWheres(args.Target));
                wheres.AddRange(MakeAccessPasswordWheres());

                return String.Format("{0:D} {1}", act.Action, WheresToRql(wheres));
            }
            else if (value is MaskedBits)
            {
                MaskedBits mb = (MaskedBits) value;
                return String.Format("0x{0:X} WHERE mask=0x{1:X}", mb.Value, mb.Mask);
            }
            else if (value is WriteTagArgs)
            {
                WriteTagArgs args = (WriteTagArgs) value;

                Gen2.Password password = (Gen2.Password)(ParamGet("/reader/gen2/accessPassword"));
                List<string> wheres = new List<string>();
                wheres.AddRange(MakeTagopWheres(args.Target));
                if ((TagProtocol.GEN2 == (TagProtocol)ParamGet("/reader/tagop/protocol")) && (password.Value != 0))
                {
                    wheres.Add(String.Format("password=0x{0:X}", password));
                }
                return String.Format("0x{0} {1}", args.Epc.EpcString, WheresToRql(wheres));                
            }
            else if (value is Iso180006bLockArgs)
            {
                Iso180006bLockArgs args = (Iso180006bLockArgs)value;
                List<string> wheres = new List<string>();
                wheres.Add(String.Format(" block_number={0:D}", args.Address));
                wheres.AddRange(MakeTagopWheres(args.Target));

                return String.Format("{0:D} {1}", args.Locked, WheresToRql(wheres));
            }
            else
            {
                return String.Format("'{0}'", value.ToString());
            }
        }

        #endregion

        #region AntennaToRql

        /// <summary>
        /// Create RQL representing a single antenna
        /// </summary>
        /// <param name="ant">Antenna identifiers</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private string AntennaToRql(int ant)
        {
            return String.Format("antenna_id={0:D}", ant);
        }

        #endregion

        #region AntennasToWhereClause
        /// <summary>
        /// Create RQL representing a list of antennas
        /// </summary>
        /// <param name="ants">List of antenna identifiers</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> AntennasToWhereClause(int[] ants)
        {
            List<string> wheres = new List<string>();

            if ((null != ants) && (0 < ants.Length))
            {
                StringBuilder sb = new StringBuilder();
                sb.Append("(");
                List<string> antwords = new List<string>();

                foreach (int ant in ants)
                    antwords.Add(AntennaToRql(ant));

                sb.Append(String.Join(" OR ", antwords.ToArray()));
                sb.Append(")");
                wheres.Add(sb.ToString());
            }

            return wheres;
        } 
        #endregion

        #region ReadPlanToWhereClause
        /// <summary>
        /// Create WHERE clauses representing a ReadPlan
        /// </summary>
        /// <param name="readPlan">Read plan</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> ReadPlanToWhereClause(ReadPlan readPlan)
        {
            List<string> wheres = new List<string>();
            
            if (readPlan is SimpleReadPlan)
            {
                SimpleReadPlan srp = (SimpleReadPlan) readPlan;
                wheres.AddRange(TagProtocolToWhereClause(srp.Protocol));
                wheres.AddRange(AntennasToWhereClause(srp.Antennas));
                if (null != srp.Op && srp.Op is Gen2.ReadData)
                {
                    wheres.Add(String.Format("mem_bank={0:D}", ((Gen2.ReadData)(srp.Op)).Bank));
                    wheres.Add(String.Format("block_count={0:D}", ((Gen2.ReadData)(srp.Op)).Len));
                    wheres.Add(String.Format("block_number={0:D}", (((Gen2.ReadData)(srp.Op)).WordAddress)).ToString());
                }
            }
            else
                throw new ArgumentException("Unrecognized /reader/read/plan type " + typeof(ReadPlan).ToString());
            
            return wheres;
        }

        #endregion

        #region TagFilterToWhereClause

        /// <summary>
        /// Create WHERE clauses representing a tag filter
        /// </summary>
        /// <param name="tagFilter">Tag filter</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> TagFilterToWhereClause(TagFilter tagFilter)
        {
            List<string> wheres = new List<string>();

            if (null == tagFilter)
            {
                // Do nothing
            }
            else if (tagFilter is Iso180006b.TagData)
            {
                Iso180006b.TagData td = (Iso180006b.TagData)tagFilter;
                wheres.Add(String.Format("id={0}", ByteFormat.ToHex(td.EpcBytes)));
            }
            else if (tagFilter is TagData)
            {
                TagData td = (TagData) tagFilter;
                wheres.Add(String.Format("id={0}", ByteFormat.ToHex(td.EpcBytes)));
            }
            else if (tagFilter is Gen2.Select)
            {
                Gen2.Select sel = (Gen2.Select)tagFilter;
                wheres.Add("filter_subtype=0");
                wheres.Add(String.Format("filter_mod_flags=0x{0:X}", (sel.Invert) ? "00000001" : "00000000"));
                wheres.Add(String.Format("filter_bank=0x{0:X}" , sel.Bank));
                wheres.Add("filter_bit_address=" + sel.BitPointer);
                wheres.Add("filter_bit_length="+sel.BitLength);
                wheres.Add("filter_data=0x" + BitConverter.ToString(sel.Mask).Replace("-", string.Empty));
            }
            else if (tagFilter is Iso180006b.Select)
            {
                Iso180006b.Select sel = (Iso180006b.Select)tagFilter;
                wheres.Add("filter_subtype=0");
                string operation = string.Empty;
                switch (sel.Op)
                {
                    case Iso180006b.SelectOp.EQUALS: operation = "00"; break;
                    case Iso180006b.SelectOp.NOTEQUALS: operation = "01"; break;
                    case Iso180006b.SelectOp.GREATERTHAN: operation = "02"; break;
                    case Iso180006b.SelectOp.LESSTHAN: operation = "03"; break;
                    default: break;
                };
                wheres.Add(String.Format("filter_command=0x{0:X}",operation));
                wheres.Add(String.Format("filter_mod_flags=0x{0:X}", (sel.Invert) ? "00000001" : "00000000"));                
                wheres.Add("filter_bit_address=" + sel.Address);
                wheres.Add("filter_bit_length=64");
                wheres.Add("filter_data=0x" + BitConverter.ToString(sel.Data).Replace("-", string.Empty));
            }
            else
                throw new ArgumentException("RQL only supports singulation by EPC. " + tagFilter.ToString() + " is not supported.");

            return wheres;
        }

        #endregion

        #region TagProtocolsToWhereClause

        /// <summary>
        /// Create WHERE clauses representing a list of tag protocols
        /// </summary>
        /// <param name="tp">Tag protocols</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> TagProtocolsToWhereClause(TagProtocol[] tp)
        {
            List<string> wheres = new List<string>();

            if ((null != tp) && (0 < tp.Length))
            {
                List<string> protwords = new List<string>();

                foreach (TagProtocol protocol in tp)
                    protwords.Add(TagProtocolToWhereClause(protocol)[0]);

                string clause = "(" + String.Join(" OR ", protwords.ToArray()) + ")";
                wheres.Add(clause);
            }

            return wheres;
        }

        #endregion

        #region TagProtocolToWhereClause

        /// <summary>
        /// Create WHERE clauses representing a tag protocol
        /// </summary>
        /// <param name="tp">Tag protocol</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> TagProtocolToWhereClause(TagProtocol tp)
        {
            List<string> wheres = new List<string>();
            string clause;

            switch (tp)
            {
                case TagProtocol.GEN2: 
                    clause = "GEN2"; break;
                case TagProtocol.ISO180006B: 
                    clause = "ISO18000-6B"; break;
                case TagProtocol.IPX64:
                    clause = "IPX64"; break;
                case TagProtocol.IPX256:
                    clause = "IPX256"; break;
                default:
                    throw new ArgumentException("Unrecognized protocol " + tp.ToString());
            }

            wheres.Add(String.Format("protocol_id='{0}'", clause));

            return wheres;
        } 

        #endregion

        #region MakeTagopWheres
        /// <summary>
        /// Create list of WHERE clauses representing tagop reader configuration (e.g., TagopAntenna, TagopProtocol)
        /// </summary>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> MakeTagopWheres()
        {
            List<string> wheres = new List<string>();
            wheres.AddRange(TagProtocolToWhereClause((TagProtocol) ParamGet("/reader/tagop/protocol")));
            wheres.Add(AntennaToRql((int) ParamGet("/reader/tagop/antenna")));
            return wheres;
        }         

        /// <summary>
        /// Create list of WHERE clauses representing tagop reader configuration (e.g., /reader/tagop/antenna, /reader/tagop/protocol)
        /// </summary>
        /// <param name="filt">Tags to target for tagop</param>
        /// <returns>List of strings to be incorporated into "WHERE ... AND ..." phrase.
        /// List may be empty.</returns>
        private List<string> MakeTagopWheres(TagFilter filt)
        {
            List<string> wheres = (List<string>) MakeTagopWheres();
            wheres.AddRange(TagFilterToWhereClause(filt));
            return wheres;
        }

        #endregion

        #region MakeAccessPasswordWheres
        /// <summary>
        /// Makes Where clause for Access Password
        /// </summary>
        /// <returns>List of string with either no elements or the access password WHERE clause</returns>
        private List<string> MakeAccessPasswordWheres()
        {
            List<string> where = new List<string>();
            Gen2.Password password = (Gen2.Password) (ParamGet("/reader/gen2/accessPassword"));

            if (password.Value != 0)
                where.Add(String.Format("password=0x{0:X}", password));

            return where;
        } 
        #endregion

        #region WheresToRql

        private string WheresToRql(List<string> wheres)
        {
            List<string> words = new List<string>();

            if ((null != wheres) && (0 < wheres.Count))
                words.Add(String.Format("WHERE {0}", String.Join(" AND ", wheres.ToArray())));

            return String.Join(" ", words.ToArray());
        }

        #endregion

        #region ParamTable

        private string ParamTable(string name)
        {
            switch (name)
            {
                default:
                    return "params";
                case "uhf_power_centidbm":
                case "tx_power":
                case "hostname":
                case "iface":
                case "dhcpcd":
                case "ip_address":
                case "netmask":
                case "gateway":
                case "ntp_servers":
                case "epc1_id_length":
                case "primary_dns":
                case "secondary_dns":
                case "domain_name":
                case "reader_description":
                case "reader_role":
                case "ant1_readpoint_descr":
                case "ant2_readpoint_descr":
                case "ant3_readpoint_descr":
                case "ant4_readpoint_descr":
                    return "saved_settings";
            }
        }

        #endregion

        #endregion

        

        #region Misc Utility Methods

        #region ParseProtocolList

        private static TagProtocol[] ParseProtocolList(string pstrs)
        {
            List<TagProtocol> plist = new List<TagProtocol>();

            foreach (string pstr in pstrs.Split(new char[] { ' ' }))
            {
                TagProtocol prot = TagProtocol.NONE;

                switch (pstr)
                {
                    case "GEN2":
                        prot = TagProtocol.GEN2; break;
                    case "ISO18000-6B":
                        prot = TagProtocol.ISO180006B; break;
                    case "IPX64":
                        prot = TagProtocol.IPX64; break;
                    case "IPX256":
                        prot = TagProtocol.IPX256; break;
                    default:
                        // ignore unrecognized protocol names
                        break;
                }

                if (TagProtocol.NONE != prot)
                    plist.Add(prot);
            }

            return plist.ToArray();
        }

        #endregion

        #region StringsToBytes

        private static byte[] StringsToBytes(ICollection<string> strings)
        {
            List<byte> responseBytes = new List<byte>();

            foreach (string i in strings)
                responseBytes.AddRange(Encoding.ASCII.GetBytes(i+"\n"));

            responseBytes.AddRange(Encoding.ASCII.GetBytes("\n"));

            return responseBytes.ToArray();
        }

        #endregion

        #region CodeToProtocol

        /// <summary>
        ///  Translate RQL protocol IDs to TagProtocols
        /// </summary>
        /// <param name="rqlproto"></param>
        /// <returns></returns>
        private static TagProtocol CodeToProtocol(string rqlproto)
        {
            switch (rqlproto)
            {
                case "8":
                    return TagProtocol.ISO180006B;

                case "12":
                    return TagProtocol.GEN2;
                case "13":
                    return TagProtocol.IPX64;
                case "14":
                    return TagProtocol.IPX256;

                default:
                    return TagProtocol.NONE;
            }
        }

        #endregion

        #region ValidateAntenna
        /// <summary>
        /// Is requested antenna a valid antenna?
        /// </summary>
        /// <param name="reqAnt">Requested antenna</param>
        /// <returns>reqAnt if it is in the set of valid antennas, else throws ArgumentException</returns>
        private int ValidateAntenna(int reqAnt)
        {
            return ValidateParameter<int>(reqAnt, (int[]) ParamGet("/reader/antenna/portList"), "Invalid antenna");
        }

        #endregion

        /// <summary>
        /// Convert strin value to bool
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        private bool ConvertValueToBool(string value)
        {
            if (value.Equals("yes"))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="paramName"></param>
        /// <param name="value"></param>
        private void SetParamBoolValue(string paramName, bool value)
        {
            if ((bool)value)
            {
                SetField("params." + paramName, "yes");
            }
            else
            {
                SetField("params." + paramName, "no");
            }
        }
        

        #endregion  

        #region gen2BLFIntToValue

        private static int gen2BLFIntToValue(int val)
        {
            switch (val)
            {
                case 0:
                    {
                        return 250;
                    }
               /*case 1:
                    {
                        return 300;
                    }*/
                case 2:
                    {
                        return 400;
                    }
                case 3:
                    {
                        return 40;
                    }
                case 4:
                    {
                        return 640;
                    }
                default:
                    {
                        throw new ArgumentException("Unsupported tag BLF.");
                    }
            }

        }
        #endregion

        #region gen2BLFObjectToInt

        private static int gen2BLFObjectToInt(Object val)
        {
            switch ((Gen2.LinkFrequency)val)
            {
                case Gen2.LinkFrequency.LINK250KHZ:
                    {
                        return 0;
                    }
                case Gen2.LinkFrequency.LINK640KHZ:
                    {
                        return 4;
                    }
                default:
                    {
                        throw new ArgumentException("Unsupported tag BLF.");
                    }
            }

        }
        #endregion


        #region iso18000BBLFIntToValue

        private static int iso18000BBLFIntToValue(int val)
        {
            switch (val)
            {
                case 1:
                    {
                        return 40;
                    }
                case 0:
                    {
                        return 160;
                    }
                default:
                    {
                        throw new ArgumentException("Unsupported tag BLF.");
                    }
            }

        }

        #endregion

        #region iso18000BBLFObjectToInt

        private static int iso18000BBLFObjectToInt(Object val)
        {
            switch ((Iso180006b.LinkFrequency)val)
            {
                case Iso180006b.LinkFrequency.LINK40KHZ:
                    {
                        return 1;
                    }
                case Iso180006b.LinkFrequency.LINK160KHZ:
                    {
                        return 0;
                    }
                default:
                    {
                        throw new ArgumentException("Unsupported tag BLF.");
                    }
            }

        }

        #endregion
    }
}
