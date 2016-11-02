package objects;

import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;

import shaderUtilities.ShaderUtilities;

import com.sun.opengl.util.GLUT;

public class Terrain {
	
	private double xPos;
	private double yPos;
	private double zPos;
	
	private int height = 20;
	private int width = 20;
	
	
	private int MAP_SCALE =5;
	private int MAP_Z =20;
	private int MAP_X =20;
	private float[][][] terrain;
	
	public Terrain(double xPos, double yPos, double zPos)
	{
		this.xPos = xPos;
		this.yPos = yPos;
		this.zPos = zPos;
		InitializeTerrain();
	}
	
	// InitializeTerrain()
	// desc: initializes the heightfield terrain data
	void InitializeTerrain()
	{
		terrain= new float[MAP_Z][MAP_X][3];
	    // loop through all of the heightfield points, calculating
	    // the coordinates for each point
	    for (int z = 0; z < MAP_Z; z++)
	    {
	        for (int x = 0; x < MAP_X; x++)
	        {
	            terrain[x][z][0] = (float)(x)*MAP_SCALE;
	            
	            double y = Math.random();
	            //double y = 1;
	            terrain[x][z][1] = (float)(y*MAP_SCALE);
	            terrain[x][z][2] = -(float)(z)*MAP_SCALE;
	        }
	    }
	}
	public void draw_with_Shader_1(GLAutoDrawable drawable, GLUT glut)
	{
		GL gl = drawable.getGL();
		 gl.glUseProgram(ShaderUtilities.shaderprogram);
	//	gl.glUseProgram(0);
		 gl.glDisable(GL.GL_TEXTURE_2D);
		 
		 draw(drawable,glut);
		 
		 gl.glUseProgram(0);
		 
			gl.glUseProgram(0);
	}
	
	public void draw_with_Shader_2(GLAutoDrawable drawable, GLUT glut)
	{
		GL gl = drawable.getGL();
		 gl.glUseProgram(ShaderUtilities.shaderprogram2);
	//	gl.glUseProgram(0);
		 gl.glDisable(GL.GL_TEXTURE_2D);
		 
		 draw(drawable,glut);
		 
		 gl.glUseProgram(0);
		 
			gl.glUseProgram(0);
	}
	
	private void draw(GLAutoDrawable drawable, GLUT glut)
	{
		
		GL gl = drawable.getGL();
	//	 gl.glUseProgram(ShaderUtilities.shaderprogram);
		//	 gl.glDisable(GL.GL_TEXTURE_2D);
		gl.glPushMatrix();
			gl.glTranslated(xPos, yPos, zPos);

			
			for (int z = 0; z < MAP_Z-1; z++)
		    {
				gl.glBegin(GL.GL_TRIANGLE_STRIP);
				//gl.glBegin(GL.GL_QUADS);
		        for (int x = 0; x < MAP_X-1; x++)
		        {
		            // for each vertex, we calculate
		            // the grayscale shade color, 
		            // we set the texture coordinate,
		            // and we draw the vertex.
float tex_x1=(float)(x)/(float)MAP_X;
	float tex_y1=(float)(z)/(float)MAP_Z;
		            // draw vertex 0
		        	//gl.glColor3f(255,0,0);
		        	//if((x==0)&(z==0))
		        	//gl.glTexCoord2f(0.0f, 0.0f);
		        	gl.glTexCoord2f(tex_x1,tex_y1 );

		        	
		        	gl.glVertex3f(terrain[x][z][0], 
		                       terrain[x][z][1], terrain[x][z][2]);

		            // draw vertex 1
		        	//if((x==MAP_X-1)&(z==0))
		        	//gl.glTexCoord2f(1.0f, 0.0f);
		        	
		        	float tex_x2=(float)(x+1)/(float)MAP_X;
		        	float tex_y2=(float)(z)/(float)MAP_Z;
		        	gl.glTexCoord2f(tex_x2,tex_y2 );
		        	//gl.glColor3f(255,0,0);
		        	gl.glVertex3f(terrain[x+1][z][0], terrain[x+1][z][1], 
		                       terrain[x+1][z][2]);

		            // draw vertex 2

		        	
		        	//if((x==0)&(z==MAP_Z-1))			        	
		        	//gl.glTexCoord2f(0.0f, 1.0f);
		        	float tex_x3=(float)(x)/(float)MAP_X;
		        	float tex_y3=(float)(z+1)/(float)MAP_Z;
		        	
		        	gl.glTexCoord2f(tex_x3, tex_y3);
		        	
		        	//gl.glColor3f(255,0,0);
		        	gl.glVertex3f(terrain[x][z+1][0], terrain[x][z+1][1], 
		                       terrain[x][z+1][2]);

		            // draw vertex 3
		        	//gl.glColor3f(255,0,0);
		        	//if((x==MAP_X-1)&(z==MAP_Z-1))	
		        	//gl.glTexCoord2f(1.0f, 1.0f);
		        	float tex_x4=(float)(x+1)/(float)MAP_X;
		        	float tex_y4=(float)(z+1)/(float)MAP_Z;
		        	
		        	gl.glTexCoord2f(tex_x4, tex_y4);
		        	
		        	gl.glVertex3f(terrain[x+1][z+1][0], 
		                       terrain[x+1][z+1][1], 
		                       terrain[x+1][z+1][2]);
		        }
		        gl.glEnd();
		    }
			
		gl.glPopMatrix();
	//	gl.glUseProgram(0);
		
	}
	
}
