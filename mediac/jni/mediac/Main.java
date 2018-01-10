/**
 * Created by Papillon on Jan 4, 2018.
 */

package mediac;


public class Main {
	public static void main(String args[]) {		
		int ret;
        long handle[] = new long[1];
        		
        handle[0] = 0;
		ret = MediacHelper.MEDIAC_init(0, handle);
		System.out.println("MEDIAC_init returned " + ret);
        System.out.println("handle is " + handle[0]);
	}
}
