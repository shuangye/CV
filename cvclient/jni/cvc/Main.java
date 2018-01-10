/*
$ cd jni/
$ javac cvc/*.java
$ LD_LIBRARY_PATH=./ java -classpath ./ cvc.Main
 */

package cvc;


public class Main {
	public static void main(String args[]) {		
		int ret;
        int maxImageLen = 1 << 20;  /* 1MB */
        int possibility[] = new int[1];
        byte face[] = new byte[maxImageLen];
        int len[] = new int[1];
        len[0] = maxImageLen;
		
		ret = CvcHelper.CVC_determineLivingFace(possibility, face, len);
		System.out.println("CVC_determineLivingFace returned " + ret);
        System.out.println("Possibility is " + possibility[0]);
	}
}
