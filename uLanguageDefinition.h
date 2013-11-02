

//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionH
#define uLanguageDefinitionH

#include <list>
#include <vcl.h>
//---------------------------------------------------------------------------
//this class is meant to be inherited by new definition whose constructor
//then initializes all definitions needed for parsing of language by 
//AddReservedNames (space separated list) and AddSpecial
//
//Amount of special tags is limited to ~14
//
//you may also want to override definitions of whitechars or definitions of 
//alphabet characters - parser uses language definition defined functions.
//---------------------------------------------------------------------------
namespace SHEdit
{
  class FontStyle;
  enum LangDefSpecType{Empty, Nomatch, Normal, PairTag, WordTag, LineTag, NoEmpty};
  //---------------------------------------------------------------------------
  class LanguageDefinition
  {
    public:
      struct TreeItem
      {
        TreeItem(wchar_t c, LangDefSpecType type, FontStyle * format, LanguageDefinition::TreeItem * at);

        wchar_t thisItem;
        std::list<TreeItem*> items;
        TreeItem * map[128];
        FontStyle * format;
        LangDefSpecType type;
        TreeItem * nextTree;
        short mask;
      };

    private:
      bool caseSensitive;
      FontStyle * defFormat;
      TreeItem * tree;
      std::list<TreeItem*> specItems;
      short masksUsed;

      TreeItem* FindOrCreateItem(TreeItem * item, wchar_t c, TreeItem * at);
    protected:
      void AddReservedNames(wchar_t * string, FontStyle * format, TreeItem * at = NULL);
      void AddJump(wchar_t * string, FontStyle * format, LangDefSpecType type, TreeItem * at, TreeItem * to);
      void AddWord(wchar_t * string, FontStyle * format, TreeItem * at = NULL);
      TreeItem * AddLine(wchar_t * string, FontStyle * format, TreeItem * at = NULL, TreeItem * to = NULL);
      TreeItem * AddPair(wchar_t * opening, wchar_t * closing, FontStyle * format, TreeItem * at = NULL);
      TreeItem * AddNewTree(FontStyle * format);

      void SetCaseSensitive(bool caseSensitive);
      void SetDefaultColor(TColor * defColor);

    public:

      __fastcall LanguageDefinition();
      ~LanguageDefinition();

      virtual bool IsAl(wchar_t c);
      virtual bool IsAlNum(wchar_t c);
      virtual bool IsNum(wchar_t c);
      virtual bool IsWhite(wchar_t c);

      TreeItem* GetTree();
      LangDefSpecType Go(TreeItem *& item, wchar_t c);

      TreeItem* GetSpecItem(short mask);
  };
}
//---------------------------------------------------------------------------
#endif
