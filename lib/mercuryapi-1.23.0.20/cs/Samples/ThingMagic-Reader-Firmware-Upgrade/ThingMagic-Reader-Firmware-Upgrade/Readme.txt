This utility helps in upgrading the embedded reader to a
new firmware that is released and certified only by 
ThingMagic and is built for the .net framework. 

To run, double click on the ThingMagic-Reader-Firmware-Upgrade.exe
file, or on the command line, run ThingMagic-Reader-Firmware-Upgrade.exe
after navigating to the containing folder.

The sources for this program can be found in the Mercury
API package under cs/samples/ThingMagic-Reader-Firmware-Upgrade.
The application was built with Microsoft Visual C# 2008 
Express Edition. 

Once the program is up and running, enter the reader COM port
(Comx or comx). Click on "Choose Firmware" to navigate
to the location of the .sim firmware file. Select a file
and click OK. After verifying the correct file path and file
name click on "Upgrade!" to start upgrading the firmware.

The Progress bar will start to fill up giving an estimate of 
the time remaining in the upgrade process. Once the upgrade
is successful the progress bar will completely fill up and the
status will be printed below it.

Depending on the version of this utility, the firmware information
can be found either on the front panel of the software or
under Help->About->ThingMagic Reader. 

File->Exit will guarantee safe disconnect from the reader.

For more information, visit http://www.thingmagic.com/, or contact 
support@thingmagic.com