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

      Attribute::Attribute() :
        mValuelessAttribute(false)
      {
      }

      bool Attribute::parse(XML::ParserPos &ioPos)
      {
        Parser::AutoStack stack(ioPos);

        mName = Parser::parseLegalName(ioPos);
        ZS_THROW_BAD_STATE_IF(mName.isEmpty())

        mValuelessAttribute = false;

        bool foundWhiteSpace = Parser::skipWhiteSpace(ioPos);
        if ('=' != *ioPos)
        {
          // valueless attribute?
          if (('/' == *ioPos) ||
              ('>' == *ioPos) ||
              (foundWhiteSpace))
          {
            // this is an attribute without a value, we will parse but it is somewhat illegal
            (ioPos.getDocument())->addWarning(ParserWarningType_AttributeWithoutValue);

            // remember that it was one of these attributes
            mValuelessAttribute = true;
            mValue.clear();
            return true;
          }

          // didn't find whitespace, didn't find end of element, didn't find equals therefor illegal attribute name found
          (ioPos.getDocument())->addWarning(ParserWarningType_IllegalAttributeName, ioPos);

          // skip over this illegal attribute name
          while ((*ioPos) &&
                 (!ioPos.isString("/>")) &&
                 (!ioPos.isString("?>")) &&
                 ('>' != *ioPos) &&
                 (!Parser::isWhiteSpace(ioPos)))
          {
            ++ioPos;
          }
          return false;
        }

        // attribute has a value
        mValue.clear();

        // skip over equals
        ++ioPos;

        // skip any whitespace between equals and value
        Parser::skipWhiteSpace(ioPos);

        XML::ParserPos startOfValue(ioPos);

        char quote = 0;
        if (('\'' == *ioPos) ||
            ('\"' == *ioPos))
          quote = *ioPos;

        if (0 != quote)
        {
          // skip over the quote
          ++ioPos;

          // found a quote, parse until end of quote hit
          while ((*ioPos) &&
                 (quote != *ioPos))
          {
            mValue += *ioPos;
            ++ioPos;
          }

          if (ioPos.isEOF())
          {
            (ioPos.getDocument())->addWarning(ParserWarningType_AttributeValueMissingEndQuote, startOfValue);
            return false;
          }

          // skip the end quote
          ++ioPos;
          return true;
        }

        // continue until whitespace is hit or end of element
        while ((*ioPos) &&
               (!Parser::isWhiteSpace(ioPos)) &&
               (!ioPos.isString("/>")) &&
               (!ioPos.isString("?>")) &&
               ('>' != *ioPos))
        {
          mValue += *ioPos;
          ++ioPos;
        }

        if (mValue.isEmpty())
        {
          // no value was found, something went horribly wrong
          (ioPos.getDocument())->addWarning(ParserWarningType_AttributeValueNotFound, startOfValue);
          return false;
        }

        return true;
      }

      static String normalizeAttributeValue(const String &input)
      {
        String output = XML::Parser::convertFromEntities(input);
        return XML::Parser::makeAttributeEntitySafe(output, '\"');
      }

      ULONG Attribute::getOutputSize(const DocumentPtr &inDocument) const
      {
        ULONG result = 0;
        ZS_THROW_INVALID_USAGE_IF(mName.isEmpty())
        result += mName.getLength();
        if (!mValuelessAttribute)
        {
          String value = mValue;
          if (0 != (XML::Document::WriteFlag_NormalizeCDATA & inDocument->getWriteFlags()))
            value = normalizeAttributeValue(mValue);
          result += (ULONG)strlen("=\"");
          result += value.getLength();
          result += (ULONG)strlen("\"");
        }
        return result;
      }

      void Attribute::writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const
      {
        ZS_THROW_INVALID_USAGE_IF(mName.isEmpty())
        Parser::writeBuffer(ioPos, mName);
        if (!mValuelessAttribute)
        {
          Parser::writeBuffer(ioPos, "=");
          char quote[2];
          quote[0] = (String::npos == mValue.find('\"') ? '\"' : '\'');
          quote[1] = 0;
          String value = mValue;
          if (0 != (XML::Document::WriteFlag_NormalizeCDATA & inDocument->getWriteFlags())) {
            quote[0] = '\"';
            value = normalizeAttributeValue(mValue);
          }
          Parser::writeBuffer(ioPos, &(quote[0]));
          Parser::writeBuffer(ioPos, value);
          Parser::writeBuffer(ioPos, &(quote[0]));
        }
      }

      NodePtr Attribute::cloneAssignParent(NodePtr inParent) const
      {
        AttributePtr newObject(XML::Attribute::create());

        newObject->mName = mName;
        newObject->mValue = mValue;
        newObject->mValuelessAttribute = mValuelessAttribute;

        Parser::safeAdoptAsLastChild(inParent, newObject);

        (mThis.lock())->cloneChildren(mThis.lock(), newObject);    // should do nothing since there are no children
        return newObject;
      }

    } // namespace internal

    Attribute::Attribute() :
    internal::Attribute()
    {
    }

    AttributePtr Attribute::create()
    {
      AttributePtr object(new Attribute);
      object->mThis = object;
      return object;
    }

    String Attribute::getName() const
    {
      return mName;
    }

    void Attribute::setName(String inName)
    {
      // remember the attribute's parent
      NodePtr parent = getParent();

      // orphan this attribute temporarily
      orphan();

      mName = inName;

      // if there was a parent
      if (parent)
      {
        if (parent->isElement())
        {
          // remove any attribute with this name
          parent->toElement()->deleteAttribute(inName);

          // add this new attribute
          parent->toElement()->setAttribute(mThis.lock());
        }
        else if (parent->isDeclaration())
        {
          // remove any attribute with this name
          parent->toDeclaration()->deleteAttribute(inName);

          // add this new attribute
          parent->toDeclaration()->setAttribute(mThis.lock());
        }
        else
        {
          ZS_THROW_INVALID_USAGE("attributes are only allowed on elements and declarations")
        }
      }
    }

    void Attribute::setValue(String inValue)
    {
      mValue = inValue;
      mValuelessAttribute = false;

      if (String::npos != mValue.find('\''))
      {
        if (String::npos != mValue.find('\"'))
        {
          ZS_THROW_INVALID_USAGE("value cannot contain both set of quotes, one must be escaped using entities")
        }
      }
    }

    NodePtr Attribute::getFirstSibling() const
    {
      NodePtr parent = getParent();
      if (parent)
      {
        if (parent->isElement())
          return parent->toElement()->mFirstAttribute;

        if (parent->isDeclaration())
          return parent->toDeclaration()->mFirstAttribute;

        ZS_THROW_INVALID_USAGE("attributes only belong on elements and declarations")
      }
      return NodePtr();
    }

    NodePtr Attribute::getLastSibling() const
    {
      NodePtr parent = getParent();
      if (parent)
      {
        if (parent->isElement())
          return parent->toElement()->mLastAttribute;

        if (parent->isDeclaration())
          return parent->toDeclaration()->mLastAttribute;

        ZS_THROW_INVALID_USAGE("attributes only belong on elements and declarations")
      }
      return NodePtr();
    }

    void Attribute::orphan()
    {
      NodePtr parent = getParent();

      // if there is no parent, then already an orphan
      if (parent)
      {
        if (parent->isElement())
        {
          ElementPtr elementParent = parent->toElement();

          if (elementParent->mFirstAttribute.get() == this)
            elementParent->mFirstAttribute = (mNextSibling ? mNextSibling->toAttribute() : AttributePtr());

          if (elementParent->mLastAttribute.get() == this)
            elementParent->mLastAttribute = (mPreviousSibling ? mPreviousSibling->toAttribute() : AttributePtr());
        }
        else if (parent->isDeclaration())
        {
          DeclarationPtr declarationParent = parent->toDeclaration();

          if (declarationParent->mFirstAttribute.get() == this)
            declarationParent->mFirstAttribute = (mNextSibling ? mNextSibling->toAttribute() : AttributePtr());

          if (declarationParent->mLastAttribute.get() == this)
            declarationParent->mLastAttribute = (mPreviousSibling ? mPreviousSibling->toAttribute() : AttributePtr());
        }
        else
        {
          ZS_THROW_INVALID_USAGE("attributes only belong on elements and declarations")
        }
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

    void Attribute::adoptAsFirstChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("attributes cannot have children")
    }

    void Attribute::adoptAsLastChild(NodePtr inNode)
    {
      ZS_THROW_INVALID_USAGE("attributes cannot have children")
    }

    void Attribute::adoptAsPreviousSibling(NodePtr inNode)
    {
      if (!inNode)
        return;

      NodePtr parent = mParent.lock();

      ZS_THROW_INVALID_USAGE_IF(!parent)  // you cannot add as a sibling if there is no parent
      ZS_THROW_INVALID_USAGE_IF(!inNode->isAttribute())  // you cannot add another but attributes as sibling to attributes

      // orphan the node first
      inNode->orphan();

      // nodes both share the same parent
      inNode->mParent = mParent;

      inNode->mPreviousSibling = mPreviousSibling;
      inNode->mNextSibling = toNode();

      if (mPreviousSibling)
        mPreviousSibling->mNextSibling = inNode;

      mPreviousSibling = inNode;
      if (parent->isElement())
      {
        if (parent->toElement()->mFirstAttribute.get() == this)
          parent->toElement()->mFirstAttribute = inNode->toAttribute();
      }
      else if (parent->isDeclaration())
      {
        if (parent->toDeclaration()->mFirstAttribute.get() == this)
          parent->toDeclaration()->mFirstAttribute = inNode->toAttribute();
      }
      else
      {
        ZS_THROW_BAD_STATE("attributes can only exist on elements and declarations")
      }
    }

    void Attribute::adoptAsNextSibling(NodePtr inNode)
    {
      if (!inNode)
        return;

      NodePtr parent = mParent.lock();

      ZS_THROW_INVALID_USAGE_IF(!parent)  // you cannot add as a sibling if there is no parent
      ZS_THROW_INVALID_USAGE_IF(!inNode->isAttribute())  // you cannot add another but attributes as sibling to attributes

      // orphan the node first
      inNode->orphan();

      // nodes both share the same parent
      inNode->mParent = mParent;

      inNode->mPreviousSibling = toNode();
      inNode->mNextSibling = mNextSibling;

      if (mNextSibling)
        mNextSibling->mPreviousSibling = inNode;

      mNextSibling = inNode;
      if (parent->isElement())
      {
        if (parent->toElement()->mLastAttribute.get() == this)
          parent->toElement()->mLastAttribute = inNode->toAttribute();
      }
      else if (parent->isDeclaration())
      {
        if (parent->toDeclaration()->mLastAttribute.get() == this)
          parent->toDeclaration()->mLastAttribute = inNode->toAttribute();
      }
      else
      {
        ZS_THROW_BAD_STATE("attributes can only exist on elements and declarations")
      }
    }

    NodePtr Attribute::clone() const
    {
      return cloneAssignParent(NodePtr());
    }

    void Attribute::clear()
    {
      mName.clear();
      mValue.clear();
      mValuelessAttribute = false;
    }

    String Attribute::getValue() const
    {
      return mValue;
    }

  } // namespace XML

} // namespace zsLib
