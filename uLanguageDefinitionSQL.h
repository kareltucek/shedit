//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionSQLH
#define uLanguageDefinitionSQLH

#include <list>
#include <vcl.h>
#include "uLanguageDefinition.h"

namespace SHEdit
{
  /*!
   * This is the simplest example of how to construct lexer for a SQL language. You may probably want to store formats in separate configuration object...
   *
   * For details of how to use these functions see LanguageDefinition's members method's documentation.
   * */
  class LanguageDefinitionSQL : public LanguageDefinition
  {
    public:
      LanguageDefinitionSQL();
  };
}
//---------------------------------------------------------------------------
#endif
