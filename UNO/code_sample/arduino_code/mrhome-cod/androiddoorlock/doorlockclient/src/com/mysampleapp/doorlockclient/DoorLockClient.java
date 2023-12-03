/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.doorlockclient;

import java.io.InputStream; //(1)
import java.net.URL;
import android.net.wifi.WifiManager;
import android.widget.Button;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

public class DoorLockClient extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        Button unlockbutton = (Button) findViewById(R.id.unlockbutton); //(2)
        findViewById(R.id.unlockbutton).setOnClickListener
        (mClickListenerUnlockButton);
		  try {  
			  WifiManager wm = 
			  (WifiManager) getSystemService(WIFI_SERVICE); //(3)
			  if (!wm.isWifiEnabled()) {
				  unlockbutton.setEnabled(false);
				  wm.setWifiEnabled(true);
				  // Wait 17 seconds for WiFi to turn on and connect
				  Thread.sleep(17000);
				  unlockbutton.setEnabled(true);
			  }
      	  } catch (Exception e) {
      		  Log.e("LightSwitchClient", "Error: " + e.getMessage(), e);
    	  }	  
        
    }
    View.OnClickListener mClickListenerUnlockButton = 
    	new View.OnClickListener() { //(4)
        public void onClick(View v) {
        	try	{
        		final InputStream is = 
        		new URL("http://192.168.1.230:8000").openStream(); //(5)
        	}
        	catch (Exception e)	{
        	}
        }
    };    
}