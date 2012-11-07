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

      void Comment::parse(XML::ParserPos &ioPos)
      {
        Parser::AutoStack stack(ioPos);

        mValue.clear();

        ZS_THROW_BAD_STATE_IF(!ioPos.isString("<!--"))

        ioPos += (ULONG)strlen("<!--");

        while ((*ioPos) &&
               (!ioPos.isString("-->")))
        {
          mValue += *ioPos;
          ++ioPos;
        }

        if (ioPos.isEOF())
        {
          // did not find end to text element
          (ioPos.getDocument())->addWarning(ParserWarningType_NoEndCommentFound);
        }
        else
        {
          ioPos += (ULONG)strlen("-->");
        }
      }

      ULONG Comment::getOutputSize(const DocumentPtr &inDocument) const
      {
        ULONG result = 0;
        result += (ULONG)strlen("<!--");
        result += mValue.getLength();
        result += (ULONG)strlen("-->");
        return result;
      }

      void Comment::writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const
      {
        Parser::writeBuffer(ioPos, "<!--");
        Parser::writeBuffer(ioPos, mValue);
        Parser::writeBuffer(ioPos, "-->");
      }

      NodePtr Comment::cloneAssignParent(NodePtr inParent) const
      {
        CommentPtr newObject(XML::Comment::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);
        newObject->mValue = mValue;
        (mThis.lock())->cloneChildren(mThis.lock(), newObject);
        return newObject;
      }

    } // namespace internal

    Comment::Comment() :
    internal::Comment()
    {
    }

    CommentPtr Comment::create()
    {
      CommentPtr object(new Comment);
      object->mThis = object;
      return object;
    }

    NodePtr Comment::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    void Comment::clear()
    {
      mValue.clear();
    }

    String Comment::getValue() const
    {
      return mValue;
    }

    void Comment::adoptAsFirstChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("comments are not allowed to have child nodes")
    }

    void Comment::adoptAsLastChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("comments are not allowed to have child nodes")
    }

  } // namespace XML

} // namespace zsLib
