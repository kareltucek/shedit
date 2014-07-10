//define DEBUG                       //includes debug bindings
//define DEBUG_LOGGING              //enables logging to files
//define DEBUG_PARSELINELOOP
//define DEBUG_CURSOR               //For debug of mouse-selection; disables movement updates
#define DEBUG_REPAINT false

//#define DOUBLE_BUFFERED
#define FULL_WIDTH_PAINT
#define TAB_WIDTH 2
//define DRAW_POS
#define ALLOW_TABS

#define KEEP_VIS_TOP 4
#define KEEP_VIS_BOTTOM 4

#define MARGINS 50  //for horizontal adjustment

#define PARSEINADVANCE 1000
#define PARSEINONEGO 100


#define DEFONTSIZE 10

//define QUICKDRAW    //single, non-transparent pass vs double pass - should be much faster, but cuts off the rightmost pixel columns of some words
#define FORCELOOKAHEAD true

#define MAX_SCROLL_STEP 12
#define Y_OFF 0
#define X_OFF 2

#define FIXSCREENTOP

//NOTICE: what is not (un)set as default is NOT guaranteed to work
