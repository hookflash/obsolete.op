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

#ifndef ZSLIB_XML_H_58e5a7d8cf6414da70cf4cc573d53693
#define ZSLIB_XML_H_58e5a7d8cf6414da70cf4cc573d53693

#include <zsLib/types.h>
#include <zsLib/Exception.h>
#include <boost/shared_array.hpp>

#pragma warning(push)
#pragma warning(disable: 4290)

#define ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX '$'
#define ZS_JSON_DEFAULT_FORCED_TEXT "#text"

namespace zsLib
{
  namespace XML
  {

    enum ParserWarningTypes
    {
      ParserWarningType_None,

      // XML warnings
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
      ParserWarningType_ElementsNestedTooDeep,

      // JSON warnings
      ParserWarningType_MustOpenWithObject,
      ParserWarningType_MustCloseRootObject,
      ParserWarningType_MissingObjectClose,
      ParserWarningType_DataFoundAfterFinalObjectClose,
      ParserWarningType_MissingStringQuotes,
      ParserWarningType_InvalidEscapeSequence,
      ParserWarningType_InvalidUnicodeEscapeSequence,
      ParserWarningType_IllegalNumberSequence,
      ParserWarningType_MissingColonBetweenStringAndValue,
      ParserWarningType_AttributePrefixWithoutName,
      ParserWarningType_AttributePrefixAtRoot,
      ParserWarningType_MissingPairString,
      ParserWarningType_IllegalValue,
      ParserWarningType_IllegalArrayAtRoot,
      ParserWarningType_UnexpectedComma,
      ParserWarningType_ParserStuck,
    };

  } // namespace XML
} // namespace zsLib

#include <zsLib/internal/zsLib_XML.h>

namespace zsLib
{
  namespace XML
  {
    struct Exceptions
    {
      ZS_DECLARE_CUSTOM_EXCEPTION(CheckFailed)
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark WalkSink
    #pragma mark

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
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Node
    #pragma mark

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

      virtual String getBalue() const                 {return String();}

    protected:
      Node();
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Document
    #pragma mark

    class Document : public Node,
                     public internal::Document
    {
    public:
      static DocumentPtr create(
                                bool inElementNameIsCaseSensative = true,
                                bool inAttributeNameIsCaseSensative = true
                                );

      static DocumentPtr createFromParsedXML(
                                             const char *inXMLDocument,
                                             bool inElementNameIsCaseSensative = true,
                                             bool inAttributeNameIsCaseSensative = true
                                             );

      static DocumentPtr createFromParsedJSON(
                                              const char *inJSONDocument,
                                              const char *forcedText = ZS_JSON_DEFAULT_FORCED_TEXT,
                                              char attributePrefix = ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX,
                                              bool inElementNameIsCaseSensative = true,
                                              bool inAttributeNameIsCaseSensative = true
                                              );

      static DocumentPtr createFromAutoDetect(
                                              const char *inDocument,
                                              const char *forcedText = ZS_JSON_DEFAULT_FORCED_TEXT,
                                              char attributePrefix = ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX,
                                              bool inElementNameIsCaseSensative = true,
                                              bool inAttributeNameIsCaseSensative = true
                                              );

      // additional methods
      void setElementNameIsCaseSensative(bool inCaseSensative = true);
      bool isElementNameIsCaseSensative() const;

      void setAttributeNameIsCaseSensative(bool inCaseSensative = true);
      bool isAttributeNameIsCaseSensative() const;

      boost::shared_array<char> writeAsXML(ULONG *outLength = NULL) const;
      boost::shared_array<char> writeAsJSON(ULONG *outLength = NULL) const;

      // overrides
      virtual NodePtr clone() const;
      virtual void clear();

      virtual NodeType::Type  getNodeType()        {return NodeType::Document;}
      virtual bool            isDocument() const   {return true;}
      virtual NodePtr         toNode() const       {return mThis.lock();}
      virtual DocumentPtr     toDocument() const   {return mThis.lock();}

    protected:
      Document(
               bool inElementNameIsCaseSensative = true,
               bool inAttributeNameIsCaseSensative = true
               );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Element
    #pragma mark

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

      String getTextDecoded(
                            bool inCompressWhiteSpace = false,
                            bool inIncludeTextOfChildElements = true
                            );

      AttributePtr findAttribute(String inName) const;
      String getAttributeValue(String inName) const;

      AttributePtr findAttributeChecked(String inName) const throw(Exceptions::CheckFailed);
      String getAttributeValueChecked(String inName) const throw(Exceptions::CheckFailed);

      bool setAttribute(String inName, String inValue, bool quoted = true);  // returns true if replacing existing attribute
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
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Attribute
    #pragma mark

    class Attribute : public Node,
                      public internal::Attribute
    {
    public:
      static AttributePtr create();

      String getName() const;          // get the name for the current attribute
      void setName(String inName);     // sets the attribute name

      void setValue(String inValue);

      void setQuoted(bool inQuoted);

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
      virtual String getValueDecoded() const;

      virtual NodeType::Type  getNodeType()        {return NodeType::Attribute;}
      virtual bool            isAttribute() const  {return true;}
      virtual NodePtr         toNode() const       {return mThis.lock();}
      virtual AttributePtr    toAttribute() const  {return mThis.lock();}

    protected:
      Attribute();

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Text
    #pragma mark

    class Text : public Node,
                 public internal::Text
    {
    public:
      enum Formats
      {
        Format_EntityEncoded,
        Format_CDATA,
        Format_JSONStringEncoded,
        Format_JSONNumberEncoded,
      };

    public:
      static TextPtr create();

      void setValue(const String &inText, Formats format = Format_EntityEncoded);    // should be encoded with entities
      void setValueAndEntityEncode(const String &inText);
      void setValueAndJSONEncode(const String &inText);

      Formats getFormat() const;

      Formats getOutputFormat() const;
      void setOutputFormat(Formats format);

      // overrides
      virtual bool hasChildren()    {return false;}
      virtual void removeChildren() {return;}

      virtual NodePtr clone() const;
      virtual void clear();

      virtual String getValue() const;
      virtual String getValueDecoded() const;
      virtual String getValueInFormat(
                                      Formats format,
                                      bool normalize = false,
                                      bool encode0xDCharactersInText = false
                                      ) const;

      virtual NodeType::Type  getNodeType()  {return NodeType::Text;}
      virtual bool            isText() const {return true;}
      virtual NodePtr         toNode() const {return mThis.lock();}
      virtual TextPtr         toText() const {return mThis.lock();}

    protected:
      Text();

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Comment
    #pragma mark

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

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Declaration
    #pragma mark

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
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Unknown
    #pragma mark

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

      virtual void adoptAsFirstChild(NodePtr inNode);       // illegal
      virtual void adoptAsLastChild(NodePtr inNode);        // illegal
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ParserPos
    #pragma mark

    class ParserPos : public internal::ParserPos
    {
    public:
      ULONG mRow;
      ULONG mColumn;
      const char *mPos;

    public:
      ParserPos();
      ParserPos(const ParserPos &);
      ParserPos(
                ParserPtr inParser,
                DocumentPtr inDocument
                );

      bool isSOF() const;  // SOF = start of file
      bool isEOF() const;  // EOF = end of file

      void setSOF(); // force the parse pos to the start of the file
      void setEOF(); // force the parse pos to be at the end of the file

      void setParser(ParserPtr inParser);
      ParserPtr getParser() const;

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
      friend class Parser;
      friend class Document;

      ParserPos(Parser &, Document &);
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
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ParserWarning
    #pragma mark

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
      friend class XML::internal::Parser;
      ParserWarning(
                    ParserWarningTypes inWarningType,
                    const XML::internal::Parser::ParserStack &inStack
                    );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Parser
    #pragma mark

    class Parser : public internal::Parser
    {
    public:
      typedef internal::Parser::NoChildrenElementList NoChildrenElementList;

    public:
      static ParserPtr createXMLParser();
      static ParserPtr createJSONParser(
                                        const char *forcedText = ZS_JSON_DEFAULT_FORCED_TEXT,
                                        char attributePrefix = ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX
                                        );

      static ParserPtr createAutoDetectParser(
                                              const char *jsonForcedTextElement = ZS_JSON_DEFAULT_FORCED_TEXT,
                                              char jsonAttributePrefix = ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX
                                              );

      virtual DocumentPtr parse(
                                const char *inDocument,
                                bool inElementNameIsCaseSensative = true,
                                bool inAttributeNameIsCaseSensative = true
                                );  // must be terminated with a NUL character

      void clearWarnings();
      const Warnings &getWarnings() const;
      void enableWarnings(bool inEnableWarnings)   {mEnableWarnings = inEnableWarnings;}
      bool areWarningsEnabled()                    {return mEnableWarnings;}

      ULONG getTabSize() const;
      void setTabSize(ULONG inTabSize);

      void setNoChildrenElements(const NoChildrenElementList &noChildrenElementList);

      ParserPtr toParser() const   {return mThis.lock();}

      // helper methods
      static String convertFromEntities(const String &inString);

      static String makeTextEntitySafe(const String &inString, bool entityEncode0xD = false);
      static String makeAttributeEntitySafe(const String &inString, char willUseSurroundingQuotes = 0);

      static String convertFromJSONEncoding(const String &inString);
      static String convertToJSONEncoding(const String &inString);

    protected:
      Parser();
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Generator
    #pragma mark

    class Generator : public internal::Generator
    {
    public:
      enum XMLWriteFlags
      {
        XMLWriteFlag_None =                   0x00000000,
        XMLWriteFlag_ForceElementEndTag =     0x00000001,
        XMLWriteFlag_NormalizeCDATA =         0x00000002,
        XMLWriteFlag_EntityEncode0xDInText =  0x00000004, // requires WriteFlag_NormalizeCDATA is set
        XMLWriteFlag_NormizeAttributeValue =  0x00000008,
      };

    public:
      static GeneratorPtr createXMLGenerator(XMLWriteFlags writeFlags = XMLWriteFlag_None);
      static GeneratorPtr createJSONGenerator(
                                              const char *forcedText = ZS_JSON_DEFAULT_FORCED_TEXT,
                                              char attributePrefix = ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX
                                              );

      virtual ULONG getOutputSize(const NodePtr &onlyThisNode) const;
      virtual boost::shared_array<char> write(const NodePtr &onlyThisNode, ULONG *outLength = NULL) const;

      virtual GeneratorPtr toGenerator() const   {return mThis.lock();}

      virtual XMLWriteFlags getXMLWriteFlags() const;

    protected:
      Generator();
    };

  } // namespace XML

} // namespace zsLib

#pragma warning(pop)

#endif //ZSLIB_XML_H_58e5a7d8cf6414da70cf4cc573d53693
