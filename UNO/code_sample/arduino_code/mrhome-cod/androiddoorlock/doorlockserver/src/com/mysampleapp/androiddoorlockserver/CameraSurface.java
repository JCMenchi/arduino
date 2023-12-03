/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
package com.mysampleapp.androiddoorlockserver;
import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.GestureDetector.OnGestureListener;
import java.io.IOException;

public class CameraSurface extends SurfaceView implements SurfaceHolder.Callback, OnGestureListener{	
	private Camera camera = null;
	private SurfaceHolder holder = null;
	private CameraCallback callback = null;
	private GestureDetector gesturedetector = null;
	
	public CameraSurface(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		
		initialize(context);
	}

	public CameraSurface(Context context) {
		super(context);
		
		initialize(context);
	}

	public CameraSurface(Context context, AttributeSet attrs) {
		super(context, attrs);
		
		initialize(context);
	}
	
	public void setCallback(CameraCallback callback){
		this.callback = callback;
	}
	
	public void startPreview(){
		camera.startPreview();
	}
public void startTakePicture(){
  camera.autoFocus(new AutoFocusCallback() {
    @Override
      public void onAutoFocus(boolean success, Camera camera) {
        takePicture();
      }
    });
}
	
public void takePicture() {
  camera.takePicture(
        new ShutterCallback() {
          @Override
          public void onShutter(){
            if(null != callback) callback.onShutter();
          }
        },
        new PictureCallback() {
          @Override
          public void onPictureTaken(byte[] data, Camera camera){
            if(null != callback) callback.onRawPictureTaken(data, camera);
          }
        },
        new PictureCallback() {
          @Override
          public void onPictureTaken(byte[] data, Camera camera){
            if(null != callback) callback.onJpegPictureTaken(data, camera);
          }
        });
  }
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,int height) {
		if(null != camera)
		{
			camera.startPreview();
		}
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		camera = Camera.open();
	    /* - Set front camera:
		Camera.Parameters parameters = camera.getParameters();
	    parameters.set("camera-id", 2);
	    camera.setParameters(parameters);
		*/
		try {
			camera.setPreviewDisplay(holder);
			camera.setPreviewCallback(new Camera.PreviewCallback() {
				@Override
				public void onPreviewFrame(byte[] data, Camera camera) {
					if(null != callback) callback.onPreviewFrame(data, camera);
				}
			});
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		camera.stopPreview();
		camera.release();
		
		camera = null;
	}
	
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		return gesturedetector.onTouchEvent(event);
	}
	
	@Override
	public boolean onDown(MotionEvent e) {
		return false;
	}

	@Override
	public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
		return false;
	}

	@Override
	public void onLongPress(MotionEvent e) {
		startTakePicture();
	}

	@Override
	public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,float distanceY) {
		return false;
	}

	@Override
	public void onShowPress(MotionEvent e) {
	}

	@Override
	public boolean onSingleTapUp(MotionEvent e) {
		return false;
	}

	private void initialize(Context context) {
		holder = getHolder();
		
		holder.addCallback(this);
		holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		
		gesturedetector = new GestureDetector(this);
	}
}
