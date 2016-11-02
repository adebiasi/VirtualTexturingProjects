package virtualTexturingEntities;

import java.nio.ByteBuffer;

public class DownloadedFrame {

	int level;
	//int[] pageCacheCoord;
	int absPageIndex;
	ByteBuffer loadedBuffer;
	
	
	public DownloadedFrame(int level,  int absPageIndex,
			ByteBuffer loadedBuffer) {
		super();
		this.level = level;
		//this.pageCacheCoord = pageCacheCoord;
		this.absPageIndex = absPageIndex;
		this.loadedBuffer = loadedBuffer;
		//System.out.println("istanzio frame: "+level+" "+absPageIndex);
		
	}
	
	
}
