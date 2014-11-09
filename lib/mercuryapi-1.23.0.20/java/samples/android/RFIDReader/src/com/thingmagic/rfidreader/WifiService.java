//package com.thingmagic.rfidreader;
//
//import java.net.InetAddress;
//import java.net.UnknownHostException;
//import java.util.HashMap;
//import java.util.Map;
//
//import javax.jmdns.JmDNS;
//import javax.jmdns.ServiceEvent;
//import javax.jmdns.ServiceInfo;
//import javax.jmdns.ServiceListener;
//
//import com.thingmagic.util.Utilities;
//
//import android.app.Activity;
//import android.net.wifi.WifiInfo;
//import android.net.wifi.WifiManager;
//
//public class WifiService {
//
//	static JmDNS jmdns;
//	static Map<String, String> discoveredReaders = new HashMap<String, String>();
//	public static final java.lang.String WIFI_SERVICE = "wifi";
//	static Activity mactivity;
//
//	static class SampleListener implements ServiceListener {
//
//		@Override
//		public void serviceAdded(ServiceEvent event) {
//			jmdns.requestServiceInfo(event.getType(), event.getName());
//		}
//
//		@Override
//		public void serviceRemoved(ServiceEvent event) {
//			System.out.println("Service removed : " + event.getName() + "."
//					+ event.getType());
//		}
//
//		@Override
//		public void serviceResolved(ServiceEvent event) {
//			System.out.println("serviceResolved : "
//					+ event.getInfo().getAddress().getHostAddress() + " - "
//					+ event.getType());
//
//			discoveredReaders.put(event.getName(), event.getInfo().getAddress()
//					.getHostAddress());
//		}
//	}
//
//	public static Map<String, String> discoverReader(Activity activity) {
//		try {
//			mactivity = activity;
//			new Thread(new Runnable() {
//
//				@Override
//				public void run() {
//					WifiManager wifiManager = (WifiManager) mactivity
//							.getSystemService(WIFI_SERVICE);
//					WifiInfo wifiInfo = wifiManager.getConnectionInfo();
//					int ipAddress = wifiInfo.getIpAddress();
//					String ip = intToIp(ipAddress);
//					InetAddress inetAddress;
//					try {
//						inetAddress = InetAddress.getByName(ip);
//						jmdns = JmDNS.create(inetAddress);
//					} catch (Exception e) {
//						// TODO Auto-generated catch block
//						e.printStackTrace();
//					}
//
//					String[] serviceType = { "_llrp._tcp.local." };
//					SampleListener listener = new SampleListener();
//
//					for (String service : serviceType) {
//						jmdns.addServiceListener(service, listener);
//					}
//				}
//			}).start();
//
//			Thread.sleep(1000);
//		} catch (Exception ex) {
//			ex.printStackTrace();
//		}
//		return discoveredReaders;
//	}
//
//	public static String intToIp(int i) {
//
//		return ((i >> 24) & 0xFF) + "." + ((i >> 16) & 0xFF) + "."
//				+ ((i >> 8) & 0xFF) + "." + (i & 0xFF);
//	}
//
//}
