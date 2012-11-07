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

      Text::Text() :
      mOutputCDATA(false),
      mInCDATAFormat(false)
      {
      }

      void Text::parse(XML::ParserPos &ioPos)
      {
        Parser::AutoStack stack(ioPos);

        mValue.clear();

        mOutputCDATA = ioPos.isString("<![CDATA[");
        mInCDATAFormat = mOutputCDATA;

        if (mOutputCDATA)
        {
          // skip over the CDATA tag
          ioPos += (ULONG)strlen("<![CDATA[");

          while ((*ioPos) &&
                 (!ioPos.isString("]]>")))
          {
            mValue += *ioPos;
            ++ioPos;
          }

          if (ioPos.isEOF())
          {
            // did not find end to text element
            (ioPos.getDocument())->addWarning(ParserWarningType_CDATAMissingEndTag);
            return;
          }

          // done parsing the CDATA
          ioPos += (ULONG)strlen("]]>");
          return;
        }

        while (*ioPos)
        {
          // can only store '<' if at the start of the text block, otherwise it will think '<' is a new element
          if ((!mValue.isEmpty()) &&
              ('<' == *ioPos))
            break;

          mValue += *ioPos;
          ++ioPos;
        }
      }

      ULONG Text::getOutputSize(const DocumentPtr &inDocument) const
      {
        bool outputCDATA = mOutputCDATA;

        String actualValue = mValue;

        if (0 != (XML::Document::WriteFlag_NormalizeCDATA & inDocument->getWriteFlags())) {
          outputCDATA = false;

          if (!mInCDATAFormat)
          {
            // this is in entity format, need to convert to CDATA format
            actualValue = XML::Parser::convertFromEntities(mValue);
          }

          // now in CDATA format but we need it in entity safe format
          actualValue = XML::Parser::makeTextEntitySafe(actualValue, (0 != (XML::Document::WriteFlag_EntityEncodeWindowsCarriageReturnInText & inDocument->getWriteFlags())));
        } else {
          if (outputCDATA)
          {
            if (!mInCDATAFormat)
            {
              // this is in entity format, need to convert to CDATA format
              actualValue = XML::Parser::convertFromEntities(mValue);
            }
          }
          else
          {
            if (mInCDATAFormat)
            {
              // this is CDATA format but being output as entity, need to convert
              actualValue = XML::Parser::makeTextEntitySafe(mValue);
            }
          }
        }

        ULONG result = 0;
        if (outputCDATA)
          result += (ULONG)strlen("<![CDATA[");

        result += actualValue.getLength();

        if (outputCDATA)
          result += (ULONG)strlen("]]>");
        return result;
      }

      void Text::writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const
      {
        bool outputCDATA = mOutputCDATA;

        String actualValue = mValue;

        if (0 != (XML::Document::WriteFlag_NormalizeCDATA & inDocument->getWriteFlags())) {
          outputCDATA = false;

          if (!mInCDATAFormat)
          {
            // this is in entity format, need to convert to CDATA format
            actualValue = XML::Parser::convertFromEntities(mValue);
          }

          // now in CDATA format but we need it in entity safe format
          actualValue = XML::Parser::makeTextEntitySafe(actualValue, (0 != (XML::Document::WriteFlag_EntityEncodeWindowsCarriageReturnInText & inDocument->getWriteFlags())));
        } else {
          if (outputCDATA)
          {
            if (!mInCDATAFormat)
            {
              // this is in entity format, need to convert to CDATA format
              actualValue = XML::Parser::convertFromEntities(mValue);
            }
          }
          else
          {
            if (mInCDATAFormat)
            {
              // this is CDATA format but being output as entity, need to convert
              actualValue = XML::Parser::makeTextEntitySafe(mValue);
            }
          }
        }

        if (outputCDATA)
          Parser::writeBuffer(ioPos, "<![CDATA[");

        Parser::writeBuffer(ioPos, actualValue);

        if (outputCDATA)
          Parser::writeBuffer(ioPos, "]]>");
      }

      NodePtr Text::cloneAssignParent(NodePtr inParent) const
      {
        TextPtr newObject(XML::Text::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);
        newObject->mValue = mValue;
        newObject->mOutputCDATA = mOutputCDATA;
        newObject->mInCDATAFormat = mInCDATAFormat;
        (mThis.lock())->cloneChildren(mThis.lock(), newObject);    // should do noop since text nodes aren't allowed children
        return newObject;
      }

    } // namespace internal


    TextPtr Text::create()
    {
      TextPtr object(new Text);
      object->mThis = object;
      return object;
    }

    Text::Text() :
    internal::Text()
    {
    }

    void Text::setValue(const String &inText, bool inCDATAFormat)
    {
      mOutputCDATA = inCDATAFormat;
      mInCDATAFormat = inCDATAFormat;
      mValue = inText;
    }

    void Text::setValueAndEntityEncode(const String &inText)
    {
      mOutputCDATA = false;
      mInCDATAFormat = false;
      mValue = Parser::makeTextEntitySafe(inText);
    }

    bool Text::getOutputCDATA() const
    {
      return mOutputCDATA;
    }

    void Text::setOutputCDATA(bool inOutputCDATA)
    {
      mOutputCDATA = inOutputCDATA;
    }

    NodePtr Text::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    void Text::clear()
    {
      mValue.clear();
      mOutputCDATA = false;
    }

    String Text::getValue() const
    {
      if (mInCDATAFormat)
        return Parser::makeTextEntitySafe(mValue);

      return mValue;
    }

    String Text::getValueAndEntityDecode() const
    {
      return Parser::convertFromEntities(getValue());
    }

    void Text::adoptAsFirstChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("text blocks cannot have children")
    }

    void Text::adoptAsLastChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("text blocks cannot have children")
    }



  } // namespace XML

} // namespace zsLib
