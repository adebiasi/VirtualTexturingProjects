package virtualTexturingEntities;

import java.io.IOException;

import javax.media.opengl.GL;

import main.Graphics;

public class Util {
	public static void loadGLTextures(GL gl,int[] textures) throws IOException {
	       
		   System.out.println("textures[0] "+textures[0]);

	       gl.glGenTextures(4, textures, 0);

	       System.out.println("textures[0] "+textures[0]);
	       
	     Info.PAGE_TABLE_TEXTURE=textures[0];
	     Info.PHYSICAL_TEXTURE=textures[1];
	     Info.MIPMAP_TEXTURE=textures[3];   
	     
	       try{

	    	   MipMapTexture.initMipMapTexture(gl);
	    	   
	       PageTable.initPageTable();
	       //PageTable.readPageTable();   
	       PageCache.createPageCache(gl,Info.PHYSICAL_TEXTURE);
	       PageCache.bindPageCacheTexture(gl,Info.PHYSICAL_TEXTURE);
	       PageCache.insertFirstPage(gl,Info.PHYSICAL_TEXTURE);
	       
	       NeedBuffer.createNeedBuffer();
	      
	       PageCache.detectNeededFrames(NeedBuffer.needBuffer);
	       PageCache.insertLoadedFrames(NeedBuffer.needBuffer,gl,Info.PHYSICAL_TEXTURE);
	        
	       PageTable.updatePageTable();
	       
	       
	NeedBuffer.testNeedBuffer();
	//PageTable.readPageTable();   
	PageCache.detectNeededFrames(NeedBuffer.needBuffer);
	PageCache.insertLoadedFrames(NeedBuffer.needBuffer,gl,Info.PHYSICAL_TEXTURE);
	       
	

	//PageTable.readPageTable();  
	//PageTable.readPageTable();

	   PageTable.getPageTableTexture(gl, Info.PAGE_TABLE_TEXTURE);
	
	
	       }catch(Exception e){
	    	   e.printStackTrace();
	       }
	      

	   
	}
}
