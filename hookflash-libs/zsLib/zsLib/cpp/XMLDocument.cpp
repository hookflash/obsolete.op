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
#include <zsLib/Numeric.h>

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

#define ZS_INTERNAL_STACK_BUFFER_PADDING_SPACE 1024

namespace zsLib
{

  namespace XML
  {

    namespace internal
    {

      Document::Document() :
        mTabSize(ZS_INTERNAL_XML_DEFAULT_TAB_SIZE),
        mSOF(NULL),
        mElementNameCaseSensative(true),
        mAttributeNameCaseSensative(true),
        mEnableWarnings(true),
        mWriteFlags(0)
      {
      }

      void Document::clearStack()
      {
        mParserStack.clear();
      }

      void Document::pushPos(const XML::ParserPos &inPos)
      {
        mParserStack.push_back(inPos);
      }

      XML::ParserPos Document::popPos()
      {
        ZS_THROW_BAD_STATE_IF(mParserStack.size() < 1)

        XML::ParserPos temp;
        temp = mParserStack.back();
        mParserStack.pop_back();
        return temp;
      }

      void Document::addWarning(ParserWarningTypes inWarning)
      {
        if (mEnableWarnings)
        {
          XML::ParserWarning warning(inWarning, mParserStack);
          mWarnings.push_back(warning);
        }
      }

      void Document::addWarning(ParserWarningTypes inWarning, const XML::ParserPos &inPos)
      {
        if (mEnableWarnings)
        {
          pushPos(inPos);
          addWarning(inWarning);
          popPos();
        }
      }

      ULONG Document::getOutputSize() const
      {
        DocumentPtr self(mThis.lock());

        ULONG result = 0;
        NodePtr child = self->getFirstChild();
        while (child)
        {
          result += Parser::getOutputSize(mThis.lock(), child);
          child = child->getNextSibling();
        }
        return result;
      }

      void Document::writeBuffer(char * &ioPos) const
      {
        DocumentPtr self(mThis.lock());

        NodePtr child = self->getFirstChild();
        while (child)
        {
          Parser::writeBuffer(mThis.lock(), child, ioPos);
          child = child->getNextSibling();
        }
      }

      NodePtr Document::cloneAssignParent(NodePtr inParent) const
      {
        DocumentPtr newObject(XML::Document::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);

        newObject->mTabSize = mTabSize;
        newObject->mSOF = mSOF;
        newObject->mElementNameCaseSensative = mElementNameCaseSensative;
        newObject->mAttributeNameCaseSensative = mAttributeNameCaseSensative;
        newObject->mEnableWarnings = mEnableWarnings;
        newObject->mSingleElements = mSingleElements;
        newObject->mWarnings = mWarnings;
        newObject->mParserStack = mParserStack;

        (mThis.lock())->cloneChildren(mThis.lock(), newObject);

        return newObject;
      }

    } // namespace internal


    Document::Document() :
    internal::Document()
    {
    }

    DocumentPtr Document::create()
    {
      Document *object = new Document;
      DocumentPtr newObject(object);
      object->mThis = newObject;
      return newObject;
    }

    ULONG Document::getTabSize() const
    {
      return mTabSize;
    }

    void Document::setTabSize(ULONG inTabSize)
    {
      mTabSize = inTabSize;
    }

    void Document::addContainsNoChildrenElement(String inElement)
    {
      for (StringList::iterator iter = mSingleElements.begin(); iter != mSingleElements.end(); ++iter)
      {
        if (mElementNameCaseSensative)
        {
          if ((*iter) == inElement)
            return;
        }
        else
        {
          if (0 == ((*iter).compareNoCase(inElement)))
            return;
        }
      }
      mSingleElements.push_back(inElement);
    }

    const Document::StringList &Document::getContainsNoChildrenElements()
    {
      return mSingleElements;
    }

    void Document::setElementNameIsCaseSensative(bool inCaseSensative)
    {
      mElementNameCaseSensative = inCaseSensative;
    }

    bool Document::isElementNameIsCaseSensative() const
    {
      return mElementNameCaseSensative;
    }

    void Document::setAttributeNameIsCaseSensative(bool inCaseSensative)
    {
      mAttributeNameCaseSensative = inCaseSensative;
    }

    bool Document::isAttributeNameIsCaseSensative() const
    {
      return mAttributeNameCaseSensative;
    }

    void Document::clearWarnings()
    {
      mWarnings.clear();
    }

    const Document::Warnings &Document::getWarnings() const
    {
      return mWarnings;
    }

    void Document::parse(const char *inXMLDocument)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!inXMLDocument)

      mSOF = inXMLDocument;
      ParserPos pos(*this);

      clearStack();
      clearWarnings();

      while (*pos)
      {
        if (Parser::parseAnyExceptElement(pos, mThis.lock()))
          continue;

        // check if this is an element
        if (Parser::isLegalName(*(pos+1), true))
        {
          // this is an element
          ElementPtr element = XML::Element::create();
          adoptAsLastChild(element);
          element->parse(pos);
          continue;
        }

        Parser::skipMismatchedEndTag(pos);
      }

      clearStack();
    }

    ULONG Document::getOutputSize() const
    {
      return internal::Document::getOutputSize();
    }

    boost::shared_array<char> Document::write(ULONG *outLength) const
    {
      ULONG totalSize = Parser::getOutputSize(mThis.lock(), toNode());
      boost::shared_array<char> buffer(new char[totalSize+1]);
      char *pos = buffer.get();
      Parser::writeBuffer(mThis.lock(), toNode(), pos);
      *pos = 0;
      if (NULL != outLength)
        *outLength = totalSize;
      return buffer;
    }

    ULONG Document::getOutputSize(const NodePtr &onlyThisNode) const
    {
      return Parser::getOutputSize(mThis.lock(), onlyThisNode);
    }

    boost::shared_array<char> Document::write(const NodePtr &onlyThisNode, ULONG *outLength) const
    {
      ULONG totalSize = Parser::getOutputSize(mThis.lock(), onlyThisNode);
      boost::shared_array<char> buffer(new char[totalSize+1]);
      char *pos = buffer.get();
      Parser::writeBuffer(mThis.lock(), onlyThisNode, pos);
      *pos = 0;
      if (NULL != outLength)
        *outLength = totalSize;
      return buffer;
    }

    void Document::setWriteFlags(WriteFlags flags)
    {
      mWriteFlags = static_cast<UINT>(flags);
    }

    Document::WriteFlags Document::getWriteFlags() const
    {
      return static_cast<WriteFlags>(mWriteFlags);
    }

    NodePtr Document::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    void Document::clear()
    {
      mTabSize = ZS_INTERNAL_XML_DEFAULT_TAB_SIZE;
      mSOF = NULL;
      mElementNameCaseSensative = true;
      mAttributeNameCaseSensative = true;
      mEnableWarnings = true;
      mSingleElements.clear();
      mWarnings.clear();
      mParserStack.clear();

      Node::clear();
    }

  } // namespace XML

} // namespace zsLib
