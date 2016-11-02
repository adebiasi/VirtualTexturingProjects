package virtualTexturingEntities;

public class Info {

	public static int PHYSICAL_TEXTURE;
	public static int PAGE_TABLE_TEXTURE;
	public static int MIPMAP_TEXTURE;
	//public static int SHADER_TEXTURE;
	//public static int WORLD_TEXTURE;
	//public static int PART_WORLD_TEXTURE;
	
	
	static public int h_mipmap_texture = 2^5;
	static public int w_mipmap_texture =2^5;
	
	static int num_x_Pixel =256;
	static int num_y_Pixel =256;
	static public int h_pageCache = 15;
	static public int w_pageCache =15;
	static public int h_pixel_pageCache = num_y_Pixel*h_pageCache;
	static public int w_pixel_pageCache =num_x_Pixel*w_pageCache;
	
	static public int pageTableSize=h_pageCache*w_pageCache;
	
	static public int numLevels=8;
	static public int lastLevel=numLevels-1;
	static int numPages=returnNumPagesInAllLevels(numLevels);
	
	
	//static int[] pagesPerLevel = {1,4,4*4,4*4*4,4*4*4*4,4*4*4*4*4,4*4*4*4*4*4,4*4*4*4*4*4*4};
	
//	public static int  returnCurrLevel(int absoluteIndex){
//		
//		return (int)(logOfBase(2, (absoluteIndex))/2);
//		
//	}
//	
public static  int[] returnRelativePageIndex(int absolutePageIndex){
		
	//System.out.println("--------------------------------");
	//System.out.println("abs: "+absolutePageIndex);
	int lev=0;
	int numPagesInAllLevels=0;
	for(int i =0;i<Info.numLevels+1;i++){
		
		numPagesInAllLevels=returnNumPagesInAllLevels(i);
		//System.out.println("returnNumPagesInAllLevels(i): "+numPagesInAllLevels);
		if(absolutePageIndex<numPagesInAllLevels){
			lev=i;
			break;
		}
	}
	
		
		
		//System.out.println("level= "+(lev-1));
		
		int pag=returnNumPagesInAllLevels(lev-1);
		
		//System.out.println("num pag prev levels: "+numPages);
		
		int rel_index=absolutePageIndex-pag;
		
		//System.out.println("rel coord: "+rel_index);
		
		int[] res={lev,rel_index};
		
		return res;
	}	
	

public static double logOfBase(int base, int num) {  
    return Math.log(num) / Math.log(base);  
} 
public static  int returnAbsolutePageIndex(int relativePageIndex,int level){
		
		int num=0;
		
		for(int currLevel=0;currLevel<(level);currLevel++){
			
			num=num+(int)(Math.pow(2, currLevel)*Math.pow(2, currLevel)); 
			
		}
				
		return num+relativePageIndex;
	}	
	
/*
public static  int[] returnRelativeCoordinatesInPageCache(int relativePageIndex){
	
	int x= (int)(relativePageIndex%w_pageCache);
	int y=	(int)(relativePageIndex/h_pageCache);
	
	int[] res = {x*num_x_Pixel,y*num_y_Pixel};
	
	return res;
}	
*/
public static  int[] returnRelativeCoordinatesFxFyInPageCache(int relativePageIndex){
	
	int x= (int)(relativePageIndex%w_pageCache);
	int y=	(int)(relativePageIndex/h_pageCache);
	
	int[] res = {x,y};
	
	if(y>=h_pageCache){
		System.out.println("----->"+relativePageIndex);
	}
	
	return res;
}	


public static  int[] returnRelativeCoordinatesFxFyInPagePageTable(int relativePageTableIndex,int level){
	
	int x= (int)(relativePageTableIndex%(Math.pow(2, level)));
	int y=	(int)(relativePageTableIndex/(Math.pow(2, level)));
	
	int[] res = {x,y};
	
	return res;
}	

public static  int[] returnRelativeCoordinatesPageIndex(int relativePageIndex,int level){
	
	int x= (int)(relativePageIndex%Math.pow(2,level ));
	int y=	(int)(relativePageIndex/	Math.pow(2,level ));
	
	int[] res = {x,y};
	
	return res;
}	
public static  int returnRelativePageIndex(int x,int y,int level){
	
	int res=(int)(x+(y*(Math.pow(2,level ))));
	
	return res;
}	

private static  int returnNumPagesInAllLevels(int numLevels){
		
		int num=0;
		
		for(int currLevel=0;currLevel<numLevels;currLevel++){
			
			num=num+(int)(Math.pow(2, currLevel)*Math.pow(2, currLevel)); 
			
		}
		return num;
	}
public static  int returnNumPagesInCurrentLevel(int currLevel){
	
	int num=0;
	
	
		
		num=(int)(Math.pow(2, currLevel)*Math.pow(2, currLevel)); 
		
	
	return num;
}	

}
