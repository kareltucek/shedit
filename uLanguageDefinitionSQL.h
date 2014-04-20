//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionSQLH
#define uLanguageDefinitionSQLH

#include <list>
#include <vcl.h>
#include "uLanguageDefinition.h"

namespace SHEdit
{
  /*!
   * SQL example class
   * -----------------
   * This is the simplest example of how to construct lexer for a SQL language. You may probably want to store formats in separate configuration object...
   *
   * For details of how to use these functions see LanguageDefinition's members method's documentation.
   * */
  class LanguageDefinitionSQL : public LanguageDefinition
  {

    TColor foreground;
    TColor number ;
    TColor comment;
    TColor commentback;
    TColor string ;
    TColor variable;
    TColor reserved;
    TColor preprocessor;
    TColor background;

    public:
    LanguageDefinitionSQL();
  };
}
//---------------------------------------------------------------------------
#endif
