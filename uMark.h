//---------------------------------------------------------------------------
#ifndef uMarkH
#define uMarkH

#include "uIPos.h"

namespace SHEdit
{
  class Iter;
  class Format;
  //---------------------------------------------------------------------------
  /*!
   * Mark represents positionless markup. The markup information are hardlinked to the buffer's structure and kept up-to-date by Parser. Specifically it means that each Span has it's own Stack for marks. It is not possible (in sensible complexity) to find out where a mark is positioned.
   * 
   * Links to Marks are kept in Format::marks list, so that they can be easily removed.
   * */
  class Mark
  {
    public:
      Mark(Format * format, bool begin, short pos);
      Format * format;
      bool begin;
      short pos;
  };

  //---------------------------------------------------------------------------
  /*!
   * IMark represents Iterator-handled markup. Position is kept by the IPos class in the same way as Iterators are kept up to date. Each Mark is linked to by:
   * - Buffer::ItrList - a list that keeps position up to date
   * - Format::imarks - a search tree that keeps links to Imarks of specified format (pointer to the Format is also kept by buffer in Buffer::FormatList, so in oder to find whether on a position is any markup you need just to go through all elements in Buffer::FormatList and for each list to search by Format::GetMarkBefor(IPos*) and look whether IMark::begin is true)
   * - Buffer::IMarkList - a search tree that holds links to all IMarks applied to the buffer (reason is that it makes possible to find out quickly positions where markup changes)
   * */
  class IMark : public IPos
  {
    public:
      bool begin;
      Format * format;
      IMark(Format * format, bool begin, IPos * itr);
      ~IMark();
  };

}
//---------------------------------------------------------------------------

#endif




