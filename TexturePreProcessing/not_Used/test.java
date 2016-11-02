package not_Used;






/**
 * @author Pepijn Van Eeckhoudt
 */
public class test {
    public static void main(String[] args) {
        GLDisplay neheGLDisplay = GLDisplay.createGLDisplay("Test");
        Renderer renderer = new Renderer();
        neheGLDisplay.addGLEventListener(renderer);
        neheGLDisplay.start();
    }
}

