import virtualTexturingEntities.Info;


public class testing {

	public static void main(String[] args) {
		
//		int level =2;
//		
//		int relativePageIndex=10;
//		
//		int abs=Info.returnAbsolutePageIndex(relativePageIndex, level);
//		
//		int[] xy=Info.returnRelativeCoordinatesPageIndex(relativePageIndex, level);
//		
//		int rel=Info.returnRelativePageIndex(xy[0],xy[1],level);
//		
//		System.out.println("relativePageIndex: "+relativePageIndex);
//		System.out.println("abs: "+abs);
//		System.out.println("xy: "+xy[0]+" "+xy[1]);
//		System.out.println("relativePageIndex: "+rel);
		
		
//		for(int i=0;i<30;i++){
//		Info.returnRelativePageIndex(i);
//		}
		
//		
//		String[] fsrc = new String[]{"uniform float mandel_x;\n" +
//	            "uniform float mandel_y;\n" +
//	            "uniform float mandel_width;\n" +
//	            "uniform float mandel_height; \n" +
//	            "uniform float mandel_iterations;\n" +
//	            "\n" +
//	            "float calculateMandelbrotIterations(float x, float y) {\n" +
//	            "\tfloat xx = 0.0;\n" +
//	            "    float yy = 0.0;\n" +
//	            "    float iter = 0.0;\n" +
//	            "    while (xx * xx + yy * yy <= 4.0 && iter<mandel_iterations) {\n" +
//	            "        float temp = xx*xx - yy*yy + x;\n" +
//	            "        yy = 2.0*xx*yy + y;\n" +
//	            "\n" +
//	            "        xx = temp;\n" +
//	            "\n" +
//	            "        iter ++;\n" +
//	            "    }\n" +
//	            "    return iter;\n" +
//	            "}\n" +
//	            "\n" +
//	            "vec4 getColor(float iterations) {\n" +
//	            "\tfloat oneThirdMandelIterations = mandel_iterations/3.0;\n" +
//	            "\tfloat green = iterations/oneThirdMandelIterations;\n" +
//	            "\tfloat blue = (iterations-1.3*oneThirdMandelIterations)/oneThirdMandelIterations;\n" +
//	            "\tfloat red = (iterations-2.2*oneThirdMandelIterations)/oneThirdMandelIterations;\n" +
//	            "\treturn vec4(red,green,blue,1.0);\n" +
//	            "}\n" +
//	            "\n" +
//	            "void main()\n" +
//	            "{\n" +
//	            "\tfloat x = mandel_x+gl_TexCoord[0].x*mandel_width;\n" +
//	            "\tfloat y = mandel_y+gl_TexCoord[0].y*mandel_height;\n" +
//	            "\tfloat iterations = calculateMandelbrotIterations(x,y);\n" +
//	            "\tgl_FragColor = getColor(iterations);\n" +
//	            "}"};
//		
//		 String[] vsrc = new String[]{"uniform float mandel_x;\n" +
//		            "uniform float mandel_y;\n" +
//		            "uniform float mandel_width;\n" +
//		            "uniform float mandel_height; \n" +
//		            "uniform float mandel_iterations;\n" +
//		            "\n" +
//		            "void main()\n" +
//		            "{\n" +
//		            "\tgl_TexCoord[0] = gl_MultiTexCoord0;\n" +
//		            "\tgl_Position = ftransform();\n" +
//		            "}"};
//	
//	
//	for(int i=0;i<vsrc.length;i++){
//		System.out.println(vsrc[i]);
//		}
		
		int i=2;
		byte b=(byte)i;
		System.out.println(i+" "+b);
		
		
}
}