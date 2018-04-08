import edu.princeton.cs.introcs.StdDraw;
import java.awt.*;
import java.util.Arrays;

public class Main {

    private static final int HEIGHT = 450,
                             WIDTH = 700;

    public static void main(String[] args) {

        boolean a[][] = new boolean[HEIGHT][WIDTH];

        int exit = 0;

        Arrays.fill(a[0], 2, 3, true ); // two gliders for testing
        Arrays.fill(a[1], 3, 4, true );
        Arrays.fill(a[2], 1, 4, true );

        Arrays.fill(a[10], 6, 7, true);
        Arrays.fill(a[9], 7, 8, true );
        Arrays.fill(a[8], 5, 8, true );

        initArr(a);
        initCanvas();
        draw(a);

        while(exit < 10000){
            a = logics(a);
            draw(a);
            exit++;
        }
    }

    static void initArr(boolean a[][]){
        int i, j;
        for(i=0; i<a.length; i++)
            for(j=0; j< a[i].length; j++)
                a[i][j] = Math.random() > 0.85;
    }

    static void initCanvas(){
        StdDraw.setCanvasSize(1400,950);
        StdDraw.enableDoubleBuffering();
        StdDraw.setXscale(0, 699);
        StdDraw.setYscale(0, 475);
        StdDraw.clear(Color.DARK_GRAY);
    }

    static void draw(boolean a[][]){
        int i, j;
        StdDraw.clear(Color.BLACK);
        StdDraw.setPenColor(Color.WHITE);
        for(i=0; i<a.length; i++) {
            for (j = 0; j < a[i].length; j++) {
                if (a[i][j])
                    StdDraw.filledCircle(j, i, 0.5);
            }
        }
        StdDraw.show();
    }

    static boolean[][] logics(boolean arr[][]){
        boolean tmp[][] = new boolean[HEIGHT][WIDTH];
        int i, j, i1, j1, live;

        for(i =0; i<arr.length; i++){
            for(j=0; j< arr[i].length; j++){  // for each cell
                live = 0;
                int a = 0, b = 0;
                for(i1 = 0; i1<3; i1++) { // find if a[i][j] has 2 (3) living neighbours, implementing 'torus'
                    switch(i1){
                        case 0:
                            a = (i-1 < 0) ? HEIGHT+i-1 : i-1; // left column
                            break;
                        case 1:                          // center
                            a = i;
                            break;
                        case 2:                          // right column
                            a = (i+1>= HEIGHT) ? 0 : i+1;
                            break;
                    }
                    for(j1 = 0; j1<3; j1++) {
                        switch(j1){
                            case 0:                      // bottom row
                                b = (j-1 < 0) ? WIDTH+j-1 : j-1;
                                break;
                            case 1:                      // center
                                b = j;
                                break;
                            case 2:                      // upper row
                                b = (j+1>= WIDTH) ? 0 : j+1;
                                break;
                        }
                        if (arr[a][b])
                            live++;
                    }
                }
                if(arr[i][j]){  // excluding a[i][j] itself
                    live--;
                }
                if((live == 2 && arr[i][j]) || live == 3){
                    tmp[i][j] = true;
                }else{
                    tmp[i][j] = false;
                }
            }
        }
        return tmp;
    }
}
