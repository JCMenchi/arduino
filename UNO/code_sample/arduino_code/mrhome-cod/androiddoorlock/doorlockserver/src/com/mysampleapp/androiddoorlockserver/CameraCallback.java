/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.androiddoorlockserver;

import android.hardware.Camera;

public interface CameraCallback {
	public abstract void onPreviewFrame(byte[] data, Camera camera);
	public abstract void onShutter();
	public abstract void onRawPictureTaken(byte[] data, Camera camera);
	public abstract void onJpegPictureTaken(byte[] data, Camera camera);
	public abstract String onGetVideoFilename();
}
