//---------------------------------------------------------------------------

#ifndef uLanguageDefinitionSQLH
#define uLanguageDefinitionSQLH

#include <list>
#include <vcl.h>
#include "uLanguageDefinition.h"
//This is an example language definition. Except for what you see here instance of this object must be passed to parser by SetLangDef() function in order to work.
//Language definitions may be managed by component that uses them. Thus you may need also to add hook in component.
namespace SHEdit
{
  //---------------------------------------------------------------------------
  class LanguageDefinitionSQL : public LanguageDefinition
  {
    public:
      LanguageDefinitionSQL();
  };
}
//---------------------------------------------------------------------------
#endif
