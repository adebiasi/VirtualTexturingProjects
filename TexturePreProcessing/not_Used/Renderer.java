package not_Used;




import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.glu.GLU;

import java.io.IOException;
import java.nio.ByteBuffer;

import com.sun.opengl.util.BufferUtil;
import common.TextureReader;

class Renderer implements GLEventListener {
    private int[] textures = new int[2];  // Storage For Our Font Texture

    private GLU glu = new GLU();
  private float cnt1;
  private float dist=-10f;
    public Renderer() {
    }

    public void loadGLTextures(GL gl) throws IOException {
        
        String tileNames [] = 
            {"pages/level_0/0.jpg","megatexture3_levels.bmp"};


        gl.glGenTextures(2, textures, 0);

        
            TextureReader.Texture texture;
            texture = TextureReader.readTexture(tileNames[0]);
            //Create Nearest Filtered Texture
            gl.glBindTexture(GL.GL_TEXTURE_2D, textures[0]);

            gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR);
            gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR);

            gl.glTexImage2D(GL.GL_TEXTURE_2D,
                    0,
                    3,
                    texture.getWidth(),
                    texture.getHeight(),
                    0,
                    GL.GL_RGB,
                    GL.GL_UNSIGNED_BYTE,
                    texture.getPixels());

        
          
            
       
       
            TextureReader.Texture texture2 = TextureReader.readTexture(tileNames[1]);
    //Create Nearest Filtered Texture
    gl.glBindTexture(GL.GL_TEXTURE_2D, textures[1]);

    gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR);
    gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR);

    //System.out.println(texture2.getWidth()+" "+texture2.getHeight());
    
//    gl.glTexImage2D(GL.GL_TEXTURE_2D,
//            0,
//            3,
//            texture2.getWidth()/10,
//            texture2.getHeight()/10,
//            0,
//            GL.GL_RGB,
//            GL.GL_UNSIGNED_BYTE,
//            texture2.getPixels());

    gl.glTexImage2D(GL.GL_TEXTURE_2D,
          0,
          0,
          texture2.getWidth(),
          texture2.getHeight(),
          0,
          GL.GL_RGB,
          GL.GL_UNSIGNED_BYTE,
          texture2.getPixels());

    
}

   
   

    public void init(GLAutoDrawable glDrawable) {
        GL gl = glDrawable.getGL();
        try {
            loadGLTextures(gl);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        gl.glShadeModel(GL.GL_SMOOTH);                 // Enables Smooth Color Shading
        
        // This Will Clear The Background Color To Black
        gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);       
        
        // Enables Clearing Of The Depth Buffer
        gl.glClearDepth(1.0);                  
        
        gl.glEnable(GL.GL_DEPTH_TEST);                 // Enables Depth Testing
        gl.glBlendFunc(GL.GL_SRC_ALPHA, GL.GL_ONE);    // Select The Type Of Blending
        gl.glDepthFunc(GL.GL_LEQUAL);                  // The Type Of Depth Test To Do
        
        // Really Nice Perspective Calculations
        gl.glHint(GL.GL_PERSPECTIVE_CORRECTION_HINT, GL.GL_NICEST);  
        gl.glEnable(GL.GL_TEXTURE_2D);      // Enable 2D Texture Mapping
    }

    public void display(GLAutoDrawable glDrawable) {
        GL gl = glDrawable.getGL();
        
        // Clear The Screen And The Depth Buffer
        gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);       
        gl.glLoadIdentity();  // Reset The View
       
        
      
        
        
        gl.glLoadIdentity();  // Reset The View
        gl.glBindTexture(GL.GL_TEXTURE_2D, textures[0]);  
        gl.glTranslatef(-1.0f, -1.0f,dist);  // Move Into The Screen 5 Units
     
     // Rotate On The Z Axis 45 Degrees (Clockwise)
        gl.glRotatef(45.0f, 0.0f, 0.0f, 1.0f);  
        
        // Rotate On The X & Y Axis By cnt1 (Left To Right)
        gl.glRotatef(cnt1 * 30.0f, 1.0f, 1.0f, 0.0f);  
        gl.glDisable(GL.GL_BLEND);          // Disable Blending Before We Draw In 3D
        gl.glColor3f(1.0f, 1.0f, 1.0f);     // Bright White
        gl.glBegin(GL.GL_QUADS);            // Draw Our First Texture Mapped Quad
        gl.glTexCoord2d(0.0f, 0.0f);        // First Texture Coord
        gl.glVertex2f(-1.0f, 1.0f);         // First Vertex
        gl.glTexCoord2d(1.0f, 0.0f);        // Second Texture Coord
        gl.glVertex2f(1.0f, 1.0f);          // Second Vertex
        gl.glTexCoord2d(1.0f, 1.0f);        // Third Texture Coord
        gl.glVertex2f(1.0f, -1.0f);         // Third Vertex
        gl.glTexCoord2d(0.0f, 1.0f);        // Fourth Texture Coord
        gl.glVertex2f(-1.0f, -1.0f);        // Fourth Vertex
        gl.glEnd();                         // Done Drawing The First Quad
        
        gl.glBindTexture(GL.GL_TEXTURE_2D, textures[0]);  
        gl.glTranslatef(-2.0f, -2.0f, 0.0f);  // Move Into The Screen 5 Units
        // Rotate On The X & Y Axis By 90 Degrees (Left To Right)
        gl.glRotatef(90.0f, 1.0f, 1.0f, 0.0f);  
        gl.glBegin(GL.GL_QUADS);            // Draw Our Second Texture Mapped Quad
        gl.glTexCoord2d(0.0f, 0.0f);        // First Texture Coord
        gl.glVertex2f(-1.0f, 1.0f);         // First Vertex
        gl.glTexCoord2d(1.0f, 0.0f);        // Second Texture Coord
        gl.glVertex2f(1.0f, 1.0f);          // Second Vertex
        gl.glTexCoord2d(1.0f, 1.0f);        // Third Texture Coord
        gl.glVertex2f(1.0f, -1.0f);         // Third Vertex
        gl.glTexCoord2d(0.0f, 1.0f);        // Fourth Texture Coord
        gl.glVertex2f(-1.0f, -1.0f);        // Fourth Vertex
        gl.glEnd();                         // Done Drawing Our Second Quad
        gl.glEnable(GL.GL_BLEND);           // Enable Blending

        
        gl.glBindTexture(GL.GL_TEXTURE_2D, textures[1]);  
        gl.glTranslatef(4.0f, 4.0f, 0.0f);  // Move Into The Screen 5 Units
        // Rotate On The X & Y Axis By 90 Degrees (Left To Right)
        gl.glRotatef(90.0f, 1.0f, 1.0f, 0.0f);  
        gl.glBegin(GL.GL_QUADS);            // Draw Our Second Texture Mapped Quad
        gl.glTexCoord2d(0.0f, 0.0f);        // First Texture Coord
        gl.glVertex2f(-1.0f, 1.0f);         // First Vertex
        gl.glTexCoord2d(1.0f, 0.0f);        // Second Texture Coord
        gl.glVertex2f(1.0f, 1.0f);          // Second Vertex
        gl.glTexCoord2d(1.0f, 1.0f);        // Third Texture Coord
        gl.glVertex2f(1.0f, -1.0f);         // Third Vertex
        gl.glTexCoord2d(0.0f, 1.0f);        // Fourth Texture Coord
        gl.glVertex2f(-1.0f, -1.0f);        // Fourth Vertex
        gl.glEnd();                         // Done Drawing Our Second Quad
        gl.glEnable(GL.GL_BLEND);           // Enable Blending
        
        gl.glLoadIdentity();                // Reset The View
        
        cnt1 += 0.01f;      // Increase The First Counter
  
        if(dist<-5){
        	
        	dist+= 0.01f;
        	System.out.println("dist<-5 "+dist);
        }
    }

    public void reshape(GLAutoDrawable glDrawable, int x, int y, int w, int h) {
        if (h == 0) h = 1;
        GL gl = glDrawable.getGL();
        
        // Reset The Current Viewport And Perspective Transformation
        gl.glViewport(0, 0, w, h);                       
        gl.glMatrixMode(GL.GL_PROJECTION);    // Select The Projection Matrix
        gl.glLoadIdentity();                  // Reset The Projection Matrix
        
        // Calculate The Aspect Ratio Of The Window
        glu.gluPerspective(45.0f, (float) w / (float) h, 0.1f, 100.0f);  
        gl.glMatrixMode(GL.GL_MODELVIEW);    // Select The Modelview Matrix
        gl.glLoadIdentity();                 // Reset The ModalView Matrix
    }

    public void displayChanged(GLAutoDrawable glDrawable, boolean b, boolean b1) {
    }
}