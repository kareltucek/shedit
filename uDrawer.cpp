
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

  HMax = 200;
  HPos = 0;
  x = 2;
  y = Y_OFF;
  cx = 2;
  cy = 2;
#ifdef DEBUG
  myfile.open("drawer.txt", ios::out );
#endif
  drawcanvas->Font->Size = FONTSIZE;
  drawcanvas->Font->Name = "Courier New";
  canvas->Font->Size = FONTSIZE;
  canvas->Font->Name = "Courier New";
}

//---------------------------------------------------------------------------

void __fastcall Drawer::DrawText(String text, bool newline, short linenum, FontStyle format)
{


            if(format.foreground != NULL)
              drawcanvas->Font->Color = *format.foreground;
            if(format.background != NULL)
              drawcanvas->Brush->Color = *format.background;

            if(newline)
            {

#ifdef DEBUG
   // Write("\n");
#endif
              HPos = parent->HBar->Position;
              x = 2-HPos;
            }
            y = Y_OFF+LINESIZE*linenum;

#ifdef DEBUG
          /*  //drawcanvas->Rectangle(x, y,x+drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text)), y+LINESIZE);
            Sleep(10);
            DrawTaskText *dbg = ((DrawTaskText*)tasktoprocess);
            Write((((DrawTaskText*)tasktoprocess)->format.background != NULL && *(((DrawTaskText*)tasktoprocess)->format.background) != (TColor)0xddffdd ? String("!") : String(" ")) + String((int)drawcanvas->Brush->Color) +String(" ")+ String(y) + String("(")+ String(x) + String(")<")+ String(drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text)))+ String(">:")+ *(((DrawTaskText*)tasktoprocess)->text));
            */
#endif
            drawcanvas->TextOut(x, y, text);
            x = drawcanvas->PenPos.x;


}

void __fastcall Drawer::DrawMove(int from, int to, int by)
{
#define FROM from
#define TO to
#define BY by
          {
            int adjustment = LINESIZE*(TO+1) > BottomBorder() ? BottomBorder() - LINESIZE*(TO+1)-2 : -2;
            TRect source2(0, 0, 0, 1);
            TRect source(0, LINESIZE*FROM, RightBorder(), LINESIZE*(TO+1)+adjustment);
            TRect dest(0, LINESIZE*(FROM+BY), RightBorder(), LINESIZE*(TO+BY+1)+adjustment);
            drawcanvas->CopyRect(dest, drawcanvas, source);
            if(BY < 0)
            {
              drawcanvas->Pen->Color = (TColor)0xFFFFFF;
              drawcanvas->Brush->Color = (TColor)0xFFFFFF;
              drawcanvas->Rectangle(0, LINESIZE*(TO+BY),RightBorder(), BottomBorder());
            }
          }
}

void __fastcall Drawer::DrawEof(short linenum)
{
          drawcanvas->Pen->Color = (TColor)0xFFFFFF;
          y = Y_OFF+LINESIZE*linenum;
          drawcanvas->Rectangle(0, y,RightBorder(), BottomBorder());
}

void __fastcall Drawer::UpdateCursor(int x, int y)
{
          con = false;
          DrawCursor(); //this one here deletes the last one
          cx = x;
          cy = y;
          //cursorBGcolor = canvas->Pixels[cx][cy];
}

void __fastcall Drawer::DrawResize(int w, int h)
{
#ifdef DOUBLE_BUFFERED
          bitmap->SetSize(RightBorder(), BottomBorder());
#endif
}

void __fastcall Drawer::DrawEndl(short linenum)
{
  #ifndef FULL_WIDTH_PAINT
          drawcanvas->Pen->Color = (TColor)0xFFFFFF;
          drawcanvas->Brush->Color = (TColor)0xFFFFFF;
  #else
          drawcanvas->Pen->Color = drawcanvas->Brush->Color;
  #endif
           y = Y_OFF+LINESIZE*linenum;
          drawcanvas->Rectangle(x == 2 ? 0 : x, y,RightBorder(), y+LINESIZE);
          if(x+HPos-2+200 > HMax)
          {
            HMax = x+HPos-2+200;
            parent->UpdateHBar();
          }
          HPos = parent->HBar->Position;
          x = 2-HPos;
}


void __fastcall Drawer::Paint()
{
    con = !con;
#ifdef DOUBLE_BUFFERED
    canvas->Draw(0, 0, bitmap);
#endif
    DrawCursor();
    parent->VBar->Repaint();
    parent->HBar->Repaint();

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
  canvas->LineTo(cx-HPos, cy+LINESIZE-1);
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
  return  parent->HBar->Visible ? parent->Height - 16 : parent->Height;
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
