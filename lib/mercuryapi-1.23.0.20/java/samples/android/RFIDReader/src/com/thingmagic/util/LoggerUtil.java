package com.thingmagic.util;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.Date;

import android.os.Environment;
import android.util.Log;

public class LoggerUtil {

	public static final String APP_ID = "RFIDReaderAPP";
    private static String logDir = "/Android";
    private static String logFileName = "/RFID-Debug.txt";
    private static boolean writeLogsToFile = false;
    private static final int LOG_LEVEL_VERBOSE = 4;
    private static final int LOG_LEVEL_DEBUG = 3;
    private static final int LOG_LEVEL_INFO = 2;
    private static final int LOG_LEVEL_ERROR = 1;
    private static final int LOG_LEVEL_OFF = 0;
    private static final int CURRENT_LOG_LEVEL = LOG_LEVEL_DEBUG;


    public static void log(String tag,String message, int logLevel) {
        if (logLevel <= CURRENT_LOG_LEVEL) 
        	if(logLevel != LOG_LEVEL_ERROR){
        		 Log.d(APP_ID, message);
        	}
            if (writeLogsToFile) {
                writeToFile(message);
            }
    }

    public static void writeToFile(String message) {
        try {
            File sdCard = Environment.getExternalStorageDirectory();
            File dir = new File(sdCard.getAbsolutePath() + logDir);
            dir.mkdirs();
            File file = new File(dir, logFileName);
            PrintWriter writer = new PrintWriter(new BufferedWriter(new FileWriter(file, true), 8 * 1024));
            writer.println(APP_ID + " " + new Date().toString() + " : " + message);
            writer.flush();
            writer.close();
        } catch (Exception e) {
        	Log.e(APP_ID, "Exception in logging  :", e);
        }
    }

    public static void verbose(String tag, String message) {
        log(tag, message, LOG_LEVEL_VERBOSE);
    }

    public static void debug(String tag,String message) {
        log(tag, message, LOG_LEVEL_DEBUG);
    }

    public static void error(String tag,String message,Exception ex) {
    	Log.e(tag, message, ex);
        log(tag,message, LOG_LEVEL_ERROR);
    }
    
    public static void error(String tag,String message) {
        log(tag,message, LOG_LEVEL_ERROR);
        Log.e(tag, message);
    }

    public static void info(String tag,String message) {
        log(tag, message, LOG_LEVEL_INFO);
    }
}