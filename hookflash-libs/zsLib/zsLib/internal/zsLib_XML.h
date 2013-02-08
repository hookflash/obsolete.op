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

#pragma once

#ifndef ZSLIB_INTERNAL_XML_H_b78f79d54e0d6193bc57647097d8ce0e
#define ZSLIB_INTERNAL_XML_H_b78f79d54e0d6193bc57647097d8ce0e

#include <list>

#include <zsLib/types.h>
#include <zsLib/String.h>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#define ZS_INTERNAL_XML_DEFAULT_TAB_SIZE 2

namespace zsLib
{

  namespace XML
  {

    namespace internal
    {

      class Node;
      class Document;
      class Element;
      class Attribute;
      class Text;
      class Comment;
      class Declaration;
      class Unknown;

      class ParserPos;
      class Parser;
      class ParserHelper;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Node : public boost::noncopyable
      {
      protected:
        friend class XML::Document;
        friend class XML::Element;
        friend class XML::Attribute;
        friend class XML::Text;
        friend class XML::Comment;
        friend class XML::Declaration;
        friend class XML::Unknown;
        friend class XML::Parser;

        friend class Document;
        friend class Element;
        friend class Attribute;
        friend class Text;
        friend class Comment;
        friend class Declaration;
        friend class Unknown;
        friend class Parser;

      protected:
        Node();

        void cloneChildren(const NodePtr &inSelf, NodePtr inNewObject) const;

      protected:
        NodeWeakPtr mParent;

        NodePtr mPreviousSibling;
        NodePtr mNextSibling;

        NodePtr mFirstChild;
        NodePtr mLastChild;

        void *mUserData;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Document : public boost::noncopyable
      {
      public:
        Document(
                 bool inElementNameIsCaseSensative,
                 bool inAttributeNameIsCaseSensative
                 );

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        DocumentWeakPtr mThis;

        bool        mElementNameCaseSensative;
        bool        mAttributeNameCaseSensative;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Element : public boost::noncopyable
      {
      protected:
        friend class XML::Attribute;

      public:
        typedef std::list<AttributePtr> AttributeList;

      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      private:
        void cloneAttributes(ElementPtr inOriginalElement);

        static void actualParse(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos);
        static void parseAttributes(ElementPtr inCurrentElement, XML::ParserPos &ioPos);
        static bool parseTagSingleElementEndSlash(XML::ParserPos &ioPos);
        static bool parseIsDocumentSingleElement(ElementPtr inCurrentElement, XML::ParserPos &ioPos);
        static bool parseNewElement(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos, ULONG &ioTotalPushes);
        static void parseEndTag(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos, ULONG &ioTotalPushes);
        static void parsePopElement(ElementPtr &ioCurrentElement, XML::ParserPos &ioPos, ULONG &ioTotalPushes);
        static void parsePopPushes(XML::ParserPos &ioPos, ULONG &ioTotalPushes);

      protected:
        ElementWeakPtr mThis;

        String mName;

        AttributePtr mFirstAttribute;
        AttributePtr mLastAttribute;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Attribute : public boost::noncopyable
      {
      public:
        bool parse(XML::ParserPos &ioPos);

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        Attribute();

      protected:
        String mName;
        String mValue;

        bool mValuelessAttribute;
        bool mHasQuotes;
        AttributeWeakPtr mThis;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Text : public boost::noncopyable
      {
      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

        String getValueInFormat(
                                UINT format,
                                bool normalize = false,
                                bool encode0xDCharactersInText = false
                                ) const;

      protected:
        Text();

        String mValue;
        UINT mFormat;
        UINT mOutputFormat;

        TextWeakPtr mThis;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Comment : public boost::noncopyable
      {
      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        String mValue;

        CommentWeakPtr mThis;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Declaration : public boost::noncopyable
      {
      protected:
        friend class XML::Attribute;

      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        DeclarationWeakPtr mThis;

        AttributePtr mFirstAttribute;
        AttributePtr mLastAttribute;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Unknown : public boost::noncopyable
      {
      public:
        void parse(XML::ParserPos &ioPos, const char *start = NULL, const char *ending = NULL);

        ULONG getOutputSizeXML(const GeneratorPtr &inGenerator) const;
        void writeBufferXML(const GeneratorPtr &inGenerator, char * &ioPos) const;

        ULONG getOutputSizeJSON(const GeneratorPtr &inGenerator) const;
        void writeBufferJSON(const GeneratorPtr &inGenerator, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        String mValue;

        UnknownWeakPtr mThis;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class ParserPos
      {
      protected:
        ParserPos();
        ParserPos(const ParserPos &);

      protected:
        ParserWeakPtr mParser;
        DocumentWeakPtr mDocument;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Parser
      {
      public:
        friend class XML::Node;
        friend class XML::Document;
        friend class XML::Element;
        friend class XML::Attribute;
        friend class XML::Text;
        friend class XML::Comment;
        friend class XML::Declaration;
        friend class XML::Unknown;
        friend class XML::Parser;

        friend class Node;
        friend class Document;
        friend class Element;
        friend class Attribute;
        friend class Text;
        friend class Comment;
        friend class Declaration;
        friend class Unknown;

      public:
        enum ParserModes
        {
          ParserMode_AutoDetect,
          ParserMode_XML,
          ParserMode_JSON,
        };

        class AutoStack
        {
        public:
          AutoStack(const XML::ParserPos &inPos);
          ~AutoStack();
        private:
          ParserPtr mParser;
        };

        typedef std::list<String> NoChildrenElementList;

        typedef std::list<XML::ParserWarning> Warnings;
        typedef std::list<XML::ParserPos> ParserStack;

        friend class XML::ParserPos;

      public:
        Parser();

        void clearStack();
        void pushPos(const XML::ParserPos &inPos);
        XML::ParserPos popPos();

        void addWarning(ParserWarningTypes inWarning);
        void addWarning(ParserWarningTypes inWarning, const XML::ParserPos &inPos);

      protected:
        const NoChildrenElementList &getContainsNoChildrenElements() const {return mSingleElements;}

      protected:
        static bool isAlpha(char inLetter);
        static bool isDigit(char inLetter);
        static bool isAlphaNumeric(char inLetter);
        static bool isHexDigit(char inLetter);

        static bool isLegalName(char inLetter, bool inFirstLetter);

        static WCHAR findEntity(const String &inLookup);

        static bool isWhiteSpace(char inLetter);
        static bool isWhiteSpace(const XML::ParserPos &inPos);
        static bool skipWhiteSpace(XML::ParserPos &inPos);

        static bool parseAnyExceptElement(XML::ParserPos &ioPos, NodePtr parent);

        static String parseLegalName(XML::ParserPos &inPos);
        static bool skipMismatchedEndTag(XML::ParserPos &ioPos);

        static String compressWhiteSpace(const String &inString);

        static NodePtr cloneAssignParent(NodePtr inNewParent, NodePtr inExistingChild);
        static void safeAdoptAsLastChild(NodePtr inParent, NodePtr inNewChild);

        static String parseJSONString(XML::ParserPos &ioPos);
        static String parseJSONNumber(XML::ParserPos &ioPos);
        bool parseSimpleJSONValue(XML::ParserPos &ioPos, String &outResult, bool &outIsQuoted);

      protected:
        ParserWeakPtr mThis;

        ParserModes mParserMode;

        ULONG       mTabSize;               // how big is the tab size for the document
        const char *mSOF;                   // start of file

        bool        mEnableWarnings;
        Warnings    mWarnings;
        ParserStack mParserStack;

        NoChildrenElementList  mSingleElements;

        String mJSONForcedText;
        char mJSONAttributePrefix;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class Generator : public boost::noncopyable
      {
      public:
        friend class XML::Node;
        friend class XML::Document;
        friend class XML::Element;
        friend class XML::Attribute;
        friend class XML::Text;
        friend class XML::Comment;
        friend class XML::Declaration;
        friend class XML::Unknown;
        friend class XML::Parser;

        friend class Node;
        friend class Document;
        friend class Element;
        friend class Attribute;
        friend class Text;
        friend class Comment;
        friend class Declaration;
        friend class Unknown;
        friend class Parser;

      public:
        enum GeneratorModes
        {
          GeneratorMode_XML,
          GeneratorMode_JSON,
        };

        enum GeneratorJSONElementModes
        {
          GeneratorJSONElementMode_ObjectType,
          GeneratorJSONElementMode_ArrayType,
        };

        enum GeneratorJSONELementChildStates
        {
          GeneratorJSONELementChildState_None,
          GeneratorJSONELementChildState_TextOnly,
          GeneratorJSONELementChildState_Complex,
        };

        enum GeneratorJSONELementArrayPositions
        {
          GeneratorJSONELementArrayPositions_First,
          GeneratorJSONELementArrayPositions_Middle,
          GeneratorJSONELementArrayPositions_Last,
        };

        enum GeneratorJSONTextModes
        {
          GeneratorJSONTextMode_String,
          GeneratorJSONTextMode_Number,
        };

      public:
        Generator();

      protected:
        static ULONG getOutputSize(const GeneratorPtr &inGenerator, NodePtr inNode);
        static void writeBuffer(const GeneratorPtr &inGenerator, NodePtr inNode, char * &ioPos);
        static void writeBuffer(char * &ioPos, CSTR inString);

        void getJSONEncodingMode(
                                 const ElementPtr &el,
                                 GeneratorJSONElementModes &outMode,
                                 GeneratorJSONELementChildStates &outChildState,
                                 GeneratorJSONELementArrayPositions &outPositionIfApplicable,
                                 GeneratorJSONTextModes &outTextModeIfApplicable,
                                 bool &outNextInList
                                 ) const;

      protected:
        GeneratorWeakPtr mThis;

        UINT mWriteFlags;
        GeneratorModes mGeneratorMode;

        String mJSONForcedText;
        char mJSONAttributePrefix;

        mutable NodePtr mGeneratorRoot;
      };

    } // namespace internal

  } // namespace XML

} // namespace zsLib

#endif //ZSLIB_INTERNAL_XML_H_b78f79d54e0d6193bc57647097d8ce0e
