
//---------------------------------------------------------------------------
#pragma hdrstop


#include "uDrawer.h"
#include "cSHEdit.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>
#include <list>
#include <windows.h>

using namespace SHEdit;

#ifdef SHEDIT_DEBUG_LOGGING
#include <fstream>
std::wofstream myfiledraw;
#endif

#pragma package(smart_init)
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall Drawer::Drawer(TCanvas * canvas, TSHEdit * parent)
{
#ifdef SHEDIT_DEBUG_LOGGING
  if(!myfiledraw.is_open())
    myfiledraw.open("drawer.txt", ios::out );
#endif

#ifdef DOUBLE_BUFFERED
  bitmap = new Graphics::TBitmap();
  bitmap->SetSize(parent->Width, parent->Height);

  drawcanvas = bitmap->Canvas;
  drawcanvas->Font->Quality = fqClearTypeNatural;
#else
  drawcanvas = canvas;
#endif
  this->parent = parent;
  this->canvas = canvas;

  linenumsenabled = true;
  linesize = 11;
  fontsize = DEFONTSIZE;

  drawcanvas->Font->Name = "Courier New";
  canvas->Font->Name = "Courier New";
  SetFontsize(fontsize);
  UpdateLinenumWidth(1);

  HMax = 200;
  HPos = 0;
  x = X_OFF;
  y = Y_OFF;
  cx = X_OFF  + (linenumsenabled ? linenumwidth : 0);
  cy = 0;

}

//---------------------------------------------------------------------------

void __fastcall Drawer::DrawText(String text, bool newline, short linenum, FontStyle format)
{
#ifdef SHEDIT_DEBUG
  TColor bg = *format.background;
  TColor fg = *format.foreground;
  assert(linenum < parent->Height/GetLinesize() + 5);
#endif

  if (bitmap->Width == 0)      //the initial resize signal gets lost sometimes :/
    bitmap->SetSize(RightBorder(), BottomBorder());

  y = Y_OFF+linesize*linenum;
  if(newline)
  {
    HPos = parent->HBar->Position;
    x = X_OFF-HPos+GetLinenumWidth();
  }

  if(x > parent->Width)
  {
    x += drawcanvas->TextWidth(text);
    return;
  }
#ifndef QUICKDRAW
  if(true || newline || format.background == NULL || *format.background != drawcanvas->Brush->Color )
  {
    if(format.background != NULL)
    {
      drawcanvas->Pen->Color = *format.background;
      drawcanvas->Brush->Color = *format.background;
    }
    else
    {
      drawcanvas->Pen->Color = (TColor)0xFFFFFF;
      drawcanvas->Brush->Color = (TColor)0xFFFFFF;
    }
    drawcanvas->Rectangle(x > linenumwidth+3 ? x+1 : x-1, y, parent->Width, y+linesize);
  }
#endif

  if(format.foreground != NULL)
    drawcanvas->Font->Color = *format.foreground;
  if(format.background != NULL)
    drawcanvas->Brush->Color = *format.background;
  drawcanvas->Font->Style = format.style;
#ifndef QUICKDRAW
  drawcanvas->Brush->Style = bsClear;
#endif

  drawcanvas->TextOut(x, y, text);
  x = drawcanvas->PenPos.x;

#ifndef QUICKDRAW
  drawcanvas->Brush->Style = bsSolid;
#endif
#ifdef SHEDIT_DEBUG_COPY_DRAW
  StressTest();
#endif

}
//---------------------------------------------------------------------------

void __fastcall Drawer::DrawMove(int from, int to, int by)
{
#define FROM from
#define TO to
#define BY by
    int adjustment = linesize*(TO+1) > BottomBorder() ? BottomBorder() - linesize*(TO+1)-X_OFF : -X_OFF;
    TRect source2(from == 0 ? 0 : GetLinenumWidth(), 0, 0, 1);
    TRect source(from == 0 ? 0 : GetLinenumWidth(), linesize*FROM, RightBorder(), linesize*(TO+1)-adjustment);
    TRect dest(from == 0 ? 0 : GetLinenumWidth(), linesize*(FROM+BY), RightBorder(), linesize*(TO+BY+1)-adjustment);
    drawcanvas->CopyRect(dest, drawcanvas, source);
    if(BY < 0)
    {
      drawcanvas->Pen->Color = (TColor)0xFFFFFF;
      drawcanvas->Brush->Color = (TColor)0xFFFFFF;
      drawcanvas->Rectangle(0, linesize*(TO+BY),RightBorder(), BottomBorder());
    }
#ifdef SHEDIT_DEBUG_COPY_DRAW
  drawcanvas->Pen->Color = clBlack;
  drawcanvas->Pen->Width = 2;
  drawcanvas->Brush->Style = bsClear;
  drawcanvas->Rectangle(dest.TopLeft().X +1,dest.TopLeft().Y+1, dest.BottomRight().X-1, dest.BottomRight().Y-1);
  StressTest();
#endif
}
//---------------------------------------------------------------------------

void __fastcall Drawer::DrawEof(short linenum)
{
  drawcanvas->Pen->Color = (TColor)0xFFFFFF;
  drawcanvas->Brush->Color = (TColor)0xFFFFFF;
  y = Y_OFF+linesize*linenum;
  drawcanvas->Rectangle(0, y,RightBorder(), BottomBorder());
  if(linenumsenabled)
    DrawLinenum(linenum);
#ifdef SHEDIT_DEBUG_COPY_DRAW
  StressTest();
#endif
}
//---------------------------------------------------------------------------

void __fastcall Drawer::UpdateCursor(int x, int y)
{
  con = false;
  DrawCursor(); //this one here deletes the last one
  cx = x;
  cy = y;
#ifdef SHEDIT_DEBUG_COPY_DRAW
  StressTest();
#endif
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
void __fastcall Drawer::DrawEndl(short linenum, FontStyle format)
{
#ifndef FULL_WIDTH_PAINT
  drawcanvas->Pen->Color = (TColor)0xFFFFFF;
  drawcanvas->Brush->Color = (TColor)0xFFFFFF;
#else
  if(format.background != NULL)
  {
    drawcanvas->Pen->Color = *format.background;
    drawcanvas->Brush->Color = *format.background;
  }
  else
    drawcanvas->Pen->Color = drawcanvas->Brush->Color;
#endif
  con = false;
  y = Y_OFF+linesize*linenum;
  if(x < parent->Width)
    drawcanvas->Rectangle(x == X_OFF ? 0 : x, y,RightBorder(), y+linesize);
  if(x+HPos-2+200 > HMax)
  {
    HMax = x+HPos-X_OFF+200;
    parent->UpdateHBar();
  }
  HPos = parent->HBar->Position;
  x = X_OFF-HPos;

  if(linenumsenabled)
  {
    drawcanvas->Font->Color = (TColor)0x444444;
    drawcanvas->Pen->Color = (TColor)0xEEEEEE;
    drawcanvas->Brush->Color = (TColor)0xEEEEEE;
    drawcanvas->Font->Style = TFontStyles();
    drawcanvas->Rectangle(0, y, GetLinenumWidth(), y+linesize);
    drawcanvas->TextOut(X_OFF, y, String(parent->GetActLine()+linenum));
    drawcanvas->MoveTo(GetLinenumWidth(), y);
    drawcanvas->Pen->Color = (TColor)0x444444;
    drawcanvas->LineTo(GetLinenumWidth(), y+linesize);
  }
#ifdef SHEDIT_DEBUG_COPY_DRAW
  StressTest();
#endif
}
//---------------------------------------------------------------------------


void __fastcall Drawer::Paint()
{
  con = !con;
#ifdef DOUBLE_BUFFERED
  canvas->Draw(0, 0, bitmap);
#endif
  DrawCursor();

#ifdef DRAW_LINENUM
  //DrawLinenum();
#endif

  parent->VBar->Refresh();
  parent->HBar->Refresh();

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
  {
    canvas->Pen->Color = clBlack;
  }
  else
  {
#ifdef DOUBLE_BUFFERED
    return;
#else
  canvas->Pen->Color = clWhite;
#endif
  }
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
  drawcanvas->Font->Color = (TColor)0x444444;
  drawcanvas->Pen->Color = (TColor)0xEEEEEE;
  drawcanvas->Brush->Color = (TColor)0xEEEEEE;
  drawcanvas->Rectangle(0, from*linesize, GetLinenumWidth(), parent->Height);
  for(int i = parent->Height/linesize, j = from; j <= i; j++)
    drawcanvas->TextOut(X_OFF, j*linesize, String(parent->GetActLine()+j));

  drawcanvas->Pen->Color = (TColor)0x444444;
  drawcanvas->MoveTo(GetLinenumWidth(), 0);
  drawcanvas->LineTo(GetLinenumWidth(), parent->Height);
#ifdef SHEDIT_DEBUG_COPY_DRAW
  StressTest();
#endif
}
//---------------------------------------------------------------------------
void __fastcall Drawer::SetFontsize(int size)
{
  if(size < 3 || size > 100)
    return;
  canvas->Font->Size = size;
  drawcanvas->Font->Size = size;
  fontsize = size;
  //find out real height
  Graphics::TBitmap * bitmap = new Graphics::TBitmap();
  bitmap->SetSize(1, 1000);
  bitmap->Canvas->Brush->Style = bsSolid;
  bitmap->Canvas->Brush->Color = clGreen;
  bitmap->Canvas->Font->Color = clGreen;
  bitmap->Canvas->Font->Size = size;
  bitmap->Canvas->Font->Name = "Courier New";
  bitmap->Canvas->TextOut(0, 0, " ");
  int i = 0;
  TColor green = bitmap->Canvas->Pixels[0][0];
  while( bitmap->Canvas->Pixels[0][i+1] == green && i < 999)
    i++;

  linesize = i+1;

#ifndef QUICKDRAW
  linesize++;
#endif
  delete bitmap;

  drawcanvas->Font->Name = "Courier New";
  canvas->Font->Name = "Courier New";

  if(linesize < size)
    linesize = 3*size/2+1;

  UpdateLinenumWidth(lastlinenumcount);
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
  int w;

  Graphics::TBitmap * bitmap = new Graphics::TBitmap();
  bitmap->SetSize(1, 1);
  bitmap->Canvas->Font->Size = fontsize;
  bitmap->Canvas->Font->Name = "Courier New";
  w = bitmap->Canvas->TextWidth(count)+5; //2*2 margins + 1 line width
  delete bitmap;
    
    bool ret = linenumwidth != w;
  linenumwidth = w;
  return ret;
}
//---------------------------------------------------------------------------
int __fastcall Drawer::GetLinenumWidth()
{
  return linenumsenabled ? linenumwidth : 0;
}
//---------------------------------------------------------------------------
void __fastcall Drawer::SetLinenumsEnabled(bool enable)
{
  linenumsenabled = enable;
}
//---------------------------------------------------------------------------
bool __fastcall Drawer::GetLinenumsEnabled()
{
  return linenumsenabled;
}
//---------------------------------------------------------------------------
__fastcall Drawer::~Drawer()
{
}
//---------------------------------------------------------------------------
#ifdef SHEDIT_DEBUG
void Drawer::Write(String message)
{
#ifdef SHEDIT_DEBUG_LOGGING
  assert(myfiledraw.is_open());
  myfiledraw << UTF8String(message.c_str()).c_str();
  myfiledraw << std::endl;
#endif
}
#endif
//---------------------------------------------------------------------------
#ifdef SHEDIT_DEBUG
void Drawer::StressTest()
{
  Sleep(20);
  Paint();
}
#endif
//---------------------------------------------------------------------------
