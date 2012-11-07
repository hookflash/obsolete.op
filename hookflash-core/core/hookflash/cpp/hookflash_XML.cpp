/*
 
 Copyright (c) 2012, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#include <hookflash/internal/hookflash_XML.h>

#include <zsLib/XML.h>

namespace hookflash
{
  typedef zsLib::String String;
  typedef zsLib::XML::Node Node;
  typedef zsLib::XML::NodePtr NodePtr;
  typedef zsLib::XML::ElementPtr ElementPtr;
  typedef zsLib::XML::Document Document;
  typedef zsLib::XML::DocumentPtr DocumentPtr;

  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark XML
    #pragma mark

    //-------------------------------------------------------------------------
    ElementPtr XML::createFromString(const String &elementStr)
    {
      if (!elementStr) return ElementPtr();

      DocumentPtr doc = Document::create();
      doc->parse(elementStr);

      ElementPtr childEl = doc->getFirstChildElement();
      if (!childEl) return ElementPtr();

      childEl->orphan();
      return childEl;
    }

    //-------------------------------------------------------------------------
    String XML::convertToString(const ElementPtr &element)
    {
      if (!element) return String();

      DocumentPtr doc = Document::create();
      boost::shared_array<char> output = doc->write(element);

      return output.get();
    }

    //-------------------------------------------------------------------------
    ElementPtr XML::clone(const ElementPtr &element)
    {
      if (!element) return ElementPtr();
      return element->clone()->toElement();
    }
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IXML
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IXML::createFromString(const String &elementStr)
  {
    return internal::XML::createFromString(elementStr);
  }

  //---------------------------------------------------------------------------
  String IXML::convertToString(const ElementPtr &element)
  {
    return internal::XML::convertToString(element);
  }

  //---------------------------------------------------------------------------
  ElementPtr IXML::clone(const ElementPtr &element)
  {
    return internal::XML::clone(element);
  }
}
