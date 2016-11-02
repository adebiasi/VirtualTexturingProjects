package main;
import java.awt.Color;
import java.awt.Font;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;

import javax.media.opengl.*;
import javax.media.opengl.glu.*;

import objects.Character;
import objects.Pacman;
import objects.PageTableTexture;
import objects.PhysicalTexture;
import objects.Terrain;
import objects.TestPicture;
import objects.TestTexture;
import shaderUtilities.ShaderUtilities;
import threads.DetectFrames;
import virtualTexturingEntities.Info;
import virtualTexturingEntities.NeedBuffer;
import virtualTexturingEntities.PageCache;
import virtualTexturingEntities.PageTable;
import virtualTexturingEntities.Util;

import com.sun.corba.se.spi.copyobject.CopyobjectDefaults;
import com.sun.opengl.util.BufferUtil;
import com.sun.opengl.util.GLUT;
import com.sun.opengl.util.j2d.TextRenderer;


public class Graphics implements GLEventListener, KeyListener 
{
	
	// Thread updatePhysicalTexture;
    // Runnable runnable ;
    public  static boolean theadIsRunning = false;
    public  static boolean retrieveReadbackBuffer = false;
	
    public  static boolean clearPhysicalTexture=false;
    
	 static public int[] textures = new int[5];  // Storage For Our Font Texture
	 private boolean updateUniformVars = true;
	// TextRenderer renderer;
	 
	// private int floorWidth=Info.w_pixel_pageCache, floorHeight=Info.h_pixel_pageCache;
	// private int floorWidth=1000, floorHeight=1000;
	 private int floorWidth=1000, floorHeight=1000;
	    int[] frameBufferID = new int[1];
	    int[] depthRenderBufferID = new int[1];
	  public static   ByteBuffer readback_pixels;
	 
    private GLU glu;
    private GLUT glut;
    private int width, height;
    
  //  private int width2d=200, height2d=200,  
    int xdim2dTexture=floorWidth;
    int ydim2dTexture=floorHeight;
  //  private ArrayList<Character> characters;
    
    // Keyboard input
    private boolean[] keys;
    
    // Camera variables
    private Camera camera;
    private float zoom = 10.0f;
    private float cameraXPosition = 0.0f;
    private float cameraYPosition = 0.0f;
    private float cameraZPosition = 10.0f;
    
    private float cameraLXPosition = cameraXPosition;
    private float cameraLYPosition = cameraYPosition;
    private float cameraLZPosition = cameraZPosition - zoom;
    
    //private Pacman pacman;
    //private Pacman missPacman;
    private TestPicture picture;
    private Terrain terrain;
    //private TestPicture picture2;
    private PhysicalTexture physicalTexture2d;
    private PageTableTexture pageTableTexture;
  //  private TestTexture testTexture;
      
    public Graphics()
    {  
    	// boolean array for keyboard input
        keys = new boolean[256];
        
    	// Initialize the user camera
    	camera = new Camera();
        //camera.yawLeft(2.5);
        //camera.pitchDown(0.3);
        camera.moveForward(-30);
        camera.look(10);
        
        
        camera.strafeRight(10.1);
        camera.moveForward(6.1);
        camera.look(10);
      //  characters = new ArrayList<Character>();
        
     //   pacman = new Pacman(0.0,0.0,0.0);
       // missPacman = new Pacman(6.0,3.0,8.0);
        picture = new TestPicture(0.0,0.0,12.0);
        terrain = new Terrain(-10.0,-15.0,10.0);
       // picture2 = new TestPicture(0.0,0.0,12.0);
        
       // characters.add(pacman);
        //characters.add(missPacman);
        
    }
    
    public void init(GLAutoDrawable drawable) 
    {
    	 
    	
    	
    	width = drawable.getWidth();
        height = drawable.getHeight();
        
        GL gl = drawable.getGL();
        glu = new GLU();
        glut = new GLUT();
        
        gl.setSwapInterval(0); 												// Refreshes screen at 60fps   
        
        float light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        float light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
        float light_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };

        /* light_position is NOT default value */
        float light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };


        gl.glLightfv(GL.GL_LIGHT0, GL.GL_AMBIENT, light_ambient, 0);
        gl.glLightfv(GL.GL_LIGHT0, GL.GL_DIFFUSE, light_diffuse, 0);
        gl.glLightfv(GL.GL_LIGHT0, GL.GL_SPECULAR, light_specular, 0);
        gl.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION, light_position, 0);
        
       
        gl.glEnable(GL.GL_LIGHTING);
        gl.glEnable(GL.GL_LIGHT0);
        
        gl.glShadeModel(GL.GL_SMOOTH);
        gl.glClearColor(0.0f,0.0f,0.0f,0.0f);
        gl.glClearDepth(1.0f);												// Depth Buffer Setup
    	gl.glEnable(GL.GL_DEPTH_TEST);										// Enables Depth Testing
    	gl.glDepthFunc(GL.GL_LEQUAL);										// The Type Of Depth Test To Do
    	gl.glHint(GL.GL_PERSPECTIVE_CORRECTION_HINT, GL.GL_NICEST);			// Really Nice Perspective Calculations
    	
    	glu.gluPerspective(70.0, (float)width/(float)height, 1, 50);
        glu.gluLookAt(camera.getXPos(), camera.getYPos() , camera.getZPos(), camera.getXLPos(), camera.getYLPos(), camera.getZLPos(), 0.0, 1.0, 0.0);
        

        
        gl.glShadeModel(GL.GL_FLAT);
        try {
        	Util.loadGLTextures(gl,textures);
        	
        	renderShadowsToTexture(gl);
        	
           ShaderUtilities.attachShaders(gl);
           ShaderUtilities.attachShaders2(gl);
        } catch (Exception e) {
           e.printStackTrace();
        }
        
        // renderer = new TextRenderer(new Font("SansSerif", Font.BOLD, 36));
        physicalTexture2d= new PhysicalTexture(0.0,200.0);
        pageTableTexture= new PageTableTexture(0.0,0.0);
       // testTexture= new TestTexture(200.0,200.0);
        
        
        Runnable runnable = new DetectFrames();
        //Runnable runnable = new CopyOfUpdatePhysicalTexture(gl);
        Thread tr = new Thread(runnable);
        //tr.start();
    }
    private void renderShadowsToTexture(GL gl) {
    	
    	
 	
    	//init readback buffer
    	readback_pixels = BufferUtil.newByteBuffer(floorWidth*floorHeight*4);
    	//readback_pixels = BufferUtil.newByteBuffer(floorWidth*floorHeight*3);
    	//
for(int i=0; i<readback_pixels.capacity(); i+=(4)) {
//for(int i=0; i<readback_pixels.capacity(); i+=(3)) {
		
		byte x_coord= 127;
		byte y_coord= 0;
		byte level= 0;
		
		readback_pixels.put((byte)127);
		readback_pixels.put((byte)0);
		readback_pixels.put((byte)127);
		readback_pixels.put((byte)0);
		
}    	
readback_pixels.flip();
    	//GENERO TEXTURE

  	gl.glGenTextures(4, textures, 0);
        gl.glBindTexture(GL.GL_TEXTURE_2D, textures[3]);
         gl.glTexImage2D(GL.GL_TEXTURE_2D, 0, GL.GL_RGBA, floorWidth, floorHeight,0, GL.GL_RGBA, GL.GL_FLOAT, null);
          gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_NEAREST);
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_NEAREST);        
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_S, GL.GL_CLAMP);
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_T, GL.GL_CLAMP);
        

        
        int tex1=textures[3];
        int tex2=textures[4];
        
System.out.println("tex1 "+tex1);        
System.out.println("tex2 "+tex2); 
System.out.println(": "+textures[1]);

        int errorCode = gl.glGetError();
        if(errorCode!=0){
        String errorStr = glu.gluErrorString( errorCode );
        System.out.println("3...error: "+ errorStr );
        System.out.println("3....errorCode: "+ errorCode );
        }
        //GENERO FRAMEBUFFER
        gl.glGenFramebuffersEXT(1, frameBufferID, 0);
     
              
        gl.glBindFramebufferEXT(GL.GL_FRAMEBUFFER_EXT, frameBufferID[0]);
         errorCode = gl.glGetError();
       
         errorCode = gl.glGetError();
         if(errorCode!=0){
         String errorStr = glu.gluErrorString( errorCode );
         System.out.println("23...error: "+ errorStr );
         System.out.println("23....errorCode: "+ errorCode );
         }
         
          
          gl.glFramebufferTexture2DEXT(GL.GL_FRAMEBUFFER_EXT, GL.GL_COLOR_ATTACHMENT0_EXT,GL.GL_TEXTURE_2D, tex1, 0);

        
        if(gl.glCheckFramebufferStatusEXT(GL.GL_FRAMEBUFFER_EXT) == GL.GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            System.out.println("[Viewer] GL_FRAMEBUFFER_COMPLETE!!");
        }
        else
            System.out.println("..error");
       
       
        gl.glBindFramebufferEXT(GL.GL_FRAMEBUFFER_EXT, 0);
      
       
        
    }
    
    static public boolean   firstTime=false;
    
    public void display(GLAutoDrawable drawable) 
    {
    	
    	//System.out.println("aaaaaaaaa");
        GL gl = drawable.getGL();
       // 
        
        
        int errorCode = gl.glGetError();
        if(errorCode!=0){
        String errorStr = glu.gluErrorString( errorCode );
        System.out.println("error: "+ errorStr );
        System.out.println("errorCode: "+ errorCode );
        }
        if (updateUniformVars){
            ShaderUtilities.updateUniformVars(gl);
         }
        
       
          //width = drawable.getWidth();
          //height = drawable.getHeight();
        
         
        
        // updatePhysicalTexture.run();
          
      //  updatePhysicalTextureThreadCheck(gl);
        
        
        keyboardChecks( gl);													// Responds to keyboard input
        
        // Clear The Screen And The Depth Buffer
        gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);       

        draw2dTextures(drawable);
         
   //     if(!theadIsRunning){
      //  System.out.println("stò scrivendo su readback_pixels");
        //	System.out.println("check thread is running...no");
        	
         
            
         
             	
     //       	retrieveReadbackBuffer=true;
            	
            	synchronized(Graphics.readback_pixels){
            	
            gl.glBindFramebufferEXT(GL.GL_FRAMEBUFFER_EXT, frameBufferID[0]);	   
            gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);  
       	 gl.glUseProgram(ShaderUtilities.shaderprogram2);
    	 gl.glBindTexture(GL.GL_TEXTURE_2D, Info.MIPMAP_TEXTURE);
 		
            draw3dSceneB(drawable);												// Draw the scene
        	gl.glReadPixels(0, 0, floorWidth, floorHeight, GL.GL_RGBA,GL.GL_UNSIGNED_BYTE, readback_pixels);
            //gl.glReadPixels(0, 0, floorWidth, floorHeight, GL.GL_RGB,GL.GL_UNSIGNED_BYTE, readback_pixels);
            
  		  errorCode = gl.glGetError();
  	        if(errorCode!=0){
  	        String errorStr = glu.gluErrorString( errorCode );
  	        System.out.println("6...error: "+ errorStr );
  	        System.out.println("6....errorCode: "+ errorCode );
  	        }
  	        
     	gl.glBindFramebufferEXT(GL.GL_FRAMEBUFFER_EXT, 0);  
     //	retrieveReadbackBuffer=false;
   //  	System.out.println("ho finito di scrivere su readback_pixels");
        
     //       }
        //////////////////draw3dScene2(drawable);		
        }
     	
        draw3dScene(drawable);												// Draw the scene
           
        
        //keyboardChecks( gl);													// Responds to keyboard input
     	
        try{
        	if(PageCache.downloadedFrames.size()>0){
        	//	System.out.println("I have new : "+PageCache.downloadedFrames.size()+" pages");
        	}
        	
    	PageCache.insertLoadedFrames(NeedBuffer.needBuffer,gl,Info.PHYSICAL_TEXTURE);
    	PageTable.getPageTableTexture(gl, Info.PAGE_TABLE_TEXTURE);
        }catch(Exception e){
        	e.printStackTrace();
        }

     // Flush all drawing operations to the graphics card
        gl.glFlush();

    
    }
/*
    static private void  updatePhysicalTextureThreadCheck(GL gl){
    	
    	if(clearPhysicalTexture){
    		try{
    		   PageCache.createPageCache(gl,Info.PHYSICAL_TEXTURE);
    		   PageCache.bindPageCacheTexture(gl,Info.PHYSICAL_TEXTURE);
    	       PageCache.insertFirstPage(gl,Info.PHYSICAL_TEXTURE);
    	       PageTable.getPageTableTexture(gl, Info.PAGE_TABLE_TEXTURE);
    	       clearPhysicalTexture=false;
    		}catch(Exception e){
    			e.printStackTrace();
    		}
    	}
    	
    }
  */  
    static public void updateNeedBuffer(){
    	
    	
    	ByteBuffer newneedBuffer =readBackBufferToNeedBuffer();
    	NeedBuffer.assignNeedBuffer(newneedBuffer);
    	
    }
    
    private static  ByteBuffer readBackBufferToNeedBuffer(){
    	 ByteBuffer pixels2 = readback_pixels;
     	
     	ByteBuffer newneedBuffer = BufferUtil.newByteBuffer(pixels2.capacity()/4*3);
    	 //ByteBuffer newneedBuffer = BufferUtil.newByteBuffer(pixels2.capacity()/3*3);
     	int maxlev=0;
     	int minlev=10000;
     	while (pixels2.hasRemaining()){
     		
     		byte r=pixels2.get();
     		byte g=pixels2.get();
     		byte b=pixels2.get();
     		byte a=pixels2.get();
     		//byte b=1;
     		
     		
     		//if(b!=0){
     		if(true){
     		
     			if(maxlev<b){
         			maxlev=b;
         		}
         		
         		if(minlev>b){
         			minlev=b;
         		}
     			
     		newneedBuffer.put(r);
     		newneedBuffer.put(g);
     		newneedBuffer.put(b);
     		


     	      
     		}
     		/*
     		else{
     			System.out.println(readback_pixels.position() + " (" + r+","+g+") lev: "+b);
     		}
     		*/
    	  }
     	System.out.println("maxlev "+maxlev);
    	System.out.println("minlev "+minlev);
    	pixels2.rewind();
    	
    	
    	newneedBuffer.rewind();
    	
    	return newneedBuffer;
    }
    
    private void draw2dTextures(GLAutoDrawable drawable){
    	 GL gl = drawable.getGL();
					// Select The Projection Matrix
        
       	
        gl.glViewport(0, 0, width, height);		
        gl.glMatrixMode(GL.GL_PROJECTION);				
        gl.glLoadIdentity();	        
        glu.gluOrtho2D (0.0, width, 0.0, height);  // define drawing area

        
       
        
        physicalTexture2d.draw(drawable, glut);
        pageTableTexture.draw(drawable, glut);
        
       // testTexture.draw(drawable, glut);
       
        
    
		
        gl.glDisable(GL.GL_TEXTURE_2D);
        
        
        
        /*
        gl.glWindowPos2i( 0, 400 );	      
		gl.glPixelZoom(200f /  floorWidth, 200f /  floorHeight);
		gl.glDrawPixels(floorWidth, floorHeight, GL.GL_RGBA, GL.GL_UNSIGNED_BYTE, readback_pixels);
      */

		         

		          
		         
		      	
		  
	
		
	
    }
    
	public void reshape(GLAutoDrawable drawable, int x, int y, int w2, int h2) 
    {
		GL gl = drawable.getGL();
        
        w2 = drawable.getWidth();
        h2 = drawable.getHeight();
        
        gl.glMatrixMode(GL.GL_MODELVIEW);
        gl.glLoadIdentity();
        
        // perspective view
        gl.glViewport(10, 10, width-20, height-20);
        gl.glMatrixMode(GL.GL_PROJECTION);
        gl.glLoadIdentity();
        //glu.gluPerspective(45.0f,(float)width/(float)height,0.1f,100.0f);
        glu.gluPerspective(70.0, (float)width/(float)height, 1, 500);
        glu.gluLookAt(camera.getXPos(), camera.getYPos() , camera.getZPos(), camera.getXLPos(), camera.getYLPos(), camera.getZLPos(), 0.0, 1.0, 0.0);
    }

    public void displayChanged(GLAutoDrawable drawable, boolean modeChanged, boolean deviceChanged) 
    {      
    }
    
    private void draw3dScene(GLAutoDrawable drawable)
    {
    	GL gl = drawable.getGL();
    	 gl.glLoadIdentity();												// Reset The Projection Matrix
         gl.glViewport(0, 0, width, height);		
       
         gl.glMatrixMode(GL.GL_PROJECTION);
         gl.glLoadIdentity();
          glu.gluPerspective(70.0, (float)width/(float)height, 1, 200);
        
          
         	
          
          glu.gluLookAt(camera.getXPos(), camera.getYPos() , camera.getZPos(), 
         		camera.getXLPos(), camera.getYLPos(), camera.getZLPos(), 0.0, 1.0, 0.0);
         
         /*
         gl.glMatrixMode(GL.GL_PROJECTION);
         gl.glLoadIdentity();
         gl.glOrtho(0.0, 256.0, 0.0, 256.0, -1.0, 1.0); 
         */
         
          gl.glMatrixMode(GL.GL_MODELVIEW);									// Select The Modelview Matrix
          gl.glLoadIdentity();	
         										// Reset The Modelview Matrix     
         
          gl.glBlendFunc(GL.GL_SRC_ALPHA, GL.GL_ONE_MINUS_SRC_ALPHA);
          gl.glEnable(GL.GL_BLEND);
		//picture.drawNeedBuffer(drawable, texRender_32x32);
		//picture.draw(drawable, glut);
		terrain.draw_with_Shader_1(drawable, glut);
    	////////////////picture2.drawShader2(drawable, glut);
		gl.glDisable(GL.GL_BLEND);

    }
    
    
    private void draw3dSceneB(GLAutoDrawable drawable)
    {
    	GL gl = drawable.getGL();
    	 gl.glLoadIdentity();	
    	 
    	// width = drawable.getWidth();
        // height = drawable.getHeight();
    	 
    	 // Reset The Projection Matrix
         gl.glViewport(0, 0, width, height);		
       
         gl.glMatrixMode(GL.GL_PROJECTION);
         gl.glLoadIdentity();
          glu.gluPerspective(70.0, (float)width/(float)height, 1, 50);
        
          
         	
          
          glu.gluLookAt(camera.getXPos(), camera.getYPos() , camera.getZPos(), 
         		camera.getXLPos(), camera.getYLPos(), camera.getZLPos(), 0.0, 1.0, 0.0);
         
         /*
         gl.glMatrixMode(GL.GL_PROJECTION);
         gl.glLoadIdentity();
         gl.glOrtho(0.0, 256.0, 0.0, 256.0, -1.0, 1.0); 
         */
         
          gl.glMatrixMode(GL.GL_MODELVIEW);									// Select The Modelview Matrix
          gl.glLoadIdentity();	
         										// Reset The Modelview Matrix     
         
         

    	//picture2.drawShader2(drawable, glut);
          //picture.drawShader2(drawable, glut);
          terrain.draw_with_Shader_2(drawable, glut);

    }
    

	@Override
	public void keyPressed(KeyEvent key) 
    {
    	try
    	{
        char i = key.getKeyChar();
        keys[(int)i] = true;
    	}
    	catch(Exception e){};
        
        
    }
    
	@Override
    public void keyReleased(KeyEvent key) 
    {
    	try
    	{
        char i = key.getKeyChar();
        keys[(int)i] = false;
    	}
    	catch(Exception e){};
    }

	@Override
	public void keyTyped(KeyEvent arg0) 
	{
		// TODO Auto-generated method stub
		
	}

	
	
	public void keyboardChecks(GL gl)
    {
        if(keys['w'])
        {
            cameraZPosition -= 0.1;
            cameraLZPosition -= 0.1;
            
            camera.moveForward(0.1);
            //pacman.moveForward(0.1);
            camera.look(10);
            
        }
        
        if(keys['s'])
        {
            cameraZPosition += 0.1;
            cameraLZPosition += 0.1;
            
            camera.moveForward(-0.1);
            //pacman.moveForward(-0.1);
            camera.look(10);
        }

        if(keys['j'])
        {
            camera.pitchUp(0.05);
            camera.look(10);
        }
        
        if(keys['k'])
        {
            camera.pitchDown(0.05);
            camera.look(10);
        }
        
        if(keys['q'])
        {
            camera.yawLeft(0.01);
            camera.look(10);
        }
        
        if(keys['e'])
        {
            camera.yawRight(0.01);
            camera.look(10);
        }
        
        if(keys['a'])
        {
            camera.strafeLeft(0.1);
            camera.look(10);
        }
        
        if(keys['d'])
        {
            camera.strafeRight(0.1);
            camera.look(10);
        }
        if(keys['m'])
        {
        	
        
        	if(!theadIsRunning){
        		System.out.println("Attivo thread");
            Runnable runnable = new DetectFrames();
            //Runnable runnable = new CopyOfUpdatePhysicalTexture(gl);
            Thread tr = new Thread(runnable);
            tr.start();
            theadIsRunning=true;
        	}
        }
        
        if(keys['n'])
        {
        	try{
        		
        PageTable.initPageTable();
        PageTable.updatePageTable();
         //PageTable.readPageTable();   
	       //PageCache.createPageCache(gl,Info.PHYSICAL_TEXTURE);
        PageTable.getPageTableTexture(gl, Info.PAGE_TABLE_TEXTURE);
   	       
        PageCache.bindPageCacheTexture(gl,Info.PHYSICAL_TEXTURE);
	       System.out.println("4");
	       PageCache.insertFirstPage(gl,Info.PHYSICAL_TEXTURE);
	       System.out.println("5");
	       
	       }catch(Exception e){
	    	   e.printStackTrace();
	       }
        }
        
    }    
}


