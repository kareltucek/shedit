

//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionH
#define uLanguageDefinitionH

#include <list>
#include <vcl.h>
//---------------------------------------------------------------------------
namespace SHEdit
{
  /*!
   * LanguageDefinition
   * ------------------
   * LanguageDefinition is a class that provides Parser definitions of what is to be how parsed and interpretted. Entire dictionary consists of a tree structure of TreeItem, that hold entire structure of the dictionary - the LanguageDefinition is rather just a wrapper that simplifies representation of entire structure and provides functions for building of the tree structure.
   *
   * The dictionary is at the moment implemented in a very naive (and inefficient) way - each TreeItem represents just one character and has an array of character's that allows constant-time acces to the next node. That means that each part of dictionary has it's root which represents an empty search from which strings of tree items actually form reserved words of the language represented. The Language definition then let'đ pointer move along these strings as it is provided characters from parser. Every tree has assigned one value of enum LangDefSpecType which represents what type of word ends at that point and a pointer back to the root of tree. The LangDefSpecTypes are:
   * - Empty - represents root of part of dictionary structure
   * - Nomatch - a place where nothing is matched, but from where search may continue to next character (i.e. middle of a reserved word)
   * - Normal - end of a standard word, that is to be formatted by the format the node points to (the search item is then pointed back to root)
   * - PairTag - better word would be "Jump" - is matched as the Normal tag, but does not retur the search item back to root, but carries it further to a root of another tree. The pair tag then refers to two different jump tokens that do lead in ine direction to another tree, from where another token leads back
   * - LineTag - as PairTag leads to another tree from where the search token is returned back at an end of line (by Parser)
   * - WordTag - as normal tag, but formats everything until end of word (by standard convention that word begins by alpha and continues as alpha-numeric - these can be overriden). Written with regard i.e. for sql/php variables
   *
   * At the moment there's no way of passing more advanced matching conditions (jumps are one-directional - when jump is done, then only way to return is to use another jump, which though cannot distinguished whether it is jumping to the original tree or somewhere else. Should be reimplemented some day in future as a pair of tree identification and mask that would be stored as stack in parser's state. Though this should work quite well as long as the highlight structure is not too complex and as long as the language that is being highlighted is well structured (i.e. jump tokens are unique).
   *
   * When creating a specification, you may imagine that TreeItem just specifies basic root for a part of dictionary with it's own private dictionary and default font style. Then you need to work with all dictionaries that you want to use for highlight and add jumps between them.
   * */
  class FontStyle;

  enum LangDefSpecType{Empty = 0x1, Nomatch = 0x2, Normal=0x4, Jump=0x8, PushPop=0x10, WordTag=0x20, LineTag=0x40, Lookahead = 0x80};
  //---------------------------------------------------------------------------
  class LanguageDefinition
  {
    public:
      struct Jump;
      struct TreeItem;
      struct SearchIt;

    private:
      bool caseSensitive;
      FontStyle * defFormat;
      TreeItem * tree;

      TreeItem* FindOrCreateItem(TreeItem * item, wchar_t c, TreeItem * at);

    public:
      struct Jump
      {
        Jump(short _pushmask, short _newmask, TreeItem * _next);
        Jump();
        short pushmask;
        short newmask;
        TreeItem * nextTree;
      };

      struct TreeItem
      {
        TreeItem(wchar_t c, LangDefSpecType type, FontStyle * format);

        void AddJump(short pushmask, short newmask, TreeItem * to);

        wchar_t thisItem;
        std::list<TreeItem*> items;
        TreeItem * map[128];
        FontStyle * format;
        LangDefSpecType type;
        //TreeItem * nextTree;

        short jumpcount;
        Jump * jumps; //an array

        short popmask;
        short popcount;
      };

      struct SearchIt
      {
        TreeItem * current;
        TreeItem * base;
        short mask;

        bool operator==(const SearchIt& sit);
        bool operator!=(const SearchIt& sit);
      };

      typedef SearchIt SearchIter;

      void AddReservedNames(wchar_t * string, FontStyle * format, TreeItem * at = NULL); /*!< Adds all words that are contained in string (as space-separated list) to the tree given as "at". If tree is not given, then the main root is used */
      void AddJump(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeItem * at, TreeItem * to); /*!< Adds a custom jump from "at" tree to "to" tree.*/
      void AddPush(wchar_t * string, FontStyle * format, TreeItem * at, TreeItem * to, short pushmask, short newmask);
      void AddPop(wchar_t * string, FontStyle * format, TreeItem * at, short popmask);
      void AddWord(wchar_t * string, FontStyle * format, TreeItem * at = NULL); /*!< Adds a wordtag item (i.e. for highlighting php variables as $test just by $) */
      TreeItem * AddLine(wchar_t * string, FontStyle * format, TreeItem * at = NULL, TreeItem * to = NULL); /*!< Adds a linetag item - as c commenting //. Returns new tree that was created for the line's formatting */
      TreeItem * AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeItem * at = NULL); /*!< Is just an abreviation for two jumps. Adds jump from "opening" tag at "at" tree to newly created tree and then corresponding jump back */
      TreeItem * AddNewTree(FontStyle * format); /*!< Just creates and returns an empty new tree with format as the default format */


      void SetCaseSensitive(bool caseSensitive);
      void SetDefaultColor(TColor * defColor);

      LanguageDefinition();
      ~LanguageDefinition();

      virtual bool IsAl(wchar_t c);
      virtual bool IsAlNum(wchar_t c);
      virtual bool IsNum(wchar_t c);
      virtual bool IsWhite(wchar_t c); /*!< not used anywhere in project if I am not mistaken */

      TreeItem* GetTree();
      SearchIter GetDefSC();
      LangDefSpecType Go(SearchIter& item, wchar_t c, bool & lookahead); /*!< Serves Parser for retrieving information about where to go further. Lookahead is set to true, if item was returned to empty and directly went to first unmatched character */

      //TreeItem* GetSpecItem(short id); /*!< returns item corresponding with mask (mask is actually just identifier atm */
  };
}
//---------------------------------------------------------------------------
#endif
