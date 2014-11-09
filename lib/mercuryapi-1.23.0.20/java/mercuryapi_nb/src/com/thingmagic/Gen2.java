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
import java.util.Calendar;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

/**
 * This class is a namespace for Gen2-specific subclasses of generic
 * Mercury API classes, Gen2 data structures, constants, and Gen2
 * convenience functions.
 */
public class Gen2
{

  // non-public
  Gen2() { }

/**
 * This class extends {@link TagData} to represent the details of a Gen2 RFID tag.
 */
  public static class TagData extends com.thingmagic.TagData 
  {
    final byte[] pc;

    public TagData(byte[] bEPC)
    {
      super(bEPC);

      pc = new byte[2];
      pc[0] = (byte)((epc.length) << 3);
      pc[1] = 0;
    }

    public TagData(byte[] bEPC, byte[] newPC)
    {
      super(bEPC);

      pc = newPC.clone();
    }

    public TagData(byte[] bEPC, byte[] crc, byte[] newPC)
    {
      super(bEPC, crc);

      pc = newPC.clone();
    }

    public TagData(String sEPC)
    {
      super(sEPC);

      pc = new byte[2];
      pc[0] = (byte)((epc.length) << 3);
      pc[1] = 0;
    }

    public TagData(String sEPC, String sCrc)
    {
      super(sEPC,sCrc);

      pc = new byte[2];
      pc[0] = (byte)((epc.length) << 3);
      pc[1] = 0;
    }

    public TagProtocol getProtocol()
    {
      return TagProtocol.GEN2;
    }

    public byte[] pcBytes()
    {
      return pc.clone();
    }

    @Override
    boolean checkLen(int epcbytes)
    {
      if (epcbytes < 0)
      {
        return false;
      }
      if (epcbytes > 62)
      {
        return false;
      }
      if ((epcbytes & 1) != 0)
      {
        return false;
      }
    return true;
    }

    public String toString()
    {
      return String.format("GEN2:%s", epcString());
    }

  }

  /**
   * Gen2 memory banks
   */
  public enum Bank
  {
    RESERVED (0),
      EPC (1),
      TID (2),
      USER (3),
      // Used to enable the read of additional membanks - reserved mem bank
      GEN2BANKRESERVEDENABLED(0x4),
      // Used to enable the read of additional membanks - epc mem bank
      GEN2BANKEPCENABLED(0x8),
      // Used to enable the read of additional membanks - tid mem bank
      GEN2BANKTIDENABLED(0x10),
      /// Used to enable the read of additional membanks - user mem bank
      GEN2BANKUSERENABLED(0x20);

    int rep;
    Bank(int rep)
    {
      this.rep = rep;
    }

    public static Bank getBank(int v)
    {
      switch(v)
      {
      case 0: return RESERVED;
      case 1: return EPC;
      case 2: return TID;
      case 3: return USER;
      case 4: return GEN2BANKRESERVEDENABLED;
      case 8: return GEN2BANKEPCENABLED;
      case 10: return GEN2BANKTIDENABLED;
      case 20: return GEN2BANKUSERENABLED;
      }
      throw new IllegalArgumentException("Invalid Gen2 bank " + v);
    }
  }

  /**
   * Gen2 session values
   */
  public enum Session
  {
      /**
       * Session 0
       */
      S0(0),
      /**
       * Session 1
       */
      S1(1),
      /**
       * Session 2
       */
      S2(2),
      /**
       * Session 3
       */
      S3(3);

      int rep; /* Gen2 air protocol representation of the value */
      Session(int rep)
      {
          this.rep = rep;
      }
      private static final Map<Integer, Session> lookup = new HashMap<Integer, Session>();
      static
      {
          for (Session session : EnumSet.allOf(Session.class))
          {
              lookup.put(session.getCode(), session);
          }
      }
      public int getCode()
      {
          return rep;
      }
      public static Session get(int rep)
      {
          return lookup.get(rep);
      }
  }
  
  /**
   * Target algorithm options
   */
  public enum Target
  {
      /**
       * Search target A until exhausted, then search target B
       */
      AB(0),
      /**
       * Search target B until exhausted, then search target A
       */
      BA(1),
      /**
       * Search target A
       */
      A(2),
      /**
       * Search target B
       */
      B(3);
    
      int rep; /* Gen2 air protocol representation of the value */
      Target(int rep)
      {
          this.rep = rep;
      }
      private static final Map<Integer, Target> lookup = new HashMap<Integer, Target>();
      static
      {
          for (Target target : EnumSet.allOf(Target.class))
          {
              lookup.put(target.getCode(), target);
          }
      }
      public int getCode()
      {
          return rep;
      }
      public static Target get(int rep)
      {
          return lookup.get(rep);
      }
  }

  /**
   * Miller M values
   */
  public enum TagEncoding
  {
      /**
       * FM0
       */
      FM0(0),
      /**
       * M=2
       */
      M2(1),
      /**
       * M=4
       */
      M4(2),
      /**
       * M=8
       */
      M8(3);

      int rep; /* Gen2 air protocol representation of the value */
      TagEncoding(int rep)
      {
          this.rep = rep;
      }
      public static int get(TagEncoding te)
      {
          int tagEncode = -1;
          switch (te) {
              case FM0:
                  tagEncode = 0;
                  break;
              case M2:
                  tagEncode = 1;
                  break;
              case M4:
                  tagEncode = 2;
                  break;
              case M8:
                  tagEncode = 3;
                  break;
              default:
                  throw new IllegalArgumentException("Invalid Tag Encoding " + te);
          }
          return tagEncode;
      }
      private static final Map<Integer, TagEncoding> lookup = new HashMap<Integer, TagEncoding>();
      static
      {
          for (TagEncoding TagEncoding : EnumSet.allOf(TagEncoding.class))
          {
              lookup.put(TagEncoding.getCode(), TagEncoding);
          }
      }
      public int getCode()
      {
          return rep;
      }
      public static TagEncoding get(int rep)
      {
          return lookup.get(rep);
      }
    
  }
  /**
   * Divide Ratio values
   */
  public enum DivideRatio
  {
    /** 8 */
    DR8 (0),
      /** 64/3 */
      DR64_3 (1);

    int rep; /* Gen2 air protocol representation of the value */
    DivideRatio(int rep)
    {
      this.rep = rep;
    }
  }

  /**
   * TRext
   */ 
  public enum TrExt
  {
    NOPILOTTONE (0),
      PILOTTONE (1);

    int rep; /* Gen2 air protocol representation of the value */
    TrExt(int rep)
    {
      this.rep = rep;
    }
  }
  /**
   * Mode for write operation
   */
  public enum WriteMode
  {
    /* use standard write only */
    WORD_ONLY,
    /* use block write only */
    BLOCK_ONLY,
    /* use BlockWrite first, if fail, use standard write */
    BLOCK_FALLBACK;

  }

  /**
   * Algorithm choices for Q - superclass
   */
  public abstract static class Q
  {
  }

  /**
   * Dynamic Q algorithm
   */
  public static class DynamicQ extends Q
  {
    @Override
    public String toString()
    {
      return String.format("DynamicQ");
    }
  }

  /**
   * Static initial Q algorithm
   */
  public static class StaticQ extends Q
  {
    /**
     * The initial Q value to use
     */
    public int initialQ;

    /**
     * Create a static Q algorithim instance with a particular value.
     * 
     * @param initialQ the q value
     */
    public StaticQ(int initialQ)
    {
      this.initialQ = initialQ;
    }

    @Override
    public String toString()
    {
      return String.format("StaticQ(%d)", initialQ);
    }
  }

  public enum LinkFrequency
  {      
      LINK250KHZ(250),
      LINK320KHZ(320),
//      LINK400KHZ(400),
//      LINK40KHZ(40),
      LINK640KHZ(640);
    int rep;
    LinkFrequency(int rep)
    {
        this.rep=rep;
    }
    public static LinkFrequency getFrequency(int fr)
    {
        LinkFrequency freq ;
        switch(fr)
        {           
            case 0:
                freq = Gen2.LinkFrequency.LINK250KHZ;
                break;
            case 2:
                freq = Gen2.LinkFrequency.LINK320KHZ;
                break;
//            case 1:
//                freq = Gen2.LinkFrequency.LINK400KHZ;
//                break;
//            case 3:
//                freq = Gen2.LinkFrequency.LINK40KHZ;
//                break;
            case 4:
                freq = Gen2.LinkFrequency.LINK640KHZ;
                break;
            default:
                throw new IllegalArgumentException("Invalid Gen2 Link Frequency " + fr);
        }
        
        return freq;
    }

    public static int get(LinkFrequency lf)
    {
        int freq = -1;
        switch(lf)
        {
            case LINK250KHZ:
                freq = 0;
                break;
            case LINK320KHZ:
                freq = 2;
                break;
//            case LINK400KHZ:
//                freq = 1;
//                break;
//            case LINK40KHZ:
//                freq = 3;
//                break;
            case LINK640KHZ:
                freq = 4;
                break;
            default:
                throw new IllegalArgumentException("Invalid Gen2 Link Frequency " + lf);
        }

        return freq;
    }
  }

  public static class  Bap
  {
    /* Default to 3000 us */
    public int powerUpDelayUs = -1;
    /* Default to 20000 us */
    public int freqHopOfftimeUs = -1;

    public Bap(){
        
    }

    public Bap(int powerUpDelayUs,int freqHopOfftimeUs){
        this.powerUpDelayUs = powerUpDelayUs;
        this.freqHopOfftimeUs = freqHopOfftimeUs;
    }  
  }
    public enum Tari
    {

        TARI_25US(0),
        TARI_12_5US(1),
        TARI_6_25US(2);

        int rep;
        Tari(int rep)
        {
            this.rep = rep;
        }
        public static int getTari(Tari t)
        {
            int tari = -1;
            switch (t)
            {
                case TARI_12_5US:
                    tari = 0;
                    break;
                case TARI_25US:
                    tari = 1;
                    break;
                case TARI_6_25US:
                    tari = 2;
                    break;
            }
            return tari;
        }
        private static final Map<Integer, Tari> lookup = new HashMap<Integer, Tari>();
        static
        {
            for (Tari tari : EnumSet.allOf(Tari.class))
                lookup.put(tari.getCode(), tari);
        }
        public int getCode()
        {
            return rep;
        }
        public static Tari get(int rep)
        {
            return lookup.get(rep);
        }        
    }

  /**
   * Representation of a Gen2 Select operation
   */
  public static class Select implements TagFilter
  {
    /**
     * Whether to invert the selection (deselect tags that meet the
     * comparison and vice versa).
     */
    public boolean invert;

    /**
     * The memory bank in which to compare the mask
     */
    public Bank bank;
    
    /**
     * The location (in bits) at which to begin comparing the mask
     */
    public int bitPointer;
    /**
     * The length (in bits) of the mask
     */
    public int bitLength;
    /**
     * The mask value to compare with the specified region of tag
     * memory, MSB first
     */
    public byte[] mask; 

    public Select(boolean invert, Bank bank, int bitPointer,
                  int bitLength, byte[] mask)
    {
      this.invert = invert;
      if (bank == Bank.RESERVED)
      {
        throw new IllegalArgumentException("Gen2 Select may not operate on reserved memory bank");
      }
      this.bank = bank;
      this.bitPointer = bitPointer;
      this.bitLength = bitLength;
      this.mask =(mask==null) ? null: mask.clone();
    }

    public boolean matches(com.thingmagic.TagData t)
    {
      boolean match = true;
      int i, bitAddr;

      if (bank != Bank.EPC)
      {
        throw new UnsupportedOperationException(
          "Can't match against non-EPC memory");
      }

      i = 0;
      bitAddr = bitPointer;
      // Matching against the CRC and PC does not have defined
      // behavior; see section 6.3.2.11.1.1 of Gen2 version 1.2.0.
      // We choose to let it match, because that's simple.
      bitAddr -= 32;
      if (bitAddr < 0)
      {
        i -= bitAddr;
        bitAddr = 0;
      }

      for (; i < bitLength; i++, bitAddr++)
      {
        if (bitAddr > (t.epc.length*8))
        {
          match = false;
          break;
        }
        // Extract the relevant bit from both the EPC and the mask.
        if (((t.epc[bitAddr/8] >> (7-(bitAddr&7)))&1) !=
            ((mask[i/8] >> (7-(i&7)))&1))
        {
          match = false;
          break;
        }
      }
      if (invert)
      {
        match = match ? false : true;
      }
      return match;
    }

    @Override
    public String toString()
    {
      StringBuilder maskHex = new StringBuilder(mask.length * 2);

      for (byte b : mask)
      {
        maskHex.append(String.format("%02X", b));
      }
      
      return String.format("Gen2.Select:[%s%s,%d,%d,%s]",
                           invert ? "Invert," : "",
                           bank, bitPointer, bitLength, 
                           maskHex.toString());
    }

  }

  /**
   * Individual Gen2 lock bits.
   */
  public class LockBits
  {
      public static final int
      USER_PERM    = 1 << 0,
      USER         = 1 << 1,
      TID_PERM     = 1 << 2,
      TID          = 1 << 3,
      EPC_PERM     = 1 << 4,
      EPC          = 1 << 5,
      ACCESS_PERM  = 1 << 6,
      ACCESS       = 1 << 7,
      KILL_PERM    = 1 << 8,
      KILL         = 1 << 9;
  }

  /**
   * The arguments to a Reader.lockTag() method for Gen2 tags. 
   * Represents the "action" and "mask" bits.
   */
  public static class LockAction
    extends TagLockAction
  {

    /**
     * Lock Kill Password (readable and writable when access password supplied)
     */
    public static final LockAction KILL_LOCK = 
      new LockAction(LockBits.KILL | LockBits.KILL_PERM,
                     LockBits.KILL);
    /**
     * Unlock Kill Password (always readable and writable)
     */
    public static final LockAction KILL_UNLOCK = 
      new LockAction(LockBits.KILL | LockBits.KILL_PERM,
                     0);
    /**
     * Permanently Lock Kill Password (never readable or writable again)
     */
    public static final LockAction KILL_PERMALOCK = 
      new LockAction(LockBits.KILL | LockBits.KILL_PERM,
                     LockBits.KILL | LockBits.KILL_PERM);
    /**
     * Permanently Unlock Kill Password (always readable and writable, may never be locked again)
     */
    public static final LockAction KILL_PERMAUNLOCK = 
      new LockAction(LockBits.KILL | LockBits.KILL_PERM,
                     LockBits.KILL_PERM);

    /**
     * Lock Access Password (readable and writable when access password supplied)
     */
    public static final LockAction ACCESS_LOCK = 
      new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM,
                     LockBits.ACCESS);
    /**
     * Unlock Access Password (always readable and writable)
     */
    public static final LockAction ACCESS_UNLOCK = 
      new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM,
                     0);
    /**
     * Permanently Lock Access Password (never readable or writable again)
     */
    public static final LockAction ACCESS_PERMALOCK = 
      new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM,
                     LockBits.ACCESS | LockBits.ACCESS_PERM);
    /**
     * Permanently Unlock Access Password (always readable and writable, may never be locked again)
     */
    public static final LockAction ACCESS_PERMAUNLOCK = 
      new LockAction(LockBits.ACCESS | LockBits.ACCESS_PERM,
                     LockBits.ACCESS_PERM);

    /**
     * Lock EPC Memory (always readable, writable when access password supplied)
     */
    public static final LockAction EPC_LOCK = 
      new LockAction(LockBits.EPC | LockBits.EPC_PERM,
                     LockBits.EPC);
    /**
     * Unlock EPC Memory (always readable and writable)
     */
    public static final LockAction EPC_UNLOCK = 
      new LockAction(LockBits.EPC | LockBits.EPC_PERM,
                     0);
    /**
     * Permanently Lock EPC Memory (always readable, never writable again)
     */
    public static final LockAction EPC_PERMALOCK = 
      new LockAction(LockBits.EPC | LockBits.EPC_PERM,
                     LockBits.EPC | LockBits.EPC_PERM);
    /**
     * Permanently Unlock EPC Memory (always readable and writable, may never be locked again)
     */
    public static final LockAction EPC_PERMAUNLOCK = 
      new LockAction(LockBits.EPC | LockBits.EPC_PERM,
                     LockBits.EPC_PERM);
    /**
     * Lock TID Memory (always readable, writable when access password supplied)
     */
    public static final LockAction TID_LOCK = 
      new LockAction(LockBits.TID | LockBits.TID_PERM,
                     LockBits.TID);
    /**
     * Unlock TID Memory (always readable and writable)
     */
    public static final LockAction TID_UNLOCK = 
      new LockAction(LockBits.TID | LockBits.TID_PERM,
                     0);
    /**
     * Permanently Lock TID Memory (always readable, never writable again)
     */
    public static final LockAction TID_PERMALOCK = 
      new LockAction(LockBits.TID | LockBits.TID_PERM,
                     LockBits.TID | LockBits.TID_PERM);
    /**
     * Permanently Unlock TID Memory (always readable and writable, may never be locked again)
     */
    public static final LockAction TID_PERMAUNLOCK = 
      new LockAction(LockBits.TID | LockBits.TID_PERM,
                     LockBits.TID_PERM);
    /**
     * Lock User Memory (always readable, writable when access password supplied)
     */
    public static final LockAction USER_LOCK = 
      new LockAction(LockBits.USER | LockBits.USER_PERM,
                     LockBits.USER);
    /**
     * Unlock User Memory (always readable and writable)
     */
    public static final LockAction USER_UNLOCK = 
      new LockAction(LockBits.USER | LockBits.USER_PERM,
                     0);
    /**
     * Permanently Lock User Memory (always readable, never writable again)
     */
    public static final LockAction USER_PERMALOCK = 
      new LockAction(LockBits.USER | LockBits.USER_PERM,
                     LockBits.USER | LockBits.USER_PERM);
    /**
     * Permanently Unlock User Memory (always readable and writable, may never be locked again)
     */
    public static final LockAction USER_PERMAUNLOCK = 
      new LockAction(LockBits.USER | LockBits.USER_PERM,
                     LockBits.USER_PERM);


    final short mask, action;

    /**
     * Construct a new LockAction from a combination of other LockActions.
     *
     * @param actions lock actions to combine. If a data field is
     * repeated, the last one takes precedence; e.g.,
     * Gen2.LockAction.USER_LOCK, Gen2.LockAction.USER_UNLOCK turns
     * into Gen2.LockAction.USER_UNLOCK.
     */
    public LockAction(LockAction... actions)
    {
      short _mask, _action;

      _mask = 0;
      _action = 0;

      for (LockAction la : actions)
      {
        // Union mask
        _mask |= la.mask;
        // Overwrite action
        _action &= ~la.mask;
        _action |= (la.action & la.mask);
      }

      this.mask = _mask;
      this.action = _action;
    }

    /**
     * Convert the string representation into a LockAction object.
     *
     * @param value A string containing the name to convert. May be
     * the name of one of the predefined constants, or a
     * comma-separated list of the names.
     */
    public static LockAction parse(String value)
    {
      List<LockAction> actions = new Vector<LockAction>();

      initNameToLockAction();

      for (String v : value.toUpperCase().split(","))
      {
        if (nameToLockAction.containsKey(v))
        {
          actions.add(nameToLockAction.get(v));
        }
        else
        {
          throw new IllegalArgumentException("Unknown Gen2.LockAction " + v);
        }
      }
      return new LockAction(actions.toArray(new LockAction[actions.size()]));
    }

    private static Map<String,LockAction> nameToLockAction;
    private static Set<Map.Entry<String,LockAction>> nameToLockActionEntries;

    private static void initNameToLockAction() 
    {
      if (nameToLockAction != null)
      {
        return;
      }

      nameToLockAction = new HashMap<String,LockAction>(20);
      nameToLockAction.put("KILL_LOCK", KILL_LOCK);
      nameToLockAction.put("KILL_UNLOCK", KILL_UNLOCK);
      nameToLockAction.put("KILL_PERMALOCK", KILL_PERMALOCK);
      nameToLockAction.put("KILL_PERMAUNLOCK", KILL_PERMAUNLOCK);

      nameToLockAction.put("ACCESS_LOCK", ACCESS_LOCK);
      nameToLockAction.put("ACCESS_UNLOCK", ACCESS_UNLOCK);
      nameToLockAction.put("ACCESS_PERMALOCK", ACCESS_PERMALOCK);
      nameToLockAction.put("ACCESS_PERMAUNLOCK", ACCESS_PERMAUNLOCK);

      nameToLockAction.put("EPC_LOCK", EPC_LOCK);
      nameToLockAction.put("EPC_UNLOCK", EPC_UNLOCK);
      nameToLockAction.put("EPC_PERMALOCK", EPC_PERMALOCK);
      nameToLockAction.put("EPC_PERMAUNLOCK", EPC_PERMAUNLOCK);

      nameToLockAction.put("TID_LOCK", TID_LOCK);
      nameToLockAction.put("TID_UNLOCK", TID_UNLOCK);
      nameToLockAction.put("TID_PERMALOCK", TID_PERMALOCK);
      nameToLockAction.put("TID_PERMAUNLOCK", TID_PERMAUNLOCK);

      nameToLockAction.put("USER_LOCK", USER_LOCK);
      nameToLockAction.put("USER_UNLOCK", USER_UNLOCK);
      nameToLockAction.put("USER_PERMALOCK", USER_PERMALOCK);
      nameToLockAction.put("USER_PERMAUNLOCK", USER_PERMAUNLOCK);

      nameToLockActionEntries = nameToLockAction.entrySet();
    }
      
    /**
     * Construct a new LockAction from an explicit Gen2 action and mask
     * integer values. Only the bottom ten bits of each value are used.
     *
     * @param mask bitmask of which bits to set
     * @param action the bit values to set
     */
    public LockAction(int mask, int action)
    {
      this.mask = (short)(mask & 0x03ff);
      this.action = (short)(action & 0x03ff);
    }


    public String toString()
    {
      StringBuilder sb = new StringBuilder();
      boolean next = false;

      initNameToLockAction();

      for (Map.Entry<String,LockAction> kv : nameToLockActionEntries)
      {
        String name = kv.getKey();
        LockAction value = kv.getValue();
        
        if (((mask & value.mask) == value.mask) &&
            ((action & value.mask) == value.action))
        {
          if (next)
          {
            sb.append(",");
          }
          sb.append(name);
          next = true;
        }
      }
      return sb.toString();
    }
    
  }

  /**
   * Stores a 32-bit Gen2 password for use as an access or kill password.
   */
  public static class Password extends TagAuthentication
  {
    int value;

    /**
     * Create a new Password object.
     */ 
    public Password(int value )
    {
      this.value = value;
    }

    public int getValue()
    {
      return this.value;
    }
  }
  /** Embedded Tag operation : Write Data */
  public static class WriteData extends TagOp
  {
      /** Gen2  memory bank to write to */
      public Gen2.Bank Bank;

      /** word address to start writing at */
      public int WordAddress;

      /** Data to write */
      public short[] Data;
      /** Constructor to initialize the parameter of Write Data
       *
       * @param bank Memory Bank to Write
       * @param wordAddress Write starting address
       * @param data the data to write
       */
      public WriteData( Gen2.Bank bank,int wordAddress, short[] data)
      {
       this.Bank=bank;
       this.WordAddress=wordAddress;
       this.Data=data;
      }

  }

   /** Embedded Tag operation : Read Data */
  public static class ReadData extends TagOp
  {
      /** Gen2  memory bank to write to */
      public Gen2.Bank Bank;

      /** word address to start writing at */
      public int WordAddress;

      /** Number of words to read */
      public int Len;

      /** Gen2 memory banks to read */
      public EnumSet<Gen2.Bank> banks;
      /**Default Constructor */
      ReadData()
      {

      }

      /** Constructor to initialize the parameter of Read Data
       *
       * @param bank Memory Bank to read
       * @param wordAddress read starting address
       * @param length the length of data to read
       */
      public ReadData( Gen2.Bank bank,int wordAddress, byte length)
      {
       this.Bank=bank;
       this.WordAddress=wordAddress;
       this.Len=length;
      }

      /** Constructor to initialize the parameter of Read Data
       *
       * @param bank Memory Bank to read
       * @param wordAddress read starting address
       * @param length the length of data to read
       */
      public ReadData( EnumSet<Gen2.Bank> banks,int wordAddress, byte length)
      {
       this.banks=banks;
       this.WordAddress=wordAddress;
       this.Len=length;
  }
  }

  public static class SecurePasswordLookup extends TagAuthentication
  {
      /*
       * @param addressOffset the number of bits used to address the accessPassword list
       * @param addressLength the EPC word offset
       * @param flashOffset the user flash offset
       */
      public byte addressLength;
      public byte addressOffset;
      public byte[] flashOffset;

      /**
       * Create a new Password object.
       */
      public SecurePasswordLookup(byte addOffset, byte addLength, byte[] flashOffset)
      {
          this.addressLength = addLength;
          this.addressOffset = addOffset;
          this.flashOffset = flashOffset;
      }
  }

  public static class SecureReadData extends Gen2.ReadData
  {
      SecureTagType type;
      TagAuthentication password;
      
      /** Constructor to initialize the parameter of Read Data
       *
       * @param bank Memory Bank to read
       * @param wordAddress read starting address
       * @param length the length of data to read      
       * @param type the type of the tag
       * password the password given by user.
       */
      public SecureReadData(Gen2.Bank bank, int wordAddress, byte length, SecureTagType type, TagAuthentication password)
      {
          this.Bank = bank;
          this.WordAddress = wordAddress;
          this.Len = length;
          this.type = type;
          this.password = password;
      }
  }

  /*
   * Options to select Alien Higgs 3 Tag type and Monza 4 Tag
   */
  public static enum SecureTagType 
  {
      DEFAULT(0),
      HIGGS3(2),
      MONZA4(4);
      int type;

      private SecureTagType(int type)
      {
          this.type = type;
      }
      private static final Map<Integer, SecureTagType> lookup = new HashMap<Integer, SecureTagType>();

      static
      {
          for (SecureTagType tagType : EnumSet.allOf(SecureTagType.class))
          {
              lookup.put(tagType.getCode(), tagType);
          }
      }

      public int getCode()
      {
          return type;
      }

      public static SecureTagType get(int rep)
      {
          return lookup.get(rep);
      }
  }

   /** Embedded Tag operation : Lock */
  public static class Lock extends TagOp
  {
      /** Access Password */
      public int AccessPassword;

     
      /** New values of each bit specified in the mask */
      public LockAction Action;
      /** Constructor to initialize parameters of lock
       *
       * @param accessPassword The access password
       * @param action The lock Action
       */
      public Lock( int accessPassword,LockAction action)
      {
       this.AccessPassword=accessPassword;
      
       this.Action=action;
      }

  }

   /** Embedded Tag operation : Kill */
  public static class Kill extends TagOp
  {
      /**  Kill Password to use to kill the tag */
      public int KillPassword;


      /** Constructor to initialize parameters of Kill
       *
       * @param killPassword The kill password
      */
      public Kill( int killPassword)
      {
       this.KillPassword=killPassword;
      }
    }

  
   /** Write a new id to a tag */
  public static class WriteTag extends TagOp
  {
      /**  the new tag id to write  */
      public TagData Epc;


      /** Constructor to initialize parameters of WriteTag
       *
       * @param epc The tagid to write
       */
      public WriteTag( TagData epc)
      {
       this.Epc=epc;
      }
  }

 
   /** Block Write */
  public static class  BlockWrite extends TagOp
  {
      /**  the tag memory bank to write to  */
      public Gen2.Bank Bank;

      /** the word address to start writing to */
      public int WordPtr;

      /** the number words to write */
      public byte WordCount;

      /** the bytes to write */
      public short[] Data;

      /** Constructor to initialize parameters of Block
       *
       * @param bank The tag memory bank to write to
       * @param wordPtr Word Address to start writing to
       * @param wordCount The length of the data to write in words
       * @param data  Data to write
       */
      public BlockWrite(Gen2.Bank bank, int wordPtr, byte wordCount , short[] data)
      {
       this.Bank=bank;
       this.WordPtr=wordPtr;
       this.WordCount=wordCount;
       this.Data=data;
      }
    }

     /** Block PermaLock */
  public static class BlockPermaLock extends TagOp
  {
      /**  the tag memory bank to write to  */
      public Gen2.Bank Bank;
      /** Read or Lock ?? */
      public byte ReadLock;
      /** the starting word address to lock */
      public int BlockPtr;

      /** the number 16 blocks */
      public byte BlockRange;

      /** the Mask */
      public short[] Mask;

      /** Constructor to initialize parameters of Block
       *
       * @param bank The tag memory bank to write to
       * @param blockPtr Starting Address of the blocks to operate
       * @param blockRange Number of 16 blocks
       * @param mask  The mask
       */
      public BlockPermaLock(Gen2.Bank bank,byte readLock, int blockPtr, byte blockRange , short[] mask)
      {
       this.Bank=bank;
       this.ReadLock=readLock;
       this.BlockPtr=blockPtr;
       this.BlockRange=blockRange;
       this.Mask=mask;
      }
  }

  /** Block Erase */
  public static class BlockErase extends TagOp
  {
      /**  the tag memory bank to erase  */
      public Gen2.Bank Bank;
      
      /** the starting word address to erase */
      public int WordPtr;

      /** the number 16 blocks */
      public byte WordCount;

      
      /**
       * Constructor to initialize parameters of BlockErase
       * @param bank
       * @param wordPtr
       * @param wordCount
       */
      public BlockErase(Gen2.Bank bank, int wordPtr, byte wordCount)
      {
       this.Bank=bank;      
       this.WordPtr=wordPtr;
       this.WordCount=wordCount;              
      }
  }

  public enum Gen2_SiliconType
  {
      GEN2_ANY_SILICON(0x00),
      GEN2_ALIEN_HIGGS_SILICON(0x01),
      GEN2_NXP_G2XL_SILICON(0x02),
      GEN2_ALIEN_HIGGS3_SILICON(0x05),
      GEN2_HITACHI_HIBIKI_SILICON(0x06),
      GEN2_NXP_G2IL_SILICON(0x07),
      GEN2_IMPINJ_MONZA4_SILICON(0x08),
      GEN2_IAV_DENATRAN(0xB);
      int chipType;

      Gen2_SiliconType(int type)
      {
          this.chipType = type;
      }
  }


    public static class Gen2CustomTagOp extends TagOp
    {
        public static int chipType = -1;
    }

    public static class Alien extends Gen2CustomTagOp
    {
        public static class Higgs2 extends Alien
        {
            public Higgs2()
            {
                chipType = Gen2_SiliconType.GEN2_ALIEN_HIGGS_SILICON.chipType;
            }
            /**
             * Higgs2 Partial Load Image
             */
            public static class PartialLoadImage extends Higgs2
            {
                /** Kill password to write to the tag */
                public int killPassword;
                /** Access password to write to the tag */
                public int accessPassword;
                /** Tag EPC to write to the tag*/
                public byte[] epc;

                /**
                 * Constructor to initialize parameters of PartialLoadImage
                 * @param killPassword
                 * @param accessPassword
                 * @param epc
                 */
                public PartialLoadImage(int killPassword, int accessPassword, byte[] epc)
                {
                    this.killPassword = killPassword;
                    this.accessPassword = accessPassword;
                    this.epc = epc;
                }
            }
            /**
             * Higgs2 Full Load Image
             */
            public static class FullLoadImage extends Higgs2
            {
                /** Kill password to write to the tag */
                public int killPassword;
                /** Access password to write to the tag */
                public int accessPassword;
                /** lockAccess password to write to the tag */
                public int lockBits;
                /** pc bits to write to the tag */
                public int pcWord;
                /** Tag EPC to write to the tag*/
                public byte[] epc;

                /**
                 * Constructor to initialize parameters of PartialLoadImage
                 * @param killPassword
                 * @param accessPassword
                 * @param epc
                 */
                public FullLoadImage(int killPassword, int accessPassword, int lockBits, int pcWord, byte[] epc)
                {
                    this.killPassword = killPassword;
                    this.accessPassword = accessPassword;
                    this.lockBits = lockBits;
                    this.pcWord = pcWord;
                    this.epc = epc;
                }
            }//end of FullLoadImage inner class
        }//end of Higgs2 inner class

        public static class Higgs3 extends Alien
        {
            public Higgs3()
            {
                chipType = Gen2_SiliconType.GEN2_ALIEN_HIGGS3_SILICON.chipType;
            }
            /**
             * Higgs3 Block Read Lock
             */
            public static class BlockReadLock extends Higgs3
            {
                /** Access password to write to the tag */
                public int accessPassword;
                /** lockAccess password to write to the tag */
                public int lockBits;

                /**
                 * Constructor to initialize parameters of Higgs3BlockReadLock
                 * @param accessPassword
                 * @param lockBits
                 */
                public BlockReadLock(int accessPassword, int lockBits) {
                    this.accessPassword = accessPassword;
                    this.lockBits = lockBits;
                }
            }

            /**
             * Higgs3 FastLoadImage
             */
            public static class FastLoadImage extends Higgs3
            {
                /** current access password to write to the tag */
                public int currentAccessPassword;
                /** Access password to write to the tag */
                public int accessPassword;
                /** killPassword password to write to the tag */
                public int killPassword;
                /** pc bits to write to the tag */
                public int pcWord;
                /** Tag EPC to write to the tag*/
                public byte[] epc;

                /**
                 * Constructor to initialize parameters of Higgs3FastLoadImage
                 * @param currentAccessPassword The access password to use to write on the tag
                 * @param accessPassword The new access password to be written on the tag
                 * @param killPassword The new kill password to be written on the tag
                 * @param pcWord The PC word to be written on the tag
                 * @param epc The 96 bit EPC value to be written to the tag.
                 */
                public FastLoadImage(int currentAccessPassword,int accessPassword, int killPassword,int pcWord, byte[] epc)
                {
                    this.killPassword = killPassword;
                    this.accessPassword = accessPassword;
                    this.currentAccessPassword = currentAccessPassword;
                    this.pcWord = pcWord;
                    this.epc = epc;
                }
            }

            /**
             * Higgs3 LoadImage
             */
            public static class LoadImage extends Higgs3
            {
                /** current access password to write to the tag */
                public int currentAccessPassword;
                /** Access password to write to the tag */
                public int accessPassword;
                /** killPassword password to write to the tag */
                public int killPassword;
                /** pc bits to write to the tag */
                public int pcWord;
                /** Tag EPC to write to the tag*/
                public byte[] EPCAndUserData;

                /**
                 * Constructor to initialize parameters of Higgs3LoadImage
                 * @param currentAccessPassword The access password to use to write on the tag.
                 * @param accessPassword The new access password to be written on the tag.
                 * @param killPassword The new kill password to be written on the tag.
                 * @param pcWord The PC word to be written on the tag.
                 * @param EPCAndUserData 76 bytes of EPC and User memory data to be written on the tag.         
                 */
                public LoadImage(int currentAccessPassword,int accessPassword, int killPassword,
                        int pcWord, byte[] EPCAndUserData)
                {
                    this.killPassword = killPassword;
                    this.accessPassword = accessPassword;
                    this.currentAccessPassword = currentAccessPassword;
                    this.pcWord = pcWord;
                    this.EPCAndUserData = EPCAndUserData;
                }
            }// end of LoadImage inner class
        }//end of Higgs3 inner class
    }//end of Alien class
        
    public static class IDS extends Gen2CustomTagOp
    {

        public static class SL900A extends IDS
        {
            public enum Level
            {
                NOT_ALLOWED(0),
                SYSTEM(1),
                APPLICATION(2),
                MEASUREMENT(3);
                int rep;

                Level(int rep)
                {
                    this.rep = rep;
                }
                private static final Map<Integer, Level> lookup = new HashMap<Integer, Level>();

                static
                {
                    for (Level value : EnumSet.allOf(Level.class))
                    {
                        lookup.put(value.getCode(), value);
                    }
                }

                public int getCode() 
                {
                    return rep;
                }

                public static Level get(int rep)
                {
                    return lookup.get(rep);
                }
            }
            public enum DelayMode
            {
                // Start logging after delay time
                TIMER(0),
                // Start logging on external input
                EXTSWITCH(1);
                int rep;

                private DelayMode(int rep)
                {
                    this.rep = rep;
                }
            }
            public enum Sensor
            {
                TEMP(0),
                EXT1(1),
                EXT2(2),
                BATTV(3);
                int rep;

                Sensor(int rep)
                {
                    this.rep = rep;
                }
                private static final Map<Integer, Sensor> lookup = new HashMap<Integer, Sensor>();

                static
                {
                    for (Sensor value : EnumSet.allOf(Sensor.class))
                    {
                        lookup.put(value.getCode(), value);
                    }
                }

                public int getCode()
                {
                    return rep;
                }

                public static Sensor get(int rep)
                {
                    return lookup.get(rep);
                }
            }
            public enum LoggingForm
            {
                DENSE(0),
                OUTOFLIMITS(1),
                LIMITSCROSSING(3),
                IRQ1(5),
                IRQ2(6),
                IRQ1IRQ2(7);
                int rep;

                private LoggingForm(int rep)
                {
                    this.rep = rep;
                }
                private static final Map<Integer, LoggingForm> lookup = new HashMap<Integer, LoggingForm>();

                static
                {
                    for (LoggingForm value : EnumSet.allOf(LoggingForm.class))
                    {
                        lookup.put(value.getCode(), value);
                    }
                }

                public int getCode()
                {
                    return rep;
                }

                public static LoggingForm get(int rep)
                {
                    return lookup.get(rep);
                }
            }

            public enum StorageRule
            {
                NORMAL(0),
                ROLLING(1);
                int rep;

                private StorageRule(int rep)
                {
                    this.rep = rep;
                }
                private static final Map<Integer, StorageRule> lookup = new HashMap<Integer, StorageRule>();

                static
                {
                    for (StorageRule value : EnumSet.allOf(StorageRule.class))
                    {
                        lookup.put(value.getCode(), value);
                    }
                }

                public int getCode()
                {
                    return rep;
                }

                public static StorageRule get(int rep)
                {
                    return lookup.get(rep);
                }
            }
            public byte commandCode;
            public int accessPassword;
            public int password;
            public Level passwordLevel;

            public SL900A(byte commandCode)
            {
                this.commandCode = commandCode;
                chipType = 0x0A;
                accessPassword = 0;
                password = 0;
                passwordLevel = Level.NOT_ALLOWED;
            }

            public SL900A(byte commandCode, int password, byte passwordLevel)
            {
                this.commandCode = commandCode;
                chipType = 0x0A;
                accessPassword = 0;
                this.password = password;
                this.passwordLevel = Level.get(passwordLevel);
            }


            public static class StartLog extends SL900A
            {
                // Time to initialize log timestamp counter with
                public Calendar startTime;
                // Create Start Log tagop

                public StartLog(Calendar startTime)
                {
                    this(startTime, (byte)0, 0);
                }

                // Constructor to initialize the parameters of StartLog tagOp
                public StartLog(Calendar startTime, byte passwordLevel, int password)
                {
                    super((byte) 0xA7, password, passwordLevel);
                    this.startTime = startTime;
                }
            }

            // SL900A End Log tagop
            public static class EndLog extends SL900A
            {
                // Create EndLog tagop
                public EndLog()
                {
                    super((byte) 0xA6);
                }

                // Constructor to initialize the parameters of EndLog tagOp
                public EndLog(byte passwordLevel, int password)
                {
                    super((byte) 0xA6, password, passwordLevel);
            }
            }

            // Logging memory configuration
            public static class ApplicationData
            {
                short raw = 0;

                public short getNumberofWords()
                {
                    return (short) ((raw >> 7) & 0x1FF);
                }

                public void setNumberofWords(short value)
                {
                    if ((value & 0x1FF) != value) {
                        throw new IllegalArgumentException("Number of words must fit in 9 bits");
                    }
                    short mask = 0x1FF;
                    raw &= (short) ~mask;
                    raw |= (short) (value << 7);
                }

                public byte getBrokenWordPointer()
                {
                    return (byte) (raw & 0x7);
                }

                public void setBrokenWordPointer(byte value)
                {
                    if ((value & 0x7) != value)
                    {
                        throw new IllegalArgumentException("Broken word pointer must fit in 3 bits");
                    }
                    short mask = 0x7;
                    raw &= (short) ~mask;
                    raw |= (short) (value);
                }

                @Override
                // Human-readable representation
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "NumberofWords = " + getNumberofWords(),
                                "BrokenWordPointer = " + getBrokenWordPointer(),});
            }
            }

            // Delay Time structure for Initialize command
            public static class Delay
            {
                // Raw 16-bit protocol value
                short raw;
                public DelayMode mode;
                public short time;
                public boolean irqTimerEnable;

                // Create default Delay setting
                public Delay()
                {
                    mode = DelayMode.TIMER;
                    time = 0;
                    irqTimerEnable = false;
                }

                // Logging start mode
                public DelayMode getMode()
                {
                    return (0 == ((raw >> 1) & 0x1))
                            ? DelayMode.TIMER
                            : DelayMode.EXTSWITCH;
                }

                public void setMode(DelayMode value)
                {
                    short mask = 0x1 << 1;
                    if (DelayMode.TIMER == value)
                    {
                        raw &= (short) ~mask;
                    }
                    else
                    {
                        raw |= mask;
                    }
                }

                // Logging timer delay value (units of 512 seconds)
                public short getTime()
                {
                    return (short) ((raw >> 4) & 0xFFF);
                }

                public void setTime(short value)
                {
                    if ((value & 0xFFF) != value)
                    {
                        throw new IllegalArgumentException("Delay Time must fit in 12 bits");
                    }
                    short mask = (short) (0xFFF << 4);
                    raw &= (short) ~mask;
                    raw |= (short) (value << 4);
                }

                // Trigger log on both timer and external interrupts?
                public boolean getIrqTimerEnable()
                {
                    return (0 != (raw & 0x1));
                }

                public void setIrqTimerEnable(boolean value)
                {
                    short mask = 0x1;
                    if (value)
                    {
                        raw |= mask;
                    }
                    else
                    {
                        raw &= (short) ~mask;
                    }
                }
                @Override
                // Human-readable representation
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "Mode = " + getMode(),
                                "time = " + getTime(),
                                "irqTimerEnable = " + getIrqTimerEnable(),});
            }
            }

            // SL900A Initialize Tagop
            public static class Initialize extends SL900A
            {
                //  Create Initialize tagop
                public Initialize()
                {
                    this((byte)0,0);
                }

                // Constructor to initialize the parameters of Initialize tagOp
                public Initialize(byte passwordLevel, int password)
                {
                    super((byte) 0xAC, password, passwordLevel);
                    delayTime = new Delay();
                    appData = new ApplicationData();
                }
                // Log start delay settings
                public Delay delayTime;
                // Log memory configuration
                public ApplicationData appData;
            }

            // Sensor limit excursion counters
            public static class LimitCounter
            {
                private byte _extremeLower;
                private byte _lower;
                private byte _upper;
                private byte _extremeUpper;

                // Create LimitCounter reply object
                public LimitCounter(byte[] reply, int offset)
                {
                    _extremeLower = reply[offset + 0];
                    _lower = reply[offset + 1];
                    _upper = reply[offset + 2];
                    _extremeUpper = reply[offset + 3];
                }
                // Number of times selected sensor has gone beyond extreme lower limit

                public byte getExtremeLower()
                {
                    return _extremeLower;
                }

                // Number of times selected sensor has gone beyond lower limit
                public byte getLower()
                {
                    return _lower;
                }

                // Number of times selected sensor has gone beyond upper limit
                public byte getUpper()
                {
                    return _upper;
                }

                // Number of times selected sensor has gone beyond extreme upper limit
                public byte getExtremeUpper()
                {
                    return _extremeUpper;
                }

                @Override
                // Human-readable representation
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "ExtremeLower=" + getExtremeLower(),
                                "Lower=" + getLower(),
                                "Upper=" + getUpper(),
                                "ExtremeUpper=" + getExtremeUpper(),});
                }
            }

            public static class SystemStatus
            {
               int raw;
                // Create SystemStatus reply object
               public SystemStatus(byte[] reply, int offset)
               {
                    raw = ReaderUtil.byteArrayToInt(reply, offset);
               }

               // Measurement Address Pointer
               public short getMeasurementAddressPointer()
               {
                    return (short) ((raw >> 22) & 0x1FF);
               }

                // Number of memory replacements
               public byte getNumMemReplacements()
               {
                    return (byte) ((raw >> 16) & 0x3F);
               }

                // Number of measurements
                public short getNumMeasurements()
                {
                    return (short) ((raw >> 1) & 0x7FFF);
                }

                // Active
                public boolean getActive()
                {
                    return 0 != (raw & 0x1);
                }

                @Override
                // Human-readable representation
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "MeasurementAddressPointer=" + getMeasurementAddressPointer(),
                                "NumMemReplacements=" + getNumMemReplacements(),
                                "NumMeasurements=" + getNumMeasurements(),
                                "Active=" + getActive(),});
                }
            }

            public static class StatusFlags
            {
              byte raw;
                // Crate StatusFlags object from raw 1-byte reply
                public StatusFlags(byte reply)
                {
                   raw = reply;
                }

                // Logging active?
                public boolean getActive()
                {
                    return 0 != ((raw >> 7) & 1);
                }

                // Measurement area full?
                public boolean getFull()
                {
                    return 0 != ((raw >> 6) & 1);
                }
                // Measurement overwritten?
                public boolean getOverwritten()
                {
                    return 0 != ((raw >> 5) & 1);
                }

                // A/D error occurred?
                public boolean getADError()
                {
                  return 0 != ((raw >> 4) & 1);
                }

               // Low battery?
                public boolean getLowBattery()
                {
                   return 0 != ((raw >> 3) & 1);
                }

                // Shelf life low error?
                public boolean getShelfLifeLow()
                {
                    return 0 != ((raw >> 2) & 1);
                }
                // Shelf life high error?
                public boolean getShelfLifeHigh()
                {
                    return 0 != ((raw >> 1) & 1);
                }
                // Shelf life expired?
                public boolean getShelfLifeExpired()
                {
                    return 0 != ((raw >> 0) & 1);
                }

                @Override
                /// Human-readable representation
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "Active=" + getActive(),
                                "Full=" + getFull(),
                                "Overwritten=" + getOverwritten(),
                                "ADError=" + getADError(),
                                "LowBattery=" + getLowBattery(),
                                "ShelfLifeLow=" + getShelfLifeLow(),
                                "ShelfLifeHigh=" + getShelfLifeHigh(),
                                "ShelfLifeExpired=" + getShelfLifeExpired(),});
                }
            }

           // Get Log State reply
            public static class LogState
            {
                private LimitCounter _limitCount;
                private SystemStatus _systemStat;
                private StatusFlags _statFlags;

                // Create Get Log state reply object
                public LogState(byte[] reply)
                {
                    if (!((9 == reply.length) || (20 == reply.length)))
                    {
                        throw new IllegalArgumentException("GetLogState replies must be 9 or 21 bytes in length");
                    }
                    int offset = 0;
                    _limitCount = new LimitCounter(reply, offset);
                    offset += 4;
                    _systemStat = new SystemStatus(reply, offset);
                    offset += 4;
                    if (20 == reply.length)
                   {
                        // @todo Fully support shelf life arguments.  For now, just skip over them
                        offset += 8;
                        offset += 4;
                    }
                    _statFlags = new StatusFlags(reply[offset]);
                    offset += 1;
                }

                // Number of excursions beyond set limits
                public LimitCounter getLimitCount()
                {
                    return _limitCount;
                }

                // Logging system status
                public SystemStatus getSystemStat()
                {
                    return _systemStat;
                }

                // Logging status flags
                public StatusFlags getStatFlags()
                {
                   return _statFlags;
                }

                @Override
                // Human-readable representation
                public String toString() {
                    return ReaderUtil.join(" ", new String[]{
                                getLimitCount().toString(),
                               getSystemStat().toString(),
                                getStatFlags().toString()});
                }
            }

           // SL900A Get Log State tagop
            public static class GetLogState extends SL900A
            {
                // Create GetLogState tagop
                public GetLogState()
                {
                    super((byte) 0xA8);
                }

                // Constructor to initialize the parameters of GetLogState tagOp
                public GetLogState(byte passwordLevel, int password)
                {
                    super((byte) 0xA8, password, passwordLevel);
            }
            }

            public static class SensorReading
            {
                // Raw 16-bit response from GetSensorValue command
                short _reply;

                // Create Get Sensor Value reply object
                public SensorReading(short reply)
                {
                    _reply = reply;
                }

                // Create Get Sensor Value reply object
                public SensorReading(byte[] reply)
                {
                    if (2 != reply.length)
                    {
                        throw new IllegalArgumentException(String.format(
                                "Sensor Reading value must be exactly 2 bytes long (got {0} bytes)",
                                reply.length));
                    }
                    _reply = ReaderUtil.byteArrayToShort(reply, 0);
                }

                // Raw sensor reply
                public short getRaw()
                {
                    return _reply;
                }

                // Did A/D conversion error occur?
                public boolean getADError()
                {
                    return ((_reply >> 15) & 0x1) != 0;
                }

                // 5-bit Range/Limit value
                public byte getRangeLimit()
                {
                    return (byte) ((_reply >> 10) & 0x1F);
                }

                // 10-bit Sensor value
                public short getValue()
                {
                    return (short) ((_reply >> 0) & 0x3FF);
                }
            }

            // SL900A Get Sensor Value Tagop
            public static class GetSensorValue extends SL900A
            {
                // Which sensor to read
                public Sensor sensorType;

                // Constructor to initialize the parameters of GetSensorValue
                public GetSensorValue(Sensor sensorType)
                {
                    this(sensorType, (byte)0, 0);
                }

                // Constructor to initialize the parameters of GetSensorValue tagOp
                public GetSensorValue(Sensor sensorType, byte passwordLevel, int password)
                {
                    super((byte) 0xAD, password, passwordLevel);
                    this.sensorType = sensorType;
            }
            }

            // SL900A Set Log Mode Tagop
            public static class SetLogMode extends SL900A
            {
                // Create Set Log Mode Tagop

                public SetLogMode()
                {
                    super((byte) 0xA1);
                }

                // Constructor to initialize the parameters of SetLogMode tagOp
                public SetLogMode(byte passwordLevel, int password)
                {
                    super((byte)0xA1, password, passwordLevel);
                }

                // Logging Format
                public LoggingForm form = LoggingForm.DENSE;
                // Log Memory-Full Behavior
                public StorageRule storage = StorageRule.NORMAL;
                // Enable log for EXT1 external sensor
                public boolean ext1Enable = false;
                // Enable log for EXT2 external sensor
                public boolean ext2Enable = false;
                // Enable log for temperature sensor
                public boolean tempEnable = false;
                // Enable log for battery sensor
                public boolean battEnable = false;
                short _logInterval = 1;

                // Time (seconds) between log readings
                public short getLogInterval()
                {
                    return _logInterval;
                }

                public void setLogInterval(short value)
                {
                    if ((value & 0x7FFF) != value)
                    {
                        throw new IllegalArgumentException("Log interval must fit in 15 bits");
                    }
                    _logInterval = value;
                }
            }

            //SL900A Set Log Limit tag Op
            public static class SetLogLimit extends SL900A
            {
                LogLimit logLimit;

                //Create SetLogLimit object from raw 5-byte value
                public SetLogLimit(LogLimit value)
                {
                    this(value, (byte)0, 0);
                }

                // Constructor to initialize the parameters of SetLogLimit
                public SetLogLimit(LogLimit value, byte passwordLevel, int password)
                {
                    super((byte) 0xA2, password, passwordLevel);
                    logLimit = value;
                }
            }

            public static class LogLimit
            {
                long raw;

                public LogLimit()
                {

                }

                // Constructor to initialize the parameters of LogLimit
                public LogLimit(byte[] value)
                {
                    // byteArrayToLong requires 8 bytes of input, but LogLimit is only 5 bytes long.
                    // Create a temporary array to provide the necessary padding.
                    byte[] tmp = new byte[]{0, 0, 0, 0, 0, 0, 0, 0};
                    System.arraycopy(value, 0, tmp, 3, 5);
                    raw = ReaderUtil.byteArrayToLong(tmp);
                    raw &= 0x000000FFFFFFFFFFL;
                }

                public void setExtremeLowerLimit(short value)
                {
                    raw |= ReaderUtil.initBits(raw, 30, 10, value, "ExtremeLowerLimit");
                }

                public void setLowerLimit(short value)
                {
                    raw |= ReaderUtil.initBits(raw, 20, 10, value, "LowerLimit");
                }

                public void setUpperLimit(short value)
                {
                    raw |= ReaderUtil.initBits(raw, 10, 10, value, "UpperLimit");
                }

                public void setExtremeUpperLimit(short value)
                {
                    raw |= ReaderUtil.initBits(raw, 0, 10, value, "ExtremeUpperLimit");
                }

                /*
                 * Internal to API usage, created getters for SetLogLimit tag operation.
                 */
                protected short getExtremeUpperLimit()
                {
                    return (short) ReaderUtil.getBits(raw, 0, 10);
                }

                protected  short getExtremeLowerLimit()
                {
                    return (short) ReaderUtil.getBits(raw, 30, 10);
                }

                protected short getLowerLimit()
                {
                    return (short) ReaderUtil.getBits(raw, 20, 10);
                }

                protected short getUpperLimit()
                {
                    return (short) ReaderUtil.getBits(raw, 10, 10);
                }
            }
            // SL900A Access FIFO Tagop
            public static abstract class AccessFifo extends SL900A
            {
                // AccessFifo subcommand values
                public enum SubcommandCode
                {
                    // Read from FIFO
                    READ(0x80),
                    // Write to FIFO
                    WRITE(0xA0),
                    // Get FIFO status
                    STATUS(0xC0);
                    int rep;

                    private SubcommandCode(int rep)
                    {
                        this.rep = rep;
                    }
                }
                // AccessFifo subcommand code
                public SubcommandCode subcommand;

                // Create AccessFifo tagop
                public AccessFifo()
                {
                    super((byte) 0xAF);
                }

                // Constructor to initialize the parameters of AccessFifo tagOp
                public AccessFifo(byte passwordLevel, int password)
                {
                    super((byte) 0xAF, password, passwordLevel);
            }
            }

            // AccessFifo "Read Status" tagop
            public static class AccessFifoStatus extends AccessFifo
            {
                // Create AccessFifo "Read Status" tagop
                public AccessFifoStatus()
                {
                    subcommand = SubcommandCode.STATUS;
                }

                // Constructor to initialize the parameters of AccessFifoStatus tagOp
                public AccessFifoStatus(byte passwordLevel, int password)
                {
                    super(passwordLevel, password);
                    subcommand = SubcommandCode.STATUS;
            }
            }

            // AccessFifo "Read" tagop
            public static class AccessFifoRead extends AccessFifo
            {
                // Number of bytes to read from FIFO
                public byte length;

                // Create AccessFifo "Read" tagop
                public AccessFifoRead(byte length)
                {
                    this(length, (byte)0, 0);
                }

                // Constructor to initialize the parameters of AccessFifoRead tagOp
                public AccessFifoRead(byte length, byte passwordLevel, int password)
                {
                    super(passwordLevel, password);
                    subcommand = SubcommandCode.READ;
                    if (length != (length & 0xF))
                    {
                        throw new IllegalArgumentException("Invalid AccessFifo read length: " + length);
                    }
                    this.length = length;
                }
            }

            // Source of FIFO data
            public enum FifoSource
            {
                // Data from SPI
                SPI(0),
                // Data from RFID
                RFID(1);
                int rep;

                FifoSource(int rep)
                {
                    this.rep = rep;
                }
                private static final Map<Integer, FifoSource> lookup = new HashMap<Integer, FifoSource>();

                static
                {
                    for (FifoSource value : EnumSet.allOf(FifoSource.class))
                    {
                        lookup.put(value.getCode(), value);
                    }
                }

                public int getCode()
                {
                    return rep;
                }

                public static FifoSource get(int rep)
                {
                    return lookup.get(rep);
                }
            }

            // AccessFifo "Write" tagop
            public static class AccessFifoWrite extends AccessFifo
            {
                // Bytes to write to FIFO
                public byte[] payload;

                // Create AccessFifo "Write" tagop
                public AccessFifoWrite(byte[] payload)
                {
                    this(payload, (byte)0, 0);
                }

                // Constructor to initialize the parameters of AccessFifoWrite tagOp
                public AccessFifoWrite(byte[] payload, byte passwordLevel, int password)
                {
                    super(passwordLevel, password);
                    subcommand = SubcommandCode.WRITE;

                    if (payload.length != (payload.length & 0xF))
                    {
                        throw new IllegalArgumentException("Invalid AccessFifo write length: " + payload.length);
                    }
                    this.payload = payload;
                }
            }

            // FIFO Status return value
            public static class FifoStatus
            {
                // Raw 8-bit response from AccessFifo Status command
                byte _reply;

                // Create FifoStatus object from AccessFifo Status return value
                public FifoStatus(byte reply)
                {
                    _reply = reply;
                }

                // Create FIFO Status reply object
                public FifoStatus(byte[] reply)
                {
                    if (1 != reply.length)
                    {
                        throw new IllegalArgumentException(String.format(
                                "Fifo Status value must be exactly 1 byte long (got {0} bytes)",
                                reply.length));
                    }
                    _reply = reply[0];
                }

                // Raw 8-bit response from AccessFifo Status command
                public byte getRaw()
                {
                    return _reply;
                }

                // FIFO Busy bit
                public boolean getFifoBusy()
                {
                    return 0 != ((_reply >> 7) & 1);
                }

                // Data Ready bit
                public boolean getDataReady()
                {
                    return 0 != ((_reply >> 6) & 1);
                }

                // No Data bit
                public boolean getNoData()
                {
                    return 0 != ((_reply >> 5) & 1);
                }

                // Data Source bit (SPI, RFID)
                public FifoSource getSource()
                {
                    return FifoSource.get((_reply >> 4) & 1);
                }

                // Number of valid bytes in FIFO register
                public byte getNumValidBytes()
                {
                    return (byte) (_reply & 0xF);
                }

                // Returns a string that represents the current object.
                @Override
                public String toString() {
                    return ReaderUtil.join(" ", new String[]
                    {
                                "Raw=" + getRaw(),
                                "Busy=" + getFifoBusy(),
                                "Ready=" + getDataReady(),
                                "NoData=" + getNoData(),
                                "Source=" + getSource(),
                                "#bytes=" + getNumValidBytes(),});
                }
            }

            public static class CalibrationData
            {
                long raw;

                //Default Constructor
                public CalibrationData()
                {

        }

                // Create CalibrationData object from raw 2-byte reply
                public CalibrationData(byte[] reply, int offset)
                {
                    // byteArrayToLong requires 8 bytes of input, but CalibrationData is only 7 bytes long.
                    // Create a temporary array to provide the necessary padding.
                    byte[] tmp = new byte[]{0, 0, 0, 0, 0, 0, 0, 0};
                    System.arraycopy(reply, offset, tmp, 1, 7);
                    raw = ReaderUtil.byteArrayToLong(tmp);
                    //this String(00FFFFFFFFFFFFFF) is formed coresponding to each nibble in raw;
                    raw &= 0x00FFFFFFFFFFFFFFL;
    }

                // AD1 lower voltage reference - fine - DO NOT MODIFY
                public byte getAd1()
                {
                    return (byte) ReaderUtil.getBits(raw, 51, 5);
                }

                // AD1 lower voltage reference - coarse
                public byte getCoarse1()
                {
                    return (byte) ReaderUtil.getBits(raw, 48, 3);
                }
                public void setCoars1(byte value)
                {
                    raw = ReaderUtil.initBits(raw, 48, 3, value, "Coarse1");
                }

                // AD2 lower voltage reference - fine - DO NOT MODIFY
                public byte getAd2()
                {
                    return (byte) ReaderUtil.getBits(raw, 43, 5);
                }

                // AD2 lower voltage reference - coarse
                public byte getCoarse2()
                {
                    return (byte) ReaderUtil.getBits(raw, 40, 3);
                }
                public void setCoarse2(byte value)
                {
                    raw = ReaderUtil.initBits(raw, 40, 3, value, "Coarse2");
                }

                // Switches the lower AD voltage reference to ground
                public boolean getGndSwitch()
                {
                    return 0 != ReaderUtil.getBits(raw, 39, 1);
                }
                public void setGndSwitch(boolean value)
                {
                    long data = (value ? 1 : 0);
                    raw = ReaderUtil.initBits(raw, 39, 1, data, "GndSwitch");
                }

                // POR voltage level for 1.5V system
                public byte getSelp12()
                {
                    return (byte) ReaderUtil.getBits(raw, 37, 2);
                }
                public void setSelp12(byte value)
                {
                    raw = ReaderUtil.initBits(raw, 37, 2, value, "Selp12");
                }

                // Main reference voltage calibration -- DO NOT MODIFY
                public byte getAdf()
                {
                    return (byte) ReaderUtil.getBits(raw, 32, 5);
                }

                // RTC oscillator calibration
                public byte getDf()
                {
                    return (byte) ReaderUtil.getBits(raw, 24, 8);
                }
                public void setDf(byte value)
                {
                    raw = ReaderUtil.initBits(raw, 24, 8, value, "Df");
                }

                // Controlled battery supply for external sensor - the battery voltage is connected to the EXC pin
                public boolean getSwExtEn()
                {
                    return 0 != ReaderUtil.getBits(raw, 23, 1);
                }
                public void setSwExtEn(boolean value)
                {
                    long data = (value ? 1 : 0);
                    raw = ReaderUtil.initBits(raw, 23, 1, data, "SwExtEn");
                }

                // POR voltage level for 3V system
                public byte getSelp22()
                {
                    return (byte) ReaderUtil.getBits(raw, 21, 2);
                }
                public void setSelp22(byte value)
                {
                    raw = ReaderUtil.initBits(raw, 21, 2, value, "Selp22");
                }

                // Voltage level interrupt level for external sensor -- ratiometric
                public byte getIrlev()
                {
                    return (byte) ReaderUtil.getBits(raw, 19, 2);
                }
                public void setIrlev(byte value)
                {
                    raw = ReaderUtil.initBits(raw, 19, 2, value, "Irlev");
                }

                // Main system clock oscillator calibration -- DO NOT MODIFY
                public byte getRingCal()
                {
                    return (byte) ReaderUtil.getBits(raw, 14, 5);
                }

                // Temperature conversion offset calibration -- DO NOT MODIFY
                public byte getOffInt()
                {
                    return (byte) ReaderUtil.getBits(raw, 7, 7);
                }

                // Bandgap voltage temperature coefficient calibration -- DO NOT MODIFY
                public byte getReftc()
                {
                    return (byte) ReaderUtil.getBits(raw, 3, 4);
                }

                // Excitate for resistive sensors without DC
                public boolean getExcRes()
                {
                    return 0 != ReaderUtil.getBits(raw, 2, 1);
                }
                public void setExcRes(boolean value)
                {
                    long data = (value ? 1 : 0);
                    raw = ReaderUtil.initBits(raw, 2, 1, data, "ExcRes");
                }

                //  Reserved for Future Use
                public byte getRFU()
                {
                    return (byte) ReaderUtil.getBits(raw, 0, 2);
                }

                @Override
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "Ad1=" + getAd1(),
                                "Coarse1=" + getCoarse1(),
                                "Ad2=" + getAd2(),
                                "Coarse2=" + getCoarse2(),
                                "GndSwitch=" + getGndSwitch(),
                                "Selp12=" + getSelp12(),
                                "Adf=" + getAdf(),
                                "Df=" + getDf(),
                                "SwExtEn=" + getSwExtEn(),
                                "Selp22=" + getSelp22(),
                                "Irlev=" + getIrlev(),
                                "RingCal=" + getRingCal(),
                                "OffInt=" + getOffInt(),
                                "Reftc=" + getReftc(),
                                "ExcRes=" + getExcRes(),});
                }
          }

          // SL900A Get Calibration Data tagop
          public static class GetCalibrationData extends SL900A
          {
                // Create GetCalibrationData tagop
                public GetCalibrationData()
                {
                    super((byte) 0xA9);
                }

                // Constructor to initialize the parameters of GetCalibrationData tagOp
                public GetCalibrationData(byte passwordLevel, int password)
                {
                    super((byte) 0xA9, password, passwordLevel);
                }
           }

           // SL900A Set Calibration Data tagop
           public static class SetCalibrationData extends SL900A
           {
               // Calibration Data
               public CalibrationData cal;

               // Create SetCalibrationData tagop
               public SetCalibrationData(CalibrationData cal)
               {
                   this(cal, (byte)0, 0);
               }

               // Constructor to initialize the parameters of SetCalibrationData tagOp
               public SetCalibrationData(CalibrationData cal, byte passwordLevel, int password)
               {
                   super((byte) 0xA5, password, passwordLevel);
                   this.cal = cal;
               }
           }

             // Sensor Front End Parameters
           public static class SfeParameters
           {
               // Raw 16-bit SFE parameters value
               short raw;

               public SfeParameters()
               {

               }
               // Create SFEParameters object from raw 2-byte reply
               public SfeParameters(byte[] reply, int offset)
               {
                   raw = ReaderUtil.byteArrayToShort(reply, offset);
               }

               // External sensor 2 range
               public byte getRang()
               {
                   return (byte) ReaderUtil.getBits(raw, 11, 5);
               }
               public void setRang(byte value)
               {
                   raw = (short) ReaderUtil.initBits(raw, 11, 5, value, "Rang");
               }

               // External sensor 1 range
               public byte getSeti()
               {
                   return (byte) ReaderUtil.getBits(raw, 6, 5);
               }
               public void setSeti(byte value)
               {
                   raw = (short) ReaderUtil.initBits(raw, 6, 5, value, "Seti");
               }

               /*
                *  External sensor 1 type
                *  00 -- linear resistive sensor
                *  01 -- high impedance input (voltage follower), bridge
                *  10 -- capacitive sensor with DC
                *  11 -- capacitive or resistive sensor without DC
                */
               public byte getExt1()
               {                   
                   return (byte) ReaderUtil.getBits(raw, 4, 2);
               }
               public void setExt1(byte value)
               {
                   raw = (short)ReaderUtil.initBits(raw, 4, 2, value, "Ext1");
               }

               /*
                *  External sensor 2 type
                *  00 -- linear conductive sensor
                *  01 -- high impedance input (voltage follower), bridge
                */
               public byte getExt2()
               {
                   return (byte) ReaderUtil.getBits(raw, 3, 1);
               }
               public void setExt2(byte value)
               {
                   raw = (short) ReaderUtil.initBits(raw, 3, 1, value, "Ext2");
               }

               // Use preset range
               public boolean getAutorangeDisable()
               {
                   return 0 != ReaderUtil.getBits(raw, 2, 1);
               }
               public void setAutorangeDisable(boolean value)
               {
                   long data = (value ? 1 : 0);
                   raw = (short) ReaderUtil.initBits(raw, 2, 1, data, "AutorangeDisable");
               }

                /*
                 * Sensor used in limit check
                 * 00 - first selected sensor
                 * 01 -- second selected sensor
                 * 10 -- third selected sensor
                 * 11 -- fourth selected sensor
                 */
                public byte getVerifySensorID()
                {
                    return (byte) ReaderUtil.getBits(raw, 0, 2);
                }
                public void setVerifySensorID(byte value)
                {
                    raw = (short) ReaderUtil.initBits(raw, 0, 2, value, "VerifySensorID");
                }

                // Human-readable representation
                @Override
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "Rang=" + getRang(),
                                "Seti=" + getSeti(),
                                "Ext1=" + getExt1(),
                                "Ext2=" + getExt2(),
                                "AutorangeDisable=" + getAutorangeDisable(),
                                "VerifySensorID=" + getVerifySensorID(),});
                }
            }

            // SL900A Set SFE Parameters tagop
            public static class SetSfeParameters extends SL900A
            {
                // Calibration Data
                public SfeParameters sfeParameter;

                // Create SetSfeParameters tagop
                public SetSfeParameters(SfeParameters sfe)
                {
                    this(sfe, (byte)0, 0);
                }

                // Constructor to initialize the parameters of SetSfeParameters tagOp
                public SetSfeParameters(SfeParameters sfe, byte passwordLevel, int password)
                {
                    super((byte) 0xA4, password, passwordLevel);
                    sfeParameter = sfe;
                }
            }

            public static class CalSfe
            {
                // Calibration Data
                public CalibrationData calibrationData;
                // Sensor Front End Parameters
                public SfeParameters sfeParameter;

                // Create Calibration Data / SFE Parameter object from raw 72-bit reply
                public CalSfe(byte[] reply, int offset)
                {
                    calibrationData = new CalibrationData(reply, offset + 0);
                    sfeParameter = new SfeParameters(reply, offset + 7);
                }

                // Human-readable representation
                @Override
                public String toString()
                {
                    return ReaderUtil.join(" ", new String[]{
                                "Cal=(" + calibrationData + ")",
                                "Sfe=(" + sfeParameter + ")",});
                }
            }

            // SL900A  Log Mode Data
            public static class LogModeData
            {
                // Raw 8-bit response from Measurement Setup Data command
                byte _reply;

                // Create LogModeData object from Measurement Setup Data return value
                public LogModeData(byte reply)
                {
                    _reply = reply;
                }

                // Create LogModeData reply object
                public LogModeData(byte[] reply)
                {
                    if (1 != reply.length) {
                        throw new IllegalArgumentException(
                                "LogModeData value must be exactly 1 byte long (got {0} bytes)"
                                + reply.length);
                    }
                    _reply = reply[0];
                }

                // Raw 8-bit response from Measurement Setup Data command
                public byte getRaw()
                {
                    return _reply;
                }

                // Logging Format
                public LoggingForm getForm()
                {
                    return LoggingForm.get((_reply >> 5) & 7);
                }

                // Log Memory-Full Behavior
                public StorageRule getStorage()
                {
                    return StorageRule.get(((_reply >> 4) & 1));
                }

                // Enable log for EXT1 external sensor
                public boolean getExt1SensorEnable()
                {
                    return 0 != ((_reply >> 3) & 1);
                }

                // Enable log for EXT2 external sensor
                public boolean getExt2SensorEnable()
                {
                    return 0 != ((_reply >> 2) & 1);
                }

                // Enable log for temperature sensor
                public boolean getTempSensorEnable()
                {
                    return 0 != ((_reply >> 1) & 1);
                }

                // Enable log for battery sensor
                public boolean getBattCheckEnable()
                {
                    return 0 != ((_reply >> 0) & 1);
                }

                // Returns a string that represents the current object.
                @Override
                public String toString() {
                    return ReaderUtil.join(" ", new String[]{
                                "Raw = " + getRaw(),
                                "LoggingForm = " + getForm(),
                                "StorageRule = " + getStorage(),
                                "EXT1 = " + getExt1SensorEnable(),
                                "EXT2 = " + getExt2SensorEnable(),
                                "Temperature Sensor = " + getTempSensorEnable(),
                                "Battery Check = " + getBattCheckEnable(),});
                }
            }

            // Measurement Setup Data object
            public static class MeasurementSetupData
            {
                // Raw 16 byte response from Measurement Setup Data command
                byte[] raw;
                // Limit Counter Data
                public GetMeasurementLogLimit lmtCounter;
                // Log Mode Data
                public LogModeData logModeData;
                // Delay Data
                public Delay delayData;
                // Application Data
                public ApplicationData appData;
                // Time (seconds) between log readings
                short logInterval;
                // Start Time
                Calendar startTime;

                // Create Measurement Setup Data object from raw 16 byte reply
                public MeasurementSetupData(byte[] reply)
                {
                    raw = reply;
                }

                // Create Measurement Setup Data object from raw 16 byte reply
                public MeasurementSetupData(byte[] reply, int offset)
                {
                    if (16 != reply.length)
                    {
                        throw new IllegalArgumentException("MeasurementSetupData value must be exactly 16 byte long (got {0} bytes)"
                                + reply.length);
                    }
                    byte[] data;
                    //Start Time
                    data = new byte[4];
                    System.arraycopy(reply, 0, data, 0, 4);
                    startTime = SerialReader.fromSL900aTime(ReaderUtil.byteArrayToInt(data, 0));
                    data = null;

                    //Limit Counter
                    data = new byte[]{0,0,0,0,0,0,0,0};
                    System.arraycopy(reply, 4, data, 3, 5);
                    short[] logModeData1 = parseLogLimitsData(data);
                    lmtCounter = new GetMeasurementLogLimit(logModeData1, 0);
                    data = null;

                    //Log Mode
                    logModeData = new LogModeData(reply[9]);

                    //Log Interval
                    data = new byte[2];
                    System.arraycopy(reply, 10, data, 0, 2);
                    logInterval = (short) (((ReaderUtil.byteArrayToShort(data, 0)) >> 1) & 0x0001);
                    data = null;

                    //Delay time
                    data = new byte[2];
                    Delay rawDelayData = new Delay();
                    System.arraycopy(reply, 12, data, 0, 2);
                    rawDelayData.raw = ReaderUtil.byteArrayToShort(data, 0);
                    delayData = rawDelayData;
                    data = null;

                    //Application Data
                    data = new byte[2];
                    ApplicationData rawAppData = new ApplicationData();
                    System.arraycopy(reply, 14, data, 0, 2);
                    rawAppData.raw = ReaderUtil.byteArrayToShort(data, 0);
                    appData = rawAppData;
                    data = null;

                    raw = reply;
                }

                // Human-readable representation
                @Override
                public String toString() {
                    return ReaderUtil.join(" ", new String[]{
                                "StartTime = (" + startTime.getTime() + ")",
                                "LogLimits = (" + lmtCounter.toString() + ")",
                                "LogMode = (" + logModeData.toString() + ")",
                                "LogInterval = (" + logInterval + ")",
                                "DelayTime = (" + delayData.toString() + ")",
                                "ApplicationData = (" + appData.toString() + ")",});
                }
            }

            public static short[] parseLogLimitsData(byte[] data)
            {
               long value = 0x0L;
               short[] _data = new short[4];
               value = ReaderUtil.byteArrayToLong(data);
                _data[3] = (short) ReaderUtil.getBits(value, 0, 10);
                _data[2] = (short) ReaderUtil.getBits(value, 10, 10);
                _data[1] = (short) ReaderUtil.getBits(value, 20, 10);
                _data[0] = (short) ReaderUtil.getBits(value, 30, 10);

                return _data;
            }
            // SL900A Get GetMeasurementSetup Data tagop
            public static class GetMeasurementSetup extends SL900A
            {
                // Create GetMeasurementSetup tagop
                public GetMeasurementSetup()
                {
                    super((byte) 0xA3);
                }

                // Constructor to initialize the parameters of GetMeasurementSetup tagOp
                public GetMeasurementSetup(byte passwordLevel, int password)
                {
                    super((byte) 0xA3, password, passwordLevel);
                }
            }

            // Sensor limit excursion counters
            public static class GetMeasurementLogLimit
            {
                private short _extremeLower;
                private short _lower;
                private short _upper;
                private short _extremeUpper;

                // Create LimitCounter reply object
                public GetMeasurementLogLimit(short[] reply, int offset)
                {
                    _extremeLower = reply[offset + 0];
                    _lower = reply[offset + 1];
                    _upper = reply[offset + 2];
                    _extremeUpper = reply[offset + 3];
                }

                // Number of times selected sensor has gone beyond extreme lower limit
                public short getExtremeLower()
                {
                    return _extremeLower;
                }

                // Number of times selected sensor has gone beyond lower limit
                public short getLower()
                {
                   return _lower;
                }

                // Number of times selected sensor has gone beyond upper limit
                public short getUpper()
                {
                    return _upper;
                }

                // Number of times selected sensor has gone beyond extreme upper limit
                public short getExtremeUpper()
                {
                   return _extremeUpper;
                }

                @Override
                // Human-readable representation
                public String toString()
                {

                    return ReaderUtil.join(" ", new String[]{
                                String.format("ExtremeLower=%x",getExtremeLower()),
                                String.format("Lower=%x", getLower()),
                                String.format("Upper=%x", getUpper()),
                                String.format("ExtremeUpper=%x",getExtremeUpper()),});
                }
            }

            // IDS SL900A Get Battery Level reply value
            public static class BatteryLevelReading
            {
                short _reply;
                public BatteryLevelReading(short _reply)
                {
                    this._reply = _reply;
                }
                // Create BatteryLevelReading Value reply object
                public BatteryLevelReading(byte[] reply)
                {
                    if (2 != reply.length)
                    {
                        throw new IllegalArgumentException(String.format(
                                "BatteryLevel Reading value must be exactly 2 bytes long (got {0} bytes)",
                                reply.length));
                    }
                    _reply = ReaderUtil.byteArrayToShort(reply, 0);
                }

                // Raw Battery reply
                public short getRaw()
                {
                    return _reply;
                }

                // Did A/D conversion error occur?
                public boolean getADError()
                {
                    return ((_reply >> 15) & 0x1) != 0;
                }

                // 1-bit BatteryType
                public int getBatteryType()
                {
                    return ((_reply >> 14) & 0x1F) ;
                }

                // 10-bit Battery value
                public short getValue()
                {
                    return (short) ((_reply >> 0) & 0x3FF);
                }

                @Override
                // Human-readable representation
                public String toString() {
                    return ReaderUtil.join(" ", new String[]{
                               "ADError : " + getADError(),
                               "BatteryType : " + getBatteryType(),
                               "Battery Level : " + getValue()});
                }
            }

            //enum BatterType, re-check or default
            public enum BatteryType
            {
                CHECK(0),
                RECHECK(1);
                int rep;
                private BatteryType(int rep)
                {
                    this.rep = rep;
                }
            }

            // SL900A Get Battery Level Tagop
            public static class GetBatteryLevel extends SL900A
            {
                BatteryType type;
                public GetBatteryLevel(BatteryType type)
                {
                    this(type, (byte)0, 0);
                }

                // Constructor to initialize the parameters of GetBatteryLevel tagOp
                public GetBatteryLevel(BatteryType type, byte passwordLevel, int password)
                {
                    super((byte)0xAA, password, passwordLevel);
                    this.type = type;
                }
            }

            //SL900A Set Password Tagop
            public static class SetPassword extends SL900A
            {
                int newPassword;
                byte newPasswordLevel;

                public SetPassword(byte passwordLevel, int password)
                {
                    this((byte)0, 0, passwordLevel, password);
                }

                // Constructor to initialize the parameters of SetPassword tagOp
                public SetPassword(byte currentPasswordLevel, int currentPassword, byte passwordLevel, int password)
                {
                    super((byte)0xA0, currentPassword, currentPasswordLevel);
                    this.newPassword = password;
                    this.newPasswordLevel = passwordLevel;
                }
            }

            public static class ShelfLifeBlock0
            {
                public int raw;

                public ShelfLifeBlock0()
                {

                }
                public ShelfLifeBlock0(byte[] value, int offset)
                {
                    raw = ReaderUtil.byteArrayToInt(value, offset);
                }
                public void setTmax(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 24, 8, value, "Tmax");;
                }
                public void setTmin(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 16, 8, value, "Tmin");
                }
                public void setTstd(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 8, 8, value, "Tstd");
                }
                public void setEa(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 0, 8, value, "Ea");
                }

                //Internal to API usage, created getters for ShelfLifeBlock0 tag operation
                protected byte getTmax()
                {
                    return (byte) ReaderUtil.getBits(raw, 24, 8);
                }
                protected byte getTmin()
                {
                    return  (byte) ReaderUtil.getBits(raw, 16, 8);
                }
                protected byte getTstd()
                {
                    return (byte) ReaderUtil.getBits(raw, 8, 8);
                }
                protected byte getEa()
                {
                    return (byte) ReaderUtil.getBits(raw, 0, 8);
                }
            }

            public static class ShelfLifeBlock1
            {
                public int raw;
                public ShelfLifeBlock1()
                {

                }
                public ShelfLifeBlock1(byte[] value, int offset)
                {
                    raw = ReaderUtil.byteArrayToInt(value, offset);
                }
                public void setSLinit(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 16, 16, value, "Tint");
                }
                public void setTint(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 6, 10, value, "Tint");
                }
                public void setSensor(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 4, 2, value, "Sensor");
                }
                public void setEnableNegative(boolean  value)
                {
                    int data = (value ? 1 : 0);
                    raw = (int) ReaderUtil.initBits(raw, 3, 1, data, "EnableNegative");
                }
                public void setAlgorithmEnable(boolean  value)
                {
                    int data = (value ? 1 : 0);
                    raw = (int) ReaderUtil.initBits(raw, 2, 1, data, "AlgorithmEnable");
                }
                private void setRFU(byte value)
                {
                    raw = (int) ReaderUtil.initBits(raw, 0, 2, 0, "RFU");
                }

                //Internal to API usage, created getters for ShelfLifeBlock1 tag operation
                protected byte getSLinit()
                {
                    return (byte) ReaderUtil.getBits(raw, 16, 16);
                }
                protected byte getTint()
                {
                    return  (byte) ReaderUtil.getBits(raw, 6, 10);
                }
                protected byte getSensor()
                {
                    return (byte) ReaderUtil.getBits(raw, 4, 2);
                }
                protected boolean  getEnableNegative()
                {
                    return 0 != ReaderUtil.getBits(raw, 3, 1);
                }
                protected boolean  getAlgorithmEnable()
                {
                    return 0 != ReaderUtil.getBits(raw, 2, 1);
                }
            }

            public static class SetShelfLife extends SL900A
            {
               ShelfLifeBlock0 slBlock0;
               ShelfLifeBlock1 slBlock1;
               public SetShelfLife(ShelfLifeBlock0 slBlock0, ShelfLifeBlock1 slBlock1)
               {
                   this(slBlock0, slBlock1, (byte)0, 0);
               }

               public SetShelfLife(ShelfLifeBlock0 slBlock0, ShelfLifeBlock1 slBlock1, byte passwordLevel, int password)
               {
                   super((byte)0xAB, password, passwordLevel);
                   this.slBlock0 = slBlock0;
                   this.slBlock1 = slBlock1;
               }
            }
       }
    }
    
    /**
     * NXP Gen2 Tag Operation
     */
    protected static class NxpGen2TagOp extends Gen2CustomTagOp
    {
        /** Access password to write to the tag */
        public int accessPassword;

        public NxpGen2TagOp()
        {
            this.accessPassword = 0;            
        }
        public NxpGen2TagOp(int accessPassword)
        {
            this.accessPassword = accessPassword;
        }        
        
        /**
         * NxpSetReadProtect
         */
        public abstract static class SetReadProtect extends NxpGen2TagOp
        {
            /**
             * Constructor to initialize parameters of NxpSetReadProtect
             * @param accessPassword             
             */
            public SetReadProtect(int accessPassword)
            {
                super(accessPassword);
            }
        }

         /**
         * NxpResetReadProtect
         */
        public abstract static class ResetReadProtect extends NxpGen2TagOp
        {
            /**
             * Constructor to initialize parameters of NxpResetReadProtect
             * @param accessPassword             
             */
            public ResetReadProtect(int accessPassword)
            {
                super(accessPassword);
            }
        }


        /**
         * NxpChangeEas
         */
        public abstract static class ChangeEas extends NxpGen2TagOp
        {
            /** reset */
            public boolean reset;

            /**
             * Constructor to initialize parameters of NxpChangeEas
             * @param accessPassword The access password to be used
             * @param reset Set or Reset the EAS bit             
             */
            public ChangeEas(int accessPassword, boolean reset)
            {
                super(accessPassword);
                this.reset = reset;
            }
        }

        /**
         * NxpEasAlarm
         */
        public abstract static class EasAlarm extends NxpGen2TagOp
        {
            public Gen2.DivideRatio divideRatio;
            public Gen2.TagEncoding tagEncoding;
            public Gen2.TrExt trExt;
        }

        public abstract static class Calibrate extends NxpGen2TagOp
        {
            /**
             * Constructor to initialize parameters of NxpCalibrate
             * @param accessPassword
             */
            public Calibrate(int accessPassword)
            {
                super(accessPassword);
            }
        }
    }

    public static class NXP extends Gen2CustomTagOp
    {        
        public static class G2X extends NxpGen2TagOp
        {
            public G2X()
            {
                chipType = Gen2_SiliconType.GEN2_NXP_G2XL_SILICON.chipType;
            }
            /**
            * NxpSetReadProtect
            */
            public static class SetReadProtect extends NxpGen2TagOp.SetReadProtect
            {
                /**
                 * Constructor to initialize parameters of NxpSetReadProtect
                 * @param accessPassword
                 */
                public SetReadProtect(int accessPassword)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2XL_SILICON.chipType;
                }
            }

             /**
             * NxpResetReadProtect
             */
            public static class ResetReadProtect extends NxpGen2TagOp.ResetReadProtect
            {
                /**
                 * Constructor to initialize parameters of NxpResetReadProtect
                 * @param accessPassword
                 */
                public ResetReadProtect(int accessPassword)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2XL_SILICON.chipType;
                }
            }


            /**
             * NxpChangeEas
             */
            public static class ChangeEas extends NxpGen2TagOp.ChangeEas
            {                
                /**
                 * Constructor to initialize parameters of NxpChangeEas
                 * @param accessPassword The access password to be used
                 * @param reset Set or Reset the EAS bit
                 */
                public ChangeEas(int accessPassword, boolean reset)
                {
                    super(accessPassword, reset);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2XL_SILICON.chipType;
                }
            }

            /**
             * NxpEasAlarm
             */
            public static class EasAlarm extends NxpGen2TagOp.EasAlarm
            {                
                /**
                 * Constructor to initialize parameters of NxpEasAlarm
                 * @param divideRatio
                 * @param tagEncoding
                 * @param trExt
                 */
                public EasAlarm(Gen2.DivideRatio divideRatio, Gen2.TagEncoding tagEncoding, Gen2.TrExt trExt)
                {
                    this.divideRatio = divideRatio;
                    this.tagEncoding = tagEncoding;
                    this.trExt = trExt;
                    chipType = Gen2_SiliconType.GEN2_NXP_G2XL_SILICON.chipType;
                }
            }
            public static class Calibrate extends NxpGen2TagOp.Calibrate
            {
                /**
                 * Constructor to initialize parameters of NxpCalibrate
                 * @param accessPassword
                 */
                public Calibrate(int accessPassword)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2XL_SILICON.chipType;
                }
            }

        }//end of inner class G2X

        public static class G2I extends NxpGen2TagOp
        {
            G2I()
            {
                chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
            }
            /**
             * NxpSetReadProtect
             */
            public static class SetReadProtect extends NxpGen2TagOp.SetReadProtect
            {
                /**
                 * Constructor to initialize parameters of NxpSetReadProtect
                 * @param accessPassword
                 */
                public SetReadProtect(int accessPassword)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
                }
            }

             /**
             * NxpResetReadProtect
             */
            public static class ResetReadProtect extends NxpGen2TagOp.ResetReadProtect
            {
                /**
                 * Constructor to initialize parameters of NxpResetReadProtect
                 * @param accessPassword
                 */
                public ResetReadProtect(int accessPassword)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
                }
            }


            /**
             * NxpChangeEas
             */
            public static class ChangeEas extends NxpGen2TagOp.ChangeEas
            {                
                /**
                 * Constructor to initialize parameters of NxpChangeEas
                 * @param accessPassword The access password to be used
                 * @param reset Set or Reset the EAS bit
                 */
                public ChangeEas(int accessPassword, boolean reset)
                {
                    super(accessPassword, reset);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
                    this.reset = reset;
                }
            }

            /**
             * NxpEasAlarm
             */
            public static class EasAlarm extends NxpGen2TagOp.EasAlarm
            {                
                /**
                 * Constructor to initialize parameters of NxpEasAlarm
                 * @param divideRatio
                 * @param tagEncoding
                 * @param trExt
                 */
                public EasAlarm(Gen2.DivideRatio divideRatio, Gen2.TagEncoding tagEncoding, Gen2.TrExt trExt)
                {
                    this.divideRatio = divideRatio;
                    this.tagEncoding = tagEncoding;
                    this.trExt = trExt;
                    chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
                }
            }
            public static class Calibrate extends NxpGen2TagOp.Calibrate
            {
                /**
                 * Constructor to initialize parameters of NxpCalibrate
                 * @param accessPassword
                 */
                public Calibrate(int accessPassword)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
                }
            }
            // operations specific to G2I tags will come in this space like NXP Change Config
           
            /**
             * NXP Change Configuration
             */
            public static class ChangeConfig extends NxpGen2TagOp
            {

                /** configuration word */
                public int configWord;

                /**
                 * Constructor to initialize parameters of NxpConfigChange
                 * @param accessPassword
                 * @param configWord                 
                 */
                public ChangeConfig(int accessPassword, ConfigWord configWord)
                {
                    super(accessPassword);
                    chipType = Gen2_SiliconType.GEN2_NXP_G2IL_SILICON.chipType;
                    setConfigWord(configWord);
                }

                /**
                 * Form the config word
                 * @param inputData
                 */
                private void setConfigWord(ConfigWord inputData)
                {
                    /* format config data*/
                    configWord = 0x0000;
                    if (inputData.tamperAlarm)
                    {
                        configWord |= (0x0001 << 15);
                    }
                    if (inputData.externalSupply)
                    {
                        configWord |= (0x0001 << 14);
                    }
                    if (inputData.RFU2)
                    {
                        configWord |= (0x0001 << 13);
                    }
                    if (inputData.RFU3)
                    {
                        configWord |= (0x0001 << 12);
                    }
                    if (inputData.invertDigitalOutput)
                    {
                        configWord |= (0x0001 << 11);
                    }
                    if (inputData.transparentMode)
                    {
                        configWord |= (0x0001 << 10);
                    }
                    if (inputData.dataMode)
                    {
                        configWord |= (0x0001 << 9);
                    }
                    if (inputData.conditionalReadRangeReduction_onOff)
                    {
                        configWord |= (0x0001 << 8);
                    }
                    if (inputData.conditionalReadRangeReduction_openShort)
                    {
                        configWord |= (0x0001 << 7);
                    }
                    if (inputData.maxBackscatterStrength)
                    {
                        configWord |= (0x0001 << 6);
                    }
                    if (inputData.digitalOutput)
                    {
                        configWord |= (0x0001 << 5);
                    }
                    if (inputData.privacyMode)
                    {
                        configWord |= (0x0001 << 4);
                    }
                    if (inputData.readProtectUser)
                    {
                        configWord |= (0x0001 << 3);
                    }
                    if (inputData.readProtectEPC)
                    {
                        configWord |= (0x0001 << 2);
                    }
                    if (inputData.readProtectTID)
                    {
                        configWord |= (0x0001 << 1);
                    }
                    if (inputData.psfAlarm)
                    {
                        configWord |= 0x0001;
                    }
                }
            }

            public static class ConfigWord
            {
                // Down - MSB to LSB
                /** PSF alarm flag (Permanently stored in tag memory)*/
                public boolean psfAlarm;
                /** Read protect TID bank (permanently stored in tag memory)*/
                public boolean readProtectTID;
                /** Read protect EPC bank (Permanently stored in tag memory)*/
                public boolean readProtectEPC;
                /** Read protect User memory bank (permanently stored in tag memory)*/
                public boolean readProtectUser;
                /** Read range reduction on/off (permanently stored in tag memory)*/
                public boolean privacyMode;
                /** Digital output (permanently stored in tag memory)*/
                public boolean digitalOutput;
                /** Maximum backscatter strength (permanently stored in tag memory)*/
                public boolean maxBackscatterStrength;
                /** Conditional Read Range Reduction open/short (permanently stored in tag memory)*/
                public boolean conditionalReadRangeReduction_openShort;
                /** Conditional Read Range Reduction on/off (permanently stored in tag memory)*/
                public boolean conditionalReadRangeReduction_onOff;
                /** Transparent mode data/raw (reset at power up)*/
                public boolean dataMode;
                /** Transparent mode on/off (reset at power up)*/
                public boolean transparentMode;
                /** Invert digital output (reset at power up)*/
                public boolean invertDigitalOutput;
                /** RFU 3*/
                public boolean RFU3;
                /** RFU 2*/
                public boolean RFU2;
                /** External supply flag digital input (read only)*/
                public boolean externalSupply;
                /** Tamper alarm flag (Read only)*/
                public boolean tamperAlarm;

                public ConfigWord ConfigWord(int inputData)
                {
                    return getConfigWord(inputData);
                }

                /**
                 * Retrieve configuration word
                 * @param inputData
                 * @return cfgWord
                 */
                public ConfigWord getConfigWord(int inputData)
                {
                    ConfigWord cfgWord = new ConfigWord();
                    /* format config data*/
                    if ((inputData&0x8000)!=0)
                    {
                        cfgWord.tamperAlarm = true;
                    }
                    if ((inputData&0x4000)!=0)
                    {
                        cfgWord.externalSupply = true;
                    }
                    if ((inputData&0x2000)!=0)
                    {
                        cfgWord.RFU2 = true;
                    }
                    if ((inputData&0x1000)!=0)
                    {
                        cfgWord.RFU3 = true;
                    }
                    if ((inputData&0x0800)!=0)
                    {
                        cfgWord.invertDigitalOutput = true;
                    }
                    if ((inputData&0x0400)!=0)
                    {
                        cfgWord.transparentMode = true;
                    }
                    if ((inputData&0x0200)!=0)
                    {
                        cfgWord.dataMode = true;
                    }
                    if ((inputData&0x0100)!=0)
                    {
                        cfgWord.conditionalReadRangeReduction_onOff = true;
                    }
                    if ((inputData&0x0080)!=0)
                    {
                        cfgWord.conditionalReadRangeReduction_openShort = true;
                    }
                    if ((inputData&0x0040)!=0)
                    {
                        cfgWord.maxBackscatterStrength = true;
                    }
                    if ((inputData&0x0020)!=0)
                    {
                        cfgWord.digitalOutput = true;
                    }
                    if ((inputData&0x0010)!=0)
                    {
                        cfgWord.privacyMode = true;
                    }
                    if ((inputData&0x0008)!=0)
                    {
                        cfgWord.readProtectUser = true;
                    }
                    if ((inputData&0x0004)!=0)
                    {
                        cfgWord.readProtectEPC = true;
                    }
                    if ((inputData&0x0002)!=0)
                    {
                        cfgWord.readProtectTID = true;
                    }
                    if ((inputData&0x0001)!=0)
                    {
                        cfgWord.psfAlarm = true;
                    }
                    return cfgWord;
                }
           }
        }//end of G2I inner class
    }


    public static class Impinj extends Gen2CustomTagOp
    {
        public static class Monza4 extends Impinj
        {
            Monza4()
            {

            }
            /**
             *  Monza4QTReadWrite
             */
            public static class QTReadWrite extends Monza4
            {
                /** access password */
                public int accessPassword;

                /** control byte */
                public int controlByte;

                /** payload byte */
                public int payloadWord;

                /**
                 * Constructor to initialize the parameters QTPayload and QTControlByte
                 * @param accessPassword
                 * @param payload - comprises of qtSR - Bit 15 (MSB) is first transmitted bit of the
                 * payload field and qtMEM - Tag uses Public/Private Memory Map
                 * @param controlByte - comprises of qtReadWrite - The Read/Write field indicates whether the tag
                 * reads or writes QT control data and
                 * persistence - The Persistence field indicates whether the QT control is written to non-volatile memory
                 */
                public QTReadWrite(int accessPassword, QTPayload payload, QTControlByte controlByte)
                {
                    this.accessPassword = accessPassword;
                    setPayload(payload);
                    setControlByte(controlByte);
                }

                /**
                 * setting the payload
                 * @param payload
                 */
                private void setPayload(QTPayload payload)
                {
                    //payload
                    payloadWord = 0x0000;
                    if (payload.QTSR)
                    {
                        payloadWord |= 0x8000;
                    }
                    if (payload.QTMEM)
                    {
                        payloadWord |= 0x4000;
                    }
                    if(payload.RFU13)
                    {
                        payloadWord |= 0x2000;
                    }
                    if(payload.RFU12)
                    {
                        payloadWord |= 0x1000;
                    }
                    if(payload.RFU11)
                    {
                        payloadWord |= 0x0800;
                    }
                    if(payload.RFU10)
                    {
                        payloadWord |= 0x0400;
                    }
                    if(payload.RFU9)
                    {
                        payloadWord |= 0x0200;
                    }
                    if(payload.RFU8)
                    {
                        payloadWord |= 0x0100;
                    }
                    if(payload.RFU7)
                    {
                        payloadWord |= 0x0080;
                    }
                    if(payload.RFU6)
                    {
                        payloadWord |= 0x0040;
                    }
                    if(payload.RFU5)
                    {
                        payloadWord |= 0x0020;
                    }
                    if(payload.RFU4)
                    {
                        payloadWord |= 0x0010;
                    }
                    if(payload.RFU3)
                    {
                        payloadWord |= 0x0008;
                    }
                    if(payload.RFU2)
                    {
                        payloadWord |= 0x0004;
                    }
                    if(payload.RFU1)
                    {
                        payloadWord |= 0x0002;
                    }
                    if(payload.RFU0)
                    {
                        payloadWord |= 0x0001;
                    }
                }

                /**
                 * setting the control byte
                 * @param controlByte
                 */
                private void setControlByte(QTControlByte ctrlByte)
                {
                     //control byte
                    controlByte = 0x00;
                    if (ctrlByte.QTReadWrite)
                    {
                        controlByte |= 0x80;
                    }
                    if (ctrlByte.Persistence)
                    {
                        controlByte |= 0x40;
                    }
                }
            }

            /**
             * Monza4 QT Control Byte
             */
            public static class QTControlByte
            {
                /**
                 *  The following bits are Reserved for Future Use. And will be ignored.
                 *  RFU_TM's are ThingMagic specific RFU bits
                 */
                public boolean RFU_TM0;
                public boolean RFU_TM1;
                public boolean RFU_TM2;
                public boolean RFU_TM3;
                /**
                 * RFU_Impinj bits are as per the Monza4 specification.
                 */
                public boolean RFU_Impinj4;
                public boolean RFU_Impinj5;
                /**
                 * The Read/Write field indicates whether the tag reads or writes QT control data.
                 * Read/Write=0 means read the QT control bits in cache.
                 * Read/Write=1 means write the QT control bits
                 */
                public boolean QTReadWrite;
                /**
                 * If Read/Write=1, the Persistence field indicates whether the QT control is
                 * written to nonvolatile (NVM) or volatile memory.
                 * Persistence=0 means write to volatile memory. Persistence=1 means write to NVM memory
                 */
                public boolean Persistence;
            }//end of QTControlByte class

            /**
             * Monza QT Payload
             */
            public static class QTPayload
            {

                /**
                 *  Bit 15 (MSB) is first transmitted bit of the payload field.
                 *  Bit # Name Description
                 *  1: Tag reduces range if in or about to be in OPEN or SECURED state
                 *  0: Tag does not reduce range
                 */
                public boolean QTSR;

                /**
                 *  Bit 14 (MSB) is first transmitted bit of the payload field.
                 *  1:Tag uses Public Memory Map (see Table 2-10)
                 *  0: Tag uses Private Memory Map (see Table 2-9)
                 */
                public boolean QTMEM;

                /**
                 * 14 RFU Bits - Reserved for future use. Tag will return these bits as zero.
                 */
                public boolean RFU13;
                public boolean RFU12;
                public boolean RFU11;
                public boolean RFU10;
                public boolean RFU9;
                public boolean RFU8;
                public boolean RFU7;
                public boolean RFU6;
                public boolean RFU5;
                public boolean RFU4;
                public boolean RFU3;
                public boolean RFU2;
                public boolean RFU1;
                public boolean RFU0;
            }//end of QTPayload class
        }//end of Monza4 inner class
    }//end of Impinj inner class

    //IAV Denatran custom tagOp
    public static class Denatran extends Gen2CustomTagOp
    {
        public static class IAV extends Denatran
        {
            byte commandCode;
            byte payload = 0;
            byte userPointer = 0;
            byte writtenWord = 0;
            byte[] writeCredentials;
            byte[] tagId;
            byte[] token;
            byte[] dataWords;

            public IAV(byte commandCode)
            {
                chipType = Gen2_SiliconType.GEN2_IAV_DENATRAN.chipType;
                this.commandCode = commandCode;
            }


            //enum IAV secure tag operation
            private static enum SecureTagOpType
            {
                GEN2_ACTIVATE_SECURE_MODE((byte)0),
                GEN2_AUTHENTICATE_OBU((byte)1),
                GEN2_ACTIVATE_SINIAV_MODE((byte)2),
                GEN2_OBU_AUTH_ID((byte)3),
                GEN2_AUTHENTICATE_OBU_FULL_PASS1((byte)4),
                GEN2_AUTHENTICATE_OBU_FULL_PASS2((byte)5),
                GEN2_OBU_READ_FROM_MEM_MAP((byte)6),
                GEN2_OBU_WRITE_TO_MEM_MAP((byte)7),
                GEN2_AUTHENTICATE_OBU_FULL_PASS((byte)8),
                GEN2_GET_TOKEN_ID((byte)9),
                GEN2_IAV_READ_SEC((byte)10),
                GEN2_IAV_WRITE_SEC((byte)11);

                byte value;
                private SecureTagOpType(byte value)
                {
                    this.value = value;
                }
            }

            //IAV Activate Secure Mode
            public static class ActivateSecureMode extends IAV
            {
                public ActivateSecureMode()
                {
                    this((byte)0);
                }
                public ActivateSecureMode(byte payload)
                {
                    super(SecureTagOpType.GEN2_ACTIVATE_SECURE_MODE.value);
                    this.payload = payload;
                }
            }

            //IAV Authenticata OBU
            public static class AuthenticateOBU extends IAV
            {
                public AuthenticateOBU()
                {
                    this((byte)0);
                }
                public AuthenticateOBU(byte payload)
                {
                    super(SecureTagOpType.GEN2_AUTHENTICATE_OBU.value);
                    this.payload = payload;
                }
            }

            //IAV Activate Siniav Mode
            public static class ActivateSiniavMode extends IAV
            {
                public ActivateSiniavMode(byte payload,byte[] token)
                {
                    super(SecureTagOpType.GEN2_ACTIVATE_SINIAV_MODE.value);
                    this.payload = payload;
                    this.token = token;
                }
            }

            //IAV OBU AuthID
            public static class OBUAuthID extends IAV
            {
                public OBUAuthID(byte payload)
                {
                    super(SecureTagOpType.GEN2_OBU_AUTH_ID.value);
                    this.payload = payload;
                }
            }

            //IAV OBU AuthFullPass
            public static class OBUAuthFullPass extends IAV
            {
                public OBUAuthFullPass(byte payload)
                {
                    super(SecureTagOpType.GEN2_AUTHENTICATE_OBU_FULL_PASS.value);
                    this.payload = payload;
                }
            }

            //IAV OBU AuthFullPass1
            public static class OBUAuthFullPass1 extends IAV
            {
                public OBUAuthFullPass1(byte payload)
                {
                    super(SecureTagOpType.GEN2_AUTHENTICATE_OBU_FULL_PASS1.value);
                    this.payload = payload;
                }
            }

            //IAV OBU AuthFullPass2
            public static class OBUAuthFullPass2 extends IAV
            {
                public OBUAuthFullPass2(byte payload)
                {
                    super(SecureTagOpType.GEN2_AUTHENTICATE_OBU_FULL_PASS2.value);
                    this.payload = payload;
                }
            }

            //IAV OBU ReadFromMemMap
            public static class OBUReadFromMemMap extends IAV
            {
                public OBUReadFromMemMap(byte payload,byte userPointer)
                {
                    super(SecureTagOpType.GEN2_OBU_READ_FROM_MEM_MAP.value);
                    this.payload = payload;
                    this.userPointer = userPointer;
                }
            }

            //IAV OBU WritetoMemMap
            public static class OBUWriteToMemMap extends IAV
            {
                public OBUWriteToMemMap(byte payload,byte userPointer,byte writtenWord,byte[] tagId,byte[] writeCredentials)
                {
                    super(SecureTagOpType.GEN2_OBU_WRITE_TO_MEM_MAP.value);
                    this.payload = payload;
                    this.userPointer = userPointer;
                    this.writtenWord = writtenWord;
                    this.writeCredentials=writeCredentials;
                    this.tagId=tagId;
                }
            }

            //IAV OBU GetTokenId
            public static class GetTokenId extends IAV
            {
                public GetTokenId()
                {
                    super(SecureTagOpType.GEN2_GET_TOKEN_ID.value);
                }
            }

             //IAV ReadSec
            public static class ReadSec extends IAV
            {
                public ReadSec(byte payload,byte writtenWord)
                {
                    super(SecureTagOpType.GEN2_IAV_READ_SEC.value);
                    this.payload = payload;
                    this.writtenWord = writtenWord;
                }
            }

             //IAV WriteSec
            public static class WriteSec extends IAV
            {
                public WriteSec(byte payload,byte[] dataWords,byte[] writeCredentials)
                {
                    super(SecureTagOpType.GEN2_IAV_WRITE_SEC.value);
                    this.payload = payload;
                    this.writeCredentials=writeCredentials;
                    this.dataWords=dataWords;
                }
            }
        }
    }
}