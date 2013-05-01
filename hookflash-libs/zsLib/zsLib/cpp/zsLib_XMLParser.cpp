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

#pragma warning(push)
#pragma warning(disable: 4996)

#define ZS_INTERNAL_UTF8_MAX_CHARACTER_ENCODED_BYTE_SIZE (sizeof(BYTE)*6)

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

using namespace std;

namespace zsLib
{

  namespace XML
  {

    namespace internal
    {

      struct HTMLEntities
      {
        WCHAR mWChar;
        CSTR mEntityName;
      };

      static HTMLEntities gEntities[] =
      {
        {32,"nbsp"},
        {34,"quot"},
        {39,"apos"},
        {38,"amp"},
        {60,"lt"},
        {62,"gt"},

        {161,"iexcl"},
        {162,"cent"},
        {163,"pound"},
        {164,"curren"},
        {165,"yen"},
        {166,"brvbar"},
        {167,"sect"},
        {168,"uml"},
        {169,"copy"},
        {170,"ordf"},
        {171,"laquo"},
        {172,"not"},
        {173,"shy"},
        {174,"reg"},
        {175,"macr"},
        {176,"deg"},
        {177,"plusmn"},
        {178,"sup2"},
        {179,"sup3"},
        {180,"acute"},
        {181,"micro"},
        {182,"para"},
        {183,"middot"},
        {184,"cedil"},
        {185,"sup1"},
        {186,"ordm"},
        {187,"raquo"},
        {188,"frac14"},
        {189,"frac12"},
        {190,"frac34"},
        {191,"iquest"},
        {215,"times"},
        {247,"divide"},

        {192,"Agrave"},
        {193,"Aacute"},
        {194,"Acirc"},
        {195,"Atilde"},
        {196,"Auml"},
        {197,"Aring"},
        {198,"AElig"},
        {199,"Ccedil"},
        {200,"Egrave"},
        {201,"Eacute"},
        {202,"Ecirc"},
        {203,"Euml"},
        {204,"Igrave"},
        {205,"Iacute"},
        {206,"Icirc"},
        {207,"Iuml"},
        {208,"ETH"},
        {209,"Ntilde"},
        {210,"Ograve"},
        {211,"Oacute"},
        {212,"Ocirc"},
        {213,"Otilde"},
        {214,"Ouml"},
        {216,"Oslash"},
        {217,"Ugrave"},
        {218,"Uacute"},
        {219,"Ucirc"},
        {220,"Uuml"},
        {221,"Yacute"},
        {222,"THORN"},
        {223,"szlig"},
        {224,"agrave"},
        {225,"aacute"},
        {226,"acirc"},
        {227,"atilde"},
        {228,"auml"},
        {229,"aring"},
        {230,"aelig"},
        {231,"ccedil"},
        {232,"egrave"},
        {233,"eacute"},
        {234,"ecirc"},
        {235,"euml"},
        {236,"igrave"},
        {237,"iacute"},
        {238,"icirc"},
        {239,"iuml"},
        {240,"eth"},
        {241,"ntilde"},
        {242,"ograve"},
        {243,"oacute"},
        {244,"ocirc"},
        {245,"otilde"},
        {246,"ouml"},
        {248,"oslash"},
        {249,"ugrave"},
        {250,"uacute"},
        {251,"ucirc"},
        {252,"uuml"},
        {253,"yacute"},
        {254,"thorn"},
        {255,"yuml"},

        {8704,"forall"},
        {8706,"part"},
        {8707,"exists"},
        {8709,"empty"},
        {8711,"nabla"},
        {8712,"isin"},
        {8713,"notin"},
        {8715,"ni"},
        {8719,"prod"},
        {8721,"sum"},
        {8722,"minus"},
        {8727,"lowast"},
        {8730,"radic"},
        {8733,"prop"},
        {8734,"infin"},
        {8736,"ang"},
        {8743,"and"},
        {8744,"or"},
        {8745,"cap"},
        {8746,"cup"},
        {8747,"int"},
        {8756,"there4"},
        {8764,"sim"},
        {8773,"cong"},
        {8776,"asymp"},
        {8800,"ne"},
        {8801,"equiv"},
        {8804,"le"},
        {8805,"ge"},
        {8834,"sub"},
        {8835,"sup"},
        {8836,"nsub"},
        {8838,"sube"},
        {8839,"supe"},
        {8853,"oplus"},
        {8855,"otimes"},
        {8869,"perp"},
        {8901,"sdot"},

        {913,"Alpha"},
        {914,"Beta"},
        {915,"Gamma"},
        {916,"Delta"},
        {917,"Epsilon"},
        {918,"Zeta"},
        {919,"Eta"},
        {920,"Theta"},
        {921,"Iota"},
        {922,"Kappa"},
        {923,"Lambda"},
        {924,"Mu"},
        {925,"Nu"},
        {926,"Xi"},
        {927,"Omicron"},
        {928,"Pi"},
        {929,"Rho"},
        {931,"Sigma"},
        {932,"Tau"},
        {933,"Upsilon"},
        {934,"Phi"},
        {935,"Chi"},
        {936,"Psi"},
        {937,"Omega"},

        {945,"alpha"},
        {946,"beta"},
        {947,"gamma"},
        {948,"delta"},
        {949,"epsilon"},
        {950,"zeta"},
        {951,"eta"},
        {952,"theta"},
        {953,"iota"},
        {954,"kappa"},
        {955,"lambda"},
        {956,"mu"},
        {957,"nu"},
        {958,"xi"},
        {959,"omicron"},
        {960,"pi"},
        {961,"rho"},
        {962,"sigmaf"},
        {963,"sigma"},
        {964,"tau"},
        {965,"upsilon"},
        {966,"phi"},
        {967,"chi"},
        {968,"psi"},
        {969,"omega"},

        {977,"thetasym"},
        {978,"upsih"},
        {982,"piv"},

        {338,"OElig"},
        {339,"oelig"},
        {352,"Scaron"},
        {353,"scaron"},
        {376,"Yuml"},
        {402,"fnof"},
        {710,"circ"},
        {732,"tilde"},
        {8194,"ensp"},
        {8195,"emsp"},
        {8201,"thinsp"},
        {8204,"zwnj"},
        {8205,"zwj"},
        {8206,"lrm"},
        {8207,"rlm"},
        {8211,"ndash"},
        {8212,"mdash"},
        {8216,"lsquo"},
        {8217,"rsquo"},
        {8218,"sbquo"},
        {8220,"ldquo"},
        {8221,"rdquo"},
        {8222,"bdquo"},
        {8224,"dagger"},
        {8225,"Dagger"},
        {8226,"bull"},
        {8230,"hellip"},
        {8240,"permil"},
        {8242,"prime"},
        {8243,"Prime"},
        {8249,"lsaquo"},
        {8250,"rsaquo"},
        {8254,"oline"},
        {8364,"euro"},
        {8482,"trade"},
        {8592,"larr"},
        {8593,"uarr"},
        {8594,"rarr"},
        {8595,"darr"},
        {8596,"harr"},
        {8629,"crarr"},
        {8968,"lceil"},
        {8969,"rceil"},
        {8970,"lfloor"},
        {8971,"rfloor"},
        {9674,"loz"},
        {9824,"spades"},
        {9827,"clubs"},
        {9829,"hearts"},
        {9830,"diams"},

        {NULL, NULL}
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark XML::internal::Parser::AutoStack
      #pragma mark

      //-----------------------------------------------------------------------
      Parser::AutoStack::AutoStack(const XML::ParserPos &inPos) :
        mParser(inPos.getParser())
      {
        if (mParser)
          mParser->pushPos(inPos);
      }

      //-----------------------------------------------------------------------
      Parser::AutoStack::~AutoStack()
      {
        if (mParser)
          mParser->popPos();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark XML::internal::Parser
      #pragma mark

      //-----------------------------------------------------------------------
      Parser::Parser() :
        mParserMode(ParserMode_XML),
        mTabSize(ZS_INTERNAL_XML_DEFAULT_TAB_SIZE),
        mSOF(NULL),
        mEnableWarnings(true),
        mJSONForcedText(ZS_JSON_DEFAULT_FORCED_TEXT),
        mJSONAttributePrefix(ZS_JSON_DEFAULT_ATTRIBUTE_PREFIX)
      {
      }

      //-----------------------------------------------------------------------
      void Parser::clearStack()
      {
        mParserStack.clear();
      }

      //-----------------------------------------------------------------------
      void Parser::pushPos(const XML::ParserPos &inPos)
      {
        mParserStack.push_back(inPos);
      }

      //-----------------------------------------------------------------------
      XML::ParserPos Parser::popPos()
      {
        ZS_THROW_BAD_STATE_IF(mParserStack.size() < 1)

        XML::ParserPos temp;
        temp = mParserStack.back();
        mParserStack.pop_back();
        return temp;
      }

      //-----------------------------------------------------------------------
      void Parser::addWarning(ParserWarningTypes inWarning)
      {
        if (mEnableWarnings)
        {
          XML::ParserWarning warning(inWarning, mParserStack);
          mWarnings.push_back(warning);
        }
      }

      //-----------------------------------------------------------------------
      void Parser::addWarning(ParserWarningTypes inWarning, const XML::ParserPos &inPos)
      {
        if (mEnableWarnings)
        {
          pushPos(inPos);
          addWarning(inWarning);
          popPos();
        }
      }

      //-----------------------------------------------------------------------
      bool Parser::isAlpha(char inLetter)
      {
        if (inLetter < 0)
          return false;
        return 0 != isalpha(inLetter);
      }

      //-----------------------------------------------------------------------
      bool Parser::isDigit(char inLetter)
      {
        if (inLetter < 0)
          return false;
        return 0 != isdigit(inLetter);
      }

      //-----------------------------------------------------------------------
      bool Parser::isAlphaNumeric(char inLetter)
      {
        if (inLetter < 0)
          return false;
        return 0 != isalnum(inLetter);
      }

      //-----------------------------------------------------------------------
      bool Parser::isHexDigit(char inLetter)
      {
        return (((inLetter >= 'A') && (inLetter <= 'F')) ||
                ((inLetter >= 'a') && (inLetter <= 'f')) ||
                (isDigit(inLetter)));
      }

      //-----------------------------------------------------------------------
      bool Parser::isLegalName(char inLetter, bool inFirstLetter)
      {
        BYTE letter = (BYTE)inLetter;
        if (letter >= 127)
          return true;

        if (inFirstLetter)
          return isAlpha(inLetter) || ('_' == inLetter);

        switch (inLetter)
        {
          case '_':
          case '-':
          case ':':
          case '.':
          {
            return true;
          }
        }

        return isAlphaNumeric(inLetter);
      }

      //-----------------------------------------------------------------------
      WCHAR Parser::findEntity(const String &inLookup)
      {
        for (ULONG loop = 0; NULL != internal::gEntities[loop].mEntityName; ++loop)
        {
          if (inLookup == gEntities[loop].mEntityName)
            return gEntities[loop].mWChar;
        }
        return 0;
      }

      //-----------------------------------------------------------------------
      bool Parser::isWhiteSpace(char inLetter)
      {
        switch (inLetter)
        {
          case ' ':
          case '\v':
          case '\f':
          case '\t':
          case '\r':
          case '\n':
          {
            return true;
          }
        }
        return false;
      }

      //-----------------------------------------------------------------------
      bool Parser::isWhiteSpace(const XML::ParserPos &inPos)
      {
        return isWhiteSpace(*inPos);
      }

      //-----------------------------------------------------------------------
      bool Parser::skipWhiteSpace(XML::ParserPos &inPos)
      {
        bool skipped = false;
        while (isWhiteSpace(inPos))
        {
          ++inPos;
          skipped = true;
        }
        return skipped;
      }

      //-----------------------------------------------------------------------
      bool Parser::parseAnyExceptElement(XML::ParserPos &ioPos, NodePtr parent)
      {
        AutoStack stack(ioPos);

        if ('<' == *ioPos)
        {
          // this is some kind of entity, which kind?
          if (ioPos.isString("</"))
          {
            // this is an end of element tag, do not add to the parsing

            // before the end tag is parsed, ensure the end tag matches a parent
            XML::ParserPos temp = (ioPos + strlen("</"));
            skipWhiteSpace(temp);
            String elementName = parseLegalName(temp);

            if (elementName.isEmpty())
            {
              if (*temp == '>')
              {
                // this is a generic end tag, allow caller to process it
                return false;
              }

              if (temp.isEOF())
              {
                (temp.getParser())->addWarning(ParserWarningType_NoEndBracketFound, ioPos);
                return false;
              }

              // this is a wonky end tag, skip it
              skipMismatchedEndTag(ioPos);
              return true;
            }

            // try to find a parent element that has this element name
            bool caseSensative = (ioPos.getDocument())->isElementNameIsCaseSensative();
            for (NodePtr check = parent; check; check = check->getParent())
            {
              if (check->isElement())
              {
                ElementPtr element = check->toElement();
                String name = element->getValue();
                if (caseSensative)
                {
                  if (name == elementName)
                    return false;
                }
                else
                {
                  if (0 == name.compareNoCase(elementName))
                    return false;
                }
              }
            }

            // this is a mismatched end tag, skip it since the parent was not found
            skipMismatchedEndTag(ioPos);
            return true;
          }
          if (ioPos.isString("<!--"))
          {
            CommentPtr comment = XML::Comment::create();
            parent->adoptAsLastChild(comment);
            comment->parse(ioPos);
            return true;
          }
          if (ioPos.isString("<![CDATA["))
          {
            TextPtr text = XML::Text::create();
            parent->adoptAsLastChild(text);
            text->parse(ioPos);
            return true;
          }
          if (ioPos.isString("<?xml", false))
          {
            DeclarationPtr declaration = XML::Declaration::create();
            parent->adoptAsLastChild(declaration);
            declaration->parse(ioPos);
            return true;
          }
          if (ioPos.isString("<?"))
          {
            UnknownPtr unknown = XML::Unknown::create();
            parent->adoptAsLastChild(unknown);
            unknown->parse(ioPos, "<?", "?>");
            return true;
          }
          if (ioPos.isString("<!"))
          {
            UnknownPtr unknown = XML::Unknown::create();
            parent->adoptAsLastChild(unknown);
            unknown->parse(ioPos);
            return true;
          }
          if (isLegalName(*(ioPos+1), true))
          {
            // this is an element
            return false;
          }

          // what is this? who knows, parse it as unknown element
          UnknownPtr unknown = XML::Unknown::create();
          parent->adoptAsLastChild(unknown);
          unknown->parse(ioPos);
          return true;
        }

        if (ioPos.isEOF())
          return true;

        // this is text, parse it
        TextPtr text = XML::Text::create();
        parent->adoptAsLastChild(text);
        text->parse(ioPos);
        return true;
      }

      //-----------------------------------------------------------------------
      String Parser::parseLegalName(XML::ParserPos &inPos)
      {
        String result;
        while (isLegalName(*inPos, result.isEmpty()))
        {
          result += *inPos;
          ++inPos;
        }
        return result;
      }

      //-----------------------------------------------------------------------
      bool Parser::skipMismatchedEndTag(XML::ParserPos &ioPos)
      {
        // this is an end of element, except there is no matching element
        AutoStack stack(ioPos);

        XML::ParserPos start = ioPos;
        while (*ioPos)
        {
          if ('<' == *ioPos)
          {
            if (ioPos == start)
              ++ioPos;
            else
              break;
          }
          else if ('>' == *ioPos)
          {
            ++ioPos;
            break;
          }
          ++ioPos;
        }

        (ioPos.getParser())->addWarning(ParserWarningType_MismatchedEndTag);
        return start != ioPos;
      }

      //-----------------------------------------------------------------------
      String Parser::compressWhiteSpace(const String &inString)
      {
        boost::shared_array<char> temp(new char[inString.getLength()+1]);

        bool lastWasWhiteSpace = false;

        char *dest = temp.get();
        const char *source = inString;
        while (*source)
        {
          if (isWhiteSpace(*source))
          {
            ++source;
            lastWasWhiteSpace = true;
            continue;
          }

          if (lastWasWhiteSpace)
          {
            if (dest != temp.get())
            {
              // only add the space if not the first position
              *dest = ' ';
              ++dest;
            }

            lastWasWhiteSpace = false;
            continue;
          }

          lastWasWhiteSpace = false;
          *dest = *source;
          ++dest;
          ++source;
        }
        *dest = 0;

        return String((CSTR)temp.get());
      }

      //-----------------------------------------------------------------------
      NodePtr Parser::cloneAssignParent(NodePtr inNewParent, NodePtr inExistingChild)
      {
        if (!inExistingChild)
          return NodePtr();

        switch (inExistingChild->getNodeType())
        {
          case XML::Node::NodeType::Document:      return inExistingChild->toDocument()->cloneAssignParent(inNewParent);
          case XML::Node::NodeType::Element:       return inExistingChild->toElement()->cloneAssignParent(inNewParent);
          case XML::Node::NodeType::Attribute:     return inExistingChild->toAttribute()->cloneAssignParent(inNewParent);
          case XML::Node::NodeType::Text:          return inExistingChild->toText()->cloneAssignParent(inNewParent);
          case XML::Node::NodeType::Comment:       return inExistingChild->toComment()->cloneAssignParent(inNewParent);
          case XML::Node::NodeType::Declaration:   return inExistingChild->toDeclaration()->cloneAssignParent(inNewParent);
          case XML::Node::NodeType::Unknown:       return inExistingChild->toUnknown()->cloneAssignParent(inNewParent);
          default:
          {
            ZS_THROW_BAD_STATE("missing node type in cloneAssignParent table")
          }
        }
        return NodePtr();
      }

      //-----------------------------------------------------------------------
      void Parser::safeAdoptAsLastChild(NodePtr inParent, NodePtr inNewChild)
      {
        if (!inParent)
          return;

        ZS_THROW_BAD_STATE_IF(!inNewChild)

        inParent->adoptAsLastChild(inNewChild);
      }

      //-----------------------------------------------------------------------
      String Parser::parseJSONString(XML::ParserPos &ioPos)
      {
        AutoStack stack(ioPos);

        if (*ioPos == '\"') {
          ++ioPos;
        } else {
          (ioPos.getParser())->addWarning(ParserWarningType_MissingStringQuotes, ioPos);
        }

        XML::ParserPos startPos = ioPos;

        while ((*ioPos) &&
               ('\"' != *ioPos)) {

          if ('\\' != *ioPos) {
            ++ioPos;
            continue;
          }

          XML::ParserPos escapePos = ioPos;

          ++ioPos;
          char escapeChar = *ioPos;
          ++ioPos;

          switch (escapeChar) {
            case 'u': {
              ULONG digits = 4;
              while (0 != digits) {
                if (!isHexDigit(*ioPos)) {
                  (ioPos.getParser())->addWarning(ParserWarningType_InvalidUnicodeEscapeSequence, escapePos);
                  break;
                }
                --digits;
                ++ioPos;
              }
              break;
            }
            case '\"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't': {
              // this is a legal escape sequence...
              break;
            }
            default: {
              (ioPos.getParser())->addWarning(ParserWarningType_InvalidEscapeSequence, escapePos);
              break;
            }
          }
        }

        const char *start = (CSTR)startPos;
        const char *end = (CSTR)ioPos;

        if (*ioPos == '\"') {
          ++ioPos;
        } else {
          (ioPos.getParser())->addWarning(ParserWarningType_MissingStringQuotes, ioPos);
        }

        size_t length = (end - start);

        return std::string(start, length);
      }

      //-----------------------------------------------------------------------
      String Parser::parseJSONNumber(XML::ParserPos &ioPos)
      {
        AutoStack stack(ioPos);

        XML::ParserPos startPos = ioPos;

        // scope: parse number
        {
          if ('-' == *ioPos) {
            ++ioPos;
          }
          if ('0' == *ioPos) {
            ++ioPos;
            if (isDigit(*ioPos)) {
              (ioPos.getParser())->addWarning(ParserWarningType_IllegalNumberSequence, ioPos);
              while (isDigit(*ioPos)) {
                ++ioPos;
              }
            }
          } else {
            if (!isDigit(*ioPos)) {
              (ioPos.getParser())->addWarning(ParserWarningType_IllegalNumberSequence, ioPos);
              goto done_parseJSONNumber;
            }
            while (isDigit(*ioPos)) {
              ++ioPos;
            }
          }

          if ('.' == *ioPos) {
            ++ioPos;
            if (!isDigit(*ioPos)) {
              (ioPos.getParser())->addWarning(ParserWarningType_IllegalNumberSequence, ioPos);
              goto done_parseJSONNumber;
            }
            while (isDigit(*ioPos)) {
              ++ioPos;
            }
          }

          if (('e' == *ioPos) ||
              ('E' == *ioPos)) {
            ++ioPos;
            if (('-' == *ioPos) || ('+' == *ioPos)) {
              ++ioPos;
            }
            if (!isDigit(*ioPos)) {
              (ioPos.getParser())->addWarning(ParserWarningType_IllegalNumberSequence, ioPos);
              goto done_parseJSONNumber;
            }
            while (isDigit(*ioPos)) {
              ++ioPos;
            }
          }
        }

      done_parseJSONNumber:

        const char *start = (CSTR)startPos;
        const char *end = (CSTR)ioPos;

        size_t length = (end - start);

        return std::string(start, length);
      }

      //-----------------------------------------------------------------------
      bool Parser::parseSimpleJSONValue(
                                        XML::ParserPos &ioPos,
                                        String &outResult,
                                        bool &outIsQuoted
                                        )
      {
        outIsQuoted = false;
        switch (*ioPos) {
          case '\"': outResult = parseJSONString(ioPos); outIsQuoted = true; return true;
          case '[': return false;
          case '{': return false;
          case '-': outResult = parseJSONNumber(ioPos); return true;
          default:  break;
        }
        if (isDigit(*ioPos)) {
          outResult = parseJSONNumber(ioPos);
          outIsQuoted = false;
          return true;
        }
        if (ioPos.isString("true")) {
          ioPos += strlen("true");
          outResult = "true";
          outIsQuoted = false;
          return true;
        }
        if (ioPos.isString("false")) {
          ioPos += strlen("false");
          outResult = "false";
          outIsQuoted = false;
          return true;
        }
        if (ioPos.isString("null")) {
          ioPos += strlen("null");
          outResult = "null";
          outIsQuoted = false;
          return true;
        }
        return false;
      }

    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::Parser
    #pragma mark

    //-------------------------------------------------------------------------
    ParserPtr Parser::createXMLParser()
    {
      ParserPtr pThis(new Parser);
      pThis->mThis = pThis;
      pThis->mParserMode = ParserMode_XML;
      return pThis;
    }

    //-------------------------------------------------------------------------
    ParserPtr Parser::createJSONParser(
                                       const char *forcedText,
                                       char attributePrefix
                                       )
    {
      ParserPtr pThis(new Parser);
      pThis->mThis = pThis;
      pThis->mParserMode = ParserMode_JSON;
      pThis->mJSONForcedText = (forcedText ? forcedText : "");
      pThis->mJSONAttributePrefix = attributePrefix;
      return pThis;
    }

    ParserPtr Parser::createAutoDetectParser(
                                             const char *jsonForcedText,
                                             char jsonAttributePrefix
                                             )
    {
      ParserPtr pThis(new Parser);
      pThis->mThis = pThis;
      pThis->mParserMode = ParserMode_AutoDetect;
      pThis->mJSONForcedText = (jsonForcedText ? jsonForcedText : "");
      pThis->mJSONAttributePrefix = jsonAttributePrefix;
      return pThis;
    }

    //-------------------------------------------------------------------------
    Parser::Parser() :
      internal::Parser()
    {
    }

    enum JSONParserStackTypes
    {
      JSONParserStackType_Object,
      JSONParserStackType_Array,
    };

    struct JSONStackEntry
    {
      ParserPos mStartPos;
      JSONParserStackTypes mStackType;
      ElementPtr mElement;
    };

    //-------------------------------------------------------------------------
    DocumentPtr Parser::parse(
                              const char *inDocument,
                              bool inElementNameIsCaseSensative,
                              bool inAttributeNameIsCaseSensative
                              )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!inDocument)

      DocumentPtr outDocument = Document::create(inElementNameIsCaseSensative, inAttributeNameIsCaseSensative);

      mSOF = inDocument;
      ParserPos pos(*this, *(outDocument.get()));

      clearStack();
      clearWarnings();

      if (ParserMode_AutoDetect == mParserMode) {
        mParserMode = ParserMode_XML;
        ParserPos lookAheadPos(pos);
        while (*lookAheadPos) {
          if (Parser::isWhiteSpace(*lookAheadPos)) {
            ++lookAheadPos;
            continue;
          }
          if (('{' == *lookAheadPos) ||
              ('[' == *lookAheadPos)) {
            mParserMode = ParserMode_JSON;
            break;
          }
          mParserMode = ParserMode_XML;
          break;
        }
      }

      switch (mParserMode)
      {
        case ParserMode_AutoDetect: {
          ZS_THROW_BAD_STATE("auto detect is an illegal parser mode")
        }
        //.....................................................................
        case ParserMode_XML: {
          while (*pos)
          {
            if (Parser::parseAnyExceptElement(pos, outDocument))
              continue;

            // check if this is an element
            if (Parser::isLegalName(*(pos+1), true))
            {
              // this is an element
              ElementPtr element = XML::Element::create();
              outDocument->adoptAsLastChild(element);
              element->parse(pos);
              continue;
            }

            Parser::skipMismatchedEndTag(pos);
          }
          break;
        }
        //.....................................................................
        case ParserMode_JSON: {

          typedef std::list<JSONStackEntry> JSONStack;

          JSONStack jsonStack;

          while (*pos) {
            skipWhiteSpace(pos);
            if (('{' != *pos) &&
                ('[' != *pos)) {
              addWarning(ParserWarningType_MustOpenWithObject, pos);
              break;
            }

            AutoStack entry(pos);

            char openChar = (*pos);

            ++pos;

            skipWhiteSpace(pos);

            ElementPtr currentEl;
            JSONParserStackTypes currentType = ('{' == openChar ? JSONParserStackType_Object: JSONParserStackType_Array);
            JSONParserStackTypes rootType = currentType;

            if (JSONParserStackType_Array == currentType) {
              currentEl = Element::create();
              outDocument->adoptAsFirstChild(currentEl);
            }

            const char *lastPos = NULL;

            bool legalComma = false;

            while (*pos)
            {
              if (skipWhiteSpace(pos)) {
                continue;
              }

              char endChar = (currentType == JSONParserStackType_Object ? '}' : ']');
              if (endChar == *pos) {
                if (jsonStack.size() < 1) {
                  if (JSONParserStackType_Array == currentType) {
                    if (!currentEl->hasChildren()) {
                      // there was no values contained within this element
                      currentEl->orphan();
                    }
                  }
                  break;
                }
                ++pos;

                skipWhiteSpace(pos);

                JSONStackEntry &entry = jsonStack.back();
                currentType = entry.mStackType;
                currentEl = entry.mElement;
                jsonStack.pop_back();
                popPos();

                // can now exact a comma after
                legalComma = true;
                continue;
              }

              if (',' == *pos) {
                if (!legalComma) {
                  addWarning(ParserWarningType_UnexpectedComma, pos);
                }
                ++pos;

                legalComma = false;

                if (JSONParserStackType_Array == currentType) {
                  if (currentEl) {
                    // this is the next element in the array so needs a new element to hold the value
                    ElementPtr el = Element::create(currentEl->getValue());
                    currentEl->adoptAsNextSibling(el);
                    currentEl = el;
                  }
                }
                continue;
              }

              AutoStack stack(pos);

              if (((CSTR)pos) == lastPos) {
                addWarning(ParserWarningType_ParserStuck, pos);
                break;
              }
              lastPos = ((CSTR)pos);

              String pairFirst;

              if (JSONParserStackType_Object == currentType) {
                // this must be a JSON "member" inside an object thus must have a pair string
                pairFirst = convertFromJSONEncoding(parseJSONString(pos));

                skipWhiteSpace(pos);
                if (!*pos) continue;  // end of file found

                if (':' == *pos) {
                  ++pos;
                } else {
                  addWarning(ParserWarningType_MissingColonBetweenStringAndValue, pos);
                }

                skipWhiteSpace(pos);

                if (!*pos) continue;  // end of file found
              }

              bool isQuoted = false;
              String pairSecond;
              if (parseSimpleJSONValue(pos, pairSecond, isQuoted)) {

                // now can expect a comma
                legalComma = true;

                TextPtr text = Text::create();
                text->setValue(pairSecond, isQuoted ? Text::Format_JSONStringEncoded : Text::Format_JSONNumberEncoded);

                if (JSONParserStackType_Object == currentType) {

                  // this is a JSON object...
                  if (pairFirst.length() < 1) {
                    addWarning(ParserWarningType_MissingPairString);
                    continue;
                  }

                  if (pairFirst == mJSONForcedText) {
                    if (currentEl) {
                      currentEl->adoptAsLastChild(text);
                    } else {
                      outDocument->adoptAsLastChild(text);
                    }
                  } else if ((*(pairFirst.c_str())) == mJSONAttributePrefix) {
                    if (currentEl) {
                      String actualName = (pairFirst.c_str()+1);
                      if (actualName.length() > 0) {
                        currentEl->setAttribute(actualName, makeAttributeEntitySafe(convertFromJSONEncoding(pairSecond)), isQuoted);
                      } else {
                        addWarning(ParserWarningType_AttributePrefixWithoutName);
                      }
                    } else {
                      addWarning(ParserWarningType_AttributePrefixAtRoot);
                    }
                  } else {
                    // this is an element which contains a single value
                    ElementPtr singleValueElement = Element::create(pairFirst);
                    singleValueElement->adoptAsLastChild(text);
                    if (currentEl) {
                      currentEl->adoptAsLastChild(singleValueElement);
                    } else {
                      outDocument->adoptAsLastChild(singleValueElement);
                    }
                  }
                } else {
                  // this is XML array ...
                  if (currentEl) {
                    currentEl->adoptAsLastChild(text);
                  } else {
                    addWarning(ParserWarningType_IllegalArrayAtRoot, pos);
                  }
                }

              } else {

                JSONParserStackTypes nextType = JSONParserStackType_Object;
                ParserPos pushPointPos(pos);

                switch (*pos) {
                  case '{': {
                    ++pos;
                    skipWhiteSpace(pos);
                    nextType = JSONParserStackType_Object;
                    break;
                  }
                  case '[': {
                    ++pos;
                    skipWhiteSpace(pos);
                    nextType = JSONParserStackType_Array;
                    break;
                  }
                  default: {
                    addWarning(ParserWarningType_IllegalValue, pos);
                    ++pos;

                    while (*pos) {
                      if (skipWhiteSpace(pos)) {
                        continue;
                      }
                      switch (*pos) {
                        case '\"':
                        case '[':
                        case ']':
                        case '{':
                        case '}':
                        case ',':
                        case ':':
                          goto skipUntilSomethingUseful;
                        default:  break;
                      }
                      ++pos;
                    }

                  skipUntilSomethingUseful:

                    continue;
                  }
                }

                ElementPtr adoptedChildEl;
                switch (currentType) {
                  case JSONParserStackType_Object:
                  {
                    adoptedChildEl = Element::create(pairFirst);
                    break;
                  }
                  case JSONParserStackType_Array:
                  {
                    if (!currentEl) {
                      addWarning(ParserWarningType_IllegalArrayAtRoot, pos);
                      continue;
                    }
                    switch (nextType) {
                      case JSONParserStackType_Object:
                      {
                        // any object inside an array will create a sub element inside the current object
                        break;
                      }
                      case JSONParserStackType_Array:
                      {
                        if (!currentEl) {
                          addWarning(ParserWarningType_IllegalArrayAtRoot, pos);
                          continue;
                        }
                        // array inside an array, base name of this array off parent array element name
                        adoptedChildEl = Element::create(currentEl->getValue());
                        break;
                      }
                    }
                  }
                }

                // this is either an object or an array since it didn't parse as a simple value
                JSONStackEntry newEntry;
                newEntry.mStartPos = pushPointPos;
                newEntry.mStackType = currentType;
                newEntry.mElement = currentEl;
                jsonStack.push_back(newEntry);
                pushPos(pushPointPos);

                // change the current type and current element as required

                if (adoptedChildEl) {
                  if (currentEl) {
                    currentEl->adoptAsLastChild(adoptedChildEl);
                  } else {
                    outDocument->adoptAsLastChild(adoptedChildEl);
                  }
                  currentEl = adoptedChildEl;
                }

                currentType = nextType;
              }

              // skip space to next pair or value
              skipWhiteSpace(pos);
            }

            if (jsonStack.size() > 0) {
              JSONStackEntry &entry = jsonStack.back();
              addWarning(ParserWarningType_MissingObjectClose, entry.mStartPos);
            }

            if (JSONParserStackType_Array == rootType) {
              if (']' != *pos) {
                addWarning(ParserWarningType_MustCloseRootObject, pos);
                break;
              }
            } else {
              if ('}' != *pos) {
                addWarning(ParserWarningType_MustCloseRootObject, pos);
                break;
              }
            }
            ++pos;

            skipWhiteSpace(pos);
            if (*pos) {
              addWarning(ParserWarningType_DataFoundAfterFinalObjectClose, pos);
              break;
            }
          }
        }
      }

      clearStack();
      return outDocument;
    }

    //-------------------------------------------------------------------------
    void Parser::clearWarnings()
    {
      mWarnings.clear();
    }

    //-------------------------------------------------------------------------
    const Parser::Warnings &Parser::getWarnings() const
    {
      return mWarnings;
    }

    //-------------------------------------------------------------------------
    ULONG Parser::getTabSize() const
    {
      return mTabSize;
    }

    //-------------------------------------------------------------------------
    void Parser::setTabSize(ULONG inTabSize)
    {
      mTabSize = inTabSize;
    }

    //-------------------------------------------------------------------------
    void Parser::setNoChildrenElements(const NoChildrenElementList &noChildrenElementList)
    {
      mSingleElements = noChildrenElementList;
    }

    //-------------------------------------------------------------------------
    String Parser::convertFromEntities(const String &inString)
    {
      boost::shared_array<char> result(new char[inString.getLength()*ZS_INTERNAL_UTF8_MAX_CHARACTER_ENCODED_BYTE_SIZE+1]);
      memset(result.get(), 0, sizeof(char)*(inString.getLength()*ZS_INTERNAL_UTF8_MAX_CHARACTER_ENCODED_BYTE_SIZE+1));

      char *dest = result.get();

      const char *pos = inString;
      while (*pos)
      {
        if ('&' == *pos)
        {
          const char *start = pos;
          ++pos;

          bool isDecimal = false;
          bool isHex = false;
          if ('#' == *pos)
          {
            isDecimal = true;
            ++pos;
            if ('x' == tolower(*pos))
            {
              isHex = true;
              ++pos;
            }
          }

          String lookup;
          while ((*pos != ';') && (*pos))
          {
            if (isDecimal)
            {
              if (isHex)
              {
                if (!isHexDigit(*pos))
                  break;
              }
              else if (!isDigit(*pos))
                break;
            }
            else if (!((isAlpha(*pos)) || (isDigit(*pos))))
              break;

            lookup += *pos;
            ++pos;
          }

          if ((';' == *pos) &&
              (!lookup.isEmpty()))
          {
            ++pos;

            WCHAR newLetter = 0;
            if (isDecimal)
            {
              ULONG number = 0;
              try {
                Numeric<ULONG> numeric(lookup, false, isHex ? 16 : 10);
                number = (ULONG)numeric;
              } catch (Numeric<ULONG>::ValueOutOfRange &) {
              }
              newLetter = (WCHAR)number;
            }
            else
              newLetter = findEntity(lookup);

            if (0 != newLetter)
            {
              if ((newLetter > 127) || (newLetter < 0))
              {
                WCHAR letArray[2];
                letArray[0] = newLetter;
                letArray[1] = 0;
                String temp((CWSTR)&(letArray[0]));
                memcpy(dest, (CSTR)temp, temp.getLength()*sizeof(char));

                dest += temp.getLength();
                continue;
              }

              // utf-8 safe
              *dest = (char)newLetter;
              ++dest;
              continue;
            }
          }

          // this is not a valid entity, just add it as a regular string
          pos = start;

          *dest = *pos;
          ++dest;
          ++pos;
          continue;
        }

        // this is not an entity
        *dest = *pos;
        ++dest;
        ++pos;
      }
      return String((CSTR)result.get());
    }

    // &amp;
    // &lt;
    // &gt;
    // &quot;
    // &apos;

#define ZS_INTERNAL_LONGEST_MANDITORY_ENTITY "&quot;"

    //-------------------------------------------------------------------------
    String Parser::makeTextEntitySafe(const String &inString, bool entityEncode0xD)
    {
      boost::shared_array<char>buffer(new char[(inString.getLength()*strlen(ZS_INTERNAL_LONGEST_MANDITORY_ENTITY))+1]);

      const char *source = inString;
      char *dest = buffer.get();
      while (*source)
      {
        switch (*source)
        {
          case '&':
          {
            strcpy(dest, "&amp;");
            dest += strlen("&amp;");
            break;
          }
          case '<':
          {
            strcpy(dest, "&lt;");
            dest += strlen("&lt;");
            break;
          }
          case '>':
          {
            strcpy(dest, "&gt;");
            dest += strlen("&gt;");
            break;
          }
          case '\xD':
          {
            if (entityEncode0xD) {
              strcpy(dest, "&#xD;");
              dest += strlen("&#xD;");
            } else {
              *dest = *source;
              ++dest;
            }
            break;
          }
          default:
          {
            *dest = *source;
            ++dest;
            break;
          }
        }
        ++source;
      }
      *dest = 0;

      return String((CSTR)buffer.get());
    }

    //-------------------------------------------------------------------------
    String Parser::makeAttributeEntitySafe(const String &inString, char willUseSurroundingQuotes)
    {
      bool escapeDoubleQuote = false;
      bool escapeSingleQuote = false;

      if (0 != willUseSurroundingQuotes) {
        if (willUseSurroundingQuotes == '\"') {
          escapeDoubleQuote = true;
        } else if (willUseSurroundingQuotes == '\'') {
          escapeSingleQuote = true;
        } else {
          ZS_THROW_INVALID_USAGE("Must specify \' or \" only surrounding attribute")
        }
      } else {
        if (String::npos != inString.find('\"'))
        {
          // found double quote, check for single quote
          if (String::npos != inString.find("\'"))
          {
            // found single quote, have to escape both
            escapeDoubleQuote = escapeSingleQuote = true;
          }
          else
          {
            // did not find single quote, so could encode attribute with single quote
          }
        }
      }

      boost::shared_array<char> buffer(new char[(inString.getLength()*strlen(ZS_INTERNAL_LONGEST_MANDITORY_ENTITY))+1]);

      const char *source = inString;
      char *dest = buffer.get();
      while (*source)
      {
        bool doDefault = false;
        switch (*source)
        {
          case '&':
          {
            strcpy(dest, "&amp;");
            dest += strlen("&amp;");
            break;
          }
          case '<':
          {
            strcpy(dest, "&lt;");
            dest += strlen("&lt;");
            break;
          }
          case '>':
          {
            strcpy(dest, "&gt;");
            dest += strlen("&gt;");
            break;
          }
          case '\"':
          {
            if (escapeDoubleQuote)
            {
              strcpy(dest, "&quot;");
              dest += strlen("&quot;");
            }
            else
              doDefault = true;
            break;
          }
          case '\'':
          {
            if (escapeSingleQuote)
            {
              strcpy(dest, "&apos;");
              dest += strlen("&apos;");
            }
            else
              doDefault = true;
            break;
          }
          case '\x9':
          {
            strcpy(dest, "&#x9;");
            dest += strlen("&#x9;");
            break;
          }
          case '\xA':
          {
            strcpy(dest, "&#xA;");
            dest += strlen("&#xA;");
            break;
          }
          case '\xD':
          {
            strcpy(dest, "&#xD;");
            dest += strlen("&#xD;");
            break;
          }
          default:
          {
            doDefault = true;
            break;
          }
        }
        if (doDefault)
        {
          *dest = *source;
          ++dest;
        }
        ++source;
      }
      *dest = 0;

      return String((CSTR)buffer.get());
    }

    //-------------------------------------------------------------------------
    String Parser::convertFromJSONEncoding(const String &inString)
    {
      boost::shared_array<char> temp(new char[inString.getLength()+1]);

      const char *source = inString.c_str();
      char *dest = temp.get();

      while (*source)
      {
        if ('\\' != *source) {
          *dest = *source;
          ++source;
          ++dest;
          continue;
        }

        ++source;
        switch (*source) {
          case '\"': *dest = '\"'; ++dest; ++source; break;
          case '\\': *dest = '\\'; ++dest; ++source; break;
          case '/': *dest = '/'; ++dest; ++source; break;
          case 'b': *dest = '\b'; ++dest; ++source; break;
          case 'f': *dest = '\f'; ++dest; ++source; break;
          case 'n': *dest = '\n'; ++dest; ++source; break;
          case 'r': *dest = '\r'; ++dest; ++source; break;
          case 't': *dest = '\t'; ++dest; ++source; break;
          case 'u': {
            ++source;

            std::string hexDigits(source, 4);
            if (hexDigits.length() < 4) {
              *dest = '\\';
              ++dest;
              --source;
              continue;
            }

            try {
              WCHAR unicodeValue[2];
              unicodeValue[0] = Numeric<WORD>(hexDigits, false, 16);
              unicodeValue[1] = 0;

              String utf8((CWSTR) (&(unicodeValue[0])) );

              memcpy(dest, utf8.c_str(), utf8.length() * sizeof(char));
              dest += utf8.length();

              source += 4;

            } catch(Numeric<WORD>::ValueOutOfRange &) {
              *dest = '\\';
              ++dest;
              --source;
              continue;
            }
            break;
          }
          default:  {
            *dest = '\\';
            ++dest;
            continue;
          }
        }
      }

      *dest = 0;

      return temp.get();
    }

    //-------------------------------------------------------------------------
    String Parser::convertToJSONEncoding(const String &inString)
    {
      boost::shared_array<char> temp(new char[(inString.getLength()*2)+1]);

      const char *source = inString.c_str();
      char *dest = temp.get();

      while (*source)
      {
        switch (*source) {
          case '\"': *dest = '\\'; ++dest; *dest = '\"'; ++dest; break;
          case '\\': *dest = '\\'; ++dest; *dest = '\\'; ++dest; break;
          case '\b': *dest = '\\'; ++dest; *dest = 'b'; ++dest; break;
          case '\f': *dest = '\\'; ++dest; *dest = 'f'; ++dest; break;
          case '\n': *dest = '\\'; ++dest; *dest = 'n'; ++dest; break;
          case '\r': *dest = '\\'; ++dest; *dest = 'r'; ++dest; break;
          case '\t': *dest = '\\'; ++dest; *dest = 't'; ++dest; break;
          default: {
            *dest = *source;
            ++dest;
          }
        }
        ++source;
      }

      *dest = 0;

      return temp.get();
    }

  } // namespace XML

} // namespace zsLib

#pragma warning(pop)
