

//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionH
#define uLanguageDefinitionH

#include <list>
#include <vcl.h>

#include "config.h"
//---------------------------------------------------------------------------
namespace SHEdit
{
  /*!
   * LanguageDefinition
   * ==================
   * Parser is not a good name for this class because what is does is basically just a lexical analysis.

   * Parser's task is to parse lines and draw them onto screen. Perser keeps an instance of ParserState at each endline, which specifies in which state parser was at that point. Each time buffer is altered, the component is supposed to call Parser::ParseFromLine function, to tell the Parser to reparse text from given line. Parser then parses code and updates ParserStates of lines it goes over until it reaches a ParserState that is same as the Parser's own. Parser does not parse text right away, but pushes the items into 2 queues (normal queue and priority queue). Parsing is done when Parser::Execute method is called.

   * Parser::Execute() first sorts both queues, and then picks the tasks one by one (first from priority queue). Each time it picks new task, it checkes its screen position to know whether to paint line or just parse. In the former case it also initializes formats (reconstructs it from markupStack and searches through Iterator handled markup. Then it copies the line's parser State into it's own, calls Parser::ParseLine and continues to do so until some conditions are met. On Each line, the queues are checked and corresponding tasks removed. If priority queue is not empty, and parser has gone out of visible area of screen, then current line is pushed onto non-priority queue, and next task from priority queue is picked. When all priority tasks are reparsed, Parser calls Paint, to actually refresh all what has been painted. If non-priority queue is non empty, parser places its callback into Application's OnIdle event handler and parsing of non-priority queue continues later. Such parsing is stopped each PARSEINONEGO (currently 100) lines, to allow Application to get from "Idle event loop". Non-priority queue is parsed until all tasks under a specified Parser::upperbound are parsed. Upperbound is now set to be component's line Iter + 1000 lines. Reason is mainly so that inserting of " followed by another " few seconds later does not throttle CPU on chasing the already reparsed results of such edits.

   * Parser::ParseLine() parses line char by char using its LanguageDefinition. Each char is given to the LanguageDefinition with current lexer position, and is returned new lexer position altogether with state. Upon the state depends what is done next - if special state was matched, then it is taken cared of appropriatelly - every time current char is pushed into String Parser::actText, where it waits until some "matched" state is returned, when it is drawn. ParseLine() also checks changes in markup. Parser ensures that no word is broken in half or just a substring is matched as entire word using langdef's IsAl and IsAlNum functions (by standard convention - if you do not want it to behave standartly, then override the 2 functions). Also by default Parser allows skipping of white characters during search for matching entries in the dictionaries. The white-character is defined by IsWhite function of langdef. White character skipping can be turned of directly by a bool in LanguageDefinition.

   * 7. LanguageDefinition
   * =====================
   * Language highlighting is done in a way that resembles an simple automaton, whose behaviour is  defined by this class. LanguageDefinition class itself is rather a wrapper, that holds general information - the actual specification of language is held by a set of trees of TreeNodes (creating sort of directed graph).

   * Actual searching is done by passing an instance of SearchIter between Parser and LanguageDefinition, while LanguageDefinition provides language-related information and Parser interprets them regarding the structure of source code. SearchIter contains a pointer to the base (/root) of current tree, a pointer to the current item and a "mask" that represents a 16 bit register in which some information may be stored. Parser actually stores SearchIters in a stack that allows parser to return from a tree to a tree from which it got there.

   * At the moment it is implemented in a very naive (and memory-inefficient) way - each TreeNode represents just one character and has an array of characters that allows constant-time acces to next node. Each part of dictionary has its root (also called basee here) which represents an empty search from which strings of tree items form reserved words of the language represented. The Language definition then let's a pointer move along these strings as it is provided characters from parser. Every TreeNode has assigned one value of enum LangDefSpecType which represents what type of word ends at that node and for some nodes also some additional information. The LangDefSpecTypes are:
   *   - Empty - represents root of part of dictionary structure
   *   - Nomatch - a place where nothing is matched, but from where search may continue to next character (i.e. middle of a reserved word)
   *   - Normal - end of a standard word, that is to be formatted by the format the node points to (the search iter is then pointed back to root)
   *   - Jump/Push - is matched as the Normal tag, but does not return the search item back to root, but carries it further to a root of another tree. Before every jump, pop mask is checked, and if it matches, then pop is performed instead. Three optional masks can be specified:
   *     - a "jumpmask" - if it is specified, then jump occurs only if current searchIter's mask contains something of the jumpmask (== when jumpmask & currentmask > 0). 
   *     - a "newmask" - if jump is performed, the current searchIter's mask is xored with the newmask
   *     - a "globalmask" - same as newmask but this one is global which means that it is not altered by pushing and popping (both masks are "or"ed before checking against jump/push masks)
   *     Each TreeNode can have more sets of the masks and target trees assigned. In that case they are tried one by one in order in which the jump records were created until some jump is matched. Default values for masks are 0 and 0 which means "jump always and let the mask and stack be as it is". Pop mask is not used in jump logic at all - its purpose here is to prevent jumping if a pop should be performed instead (and in that case regular pop is performed instead).
   *   - PushPop
   *     - Push is implemented exactly the same way as jump is but with a little difference - If jumpmask (abreviated also as pushmask) is matched, then jump is performed by Parser and former state is pushed onto stack, where it waits until a pop occurs. (then also the newmask value is added by xor to current mask)
   *     - Pop - again is implemented as jump/push, but its records are stored in separate list and are checked always before push/jump/pops If popmask is matched (as popmask & current mask == true) then Parser pops search iters from top of its stack according to popcount and continues parsing from the new top of the stack. If popcount is greater or equal to zero then it specifies the count of items to be popped. If popcount is -1 then items are popped while the popmask matches the top-of-the-stack's mask. -2 can be passed to functions when we do not want to alter popcount by the function call.
   *     Pop is always checked before push/jump.
   *   - LineTag - obsolete - At this point a jump is performed and current parserState is remembered until end of line, where the parser state is forced back to the saved state. For standard linetag functionality it is recommended to use pair of jumps.
   *   - WordTag - as normal tag, but formats everything until end of word (by standard convention that word begins by alpha and continues as alpha-numeric - these can be overriden). Written with regard i.e. for sql/php variables

   * Default tree (the entry point of automaton) can be obtained by GetTree() function.

   * case sensitivity - Case sensitivity is set separately for each tree root. You have to set it correctly BEFORE you start adding keywords (otherwise some of keywords may become completely unaccessible). The Langdef's SetCaseSensitive serves just for setting case sensitiviness of the newly created trees (i.e. it definitely won't do anything with the initially provided tree. 

   * LanguageDefinition can be constructed using following functions
   *       void AddKeywords(wchar_t * string, FontStyle * format, TreeNode * at = NULL); 
   *       void AddJump(wchar_t * string, FontStyle * format, LangDefJumpType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short gmask= 0); 
   *       void AddPush(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask = 0, short newgmask = 0); 
   *       void AddPop(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount = -2, short newgmask = 0);
   *       void AddWord(wchar_t * string, FontStyle * format, TreeNode * at = NULL); 
   *       TreeNode * AddLine(wchar_t * string, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); 
   *       TreeNode * AddLineStrong(wchar_t * string, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); 
   *       TreeNode * AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); 
   *       TreeNode * AddPushPopPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at, TreeNode * to, short mask); 
   *       TreeNode * AddNewTree(FontStyle * format); 
   *       TreeNode * AddDupTree(TreeNode * tree, FontStyle * format); 
   * */


#define PUSH_ALWAYS 0
#define POP_AUTO -1

  class FontStyle;

  enum LangDefSpecType{Empty = 0x1, Nomatch = 0x2, Normal=0x4, Jump=0x8, WordTag=0x20, LineTag=0x40, Lookahead = 0x80};
  enum LangDefJumpType{tJump, tPush, tMask};
  //---------------------------------------------------------------------------
  class LanguageDefinition
  {
    public:
      struct Jump;
      struct TreeNode;
      struct SearchIter;
      //typedef SearchIter SearchIter;

    private:
      bool caseSensitive;
      bool allowWhiteSkipping;
      FontStyle * defFormat;
      TreeNode * tree;

      TreeNode* FindOrCreateItem(TreeNode * item, wchar_t c, TreeNode * at);
      LangDefSpecType Go(SearchIter * item, wchar_t c, bool & lookahead);                            /*!< Serves Parser for retrieving information about where to go further. Lookahead is set to true, if item was returned to empty and directly went to first unmatched character */

      void _AddPush(bool tobegin, wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask, short newgmask);
      void _AddJump(bool tobegin, wchar_t * string, FontStyle * format, LangDefJumpType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short newgmask); /*!< Adds a custom jump from "at" tree to "to" tree.*/

      struct Jump
      {
        Jump(short _pushmask, short _newmask, short _newgmask, LangDefJumpType _type, TreeNode * _next, FontStyle * format);
        Jump();
        short pushmask;
        short newmask;
        short newgmask;
        short type;
        TreeNode * nextTree;
        FontStyle * format;
      };

      struct Pop
      {
        Pop(short _popmask, short _newgmask, short _popcount, FontStyle * format);
        Pop();
        short popmask;
        short popcount;
        short newgmask;
        FontStyle * format;
      };

    public:
      friend class Parser;

      struct TreeNode
      {
        TreeNode(wchar_t c, LangDefSpecType type, FontStyle * format, bool caseSensitive);
        TreeNode(TreeNode * tree, FontStyle * format);

        void AddJump(short pushmask, short newmask, short newgmask, LangDefJumpType type, TreeNode * to, bool begin, FontStyle *format);
        void AddPop(short popmask, short newmask, short popcount, bool begin, FontStyle *format);

        wchar_t thisItem;
        bool caseSensitive;
        std::list<TreeNode*> items;
        TreeNode * map[128];
        FontStyle * format;
        LangDefSpecType type;
        //TreeNode * nextTree;

        short jumpcount;
        Jump * jumps; //an array

        short recpopcount;
        Pop * pops;

#ifdef _DEBUG
        wchar_t * Name;
#endif
      };

      struct SearchIter
      {
        SearchIter();
        TreeNode * current;
        TreeNode * base;
        short mask;

        bool operator==(const SearchIter& sit);
        bool operator!=(const SearchIter& sit);
      };

      void AddPops(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount = -2, short newgmask = 0); /*!< Just an abreviation. */
      void AddJumps(wchar_t * string, FontStyle * format, LangDefJumpType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short newgmask= 0); /*!< Just an abreviation. */
      void AddJumpsFront(wchar_t * string, FontStyle * format, LangDefJumpType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short newgmask= 0); /*!< Just an abreviation. */
      void AddPushes(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask = 0, short newgmask = 0); /*!< Just an abreviation. */
      void AddPushesFront(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask = 0, short newgmask = 0); /*!< Just an abreviation. */

      void AddKeywords(wchar_t * string, FontStyle * format, TreeNode * at = NULL);                  /*!< Adds all words that are contained in string (as space-separated list) to the tree given as "at". If tree is not given, then the main root is used */
      void AddJump(wchar_t * string, FontStyle * format, LangDefJumpType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short gmask= 0); /*!< Adds a custom jump from "at" tree to "to" tree. String is a space separated list.*/
      void AddJumpFront(wchar_t * string, FontStyle * format, LangDefJumpType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short gmask= 0); /*!< Like AddJump but adds new jumps to the beginning of jump list.*/
      void AddPush(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask = 0, short newgmask = 0); /*!< Adds pushes specified by string (as space separated list. */
      void AddPushFront(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask = 0, short newgmask = 0);
      void AddPop(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount = -2, short newgmask = 0);
      void AddWord(wchar_t * string, FontStyle * format, TreeNode * at = NULL);                      /*!< Adds a wordtag item (i.e. for highlighting php variables as $test just by $) */
      TreeNode * AddLine(wchar_t * string, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); /*!< Adds a linetag item - as c commenting //. Returns new tree that was created for the line's formatting. Is an abbreviation for double jump. */
      TreeNode * AddLineStrong(wchar_t * string, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); /*!< as AddLine, but stores entire state of parser and at the end of line it restores it back. */
      TreeNode * AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); /*!< Is just an abreviation for two jumps. Adds jump from "opening" tag at "at" tree to newly created tree and then corresponding jump back */
      TreeNode * AddPushPopPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at, TreeNode * to, short mask); /*!< Is just an abreviation for push that adds the mask and pop that is conditioned by the same mask.*/
      TreeNode * AddNewTree(FontStyle * format);                                                     /*!< Just creates and returns an empty new tree with format as the default format */
      TreeNode * AddNewTree(FontStyle * format, bool caseSensitive);                                 /*!< Just creates and returns an empty new tree with format as the default format */
      TreeNode * AddDupTree(TreeNode * tree, FontStyle * format);                                    /*!< Duplicates the base of tree provided with the format provided. All pointers remain intact - one tree may have more than one bases. By making changes that are deeper than the duplicated TreeNode's member pointers, then you are making changes to both trees. */

      void SetTreeCaseSensitive(TreeNode * item, bool caseSensitive);

      void SetCaseSensitive(bool caseSensitive);
      void SetAllowWhiteSkipping(bool allow);
      void SetDefaultColor(TColor * defColor);

      LanguageDefinition();
      ~LanguageDefinition();

      virtual bool IsAl(wchar_t c);
      virtual bool IsAlNum(wchar_t c);
      virtual bool IsNum(wchar_t c);
      virtual bool IsWhite(wchar_t c);                                                               /*!< not used anywhere in project if I am not mistaken */

      TreeNode* GetTree();
      SearchIter GetDefSC(short id);

      //TreeNode* GetSpecItem(short id);                                                             /*!< returns item corresponding with mask (mask is actually just identifier atm */
  };
}
//---------------------------------------------------------------------------
#endif
