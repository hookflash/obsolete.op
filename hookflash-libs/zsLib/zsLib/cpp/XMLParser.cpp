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
#include <zsLib/Numeric.h>

#pragma warning(push)
#pragma warning(disable: 4996)

#define ZS_INTERNAL_UTF8_MAX_CHARACTER_ENCODED_BYTE_SIZE (sizeof(BYTE)*6)

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

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

      Parser::AutoStack::AutoStack(const XML::ParserPos &inPos) :
      mDocument(inPos.getDocument())
      {
        if (mDocument)
          mDocument->pushPos(inPos);
      }

      Parser::AutoStack::~AutoStack()
      {
        if (mDocument)
          mDocument->popPos();
      }

      bool Parser::isAlpha(char inLetter)
      {
        if (inLetter < 0)
          return false;
        return 0 != isalpha(inLetter);
      }

      bool Parser::isDigit(char inLetter)
      {
        if (inLetter < 0)
          return false;
        return 0 != isdigit(inLetter);
      }

      bool Parser::isAlphaNumeric(char inLetter)
      {
        if (inLetter < 0)
          return false;
        return 0 != isalnum(inLetter);
      }

      bool Parser::isHexDigit(char inLetter)
      {
        return (((inLetter >= 'A') && (inLetter <= 'F')) ||
                ((inLetter >= 'a') && (inLetter <= 'f')) ||
                (isDigit(inLetter)));
      }

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

      WCHAR Parser::findEntity(const String &inLookup)
      {
        for (ULONG loop = 0; NULL != internal::gEntities[loop].mEntityName; ++loop)
        {
          if (inLookup == gEntities[loop].mEntityName)
            return gEntities[loop].mWChar;
        }
        return 0;
      }

      bool Parser::isWhiteSpace(char inLetter)
      {
        switch (inLetter)
        {
          case ' ':
          case '\t':
          case '\r':
          case '\n':
          {
            return true;
          }
        }
        return false;
      }

      bool Parser::isWhiteSpace(const XML::ParserPos &inPos)
      {
        return isWhiteSpace(*inPos);
      }

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
                (temp.getDocument())->addWarning(ParserWarningType_NoEndBracketFound, ioPos);
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

      bool Parser::skipMismatchedEndTag(XML::ParserPos &ioPos)
      {
        // this is an end of element, except there is no matching element
        Parser::AutoStack stack(ioPos);

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

        (ioPos.getDocument())->addWarning(ParserWarningType_MismatchedEndTag);
        return start != ioPos;
      }

      ULONG Parser::getOutputSize(const DocumentPtr &inDocument, NodePtr inNode)
      {
        if (!inNode)
          return 0;

        switch (inNode->getNodeType())
        {
          case XML::Node::NodeType::Document:      return inNode->toDocument()->getOutputSize();
          case XML::Node::NodeType::Element:       return inNode->toElement()->getOutputSize(inDocument);
          case XML::Node::NodeType::Attribute:     return inNode->toAttribute()->getOutputSize(inDocument);
          case XML::Node::NodeType::Text:          return inNode->toText()->getOutputSize(inDocument);
          case XML::Node::NodeType::Comment:       return inNode->toComment()->getOutputSize(inDocument);
          case XML::Node::NodeType::Declaration:   return inNode->toDeclaration()->getOutputSize(inDocument);
          case XML::Node::NodeType::Unknown:       return inNode->toUnknown()->getOutputSize(inDocument);
          default:
          {
            ZS_THROW_BAD_STATE("missing node type in getOutputSize table")
          }
        }
        return 0;
      }

      void Parser::writeBuffer(const DocumentPtr &inDocument, NodePtr inNode, char * &ioPos)
      {
        if (!inNode)
          return;

        switch (inNode->getNodeType())
        {
          case XML::Node::NodeType::Document:      return inNode->toDocument()->writeBuffer(ioPos);
          case XML::Node::NodeType::Element:       return inNode->toElement()->writeBuffer(inDocument, ioPos);
          case XML::Node::NodeType::Attribute:     return inNode->toAttribute()->writeBuffer(inDocument, ioPos);
          case XML::Node::NodeType::Text:          return inNode->toText()->writeBuffer(inDocument, ioPos);
          case XML::Node::NodeType::Comment:       return inNode->toComment()->writeBuffer(inDocument, ioPos);
          case XML::Node::NodeType::Declaration:   return inNode->toDeclaration()->writeBuffer(inDocument, ioPos);
          case XML::Node::NodeType::Unknown:       return inNode->toUnknown()->writeBuffer(inDocument, ioPos);
          default:
          {
            ZS_THROW_BAD_STATE("missing node type in writeBuffer table")
          }
        }
      }

      void Parser::writeBuffer(char * &ioPos, CSTR inString)
      {
        if (NULL == inString)
          return;

        size_t length = strlen(inString);
        strcpy(ioPos, inString);
        ioPos += length;
      }

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

      void Parser::safeAdoptAsLastChild(NodePtr inParent, NodePtr inNewChild)
      {
        if (!inParent)
          return;

        ZS_THROW_BAD_STATE_IF(!inNewChild)

        inParent->adoptAsLastChild(inNewChild);
      }

    } // namespace internal


    //----------------------------------------------------------------------------


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

    String Parser::makeTextEntitySafe(const String &inString, bool entityEncodeWindowsCarriageReturn)
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
            if (entityEncodeWindowsCarriageReturn) {
              strcpy(dest, "&#xD;");
              dest += strlen("&#xD;");
            } else {
              *dest = *source;
              ++dest;
            }
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

  } // namespace XML

} // namespace zsLib

#pragma warning(pop)
