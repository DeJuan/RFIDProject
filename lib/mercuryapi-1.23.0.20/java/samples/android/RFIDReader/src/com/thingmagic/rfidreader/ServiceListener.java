package com.thingmagic.rfidreader;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;


import android.content.Context;
import android.os.AsyncTask;
import android.text.Html;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import com.thingmagic.AndroidUsbReflection;
import com.thingmagic.ReadExceptionListener;
import com.thingmagic.ReadListener;
import com.thingmagic.Reader;
import com.thingmagic.ReaderException;
import com.thingmagic.SimpleReadPlan;
import com.thingmagic.TagProtocol;
import com.thingmagic.TagReadData;
import com.thingmagic.util.LoggerUtil;

public class ServiceListener implements View.OnClickListener{

	private static String  TAG = "ServiceListener";
	private static EditText ntReaderField;
	private static Spinner serialList=null;
	private static RadioButton serialReaderRadioButton = null;
	private static RadioButton networkReaderRadioButton = null;
	static LinearLayout servicelayout;
	private static RadioGroup readerRadioGroup = null;
	private static RadioButton syncReadRadioButton = null;
	private static RadioButton asyncReadSearchRadioButton = null;
	private static Button readButton = null;
	private static Button connectButton = null;
	public static TextView searchResultCount = null;
	public static TextView totalTagCountView = null;
//	public static TextView readRateView = null;
	private static ProgressBar progressBar = null;
	private static int redColor = 0xffff0000;
	private static int textColor = 0xff000000;
	private static ReadThread readThread;
	private static TableLayout table ;
	private static LayoutInflater inflater;
	private static ArrayList<String> addedEPCRecords= new ArrayList<String>();;
	private static  ConcurrentHashMap<String,TagRecord> epcToReadDataMap=new ConcurrentHashMap<String, TagRecord>();
	private static int uniqueRecordCount=0;
	private static int totalTagCount=0;
	private static long readRatePerSec=0;
	private static long queryStartTime = 0;
	private static long queryStopTime = 0;
	static String strDateFormat = "M/d/yyyy h:m:s.SSS a";
	static SimpleDateFormat sdf = new SimpleDateFormat(strDateFormat);
	  String format="";
	
	static ReaderActivity readerActivity;
	public ServiceListener(ReaderActivity readerActivity) {
		ServiceListener.readerActivity = readerActivity;
		findAllViewsById();
	}

	private void findAllViewsById() 
	{
		syncReadRadioButton = (RadioButton) readerActivity.findViewById(R.id.SyncRead_radio_button);
		asyncReadSearchRadioButton = (RadioButton) readerActivity.findViewById(R.id.AsyncRead_radio_button);
		readButton = (Button) readerActivity.findViewById(R.id.Read_button);
		connectButton = (Button) readerActivity.findViewById(R.id.Connect_button);
		searchResultCount = (TextView) readerActivity.findViewById(R.id.search_result_view);
		totalTagCountView = (TextView) readerActivity.findViewById(R.id.totalTagCount_view);
//		readRateView = (TextView) readerActivity.findViewById(R.id.readRate_view);
		progressBar = (ProgressBar) readerActivity.findViewById(R.id.progressbar);
		textColor = searchResultCount.getTextColors().getDefaultColor();
		table = (TableLayout) readerActivity.findViewById(R.id.tablelayout);
		inflater = (LayoutInflater) readerActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		ntReaderField = (EditText) readerActivity.findViewById(R.id.search_edit_text);
		serialList = (Spinner) readerActivity.findViewById(R.id.SerialList);
		serialReaderRadioButton = (RadioButton) readerActivity.findViewById(R.id.SerialReader_radio_button);
		networkReaderRadioButton = (RadioButton) readerActivity.findViewById(R.id.NetworkReader_radio_button);
		servicelayout = (LinearLayout) readerActivity.findViewById(R.id.ServiceLayout);
		readerRadioGroup = (RadioGroup) readerActivity.findViewById(R.id.Reader_radio_group);
	}
	
	@Override
	public void onClick(View arg0) {
		try{
		    String readerModel=(String) readerActivity.reader.paramGet("/reader/version/model");
			if(readerModel.equalsIgnoreCase("M6e Micro")){
				SimpleReadPlan  simplePlan = new SimpleReadPlan(new int[] {1,2}, TagProtocol.GEN2);
				readerActivity.reader.paramSet("/reader/read/plan", simplePlan);					
			}		
		String operation = "";
		if (syncReadRadioButton.isChecked()) 
		{
			operation = "syncRead";
			readButton.setText("Reading");
			readButton.setClickable(false);
		}
		else if (asyncReadSearchRadioButton.isChecked()) 
		{
			operation = "asyncRead";
		}
		if (readButton.getText().equals("Stop Reading")) {
			readThread.setReading(false);
			readButton.setText("Stopping....");
			readButton.setClickable(false);
		} else {
			if (readButton.getText().equals("Start Reading")) {
				readButton.setText("Stop Reading");
				
			}
			clearTagRecords();
			readThread = new ReadThread(readerActivity.reader, operation );
			readThread.execute();
		}
		}catch(Exception ex){
			LoggerUtil.error(TAG, "Exception", ex);
		}
	}

	public OnClickListener clearListener = new OnClickListener() {
		
		@Override
		public void onClick(View v) 
		{
			clearTagRecords();
		}
	};
	public static void clearTagRecords(){
		addedEPCRecords.clear();
		epcToReadDataMap.clear();
		table.removeAllViews();	
		searchResultCount.setText("");
		totalTagCountView.setText("");
//		readRateView.setText("");
		uniqueRecordCount = 0;
		totalTagCount = 0;
		readRatePerSec=0;
		queryStartTime = System.currentTimeMillis();
	}
	public static class ReadThread   extends 
    AsyncTask<Void, Integer, ConcurrentHashMap<String,TagRecord>> 
     {

		private String operation;
		private Long duration;
		private static boolean exceptionOccur = false;
		private static String exception = "";
		private static boolean reading = true;
		private static Reader reader;
		private static TableRow fullRow =null ;
		private static TextView nr =null ;
		private static TextView epcValue=null ;
		private static TextView count =null ;
		static ReadExceptionListener exceptionListener = new TagReadExceptionReceiver();
		static ReadListener readListener = new PrintListener();

	
		public ReadThread(Reader reader,
				String operation) 
		{
			this.operation = operation;
			this.reader=reader;
		}
	
		@Override
		protected void onPreExecute() 
		{
			clearTagRecords();
			syncReadRadioButton.setClickable(false);
			asyncReadSearchRadioButton.setClickable(false);
			connectButton.setClickable(false);
			searchResultCount.setTextColor(textColor);
			searchResultCount
					.setText("Reading Tags....");
			progressBar.setVisibility(View.VISIBLE);
			addedEPCRecords = new ArrayList<String>();
			epcToReadDataMap=new ConcurrentHashMap<String, TagRecord>();
			exceptionOccur=false;	

		}
	
		@Override
		protected ConcurrentHashMap<String,TagRecord> doInBackground(Void... params) 
		{
			Long startTime = System.currentTimeMillis();
			try 
			{
				if (operation.equalsIgnoreCase("syncRead")) {
					queryStartTime = System.currentTimeMillis();
					TagReadData[] tagReads = reader.read(1000);
					queryStopTime = System.currentTimeMillis();
					for (TagReadData tr : tagReads) 
					{
						parseTag(tr,false);
				    }
					publishProgress(0);
				}
				else
				{
					setReading(true);
					reader.addReadExceptionListener(exceptionListener);
					reader.addReadListener(readListener);
					reader.startReading();
					queryStartTime = System.currentTimeMillis();
					refreshReadRate();					
					while (isReading()) 
					{
						/* Waiting till stop reading button is pressed */
						Thread.sleep(100);					
					}
					queryStopTime = System.currentTimeMillis();
					System.out.println("Stop reading @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"+isReading());
					reader.stopReading();
					System.out.println("Stop reading------");
					reader.removeReadListener(readListener);
					reader.removeReadExceptionListener(exceptionListener);
					System.out.println("removed listeners------");
				}
			}
			catch (Exception ex) 
			{
				exception = ex.getMessage();
				exceptionOccur = true;
				LoggerUtil.error(TAG, "Exception while reading :", ex);
			} finally {			
				Long endTime = System.currentTimeMillis();
				duration = (endTime - startTime) / 1000;
			}
	
			return epcToReadDataMap;
		}
		
		static class PrintListener implements ReadListener
		{	  
		    public void tagRead(Reader r, final TagReadData tr)
		    {
		    	readThread.parseTag(tr, true);
		    }
		}
		
		static class TagReadExceptionReceiver implements ReadExceptionListener 
		{
	        public void tagReadException(Reader r, ReaderException re) 
	        {
	        	if(re.getMessage().equalsIgnoreCase("No connected antennas found")
	        			|| re.getMessage().contains("The module has detected high return loss")
	        			 ||	re.getMessage().contains("Tag ID buffer full")
	        			 || re.getMessage().contains("No tags found"))
	        	{
	        		//exception = "No connected antennas found";
	        		/* Continue reading */
	        	}else{
		        	exception=re.getMessage();
		        	exceptionOccur = true;
		        	readThread.setReading(false);
		        	readThread.publishProgress(-1);
	        	}
	        }
	    }
		
		private  void refreshReadRate(){
			new Thread(new Runnable() {
				
				@Override
				public void run() {
					while (isReading()) 
					{
						try {
							/* Refresh tagDetails for every 900 milli sec */ 
							Thread.sleep(900);	
							publishProgress(0);
							
//							/* calculate readRate for each 1000 mill sec*/  
//							Thread.sleep(99);
//							calculateReadrate();			            	
//							publishProgress(1);
						} catch (InterruptedException e) {
							LoggerUtil.error(TAG,"Exception ", e);
						}
					}					
				}
			}).start();
		}	
		private static void calculateReadrate()
		{
			long elapsedTime = (System.currentTimeMillis() - queryStartTime) ;
			if(!isReading()){
				elapsedTime = queryStopTime- queryStartTime;
			}
			 
			long tagReadTime = elapsedTime/ 1000;
			if(tagReadTime == 0)
			{
				readRatePerSec = (long) ((totalTagCount) / ((double) elapsedTime / 1000));
			}
			else
			{
				readRatePerSec = (long) ((totalTagCount) / (tagReadTime));
			}				
		}
		
		
		private void parseTag(TagReadData tr, boolean publishResult)
		{   
			totalTagCount +=tr.getReadCount();
			String epcString = tr.getTag().epcString();
			if(epcToReadDataMap.keySet().contains(epcString)){
				TagRecord tempTR=epcToReadDataMap.get(epcString);
	    		tempTR.readCount += tr.getReadCount();
	    	}else{
	    		TagRecord tagRecord=new TagRecord();
	    		tagRecord.setEpcString(epcString);
	    		tagRecord.setReadCount(tr.getReadCount());
	    		epcToReadDataMap.put(epcString, tagRecord);
	    	}	
			if(publishResult){
				publishProgress(0);
			}
		}
			
		@Override
		protected void onProgressUpdate(Integer... progress) {
			int progressToken= progress[0];
			if(progressToken == -1){
				searchResultCount.setTextColor(redColor);
				searchResultCount.setText("ERROR :" + exception);
				totalTagCountView.setText("");
			}else{
				populateSearchResult(epcToReadDataMap);
				if(!exceptionOccur && totalTagCount > 0){
					searchResultCount.setTextColor(textColor);
					searchResultCount.setText(Html.fromHtml("<b>Unique Tags :</b> "+epcToReadDataMap.keySet().size()));
					totalTagCountView.setText(Html.fromHtml("<b>Total Tags  :</b> "+totalTagCount));
	//				readRateView.setText(Html.fromHtml("<b>R.R/Sec :</b> "+readRatePerSec));	
				}
			}
		}
	
		@Override
		protected void onPostExecute(ConcurrentHashMap<String,TagRecord> epcToReadDataMap) 
		{
			if(exceptionOccur)
			{
				searchResultCount.setTextColor(redColor);
				searchResultCount.setText("ERROR :" + exception);
				totalTagCountView.setText("");
//				readRateView.setText("");
			}
			else
			{				
				calculateReadrate();
				searchResultCount.setText(Html.fromHtml("<b>Unique Tags :</b> "+epcToReadDataMap.keySet().size()));
				totalTagCountView.setText(Html.fromHtml("<b>Total Tags  :</b> "+totalTagCount));
//				readRateView.setText(Html.fromHtml("<b>R.R/Sec :</b> "+readRatePerSec));
				populateSearchResult(epcToReadDataMap);
			}
			progressBar.setVisibility(View.INVISIBLE);
			readButton.setClickable(true);
			if (operation.equalsIgnoreCase("AsyncRead")) {
				readButton.setText("Start Reading");	
			}else if(operation.equalsIgnoreCase("SyncRead")){
				readButton.setText("Read");
			}
			readButton.setClickable(true);
			syncReadRadioButton.setClickable(true);
			asyncReadSearchRadioButton.setClickable(true);
			connectButton.setClickable(true);
			if(exceptionOccur){
				disconnectReader();
			}
		}
		 private static void disconnectReader(){
			 ntReaderField.setEnabled(true);
			 serialList.setEnabled(true);
			 serialReaderRadioButton.setClickable(true);
			 networkReaderRadioButton.setClickable(true);
			 connectButton.setText("Connect");
			 servicelayout.setVisibility(8);
			 readerRadioGroup.setVisibility(0);
			 readerActivity.reader = null;
		 }
		private static void populateSearchResult(ConcurrentHashMap<String,TagRecord> epcToReadDataMap) 
		{
			try
			{			
				Set<String> epcKeySet=epcToReadDataMap.keySet();
				for (String epcString : epcKeySet) {
					TagRecord tagRecordData=epcToReadDataMap.get(epcString);
				
					if(!addedEPCRecords.contains(epcString.toString()))
					{
						addedEPCRecords.add(epcString.toString());
						uniqueRecordCount = addedEPCRecords.size();
						if(inflater !=null)
						{
							fullRow = (TableRow)inflater.inflate(R.layout.row, null, true);
							fullRow.setId(uniqueRecordCount);
							
							if(fullRow!=null)
							{
								nr = (TextView) fullRow.findViewById(R.id.nr);
								if(nr!=null)
								{
									nr.setText(String.valueOf(uniqueRecordCount));
									nr.setWidth(readerActivity.rowNumberWidth);
									epcValue = (TextView) fullRow.findViewById(R.id.EPC);
								
									if(epcValue!=null)
									{
										epcValue.setText(tagRecordData.getEpcString());
										epcValue.setMaxWidth(readerActivity.epcDataWidth);
										count = (TextView) fullRow.findViewById(R.id.COUNT);
										if(count!=null)
										{
											count.setText(String.valueOf(tagRecordData.getReadCount()));
											count.setWidth(readerActivity.epcCountWidth);
										}
										table.addView(fullRow);
									}
								}
							}
						}
					}
					else
					{
						fullRow=(TableRow)table.getChildAt(Integer.valueOf(addedEPCRecords.indexOf(epcString)));
						if(fullRow!=null){
							count = (TextView)fullRow.getChildAt(2);
							if(count!=null && Integer.valueOf(count.getText().toString())!=tagRecordData.getReadCount())
							{
								count.setText(String.valueOf(tagRecordData.getReadCount()));
							}
						}
						
						
					}
				}
			}
			catch(Exception ex)
			{
				LoggerUtil.error(TAG, "Exception while populating tags :", ex);
			}
		}
		public static boolean isReading() {
			return reading;
		}
		public void setReading(boolean reading) {
			ReadThread.reading = reading;
		}
     }
}
