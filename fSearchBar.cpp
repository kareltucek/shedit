//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "fSearchBar.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TSearchBar *SearchBar;
//---------------------------------------------------------------------------
__fastcall TSearchBar::TSearchBar(TComponent* Owner)
  : TForm(Owner)
{
  success = false;
}
//---------------------------------------------------------------------------
void __fastcall TSearchBar::Button2Click(TObject *Sender)
{
  success = true;
  forward = true;
  cases = CheckBox1->Checked;
  text = Edit1->Text;
  this->Close();
}
//---------------------------------------------------------------------------
void __fastcall TSearchBar::Button1Click(TObject *Sender)
{
  success = true;
  forward = false;
  cases = CheckBox1->Checked;
  text = Edit1->Text;
  this->Close();
}
//---------------------------------------------------------------------------

void __fastcall TSearchBar::Edit1KeyPress(TObject *Sender, wchar_t &Key)
{
  if(Key == VK_RETURN)
    Button2->Click();
}
//---------------------------------------------------------------------------
