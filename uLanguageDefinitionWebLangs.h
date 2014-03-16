//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionWebLangsH
#define uLanguageDefinitionWebLangsH

#include <list>
#include <vcl.h>
#include "uLanguageDefinition.h"

#define MASK_APOSTROPHE 1
#define MASK_QUOTE 2
#define MASK_DOUBLE_QUOTE 4
#define MASK_ECHO_MODE 8
#define MASK_PHP 16
#define MASK_ECHO_IGNORE 32
#define MASK_PHP_APOSTROPHE 64
#define MASK_PHP_QUOTE 128
#define MASK_PREDICTION 256

#define POP_AUTO -1
#define PUSH_ALWAYS 0

namespace SHEdit
{

  /*!
   * Web Languages mor advanced example
   * ----------------------------------
   * This class was written as example of hot the automaton logic can be used to parse a bit advanced structure of highlighting.
   *
   * As example was used a group of web - oriented languages, namely html, php, css and javascript (also sql should have been here but I am terribly lazy), because of the mess one can do with 5 different languages in one file. One thing to notice is that html+jv+css are handled through different prints of php as if those were in the same context. It is principially wrong, but on average code it works very nicely and well illustrates the way the dictionaries work. 
   * */
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

      TColor jvFG;
      TColor jvBG;
      TColor jvComments;
      TColor jvQuotes;
      TColor jvKeywords;

      TreeNode * htmlTree;
      TreeNode * htmlCommentTree;
      TreeNode * htmlTagTree;
      TreeNode * htmlQuoteTree;

      TreeNode * phpTree;
      TreeNode * phpCommentTree;
      TreeNode * phpLineCommentTree;
      TreeNode * phpHtmlEnterTree;
      TreeNode * phpQuoteTree;

      TreeNode * cssPropertyTree;
      TreeNode * cssValueTree;
      TreeNode * cssEnterTree;
      TreeNode * cssTree;
      TreeNode * cssQuoteTree;

      TreeNode * jvTree;
      TreeNode * jvEnterTree;
      TreeNode * jvCommentTree;
      TreeNode * jvLineCommentTree;
      TreeNode * jvQuoteTree;

      void ConstructHtml(TreeNode * at, TColor * bg, TColor * fg);
      void ConstructPhp(TreeNode * at,  TColor * bg, TColor * fg);
      void ConstructCss(TreeNode * at,  TColor * bg, TColor * fg);
      void ConstructJavascript(TreeNode * at, TColor * bg, TColor * fg);

      void AddSafetyJumps();

      void AddPops(TreeNode * at);



    public:
      LanguageDefinitionWebLangs();
  };
}
//---------------------------------------------------------------------------
#endif
