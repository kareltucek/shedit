﻿

//---------------------------------------------------------------------------

#ifndef uLanguageDefinition2H
#define uLanguageDefinition2H

#include <list>
#include <vcl.h>

#include "config.h"
#include "tokenizer.h"
#include "uStack.h"

using namespace drgxtokenizer;

//---------------------------------------------------------------------------
namespace SHEdit
{
  /*!
   * LanguageDefinition
   * ==================
   * Parser is not a good name for this class because what is does is basically just a lexical analysis.

   * Parser's task is to parse lines and draw them onto screen. Perser keeps an instance of ParserState at each endline, which specifies in which state parser was at that point. Each time buffer is altered, the component is supposed to call Parser::ParseFromLine function, to tell the Parser to reparse text from given line. Parser then parses code and updates ParserStates of lines it goes over until it reaches a ParserState that is same as the Parser's own. Parser does not parse text right away, but pushes the items into 2 queues (normal queue and priority queue). Parsing is done when Parser::Execute method is called.

   * Parser::Execute() first sorts both queues, and then picks the tasks one by one (first from the priority (redraw) queue). Each time it picks a new task, it checkes its screen position to find out whether to paint the line or just parse. In the former case it also initializes formats (reconstructs it from markupStack and searches through the Iterator handled markup. Then it copies the line's parser State into it's own, calls Parser::ParseLine and continues to do so until some conditions are met. On Each line, the queues are checked and corresponding tasks removed. If priority queue is not empty, and parser has gone out of visible area of screen, then current line is pushed onto non-priority queue, and next task from priority queue is picked. When all priority tasks are reparsed, Parser calls Paint, to actually refresh all what has been painted. If non-priority queue is non empty, parser places its callback into Application's OnIdle event handler and parsing of non-priority queue continues later. Such parsing is stopped each PARSEINONEGO (currently 100) lines, to allow Application to get from "Idle event loop". Non-priority queue is parsed until all tasks under a specified Parser::upperbound are parsed. Upperbound is now set to be component's line Iter + 1000 lines. Reason is mainly so that inserting of " followed by another " few seconds later does not throttle CPU on chasing the already reparsed results of such edits.

   * Parser::ParseLine() parses line char by char using its LanguageDefinition. Each char is given to the LanguageDefinition with current lexer position, and is returned new lexer position altogether with state. Upon the state depends what is done next - if special state was matched, then it is taken cared of appropriatelly - every time current char is pushed into String Parser::actText, where it waits until some "matched" state is returned, when it is drawn. ParseLine() also checks changes in markup. Parser ensures that no word is broken in half or just a substring is matched as entire word using langdef's IsAl and IsAlNum functions (by standard convention - if you do not want it to behave standartly, then override the 2 functions). Also by default Parser allows skipping of white characters during search for matching entries in the dictionaries. The white-character is defined by IsWhite function of langdef. White character skipping can be turned of directly by a bool in LanguageDefinition.

   * 7. LanguageDefinition
   * =====================
   * Describes a sort of  a deterministic PEG/LL(1) grammar. That is - everything is greedy and without any backtracking. Left recursion is not accepted. Duplicit prefixes are accepted and automatically merged, but there's no guarantee that one version wont shadow another. 
   *
   *
   * Node ----------- Node --- ... Every node is either of type nonterminal, lambda, end, tail or terminal.
   * nonterm          term         On a terminal node we just look at the leave index, and jump to the next node...         
   *   |                           On a nonterminal node we do the same, but on the recursive index and also we push the current
   * Node   -------- Node  --- ...    Node onto stack. Every nonterminal node owns a lambda node, which starts the rule.
   * lambda \       nonterm        Lambda node is just an auxiliary node - practically it is never encountered during parsing. 
   *         \        |            End node signalizes, that we should return to the node on stack and continue from there 
   *          \      Node  --- ...    according to the leave index.
   *           \    lambda         Tail recursion works as the nonterm type, but does not preserve the current position.
   *            \           ...                                 Indexes are constructed to contain the lambda closures.
   *             \         /                                    That means, that it maps tokens to jump locations (Nodes).
   *              Node ------- Node --- Node --- Node --- Node  
   *              term        nonterm   term    nonterm   end   Every nonterminal node represents a rule of a grammar, and
   *             SELECT    what_clause  FROM  from_clause         thus may be referred to by other rules.
   *                            |                  |            Nonterminal node can be imagined as a graph of references to 
   *                          Node --- ...        ...             other Nonterminals and simple terminal Nodes (which represent
   *                         lambda    ...                        words (keywords, ...) of a grammar)
   *
   *             Whenever there is no possible jump, we just return one level higher. Thus if you want to highlight an unparsed
   *             code, you can do something like (lowercase denotes nonterminals, uppercase terminals):
   *             command_block -> command error COMMAND_SEPARATOR
   *             command -> <your correct grammar>
   *             error -> EVERY | SINGLE | TERMINAL | OF | YOUR | GRAMMAR | EXCEPT | THE SAFE SYMBOL
   *
   * Lexical analysis is performed by the tokenizer, which is a member object of this class. The token ids are used to uniquely identify the tokens. The tokenizer accepts basic algebraic regular expressions, always preferres the longest match and highest token id. 
   *
   *
   * */

  class FontStyle;

  //---------------------------------------------------------------------------
  class LanguageDefinition
  {
    private:
      WTokenizer tokenizer;
      std::locale loc;

      int ids;

      enum NType {ntTerm, ntNterm, ntLambda, ntEnd, ntTailrec};
      enum TokType {tS, tC, tE};


      struct Term
      {
        bool remember;
        int tokid;
        bool getstyle;
        bool call;
        FontStyle* fs;
        std::wstring name;

        Term(const std::wstring& n, FontStyle* fs_, int i, bool g, bool c) : name(n), fs(fs_), id(i), getstyle(g), call(c) {} ;
        bool Eq(const Term& n){return tokid == n.tokid);
        bool Cq(const Term& n){return tokid == n.tokid && remember == n.remember && getstyle == n.getstyle && call == n.call && *fs == *n.fs);
      };

      struct NTerm
      {
        int ruleid;
        std::wstring name;
        FontStyle* fs;
        bool gather;
        bool call;
        Node* node;
        NTerm(const std::wstring& n, FontStyle* f, int i, bool g, bool c);
        bool Eq(const NTerm& n){return ruleid == n.ruleid);
        bool Cq(const NTerm& n){return ruleid == n.ruleid && gather == n.gather && call == n.call && *fs == *n.fs);
      };

      struct Rec
      {
        Term* t;
        NTerm* nt;
        bool term;
        const std::wstring& GetName(){return term? t->name : nt->name;};
        bool operator<(const Rec& r){return GetName() < r.GetName();};
        Rec(Term* t_) : t(t_), nt(NULL), tern(true);
        Rec(NTerm* t_) : nt(t), t(NULL), term(false);
        Rec() : t(NULL), nt(NULL), term(false){};
      };

      struct Node
      {
        NType type;
        Rec r;
        std::vector<Node*> nextnodes;
        std::map<int, Node*> lftidx; //leave index
        std::map<int, Node*> recidx; //recursive index (for nonterminals)

        Node(NType t) : type(t), r(), nextnodes(), index();
        void Add(const Node& n);
        bool Eq(const Node& n);
        bool Cq(const Node& n); //more power needed - we should check te nextnodes too
        bool operator==(const Node& n);
        void Finalize(); 
        void ExpandLambda(std::map<int, Node*>& index, Node* next);
      }

      struct StackItem
      {
      }

      std::set<Node*> nodes; //temporary - for construction (and maybe for destruction?)
      std::vector<Term*> terms;
      std::vector<Nonterm*> nonterms;
      std::map<std::wstring, Rec> index;

      TokType GetToken(std::wstring& str, std::wstring& val);
    public:
      const int Auto = -1;

      void AddTerm(const std::wstring& name,FontStyle*,const std::wstring& rgx, int id, bool caseSens, bool getst, bool confirm);
      void AddNonTerm(const std::wstring& name, FontStyle* fs, int id, bool gather, bool call);
      void AddRule(const std::wstring& name, std::wstring rule);

      void Finalize(); 

      LanguageDefinition(const std::locale& loc = std::locale());
      ~LanguageDefinition();

  };
}
//---------------------------------------------------------------------------
#endif
