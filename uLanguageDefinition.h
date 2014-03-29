

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
   * Language highlight is actually done on a basis of a simple automaton, whose behaviour is  defined by this class. LanguageDefinition class itself is rather a wrapper, that holds general information - the actual specification is held by a set of trees of TreeNodes (creating sort of directed graph).
   *
   * Actual searching is done by passing an instance of SearchIter between Parser and LanguageDefinition, while LanguageDefinition provides language-related information and Parser interpret's them regarding the structure of source code. SearchIter contains a pointer to the base (/root) of current tree, a pointer to the current item and a "mask" that represents in fact a 16 bit register in which some information may be stored. Parser actually stores SearchIters in a stack that allows parser to return from a tree to a tree from which it got there.
   *
   * At the moment it is implemented in a very naive (and memory-inefficient) way - each TreeNode represents just one character and has an array of character's that allows constant-time acces to next node. That means that each part of dictionary has its root which represents an empty search from which strings of tree items actually form reserved words of the language represented. The Language definition then let's a pointer move along these strings as it is provided characters from parser. Every TreeNode has assigned one value of enum LangDefSpecType which represents what type of word ends and for some of these some additional information. The LangDefSpecTypes are:
   * - Empty - represents root of part of dictionary structure
   * - Nomatch - a place where nothing is matched, but from where search may continue to next character (i.e. middle of a reserved word)
   * - Normal - end of a standard word, that is to be formatted by the format the node points to (the search item is then pointed back to root)
   * - Jump - is matched as the Normal tag, but does not retur the search item back to root, but carries it further to a root of another tree. Before every jump, pop mask is checked, and if it matches, then pop is performed instead. Two optional masks can be specified:
   *   - a "jumpmask" - if it is specified, then jump occurs only if current searchIter's mask contains something of the jumpmask (== when jumpmask & currentmask returns true). 
   *   - a "newmask" - if jump is performed, the current searchIter's mask is xored with the newmask
   *   - a "freemask" - This one is not supposed to be used normally but it allows to explicitly clear current context. If this mask is set then before jump is performed, the target stack is popped while freemask matches the stack's top's mask.
   *   Each TreeNode can have more sets of the jump values assigned. In that case they are tried one by one in order in which the jump records were created until some jump is matched. Default values for masks are 0 and 0 which means "jump always and let the mask be as it is". Pop mask is not used in jump logic at all - its purpose here is to prevent jumping if a pop should be performed instead (and in that case regular pop is performed instead).
   * - PushPop - is implemented exactly the same way as jump but with a little difference - If jumpmask (abreviated also as pushmask) is matched, then jump is performed by Parser and former state is pushed onto stack, where it waits until a pop occurs. (then also the newmask value is added by xor to current mask)
   *   - Pop - each TreeNode has also a single popmask. If popmask is matched (as popmask & current mask) then Parser pops search iters from top of its stack according to popcount and continues parsing from the new top of the stack. If popcount is greater or equal to zero then it specifies the count of items to be popped. If popcount is -1 then items are popped while the popmask matches the top-of-the-stack's mask.
   *   Pop is always checked before push.
   * - LineTag - obsolete - At this point a jump is performed and current parserState is remembered until end of first line, where the parser state is forced back to the saved state. For standard linetag functionality it is recommended to use pair of jumps.
   * - WordTag - as normal tag, but formats everything until end of word (by standard convention that word begins by alpha and continues as alpha-numeric - these can be overriden). Written with regard i.e. for sql/php variables
   *
   *   Default tree (the entry point of automaton) can be obtained by GetTree() function.
   *
   *   case sensitivity - Case sensitivity is set separately for each tree root. You have to set it correctly BEFORE you start adding keywords (otherwise some of keywords may become completely unaccessible). The Langdef's SetCaseSensitive serves just for setting case sensitiviness of the newly created trees (i.e. it definitely won't do anything with the initially provided tree. 
   * 
   * Banks
   * --------
   * If you need different parts of your language to have separately treated stacks (i.e. entrypoint leads to tree of language A in whose context language B can be used to produce additional A code and you want to treat all recursively entered A scopes in the same context), then you may use Banks. Each stack has to have assigned its bank ID (which has to be retrieved by the GetNewBankID() except for the default 0). Then all trees with assigned bank IDs share one stack for the pushpop logic of the automaton. Then for each ID you have to set its default tree and a bank mask. The bank mask specifies which masks are used by that bank and practically is used for determining where pops do lead. On jump or push from a tree of one bank to tree of another bank, the Parser switches itself to stack of the target tree's bank without paying attention to actual target tree of the jump. The new mask is then set not just to the top of new stack, but to all its nodes. Pop works equivalently - the part of popmask that was matched to the current mask is checked against the current bank mask. If it matches then normal pop is performed. If it is not, then bank mask that does match is found and pop if performed back to that tree. 
   *
   * If you want to use masks, you need to specify BankID default bases, BankIDs and BankMasks for all trees. If you do not want to use it, then you do not need to care about everything - the default values will do.
   *
   * Formatting
   * ---------
   * Each root has it's own format that is applied to everything that was matched from the root. Every matched node (except for nomatch) then can have its own format that is added on top of the root's. You may leave some parts of format specification of key words NULL (then the root's format is applied).
   *
   * Formatting stores just pointers to colours. The colours themselves should be stored somewhere. From there you can change them at any time (you will just need to initiate screen redraw manually from TSHEdit).
   *
   * When creating a specification, you may imagine that TreeNode just specifies root for a part of entire dictionary with it's own private dictionary and default font style. Then you need to work with all dictionaries that you want to use for highlight and add jumps between them.
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
      LangDefSpecType Go(SearchIter * item, wchar_t c, bool & lookahead); /*!< Serves Parser for retrieving information about where to go further. Lookahead is set to true, if item was returned to empty and directly went to first unmatched character */

      void _AddPush(bool tobegin, wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask);
      void _AddJump(bool tobegin, wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask, short newmask, short freemask); /*!< Adds a custom jump from "at" tree to "to" tree.*/

      struct Jump
      {
        Jump(short _pushmask, short _newmask, short _newgmask, LangDefJumpType _type, TreeNode * _next);
        Jump();
        short pushmask;
        short newmask;
        short newgmask;
        short type;
        TreeNode * nextTree;
      };

      struct Pop
      {
        Pop(short _popmask, short _newgmask, short _popcount);
        Pop();
        short popmask;
        short popcount;
        short newgmask;
      };

    public:
      friend class Parser;

      struct TreeNode
      {
        TreeNode(wchar_t c, LangDefSpecType type, FontStyle * format, bool caseSensitive);
        TreeNode(TreeNode * tree, FontStyle * format);

        void AddJump(short pushmask, short newmask, short newgmask, LangDefJumpType type, TreeNode * to, bool begin);
        void AddPop(short popmask, short newmask, short popcount, bool begin);

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

#ifdef DEBUG
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


      void AddPops(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount = -2);  /*!< Just an abreviation. */
      void AddJumps(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short freemask = 0); /*!< Just an abreviation. */
      void AddJumpsFront(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short freemask = 0); /*!< Just an abreviation. */
      void AddPushes(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask);  /*!< Just an abreviation. */
      void AddPushesFront(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask);   /*!< Just an abreviation. */

      void AddKeywords(wchar_t * string, FontStyle * format, TreeNode * at = NULL); /*!< Adds all words that are contained in string (as space-separated list) to the tree given as "at". If tree is not given, then the main root is used */
      void AddJump(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short freemask = 0); /*!< Adds a custom jump from "at" tree to "to" tree. String is a space separated list.*/
      void AddJumpFront(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeNode * at, TreeNode * to, short jumpmask = 0, short newmask = 0, short freemask = 0); /*!< Like AddJump but adds new jumps to the beginning of jump list.*/
      void AddPush(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask); /*!< Adds pushes specified by string (as space separated list. */
      void AddPushFront(wchar_t * string, FontStyle * format, TreeNode * at, TreeNode * to, short pushmask, short newmask);
      void AddPop(wchar_t * string, FontStyle * format, TreeNode * at, short popmask, short popcount = -2, short newgmask = 0);
      void AddWord(wchar_t * string, FontStyle * format, TreeNode * at = NULL); /*!< Adds a wordtag item (i.e. for highlighting php variables as $test just by $) */
      TreeNode * AddLine(wchar_t * string, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); /*!< Adds a linetag item - as c commenting //. Returns new tree that was created for the line's formatting. Is an abbreviation for double jump. */
      TreeNode * AddLineStrong(wchar_t * string, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); /*!< as AddLine, but stores entire state of parser and at the end of line it restores it back. */
      TreeNode * AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at = NULL, TreeNode * to = NULL); /*!< Is just an abreviation for two jumps. Adds jump from "opening" tag at "at" tree to newly created tree and then corresponding jump back */
      TreeNode * AddPushPopPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeNode * at, TreeNode * to, short mask); /*!< Is just an abreviation for push that adds the mask and pop that is conditioned by the same mask.*/
      TreeNode * AddNewTree(FontStyle * format); /*!< Just creates and returns an empty new tree with format as the default format */
      TreeNode * AddNewTree(FontStyle * format, bool caseSensitive); /*!< Just creates and returns an empty new tree with format as the default format */
      TreeNode * AddDupTree(TreeNode * tree, FontStyle * format); /*!< Duplicates the base of tree provided with the format provided. All pointers remain intact - one tree may have more than one bases. By making changes that are deeper than the duplicated TreeNode's member pointers, then you are making changes to both trees. */

      void SetTreeCaseSensitive(TreeNode * item, bool caseSensitive);

      void SetCaseSensitive(bool caseSensitive);
      void SetAllowWhiteSkipping(bool allow);
      void SetDefaultColor(TColor * defColor);

      LanguageDefinition();
      ~LanguageDefinition();

      virtual bool IsAl(wchar_t c);
      virtual bool IsAlNum(wchar_t c);
      virtual bool IsNum(wchar_t c);
      virtual bool IsWhite(wchar_t c); /*!< not used anywhere in project if I am not mistaken */

      TreeNode* GetTree();
      SearchIter GetDefSC(short id);

      //TreeNode* GetSpecItem(short id); /*!< returns item corresponding with mask (mask is actually just identifier atm */
  };
}
//---------------------------------------------------------------------------
#endif
