
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

  //define MASK_PARSED 1
  //define MASK_MARKED 2
  //"masks" were supposed to be stored simply as masks, but it would make difficult distinguishing their order (for nested markup)

#define MODE_NORMAL 0
  //---------------------------------------------------------------------------
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
      struct ParserState
      {
        ParserState();
        ~ParserState();
        bool operator==(const ParserState& state);
        bool operator!=(const ParserState& state);
        ParserState& operator=(const ParserState& p);

        short statemask;
        short parseid;
        Stack<Format*> markupStack;
      };

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

      void ReconstructMarkup();

      void Flush();
      void FlushAll();
      void SendEof();
      void AddChar(Iter * itr, int & pos);

      void CheckMarkup(Iter * itr, bool paint);

      std::list<ParseTask> tasklistprior;
      std::list<ParseTask> tasklist;

      void ParseLine(Iter * itr, LanguageDefinition::TreeItem *& searchtoken, bool paint);
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
      void ParseFromToLine(NSpan * line, int linenum, NSpan * toline, int prior);
  };
}
//---------------------------------------------------------------------------
#endif
