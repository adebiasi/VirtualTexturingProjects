package virtualTexturingEntities;

import java.nio.ByteBuffer;

import javax.media.opengl.GL;

import com.sun.opengl.util.BufferUtil;

public class PageTable {

	static Page[] pageTable= new Page[Info.numPages];
	static ByteBuffer pageTableTexture;
	
	public static void updatePageTable(){
		
		System.out.println("IN UPDATE TABLE");
		
		for(int i=1;i<Info.numPages;i++){
			Page p =pageTable[i];
//			System.out.println("-------------------------");
//			System.out.println("p.idPage: "+p.idPage);
//			System.out.println("p.fx: "+p.fx);
//			System.out.println("p.fy: "+p.fy);
//			System.out.println("p.level: "+p.level);
//			System.out.println("p.id_ParentPage: "+p.id_ParentPage);
//			System.out.println("p.isinChace: "+p.isInCache);
//		
			if((p.isInCache==false)){
				
				//System.out.println(p.idPage+" "+p.id_ParentPage);
				
				Page parent = pageTable[p.id_ParentPage];
				
			//	System.out.println("parent: "+parent.fx+" "+parent.fy+" "+parent.level);
				
				p.fx=parent.fx;
				p.fy=parent.fy;
				p.level=parent.level;
			}
		}
	}
	
	
	public static void readPageTable(){
		for(int i=0;i<Info.numPages;i++){
			Page p =pageTable[i];
			System.out.println("-------------------------");
				System.out.println("p.idPage: "+p.idPage);
				System.out.println("p.fx: "+p.fx);
				System.out.println("p.fy: "+p.fy);
				System.out.println("p.level: "+p.level);
				System.out.println("p.id_ParentPage: "+p.id_ParentPage);
				System.out.println("p.isinChace: "+p.isInCache);
			}
		}
	
	
	public static void initPageTable(){
		
		for(int i=0;i<Info.numPages;i++){
			
			Page p = new Page();
			if(i==0){
				p.isInCache=true;
				p.id_ParentPage=-1;
				p.idPage=i;
				p.level=0;
				int[] coord=Info.returnRelativeCoordinatesFxFyInPageCache(i);
				//int [] coord=Info.returnRelativeCoordinatesInPageCache(0);
				p.fx=coord[0];
				p.fy=coord[1];
			}
			else{
			p.isInCache=false;			
			p.idPage=i;
			//p.level=Info.returnRelativePageIndex(i)[0];
			int[] pageCacheCoord=Info.returnRelativeCoordinatesFxFyInPageCache(i);
			
			//p.fx=pageCacheCoord[0];
			//p.fy=pageCacheCoord[1];
			
			p.level=0;
			p.id_ParentPage=0;
			
			}
			pageTable[i]=p;
		}
		
		
		//udpade level
		int index=0;
		int relativeIndex=0;
		for(int i=0;i<Info.numLevels;i++){
			int num=(int)Info.returnNumPagesInCurrentLevel(i);
			//int num=(int)Math.pow(2, i);
			System.out.println("num: "+num);
			for(int j=0;j<num;j++){
				//System.out.println("index: "+index+" level: "+i);
				pageTable[index].level=i;
				pageTable[index].original_level=i;
				pageTable[index].relativeIndexPage=relativeIndex;
				index++;
				relativeIndex++;
				if(index>pageTable.length){
					break;
				}
			}
			relativeIndex=0;
		}
		
		
		//udpade fx,fy
for(int i=1;i<Info.numPages;i++){
	
			Page p = pageTable[i];
			
			
			int[] fxy=Info.returnRelativeCoordinatesFxFyInPagePageTable(p.relativeIndexPage,p.level);
		//	System.out.println("relInd: "+p.idPage+"p.level: "+p.level+" xy: "+fxy[0]+" y: "+fxy[1]);
				pageTable[i].fx=fxy[0];
					pageTable[i].fy=fxy[1];		
					
					pageTable[i].original_fx=fxy[0];
					pageTable[i].original_fy=fxy[1];
					
		}
		
		
		
		for(int i=1;i<Info.numPages;i++){
			
			Page p = pageTable[i];
			int father_id=Info.returnRelativePageIndex(p.fx/2, p.fy/2, p.level-1);
			int prevPag=Info.returnAbsolutePageIndex(father_id, p.level-1);
			
			
				//	System.out.println("id: "+p.idPage+" level: "+p.level+" p.fx: "+p.fx+" p.fy: "+p.fy+" father_id: "+prevPag);
				pageTable[i].id_ParentPage=prevPag;
						
		}
		
	}
	
	public static void getPageTableTexture(GL gl, int texture_index){
		gl.glBindTexture(GL.GL_TEXTURE_2D, texture_index);

        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_NEAREST);
        gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_NEAREST);
    	
        
    	pageTableTexture = BufferUtil.newByteBuffer(Info.returnNumPagesInCurrentLevel(Info.lastLevel)*3); 
    	pageTableTexture.limit(pageTableTexture.capacity());
    	
    	
    	//System.out.println("pixels.capacity() "+pixels.capacity());
    	
    	int relIndex=0;
    	
    	int lastLevel=Info.lastLevel;
    	
    	
    	
    	for(int i=0; i<pageTableTexture.capacity(); i+=3) {
    		
    		
    		
    		int index=Info.returnAbsolutePageIndex(relIndex,lastLevel );
    		
    	//	System.out.println("abs index: "+index);
    		
    		//System.out.println("fx: "+pageTable[index].fx+" fy: "+pageTable[index].fy+" lev: "+pageTable[index].level);
    		
//    		byte r=(byte)pageTable[index].fx;
//    		byte g=(byte)pageTable[index].fy;
//    		byte b=(byte)pageTable[index].level;
//    		
    		int r=pageTable[index].fx;
    		int g=pageTable[index].fy;
    		int b=pageTable[index].level;

    		
//    		int r=1;
//    		int g=1;
//    		int b=1;
//    		
    		System.out.println("r: "+r+" g: "+g+"b: "+b);
    		
    		
//    		int r2=(int)(((float)r/5.0f)*255);
//    		int g2=(int)(((float)g/5.0f)*255);
//    		int b2=(int)(((float)b/5.0f)*255);
//    		
    		
    		int r2=(int)(((float)r/5.0f)*255);
    		int g2=(int)(((float)g/5.0f)*255);
    		int b2=(int)(((float)b/5.0f)*255);
    	
    		System.out.println(r2+" "+g2+" "+b2);
    		
    		pageTableTexture.put((byte)r2);
    		pageTableTexture.put((byte)g2);
    		pageTableTexture.put((byte)b2);
//    		
//    		pageTableTexture.asIntBuffer().put(r);
//    		pageTableTexture.asIntBuffer().put(g);
//    		pageTableTexture.asIntBuffer().put(b);
//    		
    		
    		relIndex++;
    		
        }
    	pageTableTexture.flip();
        
    		
    	gl.glBindTexture(GL.GL_TEXTURE_2D, texture_index);
    	gl.glTexImage2D(GL.GL_TEXTURE_2D,
                0,
                3,
                (int)Math.pow(2, lastLevel),
                (int)Math.pow(2, lastLevel),
                0,
                GL.GL_RGB,
                GL.GL_UNSIGNED_BYTE,
                pageTableTexture);
    	
   	
    	
	}
	
	public static void isInCache(int absIndex,int[] pageCacheCoord,int level){
		System.out.println("absolute index= "+ absIndex);
		pageTable[absIndex].fx=pageCacheCoord[0];
		pageTable[absIndex].fy=pageCacheCoord[1];
		pageTable[absIndex].level=level;
		pageTable[absIndex].isInCache=true;
	}
	
}
