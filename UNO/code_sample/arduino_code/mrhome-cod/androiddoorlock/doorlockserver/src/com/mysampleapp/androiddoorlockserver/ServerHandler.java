/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.androiddoorlockserver;

import java.io.*;
import java.net.*;

class ServerHandler extends Thread {
  private BufferedReader in;
  private PrintWriter out;
  private Socket toClient;
  
  ServerHandler(Socket s) {
    toClient = s;
  }

  public void run() {
	String document = "";

    try {
      in = new BufferedReader(new InputStreamReader(toClient.getInputStream()));
      // Receive data
      while (true) {
        String s = in.readLine().trim();

        if (s.equals("")) {
          break;
        }
        
        if (s.substring(0, 3).equals("GET")) {
        	int leerstelle = s.indexOf(" HTTP/");
        	document = s.substring(5,leerstelle);
        	document = document.replaceAll("[/]+","/");
        }
      }
    }
    catch (Exception e) {
    	Server.remove(toClient);
    	try
		{
    		toClient.close();
		}
    	catch (Exception ex){}
    }
    
    // Standard-Doc
	if (document.equals("")) {
		document = "index.html";
	}
	
	// Don't allow directory traversal
	if (document.indexOf("..") != -1) document = "403.html";
	
	// Search for files in docroot
	document = "/sdcard/androiddoorlockserver/" + document;
	document = document.replaceAll("[/]+","/");
	if(document.charAt(document.length()-1) == '/') document = "/sdcard/androiddoorlockserver/404.html";
	
	String headerBase = "HTTP/1.1 %code%\n"+
	"Server: Bolutions/1\n"+
	"Content-Length: %length%\n"+
	"Connection: close\n"+
	"Content-Type: text/html; charset=iso-8859-1\n\n";

	String header = headerBase;
	header = header.replace("%code%", "403 Forbidden");

	try {
    	File f = new File(document);
        if (!f.exists()) {
        	header = headerBase;
        	header = header.replace("%code%", "404 File not found");
        	document = "404.html";
        }
    }
    catch (Exception e) {}

    if (!document.equals("/sdcard/androiddoorlockserver/403.html")) {
    	header = headerBase.replace("%code%", "200 OK");
    }
	
    try {
      File f = new File(document);
      if (f.exists()) {
    	  BufferedInputStream in = new BufferedInputStream(new FileInputStream(document));
    	  BufferedOutputStream out = new BufferedOutputStream(toClient.getOutputStream());
    	  ByteArrayOutputStream tempOut = new ByteArrayOutputStream();
    	  
    	  byte[] buf = new byte[4096];
    	  int count = 0;
    	  while ((count = in.read(buf)) != -1){
    		  tempOut.write(buf, 0, count);
    	  }

    	  tempOut.flush();
    	  header = header.replace("%length%", ""+tempOut.size());

    	  out.write(header.getBytes());
    	  out.write(tempOut.toByteArray());
    	  out.flush();
      }
      else
      {
          // Send HTML-File (Ascii, not as a stream)
    	  header = headerBase;
    	  header = header.replace("%code%", "404 File not found");	    	  
    	  header = header.replace("%length%", ""+"404 - File not Found".length());	    	  
          out = new PrintWriter(toClient.getOutputStream(), true);
          out.print(header);
    	  out.print("404 - File not Found");
    	  out.flush();
      }

      Server.remove(toClient);
      toClient.close();
    }
    catch (Exception e) {
    	
    }
  }
}
