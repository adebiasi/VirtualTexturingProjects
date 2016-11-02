package objects;
import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;

import com.sun.opengl.util.GLUT;


public abstract class  Character 
{
	private double xPos;
	private double yPos;
	private double zPos;
	
	private double pitch;
	private double yaw;
	
	public Character(double xPos, double yPos, double zPos)
	{
		this.xPos = xPos;
		this.yPos = yPos;
		this.zPos = zPos;
		
		this.pitch = 0;
		this.yaw = 0;
	}
	
	public void moveForward(double magnitude)
    {
        double xCurrent = this.xPos;
        double yCurrent = this.yPos;
        double zCurrent = this.zPos;
        
        // Spherical coordinates maths
        double xMovement = magnitude * Math.cos(pitch) * Math.cos(yaw); 
        double yMovement = magnitude * Math.sin(pitch);
        double zMovement = magnitude * Math.cos(pitch) * Math.sin(yaw);
              
        double xNew = xCurrent + xMovement;
        double yNew = yCurrent + yMovement;
        double zNew = zCurrent + zMovement;
        
        updatePosition(xNew, yNew, zNew);
    }
	
	public void updatePosition(double xPos, double yPos, double zPos)
    {
        this.xPos = xPos;
        this.yPos = yPos;
        this.zPos = zPos;
    }
	
	public void draw(GLAutoDrawable drawable, GLUT glut)
	{
		GL gl = drawable.getGL();
		
		gl.glPushMatrix();
			gl.glTranslated(xPos, yPos, zPos);
			glut.glutSolidSphere(2.0, 12, 12);
		gl.glPopMatrix();
	}
}
