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
      #pragma mark XML::intenral::Text
      #pragma mark

      //-----------------------------------------------------------------------
      Text::Text() :
        mFormat(XML::Text::Format_EntityEncoded),
        mOutputFormat(XML::Text::Format_EntityEncoded)
      {
      }

      //-----------------------------------------------------------------------
      void Text::parse(XML::ParserPos &ioPos)
      {
        Parser::AutoStack stack(ioPos);

        mValue.clear();

        mFormat = (ioPos.isString("<![CDATA[") ? XML::Text::Format_CDATA : XML::Text::Format_EntityEncoded);
        mOutputFormat = mFormat;

        if (XML::Text::Format_CDATA == mFormat)
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
            (ioPos.getParser())->addWarning(ParserWarningType_CDATAMissingEndTag);
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

      //-----------------------------------------------------------------------
      ULONG Text::getOutputSizeXML(const GeneratorPtr &inGenerator) const
      {
        bool normalizeCDATA = (0 != (XML::Generator::XMLWriteFlag_NormalizeCDATA & inGenerator->getXMLWriteFlags()));
        bool encode0xDCharactersInText = (0 != (XML::Generator::XMLWriteFlag_EntityEncode0xDInText & inGenerator->getXMLWriteFlags()));
        if (encode0xDCharactersInText) {
          normalizeCDATA = true;
        }
        XML::Text::Formats outputFormat = static_cast<XML::Text::Formats>(mOutputFormat);
        if (normalizeCDATA) {
          if (XML::Text::Format_CDATA == outputFormat) {
            outputFormat = XML::Text::Format_EntityEncoded;
          }
        }

        if ((XML::Text::Format_JSONStringEncoded == outputFormat) ||
            (XML::Text::Format_JSONNumberEncoded == outputFormat)) {
          outputFormat = XML::Text::Format_EntityEncoded;
        }

        String value = getValueInFormat(outputFormat, normalizeCDATA, encode0xDCharactersInText);

        ULONG result = 0;

        if (XML::Text::Format_CDATA == outputFormat)
          result += (ULONG)strlen("<![CDATA[");

        result += (ULONG)value.length();

        if (XML::Text::Format_CDATA == outputFormat)
          result += (ULONG)strlen("]]>");

        return result;
      }

      //-----------------------------------------------------------------------
      void Text::writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const
      {
        bool normalizeCDATA = (0 != (XML::Generator::XMLWriteFlag_NormalizeCDATA & inGenerator->getXMLWriteFlags()));
        bool encode0xDCharactersInText = (0 != (XML::Generator::XMLWriteFlag_EntityEncode0xDInText & inGenerator->getXMLWriteFlags()));
        if (encode0xDCharactersInText) {
          normalizeCDATA = true;
        }
        XML::Text::Formats outputFormat = static_cast<XML::Text::Formats>(mOutputFormat);
        if (normalizeCDATA) {
          if (XML::Text::Format_CDATA == outputFormat) {
            outputFormat = XML::Text::Format_EntityEncoded;
          }
        }

        if ((XML::Text::Format_JSONStringEncoded == outputFormat) ||
            (XML::Text::Format_JSONNumberEncoded == outputFormat)) {
          outputFormat = XML::Text::Format_EntityEncoded;
        }

        String value = getValueInFormat(outputFormat, normalizeCDATA, encode0xDCharactersInText);

        if (XML::Text::Format_CDATA == outputFormat)
          Generator::writeBuffer(ioPos, "<![CDATA[");

        Generator::writeBuffer(ioPos, value);

        if (XML::Text::Format_CDATA == outputFormat)
          Generator::writeBuffer(ioPos, "]]>");
      }

      //-----------------------------------------------------------------------
      ULONG Text::getOutputSizeJSON(const GeneratorPtr &inGenerator) const
      {
        bool normalizeCDATA = (0 != (XML::Generator::XMLWriteFlag_NormalizeCDATA & inGenerator->getXMLWriteFlags()));
        String value = getValueInFormat(XML::Text::Format_JSONNumberEncoded == mFormat ? XML::Text::Format_JSONNumberEncoded : XML::Text::Format_JSONStringEncoded, normalizeCDATA);

        ULONG result = 0;
        result += (ULONG)value.length();
        return result;
      }

      //-----------------------------------------------------------------------
      void Text::writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const
      {
        bool normalizeCDATA = (0 != (XML::Generator::XMLWriteFlag_NormalizeCDATA & inGenerator->getXMLWriteFlags()));

        String value = getValueInFormat(XML::Text::Format_JSONNumberEncoded == mFormat ? XML::Text::Format_JSONNumberEncoded : XML::Text::Format_JSONStringEncoded, normalizeCDATA);
        Generator::writeBuffer(ioPos, value);
      }

      //-----------------------------------------------------------------------
      NodePtr Text::cloneAssignParent(NodePtr inParent) const
      {
        TextPtr newObject(XML::Text::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);
        newObject->mValue = mValue;
        newObject->mFormat = mFormat;
        newObject->mOutputFormat = mOutputFormat;
        (mThis.lock())->cloneChildren(mThis.lock(), newObject);    // should do noop since text nodes aren't allowed children
        return newObject;
      }

      //-----------------------------------------------------------------------
      String Text::getValueInFormat(
                                    UINT outputFormat,
                                    bool normalize,
                                    bool encode0xDCharactersInText
                                    ) const
      {
        String value = mValue;

        XML::Text::Formats inFormat = static_cast<XML::Text::Formats>(mFormat);

        if (normalize) {
          value = getValueInFormat(XML::Text::Format_CDATA, false);
          inFormat = XML::Text::Format_CDATA;
        }

        switch (static_cast<XML::Text::Formats>(outputFormat))
        {
          case XML::Text::Format_EntityEncoded:
          {
            switch (inFormat)
            {
              case XML::Text::Format_EntityEncoded:       return value;
              case XML::Text::Format_CDATA:               return XML::Parser::makeTextEntitySafe(value, encode0xDCharactersInText);
              case XML::Text::Format_JSONStringEncoded:   return XML::Parser::makeTextEntitySafe(XML::Parser::convertFromJSONEncoding(value));
              case XML::Text::Format_JSONNumberEncoded:   return XML::Parser::makeTextEntitySafe(XML::Parser::convertFromJSONEncoding(value));
            }
            break;
          }
          case XML::Text::Format_CDATA:
          {
            switch (inFormat)
            {
              case XML::Text::Format_EntityEncoded:       return XML::Parser::convertFromEntities(value);
              case XML::Text::Format_CDATA:               return value;
              case XML::Text::Format_JSONStringEncoded:   return XML::Parser::convertFromJSONEncoding(value);
              case XML::Text::Format_JSONNumberEncoded:   return XML::Parser::convertFromJSONEncoding(value);
            }
            break;
          }
          case XML::Text::Format_JSONStringEncoded:
          case XML::Text::Format_JSONNumberEncoded:
          {
            switch (inFormat)
            {
              case XML::Text::Format_EntityEncoded:       return XML::Parser::convertToJSONEncoding(XML::Parser::convertFromEntities(value));
              case XML::Text::Format_CDATA:               return XML::Parser::convertToJSONEncoding(value);
              case XML::Text::Format_JSONStringEncoded:   return value;
              case XML::Text::Format_JSONNumberEncoded:   return value;
            }
            break;
          }
        }
        return value;
      }

    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::Text
    #pragma mark

    //-------------------------------------------------------------------------
    TextPtr Text::create()
    {
      TextPtr object(new Text);
      object->mThis = object;
      return object;
    }

    //-------------------------------------------------------------------------
    Text::Text() :
      internal::Text()
    {
    }

    //-------------------------------------------------------------------------
    void Text::setValue(const String &inText, Formats format)
    {
      mFormat = format;
      mOutputFormat = format;

      mValue = inText;
    }

    //-------------------------------------------------------------------------
    void Text::setValueAndEntityEncode(const String &inText)
    {
      mFormat = Format_EntityEncoded;
      mOutputFormat = Format_EntityEncoded;

      mValue = Parser::makeTextEntitySafe(inText);
    }

    //-------------------------------------------------------------------------
    void Text::setValueAndJSONEncode(const String &inText)
    {
      mFormat = Format_JSONStringEncoded;
      mOutputFormat = Format_JSONStringEncoded;

      mValue = Parser::convertToJSONEncoding(inText);
    }

    //-------------------------------------------------------------------------
    Text::Formats Text::getFormat() const
    {
      return static_cast<Text::Formats>(mFormat);
    }

    //-------------------------------------------------------------------------
    Text::Formats Text::getOutputFormat() const
    {
      return static_cast<Text::Formats>(mOutputFormat);
    }

    //-------------------------------------------------------------------------
    void Text::setOutputFormat(Formats format)
    {
      mOutputFormat = format;
    }

    //-------------------------------------------------------------------------
    NodePtr Text::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    //-------------------------------------------------------------------------
    void Text::clear()
    {
      mValue.clear();
      mFormat = Format_EntityEncoded;
      mOutputFormat = Format_EntityEncoded;
    }

    //-------------------------------------------------------------------------
    String Text::getValue() const
    {
      return mValue;
    }

    //-------------------------------------------------------------------------
    String Text::getValueDecoded() const
    {
      if (Format_EntityEncoded == mFormat)
        return Parser::convertFromEntities(getValue());

      if ((Format_JSONStringEncoded == mFormat) ||
          (Format_JSONNumberEncoded == mFormat))
        return Parser::convertFromJSONEncoding(getValue());

      return mValue;
    }

    //-------------------------------------------------------------------------
    String Text::getValueInFormat(
                                  Formats outputFormat,
                                  bool normalize,
                                  bool encode0xDCharactersInText
                                  ) const
    {
      return internal::Text::getValueInFormat((UINT)outputFormat, normalize, encode0xDCharactersInText);
    }

    //-------------------------------------------------------------------------
    void Text::adoptAsFirstChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("text blocks cannot have children")
    }

    //-------------------------------------------------------------------------
    void Text::adoptAsLastChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("text blocks cannot have children")
    }

  } // namespace XML

} // namespace zsLib
