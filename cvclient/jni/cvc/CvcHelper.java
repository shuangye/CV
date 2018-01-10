/**
 * Created by Papillon on Nov 16, 2017.
 */
 
package cvc; 


/*
 * @brief This class represents the client of the <b>cvd</b> (computer vision daemon) program. It provides interface to native code of computer vision algorithms.
 */
public class CvcHelper {
    static {
		try {
			System.loadLibrary("cvc");			
		}
		catch (UnsatisfiedLinkError e) {
			System.out.println("Failed to load library: " + e);		
			System.exit(-1);
		}
    }
    

    /*
     * @brief Init this module. It must be called before any other methods. 
     * @return 0 for success, and non-zero for failure.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_init(); 
     * @endcode
     */
    public static native int CVC_init();    

    /*
     * @brief Deinit this module.
     * @return 0 for success, and non-zero for failure.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_deinit();
     * @endcode
     */
    public static native int CVC_deinit();
    
    /*
     * @brief Asks the computer vision daemon to start a new calibration procedure.
     * @param patternW The count of corners of the pattern image in horizontal direction.
     * @param patternH The count of corners of the pattern image in vertical direction.
     * @return 0 for success, and non-zero for failure.
     * @note The pattern image is an image that contains a chessboard.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_calibratorInit(const int patternW, const int patternH);
     * @endcode
     */
    public static native int CVC_calibratorInit(int patternW, int patternH);
    
    /*
     * @brief Try to collect one pattern image during the calibration procedure.
     * @return 0 for if the chessboard pattern was detected in the captured image, and non-zero if not.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_calibratorCollectImage();
     * @endcode
     */
    public static native int CVC_calibratorCollectImage();
    
    /*
     * @brief End collecting pattern images and begin to do stereo calibration.
     * @return 0 for success, and non-zero for failure.
     * @note This method takes a long period. For 20 pairs iamges with 640x480 resolution on an Intel i5-6500 CPU, it takes about 3 minutes.
     *       Before the stereo calibration ends, this method will keep blocking.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_calibratorStereoCalibrate();
     * @endcode
     */
    public static native int CVC_calibratorStereoCalibrate();
    
    /*
     * @brief Get one face detection result from computer vision daemon.
     * @param id zero-based ID of cameras, from which to get the face detection result.
     * @param faceRegion detected face region. The origin is at the upper-left corder. The positive X points to the right, and the positive Y points the the bottom.
     * @param relativeW, relativeH These two arrays are both contain only one element. They specify he image size of the detected face region is relative to. 
     *        If the displayed image size is different from this size, the face region may need to be scaled accordingly.
     * @return 0 for success, and non-zero for failure.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_detectFace(const int id, OSA_Rect *pFace, OSA_Size *pRelativeSize);
     * @endcode
     */
    public static native int CVC_detectFace(int id, CvcRect faceRegion, int[] relativeW, int[] relativeH);
    
    /*
     * @brief Set the threshold of living face determination. Living face possibilities lower than this threshold will not be considered to be living. 
     *        The encoded face image will not be provided by the `CVC_determineLivingFace` method either.
     * @param threshold The threshold value. Valid range is [0, 100].
     * @return 0 for success, and non-zero for failure.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_setLivingFaceThreshold(const int threshold);
     * @endcode
     */
    public static native int CVC_setLivingFaceThreshold(int threshold);
    
	/*
     * @brief Get living face possibility and, if the possibility is no less than the threshold set by `CVC_setLivingFaceThreshold`,
     *        also copy the JPEG-encoded face region to the user provider buffer `face`.
     * @param possibility The living face possibility. This array contains only one element.
     * @param face The buffer to receive JPEG-encoded face region.
     * @param len An array contains only one element that specifies the length of the `face` buffer. 
     *        When calling this method, the caller shall fill it will the length (in bytes) of `face`. When returned, it is updated
     *        by the implementation to reflect the actual length (in bytes) of the JPEG-encoded face region.
     * @return 0 for success, and non-zero for failure.
     * @note If the living face possibility is less than the threshold specified by `CVC_setLivingFaceThreshold`, the encoded face region will not be copied.
     * @note The corresponding C prototype is:
     * @code{.c}
     * int CVC_determineLivingFace(int *pPossibility, void *pFace, size_t *pLen);
     * @endcode
     */
	public static native int CVC_determineLivingFace(int[] possibility, byte[] face, int[] len);    
}

