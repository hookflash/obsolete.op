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
#include <zsLib/Numeric.h>

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

#define ZS_INTERNAL_STACK_BUFFER_PADDING_SPACE 1024

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
      #pragma mark XML::internal::Document
      #pragma mark

      //-----------------------------------------------------------------------
      Document::Document(
                         bool inElementNameIsCaseSensative,
                         bool inAttributeNameIsCaseSensative
                         ) :
        mElementNameCaseSensative(inElementNameIsCaseSensative),
        mAttributeNameCaseSensative(inAttributeNameIsCaseSensative)
      {
      }

      //-----------------------------------------------------------------------
      ULONG Document::getOutputSizeXML(const GeneratorPtr &generator) const
      {
        DocumentPtr self(mThis.lock());

        ULONG result = 0;
        NodePtr child = self->getFirstChild();
        while (child)
        {
          result += Generator::getOutputSize(generator, child);
          child = child->getNextSibling();
        }
        return result;
      }

      //-----------------------------------------------------------------------
      void Document::writeBufferXML(const GeneratorPtr &generator, char * &ioPos) const
      {
        DocumentPtr self(mThis.lock());

        NodePtr child = self->getFirstChild();
        while (child)
        {
          Generator::writeBuffer(generator, child, ioPos);
          child = child->getNextSibling();
        }
      }

      //-----------------------------------------------------------------------
      ULONG Document::getOutputSizeJSON(const GeneratorPtr &generator) const
      {
        DocumentPtr self(mThis.lock());

        bool foundText = false;

        ULONG result = 0;
        NodePtr child = self->getFirstChild();
        while (child)
        {
          if (child->isText()) {
            foundText = true;
          }
          result += Generator::getOutputSize(generator, child);
          child = child->getNextSibling();
        }

        if (foundText) {
          // text at the root will have to become encoded as a single entry text node
          result += strlen("\"\":\"\"");
          if (self->getFirstChildElement()) {
            result += strlen(",");
          }
          result += strlen(generator->mJSONForcedText);
        }

        return result;
      }

      //-----------------------------------------------------------------------
      void Document::writeBufferJSON(const GeneratorPtr &generator, char * &ioPos) const
      {
        DocumentPtr self(mThis.lock());

        bool foundText = false;

        NodePtr child = self->getFirstChild();
        while (child)
        {
          if (child->isText()) {
            if (!foundText) {
              Generator::writeBuffer(ioPos, "\"");
              Generator::writeBuffer(ioPos, generator->mJSONForcedText);
              Generator::writeBuffer(ioPos, "\":\"");
            }
            Generator::writeBuffer(generator, child, ioPos);
            foundText = true;
          }
          child = child->getNextSibling();
        }
        if (foundText) {
          Generator::writeBuffer(ioPos, "\"");
          if (self->getFirstChildElement()) {
            Generator::writeBuffer(ioPos, ",");
          }
        }

        child = self->getFirstChild();
        while (child)
        {
          if (!child->isText()) {
            // text was already handled, now write out all the non-text
            Generator::writeBuffer(generator, child, ioPos);
          }
          child = child->getNextSibling();
        }
      }

      //-----------------------------------------------------------------------
      NodePtr Document::cloneAssignParent(NodePtr inParent) const
      {
        DocumentPtr newObject(XML::Document::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);

        newObject->mElementNameCaseSensative = mElementNameCaseSensative;
        newObject->mAttributeNameCaseSensative = mAttributeNameCaseSensative;

        (mThis.lock())->cloneChildren(mThis.lock(), newObject);

        return newObject;
      }

    } // namespace internal


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::Document
    #pragma mark

    //-------------------------------------------------------------------------
    Document::Document(
                       bool inElementNameIsCaseSensative,
                       bool inAttributeNameIsCaseSensative
                       ) :
      internal::Document(inElementNameIsCaseSensative, inAttributeNameIsCaseSensative)
    {
    }

    //-------------------------------------------------------------------------
    DocumentPtr Document::create(
                                 bool inElementNameIsCaseSensative,
                                 bool inAttributeNameIsCaseSensative
                                 )
    {
      DocumentPtr pThis(new Document(inElementNameIsCaseSensative, inAttributeNameIsCaseSensative));
      pThis->mThis = pThis;
      return pThis;
    }

    //-------------------------------------------------------------------------
    DocumentPtr Document::createFromParsedXML(
                                              const char *inXMLDocument,
                                              bool inElementNameIsCaseSensative,
                                              bool inAttributeNameIsCaseSensative
                                              )
    {
      ParserPtr parser = Parser::createXMLParser();
      return parser->parse(inXMLDocument, inElementNameIsCaseSensative, inAttributeNameIsCaseSensative);
    }

    //-------------------------------------------------------------------------
    DocumentPtr Document::createFromParsedJSON(
                                               const char *inJSONDocument,
                                               const char *forcedText,
                                               char attributePrefix,
                                               bool inElementNameIsCaseSensative,
                                               bool inAttributeNameIsCaseSensative
                                               )
    {
      ParserPtr parser = Parser::createJSONParser(forcedText, attributePrefix);
      return parser->parse(inJSONDocument, inElementNameIsCaseSensative, inAttributeNameIsCaseSensative);
    }

    //-------------------------------------------------------------------------
    DocumentPtr Document::createFromAutoDetect(
                                               const char *inDocument,
                                               const char *forcedText,
                                               char attributePrefix,
                                               bool inElementNameIsCaseSensative,
                                               bool inAttributeNameIsCaseSensative
                                               )
    {
      ParserPtr parser = Parser::createAutoDetectParser(forcedText, attributePrefix);
      return parser->parse(inDocument, inElementNameIsCaseSensative, inAttributeNameIsCaseSensative);
    }

    //-------------------------------------------------------------------------
    void Document::setElementNameIsCaseSensative(bool inCaseSensative)
    {
      mElementNameCaseSensative = inCaseSensative;
    }

    //-------------------------------------------------------------------------
    bool Document::isElementNameIsCaseSensative() const
    {
      return mElementNameCaseSensative;
    }

    //-------------------------------------------------------------------------
    void Document::setAttributeNameIsCaseSensative(bool inCaseSensative)
    {
      mAttributeNameCaseSensative = inCaseSensative;
    }

    //-------------------------------------------------------------------------
    bool Document::isAttributeNameIsCaseSensative() const
    {
      return mAttributeNameCaseSensative;
    }

    //-------------------------------------------------------------------------
    boost::shared_array<char> Document::writeAsXML(ULONG *outLength) const
    {
      GeneratorPtr generator = Generator::createXMLGenerator();
      return generator->write(mThis.lock(), outLength);
    }

    //-------------------------------------------------------------------------
    boost::shared_array<char> Document::writeAsJSON(ULONG *outLength) const
    {
      GeneratorPtr generator = Generator::createJSONGenerator();
      return generator->write(mThis.lock(), outLength);
    }

    //-------------------------------------------------------------------------
    NodePtr Document::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    //-------------------------------------------------------------------------
    void Document::clear()
    {
      mElementNameCaseSensative = true;
      mAttributeNameCaseSensative = true;

      Node::clear();
    }

  } // namespace XML

} // namespace zsLib
