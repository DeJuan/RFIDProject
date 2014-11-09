/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.thingmagic;

/**
 * A StopTriggerReadPlan is a read plan covering one or more protocol and
 * one or more antennas
 * 
 */
public class StopTriggerReadPlan extends SimpleReadPlan
{
      
    public StopOnTagCount stopOnCount;

    public StopTriggerReadPlan(StopOnTagCount sotc, int antennas[], TagProtocol protocol)
    {
        super(antennas, protocol);
        this.stopOnCount = sotc;
    }

    public StopTriggerReadPlan(StopOnTagCount sotc, int antennas[], TagProtocol protocol, TagFilter filter, TagOp op, int weight)
    {
        super(antennas, protocol, filter,op, weight);
        this.stopOnCount = sotc;
    }

    public StopTriggerReadPlan(StopOnTagCount sotc, int antennas[], TagProtocol protocol,boolean useFastSearch)
    {
        super(antennas, protocol, useFastSearch);
        this.stopOnCount = sotc;
    }
    
}
