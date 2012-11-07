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

#pragma once

#ifndef ZSLIB_INTERNAL_XML_H_b78f79d54e0d6193bc57647097d8ce0e
#define ZSLIB_INTERNAL_XML_H_b78f79d54e0d6193bc57647097d8ce0e

#include <list>

#include <zsLib/zsTypes.h>
#include <zsLib/String.h>

#define ZS_INTERNAL_XML_DEFAULT_TAB_SIZE 2

namespace zsLib
{

  namespace XML
  {

    class ParserPos;
    class ParserWarning;

    class Node;
    typedef boost::shared_ptr<Node> NodePtr;
    typedef boost::weak_ptr<Node> NodeWeakPtr;

    class Document;
    typedef boost::shared_ptr<Document> DocumentPtr;
    typedef boost::weak_ptr<Document> DocumentWeakPtr;

    class Element;
    typedef boost::shared_ptr<Element> ElementPtr;
    typedef boost::weak_ptr<Element> ElementWeakPtr;

    class Attribute;
    typedef boost::shared_ptr<Attribute> AttributePtr;
    typedef boost::weak_ptr<Attribute> AttributeWeakPtr;

    class Text;
    typedef boost::shared_ptr<Text> TextPtr;
    typedef boost::weak_ptr<Text> TextWeakPtr;

    class Comment;
    typedef boost::shared_ptr<Comment> CommentPtr;
    typedef boost::weak_ptr<Comment> CommentWeakPtr;

    class Declaration;
    typedef boost::shared_ptr<Declaration> DeclarationPtr;
    typedef boost::weak_ptr<Declaration> DeclarationWeakPtr;

    class Unknown;
    typedef boost::shared_ptr<Unknown> UnknownPtr;
    typedef boost::weak_ptr<Unknown> UnknownWeakPtr;

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

      class Node
      {
      protected:
        friend class XML::Document;
        friend class XML::Element;
        friend class XML::Attribute;
        friend class XML::Text;
        friend class XML::Comment;
        friend class XML::Declaration;
        friend class XML::Unknown;

        friend class Document;
        friend class Element;
        friend class Attribute;
        friend class Text;
        friend class Comment;
        friend class Declaration;
        friend class Unknown;

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

      class Document
      {
      public:
        typedef std::list<String> StringList;
        typedef std::list<XML::ParserWarning> Warnings;
        typedef std::list<XML::ParserPos> ParserStack;
        typedef std::list<zsLib::String> SingleElement;

        friend class XML::ParserPos;

      public:
        Document();

        void clearStack();
        void pushPos(const XML::ParserPos &inPos);
        XML::ParserPos popPos();

        void addWarning(ParserWarningTypes inWarning);
        void addWarning(ParserWarningTypes inWarning, const XML::ParserPos &inPos);

        ULONG getOutputSize() const;
        void writeBuffer(char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        DocumentWeakPtr mThis;

        ULONG       mTabSize;               // how big is the tab size for the document
        const char *mSOF;                   // start of file

        bool        mElementNameCaseSensative;
        bool        mAttributeNameCaseSensative;
        bool        mEnableWarnings;

        StringList  mSingleElements;

        Warnings    mWarnings;
        ParserStack mParserStack;
        UINT        mWriteFlags;
      };

      class Element
      {
      protected:
        friend class XML::Attribute;

      public:
        typedef std::list<AttributePtr> AttributeList;

      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSize(const DocumentPtr &inDocument) const;
        void writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const;

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

      class Attribute
      {
      public:
        bool parse(XML::ParserPos &ioPos);

        ULONG getOutputSize(const DocumentPtr &inDocument) const;
        void writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        Attribute();

      protected:
        String mName;
        String mValue;

        bool mValuelessAttribute;
        AttributeWeakPtr mThis;
      };

      class Text
      {
      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSize(const DocumentPtr &inDocument) const;
        void writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        Text();

        String mValue;
        bool mOutputCDATA;
        bool mInCDATAFormat;

        TextWeakPtr mThis;
      };

      class Comment
      {
      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSize(const DocumentPtr &inDocument) const;
        void writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        String mValue;

        CommentWeakPtr mThis;
      };

      class Declaration
      {
      protected:
        friend class XML::Attribute;

      public:
        void parse(XML::ParserPos &ioPos);

        ULONG getOutputSize(const DocumentPtr &inDocument) const;
        void writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        DeclarationWeakPtr mThis;

        AttributePtr mFirstAttribute;
        AttributePtr mLastAttribute;
      };

      class Unknown
      {
      public:
        void parse(XML::ParserPos &ioPos, const char *start = NULL, const char *ending = NULL);

        ULONG getOutputSize(const DocumentPtr &inDocument) const;
        void writeBuffer(const DocumentPtr &inDocument, char * &ioPos) const;

        NodePtr cloneAssignParent(NodePtr inParent) const;

      protected:
        String mValue;

        UnknownWeakPtr mThis;
      };

      class ParserPos
      {
      protected:
        ParserPos();
        ParserPos(const ParserPos &);

      protected:
        DocumentWeakPtr mDocument;
      };

      class Parser
      {
      public:
        class AutoStack
        {
        public:
          AutoStack(const XML::ParserPos &inPos);
          ~AutoStack();
        private:
          DocumentPtr mDocument;
        };

      public:
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

        static ULONG getOutputSize(const DocumentPtr &inDocument, NodePtr inNode);
        static void writeBuffer(const DocumentPtr &inDocument, NodePtr inNode, char * &ioPos);
        static void writeBuffer(char * &ioPos, CSTR inString);

        static String compressWhiteSpace(const String &inString);

        static NodePtr cloneAssignParent(NodePtr inNewParent, NodePtr inExistingChild);
        static void safeAdoptAsLastChild(NodePtr inParent, NodePtr inNewChild);
      };

    } // namespace internal

  } // namespace XML

} // namespace zsLib

#endif //ZSLIB_INTERNAL_XML_H_b78f79d54e0d6193bc57647097d8ce0e
