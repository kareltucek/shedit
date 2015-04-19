

//---------------------------------------------------------------------------

#ifndef uLanguageDefinition2H
#define uLanguageDefinition2H

#include <list>
#include <set>
//#include <vcl.h> //why?

#include "config.h"

#define DRGXTOKENIZER_TEMPLATED

#include "tokenizer.h"
//#include "uStack.h"

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
   * Node ----------- Node --- ... Every node is either of type nonterminal, lambda, tail or terminal.
   * nonterm          term         On a terminal node we just look at the leave index, and jump to the next node...         
   *   |                           On a nonterminal node we do the same, but on the recursive index and also we push the current
   * Node   -------- Node  --- ...    Node onto stack. Every nonterminal node owns a lambda node, which starts the rule.
   * lambda \       nonterm        Lambda node is just an auxiliary node - practically it is never encountered during parsing. 
   *         \        |            x End - after all we dont need this type...
   *          \      Node  --- ... x 
   *           \    lambda         Tail recursion works as the nonterm type, but does not preserve the current position.
   *            \           ...                                 Indexes are constructed to contain the lambda closures.
   *             \         /                                    That means, that it maps tokens to jump locations (Nodes).
   *              Node ------- Node --- Node --- Node           
   *              term        nonterm   term    nonterm         Every nonterminal node represents a rule of a grammar, and
   *             SELECT    what_clause  FROM  from_clause         thus may be referred to by other rules.
   *                            |                  |            Nonterminal node can be imagined as a graph of references to 
   *                          Node --- ...        ...             other Nonterminals and simple terminal Nodes (which represent
   *                         lambda    ...                        words (keywords, ...) of a grammar)
   *
   *             Whenever there is no possible jump, we just return one level higher. Thus if you want to highlight an unparsed
   *             code, you can do something like (lowercase denotes nonterminals, uppercase terminals):
   *             command_block -> command error COMMAND_SEPARATOR
   *             command -> <your correct grammar>
   *             error -> EVERY | SINGLE | TERMINAL | OF | YOUR | GRAMMAR <except for the COMMAND_SEPARATOR
   *
   * Lexical analysis is performed by the tokenizer, which is a member object of this class. The token ids are used to uniquely identify the tokens. The tokenizer accepts basic algebraic regular expressions, always preferres the longest match and highest token id. 
   *
   *
   * */

  class FontStyle;

  //---------------------------------------------------------------------------

  class LanguageDefinition
  {
    public:
      class NTree;
    private:
      WTokenizer tokenizer;
      std::locale loc;

      int ids;

      enum NType {ntTerm, ntNTerm, ntLambda, ntEnd, ntTailrec};
      enum TokType {tS, tC, tE}; //string, choice, end?

      class Node;
      friend class Node;

      struct Term
      {
        bool remember;
        int tokid;
        bool getstyle;
        bool gather;
        bool call;
        FontStyle* fs;
        std::wstring name;

        Term(const std::wstring& n, FontStyle* fs_, int i, bool g, bool c) : name(n), fs(fs_), tokid(i), getstyle(g), call(c){} ;
        bool Eq(const Term& n){return tokid == n.tokid;};
        bool Cq(const Term& n){return tokid == n.tokid && remember == n.remember && getstyle == n.getstyle && call == n.call && fs == n.fs ;};
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
        bool Eq(const NTerm& n){return ruleid == n.ruleid;};
        bool Cq(const NTerm& n){return ruleid == n.ruleid && gather == n.gather && call == n.call && fs == n.fs;};
      };

      struct Rec
      {
        Term* t;
        NTerm* nt;
        bool term;
        const std::wstring& GetName()const{return term? t->name : nt->name;};
        bool operator<(const Rec& r)const{return GetName() < r.GetName();};
        Rec(Term* t_) : t(t_), nt(NULL), term(true){};
        Rec(NTerm* t_) : nt(t_), t(NULL), term(false){};
        Rec() : t(NULL), nt(NULL), term(false){};
        bool GatherFlg(){ return term ? t->gather : nt->gather;};
        bool CallFlg(){return term ? t->call : nt->call;};
        int Id(){ return term ? t->tokid : nt->ruleid;};
      };

      struct Node
      {
        NType type;
        Rec r;
        std::vector<Node*> nextnodes;
        std::map<int, Node*> lftidx; //leave index
        std::map<int, Node*> recidx; //recursive index (for nonterminals)

        Node() : type(ntLambda), r(), nextnodes(), lftidx(), recidx(){};
        Node(NType t) : type(t), r(), nextnodes(), lftidx(), recidx(){};
        Node(NTerm* t) : type(ntNTerm), r(t), nextnodes(), lftidx(), recidx(){};
        Node(Term* t) : type(ntTerm), r(t), nextnodes(), lftidx(), recidx(){};
        Node* Add(const Node& n); //returns a pointer to a used node (either a new node, or an already existing done)
        bool Eq(const Node& n); //equal (indistinguishable by perser)
        bool Cq(const Node& n); //congruent; more power needed here  - we should check the rest of the tree too
        bool operator==(const Node& n);
        void Finalize(); 
        void ExpandLambda(std::map<int, Node*>& index, Node* next);
      };

      std::set<Node*> nodes; //temporary - for construction (and maybe for destruction?)
      std::vector<Term*> terms;
      std::vector<NTerm*> nonterms;
      std::map<std::wstring, Rec> index;

      std::set<std::wstring> symtab;

      Node* global;
      Node* entering;

      std::wstring emptstr;

      void ParseString(std::wstring& rule, std::vector<Node*>& endnodes);
      TokType GetToken(std::wstring& str, std::wstring& val, bool eat);
      TokType GetToken(std::wstring& str, std::wstring& val);
      void Construct(std::wstring& rule, std::vector<Node*>& endnodes);

      void DestroyTree(NTree* n);
    public:

      struct NTree
      {
        typedef std::vector<NTree*> container_type;

        container_type childs;
        const std::wstring* value;
        int id;

        NTree(int ident, const std::wstring* v = NULL) : childs(), value(v), id(ident){};

        const NTree& operator[](int i) const;
      };

      struct StackItem
      {
        Node* ptr;
        NTree* tree;

        StackItem(Node* n, NTree* t = NULL) : ptr(n), tree(t){};
        void AddValue(NTree*);
      };

      struct PState
      {
        PState() : st(){};
        std::vector<StackItem> st;
      };

      const static int Auto = -1;

      enum flags {tfCall = 1, tfGetStyle = 2, tfGather = 4, tfGlobal = 8, tfCaseSens = 16, tfEntering = 32};

      void AddTerm(const std::wstring& name,FontStyle*,const std::wstring& rgx, int id, int flags);
      void AddNonTerm(const std::wstring& name, FontStyle* fs, int id, int flags);
      void AddNonTerm(const std::wstring& name, FontStyle* fs, const std::wstring& rule, int id, int flags);
      void AddRule(const std::wstring& name, std::wstring rule);

      virtual void GetStyle(int id, const std::wstring& val, bool& draw, PState & s, FontStyle *& fs){};
      virtual void Call(int id, PState & s){};
      virtual void EraseState(int identif){};

      void Finalize(); 

      template<class IT>
      void Parse(IT& from, IT& to, PState&, bool& stylechanged, FontStyle*&fs, bool& draw);

      LanguageDefinition(const std::locale& loc = std::locale());
      ~LanguageDefinition();

    private:
      void Push(PState& s, Node* ptr);
      void Pop(PState& s);

  };
  //---------------------------------------------------------------------------
  template<class IT>
    void LanguageDefinition::Parse(IT& from, IT& to, PState& s, bool& stylechanged, FontStyle*&fs, bool& draw)
    //the stylechanged refers to the inherited styles; the fs of the terminals is returned with false (even when set)
    {
      if(s.st.empty())
        // s.st.push_back(StackItem(entering->r.nt->node)); //not working (we are not prepared (to encounter Lambda))
        s.st.push_back(StackItem(entering));

      IT a(from);
      int tokid;
      tokenizer.NextToken(from, to, tokid);

      bool goingup = false; //signalizes to search leaveindexes instead of recindexes
      bool retried = false; //signalizes that we do second pass from the entering clause

      while(true)
      {
        const std::map<int, Node*>& ref = s.st.back().ptr->type == ntNTerm && !goingup ? s.st.back().ptr->recidx: s.st.back().ptr->lftidx;
        std::map<int, Node*>::const_iterator itr = ref.find(tokid);
        bool skip = false; //goto or not goto, that's the question
        if(itr == ref.end())
        {
          if(!goingup && !retried)
          {
            //if we are here, we are pointing either on the entering nonterminal or on a terminal
            itr = global->recidx.find(tokid);
            if(itr != global->recidx.end())
            {
              if(itr->second->type == ntTerm && itr->second->lftidx.empty() && itr->second->recidx.empty())
                return;
              Push(s, global);
              skip = true;
            }
          }

          if(!skip)
          {
            goingup = true;
            stylechanged |= s.st.back().ptr->type == ntNTerm && s.st.back().ptr->r.nt->fs != NULL;
            Pop(s);

            if(s.st.empty())
            {
              s.st.push_back(StackItem(entering));
              std::wcout << "popped from the stack!" << std::endl;
              if(retried)
                return;
              goingup = false;
              retried = true;
              continue;
            }
            else
            {
              continue;
            }
          }
        }

        if(!skip)
        {
          if(s.st.back().ptr->type == ntNTerm && !goingup)
          {
            Push(s, s.st.back().ptr);
            stylechanged |= s.st.back().ptr->r.nt->fs != NULL;
          }
        }

          if(itr->second->type == ntNTerm)
          {
            s.st.back().ptr = itr->second;
            continue;
          }
          else
          {
            s.st.back().ptr = itr->second;
            fs = s.st.back().ptr->r.t->fs;
            const std::wstring* sym = itr->second->r.t->getstyle || itr->second->r.t->gather ? &*symtab.insert(std::wstring(a,from)).first : NULL;
            if(itr->second->r.t->getstyle)
            {
              FontStyle * tmp = fs;
              GetStyle(tokid, *sym, draw, s, fs);
              stylechanged |= tmp != fs;
            }
            if(s.st.back().ptr->r.CallFlg())
              Call(tokid, s);
            if(itr->second->r.t->gather && s.st.size() > 1 && (s.st.end()-2)->ptr->r.GatherFlg())
            {
              (s.st.end()-2)->AddValue(new NTree(tokid, sym));
            }
            return;
          }
      }
    }
}

//---------------------------------------------------------------------------
#endif
