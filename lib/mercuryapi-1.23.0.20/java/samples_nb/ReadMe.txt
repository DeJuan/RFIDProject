// This file contains description for each and every java codelet which will help in understanding and running the codelets.


read.java - A sample program that reads tags for a fixed amount of time and prints the tags found.

readasync.java - A sample program that reads tags in background for a fixed amount of time and prints the tags found.

readasyncfilter.java - A sample program that reads tags in background for a fixed amount of time and prints the tags found that match the filter criteria.

readasynctrack.java - Sample program that reads tags in the background and track tags that have been seen; only print the tags that have not been seen before.

SavedConfig.java - This works only for Serial Readers. User configuration can be saved and restored using this program.

serialtime.java - Sample program that reads tags for a fixed period of time (500ms) and prints the tags found, while logging the serial message with timestamps.

writetag.java - Sample program to write EPC of a tag which is first found in the antenna range.

MultiProtocol.java - A sample program to read tags of different protocols like Gen2, ISO18K, IPX256, IPX64 and prints the tags found.

multireadasync.java - Sampled program to read tags from 2 different readers at the same time in background and print the tags found.
                      This codelet expects 2 input arguments of 2 readers.
                      Usage: java samples.multireadasync <reader1-uri> <reader2-uri>
                             ex: java samples.multireadasync tmr://10.0.23.24 tmr://10.0.25.26

BlockPermaLock.java - Functionality will not work for M6 Readers

BlockWrite.java - Functionality will not work for M6 Readers

demo.java - An interactive program which would demo all the features the API exposes to the external world.

demo(.bat/.sh) - To run demo.java from command line ( windows and unix variants)

sample(.bat/.sh) - To run codelets or sample applications from command line with valid input params.
                   Usage: java samples.%class% %params%


filter.java - Sample program that demonstrates the usage of filter and different types of filters.

locktag.java - Sample program that sets an access password on a tag and locks its EPC. This program has dummy accesspassword hard-coded within.

LicenseKey.java - Sample program to set the license key to a reader. This program has a dummy license key hard-coded within.

Note:
All the codelets expect one input argument i.e, Reader URI (except multireadasync codelet which expects two input arguments)

