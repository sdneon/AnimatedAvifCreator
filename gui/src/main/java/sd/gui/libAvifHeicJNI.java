/*
 * Uses AVIF + HEIF decoders from JPEGView
 */
package sd.gui;

/**
 *
 * @author Neon
 */
public class libAvifHeicJNI {
    
    static {
        try {
            System.loadLibrary("avif");
        } catch (UnsatisfiedLinkError e) {
          System.err.println("Failed to load native avif library.\n" + e);
        }
        try {
            System.loadLibrary("heif");
        } catch (UnsatisfiedLinkError e) {
          System.err.println("Failed to load native heif library.\n" + e);
        }
        try {
            System.loadLibrary("libAvifHeic");
        } catch (UnsatisfiedLinkError e) {
          System.err.println("Failed to load native libAvifHeic library.\n" + e);
        }
    }

    public static final native int[] AvifHeicDecode(byte[] src, long size, int frameIndex, int[] width, int[] height, int[] numFrames, long[] frameIntervalMs);
    public static final native boolean AvifHeicWrite(java.io.OutputStream out, int[][] src, int width, int height, int numFrames, int quality, long[] frameIntervalMs);
}
