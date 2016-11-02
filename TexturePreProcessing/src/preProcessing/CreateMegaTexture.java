package preProcessing;

import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import javax.imageio.ImageIO;

public class CreateMegaTexture {
	static int x_pixel=256;
	static int y_pixel=256;
	
	static int x_pixel_megaTexture=256*2;
	static int y_pixel_megaTexture=256*2;
	
	static int numLevel=3;
	
	
	public static void main(String[] args) throws IOException { 

	
	
		//File file = new File("img0.bmp"); // I have bear.jpg in my working directory 
		//FileInputStream fis = new FileInputStream(file); 
		//BufferedImage image = ImageIO.read(fis); //reading the image file 

		int numMipMaps=returnNumMipmapsInAllLevels(numLevel);
		//System.out.println("numMipMaps: "+numMipMaps);

		int rows =calculate_Height_Megatexture(numMipMaps);
		int cols=rows;
		//System.out.println("num col & rows: "+rows+" "+cols);
		
		x_pixel_megaTexture=rows*x_pixel;
		y_pixel_megaTexture=cols*x_pixel;
		
		BufferedImage mipmaps[] = new BufferedImage[numMipMaps];
		int type=0;
		
		int currIndex=0;
for(int level=0;level<numLevel;level++){
	
	int numMipMapsInCurrentLevel=returnNumMipmapsInCurrentLevel(level);
	//System.out.println("numMipMapsInCurrentLevel: "+numMipMapsInCurrentLevel);
	
	for(int i=0;i<numMipMapsInCurrentLevel;i++){
		
		String currFile = "bipmap/img_"+level+"/" + currIndex + ".bmp";
		//System.out.println(currFile);
		File fileMipMap = new File(currFile); // I have bear.jpg in my working directory 
		FileInputStream fis = new FileInputStream(fileMipMap); 
		BufferedImage image = ImageIO.read(fis); //reading the image file 
		 type=image.getType();
		
		mipmaps[currIndex]=image;
		
		currIndex++;
	}
}
int chunks = rows * cols; 

//System.out.println("num parti: "+chunks);
/*
int chunkWidth = image.getWidth() / cols; // determines the chunk width and height 
int chunkHeight = image.getHeight() / rows; 
int count = 0; 
BufferedImage imgs[] = new BufferedImage[chunks]; //Image array to hold image chunks 
*/
BufferedImage megatexture = new BufferedImage(x_pixel_megaTexture, y_pixel_megaTexture, type);
Graphics2D gr = megatexture.createGraphics(); 


currIndex=0;
for (int x = 0; x < rows; x++) { 
for (int y = 0; y < cols; y++) { 
//Initialize the image array with image chunks 
//imgs[count] = new BufferedImage(chunkWidth, chunkHeight, image.getType()); 
//	imgs[count] = new BufferedImage(x_pixel*2, y_pixel*2, image.getType());
//	megatexture = new BufferedImage(x_pixel*2, y_pixel*2, image.getType());
// draws the image chunk 

	
	if(currIndex<numMipMaps){
		
	
gr.drawImage(
		mipmaps[currIndex],
		//image,
		x_pixel* x,x_pixel* y, 
		x_pixel* (x+1),x_pixel* (y+1), 
		0, 0, 
		x_pixel , y_pixel,
		null); 
//gr.drawImage(image, 0, 0, chunkWidth, chunkHeight, chunkWidth * y, chunkHeight * x, chunkWidth * y + chunkWidth, chunkHeight * x + chunkHeight, null);
currIndex++;

	}

} 


}

gr.dispose(); 

System.out.println("Splitting done"); 
//ImageIO.write(megatexture, "jpg", new File("megatexture"+numLevel +"_levels.jpg")); 
ImageIO.write(megatexture, "bmp", new File("megatexture"+numLevel +"_levels.bmp"));



System.out.println("MEGATEXTURE created"); 
} 
 
	private static  int returnNumMipmapsInAllLevels(int numLevels){
		
		int num=0;
		
		for(int currLevel=0;currLevel<numLevels;currLevel++){
			
			num=num+(int)(Math.pow(2, currLevel)*Math.pow(2, currLevel)); 
			
		}
		return num;
	}
	
private static  int returnNumMipmapsInCurrentLevel(int currLevel){
		
		int num=0;
		
		
			
			num=(int)(Math.pow(2, currLevel)*Math.pow(2, currLevel)); 
			
		
		return num;
	}
	
private static  int calculate_Height_Megatexture(int numMipmaps){		
		int num=0;		
double res=Math.sqrt(numMipmaps);
		return (int)res+1;
	}
	
}
