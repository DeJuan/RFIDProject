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
 * A SimpleReadPlan is a read plan covering one or more protocols and
 * one or more antennas, with all protocols searched on all antennas.
 */
public class SimpleReadPlan extends ReadPlan
{
  public int antennas[]; // null implies "use detected antennas"
  public TagProtocol protocol;
  public TagFilter filter;
  public boolean useFastSearch;

  static final TagProtocol defaultProtocol = TagProtocol.GEN2;
  static final int[] noAntennas = new int[0];

  /** Tag Operation */
  public TagOp Op = null;

  /**
   * Create a SimpleReadPlan with the default antenna setting
   * (detected), the default protocol, no filter, and the default
   * weight.
   */ 
  public SimpleReadPlan()
  {
    antennas = noAntennas;
    protocol = defaultProtocol;
    filter = null;
  }

  /**
   * Create a SimpleReadPlan with a list of antennas, a protocol, no
   * filter, and the default weight.
   *
   * @param antennaList the antennas to include in the read plan
   * @param protocol the protocol to include in the read plan
   */
  public SimpleReadPlan(int[] antennaList, TagProtocol protocol)
  {
    antennas = (antennaList == null) ? noAntennas : antennaList.clone();
    this.protocol = protocol;
    filter = null;
  }

  /**
   * Create a SimpleReadPlan with a protocol, a list of
   * antennas,and a useFastSearch.
   *
   * @param antennaList the antennas to include in the read plan
   * @param protocol the protocol to include in the read plan
   * @param useFastSearch that enables fast moving tag read functionality
   */
  public SimpleReadPlan(int[] antennaList, TagProtocol protocol, boolean useFastSearch)
  {
    this(antennaList, protocol);
    this.useFastSearch = useFastSearch;
  }

  /**
   * Create a SimpleReadPlan with a protocol, a list of
   * antennas, a filter, and a weight.
   *
   * @param antennaList the antennas to include in the read plan
   * @param protocol the protocol to include in the read plan
   * @param filter the filter to include in the read plan
   * @param weight the weight of this read plan relative to others, when
   * included in another plan
   */
  public SimpleReadPlan(int[] antennaList, TagProtocol protocol,
                        TagFilter filter, int weight)
  {
    super(weight);
    antennas = (antennaList == null) ? noAntennas : antennaList.clone();
    this.protocol = protocol;
    this.filter = filter;
  }

   /**
   * Create a SimpleReadPlan with a protocol, a list of
   * antennas, a filter, a weight, and a useFastSearch.
   *
   * @param antennaList the antennas to include in the read plan
   * @param protocol the protocol to include in the read plan
   * @param filter the filter to include in the read plan
   * @param weight the weight of this read plan relative to others, when
   * included in another plan
   * @param useFastSearch that enables fast moving tag read functionality
   */
  public SimpleReadPlan(int[] antennaList, TagProtocol protocol,
                        TagFilter filter, int weight, boolean useFastSearch)
  {
    this(antennaList, protocol, filter, weight);
    this.useFastSearch = useFastSearch;
  }


   /**
   * Create a SimpleReadPlan with a list of protocols, a list of
   * antennas, a filter,a tagop and a weight.
   *
   * @param antennaList the antennas to include in the read plan
   * @param protocol the protocol to include in the read plan
   * @param filter the filter to include in the read plan
   * @param op operation mode
   * @param weight the weight of this read plan relative to others, when
   * included in another plan
   */
  public SimpleReadPlan(int[] antennaList, TagProtocol protocol,
                        TagFilter filter,TagOp op, int weight)
  {
    super(weight);
    antennas = (antennaList == null) ? noAntennas : antennaList.clone();
    this.protocol = protocol;
    this.filter = filter;
    this.Op=op;
  }

  /**
   * Create a SimpleReadPlan with a list of protocols, a list of
   * antennas, a filter,a tagop, a weight, and a useFastSearch.
   *
   * @param antennaList the antennas to include in the read plan
   * @param protocol the protocol to include in the read plan
   * @param filter the filter to include in the read plan
   * @param op operation mode
   * @param weight the weight of this read plan relative to others, when
   * included in another plan
   * @param useFastSearch that enables fast moving tag read functionality
   */
  public SimpleReadPlan(int[] antennaList, TagProtocol protocol,
                        TagFilter filter,TagOp op, int weight, boolean useFastSearch)
  {
    this(antennaList, protocol, filter, op, weight);
    this.useFastSearch = useFastSearch;
  }

  public String toString()
  {
    StringBuilder sb = new StringBuilder();

    sb.append("SimpleReadPlan:[");
    sb.append(String.format("%s ",protocol.toString()));
    if (antennas.length > 0)
    {
      for (int a : antennas)
      {
        sb.append(String.format(" %d",a));
      }
    }
    else
    {
      sb.append(" auto");
    }
    if (filter != null)
    {
      sb.append(filter.toString());
    }
    sb.append(String.format(", %d", weight));
    sb.append("]");

    return sb.toString();
  }
}
