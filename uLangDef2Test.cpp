#include <iostream>
#define TESTBED
#include "uLanguageDefinition2.h"

using namespace SHEdit;

class TestParser : public LanguageDefinition
{
  public:
    TestParser(): LanguageDefinition(){};
};

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

