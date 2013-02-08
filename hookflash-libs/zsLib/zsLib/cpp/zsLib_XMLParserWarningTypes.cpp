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
#include <zsLib/Stringize.h>

#ifdef _DEBUG
#include <iostream>
#endif //_DEBUG

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

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
      #pragma mark XML::intenral::ParserWarning
      #pragma mark

      //-----------------------------------------------------------------------
      static CSTR convertToString(ParserWarningTypes inWarningType)
      {
        switch (inWarningType)
        {
          case ParserWarningType_None:                              return "No Warning";
          case ParserWarningType_MismatchedEndTag:                  return "Element end tag mismatched";
          case ParserWarningType_NoEndBracketFound:                 return "The closing \'>\' tag was not found";
          case ParserWarningType_ContentAfterCloseSlashInElement:   return "Content found after closing \'/\' in element";
          case ParserWarningType_ContentAfterCloseElementName:      return "Content found after \"</name \" in closing of element";
          case ParserWarningType_IllegalAttributeName:              return "Illegal attribute name found";
          case ParserWarningType_AttributeWithoutValue:             return "Attribute found that does not have a value";
          case ParserWarningType_AttributeValueNotFound:            return "Attribute is missing value";
          case ParserWarningType_AttributeValueMissingEndQuote:     return "Attribute value is missing the end quote";
          case ParserWarningType_CDATAMissingEndTag:                return "CDATA is missing end tag";
          case ParserWarningType_NoEndTagFound:                     return "Start element found but no \"</>\" end tag found";
          case ParserWarningType_NoEndCommentFound:                 return "Comment was not closed properly";
          case ParserWarningType_NoEndUnknownTagFound:              return "Open \'<\' was found but no closing \'>\' was found";
          case ParserWarningType_NoEndDeclarationFound:             return "Declation \"<?xml ...\" was found but closing \"?>\" was not found";
          case ParserWarningType_NotProperEndDeclaration:           return "Declation \"<?xml ...\" was found but found \">\" instead of closing \"?>\"";
          case ParserWarningType_DuplicateAttribute:                return "An attribute on an element was duplicated";
          case ParserWarningType_ElementsNestedTooDeep:             return "Elements are nested too deep and likely caused by missing close tags";

          case ParserWarningType_MustOpenWithObject:                return "Expected to open object type with \"{\"";
          case ParserWarningType_MustCloseRootObject:               return "Expected to close root object with \"}\"";
          case ParserWarningType_MissingObjectClose:                return "Missing object \"}\" close";
          case ParserWarningType_DataFoundAfterFinalObjectClose:    return "Found data after final object close";
          case ParserWarningType_MissingStringQuotes:               return "String quotes (\") were expected but not found";
          case ParserWarningType_InvalidEscapeSequence:             return "Invalid escape sequence";
          case ParserWarningType_InvalidUnicodeEscapeSequence:      return "Invalid unicode escape sequence";
          case ParserWarningType_IllegalNumberSequence:             return "Illegal number sequence";
          case ParserWarningType_MissingColonBetweenStringAndValue: return "Missing \":\" between string : value";
          case ParserWarningType_AttributePrefixWithoutName:        return "Attribute prefix found but no name for attribute found";
          case ParserWarningType_AttributePrefixAtRoot:             return "Attribute prefix found at document root";
          case ParserWarningType_MissingPairString:                 return "Empty \"string\" found in pair string : value";
          case ParserWarningType_IllegalValue:                      return "Illegal value found in pair string : value";
          case ParserWarningType_IllegalArrayAtRoot:                return "Illegal array found at object root";
          case ParserWarningType_UnexpectedComma:                   return "Parser did not expect a \",\" comma";
          case ParserWarningType_ParserStuck:                       return "Parser stuck at same location and unable to continue";
        }
        ZS_THROW_INVALID_USAGE("could not find string for warning type")
        return "";
      }

    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML::ParserWarning
    #pragma mark

    //-------------------------------------------------------------------------
    ParserWarning::ParserWarning(
                                 ParserWarningTypes inWarningType,
                                 const XML::internal::Parser::ParserStack &inStack
                                 ) :
    mWarningType(inWarningType)
    {
      ULONG lastRow = 0;
      ULONG lastColumn = 0;

      for (XML::internal::Parser::ParserStack::const_iterator iter = inStack.begin(); iter!= inStack.end(); ++iter)
      {
        if ((lastRow == (*iter).mRow) &&
            (lastColumn == (*iter).mColumn))
          continue;

        lastRow = (*iter).mRow;
        lastColumn = (*iter).mColumn;

        ParserInfo info;
        info.mPos = (*iter);
        info.mXMLSnip = String::copyFromUnicodeSafe((*iter).mPos, 30);
        info.mXMLSnip.replaceAll("\a"," ");
        info.mXMLSnip.replaceAll("\b"," ");
        info.mXMLSnip.replaceAll("\n"," ");
        info.mXMLSnip.replaceAll("\r"," ");
        info.mXMLSnip.replaceAll("\t"," ");
        info.mXMLSnip.replaceAll("\v"," ");
        info.mXMLSnip.replaceAll("\f"," ");
        info.mXMLSnip.trimLeft();
        info.mXMLSnip.trimRight();
        mStack.push_back(info);
      }
      //#ifdef _DEBUG
      //   std::cout << "---------\n";
      //   std::cout << getAsString() << "\n";
      //   std::cout << "---------\n";
      //#endif //_DEBUG
    }

    //-------------------------------------------------------------------------
    String ParserWarning::getAsString(bool inIncludeEntireStack) const
    {
      // Some issue @ row=1 column=2: xml text

      // -or-

      // Some issue
      // @ row=1 column=2: xml text
      // @ row=3 column=4: xml text

      String result(internal::convertToString(mWarningType));

      ULONG countedRows = (inIncludeEntireStack ? ((ULONG)mStack.size()) : (mStack.size() > 0 ? 1 : 0));

      if (inIncludeEntireStack)
        result += "\n";
      else
        result += " ";

      // output rows
      {
        bool firstTime = true;
        for (ParserStack::const_reverse_iterator iter = mStack.rbegin(); (0 != countedRows) && (iter != mStack.rend()); ++iter)
        {
          const ParserPos &pos = (*iter).mPos;

          if (!firstTime)
            result += "\n";

          result += "@ row=" + String(Stringize<ULONG>(pos.mRow)) + " column=" + String(Stringize<ULONG>(pos.mColumn)) + ": ";

          // cut out a snip-it of the parsing position for output
          result += (*iter).mXMLSnip;

          firstTime = false;
          --countedRows;
        }
      }

      return result;
    }

  } // namespace XML

} // namespace zsLib
