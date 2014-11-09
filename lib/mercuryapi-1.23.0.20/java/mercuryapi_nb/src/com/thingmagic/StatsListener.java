/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.thingmagic;

/**
 * The listener interface for receiving tag read events. The class
 * that is interested in processing a tag read event implements this
 * interface, and the object created with that class is registered
 * with Reader.addStatsListener(). When the tag read occurs, the
 * object's tagRead method is invoked.
 */
public interface StatsListener
{
    /**
     * This method is invoked while stats response is embedded in response while streaming
     * @param r - Reader instance
     * @param statsReport - StatusReport instance
     */
    void statsRead(SerialReader.ReaderStats readerStats);
}
