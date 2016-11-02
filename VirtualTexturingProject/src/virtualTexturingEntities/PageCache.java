package virtualTexturingEntities;


import java.io.File;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import javax.media.opengl.GL;

import textureUtilities.TextureReader;

import com.sun.opengl.util.BufferUtil;


public class PageCache {

	private  static ByteBuffer pixels;
    private static  int width=Info.w_pixel_pageCache;
    private static int height=Info.w_pixel_pageCache;
	
    //public static ArrayList<DownloadedFrame> downloadedFrames = new ArrayList<DownloadedFrame>(); 
    
    public static List<DownloadedFrame> downloadedFrames = Collections.synchronizedList(new ArrayList<DownloadedFrame>());

    
    private static int numInsertedFrames;
    
    
    public static void createPageCache(GL gl,int texture_index){
    

         
    	pixels = BufferUtil.newByteBuffer(width * height*3); 
    	pixels.limit(pixels.capacity());
    	
    	
    	//System.out.println("pixels.capacity() "+pixels.capacity());
    	
    	for(int i=0; i<pixels.capacity(); i+=3) {
    		
    		byte r= 0;
    		byte g= 127;
    		byte b= 0;
    		
    		pixels.put(r);
    		pixels.put(g);
    		pixels.put(b);
        }
    	pixels.flip();
        
    	
//    	gl.glBindTexture(GL.GL_TEXTURE_2D, texture_index);
//        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR);
//        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR);
//     
//    	
//    	gl.glTexImage2D(GL.GL_TEXTURE_2D,
//                0,
//                3,
//                width,
//                height,
//                0,
//                GL.GL_RGB,
//                GL.GL_UNSIGNED_BYTE,
//                pixels);
//    	
//   	
//    	numInsertedFrames=0;
    	
    }
    
    
    public static void bindPageCacheTexture(GL gl,int texture_index){
    	gl.glBindTexture(GL.GL_TEXTURE_2D, texture_index);
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR);
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR);
     
          
    	gl.glTexImage2D(GL.GL_TEXTURE_2D,
                0,
                3,
                width,
                height,
                0,
                GL.GL_RGB,
                GL.GL_UNSIGNED_BYTE,
                //pixels);
                null);
    
    	numInsertedFrames=0;
    }
    
    public static int getFirstFreePosition(){
    	return numInsertedFrames;
    }
    
    public static void insertFrameAtFirstFreePosition(int ind_Texture,GL gl,int[] fxfyCoord,ByteBuffer frame_pixels){
    	
    	
    	if(numInsertedFrames<Info.pageTableSize){
    	//int[] offset=Info.returnRelativeCoordinatesInPageCache(numInsertedFrames);
    	
    //	System.out.println("offset: "+offset[0]+" "+offset[1]);
    	
    	//ciclo for 
    	
  	
    
    	/*
    	gl.glBindTexture(GL.GL_TEXTURE_2D, ind_Texture);
    	gl.glTexSubImage2D(
    			GL.GL_TEXTURE_2D,//GLenum  target  
    			0,//GLint  level
    			offset[0],//GLint  xoffset
    			offset[1],//GLint  yoffset
    			Info.num_x_Pixel,//GLsizei  width
    			Info.num_y_Pixel,//GLsizei  height
    			GL.GL_RGB, //GLenum  format
    			GL.GL_UNSIGNED_BYTE,//GLenum  type
    			frame_pixels//const GLvoid *  data
    			);
    	*/
    	
    		
    	gl.glBindTexture(GL.GL_TEXTURE_2D, ind_Texture);
    	gl.glTexSubImage2D(
    			GL.GL_TEXTURE_2D,//GLenum  target  
    			0,//GLint  level
    			fxfyCoord[0]*Info.num_x_Pixel,//GLint  xoffset
    			fxfyCoord[1]*Info.num_y_Pixel,//GLint  yoffset
    			Info.num_x_Pixel,//GLsizei  width
    			Info.num_y_Pixel,//GLsizei  height
    			GL.GL_RGB, //GLenum  format
    			GL.GL_UNSIGNED_BYTE,//GLenum  type
    			frame_pixels//const GLvoid *  data
    			);
   
    	
    	
    	numInsertedFrames++;
    	//System.out.println("numInsertedFrames "+numInsertedFrames);
    	
    	
    	
    	}
    }
    
    public static void  insertFirstPage(GL gl,int index_texture) throws Exception{
    	byte level=0;
    	int relIndex=0;
    	int[] fxfy={0,0};
    	String currFile = "pages/level_"+level+"/" + relIndex + ".jpg";
    	insertFrameAtFirstFreePosition(index_texture,gl,fxfy,TextureReader.readTexture(currFile).getPixels());
    	
    }
    
    public static void  detectNeededFrames(ByteBuffer needBuffer) throws Exception{
    	
    	System.out.println("CARICO NEEDED PAGES");
    	needBuffer.rewind();
    	  for(int i=0; i<needBuffer.limit(); i+=3) {
//         	 //byte range = -128 - 127
      		byte x;
      		byte y;
      		byte level;
//      		
      		 x=needBuffer.get();
      		 y=needBuffer.get();
      		 level=needBuffer.get();
    		
			int relPageIndex=Info.returnRelativePageIndex(x, y, level);
			int absPageIndex=Info.returnAbsolutePageIndex(relPageIndex, level);			
			boolean incache = PageTable.isAlreadyInCache(absPageIndex,level);
			
			if(!incache){
				boolean isLoaded = PageTable.isAlreadyLoaded(absPageIndex,level);
				if(!isLoaded){
					String currFile = "pages/level_"+level+"/" + relPageIndex + ".jpg";
				ByteBuffer loadedBuffer =TextureReader.readTexture(currFile).getPixels();
				DownloadedFrame frame = new DownloadedFrame(level,absPageIndex,loadedBuffer);
				//downloadedFrames.add(frame);
				downloadedFrames.add(0,frame);
				PageTable.isloaded(absPageIndex,level);
				}
				}
         }
    	 
    	  System.out.println("fine");
     	  needBuffer.flip();
      }
    /*
 public synchronized static void  testDetectNeededFrames(ByteBuffer needBuffer) throws Exception{
    	
	 int newPages=0;
	 
    	System.out.println("CARICO NEEDED PAGES");
    	needBuffer.rewind();
    	  for(int i=0; i<needBuffer.limit(); i+=3) {
//         	 //byte range = -128 - 127
      		byte x;
      		byte y;
      		byte level;
//      		
      		 x=needBuffer.get();
      		 y=needBuffer.get();
      		 level=needBuffer.get();
      		 
     		
			int relPageIndex=Info.returnRelativePageIndex(x, y, level);
			int absPageIndex=Info.returnAbsolutePageIndex(relPageIndex, level);			
			boolean incache = PageTable.isAlreadyInCache(absPageIndex,level);
			
			if(!incache){
				boolean isLoaded = PageTable.isAlreadyLoaded(absPageIndex,level);
				if(!isLoaded){
					newPages++;
				String currFile = "pages/level_"+level+"/" + relPageIndex + ".jpg";
				ByteBuffer loadedBuffer =TextureReader.readTexture(currFile).getPixels();				
				DownloadedFrame frame = new DownloadedFrame(level,absPageIndex,loadedBuffer);
				downloadedFrames.add(frame);				
				PageTable.isloaded(absPageIndex,level);
			 				}
						}
         }
    	  System.out.println("new PAges: "+newPages);
    	  System.out.println("fine");
     	//  needBuffer.flip();
     	  
     	  
    
    }
    */
public static void  insertLoadedFrames(ByteBuffer needBuffer,GL gl,int index_texture) throws Exception{
    	
    
       	  
       	  int maxFramePerTime=25;
       	int currFramePerTime=0;
       	boolean isFull=false;
     	// for(DownloadedFrame frame: downloadedFrames){
       	  while((downloadedFrames.size()>0)&&(!isFull)){
       		  
       		if(numInsertedFrames<Info.pageTableSize){
       			
       			if(currFramePerTime<maxFramePerTime){
       			
       		DownloadedFrame frame = downloadedFrames.remove(0);
       	 // for(DownloadedFrame frame: downloadedFrames){
       		  
       		int absPageIndex = frame.absPageIndex;
       		int level =  frame.level;
       		
       		int ind_pageCache=getFirstFreePosition();
			int[] pageCacheCoord=Info.returnRelativeCoordinatesFxFyInPageCache(ind_pageCache);
		
       		
       		//int[] pageCacheCoord =  frame.pageCacheCoord;
       		ByteBuffer loadedBuffer = frame.loadedBuffer;
       		
boolean incache = PageTable.isAlreadyInCache(absPageIndex,level);
			
			
			
			if(!incache){
				
				//System.out.println("metto in cache: "+level+" "+absPageIndex+" "+pageCacheCoord[0]+" "+pageCacheCoord[1]);
				PageTable.isInCache(absPageIndex,pageCacheCoord,level);
				insertFrameAtFirstFreePosition(index_texture,gl,pageCacheCoord,loadedBuffer);
			
			}
			
			currFramePerTime++;
       		}else{
       			break;
       		}
			
       		} else{
       			//System.out.println("_pageCache PIENA!!!!");
       			isFull=true;
      			break;
       		}
       	  }
       	  
       	  
       	  
       	  PageTable.updatePageTable();
    }
    
    /*
    public static void  insertMaxNumOfFrames(GL gl,int index_texture) throws Exception{
    	
    	int currIndex=0;
    	
    	for(int level=0;level<Info.numLevels;level++){
    		
    		int numPagesInCurrentLevel=Info.returnNumPagesInCurrentLevel(level);
    		//System.out.println("numMipMapsInCurrentLevel: "+numPagesInCurrentLevel);
    		
    		for(int i=0;i<numPagesInCurrentLevel;i++){
    			
    			if(currIndex<Info.pageTableSize){
    			//String currFile = "pages/level_"+level+"/" + currIndex + ".jpg";
    				String currFile = "pages/level_"+level+"/" + i + ".jpg";
    			//System.out.println(currFile);
    			insertFrameAtFirstFreePosition(index_texture,gl,TextureReader.readTexture(currFile).getPixels());
    			}
    			currIndex++;
    		}
    	}
    }
    */
    
    public static ByteBuffer getFrame(GL gl,int ind_Texture,int fx, int fy, int offX,int offY){
    	
    	gl.glBindTexture(GL.GL_TEXTURE_2D, ind_Texture);
    	int x=fx+offX;
    	int y=fy+offY;
    	
    	ByteBuffer test = BufferUtil.newByteBuffer(width * height*3); 
    	gl.glReadPixels(x, y, 1, 1, GL.GL_RGB, GL.GL_UNSIGNED_BYTE, test);
    	
    	return test;
    	
    }
    
    
}
