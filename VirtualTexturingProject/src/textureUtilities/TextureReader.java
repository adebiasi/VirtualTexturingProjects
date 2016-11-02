package textureUtilities;
import com.sun.opengl.util.BufferUtil;

import javax.imageio.ImageIO;

import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.awt.image.PixelGrabber;
import java.io.IOException;
import java.net.URL;
import java.nio.ByteBuffer;

/**
 * Image loading class that converts BufferedImages into a data
 * structure that can be easily passed to OpenGL.
 * @author Pepijn Van Eeckhoudt
 */
public class TextureReader {
    public static Texture readTexture(String filename) throws IOException {
        return readTexture(filename, false);
    }

    public static Texture readTexture(String filename, boolean storeAlphaChannel) throws IOException {
        BufferedImage bufferedImage;
        
        //System.out.println("readTexture");
        
        if (filename.endsWith(".bmp")) {
        	//System.out.println("bmp");
            bufferedImage = BitmapLoader.loadBitmap(filename);
        } else {
        	//System.out.println("non bmp");
            bufferedImage = readImage(filename);
        }
        return readPixels(bufferedImage, storeAlphaChannel);
        //return readTexture(filename);
    }

   
    
    private static BufferedImage readImage(String resourceName) throws IOException {
        return ImageIO.read(ResourceRetriever.getResourceAsStream(resourceName));
    }

    /////////////////////////////////////////////////////////////////////////////////////
    protected Texture loadTexture(String file)
    {
         ByteBuffer pixels = BufferUtil.newByteBuffer(1);
         BufferedImage bi  = null;
         
         int textureWidth = 0;
         int textureHeight = 0;
         
         try{
  // load the image from file
              URL url = this.getClass().getResource( file);
              BufferedImage image = ImageIO.read(url);
  //setup the variables
               textureWidth = image.getWidth();
               textureHeight = image.getHeight();
  // read the number of colour components reqquired for the image
              int numComponents = image.getColorModel().getNumComponents();
  // define an image type for the following BufferedImage appropriate for the number of colour components.
             int type= 0;

               if(numComponents == 3)
                  type =  BufferedImage.TYPE_3BYTE_BGR;
              else
                  type = BufferedImage.TYPE_4BYTE_ABGR;
  // create a BufferedImage of the appropriate type
               bi = new BufferedImage(textureWidth,textureHeight,type);
              Graphics2D g = bi.createGraphics();
              g.scale(1,-1);
              g.drawImage(image, 0, -textureHeight, null);
  // create an array with a length that is dependent on the number of colour componets.
              byte[] data = new byte[numComponents * textureWidth * textureHeight];     
              bi.getRaster().getDataElements(0, 0, textureWidth, textureHeight, data);     // copy image to array data
              pixels = BufferUtil.newByteBuffer(data.length);     // create ByteBuffer of appropriate size.
              pixels.put(data); // place array data in ByteBuffer
              pixels.rewind();

          } catch (IOException ex) {
          ex.printStackTrace();
          }

          return new Texture(pixels, textureWidth, textureHeight);
    }
///////////////////////////////////////////////////////////////////////////////////
    
    
    private static Texture readPixels(BufferedImage img, boolean storeAlphaChannel) {
        int[] packedPixels = new int[img.getWidth() * img.getHeight()];

        
         
        PixelGrabber pixelgrabber = new PixelGrabber(img, 0, 0, img.getWidth(), img.getHeight(), packedPixels, 0, img.getWidth());
        try {
            pixelgrabber.grabPixels();
        } catch (InterruptedException e) {
            throw new RuntimeException();
        }

        int bytesPerPixel = storeAlphaChannel ? 4 : 3;
        ByteBuffer unpackedPixels = BufferUtil.newByteBuffer(packedPixels.length * bytesPerPixel);

        //for (int row = 0; row < img.getHeight(); row++) {
        for (int row = img.getHeight() - 1; row >= 0; row--) {
           // for (int col = img.getWidth()-1; col >= 0; col--) {
            	for (int col = 0; col < img.getWidth(); col++) {
                int packedPixel = packedPixels[row * img.getWidth() + col];
                unpackedPixels.put((byte) ((packedPixel >> 16) & 0xFF));
                unpackedPixels.put((byte) ((packedPixel >> 8) & 0xFF));
                unpackedPixels.put((byte) ((packedPixel >> 0) & 0xFF));
                if (storeAlphaChannel) {
                    unpackedPixels.put((byte) ((packedPixel >> 24) & 0xFF));
                }
            }
        }
       
        
 /*       
        
        for(int y = 0; y < img.getHeight(); y++){
            for(int x = 0; x < img.getWidth(); x++){
                int packedPixel = packedPixels[y * img.getWidth() + x];
                unpackedPixels.put((byte) ((packedPixel >> 16) & 0xFF));     // Red component
                unpackedPixels.put((byte) ((packedPixel >> 8) & 0xFF));      // Green component
                unpackedPixels.put((byte) (packedPixel & 0xFF));               // Blue component
                if (storeAlphaChannel) {
                unpackedPixels.put((byte) ((packedPixel >> 24) & 0xFF));    // Alpha component. Only for RGBA
                }
                }
        }
        
*/
        unpackedPixels.flip();

        
        return new Texture(unpackedPixels, img.getWidth(), img.getHeight());
    }

    public static class Texture {
        private ByteBuffer pixels;
        private int width;
        private int height;

        public Texture(ByteBuffer pixels, int width, int height) {
            this.height = height;
            this.pixels = pixels;
            this.width = width;
        }

        public int getHeight() {
            return height;
        }

        public ByteBuffer getPixels() {
            return pixels;
        }

        public int getWidth() {
            return width;
        }
    }
}

