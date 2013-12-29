
//---------------------------------------------------------------------------
#pragma hdrstop


#include "uDrawer.h"
#include "cSQLSyntax.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>
#include <list>

using namespace SHEdit;

#ifdef DEBUG
#include <fstream>
std::ofstream myfile;
#endif

#pragma package(smart_init)
//---------------------------------------------------------------------------
DrawTaskEndline::DrawTaskEndline()
{
  this->type = DrawType::Endline;
}
//---------------------------------------------------------------------------
DrawTaskText::DrawTaskText()
{
  this->type = DrawType::Text;
  text = new String();
}
//---------------------------------------------------------------------------
DrawTaskText::~DrawTaskText()
{
}
//---------------------------------------------------------------------------
DrawTaskMove::DrawTaskMove()
{
  this->type = DrawType::Move;
}
//---------------------------------------------------------------------------
DrawTaskEof::DrawTaskEof()
{
  this->type = DrawType::Eof;
}
//---------------------------------------------------------------------------
DrawTaskHMove::DrawTaskHMove()
{
  this->type = DrawType::HMove;
}
//---------------------------------------------------------------------------
DrawTaskCursor::DrawTaskCursor()
{
  this->type = DrawType::Cursor;
}
//---------------------------------------------------------------------------
DrawTaskResize::DrawTaskResize(int width, int height)
{
  this->width = width;
  this->height = height;
  this->type = DrawType::Resize;
}
//---------------------------------------------------------------------------
DrawTaskCursor::DrawTaskCursor(int x, int y)
{
  this->type = DrawType::Cursor;
  this->x = x;
  this->y = y;
}
//---------------------------------------------------------------------------
DrawTaskMove::DrawTaskMove(short from, short to, short by)
{
  this->type = DrawType::Move;
  this->from = from;
  this->to = to;
  this->by = by;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
  __fastcall Drawer::Drawer(TCanvas * canvas, TSQLEdit * parent, HANDLE drawerCanvasMutex, HANDLE drawerQueueMutex, HANDLE drawerTaskPending)
: TThread(true)
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
  DuplicateHandle(GetCurrentProcess(), drawerQueueMutex, GetCurrentProcess(), &(this->drawerQueueMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerCanvasMutex, GetCurrentProcess(), &(this->drawerCanvasMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerTaskPending, GetCurrentProcess(), &(this->drawerTaskPending),  0, false, DUPLICATE_SAME_ACCESS);

  HMax = 200;
  HPos = 0;
  x = 2;
  y = Y_OFF;
  cx = 2;
  cy = 2;
#ifdef DEBUG
  myfile.open("drawer.txt", ios::out );
#endif
}

//---------------------------------------------------------------------------
void __fastcall Drawer::Execute()
{
  drawcanvas->Font->Size = FONTSIZE;
  drawcanvas->Font->Name = "Courier New";
  canvas->Font->Size = FONTSIZE;
  canvas->Font->Name = "Courier New";
  this->Priority=tpLower;
  //parent->DoubleBuffered = true;
  while(!Terminated)
  {
#ifdef DEBUG
    //Write("new cycle");
    jump:
#endif
    if(WaitForSingleObject(drawerTaskPending, 500) != WAIT_TIMEOUT)
      con = false;

    ResetEvent(drawerTaskPending);
    while(tasklist.size() > 0)
    {
      WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);

#ifdef DEBUG
      //ReleaseMutex(drawerQueueMutex);
      //goto jump;
      //QueueDump();
#endif
      DrawTask * tasktoprocess = tasklist.front();
      tasklist.pop_front();
      ReleaseMutex(drawerQueueMutex);
      WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);
      drawcanvas->Lock();
#ifdef DEBUG
          Write(String("=")+String(tasktoprocess->type)+String("="));

#endif
      switch(tasktoprocess->type)
      {
        case DrawType::Text:
          {

            if(((DrawTaskText*)tasktoprocess)->format.foreground != NULL)
              drawcanvas->Font->Color = *(((DrawTaskText*)tasktoprocess)->format.foreground);
            if(((DrawTaskText*)tasktoprocess)->format.background != NULL)
              drawcanvas->Brush->Color = *(((DrawTaskText*)tasktoprocess)->format.background);

            if(((DrawTaskText*)tasktoprocess)->newline)
            {

#ifdef DEBUG
    Write("\n");
#endif
              HPos = parent->HBar->Position;
              x = 2-HPos;
            }
            y = Y_OFF+LINESIZE*((DrawTaskText*)tasktoprocess)->linenum;

#ifdef DEBUG
            drawcanvas->Rectangle(x, y,x+drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text)), y+LINESIZE);
            this->Sleep(10);
            DrawTaskText *dbg = ((DrawTaskText*)tasktoprocess);
            Write((((DrawTaskText*)tasktoprocess)->format.background != NULL && *(((DrawTaskText*)tasktoprocess)->format.background) != (TColor)0xddffdd ? String("!") : String(" ")) + String((int)drawcanvas->Brush->Color) +String(" ")+ String(y) + String("(")+ String(x) + String(")<")+ String(drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text)))+ String(">:")+ *(((DrawTaskText*)tasktoprocess)->text));
#endif
            //SetTextColor(drawcanvas->Handle, 0x0);
            //int dbg2 = TextOut(drawcanvas->Handle, x, y, (*(((DrawTaskText*)tasktoprocess)->text)).c_str(), (*(((DrawTaskText*)tasktoprocess)->text)).Length());
            //drawcanvas->Font->Color = clRed;
            drawcanvas->TextOut(x, y, *(((DrawTaskText*)tasktoprocess)->text));
            x = drawcanvas->PenPos.x;
            //x += drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text));

          }
          break;
        case DrawType::Move:
#define FROM ((DrawTaskMove*)tasktoprocess)->from
#define TO ((DrawTaskMove*)tasktoprocess)->to
#define BY ((DrawTaskMove*)tasktoprocess)->by
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
          break;
        case DrawType::HMove:
          //HPos = parent->HBar->Position;
          //HMax = 0;
          break;
        case DrawType::Eof:
          drawcanvas->Pen->Color = (TColor)0xFFFFFF;
          y = Y_OFF+LINESIZE*((DrawTaskEof*)tasktoprocess)->linenum;
          drawcanvas->Rectangle(0, y,RightBorder(), BottomBorder());

          break;
        case DrawType::Cursor:
          con = false;
          DrawCursor(); //this one here deletes the last one
          cx = ((DrawTaskCursor*)tasktoprocess)->x;
          cy = ((DrawTaskCursor*)tasktoprocess)->y;
          //cursorBGcolor = canvas->Pixels[cx][cy];
          break;
        case DrawType::Resize:
#ifdef DOUBLE_BUFFERED
          bitmap->SetSize(((DrawTaskResize*)tasktoprocess)->width, ((DrawTaskResize*)tasktoprocess)->height);
#endif
          break;
        case DrawType::Endline:
          drawcanvas->Pen->Color = (TColor)0xFFFFFF;
          drawcanvas->Brush->Color = (TColor)0xFFFFFF;
          y = Y_OFF+LINESIZE*((DrawTaskEndline*)tasktoprocess)->linenum;
          drawcanvas->Rectangle(x == 2 ? 0 : x, y,RightBorder(), y+LINESIZE);
          if(x+HPos-2+200 > HMax)
          {
            HMax = x+HPos-2+200;
            Synchronize(UpdateHBar);
          }
          HPos = parent->HBar->Position;
          x = 2-HPos;
#ifdef DEBUG
   // Write("\n");
#endif

          break;
      }
      drawcanvas->Unlock();
      ReleaseMutex(drawerCanvasMutex);
      delete tasktoprocess;
    }
    con = !con;
    WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);
    drawcanvas->Lock();
    canvas->Lock();
#ifdef DOUBLE_BUFFERED
    canvas->Draw(0, 0, bitmap);
#endif
    DrawCursor();
    canvas->Unlock();
    drawcanvas->Unlock();
    ReleaseMutex(drawerCanvasMutex);
    parent->VBar->Repaint();
    parent->HBar->Repaint();
#ifdef DEBUG
    Write(String("\n"));
    Write(String("freeing everything\n"));
    Write(String("\n"));
#endif
  }
}

//---------------------------------------------------------------------------
void __fastcall Drawer::UpdateHBar()
{
  parent->UpdateHBar();
}
//---------------------------------------------------------------------------
void Drawer::Draw(DrawTask * drawtask)
{
  //WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  tasklist.push_back(drawtask);
  SetEvent(drawerTaskPending); //function is called from different threads while this one is suspended/running thus it is not logical nonsense

  //ReleaseMutex(drawerQueueMutex);
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
  return  parent->VBar->Visible ? parent->Width - 15 : parent->Width;
}
//---------------------------------------------------------------------------
int Drawer::BottomBorder()
{
  return  parent->HBar->Visible ? parent->Height - 15 : parent->Height;
}
//---------------------------------------------------------------------------
__fastcall Drawer::~Drawer()
{
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void Drawer::QueueDump()
{        /*
            DrawTaskText* debug;
            Write("Dumping queue");
            for (std::list<DrawTask*>::const_iterator iterator = tasklist.begin(), end = tasklist.end(); iterator != end; ++iterator)
            {
            switch((*iterator)->type)
            {
            case DrawType::Text:
            debug = (DrawTaskText*)*iterator;
            if(((DrawTaskText*)*iterator)->text->IsEmpty())
            Write("	empty string");
            else
            Write(String("	text: ")+*((DrawTaskText*)*iterator)->text+String(" linenum: ")+((DrawTaskText*)*iterator)->linenum );
            break;
            case DrawType::Move:
            Write(String("	move: by")+((DrawTaskMove*)*iterator)->by+String(" from ")+((DrawTaskMove*)*iterator)->from+String(" to ")+((DrawTaskMove*)*iterator)->to);
            break;
            case DrawType::Endline:
            Write("	endline");
            break;
            }
            }      */
}
#endif
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
