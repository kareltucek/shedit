
//---------------------------------------------------------------------------

#ifndef uParserH
#define uParserH

#include "uFormat.h"
#include <vcl.h>
#include <list>
#include <uLanguageDefinition.h>
#include <windows.h>
#include <queue>
#include "config.h"

namespace SHEdit
{
  class NSpan;
  class TSQLEdit;
  class Drawer;
  class Mark;
  class Format;
  class FontStyle;
  class Iter;
  class DrawTaskText;

  /*!
   * Parser's task is to parse lines and keep information about highlighting that is neccessary for correct highlighting. Perser keeps a ParserState at each endline, which specifies in which state parser was at that point. Each time buffer is altered, the component is supposed to call Parser::ParseFromLine function, to tell the Parser to reparse text from given line. Parser then parses code and updates ParserStates of lines it goes over until it reaches a ParserState that is same as the Parser's own. Parser does not parse text right away, but pushes the items into 2 queues (normal queue and priority queue (the latter for lines that are supposed to be reparsed first - as visible lines, which are supposed to be redrawn as soon as possible)). Parsing is done when Parser::Execute method is called.
   *
   * Parser::Execute() first sorts both queues, and then picks tke tasks one by one (first from priority queue). Each time it picks new task, it checkes it's screen position to know whether to paint line or just parse. In the former case it also initializes formats (reconstructs it from markupStack and searches through Iterator handled markup. Then it copies the line's parser State into it's own, and calls Parser::ParseLine, and continues to do so until some conditions are met. On Each line, the queues are checked and corresponding tasks removed. If priority queue is not empty, and parser has gone out of visible area of screen, then current line is pushed onto non-priority queue, and next task from priority queue is picked. When all priority tasks are reparsed, Parser calls Paint, to actually refresh all what has been painted. If non-priority queue is non empty, parser places itself into Application's OnIdle event handler and parsing of non-priority queue continues later. Such parsing is broken each PARSEINONEGO (currently 100) lines, to allow Application to get from "Idle event loop". Non-priority queue is parsed until all tasks under a specified Parser::upperbound are parsed. Upperbound is now set to be component's line Iter + 1000 lines. Reason is mainly so that inserting of " followed by another " few seconds later does not throttle CPU on chasing the already reparsed results of such edits.
   *
   * Parser::ParseLine() parses line char by char using its LanguageDefinition. Each char is given to the LanguageDefinition with current lexer position, and is returned new lexer position altogether with state. Upon the state depends what is done next - usually current char is pushed into String Parser::actText, where it waits until some "matched" state is returned, when it is drawn. ParseLine() also checks changes in markup.
   * */
  class Parser
  {
    private:
      /*struct Node
      {
        Node(Format * format, Node * node);
        Format * format;
        Node * node;
      };*/

    public:
      /*!
       * ParserState keeps current state of parser. ParserSstate::statemask holds id of current position in lexer (not really good idea - would be probably better to establish a mask/id pair with it's own stack, but currently this suffices), parseid - which is checked with Parser::currentparseid, to allow easily invalidate some part of Buffer. ParserState::markupStack then holds information about current positionless markup (the Iterator handled markup is checked dynamicaly and not kept anywhere, except for its resulting FontStyle).
       * */
      struct ParserState
      {
        ParserState();
        ~ParserState();
        bool operator==(const ParserState& state);
        bool operator!=(const ParserState& state);
        ParserState& operator=(const ParserState& p);

        short parseid;
        Stack<LanguageDefinition::SearchIter> searchStateStack;
        Stack<Format*> markupStack;
      };

      /*!
       * ParseTask represents a task for Parser. It is passed from TSQLEdit to the Parser whenever component wants anything to be reparsed or repainted
       * */
      struct ParseTask
      {
        ParseTask(NSpan * line, int linenum);
        ParseTask();
        NSpan * line;
        int linenum;
        bool operator==(const ParseTask & pt)const;
        bool operator<(const ParseTask & pt)const;
      };

    private:
      ParserState state;
      LanguageDefinition * langdef;

      TSQLEdit * parent;
      Drawer * drawer;

      bool newline;
      short linenum;
      short currentparseid;

      int dbgscb;
      bool onidleset;
      bool inword;
      TIdleEvent oldidle;

      int upperbound;
      /*
         String * outString;
         String * actString;
         TColor * actColor;
         TColor * outColor;
         bool outMarked;
         bool actMarked; */
      String actText;
      FontStyle actFormat;
      FontStyle actMarkupCombined;
      FontStyle actMarkup;
      FontStyle actIMarkup;
      int recurse;

      void ReconstructMarkup(); /*!< Reconstructs ActMarkup (that holds positionless markup). Does not update actMarkupCombined. */

      void Flush();
      void FlushAll();
      void SendEof();
      void AddChar(Iter * itr, int & pos);

      inline void CheckMarkup(Iter * itr, bool paint);

      bool CanGoFurther(LanguageDefinition::SearchIter sit, Iter * itr, bool inword, bool checkbefore = false);

      std::list<ParseTask> tasklistprior;
      std::list<ParseTask> tasklist;

      void ParseLine(Iter * itr, LanguageDefinition::SearchIter& searchtoken, bool paint);
      void __fastcall Draw();
#ifdef DEBUG
      static void Write(AnsiString message);
#endif
    public:
#ifdef DEBUG
      bool dbgLogging;
#endif
      friend class TSQLEdit;

      void __fastcall OnIdle(TObject * Sender, bool& Done);

      bool __fastcall Execute(void);

      void InvalidateAll();

      __fastcall Parser(TSQLEdit * parent, Drawer * drawer);
      virtual __fastcall ~Parser();

      void SetLangDef(LanguageDefinition * langdef);

      void ParseFromLine(NSpan * line, int linenum, int prior);
      void ParseFromToLine(NSpan * line, int linenum, int count, int prior);
  };
}
//---------------------------------------------------------------------------
#endif
