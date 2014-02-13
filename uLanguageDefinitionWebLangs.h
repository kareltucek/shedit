//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionWebLangsH
#define uLanguageDefinitionWebLangsH

#include <list>
#include <vcl.h>
#include "uLanguageDefinition.h"

#define MASK_APOSTROPHE 1
#define MASK_QUOTE 2
#define MASK_DOUBLE_QUOTE 4

namespace SHEdit
{
  class LanguageDefinitionWebLangs : public LanguageDefinition
  {
    private:
      TColor htmlBG;
      TColor htmlFG;
      TColor htmlTags;
      TColor htmlQuotes;
      TColor htmlComments;
      TColor htmlAttribs;

      TColor phpBG;
      TColor phpFG;
      TColor phpKeywords;
      TColor phpReserved;
      TColor phpVariable;
      TColor phpQuotes;
      TColor phpQuotesBack;
      TColor phpComments;

      TColor cssFG;
      TColor cssBG;
      TColor cssAttr;
      TColor cssVal;

      void ConstructHtml(TreeItem * at, TreeItem *& commentTree, TreeItem *& tagTree, TColor * bg, TColor * fg);
      void ConstructPhp(TreeItem * at, TreeItem *& commentTree, TColor * bg, TColor * fg);
      void ConstructCss(TreeItem * at, TreeItem *& propertyTree, TreeItem *& valueTree, TColor * bg, TColor * fg);
      void AddPops(TreeItem * at);



    public:
      LanguageDefinitionWebLangs();
  };
}
//---------------------------------------------------------------------------
#endif
