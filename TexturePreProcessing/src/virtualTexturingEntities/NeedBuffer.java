package virtualTexturingEntities;

import java.nio.ByteBuffer;

import com.sun.opengl.util.BufferUtil;

public class NeedBuffer {

	public  static ByteBuffer needBuffer;
	
	public static void createNeedBuffer(){
		
		int numPages=5;
		int numElements=3;
		int bytePerElement=1;
		
		needBuffer = BufferUtil.newByteBuffer(numPages*numElements*bytePerElement); 
		needBuffer.limit(3);
	
	
	//System.out.println("createNeedBuffer() with curernt capacity "+needBuffer.capacity());
	
	//for(int i=0; i<needBuffer.capacity(); i+=(numElements*bytePerElement)) {
		
		byte x_coord= 127;
		byte y_coord= 0;
		byte level= 0;
		
		needBuffer.put((byte)0);
		needBuffer.put((byte)0);
		needBuffer.put((byte)0);		
		
	needBuffer.flip();
	
	}
	
	
public static void testNeedBuffer(){
		
		int numPages=8;
		int numElements=3;
		int bytePerElement=1;
		
		needBuffer = BufferUtil.newByteBuffer(numPages*numElements*bytePerElement); 
		needBuffer.limit(needBuffer.capacity());
	
	
	System.out.println("pixels.capacity() "+needBuffer.capacity());
	
	//for(int i=0; i<needBuffer.capacity(); i+=(numElements*bytePerElement)) {
		
		byte x_coord= 127;
		byte y_coord= 0;
		byte level= 0;
		
//		needBuffer.put((byte)0);
//		needBuffer.put((byte)0);
//		needBuffer.put((byte)0);
//		
		needBuffer.put((byte)0);
		needBuffer.put((byte)0);
		needBuffer.put((byte)1);
		
		needBuffer.put((byte)0);
		needBuffer.put((byte)1);
		needBuffer.put((byte)1);
		
		needBuffer.put((byte)1);
		needBuffer.put((byte)1);
		needBuffer.put((byte)2);
		
		needBuffer.put((byte)2);
		needBuffer.put((byte)2);
		needBuffer.put((byte)2);
		
		
		needBuffer.put((byte)6);
		needBuffer.put((byte)6);
		needBuffer.put((byte)3);
		
		needBuffer.put((byte)12);
		needBuffer.put((byte)1);
		needBuffer.put((byte)3);
		
		needBuffer.put((byte)11);
		needBuffer.put((byte)11);
		needBuffer.put((byte)4);
		
		needBuffer.put((byte)16);
		needBuffer.put((byte)12);
		needBuffer.put((byte)5);
		
//    }
	needBuffer.flip();
	
	}
	
}
