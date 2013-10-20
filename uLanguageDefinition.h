

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
  enum LangDefSpecType{Empty, Nomatch, Normal, PairTag, WordTag, LineTag};
  //---------------------------------------------------------------------------
  class LanguageDefinition
  {
    public:
      struct TreeItem
      {
        TreeItem(wchar_t c, LangDefSpecType type, FontStyle * format);

        wchar_t thisItem;
        std::list<TreeItem*> items;
        FontStyle * format;
        LangDefSpecType type;
        wchar_t * pair;
        short mask;
      };

    private:
      bool caseSensitive;
      FontStyle * defFormat;
      TreeItem * tree;
      std::list<TreeItem*> specItems;
      short masksUsed;

      TreeItem* FindOrCreateItem(TreeItem * item, wchar_t c);
      TreeItem* FindOrCreateDefinition(wchar_t c);
    protected:
      void AddReservedNames(wchar_t * string, TColor * color); 
      void AddSpecial(wchar_t * opening, wchar_t * closing, LangDefSpecType type, TColor * color);
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
