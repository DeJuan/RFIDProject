package com.thingmagic.util;

import android.app.Activity;
import android.content.Intent;

public class GlobalExceptionHandler implements Thread.UncaughtExceptionHandler {

	Activity mActivity;

	public GlobalExceptionHandler(Activity activity) {
		mActivity = activity;
		Thread.getDefaultUncaughtExceptionHandler();
	}

	@Override
	public void uncaughtException(Thread t, Throwable e) {

		StackTraceElement[] arr = e.getStackTrace();
		String report = e.toString() + "\n\n";
		report += "--------- Stack trace ---------\n\n";

		for (int i = 0; i < arr.length; i++) {
			report += "    " + arr[i].toString() + "\n";
		}
		report += "-------------------------------\n\n";

		// If the exception was thrown in a background thread inside
		// AsyncTask, then the actual exception can be found with getCause
		report += "--------- Cause ---------\n\n";
		Throwable cause = e.getCause();

		if (cause != null) {
			report += cause.toString() + "\n\n";
			arr = cause.getStackTrace();
			for (int i = 0; i < arr.length; i++)

			{
				report += "    " + arr[i].toString() + "\n";
			}
		}
		LoggerUtil.error("GlobalException", report.toString());
		
		Intent i1 = new Intent(mActivity, Error.class);
		i1.putExtra("Error", report.toString());
		i1.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		i1.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);

		mActivity.startActivity(i1);
		 System.exit(0);// If you want to restart activity and want to kill
		// after crash.s

	}

}
