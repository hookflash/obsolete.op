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
      #pragma mark XML::internal::Generator
      #pragma mark

      //-----------------------------------------------------------------------
      Generator::Generator() :
        mWriteFlags(0),
        mJSONForcedText(ZS_JSON_DEFAULT_FORCED_TEXT),
        mJSONAttributePrefix(ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX)
      {
      }

      //-----------------------------------------------------------------------
      ULONG Generator::getOutputSize(const GeneratorPtr &inGenerator, NodePtr inNode)
      {
        if (!inNode)
          return 0;

        switch (inGenerator->mGeneratorMode)
        {
          case GeneratorMode_XML:
          {
            switch (inNode->getNodeType())
            {
              case XML::Node::NodeType::Document:      return inNode->toDocument()->getOutputSizeXML(inGenerator);
              case XML::Node::NodeType::Element:       return inNode->toElement()->getOutputSizeXML(inGenerator);
              case XML::Node::NodeType::Attribute:     return inNode->toAttribute()->getOutputSizeXML(inGenerator);
              case XML::Node::NodeType::Text:          return inNode->toText()->getOutputSizeXML(inGenerator);
              case XML::Node::NodeType::Comment:       return inNode->toComment()->getOutputSizeXML(inGenerator);
              case XML::Node::NodeType::Declaration:   return inNode->toDeclaration()->getOutputSizeXML(inGenerator);
              case XML::Node::NodeType::Unknown:       return inNode->toUnknown()->getOutputSizeXML(inGenerator);
              default:
              {
                ZS_THROW_BAD_STATE("missing node type in getOutputSize table")
              }
            }
            break;
          }
          case GeneratorMode_JSON:
          {
            switch (inNode->getNodeType())
            {
              case XML::Node::NodeType::Document:      return inNode->toDocument()->getOutputSizeJSON(inGenerator);
              case XML::Node::NodeType::Element:       return inNode->toElement()->getOutputSizeJSON(inGenerator);
              case XML::Node::NodeType::Attribute:     return inNode->toAttribute()->getOutputSizeJSON(inGenerator);
              case XML::Node::NodeType::Text:          return inNode->toText()->getOutputSizeJSON(inGenerator);
              case XML::Node::NodeType::Comment:       return inNode->toComment()->getOutputSizeJSON(inGenerator);
              case XML::Node::NodeType::Declaration:   return inNode->toDeclaration()->getOutputSizeJSON(inGenerator);
              case XML::Node::NodeType::Unknown:       return inNode->toUnknown()->getOutputSizeJSON(inGenerator);
              default:
              {
                ZS_THROW_BAD_STATE("missing node type in getOutputSize table")
              }
            }
            break;
          }
        }
        return 0;
      }

      //-----------------------------------------------------------------------
      void Generator::writeBuffer(const GeneratorPtr &inGenerator, NodePtr inNode, char * &ioPos)
      {
        if (!inNode)
          return;

        switch (inGenerator->mGeneratorMode)
        {
          case GeneratorMode_XML:
          {
            switch (inNode->getNodeType())
            {
              case XML::Node::NodeType::Document:      return inNode->toDocument()->writeBufferXML(inGenerator, ioPos);
              case XML::Node::NodeType::Element:       return inNode->toElement()->writeBufferXML(inGenerator, ioPos);
              case XML::Node::NodeType::Attribute:     return inNode->toAttribute()->writeBufferXML(inGenerator, ioPos);
              case XML::Node::NodeType::Text:          return inNode->toText()->writeBufferXML(inGenerator, ioPos);
              case XML::Node::NodeType::Comment:       return inNode->toComment()->writeBufferXML(inGenerator, ioPos);
              case XML::Node::NodeType::Declaration:   return inNode->toDeclaration()->writeBufferXML(inGenerator, ioPos);
              case XML::Node::NodeType::Unknown:       return inNode->toUnknown()->writeBufferXML(inGenerator, ioPos);
              default:
              {
                ZS_THROW_BAD_STATE("missing node type in writeBuffer table")
              }
            }
            break;
          }
          case GeneratorMode_JSON:
          {
            switch (inNode->getNodeType())
            {
              case XML::Node::NodeType::Document:      return inNode->toDocument()->writeBufferJSON(inGenerator, ioPos);
              case XML::Node::NodeType::Element:       return inNode->toElement()->writeBufferJSON(inGenerator, ioPos);
              case XML::Node::NodeType::Attribute:     return inNode->toAttribute()->writeBufferJSON(inGenerator, ioPos);
              case XML::Node::NodeType::Text:          return inNode->toText()->writeBufferJSON(inGenerator, ioPos);
              case XML::Node::NodeType::Comment:       return inNode->toComment()->writeBufferJSON(inGenerator, ioPos);
              case XML::Node::NodeType::Declaration:   return inNode->toDeclaration()->writeBufferJSON(inGenerator, ioPos);
              case XML::Node::NodeType::Unknown:       return inNode->toUnknown()->writeBufferJSON(inGenerator, ioPos);
              default:
              {
                ZS_THROW_BAD_STATE("missing node type in writeBuffer table")
              }
            }
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void Generator::writeBuffer(char * &ioPos, CSTR inString)
      {
        if (NULL == inString)
          return;

        size_t length = strlen(inString);
        strcpy(ioPos, inString);
        ioPos += length;
      }

      //-----------------------------------------------------------------------
      void Generator::getJSONEncodingMode(
                                          const ElementPtr &el,
                                          GeneratorJSONElementModes &outMode,
                                          GeneratorJSONELementChildStates &outChildState,
                                          GeneratorJSONELementArrayPositions &outPositionIfApplicable,
                                          GeneratorJSONTextModes &outTextModeIfApplicable,
                                          bool &outNextInList
                                          ) const
      {
        outNextInList = false;
        outPositionIfApplicable = GeneratorJSONELementArrayPositions_First;
        outTextModeIfApplicable = GeneratorJSONTextMode_Number;

        if (el->toNode() == mGeneratorRoot) {
          outMode = GeneratorJSONElementMode_ObjectType;
        } else {
          ElementPtr prevSibling = el->getPreviousSiblingElement();
          ElementPtr nextSibling = el->getNextSiblingElement();

          bool beforeMatch = false;
          bool afterMatch = false;

          String currentName = el->getValue();

          if ((prevSibling) ||
              (nextSibling)) {
            String prevName;
            String nextName;

            if (prevSibling) {
              prevName = prevSibling->getValue();
            }
            if (nextSibling) {
              nextName = nextSibling->getValue();
            }

            bool caseSensitive = true;

            NodePtr root = el->getRoot();
            if (root->isDocument()) {
              caseSensitive = root->toDocument()->isElementNameIsCaseSensative();
            }

            if (caseSensitive) {
              beforeMatch = (currentName == prevName);
              afterMatch = (currentName == nextName);
            } else {
              beforeMatch = (0 == currentName.compareNoCase(prevName));
              afterMatch = (0 == currentName.compareNoCase(nextName));
            }
          }

          if ((beforeMatch) || (afterMatch)) {
            outMode = GeneratorJSONElementMode_ArrayType;
            if (beforeMatch) {
              if (afterMatch) {
                outPositionIfApplicable = GeneratorJSONELementArrayPositions_Middle;
              } else {
                outPositionIfApplicable = GeneratorJSONELementArrayPositions_Last;
              }
            } else {
              outPositionIfApplicable = GeneratorJSONELementArrayPositions_First;
            }
          } else {
            outMode = GeneratorJSONElementMode_ObjectType;
          }
        }

        if (el->getFirstAttribute()) {
          outChildState = GeneratorJSONELementChildState_Complex;
        } else {
          if (el->hasChildren()) {
            if (el->getFirstChildElement()) {
              outChildState = GeneratorJSONELementChildState_Complex;
            } else {
              outChildState = GeneratorJSONELementChildState_None;

              NodePtr node = el->getFirstChild();
              while (node)
              {
                if (node->isText()) {
                  outChildState = GeneratorJSONELementChildState_TextOnly;
                  break;
                }
                node = node->getNextSibling();
              }
            }
          } else {
            outChildState = GeneratorJSONELementChildState_None;
          }
        }

        switch (outChildState) {
          case GeneratorJSONELementChildState_None: break;
          case GeneratorJSONELementChildState_TextOnly:
          case GeneratorJSONELementChildState_Complex:
          {
            outTextModeIfApplicable = GeneratorJSONTextMode_Number;
            NodePtr node = el->getFirstChild();
            while (node)
            {
              if (node->isText()) {
                TextPtr text = node->toText();
                if (XML::Text::Format_JSONNumberEncoded != text->getFormat()) {
                  outTextModeIfApplicable = GeneratorJSONTextMode_String;
                  break;
                }
              }
              node = node->getNextSibling();
            }
            break;
          }
        }

        if (el->getPreviousSiblingElement()) {
          outNextInList = true;
        }
      }

    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::Generator
    #pragma mark

    //-------------------------------------------------------------------------
    GeneratorPtr Generator::createXMLGenerator(XMLWriteFlags writeFlags)
    {
      GeneratorPtr pThis(new Generator);
      pThis->mThis = pThis;
      pThis->mGeneratorMode = GeneratorMode_XML;
      pThis->mWriteFlags = writeFlags;
      return pThis;
    }

    //-------------------------------------------------------------------------
    GeneratorPtr Generator::createJSONGenerator(
                                                const char *forcedText,
                                                char attributePrefix
                                                )
    {
      GeneratorPtr pThis(new Generator);
      pThis->mThis = pThis;
      pThis->mGeneratorMode = GeneratorMode_JSON;
      pThis->mWriteFlags = 0;
      pThis->mJSONForcedText = (forcedText ? forcedText : "");
      pThis->mJSONAttributePrefix = attributePrefix;
      return pThis;
    }

    //-------------------------------------------------------------------------
    Generator::Generator() :
      internal::Generator()
    {
    }

    //-------------------------------------------------------------------------
    ULONG Generator::getOutputSize(const NodePtr &onlyThisNode) const
    {
      mGeneratorRoot = onlyThisNode;
      ULONG result = internal::Generator::getOutputSize(mThis.lock(), onlyThisNode);
      if (GeneratorMode_JSON == mGeneratorMode) {
        result += strlen("{}"); // the opening and closing object brace
      }
      mGeneratorRoot.reset();
      return result;
    }

    //-------------------------------------------------------------------------
    boost::shared_array<char> Generator::write(const NodePtr &onlyThisNode, ULONG *outLength) const
    {
      ULONG totalSize = getOutputSize(onlyThisNode);
      mGeneratorRoot = onlyThisNode;
      boost::shared_array<char> buffer(new char[totalSize+1]);
      char *pos = buffer.get();
      if (GeneratorMode_JSON == mGeneratorMode) {
        Generator::writeBuffer(pos, "{");
      }
      Generator::writeBuffer(mThis.lock(), onlyThisNode, pos);
      if (GeneratorMode_JSON == mGeneratorMode) {
        Generator::writeBuffer(pos, "}");
      }
      *pos = 0;
      if (NULL != outLength)
        *outLength = totalSize;
      mGeneratorRoot.reset();
      return buffer;
    }

    //-------------------------------------------------------------------------
    Generator::XMLWriteFlags Generator::getXMLWriteFlags() const
    {
      return static_cast<XMLWriteFlags>(mWriteFlags);
    }

  } // namespace XML

} // namespace zsLib
