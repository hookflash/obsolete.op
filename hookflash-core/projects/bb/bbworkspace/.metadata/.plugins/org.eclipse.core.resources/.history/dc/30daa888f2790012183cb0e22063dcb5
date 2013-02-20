/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2013. Robin Raymond. All rights reserved.
 *
 * This file is part of zsLib.
 *
 * zsLib is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (LGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * zsLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with zsLib; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <zsLib/XML.h>
#include <zsLib/Exception.h>

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

namespace zsLib
{

  namespace XML
  {

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark XML::internal::ParserPos
      #pragma mark

      //-----------------------------------------------------------------------
      ParserPos::ParserPos()
      {
      }

      //-----------------------------------------------------------------------
      ParserPos::ParserPos(const ParserPos &inPos) :
        mParser(inPos.mParser),
        mDocument(inPos.mDocument)
      {
      }

      //-----------------------------------------------------------------------
      static ULONG calculateColumnFromSOL(const char *const inPos, const char *const inStart, ULONG inTabSize)
      {
        // find the start of the line
        const char *startLine = inPos;
        while (startLine > inStart)
        {
          if ('\n' == (*(startLine - 1)))
            break;

          // if the previous character is an EOL, then we have reached the start of the line
          if ('\r' == (*(startLine - 1)))
          {
            if ('\n' != (*(startLine)))
              break;                     // this is a MAC EOL so therefor it is the start of the line
          }

          --startLine;
        }

        ULONG column = 1;
        const char *pos = startLine;
        while (pos != inPos)
        {
          if ('\t' == *pos)
          {
            // this is a tab character, column changes because of it
            column = ((((column-1)/inTabSize)+1)*inTabSize)+1;
          }
          else if ('\r' != *pos)
          {
            // \r are windows EOL in this case and should not count towards the column count
            ++column;
          }

          ++pos;
        }
        return column;
      }

    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::ParserPos
    #pragma mark

    //-------------------------------------------------------------------------
    ParserPos::ParserPos() :
      mRow(0),
      mColumn(0),
      mPos(NULL)
    {
    }

    //-------------------------------------------------------------------------
    ParserPos::ParserPos(const ParserPos &inPos) :
      mRow(inPos.mRow),
      mColumn(inPos.mColumn),
      mPos(inPos.mPos),
      internal::ParserPos(inPos)
    {
    }

    //-------------------------------------------------------------------------
    ParserPos::ParserPos(
                         Parser &inParser,
                         Document &inDocument
                         ) :
      mRow(0),
      mColumn(0),
      mPos(NULL)
    {
      setParser(inParser.toParser());
      setDocument(inDocument.toDocument());
      setSOF();
    }

    //-------------------------------------------------------------------------
    ParserPos::ParserPos(
                         ParserPtr inParser,
                         DocumentPtr inDocument
                         ) :
      mRow(0),
      mColumn(0),
      mPos(NULL)
    {
      setParser(inParser);
      setDocument(inDocument);
      setSOF();
    }

    //-------------------------------------------------------------------------
    bool ParserPos::isSOF() const
    {
      if (NULL == mPos)
        return false;

      ParserPtr parser = getParser();
      ZS_THROW_BAD_STATE_IF(!parser)

      return (mPos == parser->mSOF);
    }

    //-------------------------------------------------------------------------
    bool ParserPos::isEOF() const
    {
      if (NULL == mPos)
        return true;

      return (0 == (*mPos));
    }

    //-------------------------------------------------------------------------
    void ParserPos::setSOF()
    {
      ParserPtr parser = getParser();
      ZS_THROW_BAD_STATE_IF(!parser)

      mPos = parser->mSOF;
      mRow = 1;
      mColumn = 1;
      ZS_THROW_BAD_STATE_IF(NULL == mPos)
    }

    //-------------------------------------------------------------------------
    void ParserPos::setEOF()
    {
      mPos = NULL;
    }

    //-------------------------------------------------------------------------
    void ParserPos::setParser(ParserPtr inParser)
    {
      mParser = inParser;
    }

    //-------------------------------------------------------------------------
    ParserPtr ParserPos::getParser() const
    {
      return mParser.lock();
    }

    //-------------------------------------------------------------------------
    void ParserPos::setDocument(DocumentPtr inDocument)
    {
      mDocument = inDocument;
    }

    //-------------------------------------------------------------------------
    DocumentPtr ParserPos::getDocument() const
    {
      return mDocument.lock();
    }

    //-------------------------------------------------------------------------
    ParserPos ParserPos::operator++() const
    {
      ParserPos temp(*this);
      ++temp;
      return temp;
    }

    //-------------------------------------------------------------------------
    ParserPos &ParserPos::operator++()
    {
      (*this) += 1;
      return (*this);
    }

    //-------------------------------------------------------------------------
    ParserPos ParserPos::operator--() const
    {
      ParserPos temp(*this);
      --temp;
      return temp;
    }

    //-------------------------------------------------------------------------
    ParserPos &ParserPos::operator--()
    {
      (*this) -= 1;
      return (*this);
    }

    //-------------------------------------------------------------------------
    size_t ParserPos::operator-(const ParserPos &inPos) const
    {
      ParserPos startPos = inPos;
      ParserPos endPos(*this);

      const char *start = startPos.mPos;

      if (!start)
      {
        if (endPos.isEOF())
          return 0;

        startPos.setSOF();
        startPos.mPos += strlen(startPos.mPos);
        start = startPos.mPos;
      }

      const char *end = endPos.mPos;
      if (!end)
      {
        if (startPos.isEOF())
          return 0;

        endPos.setSOF();
        endPos.mPos += strlen(endPos.mPos);
        end = endPos.mPos;
      }

      return end - start;
    }

    //-------------------------------------------------------------------------
    ParserPos &ParserPos::operator+=(ULONG inDistance)
    {
      ParserPtr parser(getParser());

      if (NULL == mPos)
        return *this;

      while (0 != inDistance)
      {
        if (isEOF())
          return *this;

        switch (*mPos)
        {
          case '\r':
          {
            // does not advance row, just resets column
            if ('\n' != (*(mPos+1)))
            {
              // this is MAC EOL \r therefor we can do the same thing as \n
              ++mRow;
              mColumn = 1;
              break;
            }
            // this is DOS EOL \r\n - ingore the \r in the position
            break;
          }
          case '\n':
          {
            // advances row, resets column to zero
            ++mRow;
            mColumn = 1;
            break;
          }
          case '\t':
          {
            ZS_THROW_BAD_STATE_IF(!parser)
            ZS_THROW_BAD_STATE_IF(0 == mColumn)

            ULONG tabSize = parser->getTabSize();
            // if tab size were set to 3
            // 1..2..3 would go to position 4
            // 4..5..6 would go to position 7

            mColumn = ((((mColumn-1)/tabSize)+1)*tabSize)+1;
            break;
          }
          default:
          {
            ++mColumn;
            break;
          }
        }
        ++mPos;
        --inDistance;
      }

      return *this;
    }

    //-------------------------------------------------------------------------
    ParserPos &ParserPos::operator-=(ULONG inDistance)
    {
      ParserPtr parser(getParser());

      if (NULL == mPos)
      {
        ParserPos temp(*this);
        temp.setSOF();
        temp += (ULONG)strlen(temp.mPos);
        (*this) = temp;
      }

      while (0 != inDistance)
      {
        if (isSOF())
          return *this;

        // assuming column and row should be accurate at this point, but could become inaccurate
        // if we run into a special character

        --mPos;
        --inDistance;

        switch (*mPos)
        {
          case '\r':
          case '\n':
          case '\t':
          {
            if ('\n' == *mPos)
            {
              ZS_THROW_BAD_STATE_IF(mRow < 2)   // how did the count become inaccurate
              --mRow;
            }
            if ('\r' == *mPos)
            {
              if ('\n' != (*(mPos+1)))
              {
                // this is a MAC EOL
                ZS_THROW_BAD_STATE_IF(mRow < 2)   // how did the count become inaccurate
                --mRow;
              }
            }

            ZS_THROW_BAD_STATE_IF(!parser)
            ZS_THROW_BAD_STATE_IF(NULL == parser->mSOF)

            // these characters cause the column to get messed up, simplest method is to
            // recalculate the column position from the start of the line
            mColumn = internal::calculateColumnFromSOL(mPos, parser->mSOF, parser->mTabSize);
            break;
          }
          default:
          {
            ZS_THROW_BAD_STATE_IF(mColumn < 2)   // how did the count become inaccurate

            --mColumn;
            break;
          }
        }
      }

      return *this;
    }

    //-------------------------------------------------------------------------
    bool ParserPos::operator==(const ParserPos &inPos)
    {
      return (mPos == inPos.mPos);
    }

    //-------------------------------------------------------------------------
    bool ParserPos::operator!=(const ParserPos &inPos)
    {
      return (mPos != inPos.mPos);
    }

    //-------------------------------------------------------------------------
    char ParserPos::operator*() const
    {
      if (isEOF())
        return 0;

      return *mPos;
    }

    //-------------------------------------------------------------------------
    ParserPos::operator CSTR() const
    {
      if (isEOF())
        return "\0\0";

      return mPos;
    }

    //-------------------------------------------------------------------------
    bool ParserPos::isString(CSTR inString, bool inCaseSensative) const
    {
      if (NULL == inString)
        return isEOF();

      ParserPos temp(*this);

      const char *pos = inString;
      while ((*pos) && (*temp))
      {
        if (inCaseSensative)
        {
          if (*temp != *pos)
            return false;
        }
        else
        {
          if (tolower(*temp) != tolower(*pos))
            return false;
        }
        ++temp;
        ++pos;
      }

      return (!(*pos));
    }

    //-------------------------------------------------------------------------
    ParserPos operator+(const ParserPos &inPos, ULONG inDistance)
    {
      ParserPos temp(inPos);
      temp += inDistance;
      return temp;
    }

    //-------------------------------------------------------------------------
    ParserPos operator-(const ParserPos &inPos, ULONG inDistance)
    {
      ParserPos temp(inPos);
      temp -= inDistance;
      return temp;
    }

    //-------------------------------------------------------------------------
    ParserPos operator+(const ParserPos &inPos, int inDistance)
    {
      return inPos + ((ULONG)inDistance);
    }

    //-------------------------------------------------------------------------
    ParserPos operator-(const ParserPos &inPos, int inDistance)
    {
      return inPos - ((ULONG)inDistance);
    }

    //-------------------------------------------------------------------------
    ParserPos operator+(const ParserPos &inPos, unsigned int inDistance)
    {
      return inPos + ((ULONG)inDistance);
    }

    //-------------------------------------------------------------------------
    ParserPos operator-(const ParserPos &inPos, unsigned int inDistance)
    {
      return inPos - ((ULONG)inDistance);
    }
  } // namespace XML

} // namespace zsLib
