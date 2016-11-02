package objects;
import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;

import shaderUtilities.ShaderUtilities;

import com.sun.opengl.util.GLUT;


public abstract class Wall 
	{
		private double xPos;
		private double yPos;
		private double zPos;
		
		private int height = 20;
		private int width = 20;
		
		
		private double pitch;
		private double yaw;
		
		public Wall(double xPos, double yPos, double zPos)
		{
			this.xPos = xPos;
			this.yPos = yPos;
			this.zPos = zPos;
			
			this.pitch = 0;
			this.yaw = 0;
		}
	 
		public void updatePosition(double xPos, double yPos, double zPos)
	    {
	        this.xPos = xPos;
	        this.yPos = yPos;
	        this.zPos = zPos; 
	    }
		
		private void drawQuad(GLAutoDrawable drawable, GLUT glut){
			
			GL gl = drawable.getGL();
			
			gl.glPushMatrix();
			gl.glTranslated(xPos, yPos, zPos);
				
			gl.glRotatef(90, 0.0f, 1.0f, 0.0f);
			gl.glBegin(GL.GL_QUADS);       
		       gl.glTexCoord2f(1.0f, 0.0f);
		       //gl.glVertex3d(-5, -5, 0);
		       gl.glVertex3d(-width, -height, 0);
		       gl.glTexCoord2f(1.0f, 1.0f);
		       //gl.glVertex3d(-5, 5, 0);
		       gl.glVertex3d(-width, height, 0);
		       gl.glTexCoord2f(0.0f, 1.0f);
		       //gl.glVertex3d(5, 5, 0);
		       gl.glVertex3d(width, height, 0);
		       gl.glTexCoord2f(0.0f, 0.0f);
		       //gl.glVertex3d(5, -5, 0);
		       gl.glVertex3d(width, -height, 0);
		       gl.glEnd();
			
		gl.glPopMatrix();
		}
		
		public void draw(GLAutoDrawable drawable, GLUT glut)
		{
			
			GL gl = drawable.getGL();
			 gl.glUseProgram(ShaderUtilities.shaderprogram);
		//	gl.glUseProgram(0);
			 gl.glDisable(GL.GL_TEXTURE_2D);
			 drawQuad(drawable,	glut				 );
			gl.glUseProgram(0);
			
		}
		
		
	
		public void drawShader2(GLAutoDrawable drawable, GLUT glut)
		{
			
			GL gl = drawable.getGL();
			 gl.glUseProgram(ShaderUtilities.shaderprogram2);
		//	gl.glUseProgram(0);
			 gl.glDisable(GL.GL_TEXTURE_2D);
		
			 drawQuad(drawable,	glut				 );
			 /*
			 gl.glPushMatrix();
				gl.glTranslated(xPos, yPos, zPos);
				gl.glRotatef(90, 0.0f, 1.0f, 0.0f);
		 			
				gl.glBegin(GL.GL_QUADS);       
				gl.glTexCoord2f(1.0f, 0.0f);
			       //gl.glVertex3d(-5, -5, 0);
			       gl.glVertex3d(-width, -height, 0);
			       gl.glTexCoord2f(1.0f, 1.0f);
			       //gl.glVertex3d(-5, 5, 0);
			       gl.glVertex3d(-width, height, 0);
			       gl.glTexCoord2f(0.0f, 1.0f);
			       //gl.glVertex3d(5, 5, 0);
			       gl.glVertex3d(width, height, 0);
			       gl.glTexCoord2f(0.0f, 0.0f);
			       //gl.glVertex3d(5, -5, 0);
			       gl.glVertex3d(width, -height, 0);
			       gl.glEnd();
				
			gl.glPopMatrix();
		
			*/
		}
		
	 
	}

