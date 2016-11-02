package virtualTexturingEntities;

import java.io.IOException;

import javax.media.opengl.GL;

public class Util {
	public static void loadGLTextures(GL gl,int[] textures) throws IOException {
	       
		   

	       gl.glGenTextures(1, textures, 0);

	       
	     Info.PAGE_TABLE_TEXTURE=textures[0];
	        
	       try{
	       
	       PageTable.initPageTable();
	       //PageTable.readPageTable();   
	       PageCache.createPageCache(gl,Info.PHYSICAL_TEXTURE);
	      
	       NeedBuffer.createNeedBuffer();
	       
	       PageCache.insertNeededFrames(NeedBuffer.needBuffer,gl,Info.PHYSICAL_TEXTURE);
	        
	NeedBuffer.testNeedBuffer();
	//PageTable.readPageTable();   
	PageCache.insertNeededFrames(NeedBuffer.needBuffer,gl,Info.PHYSICAL_TEXTURE);
	       
	PageTable.getPageTableTexture(gl, Info.PAGE_TABLE_TEXTURE);

	PageTable.readPageTable();  
	//PageTable.readPageTable();

	       }catch(Exception e){
	    	   e.printStackTrace();
	       }
	      

	   
	}
}
