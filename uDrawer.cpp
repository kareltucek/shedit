
//---------------------------------------------------------------------------
#pragma hdrstop


#include "uDrawer.h"
#include "cSQLSyntax.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>
#include <list>
#include <windows.h>

using namespace SHEdit;

#ifdef DEBUG
#include <fstream>
std::ofstream myfile;
#endif

#pragma package(smart_init)
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall Drawer::Drawer(TCanvas * canvas, TSQLEdit * parent, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending)
{
#ifdef DOUBLE_BUFFERED
  bitmap = new Graphics::TBitmap();
  bitmap->SetSize(parent->Width, parent->Height);

  drawcanvas = bitmap->Canvas;
#else
  drawcanvas = canvas;
#endif
  this->parent = parent;
  this->canvas = canvas;

  linenumsenabled = true;

  drawcanvas->Font->Name = "Courier New";
  canvas->Font->Name = "Courier New";
  SetFontsize(DEFONTSIZE);
  UpdateLinenumWidth(1);

  HMax = 200;
  HPos = 0;
  x = 2;
  y = Y_OFF;
  cx = 2+GetLinenumWidth();
  cy = 2;
#ifdef DEBUG
  myfile.open("drawer.txt", ios::out );
#endif
}

//---------------------------------------------------------------------------

void __fastcall Drawer::DrawText(String text, bool newline, short linenum, FontStyle format)
{

  y = Y_OFF+linesize*linenum;
  if(newline)
  {

#ifdef DEBUG
    // Write("\n");
#endif
    HPos = parent->HBar->Position;
    x = 2-HPos+GetLinenumWidth();
  }

  if(format.foreground != NULL)
    drawcanvas->Font->Color = *format.foreground;
  if(format.background != NULL)
    drawcanvas->Brush->Color = *format.background;

#ifdef DEBUG
  /*  //drawcanvas->Rectangle(x, y,x+drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text)), y+linesize);
      Sleep(10);
      DrawTaskText *dbg = ((DrawTaskText*)tasktoprocess);
      Write((((DrawTaskText*)tasktoprocess)->format.background != NULL && *(((DrawTaskText*)tasktoprocess)->format.background) != (TColor)0xddffdd ? String("!") : String(" ")) + String((int)drawcanvas->Brush->Color) +String(" ")+ String(y) + String("(")+ String(x) + String(")<")+ String(drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text)))+ String(">:")+ *(((DrawTaskText*)tasktoprocess)->text));
      */
#endif
  drawcanvas->TextOut(x, y, text);
  x = drawcanvas->PenPos.x;


}
//---------------------------------------------------------------------------

void __fastcall Drawer::DrawMove(int from, int to, int by)
{
#define FROM from
#define TO to
#define BY by
  {
    int adjustment = linesize*(TO+1) > BottomBorder() ? BottomBorder() - linesize*(TO+1)-2 : -2;
    TRect source2(from == 0 ? 0 : GetLinenumWidth(), 0, 0, 1);
    TRect source(from == 0 ? 0 : GetLinenumWidth(), linesize*FROM, RightBorder(), linesize*(TO+1)+adjustment);
    TRect dest(from == 0 ? 0 : GetLinenumWidth(), linesize*(FROM+BY), RightBorder(), linesize*(TO+BY+1)+adjustment);
    drawcanvas->CopyRect(dest, drawcanvas, source);
    if(BY < 0)
    {
      drawcanvas->Pen->Color = (TColor)0xFFFFFF;
      drawcanvas->Brush->Color = (TColor)0xFFFFFF;
      drawcanvas->Rectangle(0, linesize*(TO+BY),RightBorder(), BottomBorder());
    }
  }
}
//---------------------------------------------------------------------------

void __fastcall Drawer::DrawEof(short linenum)
{
  drawcanvas->Pen->Color = (TColor)0xFFFFFF;
  y = Y_OFF+linesize*linenum;
  drawcanvas->Rectangle(0, y,RightBorder(), BottomBorder());
  if(linenumsenabled)
    DrawLinenum(linenum);
}
//---------------------------------------------------------------------------

void __fastcall Drawer::UpdateCursor(int x, int y)
{
  con = false;
  DrawCursor(); //this one here deletes the last one
  cx = x;
  cy = y;
  //cursorBGcolor = canvas->Pixels[cx][cy];
}

//---------------------------------------------------------------------------
void __fastcall Drawer::DrawResize(int w, int h)
{
#ifdef DOUBLE_BUFFERED
  bitmap->SetSize(RightBorder(), BottomBorder());
#endif
}

//---------------------------------------------------------------------------
void __fastcall Drawer::DrawEndl(short linenum)
{
#ifndef FULL_WIDTH_PAINT
  drawcanvas->Pen->Color = (TColor)0xFFFFFF;
  drawcanvas->Brush->Color = (TColor)0xFFFFFF;
#else
  drawcanvas->Pen->Color = drawcanvas->Brush->Color;
#endif
  y = Y_OFF+linesize*linenum;
  drawcanvas->Rectangle(x == 2 ? 0 : x, y,RightBorder(), y+linesize);
  if(x+HPos-2+200 > HMax)
  {
    HMax = x+HPos-2+200;
    parent->UpdateHBar();
  }
  HPos = parent->HBar->Position;
  x = 2-HPos;

  if(linenumsenabled)
  {
    drawcanvas->Font->Color = (TColor)0x0;
    drawcanvas->Pen->Color = (TColor)0xFFFFFF;
    drawcanvas->Brush->Color = (TColor)0xFFFFFF;
    drawcanvas->Rectangle(0, y, GetLinenumWidth(), y+linesize);
    drawcanvas->TextOut(2, y, String(parent->itrLine->linenum+linenum));
    drawcanvas->MoveTo(GetLinenumWidth(), y);
    drawcanvas->Pen->Color = (TColor)0x0;
    drawcanvas->LineTo(GetLinenumWidth(), y+linesize);
  }
}
//---------------------------------------------------------------------------


void __fastcall Drawer::Paint()
{
  con = !con;
  con = true;
#ifdef DOUBLE_BUFFERED
  canvas->Draw(0, 0, bitmap);
#endif
  DrawCursor();

#ifdef DRAW_LINENUM
  //DrawLinenum();
#endif

  parent->VBar->Repaint();
  parent->HBar->Repaint();

#ifdef DRAW_POS
  canvas->Brush->Style = bsClear;
  String str = String(parent->itrCursor->linenum)+String(" ")+String(parent->itrCursor->pos);
  canvas->TextOut(RightBorder() - canvas->TextWidth(str), BottomBorder() - linesize, str);
  canvas->Brush->Style = bsSolid;
#endif
}

//---------------------------------------------------------------------------
void __fastcall Drawer::UpdateHBar()
{
  parent->UpdateHBar();
}
//---------------------------------------------------------------------------
void Drawer::DrawCursor()
{
  if(con)
    canvas->Pen->Color = clBlack;
  else
#ifdef DOUBLE_BUFFERED
    return;
#else
  canvas->Pen->Color = clWhite;
#endif
  canvas->MoveTo(cx-HPos, cy);
  canvas->LineTo(cx-HPos, cy+linesize-1);
  canvas->Pen->Color = clBlack;
}
//---------------------------------------------------------------------------
int Drawer::RightBorder()
{
  return  parent->VBar->Visible ? parent->Width - 16 : parent->Width;
}
//---------------------------------------------------------------------------
int Drawer::BottomBorder()
{
  return  parent->HBar->Visible ? parent->Height - 17 : parent->Height;
}
//---------------------------------------------------------------------------
void __fastcall Drawer::DrawLinenum(int from)
{
  drawcanvas->Font->Color = (TColor)0x0;
  drawcanvas->Pen->Color = (TColor)0xFFFFFF;
  drawcanvas->Brush->Color = (TColor)0xFFFFFF;
  drawcanvas->Rectangle(0, from*linesize, GetLinenumWidth(), parent->Height);
  for(int i = parent->Height/linesize, j = from; j <= i; j++)
    drawcanvas->TextOut(2, j*linesize, String(parent->itrLine->linenum+j));

  drawcanvas->Pen->Color = (TColor)0x0;
  drawcanvas->MoveTo(GetLinenumWidth(), 0);
  drawcanvas->LineTo(GetLinenumWidth(), parent->Height);

}
//---------------------------------------------------------------------------
void __fastcall Drawer::SetFontsize(int size)
{
  canvas->Font->Size = size;
  drawcanvas->Font->Size = size;
  fontsize = size;
  //find out real height
  Graphics::TBitmap * bitmap = new Graphics::TBitmap();
  bitmap->SetSize(1, 1000);
  bitmap->Canvas->Brush->Color = clGreen;
  bitmap->Canvas->Font->Color = clGreen;
  bitmap->Canvas->Font->Size = size;
  bitmap->Canvas->Font->Name = "Courier New";
  bitmap->Canvas->TextOut(0, 0, " ");
  int i = 0;
  TColor g = clGreen;
  while( bitmap->Canvas->Pixels[0][i+1] == clGreen)
    i++;
  linesize = i+1;
  delete bitmap;

  drawcanvas->Font->Name = "Courier New";
  canvas->Font->Name = "Courier New";
}
//---------------------------------------------------------------------------
int __fastcall Drawer::GetFontsize()
{
  return fontsize;
}
//---------------------------------------------------------------------------
int __fastcall Drawer::GetLinesize()
{
  return linesize;
}
//---------------------------------------------------------------------------
bool __fastcall Drawer::UpdateLinenumWidth(int count)
{
  if(count < 100)
    count = 100;
  String str(count);
  int w = drawcanvas->TextWidth(count)+5; //2*2 margins + 1 line width
  bool ret = linenumwidth != w;
  linenumwidth = w;
  return ret;
}
//---------------------------------------------------------------------------
int __fastcall Drawer::GetLinenumWidth()
{
  return linenumsenabled ? linenumwidth : 0;
}//---------------------------------------------------------------------------
void __fastcall Drawer::SetLinenumsEnabled(bool enable)
{
  linenumsenabled = enable;
}
//---------------------------------------------------------------------------
__fastcall Drawer::~Drawer()
{
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void Drawer::Write(AnsiString message)
{
#ifdef DEBUG_LOGGING
  myfile << message.c_str();
#endif
}
#endif
//---------------------------------------------------------------------------
