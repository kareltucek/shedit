#include <iostream>
#define TESTBED
#include "uLanguageDefinition2.h"

using namespace SHEdit;

class TestParser : public LanguageDefinition
{
  public:
    TestParser();
};

TestParser::TestParser() : LanguageDefinition()
{
  int i = 1;
  AddTerm(L"IDENTIF", NULL, L"[a-zA-Z][a-zA-Z0-9_-]*", ++i, 0);
  AddTerm(L"TYPE", NULL,    L"(int|bool|char|string)", ++i, 0);
  AddTerm(L"OP", NULL,      L"[+*/-]", ++i, 0);
  AddTerm(L"ASS", NULL,     L"=", ++i, 0);
  AddTerm(L"NUM", NULL,     L"[0-9]+", ++i, 0);
  AddTerm(L"SEMIC", NULL,   L";", ++i, 0);
  AddTerm(L"WHITE", NULL,   L"[ \t\n\r]+", ++i, 0);

  AddNonTerm(L"program", NULL, L"decl|ass", ++i, 0);
  AddNonTerm(L"decl", NULL, L"TYPE IDENT SEMIC", ++i, 0);
  AddNonTerm(L"oper", NULL, L"num OP oper", ++i, 0);
  AddNonTerm(L"ass", NULL, L"IDENTIF ASS oper SEMIC | IDENTIF ASS IDENTIF SEMIC", ++i, 0);
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
    p.Parse<std::wstring::iterator>(b, c, ps, dc, (SHEdit::FontStyle*&)dc2,dc);
    std::wcout << std::wstring(a,b) << std::endl;
    a = b;
  }

  return 0;
}

