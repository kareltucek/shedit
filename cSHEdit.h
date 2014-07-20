//---------------------------------------------------------------------------

#ifndef cSHEditH
#define cSHEditH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <ComCtrls.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include "config.h"
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <Controls.hpp>
#include <Clipbrd.hpp>
#include <time.h>
#include "uIter.h"
#include "uIPos.h"
#include "uLanguageDefinition.h"
#include "uLanguageDefinitionSQL.h"
#include "uSpan.h"
#include "uMark.h"
#include "uBuffer.h"
#include "uDrawer.h"
#include "uParser.h"
#include "uCursor.h"
#include <windows.h>

using namespace SHEdit;

namespace SHEdit
{
  LRESULT CALLBACK ProcessKeyCall(int code, WPARAM wParam, LPARAM lParam);


  typedef void __fastcall (__closure *TMessage)(System::TObject* Sender);

  typedef void __fastcall (__closure *TMouseEvent)(System::TObject* Sender, TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
  typedef void __fastcall (__closure *TKeyEvent)(System::TObject* Sender, System::Word &Key, Classes::TShiftState Shift);
  typedef void __fastcall (__closure *TKeyPressEvent)(System::TObject* Sender, System::WideChar &Key);

  /*! \mainpage
   * 0. overrview
   * ============
   * TSHEdit is a syntax highlight source code editing component.
   *
   * TSHEdit consists of 4 main parts. The component class TSHEdit, Buffer, Parser and Drawer. Originally parser and drawer were designed to work in their own thread, but as it turned out, it was not possible due to the fact that vcl is not thread safe and thus text rendering was not possible, so I merged everything into just one thread, and cut off a lot of heavy stuff for thread sync.
   *
   * The data are stored in the buffer in a two-level linked list, which maintains a continuous text stream on one level, and newline list on the other one. Almost entire data structure is designed "lineary" - without andy binary tree indexes, mainly because of relatively high time complexity of keeping the binary tree up-to-date - that means, main approach, that amortizes time complexity to be constant (considering the size of buffer) (except for line movements) is to have an iterator that points to the current position of window, without having access to information about the rest of the data (there is just implemented a linear positioning system, that keeps track of current line number and offset, and that keeps all iterators valid and having the correct positioning info). Above the buffer works the component (actual TSHEdit class), whose job is to process all user interaction, let the buffer to insert and remove text, handle undo and redo funcionality, and to manage painting of the screen. When updating the screen, component repaints just those parts of screen that are no longer valid (by lines - always entire line is reparsed/repainted). Lines that need to be repainted or reparsed are handed over to the Parser whose job is to parse the lines and those that are in visible part of window directly send further to drawer that draws them. Parser works "asynchronously" - can reparse every time just what it needs to reparse.
   *
   * Getting started
   * ---------------
   * To get this component working you need a vcl aware IDE (it is written and tested under C++Builder XE). The steps are to install the component package. Then before you can do anything practical, you need to programaticcaly hand over an instance of language specification to the component (A descendant of LanguageDefinition class, by TSHEdit::SetLanguageDefinition call). You can obtain the object by following manners:
   *
   * 1. Derive new descendant of the LanguageDefinition, and define it's dictionaries in it's body. (more info in documentation of the LanguageDefinition class, for example see the code of LanguageDefinitionSQL)
   * 2. Create new isntance of the LanguageDefinition itself, and construct the lexer "from outside" using it's public methods. (do not forget the SHEdit namespace and include correct header :-) )
   * 3. (NOT YET IMPLEMENTED) Create specification using a configuration file and let the component load it.
   *
   * Last thing you might want to do is to go through published properties using your IDE's designer.
   *
   * Editting/Getting data into/out of component
   * -------------------------------------------
   * All editing etc should be done through the public members of TSHEdit and Cursor iterators that were obtained from the TSHEdit instance (not manually created). Members are quite self-explaining, so it should not be problem to find out yourself. Note that difference between the classes Iter and CIter is that the CIter is an interface of the Iter that takes care of Updating the component - You can get hold of iters, but you should not use them for making changes in buffer, as long as you do not want to have to take care of repainting the screen manually.
   *
   *
   *  Line numbering starts from 1. Line position numbering starts from 0. Each position refers to position on one line - i.e. pos higher than line width is not interpretted as position on next line but should end up at end of line. There is no global position numbering - just line + line position pairs.
   *
   * Text formatting
   * ---------
   * TSHEdit provides 3 ways of formatting documents:
   * 1. The syntax highlight feature. For more information visit LanguageDefinition class documentation.
   * 2. A positionless markup that is hardlinked to the buffer structure, which is kept up-to date through the Parser. It is quite efficient for large amounts of local formatting, but lacks responsivity regarding global formating changes. For more information see Mark class and Iter::MarkupBegin and Iter::MarkupEnd methods.
   * 3. An Iterator-handled global markup, that is established using system of binary search trees. It is efficient for markup changes over large areas and can quickly find current format at any point of buffer. Where it lacks is that Iterators has to be kept up to date throught editing of buffer (and amount of such iterators is thus somehow limited). For more information see IMark class and IPos::IMarkupBegin and IPos::IMarkupEnd methods.
   *
   * The syntax highlight has the lowest priority while Iterator-handled markup has the highest (that's how they are put together in Parser class).
   *
   * Todo - what is not really finished
   * ----------------------------------
   *  - Config file parser for language definitions.
   *  - Memory optimalization - new container template beside the Stack should be implemented that would store data at one place and would allow fast assigning and comparing.
   *  done - Search capability.
   *  - Dictionary structure should be somehow rewritten.
   *  - Add new layer between iterator and cursor to allow comfortable cursor movement without need of manual redrawing
   *  - Extend api to provide EVERYTHING neccessary
   *  - Debug, debug, debug
   *  - Add some sensible documentation export
   *
   * Content of Documentation
   * ------------------------
   *  1. TSHEdit - the main component class
   *  2. Buffer - overview of Buffer related functionality
   *  3. buffer's structures - NSpan Span Range and Action - just very brief defs
   *  4. IPos - positioning
   *  5. Iter - a bit more on positioning
   *  6. Parser - how parsing works
   *  7. LanguageDefinition and example LanguageDefinitionSQL and LanguageDefinitionWebLangs
   *  8. Drawer - just some notes
   *  9. Text formatting - Format FontStyle Mark IMark
   *  10. Stack - minimalistic stack reimplementation (for low memory consumption needs)
   */

  /**
   * 1. TSHEdit - the component class
   * =============================
   * This class represents the component itself. It's task is to process user input and command other objects what to do - information this class stores are Iterators pointing to the current line, cursor position and selection end, then information about scrolling deltas and mouse positioning information. It's members are on one hand overriden message/event handling procedures and on the other hand methods that are used to process these.
   *
   * Inner funcionality
   * -----------------
   * ### Keyboard capturing ###
   * Keyboard is trapped at 2 places - first is normal overriden winproc message for text input and the second is a windows hook which lets us intercept all keys we want, before they're passed to the program (that's neccessary to intercept input that would be otherwise consumed by vcl itself and would not be sent to component at all
   *
   * ### Mouse capturing ###
   * Mouse events are captured by 3 handlers - those are TSHEdit::MouseDown TSHEdit::MouseMove and TSHEdit::MouseUp methods. On mouse down itrCursor is placed at correct position. If mouse continues to move more than specified trashold, text-selection mode is entered and itrCursorSecond is used to mark the other end of selected area.
   *
   * ### Handling edits, parsing, and screen repainting ###
   *  All positioning is done using Iter class. When obtained a signal to do some editing, the editing can be done by Buffer::Insert, Buffer::Delete, Buffer::Undo and Buffer::Redo methods. After editting buffer, it is neccessary to take care of reparsing the highlight related info and repaint the screen manually. That means you need to pass to parser pointers to begins of all editted segments by Parser::ParseFromLine call. When all lines were passed to parser, you need to call the Parser::Execute(). It will take care of updating everything neccessary and also will repaint all visible lines that were either passed to the parser manually or whose formatting has changed as result of the edit tasks. That is also the process of getting any lines repainted. Before calling Parser::Execute it is also neccessary to take care of repainting things that are not repainted by Parser - like updating cursor position, moving part of text downwards or upwards (when i.e. a newline has been inserted). If DOUBLE_BUFFERD option is on, you also need to call Drawer::Paint method to refresh screen after any manual drawing that's not followed by reparsing any visible line (Parser's execute does that automatically).
   *
   * Everything needed for a text insertion and deletion is already encapsulated in TSHEdit::Insert and TSHEdit::DeleteSel methods. There are also other methods that simplify movement tasks - TSHEdit::ProcessChange (for partial screen movements) and TSHEdit::Scroll.
   */
  class PACKAGE TSHEdit : public TCustomControl
  {
    private:
      Iter itrLine;                                                                                  /*!< Iterator that points to the first line of window*/
      Iter itrCursor;                                                                                /*!< Points to actually edited position. If there is mouse drag occuring, then itrCursor is always position where drag started, To get begenning of selected area, use GetCursor() function*/
      Iter itrCursorSecond;                                                                          /*!< Serves for mouse dragging - always is the iterator that is dragged along*/
      Buffer * buffer;                                                                               /*!< */

      HHOOK KBHook;                                                                                  /*!< Serves for intercepting of keyboard events, that are not by default passed to components*/

      bool recmsg;                                                                                   /*!< Originally to handle re-entrancy of processmessages, but quite obsolete by now.*/

      Parser * parser;
      Drawer * drawer;

      TScrollBar * HBar;
      TScrollBar * VBar;
      void UpdateVBar();
      void UpdateHBar();

      bool msgLock;

      TTimer * timer;

      TColor selColor;
      TColor searchColor;

      bool readonly;

      int GetScrollStep();
      int maxScrollStep;

      Iter XYtoItr(int& x, int& y);                                                                  /*!< Converts coordinates to coresponding iterator. */
      void UpdateCursor(bool paint);                                                                 /*!< Recalculates position of itrCursor, and posts results to the drawer. If paint is set to true, it also asks drawer to redraw window.*/
      void ProcessMouseMove(int& x, int& y);                                                         /*!< processes mouse drag info (called from mouse move and mouse up handlers). */
      void ProcessMouseClear(bool redraw, bool deletecursord);                                       /*!< clears selection */
      Format * selectionFormat;
      Format * searchFormat;
      bool mouseDown;
      bool mouseSelect;                                                                              /*!< Whether component is in text-selection (=mouse drag) mode.*/
      int dx, dy;                                                                                    /*!< Position of mouse down.*/
      int cx, cy;                                                                                    /*!< Cursor position*/
      int mx, my;                                                                                    /*!< Position of itrCursorSecond*/
      int cursorLeftOffset;                                                                          /*!< Column of the cursor, taking into account tabulators. This does NOT equal iterator position.*/
      int scrolldelta;                                                                               /*!< used by ms to treat smooth-scroll wheels...*/
      int scrolldeltafontsize;                                                                       /*!< used by ms to treat smooth-scroll wheels...*/
                                                                                                     /*!< On edit and movement edjusts screen so that cursor is visible*/
      void Scroll(int by);
      void __fastcall OnVScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos);
      void __fastcall OnHScroll(TObject *Sender, TScrollCode ScrollCode, int &ScrollPos);
      void __fastcall OnResizeCallback(TObject* Sender);

      void __fastcall OnTimer(TObject * Sender);                                                     /*!< ensures cursor blinking :) */

      void __fastcall OnEnterHandler(TObject * Sender);

      void Action(String name, bool end = true);
    protected:
      virtual void __fastcall Paint();
      virtual void __fastcall PaintWindow(HDC DC);

      virtual void __fastcall WndProc(Messages::TMessage &Message);                                  /*!< self-explaining, note that some keyboard events are handled by separate callback */

      DYNAMIC void __fastcall MouseDown(TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
      DYNAMIC void __fastcall MouseMove(Classes::TShiftState Shift, int X, int Y);
      DYNAMIC void __fastcall MouseUp(TMouseButton Button, Classes::TShiftState Shift, int X, int Y);

      void __fastcall KeyDownHandler(System::TObject * Sender, System::Word &Key, Classes::TShiftState Shift)     ;
      void __fastcall KeyPressHandler(System::TObject * Sender,System::WideChar &Key)     ;
      void __fastcall KeyUpHandler(System::TObject * Sender,System::Word &Key, Classes::TShiftState Shift)         ;

      TClipboard * clipboard;
      void DeleteSel(bool allowsync = true, Iter * start = NULL, Iter * end = NULL);                 /*!< Handles all deletion tasks (except load file). If start or end are NULL, then the deletion is performed on the selection*/
      void Insert(const wchar_t * text, Iter * itr = NULL);                                          /*!< Handles all insertions.*/
      void ProcessChange(int linesMovedFrom, int linesMoved, NSpan * changed);                       /*!< Handles painting of most actions that need just partial movement of some data - line insertions and deletions, scrolling, etc.*/

      void ParseScreenBetween(Iter * it1, Iter * it2);                                               /*!< Pushes visible lines between it1 and it2 to parser. Takes care of right order of iters, and of trimming the. */

      int __fastcall GetLineNum(NSpan * line);                                                       /*!< Returns line number relative to the first visible line. Relic of a positionless handling*/
      bool __fastcall IsLineFirstVisible(NSpan * line);
      Iter __fastcall GetLineByNum(int num, bool allowEnd);                                          /*!< returns iterator of line numth visible line*/
      Iter __fastcall GetLineByNum(int num);                                                         /*!< returns iterator of line numth visible line*/

      TKeyPressEvent FOnKeyPress;
      TKeyEvent FOnKeyDown;
      TKeyEvent FOnKeyUp;
      TMessage FOnChange;
      TMessage FOnClick;
      TMessage FOnEnter;
      TMessage FOnExit;
      TMouseEvent FOnMouseUp;

     void __fastcall SetSelLen(int SelLen);
     int __fastcall GetSelLen();
     void __fastcall SetSelText(String SelText);
     String __fastcall GetSelText();
     void __fastcall SetText(String);
     String __fastcall GetText();
     bool Visible(Iter * itr);

     void ProcessNewSelection();

    public:
#ifdef DEBUG
      void __fastcall dbgIter();
      void __fastcall Log(String str);
      void CheckIntegrity();
      void CheckIterIntegrity(Iter * itr);
      void UndoCheck();
      void Write(String message, bool endl = true);
      String PositionDescription();
      String Escape(String str);
#endif
      friend class Drawer;
      friend class Parser;
      friend class CIter;

      TMemo * dbgLog;
      int __fastcall GetVisLineCount();
      int __fastcall GetLineCount();

      void __fastcall SetLanguageDefinition(LanguageDefinition * def);

      void __fastcall SetLinenumsEnabled(bool enabled);                                              /*!< sets whether line numbers are being drawn. Needs manual triggering of repaintwindow*/
      bool __fastcall GetLinenumsEnabled();

      int __fastcall GetFontsize();
      void __fastcall SetFontsize(int size);
      int __fastcall GetLineHeight();

      int __fastcall GetActLine();                                                                   /*!< returns line number of cursor (absolute position in buffer)*/

      void AdjustLine(bool paint);

      CIter GetCursor();                                                                             /*!< CIter returns a copy of the cursor (or selection start) Iter wrapped in a CIter instance. The CIter always behaves as a valid Iterator, and takes care of rmaking changes to the canvas.*/
      CIter GetCursorEnd();                                                                          /*!< CIter returns a copy of the cursor (or selection end) Iter wrapped in a CIter instance. The CIter always behaves as a valid Iterator, and takes care of rmaking changes to the canvas.*/

      Iter * GetCursorIter();                                                                        /*!< If part of text is selected, returns pointer to the beginning of selection (or cursor, if nothing is selected). Pointer points to the component's private iterator - do not modify it unless you know what you are doing. */
      Iter * GetCursorIterEnd();                                                                     /*!< If part of text is selected, returns pointer to the end of selection (or NULL, if nothing is selected). Pointer points to the component's private iterator - do not modify it unless you know what you are doing. */

      CIter begin();
      CIter end();

      int GetCursorX();
      int GetCursorY();
      int GetCursorCaretX();
      int GetCursorCaretY();
      void ItrToXY(Iter * itr, int& x, int& y);

      void Copy();
      void Paste();

      void SelectAll();

      virtual void __fastcall RepaintWindow(bool force = true);

      void Clear();
      void AddLine(const String& string);
      void AddLines(const String& string);
      String GetLine(Iter* itr);
      String GetLine(int index);

      void LoadFile(const wchar_t * filename);
      void SaveFile(const wchar_t * filename);

      void MarkAll(const String& text, bool caseSensitive = false, bool wholeWord = false);
      void UnMarkMarked();

      virtual LRESULT CALLBACK ProcessKey(int code, WPARAM wParam, LPARAM lParam);                   /*!< intercepts key messages that would not be otherwise handed to the component.*/

      __fastcall TSHEdit(TComponent* Owner);
      __fastcall ~TSHEdit();

      void SetSelection(Iter * first, Iter *  second);
      String GetRange(Iter * begin, Iter * end);

     __property int SelLen = {read=GetSelLen, write=SetSelLen};
     __property String SelText = {read=GetSelText, write=SetSelText};
     __property String Text = {read=GetText, write=SetText};
__published:
      __property TAlign Align = {read=FAlign, write=SetAlign, default=0};
      __property int FontSize = {read=GetFontsize, write=SetFontsize, default=DEFONTSIZE};
      __property TKeyPressEvent OnKeyPress = {read=FOnKeyPress, write=FOnKeyPress, default=NULL};
      __property TKeyEvent OnKeyDown = {read=FOnKeyDown, write=FOnKeyDown, default=NULL};
      __property TKeyEvent OnKeyUp = {read=FOnKeyUp, write=FOnKeyUp, default=NULL};
      __property TMessage OnChange = {read=FOnChange, write=FOnChange, default=NULL};
      __property TMessage OnClick = {read=FOnClick, write=FOnClick, default=NULL};
      __property TMessage OnEnter = {read=FOnEnter, write=FOnEnter, default=NULL};
      __property TMessage OnExit = {read=FOnExit, write=FOnExit, default=NULL};
      __property TMouseEvent OnMouseUp = {read=FOnMouseUp, write=FOnMouseUp, default=NULL};
      __property bool LineNums = {read=GetLinenumsEnabled,write=SetLinenumsEnabled, default=true};
      __property bool ReadOnly= {read=readonly,write=readonly, default=false};
  };
  //---------------------------------------------------------------------------
}
#endif
