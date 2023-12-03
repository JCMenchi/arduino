/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.androiddoorlockserver;
// Major portions of the web server code from Markus Bode's android-webserver project:
// http://code.google.com/p/android-webserver

// Major portions of the camera code from Krisnaraj Varma's android-camera project:
// http://code.google.com/p/krvarma-android-samples/

import android.os.Bundle;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.Camera;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
//import android.widget.ScrollView;
//import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.os.PowerManager;

import ioio.lib.api.DigitalOutput;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.util.AbstractIOIOActivity;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.FileOutputStream;


public class AndroidDoorLockServerActivity extends AbstractIOIOActivity implements CameraCallback {
	private FrameLayout cameraholder = null;
	private CameraSurface camerasurface = null;
	private Button takepicture;
	
    private ToggleButton mToggleButton;
    private PowerManager.WakeLock wl; // keep phone from sleeping while running server
    private EditText port;
    private Server server;
//    private static TextView mLog;
//    private static ScrollView mScroll;
    private NotificationManager mNotificationManager;
    
    final Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			Bundle b = msg.getData();
//			log(b.getString("msg"));
		}
    };

	class IOIOThread extends AbstractIOIOActivity.IOIOThread {
		/** The on-board LED. */
		private DigitalOutput powertail_;

		/**
		 * Called every time a connection with IOIO has been established.
		 * Typically used to open pins.
		 * 
		 * @throws ConnectionLostException
		 *             When IOIO connection is lost.
		 * 
		 * @see ioio.lib.util.AbstractIOIOActivity.IOIOThread#setup()
		 */
		@Override
		protected void setup() throws ConnectionLostException {
			powertail_ = ioio_.openDigitalOutput(3,false);
		}

		/**
		 * Called repetitively while the IOIO is connected.
		 * 
		 * @throws ConnectionLostException
		 *             When IOIO connection is lost.
		 * 
		 * @see ioio.lib.util.AbstractIOIOActivity.IOIOThread#loop()
		 */
		@Override
		protected void loop() throws ConnectionLostException {

		  if (mToggleButton.isChecked()) {
		    if (LockStatus.getInstance().getLockStatus()) {
			  try {
			    powertail_.write(false);
				// pause for 5 seconds to keep the lock open
				sleep(5000); 
				powertail_.write(true);
				LockStatus.getInstance().setMyVar(false);
				// Take a picture and send it as an email attachment
				camerasurface.startTakePicture();
				} catch (InterruptedException e) {				
					} 
				}else {
				  try {
				    sleep(10);
				  } catch (InterruptedException e) {
				}
				}			
			} else {
				powertail_.write(true);
			}
		}
	}

	/**
	 * A method to create our IOIO thread.
	 * 
	 * @see ioio.lib.util.AbstractIOIOActivity#createIOIOThread()
	 */
	@Override
	protected AbstractIOIOActivity.IOIOThread createIOIOThread() {
		return new IOIOThread();
	}
	
	@Override
	protected void onPause() {
	super.onPause();
	wl.release();
	}
	
	@Override
	protected void onResume() {
	super.onResume();
	wl.acquire();
	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        cameraholder = (FrameLayout)findViewById(R.id.camera_preview);
       
        setupPictureMode();
        
        takepicture = (Button)findViewById(R.id.takepicture);
        takepicture.setOnClickListener(onButtonClick);       
 
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "DoNotDimScreen");
        wl.acquire();
        
        mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        
        mToggleButton = (ToggleButton) findViewById(R.id.toggle);
        
        port = (EditText) findViewById(R.id.port);
//        mLog = (TextView) findViewById(R.id.log);
//        mScroll = (ScrollView) findViewById(R.id.ScrollView01);

        String pfad = "/sdcard/androiddoorlockserver/";
        
        boolean exists = (new File(pfad)).exists();
        try {
	        if (!exists) {
	        	(new File(pfad)).mkdir();
	         	BufferedWriter bout = new BufferedWriter(new FileWriter(pfad+"index.html"));
	         	bout.write("<html><head><title>Android Door Lock Server powered by bolutions.com</title>");
	         	bout.write("</head>");
	         	bout.write("<body>Door Unlocked for 5 seconds...");
	         	bout.write("<br><br>Pages located in /sdcard/androiddoorlockserver/</body></html>");
	         	bout.flush();
	         	bout.close();
	         	bout = new BufferedWriter(new FileWriter(pfad+"403.html"));
	         	bout.write("<html><head><title>Error 403 powered by bolutions.com</title>");
	         	bout.write("</head>");
	         	bout.write("<body>403 - Forbidden</body></html>");
	         	bout.flush();
	         	bout.close();
	         	bout = new BufferedWriter(new FileWriter(pfad+"404.html"));
	         	bout.write("<html><head><title>Error 404 powered by bolutions.com</title>");
	         	bout.write("</head>");
	         	bout.write("<body>404 - File not found</body></html>");
	         	bout.flush();
	         	bout.close();
	        }
        }catch (Exception e) {
        	Log.v("ERROR",e.getMessage());
        }
        
        mToggleButton.setOnClickListener(new OnClickListener() {
			public void onClick(View arg0) {
				if( mToggleButton.isChecked() ) {
					startServer(new Integer(port.getText().toString()));
				} else {
					stopServer();
				}
			}
			
		});
    }

    private void setupPictureMode(){
    	camerasurface = new CameraSurface(this);
    	
    	cameraholder.addView(camerasurface, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
    	
    	camerasurface.setCallback(this);
    }
	@Override
	public void onJpegPictureTaken(byte[] data, Camera camera) {
		try
		{
			FileOutputStream outStream = new FileOutputStream(String.format(
					"/sdcard/capture.jpg"));
			
			outStream.write(data);
			outStream.close();
			
/*
   			// If we were to send the photo to an Intent, this is how it would be done:
			Intent i = new Intent(Intent.ACTION_SEND);
			i.putExtra(Intent.EXTRA_SUBJECT, "Title");
			i.putExtra(Intent.EXTRA_TEXT, "Content");
			i.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(new File(Environment.getExternalStorageDirectory(),"capture.jpg")));
			i.setType("jpeg/image");
			startActivity(Intent.createChooser(i, "Send mail"));
*/
            try {   
                GMailSender mail = new GMailSender("YOUR_GMAIL_ADDRESS@gmail.com", "YOUR_GMAIL_PASSWORD");
                mail.addAttachment(Environment.getExternalStorageDirectory()+"/capture.jpg");
                String[] toArr = {"EMAIL_RECIPIENT_ADDRESS@gmail.com"};
                mail.setTo(toArr);
                mail.setFrom("YOUR_GMAIL_ADDRESS@gmail.com");
                mail.setSubject("Image capture");
                mail.setBody("Image captured - see attachment");
                if(mail.send()) {
                    Toast.makeText(AndroidDoorLockServerActivity.this, "Email was sent successfully.", Toast.LENGTH_LONG).show();
                  } else {
                    Toast.makeText(AndroidDoorLockServerActivity.this, "Email was not sent.", Toast.LENGTH_LONG).show();
                  }
            } catch (Exception e) {   
                Log.e("SendMail", e.getMessage(), e);   
            } 				
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
		
		camerasurface.startPreview();
	}
	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
	}

	@Override
	public void onRawPictureTaken(byte[] data, Camera camera) {
	}

	@Override
	public void onShutter() {
	}
	
	@Override
	public String onGetVideoFilename(){
		String filename = String.format("/sdcard/%d.3gp",System.currentTimeMillis());
		
		return filename;
	}
	
	private View.OnClickListener onButtonClick = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			switch(v.getId())
			{
				case R.id.takepicture:
				{
					camerasurface.startTakePicture();
					
					break;
				}
			}
		}
	};
    
    private void stopServer() {
    	if( server != null ) {
    		server.stopServer();
    		server.interrupt();
//    		log("Server was killed.");
    		mNotificationManager.cancelAll();
    	}
    	else
    	{
//    		log("Cannot kill server!? Please restart your phone.");
    	}
    }
    
//    public static void log( String s ) {
//    	mLog.append(s + "\n");
//    	mScroll.fullScroll(ScrollView.FOCUS_DOWN);
//    }
    
    public static String intToIp(int i) {
        return ((i       ) & 0xFF) + "." +
               ((i >>  8 ) & 0xFF) + "." +
               ((i >> 16 ) & 0xFF) + "." +
               ( i >> 24   & 0xFF);
    }
    
    private void startServer(int port) {
    	try {
    		WifiManager wifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
    		WifiInfo wifiInfo = wifiManager.getConnectionInfo();
    		
    		String ipAddress = intToIp(wifiInfo.getIpAddress());

    		if( wifiInfo.getSupplicantState() != SupplicantState.COMPLETED) {
    			new AlertDialog.Builder(this).setTitle("Error").setMessage("Please connect to a WIFI-network for starting the webserver.").setPositiveButton("OK", null).show();
    			throw new Exception("Please connect to a WIFI-network.");
    		}
            
//    		log("Starting server "+ipAddress + ":" + port + ".");
		    server = new Server(ipAddress,port,mHandler);
		    server.start();
		    
	        Intent i = new Intent(this, AndroidDoorLockServerActivity.class);
	        PendingIntent contentIntent = PendingIntent.getActivity(this, 0, i, 0);

	        Notification notif = new Notification(R.drawable.icon, "Webserver is running", System.currentTimeMillis());
	        notif.setLatestEventInfo(this, "Webserver", "Webserver is running", contentIntent);
	        notif.flags = Notification.FLAG_NO_CLEAR;
	        mNotificationManager.notify(1234, notif);
    	} catch (Exception e) {
//    		log(e.getMessage());
    		mToggleButton.setChecked(false);
    	}
    }
}