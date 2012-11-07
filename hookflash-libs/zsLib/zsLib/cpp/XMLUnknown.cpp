/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2011. Robin Raymond. All rights reserved.
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

      void Unknown::parse(XML::ParserPos &ioPos, const char *start, const char *ending)
      {
        Parser::AutoStack stack(ioPos);

        size_t endingLength = 0;
        if (ending)
          endingLength = strlen(ending);

        mValue.clear();

        ZS_THROW_BAD_STATE_IF('<' != *ioPos)

        if (start) {
          size_t startLength = strlen(start);
          ZS_THROW_BAD_STATE_IF(0 != strncmp(ioPos, start, startLength))
          ioPos += startLength;
          mValue = start+1; // insert all but the starting '<'
        } else {
          ++ioPos;
        }

        while (*ioPos)
        {
          if (ending) {
            if (0 == strncmp(ioPos, ending, endingLength))
              break;
          } else {
            if ('>' == *ioPos)
              break;
          }
          mValue += *ioPos;
          ++ioPos;
        }

        bool found = false;
        if (ending) {
          found = (0 == strncmp(ioPos, ending, endingLength));
          if (found) {
            mValue += String(ending).substr(0, endingLength-1);
            ioPos += endingLength;
          }
        } else {
          if ('>' == *ioPos)
          {
            found = true;
            ++ioPos;
          }
        }

        if (!found)
        {
          // did not find end to text element
          (ioPos.getDocument())->addWarning(ParserWarningType_NoEndUnknownTagFound);
        }
      }

      ULONG Unknown::getOutputSize(const DocumentPtr &inDocument) const
      {
        ULONG result = 0;
        result += (ULONG)strlen("<");
        result += mValue.getLength();
        result += (ULONG)strlen(">");
        return result;
      }

      void Unknown::writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const
      {
        Parser::writeBuffer(ioPos, "<");
        Parser::writeBuffer(ioPos, mValue);
        Parser::writeBuffer(ioPos, ">");
      }

      NodePtr Unknown::cloneAssignParent(NodePtr inParent) const
      {
        UnknownPtr newObject(XML::Unknown::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);
        newObject->mValue = mValue;
        (mThis.lock())->cloneChildren(mThis.lock(), newObject);
        return newObject;
      }

    } // namespace internal

    Unknown::Unknown() :
    internal::Unknown()
    {
    }

    UnknownPtr Unknown::create()
    {
      UnknownPtr object(new Unknown);
      object->mThis = object;
      return object;
    }

    NodePtr Unknown::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    void Unknown::clear()
    {
      mValue.clear();
      Node::clear();
    }

    String Unknown::getValue() const
    {
      return mValue;
    }

    void Unknown::adoptAsFirstChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("unknown elements cannot have child nodes")
    }

    void Unknown::adoptAsLastChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("unknown elements cannot have child nodes")
    }

  } // namespace XML

} // namespace zsLib
