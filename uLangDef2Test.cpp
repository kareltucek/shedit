#include <iostream>
#define TESTBED
#include "uLanguageDefinition2.h"

using namespace SHEdit;

class TestParser : public LanguageDefinition
{
  public:
    TestParser(): LanguageDefinition(){};
};

TestParser::TestParser() : LanguageDefinition()
{
  int i = 1;
  AddTerm("IDENTIF", NULL, "[a-zA-Z][a-zA-Z0-9_-]*", ++i, 0);
  AddTerm("TYPE", NULL, "(int|bool|char|string)", ++i, 0);
  AddTerm("OP", NULL, "[+*/-]", ++i, 0);
  AddTerm("ASS", NULL, "=", ++i, 0);
  AddTerm("NUM", NULL, "[0-9]+", ++i, 0);
  AddTerm("SEMIC", NULL, ";", ++i, 0);
  AddTerm("WHITE", NULL, "[ \t\n\r]+", ++i, 0);

  AddNonTerm("program", NULL, "decl|ass", ++i, 0);
  AddNonTerm("decl", NULL, "TYPE IDENT SEMIC", ++i, 0);
  AddNonTerm("oper", NULL, "num OP oper", ++i, 0);
  AddNonTerm("ass", NULL, "IDENTIF ASS oper SEMIC | IDENTIF ASS IDENTIF SEMIC", ++i, 0);
}

int main()
{
  std::wstring str = L"";
  str.append(L"int a;");
  str.append(L"int b;");
  str.append(L"bool c;");
  str.append(L"a = b;");
  str.append(L"a = 1 + 5 / 4;");

  TestParser p;
  LanguageDefinition::PState ps;

  std::wstring::iterator a = str.begin(), b = str.begin(),c = str.end();

  while(c != b)
  {
    bool dc;
    class FontStyle;
    FontStyle* dc2;
    p.Parse<std::wstring::iterator>(b, c, ps, dc, (SHEdit::FontStyle*&)dc2);
    std::wcout << std::wstring(a,b) << std::endl;
    a = b;
  }

  return 0;
}

