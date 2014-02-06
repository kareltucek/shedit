//---------------------------------------------------------------------------

#ifndef cSQLSyntaxH
#define cSQLSyntaxH
//---------------------------------------------------------------------------
#include "config.h"
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <Controls.hpp>
#include <Clipbrd.hpp>
#include <time.h>
#include "uIter.h"
#include "uBuffer.h"
#include "uDrawer.h"
#include "uParser.h"
//---------------------------------------------------------------------------
/*
 *
 * 
 */
//---------------------------------------------------------------------------


#include "config.h"

namespace SHEdit
{
  LRESULT CALLBACK ProcessKeyCall(int code, WPARAM wParam, LPARAM lParam);

  typedef void __fastcall (__closure *TKeyPressEvent)(System::TObject* Sender, int &Key, bool &Captured);

  /** \mainpage
   * overrview
   * ------------------
   * TSQLEdit is a syntax highlight source code editing component.
   *
   * TSQLEdit consists of 4 main parts. This component class TSQLEdit and then Buffer, Parser and Drawer. Originally parser and drawer were designed to work in their own thread, but as it turned out, it was not possible due to the fact that vcl is not thread safe and thus text rendering was not possible, so merged everything to just one thread, and cut off a lot of heavy stuff for thread sync.
   * 
   * The data are stored in the buffer in a two-level linked list, which maintains a continuous text stream on one level, and newline list on the other one. Almost entire data structure is designed "lineary" - without andy binary tree indexes, mainly because of relatively high time complexity of keeping the binary tree up-to-date - that means, main approach, that amortizes time complexity to be constant (considering the size of buffer) (except for line movements) is to have an iterator that points to the current position of window, without having access to information about the rest of the data (there is just implemented a linear positioning system, that keeps track of current line number and offset, and that keeps all iterators valid and having the correct positioning info). Above the buffer works the component (actual TSQLEdit class), whose job is to process all user interaction, let the buffer to insert and remove text, handle undo and redo funcionality, and to manage painting of the screen. When updating the screen, component repaints just those parts of screen that are no longer valid (by lines - always entire line is reparsed/repainted). Lines that need to be repainted or reparsed are handed over to the Parser whose job is to parse the lines and those that are in visible part of window directly send further to drawer that draws them. Parser works "asynchronously" - can reparse every time just what it needs to reparse. 
   * 
   * Getting started
   * ---------------
   * To get this component working you need a vcl aware IDE (it is written and tested under C++Builder XE). The steps are to install the component package. Then before you can do anything practical, you need to programaticcaly hand over an instance of language specification to the component (A descendant of LanguageDefinition class, by (TODO) call). You can obtain the object by following manners:
   *
   * 1. Derive new descendant of the LanguageDefinition, and define it's dictionaries in it's body. (more info in documentation of the LanguageDefinition class)
   * 2. Create new isntance of the LanguageDefinition itself, and construct the lexer "from outside" using it's public methods
   * 3. (NOT YET IMPLEMENTED) Create specification using a configuration file and let the component load it.
   *
   * Last thing you might want to do is to go through published properties using your IDE's designer.
   *
   * Formatting
   * ---------
   * TSQLEdit provides 3 ways of formatting documents:
   * 1. The syntax highlight feature. For more information visit LanguageDefinition class documentation.
   * 2. A positionless markup that is hardlinked to the buffer structure, that is kept up-to date through Parser. It is quite efficient for large amounts of local formatting, but lacks responsivity regarding global formating changes. For more information see Iter::MarkupBegin and Iter::MarkupEnd methods.
   * 3. Iterator-handled global markup, that is established using system of binary search trees. It is efficient for markup changes over large areas and can quickly find current format at any point of buffer. Where it lacks is that Iterators has to be kept up to date throught editions on buffer (and amount of such iterators is thus somehow limited). For more information see Iter::IMarkupBegin and Iter::IMarkupEnd methods.
   *
   * Content of Documentation
   * ------------------------
   *  1. TSQLEdit class
   */

  /**
   * TSQLEdit - the component class
   * ------------------------------
   * This class represents the component itself. It's task is to process user input and command other objects what to do - information this class stores are Iterators pointing to the current line, cursor position and selection end, then information about scrolling deltas and mouse positioning information. It's members are on one hand overriden message/event handling procedures and on the other hand methods that are used to process these.
   *
   * Inner funcionality
   * -----------------
   * ### Keyboard capturing ###
   * Keyboard is trapped at 2 places - first is normal overriden winproc message for text input and the second is a windows hook which lets us intercept all keys we want, before they're passed to the program (that's neccessary to intercept input that would be otherwise consumed by vcl itself and would not be sent to component at all
   *
   * ### Mouse capturing ###
   * Mouse events are captured by 3 handlers - those are TSQLEdit::MouseDown TSQLEdit::MouseMove and TSQLEdit::MouseUp methods. On mouse down itrCursor is placed at correct position. If mouse continues to move more than specified trashold, text-selection mode is entered and itrCursorSecond is used to mark the other end of selected area.
   *  
   * ### Handling edits, parsing, and screen repainting ###
   *  All positioning is done using Iter class. When obtained a signal to do some editing, the editing can be done by Buffer::Insert, Buffer::Delete, Buffer::Undo and Buffer::Redo methods. After editting buffer, it is neccessary to take care of reparsing the highlight related info and repaint the screen manually. That means you need to pass to parser pointers to begins of all editted segments by Parser::ParseFromLine call. When all lines were passed to parser, you need to call the Parser::Execute(). It will take care of updating everything neccessary and also will repaint all visible lines that were either passed to the parser manually or whose formatting has changed as result of the edit tasks. That is also the process of getting any lines repainted. Before calling Parser::Execute it is also neccessary to take care of repainting things that are not repainted by Parser - like updating cursor position, moving part of text downwards or upwards (when i.e. a newline has been inserted). If DOUBLE_BUFFERD option is on, you also need to call Drawer::Paint method to refresh screen after any manual drawing that's not followed by reparsing any visible line (Parser's execute does that automatically). 
   *
   * Everything needed for a text insertion and deletion is already encapsulated in TSQLEdit::Insert and TSQLEdit::DeleteSel methods. There are also other methods that simplify movement tasks - TSQLEdit::ProcessChange (for partial screen movements) and TSQLEdit::Scroll.
   */
  class PACKAGE TSQLEdit : public TCustomControl
  {
    private:
      Iter * itrLine;                                                                               /*!< Iterator that points to the first line of window*/
      Iter * itrCursor;                                                                             /*!< Points to actually edited position. If there is mouse drag occuring, then itrCursor is always position where drag started, To get begenning of selected area, use GetCursor() function*/
      Iter * itrCursorSecond;                                                                       /*!< Serves for mouse dragging - always is the iterator that is dragged along*/
      Buffer * buffer;                                                                              /*!< */

      HHOOK KBHook;                                                                                 /*!< Serves for intercepting of keyboard events, that are not by default passed to components*/

      bool recmsg;                                                                                  /*!< Originally to handle re-entrancy of processmessages, but quite obsolete by now.*/

      Parser * parser;                                                                               
      Drawer * drawer;                                                                               

      TScrollBar * HBar;                                                                             
      TScrollBar * VBar;                                                                             
      void UpdateVBar();                                                                             
      void UpdateHBar();                                                                             

      Iter * XYtoItr(int& x, int& y);                                                               /*!< Converts coordinates to coresponding iterator. */
      void UpdateCursor(bool paint);                                                                /*!< Recalculates position of itrCursor, and posts results to the drawer. If paint is set to true, it also asks drawer to redraw window.*/
      void ProcessMouseMove(int& x, int& y);                                                        /*!< processes mouse drag info (called from mouse move and mouse up handlers). */
      void ProcessMouseClear(bool redraw, bool deletecursord);                                      /*!< clears selection */
      Format * selectionFormat;                                                                      
      bool mouseDown;
      bool mouseSelect;                                                                             /*!< Whether component is in text-selection (=mouse drag) mode.*/
      bool cursorsInInvOrder;                                                                       /*!< obsolete*/
      int dx, dy;                                                                                   /*!< Position of mouse down.*/
      int cx, cy;                                                                                   /*!< Cursor position*/
      int mx, my;                                                                                   /*!< Position of itrCursorSecond*/
      int cursorLeftOffset;                                                                         /*!< Column of the cursor, taking into account tabulators. This does NOT equal iterator position.*/
      int scrolldelta;                                                                              /*!< used by ms to treat smooth-scroll wheels...*/
      int scrolldeltafontsize;                                                                      /*!< used by ms to treat smooth-scroll wheels...*/

      void AdjustLine(bool paint);                                                                  /*!< On edit and movement edjusts screen so that cursor is visible*/
      void Scroll(int by);                                                                           
      void __fastcall OnVScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos);            
      void __fastcall OnHScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos);            
      void __fastcall OnResizeCallback(TObject* Sender);                                             

    protected:
      virtual void __fastcall Paint();                                                               
      virtual void __fastcall PaintWindow(HDC DC);                                                   
      virtual void __fastcall RepaintWindow(bool force);                                             

      virtual void __fastcall WndProc(Messages::TMessage &Message);                                 /*!< self-explaining, note that some keyboard events are handled by separate callback */

      DYNAMIC void __fastcall MouseDown(TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
      DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
      DYNAMIC void __fastcall MouseUp(TMouseButton Button, Classes::TShiftState Shift, int X, int Y);

      TClipboard * clipboard;
      void Copy();
      void Paste();
      void DeleteSel(bool allowsync = true);                                                        /*!< Handles all deletion tasks (except load file)*/
      void Insert(wchar_t * text);                                                                  /*!< Handles all insertions.*/
      void ProcessChange(int linesMovedFrom, int linesMoved, NSpan * changed);                      /*!< Handles painting of most actions that need just partial movement of some data - line insertions and deletions, scrolling, etc.*/

      int __fastcall GetLineNum(NSpan * line);                                                      /*!< Returns line number relative to the first visible line. Relic of a positionless handling*/
      bool __fastcall GetLineFirst(NSpan * line);
      Iter * __fastcall GetLineByNum(int num, bool allowEnd);                                       /*!< returns iterator of line numth visible line*/
      Iter * __fastcall GetLineByNum(int num);                                                      /*!< returns iterator of line numth visible line*/

      TKeyPressEvent FOnKeyPress;

    public:
#ifdef DEBUG
      void __fastcall dbgIter();
      void __fastcall Log(String str);
      void CheckIntegrity();
      void CheckIterIntegrity(Iter * itr);
      void Write(AnsiString message);
#endif
      friend class Drawer;
      friend class Parser;

      TMemo * dbgLog;
      int __fastcall GetVisLineCount();
      int __fastcall GetLineCount();

      void __fastcall SetLinenumsEnabled(bool enabled);                                             /*!< sets whether line numbers are being drawn. Needs manual triggering of repaintwindow*/
      bool __fastcall GetLinenumsEnabled();

      int __fastcall GetFontsize();
      void __fastcall SetFontsize(int size);

      int __fastcall GetActLine();                                                                  /*!< returns line number of cursor (absolute position in buffer)*/

      Iter * GetCursor();                                                                           /*!< If part of text is selected, returns pointer to the beginning of selection (or cursor, if nothing is selected). Pointer points to the component's private iterator - do not modify it unless you know what you are doing. */
      Iter * GetCursorEnd();                                                                        /*!< If part of text is selected, returns pointer to the end of selection (or NULL, if nothing is selected). Pointer points to the component's private iterator - do not modify it unless you know what you are doing. */

      void SelectAll();
      void LoadFile(char * filename);

      virtual LRESULT CALLBACK ProcessKey(int code, WPARAM wParam, LPARAM lParam);                  /*!< intercepts key messages that would not be otherwise handed to the component.*/

      __fastcall TSQLEdit(TComponent* Owner);
      __fastcall ~TSQLEdit();

__published:
      __property TAlign Align = {read=FAlign, write=SetAlign, default=0};
      __property int FontSize = {read=GetFontsize, write=SetFontsize, default=DEFONTSIZE};
      __property TKeyPressEvent OnKeyPress = {read=FOnKeyPress, write=FOnKeyPress, default=NULL};
      __property bool LineNums = {write=SetLinenumsEnabled, default=true};
  };
  //---------------------------------------------------------------------------
}
#endif
