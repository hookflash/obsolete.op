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

#pragma warning(push)
#pragma warning(disable: 4290)

namespace zsLib
{

  namespace XML
  {

    namespace internal
    {

      void Element::parse(XML::ParserPos &ioPos)
      {
        ElementPtr temp = mThis.lock();
        actualParse(temp, ioPos);
      }

      ULONG Element::getOutputSize(const DocumentPtr &inDocument) const
      {
        ZS_THROW_INVALID_USAGE_IF(mName.isEmpty())

        class Walker : public WalkSink
        {
        public:
          Walker(ULONG &outResult, const DocumentPtr &inDocument) : mResult(outResult), mDocument(inDocument) {}

          virtual bool onElementEnter(ElementPtr inNode)
          {
            mResult += (ULONG)strlen("<");
            mResult += inNode->mName.getLength();

            if (inNode->mFirstAttribute)
            {
              NodePtr child = inNode->mFirstAttribute;
              while (child)
              {
                AttributePtr attribute = child->toAttribute();
                mResult += (ULONG)strlen(" ");
                mResult += Parser::getOutputSize(mDocument, attribute);
                child = child->getNextSibling();
              }
            }

            bool forceEndTag = (0 != (XML::Document::WriteFlag_ForceElementEndTag & (mDocument->getWriteFlags())));

            if ((inNode->hasChildren()) || (forceEndTag))
            {
              mResult += (ULONG)strlen(">");

              // inner content would go here

              mResult += (ULONG)strlen("</");
              mResult += inNode->mName.getLength();
              mResult += (ULONG)strlen(">");
            }
            else
            {
              mResult += (ULONG)strlen(" />");
            }
            return false;
          }

          virtual bool onText(TextPtr inNode)                      {mResult += inNode->getOutputSize(mDocument); return false;}
          virtual bool onComment(CommentPtr inNode)                {mResult += inNode->getOutputSize(mDocument); return false;}
          virtual bool onDeclarationEnter(DeclarationPtr inNode)   {mResult += inNode->getOutputSize(mDocument); return false;}
          virtual bool onUnknown(UnknownPtr inNode)                {mResult += inNode->getOutputSize(mDocument); return false;}

        private:
          const DocumentPtr &mDocument;
          ULONG &mResult;
        };

        ElementPtr self(mThis.lock());

        ULONG result = 0;
        XML::Node::FilterList filter;
        filter.push_back(XML::Node::NodeType::Element);
        filter.push_back(XML::Node::NodeType::Text);
        filter.push_back(XML::Node::NodeType::Comment);
        filter.push_back(XML::Node::NodeType::Declaration);
        filter.push_back(XML::Node::NodeType::Unknown);
        Walker walker(result, inDocument);
        self->walk(walker, &filter);

        return result;
      }

      void Element::writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const
      {
        ZS_THROW_INVALID_USAGE_IF(mName.isEmpty())

        class Walker : public WalkSink
        {
        public:
          Walker(const DocumentPtr &inDocument, char * &ioPos) :
            mDocument(inDocument),
            mPos(ioPos)
          {}

          virtual bool onElementEnter(ElementPtr inNode)
          {
            Parser::writeBuffer(mPos, "<");
            Parser::writeBuffer(mPos, inNode->mName);

            if (inNode->mFirstAttribute)
            {
              NodePtr child = inNode->mFirstAttribute;
              while (child)
              {
                AttributePtr attribute = child->toAttribute();
                Parser::writeBuffer(mPos, " ");
                Parser::writeBuffer(mDocument, attribute, mPos);
                child = child->getNextSibling();
              }
            }

            bool forceEndTag = (0 != (XML::Document::WriteFlag_ForceElementEndTag & (mDocument->getWriteFlags())));

            if ((inNode->hasChildren()) || (forceEndTag))
              Parser::writeBuffer(mPos, ">");
            else
              Parser::writeBuffer(mPos, " />");

            return false;
          }

          virtual bool onElementExit(ElementPtr inNode)
          {
            bool forceEndTag = (0 != (XML::Document::WriteFlag_ForceElementEndTag & (mDocument->getWriteFlags())));

            if ((!inNode->hasChildren()) && (!forceEndTag))
              return false;

            // put on the final end tag which won't know about
            Parser::writeBuffer(mPos, "</");
            Parser::writeBuffer(mPos, inNode->mName);
            Parser::writeBuffer(mPos, ">");
            return false;
          }

          virtual bool onText(TextPtr inNode)                      {Parser::writeBuffer(mDocument, inNode, mPos); return false;}
          virtual bool onComment(CommentPtr inNode)                {Parser::writeBuffer(mDocument, inNode, mPos); return false;}
          virtual bool onDeclarationEnter(DeclarationPtr inNode)   {Parser::writeBuffer(mDocument, inNode, mPos); return false;}
          virtual bool onUnknown(UnknownPtr inNode)                {Parser::writeBuffer(mDocument, inNode, mPos); return false;}

        private:
          const DocumentPtr &mDocument;
          char * &mPos;
        };

        ElementPtr self(mThis.lock());

        XML::Node::FilterList filter;
        filter.push_back(XML::Node::NodeType::Element);
        filter.push_back(XML::Node::NodeType::Text);
        filter.push_back(XML::Node::NodeType::Comment);
        filter.push_back(XML::Node::NodeType::Declaration);
        filter.push_back(XML::Node::NodeType::Unknown);
        Walker walker(inDocument, ioPos);
        self->walk(walker, &filter);
      }

      NodePtr Element::cloneAssignParent(NodePtr inParent) const
      {
        ElementPtr newObject(XML::Element::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);

        ElementPtr startingElement = mThis.lock();

        newObject->mName = mName;
        newObject->cloneAttributes(startingElement);

        // going to walk the tree in a non-recursive fashion
        NodePtr walk = startingElement;
        NodePtr cloneWalk = newObject;

        bool allowChildren = true;

        while (walk)
        {
          if ((walk->getFirstChild()) &&
              (allowChildren))
          {
            // there is a child
            walk = walk->getFirstChild();
            if (!walk->isElement())
            {
              // clone and continue walking
              cloneWalk = Parser::cloneAssignParent(cloneWalk, walk);
              continue;
            }

            // this is an element, do some special handling
            ElementPtr newObject(XML::Element::create());
            Parser::safeAdoptAsLastChild(cloneWalk, newObject);

            newObject->mName = walk->toElement()->mName;
            newObject->cloneAttributes(walk->toElement());
            cloneWalk = newObject;
            continue;
          }

          if (walk->getNextSibling())
          {
            allowChildren = true;

            // cannot walk beyond the start of the clone
            if (walk == startingElement->toNode())
              break;

            walk = walk->getNextSibling();
            if (!walk->isElement())
            {
              // clone and continue walking
              cloneWalk = Parser::cloneAssignParent(cloneWalk->getParent(), walk);
              continue;
            }

            // this is an element, do some special handling
            ElementPtr newObject(XML::Element::create());
            Parser::safeAdoptAsLastChild(cloneWalk->getParent(), newObject);

            newObject->mName = walk->toElement()->mName;
            newObject->cloneAttributes(walk->toElement());
            cloneWalk = newObject;
            continue;
          }

          // cannot walk beyond the starting point when cloning
          if (walk == startingElement->toNode())
            break;

          allowChildren = false;     // do not allow a child node to be processed next
          walk = walk->getParent();
          cloneWalk = cloneWalk->getParent();
        }

        return newObject;
      }

      void Element::cloneAttributes(ElementPtr inOriginalElement)
      {
        NodePtr newObject = mThis.lock();

        NodePtr child = inOriginalElement->mFirstAttribute;
        while (child)
        {
          NodePtr newChild = Parser::cloneAssignParent(newObject, child);
          child = child->getNextSibling();
        }
      }

      void Element::actualParse(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos)
      {
        ULONG totalPushes = 0;  // used to keep track how many stack positions are pushed on by this method doing its parse

        // push the pos manually for the current element
        (ioPos.getDocument())->pushPos(ioPos);
        ++totalPushes;

        while (*ioPos)
        {
          // this must be a <
          ZS_THROW_BAD_STATE_IF('<' != *ioPos)

          ++ioPos;
          ioCurrentElement->mName = Parser::parseLegalName(ioPos);

          // could not have got in here if this was an illegal name
          ZS_THROW_BAD_STATE_IF(ioCurrentElement->mName.isEmpty())

          parseAttributes(ioCurrentElement, ioPos);

          bool foundEndSlash = parseTagSingleElementEndSlash(ioPos);

          // there must be an end '>' at this point
          if ('>' != *ioPos)
          {
            (ioPos.getDocument())->addWarning(ParserWarningType_NoEndBracketFound);
          }
          else
          {
            // skip over the close >
            ++ioPos;
          }

          // check if this is a single element
          if (!foundEndSlash)
            foundEndSlash = parseIsDocumentSingleElement(ioCurrentElement, ioPos);

          // is this an element with no contents or is the stack too deep to continue parsing at this level?
          if (foundEndSlash)
          {
            parsePopElement(ioCurrentElement, ioPos, totalPushes);
            if (!ioCurrentElement)
            {
              parsePopPushes(ioPos, totalPushes);
              return;
            }
          }

          bool foundEndTag = false;
          bool foundStartTag = false;
          while (*ioPos)
          {
            if (Parser::parseAnyExceptElement(ioPos, ioCurrentElement))
              continue;

            // must be a new element, or an end of element
            ZS_THROW_BAD_STATE_IF('<' != *ioPos)

            foundStartTag = parseNewElement(ioCurrentElement, ioPos, totalPushes);
            if (foundStartTag)
              break;

            // this must be an end tag
            ZS_THROW_BAD_STATE_IF(!ioPos.isString("</"))

            foundEndTag = true;
            parseEndTag(ioCurrentElement, ioPos, totalPushes);
            if (!ioCurrentElement)
            {
              parsePopPushes(ioPos, totalPushes);
              return;
            }
          }

          if ((!foundEndTag) &&
              (!foundStartTag))
          {
            (ioPos.getDocument())->addWarning(ParserWarningType_NoEndTagFound);
          }
        }

        parsePopPushes(ioPos, totalPushes);
      }

      void Element::parseAttributes(ElementPtr inCurrentElement, XML::ParserPos &ioPos)
      {
        while ((*ioPos) &&
               ('>' != *ioPos) &&
               ('/' != *ioPos))
        {
          if (Parser::skipWhiteSpace(ioPos))
            continue;

          if (!Parser::isLegalName(*ioPos, true))
          {
            // this isn't a legal name for an attribute, skip it
            (ioPos.getDocument())->addWarning(ParserWarningType_IllegalAttributeName, ioPos);

            while ((*ioPos) &&
                   ('>' != *ioPos) &&
                   ('/' != *ioPos) &&
                   (!Parser::isWhiteSpace(ioPos)))
            {
              ++ioPos;
            }
            continue;
          }

          Parser::AutoStack stack(ioPos);

          AttributePtr attribute = XML::Attribute::create();
          if (!attribute->parse(ioPos))
            continue;

          // attribute was found, add it to the attribute list
          bool duplicateAttribute = inCurrentElement->setAttribute(attribute);
          if (duplicateAttribute)
          {
            (ioPos.getDocument())->addWarning(ParserWarningType_DuplicateAttribute);
          }
        }
      }

      bool Element::parseTagSingleElementEndSlash(XML::ParserPos &ioPos)
      {
        if ('/' != *ioPos)
          return false;

        ++ioPos;
        Parser::skipWhiteSpace(ioPos);

        bool warnedBogusStuff = false;
        while ((*ioPos) &&
               ('>' != *ioPos))
        {
          if (!warnedBogusStuff)
          {
            (ioPos.getDocument())->addWarning(ParserWarningType_ContentAfterCloseSlashInElement, ioPos);
            warnedBogusStuff = true;
          }
          ++ioPos;
        }

        return true;
      }

      bool Element::parseIsDocumentSingleElement(ElementPtr inCurrentElement, XML::ParserPos &ioPos)
      {
        bool caseSensative = (ioPos.getDocument())->isElementNameIsCaseSensative();

        const Document::StringList &singleElements = (ioPos.getDocument())->getContainsNoChildrenElements();
        for (Document::StringList::const_iterator iter = singleElements.begin(); iter != singleElements.end(); ++iter)
        {
          if (caseSensative)
          {
            if ((*iter) == inCurrentElement->mName)
              return true;
          }
          else
          {
            if (0 == (*iter).compareNoCase(inCurrentElement->mName))
              return true;
          }
        }

        return false;
      }

      bool Element::parseNewElement(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos, ULONG &ioTotalPushes)
      {
        if (!Parser::isLegalName(*(ioPos + 1), true))
          return false;

        // this is a new element
        (ioPos.getDocument())->pushPos(ioPos);
        ++ioTotalPushes;

        ElementPtr element = XML::Element::create();
        ioCurrentElement->adoptAsLastChild(element);
        ioCurrentElement = element;
        return true;
      }

      void Element::parseEndTag(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos, ULONG &ioTotalPushes)
      {
        XML::ParserPos temp(ioPos);

        temp += ((ULONG)strlen("</"));
        Parser::skipWhiteSpace(temp);

        // this could be the end tag for this element
        String elementName = Parser::parseLegalName(temp);
        if (elementName.isEmpty())
        {
          if ('>' == *temp)
          {
            // generic close tag, skip over and done parsing this element
            ioPos = temp;
            ++ioPos;

            parsePopElement(ioCurrentElement, ioPos, ioTotalPushes);
            return;
          }

          if (temp.isEOF())
          {
            // already warned about, no need to warn a second time
            //(temp.getDocument())->addWarning(ParserWarningType_NoEndBracketFound, ioPos);
            ioPos = temp;
            return;
          }

          ZS_THROW_BAD_STATE("This should have been skipped by the outer parser")
        }

        // if this end tag does not belong to the current element, the break out and give to outer element a chance to parse
        bool caseSensative = (ioPos.getDocument())->isElementNameIsCaseSensative();

        while (true)
        {
          // attempt to find the element it belongs to
          if (caseSensative)
          {
            if (ioCurrentElement->mName == elementName)
              break;
          }
          else
          {
            if (0 == ioCurrentElement->mName.compareNoCase(elementName))
              break;
          }

          parsePopElement(ioCurrentElement, ioPos, ioTotalPushes);
          if (!ioCurrentElement)
            return;
        }

        // the end tag belongs to the element
        Parser::skipWhiteSpace(temp);
        if ((*temp) &&
            ('>' != *temp))
        {
          // found bogus junk to skip
          (temp.getDocument())->addWarning(ParserWarningType_ContentAfterCloseElementName, temp);

          while ((*temp) &&
                 ('>' != *temp))
          {
            ++temp;
          }
        }

        if ('>' == *temp)
        {
          // closed tag, skip over and done parsing this element
          ioPos = temp;
          ++ioPos;

          parsePopElement(ioCurrentElement, ioPos, ioTotalPushes);
          return;
        }

        ZS_THROW_BAD_STATE_IF(!temp.isEOF())

        (ioPos.getDocument())->addWarning(ParserWarningType_NoEndBracketFound, ioPos);
        ioPos = temp;
      }

      void Element::parsePopElement(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos, ULONG &ioTotalPushes)
      {
        // this element is popped off the stack
        ZS_THROW_BAD_STATE_IF(0 == ioTotalPushes)
        (ioPos.getDocument())->popPos();
        --ioTotalPushes;

        // this element is done, need to return control to the previous element
        NodePtr parent = ioCurrentElement->getParent();
        if (parent->isDocument())
        {
          ioCurrentElement = ElementPtr();
          return;
        }
        ioCurrentElement = parent->toElement();
        ZS_THROW_BAD_STATE_IF(!ioCurrentElement)
      }

      void Element::parsePopPushes(XML::ParserPos &ioPos, ULONG &ioTotalPushes)
      {
        // this is done just in case the parser suddenly completes with some remaining stack still pushed at the end
        // since we aren't using the auto-stack to ensure the stack remains properly intact
        while (0 != ioTotalPushes)
        {
          (ioPos.getDocument())->popPos();
          --ioTotalPushes;
        }
      }


    } // namespace internal

    //----------------------------------------------------------------------------

    Element::Element() :
    internal::Element()
    {
    }

    ElementPtr Element::create(const char *name)
    {
      ElementPtr newObject(new Element());
      newObject->mThis = newObject;
      if (name) newObject->setValue(name);
      return newObject;
    }

    String Element::getText(
                            bool inCompressWhiteSpace,
                            bool inIncludeTextOfChildElements
                            )
    {
      // result will include all entities, to be converted later if required
      String result;

      if (!inIncludeTextOfChildElements)
      {
        NodePtr child = getFirstChild();
        while (child)
        {
          if (child->isText())
          {
            result += (child->toText()->getValue());
          }

          child = child->getNextSibling();
        }
      }
      else
      {
        class TextWalker : public WalkSink
        {
        public:
          TextWalker(String &outResult) : mResult(outResult) {}
          virtual bool onText(TextPtr inNode)
          {
            mResult += inNode->getValue();
            return false;
          }

        private:
          String &mResult;
        };

        TextWalker walker(result);
        walk(walker, NodeType::Text);
      }

      if (inCompressWhiteSpace)
      {
        // compress white space (additional &nbsp; do not get compressed)
        result = Parser::compressWhiteSpace(result);
      }

      return result;
    }

    String Element::getTextAndEntityDecode(
                                           bool inCompressWhiteSpace,
                                           bool inIncludeTextOfChildElements
                                           )
    {
      // result will include all entities, to be converted later if required
      String result;

      if (!inIncludeTextOfChildElements)
      {
        NodePtr child = getFirstChild();
        while (child)
        {
          if (child->isText())
          {
            result += (child->toText()->getValueAndEntityDecode());
          }

          child = child->getNextSibling();
        }
      }
      else
      {
        class TextWalker : public WalkSink
        {
        public:
          TextWalker(String &outResult) : mResult(outResult) {}
          virtual bool onText(TextPtr inNode)
          {
            mResult += inNode->getValueAndEntityDecode();
            return false;
          }

        private:
          String &mResult;
        };

        TextWalker walker(result);
        walk(walker, NodeType::Text);
      }

      if (inCompressWhiteSpace)
      {
        // compress white space (additional &nbsp; do not get compressed)
        result = Parser::compressWhiteSpace(result);
      }

      return result;
    }
    
    AttributePtr Element::findAttribute(String inName) const
    {
      DocumentPtr document(getDocument());

      bool caseSensative = true;
      if (document)
        caseSensative = document->isAttributeNameIsCaseSensative();

      NodePtr child = mFirstAttribute;
      while (child)
      {
        AttributePtr attribute(child->toAttribute());
        if (caseSensative)
        {
          if (inName == attribute->getName())
            return attribute;
        }
        else
        {
          if (0 == inName.compareNoCase(attribute->getName()))
            return attribute;
        }
        child = child->getNextSibling();
      }
      return AttributePtr();
    }

    String Element::getAttributeValue(String inName) const
    {
      AttributePtr attribute = findAttribute(inName);
      if (attribute)
        return attribute->getValue();

      return String();
    }

    AttributePtr Element::findAttributeChecked(String inName) const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = findAttribute(inName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    String Element::getAttributeValueChecked(String inName) const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = findAttribute(inName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result->getValue();
    }

    bool Element::setAttribute(String inName, String inValue)
    {
      AttributePtr attribute = Attribute::create();
      attribute->setName(inName);
      attribute->setValue(inValue);
      return setAttribute(attribute);
    }

    bool Element::setAttribute(AttributePtr inAttribute)
    {
      ZS_THROW_INVALID_USAGE_IF(!inAttribute)

      bool replaced = deleteAttribute(inAttribute->getName());
      mThis.lock()->adoptAsLastChild(inAttribute);
      return replaced;
    }

    bool Element::deleteAttribute(String inName)
    {
      AttributePtr attribute = findAttribute(inName);
      if (!attribute)
        return false;

      attribute->orphan();
      return true;
    }

    AttributePtr Element::getFirstAttribute() const
    {
      return mFirstAttribute;
    }

    AttributePtr Element::getLastAttribute() const
    {
      return mLastAttribute;
    }

    AttributePtr Element::getFirstAttributeChecked() const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = getFirstAttribute();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    AttributePtr Element::getLastAttributeChecked() const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = getLastAttribute();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    void Element::adoptAsFirstChild(NodePtr inNode)
    {
      if (!inNode)
        return;

      if (!inNode->isAttribute())
      {
        Node::adoptAsFirstChild(inNode);
        return;
      }

      AttributePtr attribute = inNode->toAttribute();

      // first the node has to be orphaned
      inNode->orphan();

      // ensure this attribute does not already exist
      deleteAttribute(attribute->getName());

      // adopt this as the parent
      inNode->mParent = toNode();

      // point existing first child's previous sibling to new node
      if (mFirstAttribute)
        mFirstAttribute->mPreviousSibling = inNode;

      // point new node's next sibling to current first child
      inNode->mNextSibling = mFirstAttribute;

      // first child is now new node
      mFirstAttribute = attribute;

      // if there wasn't a last child then it is now pointing to the new node
      if (!mLastAttribute)
        mLastAttribute = attribute;
    }

    void Element::adoptAsLastChild(NodePtr inNode)
    {
      if (!inNode)
        return;

      if (!inNode->isAttribute())
      {
        Node::adoptAsLastChild(inNode);
        return;
      }

      AttributePtr attribute = inNode->toAttribute();

      // first the node has to be orphaned
      inNode->orphan();

      // ensure this attribute does not already exist
      deleteAttribute(attribute->getName());

      // adopt this as the parent
      inNode->mParent = toNode();

      // point existing last child's next sibling to new node
      if (mLastAttribute)
        mLastAttribute->mNextSibling = inNode;

      // point new node's next sibling to current first child
      inNode->mPreviousSibling = mLastAttribute;

      // first child is now new node
      mLastAttribute = attribute;

      // if there wasn't a last child then it is now pointing to the new node
      if (!mFirstAttribute)
        mFirstAttribute = attribute;
    }

    NodePtr Element::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    void Element::clear()
    {
      mName.clear();
      NodePtr child = mFirstAttribute;
      while (child)
      {
        NodePtr tempNext = child->getNextSibling();
        child->orphan();
        child->clear();
        child = tempNext;
      }
      Node::clear();
    }

    String Element::getValue() const
    {
      return mName;
    }

  } // namespace XML

} // namespace zsLib

#pragma warning(pop)
