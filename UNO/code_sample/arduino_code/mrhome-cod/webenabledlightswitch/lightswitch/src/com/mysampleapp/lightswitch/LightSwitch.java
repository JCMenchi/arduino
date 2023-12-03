/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.lightswitch;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ToggleButton;
import android.view.View;

import java.net.URL;
import java.io.InputStream;

public class LightSwitch extends Activity {
	  /** Called when the activity is first created. */
	    @Override
		public void onCreate(Bundle savedInstanceState) {
		  super.onCreate(savedInstanceState);
		  setContentView(R.layout.main);
		  final String my_server_ip_address_and_port_number = "192.168.1.100:3344";
		  final ToggleButton toggleButton = 
				(ToggleButton) findViewById(R.id.toggleButton1);
		  toggleButton.setOnClickListener(new View.OnClickListener() 
		  {
		    public void onClick(View v) {
		      if (toggleButton.isChecked()) {
		        try	{
		            final InputStream is = new URL("http://"+ 
	my_server_ip_address_and_port_number +"/command/on").openStream();
		            }
		    	    catch (Exception e)	{
		    	    }
		      } else {
		        try	{
		            final InputStream is = new URL("http://"+ 
	my_server_ip_address_and_port_number +"/command/off").openStream();
		        	}
		        	catch (Exception e)	{
		        	}
		        }
		      }
		  });
	    }
	}