
//---------------------------------------------------------------------------

#ifndef uParserH
#define uParserH

#include <vcl.h>
#include <list>
#include <uLanguageDefinition.h>
#include <windows.h>
#include "config"

namespace SHEdit
{
  class NSpan;
  class TSQLEdit;
  class Drawer;
  class Mark;
  class Format;
  class Iter;
  class DrawTaskText;

#define MASK_PARSED 1
  //define MASK_MARKED 2
  //other masks are allocated dynamically

#define MODE_NORMAL 0
  //---------------------------------------------------------------------------
  class Parser : public TThread
  {
    private:
      struct Node
      {
        Node(Format * format, Node * node);
        Format * format;
        Node * node;
      };

    public:
      struct ParserState
      {
        ParserState();
        bool operator==(const ParserState& state);
        bool operator!=(const ParserState& state);
        ParserState& operator=(const ParserState& p);

        short parentheses;
        short braces;
        short brackets;
        short statemask;
        Node * markupStack;
      };

    private:
      ParserState state;
      LanguageDefinition * langdef;

      TSQLEdit * parent;
      Drawer * drawer;

      HANDLE bufferChanged;
      HANDLE bufferMutex;
      HANDLE drawerQueueMutex;
      HANDLE drawerCanvasMutex;
      HANDLE drawerTaskPending;

      bool newline;
      short linenum;
      /*
         String * outString;
         String * actString;
         TColor * actColor;
         TColor * outColor;
         bool outMarked;
         bool actMarked; */
      DrawTaskText * actTask;
      DrawTaskText * outTask;

      void Flush();
      void FlushAll();
      void SendString();
      void SendEof();

      void CheckMarkup(Iter * itr, bool paint);

      std::list<NSpan*> tasklistprior;
      std::list<NSpan*> tasklist;

      void ParseLine(Iter * itr, LanguageDefinition::TreeItem *& searchtoken, bool paint);
#ifdef DEBUG
      void Write(AnsiString message);
#endif
    public:
      static void MarkupPush(Node ** at, Format* format);
      static void MarkupPop(Node ** at, Format * format);
      static bool MarkupContains(Node ** at, Format * format);
      static bool MarkupEquals(Node * at, Node * bt);

      virtual void __fastcall Execute(void);

      __fastcall Parser(TSQLEdit * parent, Drawer * drawer, HANDLE bufferChanged, HANDLE bufferMutex, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending);
      virtual __fastcall ~Parser();

      void SetLangDef(LanguageDefinition * langdef);

      void ParseFromLine(NSpan * line, int prior);
  };
}
//---------------------------------------------------------------------------
#endif
