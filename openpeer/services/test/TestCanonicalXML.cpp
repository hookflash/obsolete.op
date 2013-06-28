/*
 
 Copyright (c) 2013, SMB Phone Inc.
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

#include <openpeer/services/ICanonicalXML.h>

#include <zsLib/XML.h>
#include <iostream>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "config.h"
#include "boost_replacement.h"

using openpeer::services::ICanonicalXML;

void doTestCanonicalXML()
{
  if (!OPENPEER_SERVICE_TEST_DO_CANONICAL_XML_TEST) return;

  BOOST_INSTALL_LOGGER();


  {
    const char *input =
    "<!DOCTYPE doc [\n"
    "<!ATTLIST e2 xml:space (default|preserve) 'preserve'>\n"
    "<!ATTLIST e3 id ID #IMPLIED>\n"
    "]>\n"
    "<doc xmlns=\"http://www.ietf.org\" xmlns:w3c=\"http://www.w3.org\">\n"
    " <e1>\n"
    "  <e2 xmlns=\"\">\n"
    "   <e3 id=\"E3\">\r\n"
    "  </e2>\n"
    "  <text>First line&#x0d;&#10;Second line</text>\n"
    "  <value>&#x32;</value>\n"
    "  <compute><![CDATA[value>\"0\" && value<\"10\" ?\"valid\":\"error\"]]></compute>\n"
    "  <compute expr='value>\"0\" &amp;&amp; value&lt;\"10\" ?\"valid\":\"error\"'>valid</compute>\n"
    "  <norm attr=' &apos;   &#x20;&#13;&#xa;&#9;   &apos; '/>\n"
    "  <normNames attr='   A   &#x20;&#13;&#xa;&#9;   B   '/>\n"
    "  <normId id=' &apos;   &#x20;&#13;&#xa;&#9;   &apos; '/>\n"
    " </e1>\n"
    "</doc>\n";
    
    const char *finalized =
    "<e1 xmlns=\"http://www.ietf.org\" xmlns:w3c=\"http://www.w3.org\">\n"
    "  <e2 xmlns=\"\">\n"
    "   <e3 id=\"E3\">&#xD;\n"
    "  </e3></e2>\n"
    "  <text>First line&#xD;\n"
    "Second line</text>\n"
    "  <value>2</value>\n"
    "  <compute>value&gt;\"0\" &amp;&amp; value&lt;\"10\" ?\"valid\":\"error\"</compute>\n"
    "  <compute expr=\"value&gt;&quot;0&quot; &amp;&amp; value&lt;&quot;10&quot; ?&quot;valid&quot;:&quot;error&quot;\">valid</compute>\n"
    "  <norm attr=\" \'    &#xD;&#xA;&#x9;   \' \"></norm>\n"
    "  <normNames attr=\"   A    &#xD;&#xA;&#x9;   B   \"></normNames>\n"
    "  <normId id=\" \'    &#xD;&#xA;&#x9;   \' \"></normId>\n"
    " </e1>";

    zsLib::XML::DocumentPtr doc = zsLib::XML::Document::createFromParsedXML(input);
    zsLib::XML::ElementPtr elementDoc = doc->getFirstChildElementChecked();

    zsLib::XML::ElementPtr e1 = elementDoc->getFirstChildElementChecked();

    zsLib::String output = ICanonicalXML::convert(e1);
    BOOST_EQUAL(output, finalized)
  }

  {
    const char *input =
    "<!DOCTYPE doc [<!ATTLIST e9 attr CDATA \"default\">]>\n"
    "<doc>\n"
    "   <e1   />\n"
    "   <e2   ></e2>\n"
    "   <e3   name = \"elem3\"   id=\"elem3\"   />\n"
    "   <e4   name=\"elem4\"   id=\"elem4\"   ></e4>\n"
    "   <e5 a:attr=\"out\" b:attr=\"sorted\" attr2=\"all\" attr=\"I'm\"\n"
    "      xmlns:b=\"http://www.ietf.org\"\n"
    "      xmlns:a=\"http://www.w3.org\"\n"
    "      xmlns=\"http://example.org\"/>\n"
    "   <e6 xmlns=\"\" xmlns:a=\"http://www.w3.org\">\n"
    "      <e7 xmlns=\"http://www.ietf.org\">\n"
    "         <e8 xmlns=\"\" xmlns:a=\"http://www.w3.org\">\n"
    "            <e9 xmlns=\"\" xmlns:a=\"http://www.ietf.org\"/>\n"
    "         </e8>\n"
    "      </e7>\n"
    "   </e6>\n"
    "</doc>";
    
    const char *finalized =
    "<doc>\n"
    "   <e1></e1>\n"
    "   <e2></e2>\n"
    "   <e3 id=\"elem3\" name=\"elem3\"></e3>\n"
    "   <e4 id=\"elem4\" name=\"elem4\"></e4>\n"
    "   <e5 xmlns=\"http://example.org\" xmlns:a=\"http://www.w3.org\" xmlns:b=\"http://www.ietf.org\" a:attr=\"out\" attr=\"I\'m\" attr2=\"all\" b:attr=\"sorted\"></e5>\n"
    "   <e6 xmlns:a=\"http://www.w3.org\">\n"
    "      <e7 xmlns=\"http://www.ietf.org\">\n"
    "         <e8 xmlns=\"\">\n"
    "            <e9 xmlns:a=\"http://www.ietf.org\"></e9>\n"
    "         </e8>\n"
    "      </e7>\n"
    "   </e6>\n"
    "</doc>";

    zsLib::XML::DocumentPtr doc = zsLib::XML::Document::createFromParsedXML(input);
    zsLib::XML::ElementPtr elementDoc = doc->getFirstChildElementChecked();

    zsLib::String output = ICanonicalXML::convert(elementDoc);
    BOOST_EQUAL(output, finalized)
    BOOST_STDOUT() << output.c_str() << "\n";
  }

}
