
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
DrawTaskCursor::DrawTaskCursor()
{
  this->type = DrawType::Cursor;
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
  //bitmap = new Graphics::TBitmap();
  //bitmap->SetSize(1000, 1000);
  drawcanvas = canvas;
  this->parent = parent;
  this->canvas = canvas;
  DuplicateHandle(GetCurrentProcess(), drawerQueueMutex, GetCurrentProcess(), &(this->drawerQueueMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerCanvasMutex, GetCurrentProcess(), &(this->drawerCanvasMutex),  0, false, DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(), drawerTaskPending, GetCurrentProcess(), &(this->drawerTaskPending),  0, false, DUPLICATE_SAME_ACCESS);

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
  while(!Terminated)
  {
#ifdef DEBUG
    Write("new cycle");
#endif
    if(WaitForSingleObject(drawerTaskPending, 500) != WAIT_TIMEOUT)
      con = false;
#ifdef DEBUG
    Write("resetting drawertaskpending");
#endif
    ResetEvent(drawerTaskPending);
    while(tasklist.size() > 0)
    {
#ifdef DEBUG
      Write("retrieving new drawtask");
#endif
      WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);

#ifdef DEBUG
      QueueDump();
#endif
      DrawTask * tasktoprocess = tasklist.front();
      tasklist.pop_front();
      ReleaseMutex(drawerQueueMutex);
#ifdef DEBUG
      Write(String("sync finished objtype: ") + String(tasktoprocess->type));
#endif
      WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);
      drawcanvas->Lock();
      switch(tasktoprocess->type)
      {
        case DrawType::Text:
          {
            if(((DrawTaskText*)tasktoprocess)->format.foreground)
              drawcanvas->Font->Color = *(((DrawTaskText*)tasktoprocess)->format.foreground);
            if(((DrawTaskText*)tasktoprocess)->format.background)
              drawcanvas->Brush->Color = *(((DrawTaskText*)tasktoprocess)->format.background);

            if(((DrawTaskText*)tasktoprocess)->newline)
              x = 2;
            y = Y_OFF+LINESIZE*((DrawTaskText*)tasktoprocess)->linenum;
            drawcanvas->TextOut(x, y, *(((DrawTaskText*)tasktoprocess)->text));
            x += drawcanvas->TextWidth(*(((DrawTaskText*)tasktoprocess)->text));
            
          }
          break;
        case DrawType::Move:
#define FROM ((DrawTaskMove*)tasktoprocess)->from
#define TO ((DrawTaskMove*)tasktoprocess)->to
#define BY ((DrawTaskMove*)tasktoprocess)->by
          {
            TRect source2(0, 0, 0, 1);
            TRect source(0, LINESIZE*FROM, parent->Width, LINESIZE*(TO+1));
            TRect dest(0, LINESIZE*(FROM+BY), parent->Width, LINESIZE*(TO+BY+1));
            drawcanvas->CopyRect(dest, drawcanvas, source);
            if(BY < 0)
            {
              drawcanvas->Pen->Color = (TColor)0xFFFFFF;
              drawcanvas->Brush->Color = (TColor)0xFFFFFF;
              drawcanvas->Rectangle(0, LINESIZE*(TO+BY),parent->Width, parent->Height);
            }
          }
          break;
        case DrawType::Eof:
          drawcanvas->Pen->Color = (TColor)0xFFFFFF;
          y = Y_OFF+LINESIZE*((DrawTaskEof*)tasktoprocess)->linenum;
          drawcanvas->Rectangle(0, y,parent->Width, parent->Height);

          break;
        case DrawType::Cursor:
          con = false;
          DrawCursor(); //this one here deletes the last one
          cx = ((DrawTaskCursor*)tasktoprocess)->x;
          cy = ((DrawTaskCursor*)tasktoprocess)->y;
          //cursorBGcolor = canvas->Pixels[cx][cy];
          break;
        case DrawType::Endline:
          drawcanvas->Pen->Color = (TColor)0xFFFFFF;
          drawcanvas->Brush->Color = (TColor)0xFFFFFF;
          y = Y_OFF+LINESIZE*((DrawTaskEndline*)tasktoprocess)->linenum;
          drawcanvas->Rectangle(x == 2 ? 0 : x, y,parent->Width, y+LINESIZE);
          x=2;
          break;
      }
      drawcanvas->Unlock();
      ReleaseMutex(drawerCanvasMutex);
      delete tasktoprocess;
#ifdef DEBUG
      Write(String("cycle finished"));
#endif
    }
    con = !con;
    WaitForSingleObject(drawerCanvasMutex, WAIT_TIMEOUT_TIME);
    canvas->Lock();
    //drawcanvas->Lock();
    //canvas->Draw(0, 0, bitmap);
    DrawCursor();
    //drawcanvas->Unlock();
    canvas->Unlock();
    ReleaseMutex(drawerCanvasMutex);
#ifdef DEBUG
      Write(String("freeing everything"));
#endif
  }
}
//---------------------------------------------------------------------------
void Drawer::Draw(DrawTask * drawtask)
{
      WaitForSingleObject(drawerQueueMutex, WAIT_TIMEOUT_TIME);
  tasklist.push_back(drawtask);
  SetEvent(drawerTaskPending); //function is called from different threads while this one is suspended/running thus it is not logical nonsense

    ReleaseMutex(drawerQueueMutex);
    }
//---------------------------------------------------------------------------
void Drawer::DrawCursor()
{
  if(con)
    canvas->Pen->Color = clBlack;
  else
    canvas->Pen->Color = clWhite;
  canvas->MoveTo(cx, cy);
  canvas->LineTo(cx, cy+LINESIZE-1);
  canvas->Pen->Color = clBlack;
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
  myfile << message.c_str() << std::endl;
}
#endif
//---------------------------------------------------------------------------
