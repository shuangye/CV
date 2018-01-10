/**
 * Created by Papillon on Jan 3, 2018.
 */
 
package mediac; 


/*
 * @brief This class represents the client of the <b>media</b> program. It provides interface to native code for controling the underlying media device.
 */
public class MediacHelper {
    static {
		try {
			System.loadLibrary("mediac");			
		}
		catch (UnsatisfiedLinkError e) {
			System.out.println("Failed to load library: " + e);		
			System.exit(-1);
		}
    }

    public static native int MEDIAC_init(int clientId, long[] handle);

    public static native int MEDIAC_deinit(long handle);

    public static native int MEDIAC_open(long handle);

    public static native int MEDIAC_close(long handle);

    public static native int MEDIAC_setFormat(long handle, int w, int h);

    public static native int MEDIAC_setFrameRate(long handle, int frameRate);

    public static native int MEDIAC_startStreaming(long handle);

    public static native int MEDIAC_stopStreaming(long handle);
}
