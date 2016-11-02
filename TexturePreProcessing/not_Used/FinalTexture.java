package entities;

public class FinalTexture {

	
	private void texelToPageTableIndex(int x,int y, int max_x, int max_y, int level){
		
		double delta_x=x/max_x;
		double delta_y=y/max_y;
		
		double x_PageCoord = delta_x*Math.pow(2, level);
		double y_PageCoord = delta_y*Math.pow(2, level);
		
		int relativePageIndex=Info.returnRelativePageIndex((int)x_PageCoord, (int)y_PageCoord, level);
		int absolutePageIndex=Info.returnAbsolutePageIndex(relativePageIndex, level);
		
		
		
	}
	
	
}
