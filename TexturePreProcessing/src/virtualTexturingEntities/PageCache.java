package virtualTexturingEntities;


import java.io.File;
import java.nio.ByteBuffer;

import javax.media.opengl.GL;

import textureUtilities.TextureReader;

import com.sun.opengl.util.BufferUtil;


public class PageCache {

	private  static ByteBuffer pixels;
    private static  int width=Info.w_pixel_pageCache;
    private static int height=Info.w_pixel_pageCache;
	
    private static int numInsertedFrames;
    
    
    public static void createPageCache(GL gl,int texture_index){
    	gl.glBindTexture(GL.GL_TEXTURE_2D, texture_index);

        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR);
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR);
    	
    	
//    	 TextureReader.Texture texture=null;
//    	 try{
//         texture = TextureReader.readTexture("edBlue.jpg");
//    	 }catch(Exception e){
//    		 e.printStackTrace();
//    	 }
//         gl.glTexImage2D(GL.GL_TEXTURE_2D,
//                 0,
//                 3,
//                 texture.getWidth(),
//                 texture.getHeight(),
//                 0,
//                 GL.GL_RGB,
//                 GL.GL_UNSIGNED_BYTE,
//                 texture.getPixels());
//    	
//         for(int i=0; i<texture.getPixels().capacity(); i+=3) {
//        	 //byte range = -128 - 127
//     		byte r= 5;
//     		byte g= 3;
//     		byte b= 2;
//     		
//     		 r=texture.getPixels().get();
//     		 g=texture.getPixels().get();
//     		 b=texture.getPixels().get();
//     		
//     		 System.out.println("r: "+r+" g:"+g+" b:"+b);
//     		 
//         }
//         texture.getPixels().flip();
         
         
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
        
    //	System.out.println("pixels.capacity() "+pixels.capacity());
    	
    	gl.glBindTexture(GL.GL_TEXTURE_2D, texture_index);
    	gl.glTexImage2D(GL.GL_TEXTURE_2D,
                0,
                3,
                width,
                height,
                0,
                GL.GL_RGB,
                GL.GL_UNSIGNED_BYTE,
                pixels);
    	
   	
    	numInsertedFrames=0;
    	
    }
    
    
    public static int getFirstFreePosition(){
    	return numInsertedFrames;
    }
    
    public static void insertFrameAtFirstFreePosition(int ind_Texture,GL gl,ByteBuffer frame_pixels){
    	
    	
    	int[] offset=Info.returnRelativeCoordinatesInPageCache(numInsertedFrames);
    	
    	//ciclo for 
    	
//    	void glTexSubImage2D(
//    			GLenum  target,  
//    			GLint  level,  
//    			GLint  xoffset,  
//    			GLint  yoffset,  
//    			GLsizei  width,  
//    			GLsizei  height,  
//    			GLenum  format,  
//    			GLenum  type,  
//    			const GLvoid *  data);
//    	System.out.println(width * height * 4);
//    	System.out.println("pagecache: "+pixels.capacity());
//    	System.out.println("frame_pixels: "+frame_pixels.capacity());
//    	
    
    	
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
    	
    	numInsertedFrames++;
    	
    }
    
    public static void  insertNeededFrames(ByteBuffer needBuffer,GL gl,int index_texture) throws Exception{
    	
    //	System.out.println("needBuffer.capacity() "+needBuffer.capacity());
    	//PageTable.readPageTable();
    	  for(int i=0; i<needBuffer.limit(); i+=3) {
//         	 //byte range = -128 - 127
      		byte x;
      		byte y;
      		byte level;
//      		
      		 x=needBuffer.get();
      		 y=needBuffer.get();
      		 level=needBuffer.get();
      		 
      		 int relIndex=Info.returnRelativePageIndex(x, y, level);
 			String currFile = "pages/level_"+level+"/" + relIndex + ".jpg";
			//System.out.println(currFile);
			int ind_pageCache=getFirstFreePosition();
			int relPageIndex=Info.returnRelativePageIndex(x, y, level);
			int absPageIndex=Info.returnAbsolutePageIndex(relPageIndex, level);
			
			int[] pageCacheCoord=Info.returnRelativeCoordinatesFxFyInPageCache(ind_pageCache);
			//int[] pageCacheCoord=Info.returnRelativeCoordinatesFxFyInPageCache(ind_pageCache,level);
			PageTable.isInCache(absPageIndex,pageCacheCoord,level);
			
			insertFrameAtFirstFreePosition(index_texture,gl,TextureReader.readTexture(currFile).getPixels());
//      		
//      		 System.out.println("r: "+r+" g:"+g+" b:"+b);
//      		 
         }
    	//  PageTable.readPageTable();
    		
    	  needBuffer.flip();
    	
    	  PageTable.updatePageTable();
    	  
    }
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
    
    
    public static ByteBuffer getFrame(GL gl,int ind_Texture,int fx, int fy, int offX,int offY){
    	
    	gl.glBindTexture(GL.GL_TEXTURE_2D, ind_Texture);
    	int x=fx+offX;
    	int y=fy+offY;
    	
    	ByteBuffer test = BufferUtil.newByteBuffer(width * height*3); 
    	gl.glReadPixels(x, y, 1, 1, GL.GL_RGB, GL.GL_UNSIGNED_BYTE, test);
    	
    	return test;
    	
    }
    
    
}
