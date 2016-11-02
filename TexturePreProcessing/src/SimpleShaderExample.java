
import com.sun.opengl.util.Animator;


import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLCanvas;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.glu.GLU;
import javax.swing.*;

import shaderUtilities.ShaderUtilities;
import virtualTexturingEntities.Info;
import virtualTexturingEntities.NeedBuffer;
import virtualTexturingEntities.PageCache;
import virtualTexturingEntities.PageTable;
import virtualTexturingEntities.Util;

import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.IOException;

public class SimpleShaderExample extends GLCanvas implements GLEventListener {

	 private int[] textures = new int[1];  // Storage For Our Font Texture
	 
	private float dist=-5f;
	
   private boolean updateUniformVars = true;
   //private int vertexShaderProgram;
   
   private final Animator animator = new Animator(this);
   
   public SimpleShaderExample() {
      addGLEventListener(this);
        animator.start();
    }

   
   
    public void init(GLAutoDrawable drawable) {
    	  GL gl = drawable.getGL();
    	 try {
    		
    	           
    	        
    	
    	 } catch (Exception e) {
             e.printStackTrace();
          }
    	
    	 
         gl.glEnable(GL.GL_DEPTH_TEST);                 // Enables Depth Testing
         gl.glBlendFunc(GL.GL_SRC_ALPHA, GL.GL_ONE);    // Select The Type Of Blending
         gl.glDepthFunc(GL.GL_LEQUAL);                  // The Type Of Depth Test To Do
         
         // Really Nice Perspective Calculations
   //      gl.glHint(GL.GL_PERSPECTIVE_CORRECTION_HINT, GL.GL_NICEST);  
       //  gl.glEnable(GL.GL_TEXTURE_2D);      // Enable 2D Texture Mapping
    	 
    	 

        // Enable VSync
        gl.setSwapInterval(1);
      gl.glShadeModel(GL.GL_FLAT);
      try {
    	  Util.loadGLTextures(gl,textures);
         ShaderUtilities.attachShaders(gl);
      } catch (Exception e) {
         e.printStackTrace();
      }
   }

   
   
   public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
        GL gl = drawable.getGL();
        GLU glu = new GLU();

        if (height <= 0) { // avoid a divide by zero error!

            height = 1;
        }
        final float h = (float) width / (float) height;
  
        
        // Reset The Current Viewport And Perspective Transformation
        gl.glViewport(0, 0, width, height);                       
        gl.glMatrixMode(GL.GL_PROJECTION);    // Select The Projection Matrix
        gl.glLoadIdentity();                  // Reset The Projection Matrix
        
        // Calculate The Aspect Ratio Of The Window
        glu.gluPerspective(45.0f, (float) width / (float) height, 0.1f, 100.0f);  
        gl.glMatrixMode(GL.GL_MODELVIEW);    // Select The Modelview Matrix
        gl.glLoadIdentity();                 // Reset The ModalView Matrix
        
    }



    public void display(GLAutoDrawable drawable) {
        GL gl = drawable.getGL();
        
        
        
    //    gl.glUseProgram(0);
      if (updateUniformVars){
         ShaderUtilities.updateUniformVars(gl);
      }
      
     gl.glActiveTexture(GL.GL_TEXTURE0);
      gl.glEnable(GL.GL_TEXTURE_2D);
     gl.glUseProgram(0);
      // Clear The Screen And The Depth Buffer
      gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);       
      gl.glLoadIdentity();  // Reset The View
      
        // Reset the current matrix to the "identity"
       // gl.glLoadIdentity();
        gl.glTranslatef(-2.0f, 0.0f,dist);  // Move Into The Screen 5 Units
        gl.glBindTexture(GL.GL_TEXTURE_2D, Info.PAGE_TABLE_TEXTURE);  
        drawQuad(gl);
        
        
        gl.glTranslatef(0.0f, -2.0f, 0.0f);  // Move Into The Screen 5 Units
        gl.glBindTexture(GL.GL_TEXTURE_2D, Info.PHYSICAL_TEXTURE);  
        drawQuad(gl);       
        //gl.glUseProgramObjectARB(0);
      // gl.glUseProgram(shaderprogram);
    
      //  gl.glLoadIdentity();  // Reset The View
        gl.glUseProgram(ShaderUtilities.shaderprogram);
       gl.glDisable(GL.GL_TEXTURE_2D);
       //gl.glTranslatef(-0.2f, -0.5f, 4.0f);  // Move Into The Screen 5 Units
       gl.glTranslatef(1.8f, +1.5f, 4.0f);  // Move Into The Screen 5 Units
        drawQuad(gl);
   
        // Flush all drawing operations to the graphics card
       // gl.glFlush();
        gl.glLoadIdentity();
if(dist<-2){
        	
        //	dist+= 0.01f;
        	//System.out.println("dist<-5 "+dist);
        }
    }

   
   private void drawQuad(GL gl){
	   
	   gl.glBegin(GL.GL_QUADS);       
       gl.glTexCoord2f(0.0f, 0.0f);
       gl.glVertex3d(0, 0, 0);
       gl.glTexCoord2f(0.0f, 1.0f);
       gl.glVertex3d(0, 1, 0);
       gl.glTexCoord2f(1.0f, 1.0f);
       gl.glVertex3d(1, 1, 0);
       gl.glTexCoord2f(1.0f, 0.0f);
       gl.glVertex3d(1, 0, 0);
       gl.glEnd();
   }
   
   public void displayChanged(GLAutoDrawable drawable, boolean modeChanged, boolean deviceChanged) {
    }

   public static void main(String[] args) {
      SwingUtilities.invokeLater(new Runnable() {
         public void run() {
            JFrame jf = new JFrame();
            jf.setSize(800,800);
            jf.getContentPane().setLayout(new BorderLayout());
            jf.getContentPane().add(new SimpleShaderExample(), BorderLayout.CENTER);
            jf.setVisible(true);
            jf.addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    System.exit(0);
                }
            });
            
         }
      });

   }
}