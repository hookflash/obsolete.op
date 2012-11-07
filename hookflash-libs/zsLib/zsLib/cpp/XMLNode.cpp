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

#define ZS_INTERNAL_XML_FILTER_BIT_DOCUMENT     0x0001
#define ZS_INTERNAL_XML_FILTER_BIT_ELEMENT      0x0002
#define ZS_INTERNAL_XML_FILTER_BIT_ATTRIBUTE    0x0004
#define ZS_INTERNAL_XML_FILTER_BIT_TEXT         0x0008
#define ZS_INTERNAL_XML_FILTER_BIT_COMMENT      0x0010
#define ZS_INTERNAL_XML_FILTER_BIT_DECLARATION  0x0020
#define ZS_INTERNAL_XML_FILTER_BIT_UNKNOWN      0x0040

#define ZS_INTERNAL_XML_FILTER_BIT_MASK_ALL     0x007F

#pragma warning(push)
#pragma warning(disable: 4290)

namespace zsLib
{

  namespace XML
  {

    namespace internal
    {

      Node::Node() :
      mUserData(NULL)
      {
      }

      void Node::cloneChildren(const NodePtr &inSelf, NodePtr inNewObject) const
      {
        ZS_THROW_INVALID_USAGE_IF(inSelf.get() != this)

        NodePtr child = inSelf->getFirstChild();
        while (child)
        {
          NodePtr newChild = Parser::cloneAssignParent(inNewObject, child);
          child = child->getNextSibling();
        }
      }

    } // namespace internal

    bool WalkSink::onDocumentEnter(DocumentPtr)        {return false;}
    bool WalkSink::onDocumentExit(DocumentPtr)         {return false;}
    bool WalkSink::onElementEnter(ElementPtr)          {return false;}
    bool WalkSink::onElementExit(ElementPtr)           {return false;}
    bool WalkSink::onAttribute(AttributePtr)           {return false;}
    bool WalkSink::onText(TextPtr)                     {return false;}
    bool WalkSink::onComment(CommentPtr)               {return false;}
    bool WalkSink::onDeclarationEnter(DeclarationPtr)  {return false;}
    bool WalkSink::onDeclarationExit(DeclarationPtr)   {return false;}
    bool WalkSink::onUnknown(UnknownPtr)               {return false;}

    Node::Node() :
    internal::Node()
    {
    }

    Node::~Node()
    {
      clear();
    }

    DocumentPtr Node::getDocument() const
    {
      NodePtr root = getRoot();
      if (root->isDocument())
        return root->toDocument();

      return DocumentPtr();
    }

    static ULONG getMaskBit(Node::NodeType::Type inNodeType)
    {
      switch (inNodeType)
      {
        case Node::NodeType::Document:      return ZS_INTERNAL_XML_FILTER_BIT_DOCUMENT;
        case Node::NodeType::Element:       return ZS_INTERNAL_XML_FILTER_BIT_ELEMENT;
        case Node::NodeType::Attribute:     return ZS_INTERNAL_XML_FILTER_BIT_ATTRIBUTE;
        case Node::NodeType::Text:          return ZS_INTERNAL_XML_FILTER_BIT_TEXT;
        case Node::NodeType::Comment:       return ZS_INTERNAL_XML_FILTER_BIT_COMMENT;
        case Node::NodeType::Declaration:   return ZS_INTERNAL_XML_FILTER_BIT_DECLARATION;
        case Node::NodeType::Unknown:       return ZS_INTERNAL_XML_FILTER_BIT_UNKNOWN;
        default:                            ZS_THROW_INVALID_USAGE("unknown XML type") break;
      }
      return 0;
    }

    bool Node::walk(WalkSink &inWalker, const FilterList *inFilterList) const
    {
      // walks the tree in a non-recursive way to protect the stack from overflowing

      ULONG mask = ZS_INTERNAL_XML_FILTER_BIT_MASK_ALL;
      if (inFilterList)
      {
        mask = 0;
        for (FilterList::const_iterator iter = inFilterList->begin(); iter != inFilterList->end(); ++iter)
        {
          mask = mask | getMaskBit(*iter);
        }
      }

      NodePtr startChild = toNode();
      NodePtr child = startChild;
      bool allowChildren = true;
      while (child)
      {
        NodePtr nextSibling = child->getNextSibling();  // just in case the current element was orphaned
        NodePtr nextParent = child->getParent();

        bool doContinue = false;
        while (allowChildren) // using as a scope rather than as a loop
        {
          if (0 != (mask & getMaskBit(child->getNodeType())))
          {
            bool result = false;
            switch (child->getNodeType())
            {
              case NodeType::Document:      result = inWalker.onDocumentEnter(child->toDocument()); break;
              case NodeType::Element:       result = inWalker.onElementEnter(child->toElement()); break;
              case NodeType::Attribute:     result = inWalker.onAttribute(child->toAttribute()); break;
              case NodeType::Text:          result = inWalker.onText(child->toText()); break;
              case NodeType::Comment:       result = inWalker.onComment(child->toComment()); break;
              case NodeType::Declaration:   result = inWalker.onDeclarationEnter(child->toDeclaration()); break;
              case NodeType::Unknown:       result = inWalker.onUnknown(child->toUnknown()); break;
            }
            if (result)
              return true;

            if ((child->getNextSibling() != nextSibling) ||
                (child->getParent() != nextParent)) {
              // the node was orphaned, do not process its children anymore
              break;
            }
          }

          AttributePtr attribute;

          if (child->isElement())
          {
            ElementPtr element = child->toElement();
            attribute = element->getFirstAttribute();
          }
          else if (child->isDeclaration())
          {
            DeclarationPtr declaration = child->toDeclaration();
            attribute = declaration->getFirstAttribute();
          }

          // walk the attributes
          if (attribute)
          {
            if (0 != (mask & getMaskBit(attribute->getNodeType())))
            {
              while (attribute)
              {
                NodePtr next = attribute->getNextSibling(); // need to do this in advanced in case the attribute is orphaned

                if (inWalker.onAttribute(attribute))
                  return true;

                if (!next)
                  break;

                attribute = next->toAttribute();
              }
            }
          }

          if (child->getFirstChild())
          {
            child = child->getFirstChild();
            doContinue = true;
            break;
          }
          break;  // not really intending to loop
        }

        if (doContinue)
          continue;

        if ((child->isDeclaration()) &&
            (0 != (mask & ZS_INTERNAL_XML_FILTER_BIT_DECLARATION)))
        {
          // reached the exit point for the declaration
          if (inWalker.onDeclarationExit(child->toDeclaration()))
            return true;
        }

        // going to be exiting this node node by way of the sibling or the parent (but only do exit for nodes that have exit)
        if ((child->isElement()) &&
            (0 != (mask & ZS_INTERNAL_XML_FILTER_BIT_ELEMENT)))
        {
          if (inWalker.onElementExit(child->toElement()))
            return true;
        }

        if ((child->isDocument()) &&
            (0 != (mask & ZS_INTERNAL_XML_FILTER_BIT_DOCUMENT)))
        {
          if (inWalker.onDocumentExit(child->toDocument()))
            return true;
        }

        if (nextSibling)
        {
          // can't go to the next sibling if on the root
          if (child == startChild)
            break;

          allowChildren = true;
          child = nextSibling;
          continue;
        }

        // cannot walk above the start child
        if (child == startChild)
          break;

        child = nextParent;
        allowChildren = false;
      }

      return false;
    }

    bool Node::walk(WalkSink &inWalker, NodeType::Type inType) const
    {
      FilterList filter;
      filter.push_back(inType);
      return walk(inWalker, &filter);
    }

    NodePtr Node::getParent() const
    {
      return mParent.lock();
    }

    NodePtr Node::getRoot() const
    {
      NodePtr found = toNode();
      NodePtr parent = found->getParent();
      while (parent)
      {
        found = parent;
        parent = parent->getParent();
      }
      return found;
    }

    NodePtr Node::getFirstChild() const
    {
      return mFirstChild;
    }

    NodePtr Node::getLastChild() const
    {
      return mLastChild;
    }

    NodePtr Node::getFirstSibling() const
    {
      NodePtr parent = getParent();
      if (parent)
        return parent->getFirstChild();

      return toNode();
    }

    NodePtr Node::getLastSibling() const
    {
      NodePtr parent = getParent();
      if (parent)
        return parent->getLastChild();

      return toNode();
    }

    NodePtr Node::getPreviousSibling() const
    {
      return mPreviousSibling;
    }

    NodePtr Node::getNextSibling() const
    {
      return mNextSibling;
    }

    ElementPtr Node::getParentElement() const
    {
      NodePtr found = getParent();
      while (found)
      {
        if (found->isElement())
          return found->toElement();

        found = found->getParent();
      }
      return ElementPtr();
    }

    ElementPtr Node::getRootElement() const
    {
      ElementPtr found;
      if (isElement()) {
        found = toElement();
      } else {
        found = getParentElement();
      }

      if (!found)
        return ElementPtr();

      ElementPtr parent = found->getParentElement();
      while (parent)
      {
        found = parent;
        parent = parent->getParentElement();
      }
      return found;
    }

    ElementPtr Node::getFirstChildElement() const
    {
      NodePtr found = getFirstChild();
      while (found)
      {
        if (found->isElement())
          return found->toElement();

        found = found->getNextSibling();
      }
      return ElementPtr();
    }

    ElementPtr Node::getLastChildElement() const
    {
      NodePtr found = getLastChild();
      while (found)
      {
        if (found->isElement())
          return found->toElement();

        found = found->getPreviousSibling();
      }
      return ElementPtr();
    }

    ElementPtr Node::getFirstSiblingElement() const
    {
      NodePtr found = getParent();
      if (found)
        return found->getFirstChildElement();

      return ElementPtr();
    }

    ElementPtr Node::getLastSiblingElement() const
    {
      NodePtr found = getParent();
      if (found)
        return found->getLastChildElement();

      return ElementPtr();
    }

    ElementPtr Node::getPreviousSiblingElement() const
    {
      NodePtr found = getPreviousSibling();
      while (found)
      {
        if (found->isElement())
          return found->toElement();

        found = found->getPreviousSibling();
      }
      return ElementPtr();
    }

    ElementPtr Node::getNextSiblingElement() const
    {
      NodePtr found = getNextSibling();
      while (found)
      {
        if (found->isElement())
          return found->toElement();

        found = found->getNextSibling();
      }
      return ElementPtr();
    }

    ElementPtr Node::findPreviousSiblingElement(String elementName) const
    {
      bool caseSensative = true;
      DocumentPtr document = getDocument();
      if (document)
        caseSensative = document->isElementNameIsCaseSensative();
      
      ElementPtr element = getPreviousSiblingElement();
      while (element)
      {
        if (caseSensative)
        {
          if (elementName == element->getValue())
            return element;
        }
        else
        {
          if (0 == elementName.compareNoCase(element->getValue()))
            return element;
        }
        element = element->getPreviousSiblingElement();
      }
      return ElementPtr();
    }

    ElementPtr Node::findNextSiblingElement(String elementName) const
    {
      bool caseSensative = true;
      DocumentPtr document = getDocument();
      if (document)
        caseSensative = document->isElementNameIsCaseSensative();
      
      ElementPtr element = getNextSiblingElement();
      while (element)
      {
        if (caseSensative)
        {
          if (elementName == element->getValue())
            return element;
        }
        else
        {
          if (0 == elementName.compareNoCase(element->getValue()))
            return element;
        }
        element = element->getNextSiblingElement();
      }
      return ElementPtr();
    }

    ElementPtr Node::findFirstChildElement(String elementName) const
    {
      bool caseSensative = true;
      DocumentPtr document = getDocument();
      if (document)
        caseSensative = document->isElementNameIsCaseSensative();
      
      ElementPtr element = getFirstChildElement();
      while (element)
      {
        if (caseSensative)
        {
          if (elementName == element->getValue())
            return element;
        }
        else
        {
          if (0 == elementName.compareNoCase(element->getValue()))
            return element;
        }
        element = element->getNextSiblingElement();
      }
      return ElementPtr();
    }
    
    ElementPtr Node::findLastChildElement(String elementName) const
    {
      bool caseSensative = true;
      DocumentPtr document = getDocument();
      if (document)
        caseSensative = document->isElementNameIsCaseSensative();
      
      ElementPtr element = getLastChildElement();
      while (element)
      {
        if (caseSensative)
        {
          if (elementName == element->getValue())
            return element;
        }
        else
        {
          if (0 == elementName.compareNoCase(element->getValue()))
            return element;
        }
        element = element->getPreviousSiblingElement();
      }
      return ElementPtr();
    }

    NodePtr Node::getParentChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getParent();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getRootChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getRoot();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getFirstChildChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getFirstChild();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getLastChildChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getLastChild();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getFirstSiblingChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getFirstSibling();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getLastSiblingChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getLastSibling();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getPreviousSiblingChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getPreviousSibling();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    NodePtr Node::getNextSiblingChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = getNextSibling();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getParentElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getParentElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getRootElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getRootElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getFirstChildElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getFirstChildElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getLastChildElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getLastChildElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getFirstSiblingElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getFirstSiblingElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getLastSiblingElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getLastSiblingElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getPreviousSiblingElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getPreviousSiblingElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::getNextSiblingElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = getNextSiblingElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::findPreviousSiblingElementChecked(String elementName) const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = findPreviousSiblingElement(elementName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }
    
    ElementPtr Node::findNextSiblingElementChecked(String elementName) const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = findNextSiblingElement(elementName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }
    
    ElementPtr Node::findFirstChildElementChecked(String elementName) const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = findFirstChildElement(elementName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }
    
    ElementPtr Node::findLastChildElementChecked(String elementName) const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = findLastChildElement(elementName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    void Node::orphan()
    {
      NodePtr parent = getParent();

      if (parent)
      {
        if (parent->mFirstChild.get() == this)
          parent->mFirstChild = mNextSibling;

        if (parent->mLastChild.get() == this)
          parent->mLastChild = mPreviousSibling;
      }

      if (mNextSibling)
        mNextSibling->mPreviousSibling = mPreviousSibling;

      if (mPreviousSibling)
        mPreviousSibling->mNextSibling = mNextSibling;

      // leave this node as an orphan
      mParent = NodePtr();
      mNextSibling = NodePtr();
      mPreviousSibling = NodePtr();
    }

    void Node::adoptAsFirstChild(NodePtr inNode)
    {
      if (!inNode)
        return;

      // first the node has to be orphaned
      inNode->orphan();

      // adopt this as the parent
      inNode->mParent = toNode();

      // point existing first child's previous sibling to new node
      if (mFirstChild)
        mFirstChild->mPreviousSibling = inNode;

      // point new node's next sibling to current first child
      inNode->mNextSibling = mFirstChild;

      // first child is now new node
      mFirstChild = inNode;

      // if there wasn't a last child then it is now pointing to the new node
      if (!mLastChild)
        mLastChild = inNode;
    }

    void Node::adoptAsLastChild(NodePtr inNode)
    {
      if (!inNode)
        return;

      // first the node has to be orphaned
      inNode->orphan();

      // adopt this as the parent
      inNode->mParent = toNode();

      // point existing last child's next sibling to new node
      if (mLastChild)
        mLastChild->mNextSibling = inNode;

      // point new node's previous sibling to current last child
      inNode->mPreviousSibling = mLastChild;

      // first child is now new node
      mLastChild = inNode;

      // if there wasn't a last child then it is now pointing to the new node
      if (!mFirstChild)
        mFirstChild = inNode;
    }

    void Node::adoptAsPreviousSibling(NodePtr inNode)
    {
      if (!inNode)
        return;

      NodePtr parent = mParent.lock();

      ZS_THROW_INVALID_USAGE_IF(!parent)  // you cannot add as a sibling if there is no parent

      // orphan the node first
      inNode->orphan();

      // nodes both share the same parent
      inNode->mParent = mParent;

      inNode->mPreviousSibling = mPreviousSibling;
      inNode->mNextSibling = toNode();

      if (mPreviousSibling)
        mPreviousSibling->mNextSibling = inNode;

      mPreviousSibling = inNode;
      if (parent->mFirstChild.get() == this)
        parent->mFirstChild = inNode;
    }

    void Node::adoptAsNextSibling(NodePtr inNode)
    {
      if (!inNode)
        return;

      NodePtr parent = mParent.lock();

      ZS_THROW_INVALID_USAGE_IF(!parent)  // you cannot add as a sibling if there is no parent

      // orphan the node first
      inNode->orphan();

      // nodes both share the same parent
      inNode->mParent = mParent;

      inNode->mPreviousSibling = toNode();
      inNode->mNextSibling = mNextSibling;

      if (mNextSibling)
        mNextSibling->mPreviousSibling = inNode;

      mNextSibling = inNode;
      if (parent->mLastChild.get() == this)
        parent->mLastChild = inNode;
    }

    bool Node::hasChildren()
    {
      return getFirstChild();
    }

    void Node::removeChildren()
    {
      ULONG depth = 1;

      NodePtr child = mFirstChild;
      NodePtr clean;

      while (true)
      {
        if (clean)
        {
          clean->orphan();
          clean->clear();
        }

        if (!child)
          break;

        if (child->mFirstChild)
        {
          ++depth;
          child = child->mFirstChild;
          continue;
        }

        if (child->mNextSibling)
        {
          clean = child;
          child = child->mNextSibling;
          continue;
        }

        if (1 == depth)
        {
          child->orphan();
          child->clear();
          break;
        }

        // get the parent
        clean = child;
        child = child->mParent.lock();
        --depth;
      }
    }

    void Node::clear()
    {
      removeChildren();
      mUserData = NULL;
    }

    void *Node::getUserData() const
    {
      return mUserData;
    }

    void Node::setUserData(void *inData)
    {
      mUserData = inData;
    }

    NodePtr Node::toNodeChecked() const throw(Exceptions::CheckFailed)
    {
      NodePtr result = toNode();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    DocumentPtr Node::toDocumentChecked() const throw(Exceptions::CheckFailed)
    {
      DocumentPtr result = toDocument();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    ElementPtr Node::toElementChecked() const throw(Exceptions::CheckFailed)
    {
      ElementPtr result = toElement();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    AttributePtr Node::toAttributeChecked() const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = toAttribute();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    TextPtr Node::toTextChecked() const throw(Exceptions::CheckFailed)
    {
      TextPtr result = toText();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    CommentPtr Node::toCommentChecked() const throw(Exceptions::CheckFailed)
    {
      CommentPtr result = toComment();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    DeclarationPtr Node::toDeclarationChecked() const throw(Exceptions::CheckFailed)
    {
      DeclarationPtr result = toDeclaration();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    UnknownPtr Node::toUnknownChecked() const throw(Exceptions::CheckFailed)
    {
      UnknownPtr result = toUnknown();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

  } // namespace XML

} // namespace zsLib

#pragma warning(pop)
