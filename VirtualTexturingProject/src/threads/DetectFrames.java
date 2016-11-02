package threads;

import javax.media.opengl.GL;

import main.Graphics;
import virtualTexturingEntities.NeedBuffer;
import virtualTexturingEntities.PageCache;

public class DetectFrames implements Runnable{

	
	public DetectFrames() {
		super();
	
	}



	@Override
	public void run() {
		// TODO Auto-generated method stub
		while(true){
			
			
			
			
	//		Graphics.theadIsRunning=true;
		//	if(Graphics.retrieveReadbackBuffer==false){
    		
        try{
        	synchronized(Graphics.readback_pixels){
        //		System.out.println("stò leggendo il readbuffer");
        	Graphics.updateNeedBuffer();

	//PageCache.detectNeededFrames(NeedBuffer.needBuffer);
        	
//    	System.out.println("ho letto il readbuffer");
       	}
        	//PageCache.testDetectNeededFrames(NeedBuffer.needBuffer);
        	PageCache.detectNeededFrames(NeedBuffer.needBuffer);
        	
        	System.out.println("I need : "+PageCache.downloadedFrames.size()+" pages");
        }
 catch(Exception e){
	 e.printStackTrace();
 }
		//	}
 //Graphics.theadIsRunning=false;
      //  }
		
 sleep(500);
		}
}
	
	
	private synchronized void sleep(long delay) {
        try {
            wait(delay);
        } catch (InterruptedException ie1) {
            /* ignore. */
        }
    }
}