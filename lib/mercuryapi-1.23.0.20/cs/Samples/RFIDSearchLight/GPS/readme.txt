Code Sample Name: Microsoft.WindowsMobile.Samples.Location

Feature Area: GPS and Managed Programming

Description: 
	This sample shows how to wrap the native GPS Api in a simple
 	to use C# class.  

	Project ManagedGps contains the following classes:
	Gps: The interface to the managed gps api.  use this class to 
	  open, close, query the device state, and query the position data from your gps hardware.
	
	GpsPosition: Contains the GPS position data received from the gps hardware
	
	GpsDevice: Contains GPS device state data received from the gps hardware
	
	Utils: Native memory allocation utilities

Usage: 
	Make sure your GPS hardware is setup correctly using the 
	GPS control panel in Settings. 

	You may also use the FakeGPS.cab in the Tools\GPS directory where you installed the SDK
	to use simulated GPS data. For more information see FakeGPSReadme.txt in the same directory.

	Once you have your GPS set up, you need to Build and Run the GpsSample
	to see it work.  

Relevant APIs/Associated Help Topics: 
    GPSApi, 
    Managed PInvokes, 
    Managed and Native Data Marshalling 
    

Assumptions: GPS hardware is setup on the device.

Requirements: 
    Visual Studio 2005, 
    Windows Mobile 6 Professional SDK,
    Activesync 4.5.

** For more information about this code sample, please see the Windows Mobile SDK help system. **
 
