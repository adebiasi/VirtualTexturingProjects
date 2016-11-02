import virtualTexturingEntities.Info;


public class main {
public static void main(String[] args) {
	
	for(int i=0;i<100;i++){
	
	int[] pageCacheCoord=Info.returnRelativeCoordinatesFxFyInPageCache(i);
	//int[] pageCacheCoord=Info.returnRelativeCoordinatesFxFyInPageCache(ind_pageCache,level);
System.out.println(i+" "+pageCacheCoord[0]+" "+pageCacheCoord[1]);
	}
}
}
