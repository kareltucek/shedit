//---------------------------------------------------------------------------

#ifndef fSearchBarH
#define fSearchBarH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TSearchBar : public TForm
{
__published:	// IDE-managed Components
  TEdit *Edit1;
  TLabel *Label1;
  TCheckBox *CheckBox1;
  TButton *Button1;
  TButton *Button2;
  void __fastcall Button2Click(TObject *Sender);
  void __fastcall Button1Click(TObject *Sender);
  void __fastcall Edit1KeyPress(TObject *Sender, wchar_t &Key);
  private:	// User declarations
  public:		// User declarations
  bool success;
  bool forward;
  bool cases;
  String text;
  __fastcall TSearchBar(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TSearchBar *SearchBar;
//---------------------------------------------------------------------------
#endif
