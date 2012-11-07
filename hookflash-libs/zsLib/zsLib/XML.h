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

#ifndef ZSLIB_XML_H_58e5a7d8cf6414da70cf4cc573d53693
#define ZSLIB_XML_H_58e5a7d8cf6414da70cf4cc573d53693

#include <zsLib/Exception.h>
#include <boost/shared_array.hpp>

#pragma warning(push)
#pragma warning(disable: 4290)

namespace zsLib {
  namespace XML {

    enum ParserWarningTypes
    {
      ParserWarningType_None,
      ParserWarningType_MismatchedEndTag,
      ParserWarningType_NoEndBracketFound,
      ParserWarningType_ContentAfterCloseSlashInElement,
      ParserWarningType_ContentAfterCloseElementName,
      ParserWarningType_IllegalAttributeName,
      ParserWarningType_AttributeWithoutValue,
      ParserWarningType_AttributeValueNotFound,
      ParserWarningType_AttributeValueMissingEndQuote,
      ParserWarningType_CDATAMissingEndTag,
      ParserWarningType_NoEndTagFound,
      ParserWarningType_NoEndCommentFound,
      ParserWarningType_NoEndUnknownTagFound,
      ParserWarningType_NoEndDeclarationFound,
      ParserWarningType_NotProperEndDeclaration,
      ParserWarningType_DuplicateAttribute,
      ParserWarningType_ElementsNestedTooDeep
    };

  } // namespace XML
} // namespace zsLib

#include <zsLib/internal/XML.h>

namespace zsLib
{
  namespace XML
  {
    struct Exceptions
    {
      ZS_DECLARE_CUSTOM_EXCEPTION(CheckFailed)
    };

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

    class ParserPos;
    class ParserWarning;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class WalkSink
    {
    public:
      // NOTE:  It is safe to call orphan on the current node. If done then
      //        none of children of the orphaned node will be walked.
      virtual bool onDocumentEnter(DocumentPtr inNode);
      virtual bool onDocumentExit(DocumentPtr inNode);
      virtual bool onElementEnter(ElementPtr inNode);
      virtual bool onElementExit(ElementPtr inNode);
      virtual bool onAttribute(AttributePtr inNode);
      virtual bool onText(TextPtr inNode);
      virtual bool onComment(CommentPtr inNode);
      virtual bool onDeclarationEnter(DeclarationPtr inNode);
      virtual bool onDeclarationExit(DeclarationPtr inNode);
      virtual bool onUnknown(UnknownPtr inNode);
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Node : public internal::Node
    {
    public:
      struct NodeType
      {
        enum Type
        {
          Document,
          Element,
          Attribute,
          Text,
          Comment,
          Declaration,
          Unknown
        };
      };

      typedef std::list<NodeType::Type> FilterList;

    public:
      ~Node();

      DocumentPtr getDocument() const;

      virtual bool walk(WalkSink &inWalker, const FilterList *inFilterList = NULL) const;
      virtual bool walk(WalkSink &inWalker, NodeType::Type inType) const;

      // these methods will return NULL if the related node does not exist
      virtual NodePtr getParent() const;
      virtual NodePtr getRoot() const;

      virtual NodePtr getFirstChild() const;
      virtual NodePtr getLastChild() const;

      virtual NodePtr getFirstSibling() const;
      virtual NodePtr getLastSibling() const;

      virtual NodePtr getPreviousSibling() const;
      virtual NodePtr getNextSibling() const;

      virtual ElementPtr getParentElement() const;
      virtual ElementPtr getRootElement() const;

      virtual ElementPtr getFirstChildElement() const;
      virtual ElementPtr getLastChildElement() const;

      virtual ElementPtr getFirstSiblingElement() const;
      virtual ElementPtr getLastSiblingElement() const;

      virtual ElementPtr getPreviousSiblingElement() const;
      virtual ElementPtr getNextSiblingElement() const;

      virtual ElementPtr findPreviousSiblingElement(String elementName) const;
      virtual ElementPtr findNextSiblingElement(String elementName) const;

      virtual ElementPtr findFirstChildElement(String elementName) const;
      virtual ElementPtr findLastChildElement(String elementName) const;

      // checked version of the above methods which throw an exception if they fail to return a valid value instead of returning NULL
      virtual NodePtr getParentChecked() const throw(Exceptions::CheckFailed);
      virtual NodePtr getRootChecked() const throw(Exceptions::CheckFailed);

      virtual NodePtr getFirstChildChecked() const throw(Exceptions::CheckFailed);
      virtual NodePtr getLastChildChecked() const throw(Exceptions::CheckFailed);

      virtual NodePtr getFirstSiblingChecked() const throw(Exceptions::CheckFailed);
      virtual NodePtr getLastSiblingChecked() const throw(Exceptions::CheckFailed);

      virtual NodePtr getPreviousSiblingChecked() const throw(Exceptions::CheckFailed);
      virtual NodePtr getNextSiblingChecked() const throw(Exceptions::CheckFailed);

      virtual ElementPtr getParentElementChecked() const throw(Exceptions::CheckFailed);
      virtual ElementPtr getRootElementChecked() const throw(Exceptions::CheckFailed);

      virtual ElementPtr getFirstChildElementChecked() const throw(Exceptions::CheckFailed);
      virtual ElementPtr getLastChildElementChecked() const throw(Exceptions::CheckFailed);

      virtual ElementPtr getFirstSiblingElementChecked() const throw(Exceptions::CheckFailed);
      virtual ElementPtr getLastSiblingElementChecked() const throw(Exceptions::CheckFailed);

      virtual ElementPtr getPreviousSiblingElementChecked() const throw(Exceptions::CheckFailed);
      virtual ElementPtr getNextSiblingElementChecked() const throw(Exceptions::CheckFailed);

      virtual ElementPtr findPreviousSiblingElementChecked(String elementName) const throw(Exceptions::CheckFailed);
      virtual ElementPtr findNextSiblingElementChecked(String elementName) const throw(Exceptions::CheckFailed);

      virtual ElementPtr findFirstChildElementChecked(String elementName) const throw(Exceptions::CheckFailed);
      virtual ElementPtr findLastChildElementChecked(String elementName) const throw(Exceptions::CheckFailed);

      virtual void orphan();                                // this node now is a root element and has no document

      virtual void adoptAsFirstChild(NodePtr inNode);       // this node is now adopted as the first child of the current node
      virtual void adoptAsLastChild(NodePtr inNode);        // this node is now adopted as the last child of the current node

      virtual void adoptAsPreviousSibling(NodePtr inNode);  // this node is now adopted as the previous sibling to the current
      virtual void adoptAsNextSibling(NodePtr inNode);      // this node is now adopted as the next sibling from the current

      virtual bool hasChildren();                           // does the node have children?
      virtual void removeChildren();                        // removes all children

      virtual void clear();                                 // remove all contents for the node

      virtual void *getUserData() const;                    // get private data associated with this node
      virtual void setUserData(void *inData);               // set private data associated with this node

      virtual NodeType::Type getNodeType() = 0;

      virtual bool isDocument() const                 {return false;}
      virtual bool isElement() const                  {return false;}
      virtual bool isAttribute() const                {return false;}
      virtual bool isText() const                     {return false;}
      virtual bool isComment() const                  {return false;}
      virtual bool isDeclaration() const              {return false;}
      virtual bool isUnknown() const                  {return false;}

      virtual NodePtr         toNode() const          {return NodePtr();}
      virtual DocumentPtr     toDocument() const      {return DocumentPtr();}
      virtual ElementPtr      toElement() const       {return ElementPtr();}
      virtual AttributePtr    toAttribute() const     {return AttributePtr();}
      virtual TextPtr         toText() const          {return TextPtr();}
      virtual CommentPtr      toComment() const       {return CommentPtr();}
      virtual DeclarationPtr  toDeclaration() const   {return DeclarationPtr();}
      virtual UnknownPtr      toUnknown() const       {return UnknownPtr();}

      virtual NodePtr         toNodeChecked() const throw(Exceptions::CheckFailed);
      virtual DocumentPtr     toDocumentChecked() const throw(Exceptions::CheckFailed);
      virtual ElementPtr      toElementChecked() const throw(Exceptions::CheckFailed);
      virtual AttributePtr    toAttributeChecked() const throw(Exceptions::CheckFailed);
      virtual TextPtr         toTextChecked() const throw(Exceptions::CheckFailed);
      virtual CommentPtr      toCommentChecked() const throw(Exceptions::CheckFailed);
      virtual DeclarationPtr  toDeclarationChecked() const throw(Exceptions::CheckFailed);
      virtual UnknownPtr      toUnknownChecked() const throw(Exceptions::CheckFailed);

      virtual NodePtr clone() const = 0;                    // creates a clone of this object and clone becomes root object

      virtual String getValue() const                 {return String();}

    protected:
      Node();

      const Node &operator=(const Node &);
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Document : public Node,
                     public internal::Document
    {
    public:
      typedef std::list<String> StringList;
      typedef std::list<XML::ParserWarning> Warnings;

      enum WriteFlags
      {
        WriteFlag_None =                                    0x00000000,
        WriteFlag_ForceElementEndTag =                      0x00000001,
        WriteFlag_NormalizeCDATA =                          0x00000002,
        WriteFlag_EntityEncodeWindowsCarriageReturnInText = 0x00000004, // requires WriteFlag_NormalizeCDATA is set
        WriteFlag_NormizeAttributeValue =                   0x00000008,
      };

    public:
      static DocumentPtr create();

      // additional methods
      ULONG getTabSize() const;
      void setTabSize(ULONG inTabSize);

      void addContainsNoChildrenElement(String inElement);
      const StringList &getContainsNoChildrenElements();

      void setElementNameIsCaseSensative(bool inCaseSensative = true);
      bool isElementNameIsCaseSensative() const;

      void setAttributeNameIsCaseSensative(bool inCaseSensative = true);
      bool isAttributeNameIsCaseSensative() const;

      void clearWarnings();
      const Warnings &getWarnings() const;
      void enableWarnings(bool inEnableWarnings)   {mEnableWarnings = inEnableWarnings;}
      bool areWarningsEnabled()                    {return mEnableWarnings;}

      void parse(const char *inXMLDocument); // must be terminated with a NUL character

      ULONG getOutputSize() const;
      boost::shared_array<char> write(ULONG *outLength = NULL) const;

      ULONG getOutputSize(const NodePtr &onlyThisNode) const;
      boost::shared_array<char> write(const NodePtr &onlyThisNode, ULONG *outLength = NULL) const;

      void setWriteFlags(WriteFlags);
      WriteFlags getWriteFlags() const;

      // overrides
      virtual NodePtr clone() const;
      virtual void clear();

      virtual NodeType::Type  getNodeType()        {return NodeType::Document;}
      virtual bool            isDocument() const   {return true;}
      virtual NodePtr         toNode() const       {return mThis.lock();}
      virtual DocumentPtr     toDocument() const   {return mThis.lock();}

    protected:
      Document();
      const Document &operator=(const Document &) {return *this;}
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Element : public Node,
                    public internal::Element
    {
    public:
      static ElementPtr create(const char *name = NULL);

      void setValue(String inName)  {mName = inName;}

      String getText(
                     bool inCompressWhiteSpace = false,
                     bool inIncludeTextOfChildElements = true
                     );

      String getTextAndEntityDecode(
                                    bool inCompressWhiteSpace = false,
                                    bool inIncludeTextOfChildElements = true
                                    );

      AttributePtr findAttribute(String inName) const;
      String getAttributeValue(String inName) const;

      AttributePtr findAttributeChecked(String inName) const throw(Exceptions::CheckFailed);
      String getAttributeValueChecked(String inName) const throw(Exceptions::CheckFailed);

      bool setAttribute(String inName, String inValue);  // returns true if replacing existing attribute
      bool setAttribute(AttributePtr inAttribute);       // returns true if replacing existing attribute
      bool deleteAttribute(String inName);               // remove an existing attribute

      AttributePtr getFirstAttribute() const;
      AttributePtr getLastAttribute() const;

      AttributePtr getFirstAttributeChecked() const throw(Exceptions::CheckFailed);
      AttributePtr getLastAttributeChecked() const throw(Exceptions::CheckFailed);

      // overrides
      virtual void adoptAsFirstChild(NodePtr inNode);
      virtual void adoptAsLastChild(NodePtr inNode);

      virtual NodePtr clone() const;
      virtual void clear();

      virtual String getValue() const;                         // returns the element name

      virtual NodeType::Type  getNodeType()     {return NodeType::Element;}
      virtual bool            isElement() const {return true;}
      virtual NodePtr         toNode() const    {return mThis.lock();}
      virtual ElementPtr      toElement() const {return mThis.lock();}

    protected:
      Element();
      const Element &operator=(const Element &) {return *this;}
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Attribute : public Node,
                      public internal::Attribute
    {
    public:
      static AttributePtr create();

      String getName() const;          // get the name for the current attribute
      void setName(String inName);     // sets the attribute name

      void setValue(String inValue);

      // overrides
      virtual NodePtr getFirstChild() const  {return NodePtr();}
      virtual NodePtr getLastChild() const   {return NodePtr();}

      virtual NodePtr getFirstSibling() const;
      virtual NodePtr getLastSibling() const;

      virtual void orphan();                                // this node now is a root element and has no document

      virtual void adoptAsPreviousSibling(NodePtr inNode);  // this node is now adopted as the previous sibling to the current
      virtual void adoptAsNextSibling(NodePtr inNode);      // this node is now adopted as the next sibling from the current

      virtual bool hasChildren()    {return false;}
      virtual void removeChildren() {}

      virtual NodePtr clone() const;
      virtual void clear();

      virtual String getValue() const;

      virtual NodeType::Type  getNodeType()        {return NodeType::Attribute;}
      virtual bool            isAttribute() const  {return true;}
      virtual NodePtr         toNode() const       {return mThis.lock();}
      virtual AttributePtr    toAttribute() const  {return mThis.lock();}

    protected:
      Attribute();
      const Attribute &operator=(const Attribute &) {return *this;}

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Text : public Node,
                 public internal::Text
    {
    public:
      static TextPtr create();

      void setValue(const String &inText, bool inCDATAFormat = false);    // should be encoded with entities
      void setValueAndEntityEncode(const String &inText);

      bool getOutputCDATA() const;
      void setOutputCDATA(bool inOutputCDATA = true);

      // overrides
      virtual bool hasChildren()    {return false;}
      virtual void removeChildren() {return;}

      virtual NodePtr clone() const;
      virtual void clear();

      virtual String getValue() const;       // encoded with entiries
      virtual String getValueAndEntityDecode() const;

      virtual NodeType::Type  getNodeType()  {return NodeType::Text;}
      virtual bool            isText() const {return true;}
      virtual NodePtr         toNode() const {return mThis.lock();}
      virtual TextPtr         toText() const {return mThis.lock();}

    protected:
      Text();
      const Text &operator=(const Text &) {return *this;}

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Comment : public Node,
                    public internal::Comment
    {
    public:
      static CommentPtr create();

      void setValue(String inValue) {mValue = inValue;}

      // overrides
      virtual bool hasChildren()    {return false;}
      virtual void removeChildren() {return;}

      virtual NodePtr clone() const;
      virtual void clear();

      virtual String getValue() const;

      virtual NodeType::Type  getNodeType()     {return NodeType::Comment;}
      virtual bool            isComment() const {return true;}
      virtual NodePtr         toNode() const    {return mThis.lock();}
      virtual CommentPtr      toComment() const {return mThis.lock();}

    protected:
      Comment();
      const Comment &operator=(const Comment &) {return *this;}

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Declaration : public Node,
                        public internal::Declaration
    {
    public:
      static DeclarationPtr create();

      AttributePtr findAttribute(String inName) const;
      String getAttributeValue(String inName) const;

      AttributePtr findAttributeChecked(String inName) const throw(Exceptions::CheckFailed);
      String getAttributeValueChecked(String inName) const throw(Exceptions::CheckFailed);

      bool setAttribute(String inName, String inValue);  // returns true if replacing existing attribute
      bool setAttribute(AttributePtr inAttribute);       // returns true if replacing existing attribute
      bool deleteAttribute(String inName);

      AttributePtr getFirstAttribute() const;
      AttributePtr getLastAttribute() const;

      AttributePtr getFirstAttributeChecked() const throw(Exceptions::CheckFailed);
      AttributePtr getLastAttributeChecked() const throw(Exceptions::CheckFailed);

      // overrides
      virtual void adoptAsFirstChild(NodePtr inNode);    // can only add attribute children
      virtual void adoptAsLastChild(NodePtr inNode);     // can only add attribute children

      virtual NodePtr clone() const;
      virtual void clear();

      virtual NodeType::Type  getNodeType()           {return NodeType::Declaration;}
      virtual bool            isDeclaration() const   {return true;}
      virtual NodePtr         toNode() const          {return mThis.lock();}
      virtual DeclarationPtr  toDeclaration() const   {return mThis.lock();}

    protected:
      Declaration();
      const Declaration &operator=(const Declaration &) {return *this;}
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Unknown : public Node,
                    public internal::Unknown
    {
    public:
      static UnknownPtr create();

      void setValue(String inValue) {mValue = inValue;}

      // overrides
      virtual bool hasChildren()    {return false;}
      virtual void removeChildren() {return;}

      virtual NodePtr clone() const;
      virtual void clear();

      virtual String getValue() const;

      virtual NodeType::Type  getNodeType()     {return NodeType::Unknown;}
      virtual bool            isUnknown() const {return true;}
      virtual NodePtr         toNode() const    {return mThis.lock();}
      virtual UnknownPtr      toUnknown() const {return mThis.lock();}

    protected:
      Unknown();
      const Unknown &operator=(const Unknown &) {return *this;}

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class ParserPos : public internal::ParserPos
    {
    public:
      ULONG mRow;
      ULONG mColumn;
      const char *mPos;

    public:
      ParserPos();
      ParserPos(const ParserPos &);
      ParserPos(DocumentPtr inDocument);

      bool isSOF() const;  // SOF = start of file
      bool isEOF() const;  // EOF = end of file

      void setSOF(); // force the parse pos to the start of the file
      void setEOF(); // force the parse pos to be at the end of the file

      void setDocument(DocumentPtr inDocument);
      DocumentPtr getDocument() const;

      ParserPos operator++() const;
      ParserPos &operator++();

      ParserPos operator--() const;
      ParserPos &operator--();

      size_t operator-(const ParserPos &inPos) const;

      ParserPos &operator+=(ULONG inDistance);
      ParserPos &operator-=(ULONG inDistance);

      bool operator==(const ParserPos &inPos);
      bool operator!=(const ParserPos &inPos);

      char operator*() const;

      operator CSTR() const;

      bool isString(CSTR inString, bool inCaseSensative = true) const;

    protected:
      friend class Document;
      ParserPos(Document &);
    };

    ParserPos operator+(const ParserPos &inPos, ULONG inDistance);
    ParserPos operator-(const ParserPos &inPos, ULONG inDistance);
    ParserPos operator+(const ParserPos &inPos, int inDistance);
    ParserPos operator-(const ParserPos &inPos, int inDistance);
    ParserPos operator+(const ParserPos &inPos, unsigned int inDistance);
    ParserPos operator-(const ParserPos &inPos, unsigned int inDistance);
    ParserPos operator+(const ParserPos &inPos, size_t inDistance);
    ParserPos operator-(const ParserPos &inPos, size_t inDistance);


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class ParserWarning
    {
    public:
      struct ParserInfo
      {
        ParserPos mPos;      // the const char * location is only valid for the lifetime of the buffer passed into the parser
        String mXMLSnip;
      };
      typedef std::list<ParserInfo> ParserStack;

    public:
      ParserWarningTypes mWarningType;
      ParserStack mStack;

      String getAsString(bool inIncludeEntireStack = true) const;

    protected:
      friend class XML::internal::Document;
      ParserWarning(
                    ParserWarningTypes inWarningType,
                    const XML::internal::Document::ParserStack &inStack
                    );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class Parser : public internal::Parser
    {
    public:

      // helper methods
      static String convertFromEntities(const String &inString);

      static String makeTextEntitySafe(const String &inString, bool entityEncodeWindowsCarriageReturn = false);
      static String makeAttributeEntitySafe(const String &inString, char willUseSurroundingQuotes = 0);
    };


  } // namespace XML

} // namespace zsLib

#pragma warning(pop)

#endif //ZSLIB_XML_H_58e5a7d8cf6414da70cf4cc573d53693
