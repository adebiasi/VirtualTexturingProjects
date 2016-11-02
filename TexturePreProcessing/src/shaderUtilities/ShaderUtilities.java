package shaderUtilities;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import javax.media.opengl.GL;

import virtualTexturingEntities.Info;


public class ShaderUtilities {
	
	static private int pixelShaderProgram;
	   //private int fragmentShaderProgram;
	   static public int shaderprogram;
	
	   public static void updateUniformVars(GL gl) {

		      int physPageCols = gl.glGetUniformLocation(ShaderUtilities.shaderprogram, "physPageCols");
		      int physPageRows = gl.glGetUniformLocation(ShaderUtilities.shaderprogram, "physPageRows");
		      int maxMipLevel = gl.glGetUniformLocation(ShaderUtilities.shaderprogram, "mip_bias");	      
		      int pageTableTex = gl.glGetUniformLocation(ShaderUtilities.shaderprogram, "pageTableTexture");	      
		      int physicalTex = gl.glGetUniformLocation(ShaderUtilities.shaderprogram, "physicalTexture");
		      
		      assert(physPageCols!=-1);
		      assert(physPageRows!=-1);
		      assert(maxMipLevel!=-1);
		      assert(pageTableTex!=-1);
		      assert(physicalTex!=-1);

	      gl.glUniform1f(physPageCols, Info.w_pageCache);
		      gl.glUniform1f(physPageRows, Info.h_pageCache);
		      gl.glUniform1f(maxMipLevel, Info.numLevels);
		      

		      gl.glActiveTexture(GL.GL_TEXTURE6);
		      gl.glBindTexture(GL.GL_TEXTURE_2D, Info.PAGE_TABLE_TEXTURE);
		      gl.glUniform1i(pageTableTex, 6);
		      
		     
		      gl.glActiveTexture(GL.GL_TEXTURE7);
		      gl.glBindTexture(GL.GL_TEXTURE_2D, Info.PHYSICAL_TEXTURE);
		      gl.glUniform1i(physicalTex, 7);

		   }

	   
	   public static void attachShaders(GL gl) throws Exception {
		      ShaderUtilities.pixelShaderProgram = gl.glCreateShader(GL.GL_FRAGMENT_SHADER);
		      

		      String[] vsrc = ShaderUtilities.loadShaderSrc("Shader/shader_new.txt");
		      
		      gl.glShaderSource(ShaderUtilities.pixelShaderProgram, 1, vsrc, null, 0);
		      gl.glCompileShader(ShaderUtilities.pixelShaderProgram);

		      ShaderUtilities.shaderprogram = gl.glCreateProgram();
		      gl.glAttachShader(ShaderUtilities.shaderprogram, ShaderUtilities.pixelShaderProgram);
		      
		      gl.glLinkProgram(ShaderUtilities.shaderprogram);
		      gl.glValidateProgram(ShaderUtilities.shaderprogram);
		      IntBuffer intBuffer = IntBuffer.allocate(1);
		      gl.glGetProgramiv(ShaderUtilities.shaderprogram, GL.GL_LINK_STATUS,intBuffer);
		      if (intBuffer.get(0)!=1){
		         gl.glGetProgramiv(ShaderUtilities.shaderprogram, GL.GL_INFO_LOG_LENGTH,intBuffer);
		         int size = intBuffer.get(0);
		         System.err.println("Program link error: ");
		         if (size>0){
		            ByteBuffer byteBuffer = ByteBuffer.allocate(size);
		            gl.glGetProgramInfoLog(ShaderUtilities.shaderprogram, size, intBuffer, byteBuffer);
		            for (byte b:byteBuffer.array()){
		               System.err.print((char)b);
		            }
		         } else {
		            System.out.println("Unknown");
		         }
		         System.exit(1);
		      }

	   }
	   
	static public String[] loadShaderSrc(String name){
	      StringBuilder sb = new StringBuilder();
	      try{
	    	  InputStream is = new FileInputStream(name);
	         //InputStream is = getClass().getResourceAsStream(name);
	         BufferedReader br = new BufferedReader(new InputStreamReader(is));
	         String line;
	         while ((line = br.readLine())!=null){
	            sb.append(line);
	            sb.append('\n');
	         }
	         is.close();
	      }
	      catch (Exception e){
	         e.printStackTrace();
	      }
	     // System.out.println("Shader is "+sb.toString());
	      return new String[]{sb.toString()};
	   }
}
