package preProcessing;

import javax.imageio.ImageIO; 
import java.awt.image.BufferedImage; 
import java.io.*; 
import java.awt.*; 

public class ImageSplitTest { 

	static int x_pixel=256;
	static int y_pixel=256;
	static int num_Levels=3;
	
	public static void main(String[] args) throws IOException { 

	
		File file = new File("Erebus_combined_polar-B652R1.bmp");	
//File file = new File("highResTexture.bmp"); // I have bear.jpg in my working directory 
FileInputStream fis = new FileInputStream(file); 
BufferedImage image = ImageIO.read(fis); //reading the image file 

int totalIndex=0;


for(int level=0;level<num_Levels;level++){
	new File("pages/level_"+level).mkdir();
	
int rows = (int)Math.pow(2, level); //You should decide the values for rows and cols variables 
int cols = (int)Math.pow(2, level); 
int chunks = rows * cols; 

System.out.println("num parti: "+chunks);

int chunkWidth = image.getWidth() / cols; // determines the chunk width and height 
int chunkHeight = image.getHeight() / rows; 
int count = 0; 
BufferedImage imgs[] = new BufferedImage[chunks]; //Image array to hold image chunks 
//for (int x = 0; x < rows; x++) { 
	for (int x = rows-1; x >= 0; x--) {
for (int y = 0; y < cols; y++) {
	//for (int y = cols-1; y >= 0; y--) {
//Initialize the image array with image chunks 
//imgs[count] = new BufferedImage(chunkWidth, chunkHeight, image.getType()); 
	imgs[count] = new BufferedImage(x_pixel, y_pixel, BufferedImage.TYPE_INT_RGB);
	
// draws the image chunk 
	
Graphics2D gr = imgs[count].createGraphics(); 
count++;
gr.drawImage(image, 0, 0, x_pixel, y_pixel, chunkWidth * y, chunkHeight * x, chunkWidth * y + chunkWidth, chunkHeight * x + chunkHeight, null); 
//gr.drawImage(image, 0, 0, chunkWidth, chunkHeight, chunkWidth * y, chunkHeight * x, chunkWidth * y + chunkWidth, chunkHeight * x + chunkHeight, null);

gr.dispose(); 


} 
} 
System.out.println("Splitting done"); 



//writing mini images into image files 
for (int i = 0; i < imgs.length; i++) { 
//ImageIO.write(imgs[i], "bmp", new File("pages/level_"+level+"/" + totalIndex + ".bmp")); 
//	ImageIO.write(imgs[i], "jpg", new File("pages/level_"+level+"/" + totalIndex + ".jpg"));
	ImageIO.write(imgs[i], "jpg", new File("pages/level_"+level+"/" + i + ".jpg"));
totalIndex++;
} 

}
System.out.println("Mini images created"); 
} 
} 