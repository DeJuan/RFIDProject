/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.thingmagic;

/**
 *
 * @author qvantel
 */
public class FixedReaderFirmwareLoadOptions extends FirmwareLoadOptions{

private boolean eraseContents;
    private boolean revertDefaultSettings;

    public FixedReaderFirmwareLoadOptions()
    {
        // default constructor
    }

    public FixedReaderFirmwareLoadOptions(boolean eraseFirmware)
    {
        this.eraseContents = eraseFirmware;
    }

    public FixedReaderFirmwareLoadOptions(boolean eraseFirmware, boolean revertSettings)
    {
        this.eraseContents = eraseFirmware;
        this.revertDefaultSettings = revertSettings;
    }

    public void setEraseFirmware(boolean eraseFirmware)
    {
        this.eraseContents = eraseFirmware;
    }

    public boolean getEraseFirmware()
    {
        return eraseContents;
    }

    public void setRevertDefaultSettings(boolean revertSettings)
    {
        this.revertDefaultSettings = revertSettings;
    }

    public boolean getRevertDefaultSettings()
    {
        return revertDefaultSettings;
    }

    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();

        sb.append("FixedReaderFirmwareLoadOptions:[");
        sb.append(String.format("%s ", "eraseContents: " + eraseContents));
        sb.append(String.format("%s ", "revertDefaultSettings: " + revertDefaultSettings));
        sb.append("]");

        return sb.toString();
    }
}
