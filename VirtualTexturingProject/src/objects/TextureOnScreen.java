package objects;

import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;

import virtualTexturingEntities.Info;

import com.sun.opengl.util.GLUT;

public abstract class TextureOnScreen {
	private double xPos;
	private double yPos;
	public int _id_texture;
	private int dim2dTexture= 200;
	
	public TextureOnScreen(double pos, double pos2, int id_texture) {
		super();
		xPos = pos;
		yPos = pos2;
		System.out.println("assegno: "+id_texture);
		_id_texture=id_texture;
	}



	public void draw(GLAutoDrawable drawable, GLUT glut)
	{
		
		
		GL gl = drawable.getGL();
		gl.glActiveTexture(GL.GL_TEXTURE0);
        gl.glEnable(GL.GL_TEXTURE_2D);
        gl.glBindTexture(GL.GL_TEXTURE_2D, _id_texture);
	       gl.glUseProgram(0);
	       gl.glPushMatrix();
	        gl.glTranslated(xPos, yPos,0);
				gl.glBegin(GL.GL_QUADS);       
			       gl.glTexCoord2f(0.0f, 0.0f);
			       gl.glVertex2d( 0, 0);
			       gl.glTexCoord2f(0.0f, 1.0f);
			       gl.glVertex2d(0, dim2dTexture);
			       gl.glTexCoord2f(1.0f, 1.0f);
			       gl.glVertex2d(dim2dTexture, dim2dTexture);
			       gl.glTexCoord2f(1.0f, 0.0f);
			       gl.glVertex2d(dim2dTexture, 0);
			       gl.glEnd();
			gl.glPopMatrix();
	}
}
