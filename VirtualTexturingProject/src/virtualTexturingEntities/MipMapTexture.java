package virtualTexturingEntities;

import java.nio.ByteBuffer;

import javax.media.opengl.GL;

import com.sun.opengl.util.BufferUtil;

public class MipMapTexture {

	public static void initMipMapTexture(GL gl){
		
		int width= Info.w_mipmap_texture;
		int height= Info.h_mipmap_texture;
		
		
		ByteBuffer data = BufferUtil.newByteBuffer(width * height*3); 
    	data.limit(data.capacity());
		
		gl.glBindTexture(GL.GL_TEXTURE_2D, Info.MIPMAP_TEXTURE);
        //gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR);
        //gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR);
		gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER,GL.GL_NEAREST_MIPMAP_LINEAR);
    	
    	gl.glTexImage2D(GL.GL_TEXTURE_2D,
                0,
                3,
                width,
                height,
                0,
                GL.GL_RGB,
                GL.GL_UNSIGNED_BYTE,
                data);
		
	}
	
}
