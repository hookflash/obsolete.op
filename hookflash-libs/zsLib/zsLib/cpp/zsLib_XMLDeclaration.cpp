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

#pragma warning(push)
#pragma warning(disable: 4290)

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
      #pragma mark XML::internal::Declaration
      #pragma mark

      //-----------------------------------------------------------------------
      void Declaration::parse(XML::ParserPos &ioPos)
      {
        Parser::AutoStack stack(ioPos);

        // this must be a <?xml
        ZS_THROW_BAD_STATE_IF(!ioPos.isString("<?xml", false))

        ioPos += (ULONG)strlen("<?xml");

        while ((*ioPos) &&
               (!ioPos.isString("?>")))
        {
          if (Parser::skipWhiteSpace(ioPos))
            continue;

          if (('>' == (*ioPos)) ||
              (ioPos.isString("/>")))
          {
            // this not a proper end to the tag but it must be an end tag
            (ioPos.getParser())->addWarning(ParserWarningType_NotProperEndDeclaration, ioPos);
            if (ioPos.isString("/>"))
              ioPos += (ULONG)strlen("/>");
            else
              ++ioPos;

            // stop processing this attribute
            return;
          }

          if (!Parser::isLegalName(*ioPos, true))
          {
            // this isn't a legal name for an attribute, skip it
            (ioPos.getParser())->addWarning(ParserWarningType_IllegalAttributeName, ioPos);

            while ((*ioPos) &&
                   (!ioPos.isString("?>")) &&
                   ('>' != (*ioPos)) &&
                   (!Parser::isWhiteSpace(ioPos)))
            {
              ++ioPos;
            }
            continue;
          }

          AttributePtr attribute = XML::Attribute::create();
          if (!attribute->parse(ioPos))
            continue;

          // attribute was found, add it to the attribute list
          bool duplicateAttribute = (mThis.lock())->setAttribute(attribute);
          if (duplicateAttribute)
          {
            (ioPos.getParser())->addWarning(ParserWarningType_DuplicateAttribute);
          }
        }

        if (!ioPos.isString("?>"))
        {
          (ioPos.getParser())->addWarning(ParserWarningType_NoEndDeclarationFound);
          return;
        }

        ioPos += (ULONG)strlen("?>");
      }

      //-----------------------------------------------------------------------
      ULONG Declaration::getOutputSizeXML(const GeneratorPtr &inGenerator) const
      {
        ULONG result = 0;
        result += (ULONG)strlen("<?xml");

        if (mFirstAttribute)
        {
          NodePtr child = mFirstAttribute;
          while (child)
          {
            AttributePtr attribute = child->toAttribute();
            result += (ULONG)strlen(" ");
            result += Generator::getOutputSize(inGenerator, attribute);
            child = child->getNextSibling();
          }
        }

        result += (ULONG)strlen(" ?>");
        return result;
      }

      //-----------------------------------------------------------------------
      void Declaration::writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const
      {
        Generator::writeBuffer(ioPos, "<?xml");

        if (mFirstAttribute)
        {
          NodePtr child = mFirstAttribute;
          while (child)
          {
            AttributePtr attribute = child->toAttribute();
            Generator::writeBuffer(ioPos, " ");
            Generator::writeBuffer(inGenerator, attribute, ioPos);
            child = child->getNextSibling();
          }
        }
        Generator::writeBuffer(ioPos, " ?>");
      }

      //-----------------------------------------------------------------------
      ULONG Declaration::getOutputSizeJSON(const GeneratorPtr &inGenerator) const
      {
        ULONG result = 0;
        return result;
      }

      //-----------------------------------------------------------------------
      void Declaration::writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const
      {
      }

      //-----------------------------------------------------------------------
      NodePtr Declaration::cloneAssignParent(NodePtr inParent) const
      {
        XML::DeclarationPtr newObject(XML::Declaration::create());
        Parser::safeAdoptAsLastChild(inParent, newObject);

        NodePtr child = mFirstAttribute;
        while (child)
        {
          NodePtr newChild = Parser::cloneAssignParent(newObject, child);
          child = child->getNextSibling();
        }
        (mThis.lock())->cloneChildren(mThis.lock(), newObject);
        return newObject;
      }

    } // namespace internal


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::Declaration
    #pragma mark

    //-------------------------------------------------------------------------
    Declaration::Declaration() :
      internal::Declaration()
    {
    }

    //-------------------------------------------------------------------------
    DeclarationPtr Declaration::create()
    {
      DeclarationPtr newObject(new Declaration());
      newObject->mThis = newObject;
      return newObject;
    }

    //-------------------------------------------------------------------------
    AttributePtr Declaration::findAttribute(String inName) const
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

    //-------------------------------------------------------------------------
    String Declaration::getAttributeValue(String inName) const
    {
      AttributePtr attribute = findAttribute(inName);
      if (attribute)
        return attribute->getValue();

      return String();
    }

    //-------------------------------------------------------------------------
    AttributePtr Declaration::findAttributeChecked(String inName) const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = findAttribute(inName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    //-------------------------------------------------------------------------
    String Declaration::getAttributeValueChecked(String inName) const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = findAttribute(inName);
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result->getValue();
    }

    //-------------------------------------------------------------------------
    bool Declaration::setAttribute(String inName, String inValue)
    {
      AttributePtr attribute = Attribute::create();
      attribute->setName(inName);
      attribute->setValue(inValue);
      return setAttribute(attribute);
    }

    //-------------------------------------------------------------------------
    bool Declaration::setAttribute(AttributePtr inAttribute)
    {
      ZS_THROW_INVALID_USAGE_IF(!inAttribute)

      bool replaced = deleteAttribute(inAttribute->getName());
      mThis.lock()->adoptAsLastChild(inAttribute);
      return replaced;
    }

    //-------------------------------------------------------------------------
    bool Declaration::deleteAttribute(String inName)
    {
      AttributePtr attribute = findAttribute(inName);
      if (!attribute)
        return false;

      attribute->orphan();
      return true;
    }

    //-------------------------------------------------------------------------
    AttributePtr Declaration::getFirstAttribute() const
    {
      return mFirstAttribute;
    }

    //-------------------------------------------------------------------------
    AttributePtr Declaration::getLastAttribute() const
    {
      return mLastAttribute;
    }

    //-------------------------------------------------------------------------
    AttributePtr Declaration::getFirstAttributeChecked() const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = getFirstAttribute();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    //-------------------------------------------------------------------------
    AttributePtr Declaration::getLastAttributeChecked() const throw(Exceptions::CheckFailed)
    {
      AttributePtr result = getLastAttribute();
      ZS_THROW_CUSTOM_IF(Exceptions::CheckFailed, !result)
      return result;
    }

    //-------------------------------------------------------------------------
    void Declaration::adoptAsFirstChild(NodePtr inNode)
    {
      if (!inNode)
        return;

      if (!inNode->isAttribute())
      {
        ZS_THROW_INVALID_USAGE("declarations can only have attributes as children")
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

    //-------------------------------------------------------------------------
    void Declaration::adoptAsLastChild(NodePtr inNode)
    {
      if (!inNode)
        return;

      if (!inNode->isAttribute())
      {
        ZS_THROW_INVALID_USAGE("declarations can only have attributes as children")
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

    //-------------------------------------------------------------------------
    NodePtr Declaration::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    //-------------------------------------------------------------------------
    void Declaration::clear()
    {
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

  } // namespace XML

} // namespace zsLib

#pragma warning(pop)
