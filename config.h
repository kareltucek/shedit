#ifdef _DEBUG
#define _DEBUG_LOGGING              //enables logging to files
//define _DEBUG_UNDO                 //test undo each time a change is made to buffer
//define _DEBUG_FORCE_SHORTCUTTS      //disables onkey_whatever events that may override the debug keys
//define _DEBUG_PARSELINELOOP
//define _DEBUG_CURSOR               //For debug of mouse-selection; disables movement updates
#endif

#define _DEBUG_REPAINT false

#ifndef _DEBUG
#define NDEBUG       //disables asserts (iff this header is included before the assert header)
#endif

#define DOUBLE_BUFFERED
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
