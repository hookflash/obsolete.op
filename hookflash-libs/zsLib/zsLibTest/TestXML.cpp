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
#include <set>


//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>


#include "boost_replacement.h"

using zsLib::CSTR;
using zsLib::ULONG;

struct XMLWarningInfo
{
  CSTR mSearchStr;
  ULONG mRow;
  ULONG mColumn;
};

struct XMLResultInfo
{
  CSTR mInput;
  CSTR mOutput;
  ULONG  mTabSize;
  CSTR *mSingleLineElements;
  bool mCaseSensativeElements;
  bool mCaseSensativeAttributes;
  XMLWarningInfo **mWarnings;
};

static const char *gXML1Input =
"<?xml version=\"1.0\" encoding=\"ISO8859-1\" ?>\n"
"<note>\n"
"  <!--this is a test comment-->\n"
"  <%--what is this 1%>\n"
"  <!what is this 2>\n"
"  <$what is this 3$>\n"
"  <to test=\"hello\" test2=\"goodbye\">Tove</to>\n"
"  <from>Jani</from>\n"
"  <heading>Reminder</heading>\n"
"  <body>Don't forget me this weekend!</body>\n"
"  <cdatatest><![CDATA[your momma wears <army boots>]]></cdatatest>\n"
"</note>\n";
static const char *gXML1Output = gXML1Input;

static XMLWarningInfo *gXMLWarnings1Array[] = {NULL};

static XMLResultInfo gXMLResults1 =
{
  gXML1Input,
  gXML1Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings1Array
};

static const char *gXML2Input =
"\n"
"<test>contents</test>\n"
"  \t\t <!--- this is a non-closed comment -- >\n"
"<test2>contents2</test2>\n";

static const char *gXML2Output =
"\n"
"<test>contents</test>\n"
"  \t\t <!--- this is a non-closed comment -- >\n"
"<test2>contents2</test2>\n"
"-->";

static XMLWarningInfo gXMLWarnings2_1[] =
{
  {"<!---", 3, 18},
  {"Comment was not closed properly\n"
    "@ row=3 column=18: <!--- this is a non-closed com", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings2Array[] =
{
  gXMLWarnings2_1,
  NULL
};

static XMLResultInfo gXMLResults2 =
{
  gXML2Input,
  gXML2Output,
  8,
  NULL,
  true,
  true,
  gXMLWarnings2Array
};

static const char *gXML3Input =
"<?xml +bogus=\"1\" x=\"1\"?>\n"
"</bogus>\n"
"<test>contents</test>\n"
"<outer><inner></inner></outer>\n"
"<outer><inner1></bogus></></outer>\n"
"<outer><inner2 /></outer>\n"
"<outer><inner3></outer>\n"
"<outer><inner4></></></>\n"
"<outer><inner5></outer></inner5>\n"
"\n";

static const char *gXML3Output =
"<?xml x=\"1\" ?>\n"
"\n"
"<test>contents</test>\n"
"<outer><inner /></outer>\n"
"<outer><inner1 /></outer>\n"
"<outer><inner2 /></outer>\n"
"<outer><inner3 /></outer>\n"
"<outer><inner4 /></outer>\n"
"<outer><inner5 /></outer>\n"
"\n";

static XMLWarningInfo gXMLWarnings3_1[] =
{
  {"+bogus", 1, 7},
  {"<?xml", 1, 1},
  {"Illegal attribute name found\n"
    "@ row=1 column=7: +bogus=\"1\" x=\"1\"?> </bogus> <t\n"
    "@ row=1 column=1: <?xml +bogus=\"1\" x=\"1\"?> </bog", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings3_2[] =
{
  {"</bogus>", 2, 1},
  {"Element end tag mismatched\n"
    "@ row=2 column=1: </bogus> <test>contents</test>", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings3_3[] =
{
  {"</bogus></>", 5, 16},
  {"<inner1>", 5, 8},
  {"<outer><inner1>", 5, 1},
  {"Element end tag mismatched\n"
    "@ row=5 column=16: </bogus></></outer> <outer><in\n"
    "@ row=5 column=8: <inner1></bogus></></outer> <o\n"
    "@ row=5 column=1: <outer><inner1></bogus></></ou", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings3_4[] =
{
  {"</>\n", 8, 22},
  {"Element end tag mismatched\n"
    "@ row=8 column=22: </> <outer><inner5></outer></i", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings3_5[] =
{
  {"</inner5>", 9, 24},
  {"Element end tag mismatched\n"
    "@ row=9 column=24: </inner5>", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings3Array[] =
{
  gXMLWarnings3_1,
  gXMLWarnings3_2,
  gXMLWarnings3_3,
  gXMLWarnings3_4,
  gXMLWarnings3_5,
  NULL
};

static XMLResultInfo gXMLResults3 =
{
  gXML3Input,
  gXML3Output,
  3,
  NULL,
  true,
  true,
  gXMLWarnings3Array
};

static const char *gXML4Input =
"\n"
"<test>contents</test>\n"
"<?xml unclosed_declaration illegal{}name\n";

static const char *gXML4Output =
"\n"
"<test>contents</test>\n"
"<?xml unclosed_declaration ?>";

static XMLWarningInfo gXMLWarnings4_1[] =
{
  {"unclosed_declaration", 3, 7},
  {"<?xml", 3, 1},
  {"Attribute found that does not have a value\n"
    "@ row=3 column=7: unclosed_declaration illegal{}\n"
    "@ row=3 column=1: <?xml unclosed_declaration ill", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings4_2[] =
{
  {"{}name", 3, 35},
  {"illegal{}", 3, 28},
  {"<?xml", 3, 1},
  {"Illegal attribute name found\n"
    "@ row=3 column=35: {}name\n"
    "@ row=3 column=28: illegal{}name\n"
    "@ row=3 column=1: <?xml unclosed_declaration ill", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings4_3[] =
{
  {"<?xml", 3, 1},
  {"Declation \"<?xml ...\" was found but closing \"?>\" was not found\n"
    "@ row=3 column=1: <?xml unclosed_declaration ill", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings4Array[] =
{
  gXMLWarnings4_1,
  gXMLWarnings4_2,
  gXMLWarnings4_3,
  NULL
};

static XMLResultInfo gXMLResults4 =
{
  gXML4Input,
  gXML4Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings4Array
};

static const char *gXML5Input =
"<?xml no_value_found = ?>\n"
"<test illegal{}name=\"1\" dup=value1 dup=value2 ></test bogus>\n"
"<?xml dup=value1 dup=\"value2\" no_end_quote_found=\'haha";

static const char *gXML5Output =
"<?xml ?>\n"
"<test dup=\"value2\" />\n"
"<?xml dup=\"value2\" ?>";

static XMLWarningInfo gXMLWarnings5_1[] =
{
  {"?>", 1, 24},
  {"no_value_found", 1, 7},
  {"<?xml", 1, 1},
  {"Attribute is missing value\n"
    "@ row=1 column=24: ?> <test illegal{}name=\"1\" dup\n"
    "@ row=1 column=7: no_value_found = ?> <test ille\n"
    "@ row=1 column=1: <?xml no_value_found = ?> <tes", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings5_2[] =
{
  {"{}name", 2, 14},
  {"illegal{}name", 2, 7},
  {"<test", 2, 1},
  {"Illegal attribute name found\n"
    "@ row=2 column=14: {}name=\"1\" dup=value1 dup=valu\n"
    "@ row=2 column=7: illegal{}name=\"1\" dup=value1 d\n"
    "@ row=2 column=1: <test illegal{}name=\"1\" dup=va", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings5_3[] =
{
  {"dup=value2", 2, 36},
  {"<test", 2, 1},
  {"An attribute on an element was duplicated\n"
    "@ row=2 column=36: dup=value2 ></test bogus> <?xm\n"
    "@ row=2 column=1: <test illegal{}name=\"1\" dup=va", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings5_4[] =
{
  {"bogus>", 2, 55},
  {"<test", 2, 1},
  {"Content found after \"</name \" in closing of element\n"
    "@ row=2 column=55: bogus> <?xml dup=value1 dup=\"v\n"
    "@ row=2 column=1: <test illegal{}name=\"1\" dup=va", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings5_5[] =
{
  {"<?xml dup", 3, 1},
  {"An attribute on an element was duplicated\n"
    "@ row=3 column=1: <?xml dup=value1 dup=\"value2\"", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings5_6[] =
{
  {"\'", 3, 50},
  {"no_end_quote_found", 3, 31},
  {"<?xml dup=value1", 3, 1},
  {"Attribute value is missing the end quote\n"
    "@ row=3 column=50: \'haha\n"
    "@ row=3 column=31: no_end_quote_found=\'haha\n"
    "@ row=3 column=1: <?xml dup=value1 dup=\"value2\"", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings5_7[] =
{
  {"<?xml dup=value1", 3, 1},
  {"Declation \"<?xml ...\" was found but closing \"?>\" was not found\n"
    "@ row=3 column=1: <?xml dup=value1 dup=\"value2\"", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings5Array[] =
{
  gXMLWarnings5_1,
  gXMLWarnings5_2,
  gXMLWarnings5_3,
  gXMLWarnings5_4,
  gXMLWarnings5_5,
  gXMLWarnings5_6,
  gXMLWarnings5_7,
  NULL
};

static XMLResultInfo gXMLResults5 =
{
  gXML5Input,
  gXML5Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings5Array
};


static const char *gXML6Input =
"<test>contents</test>\n"
"<whatever /bogus>\n"
"<tag";

static const char *gXML6Output =
"<test>contents</test>\n"
"<whatever />\n"
"<tag />";

static XMLWarningInfo gXMLWarnings6_1[] =
{
  {"bogus>", 2, 12},
  {"<whatever", 2, 1},
  {"Content found after closing \'/\' in element\n"
    "@ row=2 column=12: bogus> <tag\n"
    "@ row=2 column=1: <whatever /bogus> <tag", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings6_2[] =
{
  {"<tag", 3, 1},
  {"The closing \'>\' tag was not found\n"
    "@ row=3 column=1: <tag", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings6_3[] =
{
  {"<tag", 3, 1},
  {"Start element found but no \"</>\" end tag found\n"
    "@ row=3 column=1: <tag", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings6Array[] =
{
  gXMLWarnings6_1,
  gXMLWarnings6_2,
  gXMLWarnings6_3,
  NULL
};

static XMLResultInfo gXMLResults6 =
{
  gXML6Input,
  gXML6Output,
  8,
  NULL,
  true,
  true,
  gXMLWarnings6Array
};

static const char *gXML7Input =
"<test>contents</test>\n"
"<tag>whatever</tag \n";

static const char *gXML7Output =
"<test>contents</test>\n"
"<tag>whatever</tag>";

static XMLWarningInfo gXMLWarnings7_1[] =
{
  {"</tag", 2, 14},
  {"<tag>whatever", 2, 1},
  {"The closing \'>\' tag was not found\n"
    "@ row=2 column=14: </tag\n"
    "@ row=2 column=1: <tag>whatever</tag", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings7Array[] =
{
  gXMLWarnings7_1,
  NULL
};

static XMLResultInfo gXMLResults7 =
{
  gXML7Input,
  gXML7Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings7Array
};

static const char *gXML8Input =
"<test>contents</test>\n"
"<tag>whatever</ \n";

static const char *gXML8Output =
"<test>contents</test>\n"
"<tag>whatever</tag>";

static XMLWarningInfo gXMLWarnings8_1[] =
{
  {"</ ", 2, 14},
  {"<tag>whatever", 2, 1},
  {"The closing \'>\' tag was not found\n"
    "@ row=2 column=14: </\n"
    "@ row=2 column=1: <tag>whatever</", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings8Array[] =
{
  gXMLWarnings8_1,
  NULL
};

static XMLResultInfo gXMLResults8 =
{
  gXML8Input,
  gXML8Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings8Array
};

static const char *gXML9Input =
"<test>contents</test>\n"
"<tag>whatever</ {\n";

static const char *gXML9Output =
"<test>contents</test>\n"
"<tag>whatever</tag>";

static XMLWarningInfo gXMLWarnings9_1[] =
{
  {"</ {", 2, 14},
  {"<tag>whatever", 2, 1},
  {"Element end tag mismatched\n"
    "@ row=2 column=14: </ {\n"
    "@ row=2 column=1: <tag>whatever</ {", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings9_2[] =
{
  {"<tag>whatever", 2, 1},
  {"Start element found but no \"</>\" end tag found\n"
    "@ row=2 column=1: <tag>whatever</ {", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings9Array[] =
{
  gXMLWarnings9_1,
  gXMLWarnings9_2,
  NULL
};

static XMLResultInfo gXMLResults9 =
{
  gXML9Input,
  gXML9Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings9Array
};

static const char *gXML10Input =
"<test>contents</test>\n"
"<!bogus";

static const char *gXML10Output =
"<test>contents</test>\n"
"<!bogus>";

static XMLWarningInfo gXMLWarnings10_1[] =
{
  {"<!bogus", 2, 1},
  {"Open \'<\' was found but no closing \'>\' was found\n"
    "@ row=2 column=1: <!bogus", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings10Array[] =
{
  gXMLWarnings10_1,
  NULL
};

static XMLResultInfo gXMLResults10 =
{
  gXML10Input,
  gXML10Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings10Array
};

static const char *gXML11Input =
"<test>contents</test>\n"
"<BR><Br><br></br>\n"
"<HR /><hr>\n"
"<input Dup=alpha DUP=beta dup=/hello.php>\n"
"<input hi=/hello.php />";

static const char *gXML11Output =
"<test>contents</test>\n"
"<BR /><Br /><br />\n"
"<HR /><hr />\n"
"<input dup=\"/hello.php\" />\n"
"<input hi=\"/hello.php\" />";

static XMLWarningInfo gXMLWarnings11_1[] =
{
  {"</br>", 2, 13},
  {"Element end tag mismatched\n"
    "@ row=2 column=13: </br> <HR /><hr> <input Dup=al", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings11_2[] =
{
  {"DUP=beta", 4, 18},
  {"<input Dup=alpha", 4, 1},
  {"An attribute on an element was duplicated\n"
    "@ row=4 column=18: DUP=beta dup=/hello.php> <inpu\n"
    "@ row=4 column=1: <input Dup=alpha DUP=beta dup=", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo gXMLWarnings11_3[] =
{
  {"dup=/hello.php", 4, 27},
  {"<input Dup=alpha", 4, 1},
  {"An attribute on an element was duplicated\n"
    "@ row=4 column=27: dup=/hello.php> <input hi=/hel\n"
    "@ row=4 column=1: <input Dup=alpha DUP=beta dup=", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings11Array[] =
{
  gXMLWarnings11_1,
  gXMLWarnings11_2,
  gXMLWarnings11_3,
  NULL
};

static CSTR gSingleLineElements11[] =
{
  "hr",
  "br",
  "INPUT",
  NULL
};

static XMLResultInfo gXMLResults11 =
{
  gXML11Input,
  gXML11Output,
  0,
  gSingleLineElements11,
  false,
  false,
  gXMLWarnings11Array
};


static const char *gXML12Input =
"<img>test\n"
"<Img>test</Img>"
"<IMG +bogus value1=1 />";

static const char *gXML12Output =
"<img />test\n"
"<Img>test</Img>"
"<IMG value1=\"1\" />";

static XMLWarningInfo gXMLWarnings12_1[] =
{
  {"+bogus", 2, 21},
  {"<IMG", 2, 16},
  {"Illegal attribute name found\n"
    "@ row=2 column=21: +bogus value1=1 />\n"
    "@ row=2 column=16: <IMG +bogus value1=1 />", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings12Array[] =
{
  gXMLWarnings12_1,
  NULL
};

static CSTR gSingleLineElements12[] =
{
  "img",
  NULL
};

static XMLResultInfo gXMLResults12 =
{
  gXML12Input,
  gXML12Output,
  0,
  gSingleLineElements12,
  true,
  true,
  gXMLWarnings12Array
};

static const char *gXML13Input =
"<?xml test1=\"test\" />\n"
"<?xml text2=\"test\" ?>\n";

static const char *gXML13Output =
"<?xml test1=\"test\" ?>\n"
"<?xml text2=\"test\" ?>\n";

static XMLWarningInfo gXMLWarnings13_1[] =
{
  {"/>", 1, 20},
  {"<?", 1, 1},
  {"Declation \"<?xml ...\" was found but found \">\" instead of closing \"?>\"\n"
    "@ row=1 column=20: /> <?xml text2=\"test\" ?>\n"
    "@ row=1 column=1: <?xml test1=\"test\" /> <?xml te", 0, 0},
  {NULL, 0, 0}
};

static XMLWarningInfo *gXMLWarnings13Array[] =
{
  gXMLWarnings13_1,
  NULL
};

static XMLResultInfo gXMLResults13 =
{
  gXML13Input,
  gXML13Output,
  0,
  NULL,
  true,
  true,
  gXMLWarnings13Array
};


class TestXML
{
public:
  typedef std::set<zsLib::XML::Node *> FoundNodesSet;

  TestXML()
  {
    parse(gXMLResults1);
    parse(gXMLResults2);
    parse(gXMLResults3);
    parse(gXMLResults4);
    parse(gXMLResults5);
    parse(gXMLResults6);
    parse(gXMLResults7);
    parse(gXMLResults8);
    parse(gXMLResults9);
    parse(gXMLResults10);
    parse(gXMLResults11);
    parse(gXMLResults12);
    parse(gXMLResults13);
    generate();
    parserPosTest();
    {int i = 0; ++i;}
  }

  void parse(XMLResultInfo &results)
  {
    zsLib::XML::DocumentPtr document(::zsLib::XML::Document::create());

    ULONG tabSize = results.mTabSize;
    if (0 != tabSize)
      document->setTabSize(results.mTabSize);

    document->setElementNameIsCaseSensative(results.mCaseSensativeElements);
    document->setAttributeNameIsCaseSensative(results.mCaseSensativeAttributes);

    if (NULL != results.mSingleLineElements)
    {
      for (ULONG loop = 0; NULL != results.mSingleLineElements[loop]; ++loop)
      {
        document->addContainsNoChildrenElement(zsLib::String(results.mSingleLineElements[loop]));
      }
    }

    document->parse(results.mInput);

    zsLib::XML::DocumentPtr cloneDocument = (document->clone())->toDocument();

    ULONG length = 0;
    boost::shared_array<char> output = cloneDocument->write(&length);

    BOOST_CHECK(length == strlen(output.get()))

    //std::cout << "--- SOR ---\n" << output << "--- EOR ---\n";

    BOOST_CHECK(0 == strcmp(results.mOutput, output.get()))

    // original document and cloned must both validate
    treeWalkChecker(document);
    treeWalkChecker(cloneDocument);

    ULONG loop = 0;
    const zsLib::XML::Document::Warnings &warnings = document->getWarnings();
    for (zsLib::XML::Document::Warnings::const_iterator iter = warnings.begin(); iter != warnings.end(); ++iter, ++loop)
    {
      const zsLib::XML::ParserWarning &warning = (*iter);

      zsLib::String warningText = warning.getAsString();

      XMLWarningInfo *resultWarning = (results.mWarnings[loop]);
      if (NULL == resultWarning)
      {
        // found more warnings than results
        BOOST_CHECK(NULL != resultWarning)
        break;
      }

      ULONG loop2 = 0;
      for (zsLib::XML::ParserWarning::ParserStack::const_reverse_iterator iter2 = warning.mStack.rbegin(); iter2 != warning.mStack.rend(); ++iter2, ++loop2)
      {
        const zsLib::XML::ParserPos &pos = (*iter2).mPos;
        if (NULL == resultWarning[loop2+1].mSearchStr)
        {
          BOOST_CHECK(NULL != resultWarning[loop2+1].mSearchStr)

          // found too many warning positions
          break;
        }
        XMLWarningInfo &resultWarningDetail = resultWarning[loop2];

        BOOST_CHECK(pos.mPos == strstr(results.mInput, resultWarningDetail.mSearchStr))
        BOOST_CHECK(pos.mRow == resultWarningDetail.mRow)
        BOOST_CHECK(pos.mColumn == resultWarningDetail.mColumn)
      }

      BOOST_CHECK(0 == strcmp(warningText, resultWarning[loop2].mSearchStr))
    }
    // make sure there aren't more warnings expected than found
    BOOST_CHECK(NULL == results.mWarnings[loop])
  }

  void treeWalkChecker(zsLib::XML::NodePtr inNode)
  {
    FoundNodesSet foundNodes;
    treeWalkChecker(inNode, foundNodes);
  }

  void treeWalkChecker(zsLib::XML::NodePtr inNode, FoundNodesSet &foundNodes)
  {
    // make sure this node hasn't been seen before
    FoundNodesSet::iterator iter = foundNodes.find(inNode.get());
    BOOST_CHECK(iter == foundNodes.end())
    foundNodes.insert(inNode.get());

    zsLib::XML::AttributePtr firstAttribute;
    zsLib::XML::AttributePtr lastAttribute;

    // check specialized aspects of each node
    switch (inNode->getNodeType())
    {
      case zsLib::XML::Node::NodeType::Document:
      {
        // this cannot have a parent
        BOOST_CHECK(!(inNode->getParent()))
        BOOST_CHECK(inNode->getFirstSibling()->isDocument())
        BOOST_CHECK(inNode->getLastSibling()->isDocument())
        break;
      }
      case zsLib::XML::Node::NodeType::Element:
      {
        // must have a parent
        BOOST_CHECK(inNode->getParent())

        // parent must be another element or document
        BOOST_CHECK((inNode->getParent()->isElement()) ||
                  (inNode->getParent()->isDocument()))

        firstAttribute = inNode->toElement()->getFirstAttribute();
        lastAttribute = inNode->toElement()->getLastAttribute();
        break;
      }
      case zsLib::XML::Node::NodeType::Attribute:
      {
        // must have a parent
        BOOST_CHECK(inNode->getParent())

        // parents has to be an element or a declaration
        BOOST_CHECK((inNode->getParent()->isElement()) ||
                  (inNode->getParent()->isDeclaration()))

        // cannot have children
        BOOST_CHECK(!(inNode->getFirstChild()))
        BOOST_CHECK(!(inNode->getLastChild()))

        // check if previous or next are also attributes - they must be
        if (inNode->getPreviousSibling())
          BOOST_CHECK(inNode->getPreviousSibling()->isAttribute())
          if (inNode->getNextSibling())
            BOOST_CHECK(inNode->getNextSibling()->isAttribute())

            if (inNode->getParent()->isElement())
            {
              BOOST_CHECK(inNode->getFirstSibling() == inNode->getParent()->toElement()->getFirstAttribute())
              BOOST_CHECK(inNode->getLastSibling() == inNode->getParent()->toElement()->getLastAttribute())
            }
            else
            {
              BOOST_CHECK(inNode->getParent()->isDeclaration())
              BOOST_CHECK(inNode->getFirstSibling() == inNode->getParent()->toDeclaration()->getFirstAttribute())
              BOOST_CHECK(inNode->getLastSibling() == inNode->getParent()->toDeclaration()->getLastAttribute())
            }

        break;
      }
      case zsLib::XML::Node::NodeType::Text:
      {
        // must have a parent
        BOOST_CHECK(inNode->getParent())

        // parent must be another element or document
        BOOST_CHECK((inNode->getParent()->isElement()) ||
                  (inNode->getParent()->isDocument()))

        // cannot have children
        BOOST_CHECK(!(inNode->getFirstChild()))
        BOOST_CHECK(!(inNode->getLastChild()))
        break;
      }
      case zsLib::XML::Node::NodeType::Comment:
      {
        // must have a parent
        BOOST_CHECK(inNode->getParent())

        // parent must be another element or document
        BOOST_CHECK((inNode->getParent()->isElement()) ||
                  (inNode->getParent()->isDocument()))

        // cannot have children
        BOOST_CHECK(!(inNode->getFirstChild()))
        BOOST_CHECK(!(inNode->getLastChild()))
        break;
      }
      case zsLib::XML::Node::NodeType::Declaration:
      {
        // must have a parent
        BOOST_CHECK(inNode->getParent())

        // parent must be another element or document
        BOOST_CHECK((inNode->getParent()->isElement()) ||
                  (inNode->getParent()->isDocument()))

        // cannot have children
        BOOST_CHECK(!(inNode->getFirstChild()))
        BOOST_CHECK(!(inNode->getLastChild()))

        firstAttribute = inNode->toDeclaration()->getFirstAttribute();
        lastAttribute = inNode->toDeclaration()->getLastAttribute();
        break;
      }
      case zsLib::XML::Node::NodeType::Unknown:
      {
        // must have a parent
        BOOST_CHECK(inNode->getParent())

        // parent must be another element or document
        BOOST_CHECK((inNode->getParent()->isElement()) ||
                  (inNode->getParent()->isDocument()))

        // cannot have children
        BOOST_CHECK(!(inNode->getFirstChild()))
        BOOST_CHECK(!(inNode->getLastChild()))
        break;
      }
      default:
      {
        // what is this?
        BOOST_CHECK(false)
      }
    }

    // this node's previous's next must be this node
    if (inNode->getPreviousSibling())
    {
      BOOST_CHECK(inNode->getPreviousSibling()->getNextSibling().get() == inNode.get())
      if (!inNode->isAttribute())
      {
        BOOST_CHECK(inNode->getFirstSibling() == inNode->getParent()->getFirstChild())
      }
    }

    // this node's next's previous must be this node
    if (inNode->getNextSibling())
    {
      BOOST_CHECK(inNode->getNextSibling()->getPreviousSibling().get() == inNode.get())
      if (!inNode->isAttribute())
      {
        BOOST_CHECK(inNode->getLastSibling() == inNode->getParent()->getLastChild())
      }
    }

    checkChildren(inNode, firstAttribute, lastAttribute, foundNodes);
    checkChildren(inNode, inNode->getFirstChild(), inNode->getLastChild(), foundNodes);
  }

  void checkChildren(zsLib::XML::NodePtr inNode, zsLib::XML::NodePtr inFirstChild, zsLib::XML::NodePtr inLastChild, FoundNodesSet &foundNodes)
  {
    // if this does not have a first child then it cannot have a last child
    if (!inFirstChild)
      BOOST_CHECK(!inLastChild)

      // if this does not have a last child then it cannot have a first child
      if (!inLastChild)
        BOOST_CHECK(!inFirstChild)

        bool foundFirstChild = false;
    bool foundLastChild = false;
    zsLib::XML::NodePtr child = inFirstChild;
    while (child)
    {
      bool isFirstChild = false;
      bool isLastChild = false;

      // must have a last child, since a first child was found
      BOOST_CHECK(inLastChild)

      // if this is the first child, make sure it does not have a previous
      if (child.get() == inFirstChild.get())
      {
        isFirstChild = foundFirstChild = true;
        BOOST_CHECK(!(child->getPreviousSibling()))
      }
      if (child.get() == inLastChild.get())
      {
        isLastChild = foundLastChild = true;
        BOOST_CHECK(!(child->getNextSibling()))
      }

      // if this child doesn't have a previous or next then must point to the first or last child respectively
      if (!(child->getPreviousSibling()))
        BOOST_CHECK(inFirstChild.get() == child.get())

        if (!(child->getNextSibling()))
          BOOST_CHECK(inLastChild.get() == child.get())

          // if the first node equals last node then the child node must not have a previous or next
          if (inFirstChild.get() == inLastChild.get())
          {
            // cannot have a previous or next sibling
            BOOST_CHECK(!(child->getPreviousSibling()))
            BOOST_CHECK(!(child->getNextSibling()))
          }
          else
          {
            // first child must have a next and last child must have a previous
            BOOST_CHECK(inFirstChild->getNextSibling())
            BOOST_CHECK(inLastChild->getPreviousSibling())
          }

      // if its not the first child, must have a previous sibling
      if (!isFirstChild)
        BOOST_CHECK(child->getPreviousSibling())
        // if its not the last child, must have a next sibling
        if (!isLastChild)
          BOOST_CHECK(child->getNextSibling())

          // parent must be this node
          BOOST_CHECK(child->getParent().get() == inNode.get())
          treeWalkChecker(child, foundNodes);
      child = child->getNextSibling();
    }
    if (foundFirstChild)
      BOOST_CHECK(foundLastChild)
      if (foundLastChild)
        BOOST_CHECK(foundFirstChild)
        }

  void generate()
  {
    zsLib::XML::AttributePtr attribute7(::zsLib::XML::Attribute::create());
    bool caught = false;

    static CSTR gOutput =
    "<?xml value12=\"12\" value2=\"2\" value1=\"1\" value3=\"foo4\" value11=\"11\" value10=\"10\" ?>\n"
    "insert previous which is first sibling\n"
    "<test value13=\"13\" value5=\"5\" value9=\"9\" value6=\"6\" value8=\"8\"><!--comment1-->text</test>\n"
    "insert previous sibling\n"
    "insert next sibling\n"
    "\n"
    "insert next which is last sibling\n"
    "<text> this  &lt;is&gt; \n"
    "a &#96;test&#96; of &nbsp;the <wild> text </wild>\telement\n"
    "\n"
    "</text>\n"
    "<encoding value1=\"&quot;&apos;\" value2=\'\"\' value3=\"\'\"><![CDATA[<cdata test & result>]]></encoding><!unknown!>";

    zsLib::XML::DocumentPtr document(::zsLib::XML::Document::create());

    treeWalkChecker(document);

    zsLib::XML::ElementPtr element1(::zsLib::XML::Element::create());
    element1->setValue(zsLib::String("test"));
    document->adoptAsFirstChild(element1);

    treeWalkChecker(document);

    zsLib::XML::TextPtr textEOL1(::zsLib::XML::Text::create());
    textEOL1->setValue(zsLib::String("\n"));
    element1->adoptAsNextSibling(textEOL1);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text1(::zsLib::XML::Text::create());
    text1->setValue(zsLib::String("text"));
    element1->adoptAsFirstChild(text1);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text2(::zsLib::XML::Text::create());
    text2->setValue(zsLib::String("\n"));
    document->adoptAsLastChild(text2);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text3(::zsLib::XML::Text::create());
    text3->setValue(zsLib::String("insert previous sibling\n"));
    text2->adoptAsPreviousSibling(text3);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text4(::zsLib::XML::Text::create());
    text4->setValue(zsLib::String("insert next sibling\n"));
    text3->adoptAsNextSibling(text4);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text5(::zsLib::XML::Text::create());
    text5->setValue(zsLib::String("insert next which is last sibling\n"));
    text2->adoptAsNextSibling(text5);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text6(::zsLib::XML::Text::create());
    text6->setValue(zsLib::String("insert previous which is first sibling\n"));
    element1->adoptAsPreviousSibling(text6);

    treeWalkChecker(document);

    zsLib::XML::DeclarationPtr declaration1(::zsLib::XML::Declaration::create());
    declaration1->setAttribute(zsLib::String("value1"), zsLib::String("1"));
    document->adoptAsFirstChild(declaration1);

    BOOST_CHECK(0 == strcmp(declaration1->getAttributeValue(zsLib::String("value1")), "1"))
    BOOST_CHECK(0 == strcmp(declaration1->getAttributeValue(zsLib::String("whatever")), ""))

    // normally wouldn't do this after adding to the document, but this is a test after all
    BOOST_CHECK(document->isAttributeNameIsCaseSensative())
    BOOST_CHECK(0 == strcmp(declaration1->getAttributeValue(zsLib::String("VALUE1")), ""))
    document->setAttributeNameIsCaseSensative(false);
    BOOST_CHECK(!document->isAttributeNameIsCaseSensative())
    BOOST_CHECK(0 == strcmp(declaration1->getAttributeValue(zsLib::String("VALUE1")), "1"))
    document->setAttributeNameIsCaseSensative(true);

    zsLib::XML::TextPtr bogusText(::zsLib::XML::Text::create());
    bogusText->setValue(zsLib::String("bogus"));

    caught = false;
    try {(declaration1->toNode())->adoptAsFirstChild(bogusText);} catch(...) {caught = true;}
    BOOST_CHECK(caught)

    caught = false;
    try {(declaration1->toNode())->adoptAsLastChild(bogusText);} catch(...) {caught = true;}
    BOOST_CHECK(caught)

    treeWalkChecker(document);

    zsLib::XML::TextPtr textEOL2(::zsLib::XML::Text::create());
    textEOL2->setValue(zsLib::String("\n"));
    declaration1->adoptAsNextSibling(textEOL2);

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute2(::zsLib::XML::Attribute::create());
    attribute2->setName(zsLib::String("value2"));
    attribute2->setValue(zsLib::String("2"));
    declaration1->adoptAsFirstChild(attribute2);

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute3(::zsLib::XML::Attribute::create());
    attribute3->setName(zsLib::String("value4"));   // whoops, wrong name
    attribute3->setValue(zsLib::String("3"));
    declaration1->adoptAsFirstChild(attribute3);
    attribute3->setName(zsLib::String("value3"));

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute4(::zsLib::XML::Attribute::create());
    attribute4->setName(zsLib::String("foo4"));   // whoops, wrong name
    attribute4->setValue(zsLib::String("foo4"));
    declaration1->adoptAsFirstChild(attribute4);
    attribute4->setName(zsLib::String("value3"));

    BOOST_CHECK(attribute2 == attribute4->getFirstSibling())
    BOOST_CHECK(attribute4 == attribute2->getLastSibling())

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute5(::zsLib::XML::Attribute::create());
    attribute5->setName(zsLib::String("foo5"));   // whoops, wrong name
    attribute5->setValue(zsLib::String("5"));
    element1->adoptAsLastChild(attribute5);
    attribute5->setName(zsLib::String("value5"));

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute6(::zsLib::XML::Attribute::create());
    attribute6->setName(zsLib::String("value6"));
    attribute6->setValue(zsLib::String("6"));
    BOOST_CHECK(zsLib::XML::NodePtr() == attribute6->getFirstSibling()) // haven't added yet so shouldn't have any
    BOOST_CHECK(zsLib::XML::NodePtr() == attribute6->getLastSibling()) // haven't added yet so shouldn't have any
    element1->adoptAsLastChild(attribute6);

    BOOST_CHECK(attribute5 == attribute6->getFirstSibling())
    BOOST_CHECK(attribute6 == attribute5->getLastSibling())

    treeWalkChecker(document);

    caught = false;
    try {(attribute6->toNode())->adoptAsFirstChild(attribute7);} catch(...) {caught = true;}
    BOOST_CHECK(caught)
    caught = false;
    try {(attribute6->toNode())->adoptAsLastChild(attribute7);} catch(...) {caught = true;}
    BOOST_CHECK(caught)

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute8(::zsLib::XML::Attribute::create());
    attribute8->setName(zsLib::String("value8"));
    attribute8->setValue(zsLib::String("8"));
    attribute6->adoptAsNextSibling(attribute8);

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute9(::zsLib::XML::Attribute::create());
    attribute9->setName(zsLib::String("value9"));
    attribute9->setValue(zsLib::String("9"));
    attribute6->adoptAsPreviousSibling(attribute9);
    BOOST_CHECK(0 == strcmp(attribute9->getValue(), "9"))

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute10(::zsLib::XML::Attribute::create());
    attribute10->setName(zsLib::String("value10"));
    attribute10->setValue(zsLib::String("10"));
    attribute4->adoptAsNextSibling(attribute10);

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute11(::zsLib::XML::Attribute::create());
    attribute11->setName(zsLib::String("value11"));
    attribute11->setValue(zsLib::String("11"));
    attribute4->adoptAsNextSibling(attribute11);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text7(::zsLib::XML::Text::create());
    text7->setValue(zsLib::String("text7"));

    caught = false;
    try {attribute4->adoptAsNextSibling(text7);} catch(...) {caught = true;}
    BOOST_CHECK(caught)

    treeWalkChecker(document);

    caught = false;
    try {attribute4->adoptAsPreviousSibling(text7);} catch(...) {caught = true;}
    BOOST_CHECK(caught)

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute12(::zsLib::XML::Attribute::create());
    attribute12->setName(zsLib::String("value12"));
    attribute12->setValue(zsLib::String("12"));
    declaration1->getFirstAttribute()->adoptAsPreviousSibling(attribute12);

    treeWalkChecker(document);

    zsLib::XML::AttributePtr attribute13(::zsLib::XML::Attribute::create());
    attribute13->setName(zsLib::String("value13"));
    attribute13->setValue(zsLib::String("13"));
    element1->getFirstAttribute()->adoptAsPreviousSibling(attribute13);

    treeWalkChecker(document);

    zsLib::XML::CommentPtr comment1(::zsLib::XML::Comment::create());
    comment1->setValue(zsLib::String("comment1"));
    element1->adoptAsFirstChild(comment1);
    BOOST_CHECK(0 == strcmp(comment1->getValue(), "comment1"))

    treeWalkChecker(document);

    zsLib::XML::ElementPtr element2(::zsLib::XML::Element::create());
    element2->setValue(zsLib::String("text"));
    document->adoptAsLastChild(element2);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text8(::zsLib::XML::Text::create());
    text8->setValue(zsLib::String(" this  &lt;is&gt; \n" "a &#96;test&#96; of &nbsp;the "));
    element2->adoptAsLastChild(text8);

    treeWalkChecker(document);

    zsLib::XML::ElementPtr element3(::zsLib::XML::Element::create());
    element3->setValue(zsLib::String("wild"));
    element2->adoptAsLastChild(element3);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text9(::zsLib::XML::Text::create());
    text9->setValue(zsLib::String(" text "));
    element3->adoptAsLastChild(text9);

    zsLib::XML::TextPtr text10(::zsLib::XML::Text::create());
    text10->setValue(zsLib::String("\telement\n\n"));
    element3->adoptAsNextSibling(text10);

    treeWalkChecker(document);

    "<text> this  &lt;is&gt; \n"
    "a &#96;test&#96; of &nbsp;the <wild> text </wild>\telement\n"
    "\n"
    "</text>";

    zsLib::String result1 = element2->getText(false, true);
    BOOST_CHECK(0 == strcmp(result1, " this  &lt;is&gt; \n" "a &#96;test&#96; of &nbsp;the " " text " "\telement\n" "\n"))

    zsLib::String result2 = element2->getText(false, false);
    BOOST_CHECK(0 == strcmp(result2, " this  &lt;is&gt; \n" "a &#96;test&#96; of &nbsp;the " "\telement\n" "\n"))

    zsLib::String result3 = element2->getText(true, true);
    BOOST_CHECK(0 == strcmp(result3, "this &lt;is&gt; " "a &#96;test&#96; of &nbsp;the " "text " "element"))

    zsLib::String result4 = element2->getText(true, false);
    BOOST_CHECK(0 == strcmp(result4, "this &lt;is&gt; " "a &#96;test&#96; of &nbsp;the " "element"))

    result1 = zsLib::XML::Parser::convertFromEntities(result1);
    BOOST_EQUAL(result1, " this  <is> \n" "a `test` of  the " " text " "\telement\n" "\n")

    result1 = zsLib::XML::Parser::convertFromEntities(zsLib::String("&#x23;&#x4c;O&#x4c;&nbsp;&bogus;&#ab;&#xyz;&+;&#195;"));
    result2 = "#LOL &bogus;&#ab;&#xyz;&+;\xC3\x83";
    BOOST_EQUAL(result1, result2)

    result1 = zsLib::XML::Parser::makeTextEntitySafe(zsLib::String("&<>;  \"\'"));
    result2 = "&amp;&lt;&gt;;  \"\'";
    BOOST_CHECK(0 == strcmp(result1, result2))

    result1 = zsLib::XML::Parser::makeAttributeEntitySafe(zsLib::String(zsLib::String("&<>;  \"\'")));
    result2 = "&amp;&lt;&gt;;  &quot;&apos;";
    BOOST_CHECK(0 == strcmp(result1, result2))

    result1 = zsLib::XML::Parser::makeAttributeEntitySafe(zsLib::String("&<>;  \""));
    result2 = "&amp;&lt;&gt;;  \"";
    BOOST_CHECK(0 == strcmp(result1, result2))

    result1 = zsLib::XML::Parser::makeAttributeEntitySafe(zsLib::String("&<>;  \'"));
    result2 = "&amp;&lt;&gt;;  \'";
    BOOST_CHECK(0 == strcmp(result1, result2))

    zsLib::XML::TextPtr text11(::zsLib::XML::Text::create());
    text11->setValue(zsLib::String("\n"));
    element2->adoptAsNextSibling(text11);

    treeWalkChecker(document);

    zsLib::XML::ElementPtr element4(::zsLib::XML::Element::create());
    element4->setValue(zsLib::String("encoding"));
    element4->setAttribute(zsLib::String("value1"), zsLib::XML::Parser::makeAttributeEntitySafe(zsLib::String("\"\'")));
    element4->setAttribute(zsLib::String("value2"), zsLib::XML::Parser::makeAttributeEntitySafe(zsLib::String("\"")));
    element4->setAttribute(zsLib::String("value3"), zsLib::XML::Parser::makeAttributeEntitySafe(zsLib::String("\'")));
    text11->adoptAsNextSibling(element4);

    treeWalkChecker(document);

    zsLib::XML::TextPtr text12(::zsLib::XML::Text::create());
    text12->setValue(zsLib::String("<cdata test & result>"), true);
    text12->setOutputCDATA(true);
    BOOST_CHECK(text12->getOutputCDATA())

    zsLib::String cdataResult = text12->getValue();
    BOOST_CHECK(0 == strcmp(cdataResult, "&lt;cdata test &amp; result&gt;"))

    element4->adoptAsFirstChild(text12);

    treeWalkChecker(document);

    zsLib::XML::UnknownPtr unknown1(::zsLib::XML::Unknown::create());
    unknown1->setValue(zsLib::String("!unknown!"));
    BOOST_CHECK(0 == strcmp(unknown1->getValue(), "!unknown!"))
    caught = false;
    try {(unknown1->toNode())->adoptAsFirstChild(attribute7);} catch(...) {caught = true;}
    BOOST_CHECK(caught)
    caught = false;
    try {(unknown1->toNode())->adoptAsLastChild(attribute7);} catch(...) {caught = true;}
    BOOST_CHECK(caught)
    element4->adoptAsNextSibling(unknown1);
    treeWalkChecker(document);

    /*
     "<?xml value12=\"12\" value2=\"2\" value1=\"1\" value3=\"foo4\" value11=\"11\" value10=\"10\" ?>\n"
     "insert previous which is first sibling\n"
     "<test value13=\"13\" value5=\"5\" value9=\"9\" value6=\"6\" value8=\"8\"><!--comment1-->text</test>\n"
     "insert previous sibling\n"
     "insert next sibling\n"
     "\n"
     "insert next which is last sibling\n"
     "<text> this  &lt;is&gt; \n"
     "a &#96;test&#96; of &nbsp;the <wild> text </wild>\telement\n"
     "\n"
     "</text>\n"
     "<encoding value1=\"&quot;&apos;\" value2=\'\"\' value3=\"\'\"><![CDATA[<cdata test & result>]]></encoding><!unknown!>";
     */

    // check attributes
    BOOST_CHECK(0 == strcmp(element4->getAttributeValue(zsLib::String("value1")), "&quot;&apos;"))
    BOOST_CHECK(0 == strcmp(element4->getAttributeValue(zsLib::String("bogus")), ""))

    // check element walker
    BOOST_CHECK(element4->findAttribute(zsLib::String("value3"))->getParentElement().get() == element4.get())
    BOOST_CHECK(!(document->getParentElement()))
    BOOST_CHECK(!(document->getFirstSiblingElement()))
    BOOST_CHECK(!(document->getLastSiblingElement()))
    BOOST_CHECK(!(document->getPreviousSiblingElement()))
    BOOST_CHECK(!(document->getNextSiblingElement()))
    BOOST_CHECK(document->getFirstChildElement() == element1)
    BOOST_CHECK(document->getFirstChildElement()->getLastSiblingElement() == element4)
    BOOST_CHECK(document->getFirstChildElement()->getNextSiblingElement() == element2)
    BOOST_CHECK(!(element1->getFirstChildElement()))
    BOOST_CHECK(!(element1->getLastChildElement()))
    BOOST_CHECK(document->getLastChildElement() == element4)
    BOOST_CHECK(document->getLastChildElement()->getFirstSiblingElement() == element1)
    BOOST_CHECK(document->getLastChildElement()->getPreviousSiblingElement() == element2)

    BOOST_CHECK(document->findFirstChildElement(zsLib::String("test")) == element1)
    BOOST_CHECK(document->findFirstChildElement(zsLib::String("text")) == element2)
    BOOST_CHECK(!(document->findFirstChildElement(zsLib::String("wild"))))
    BOOST_CHECK(document->findFirstChildElement(zsLib::String("text"))->findFirstChildElement(zsLib::String("wild")) == element3)
    BOOST_CHECK(document->findFirstChildElement(zsLib::String("encoding")) == element4)

    //test case
    BOOST_CHECK(document->isElementNameIsCaseSensative())
    BOOST_CHECK(!(document->findFirstChildElement(zsLib::String("TEST"))))

    // normally don't change case sensativity after a document is formed but this is a special case
    document->setElementNameIsCaseSensative(false);
    BOOST_CHECK(document->findFirstChildElement(zsLib::String("TEST")) == element1)
    document->setElementNameIsCaseSensative(true);


    ULONG length = 0;
    boost::shared_array<char> output = document->write(&length);

    BOOST_CHECK(length == strlen(output.get()))

    // std::cout << "--- SOR ---\n" << output << "--- EOR ---\n";

    BOOST_CHECK(0 == strcmp(gOutput, output.get()))

    document->clear();
    BOOST_CHECK(zsLib::XML::NodePtr() == document->getLastChild())

    {int i = 0; ++i;}
  }

  void parserPosTestConst(const zsLib::XML::ParserPos &constPos)
  {
    zsLib::XML::ParserPos temp1(++constPos);
    BOOST_CHECK(temp1.isString("OL"))
    zsLib::XML::ParserPos temp2(--constPos);
    BOOST_CHECK(temp2.isString(" EOL"))
    ULONG length1 = (ULONG)(temp1 - temp2);
    BOOST_CHECK(2 == length1)

    // move the end to the EOF
    temp1.setEOF();
    length1 = (ULONG)(temp1 - temp2);
    BOOST_CHECK(strlen(" EOL") == length1)

    BOOST_CHECK(0 == strcmp((CSTR)temp2, " EOL"))

    // reverse the positions
    zsLib::XML::ParserPos temp1a(temp2);
    zsLib::XML::ParserPos temp2a(temp1);
    size_t length1a = temp1a - temp2a;
    BOOST_CHECK((-1*strlen(" EOL")) == length1a)

    temp1a.setEOF();
    temp2a.setEOF();
    length1a = temp1a - temp2a;
    BOOST_CHECK(0 == length1a)
    BOOST_CHECK(NULL != ((CSTR)temp1a))
    if (NULL != ((CSTR)temp1a))
    {
      BOOST_CHECK(0 == strcmp((CSTR)temp1a, ""))
    }

    // cause it to go the physical
    --temp2a;
    ++temp2a;
    length1a = temp1a - temp2a;
    BOOST_CHECK(0 == length1a)
  }

  void parserPosTest()
  {
    static CSTR gParse =
    "<test>this</test> DOS EOL\r\n"
    "whatever dude!\r"
    "WAS MAC EOL";
    zsLib::XML::DocumentPtr document = zsLib::XML::Document::create();
    document->parse(gParse);
    zsLib::XML::ParserPos pos(document);
    BOOST_CHECK(pos.isSOF())
    BOOST_CHECK(pos.isString("<test>"))

    pos.setEOF();
    BOOST_CHECK(pos.isEOF())

    pos -= (ULONG)strlen("EOL");
    BOOST_CHECK(pos.isString("EOL"))
    parserPosTestConst(pos);

    pos.setSOF();
    BOOST_CHECK(pos.isSOF())

    ++pos;

    BOOST_CHECK(pos.isString("test>this"))

    --pos;
    --pos;   // try and force the pos to be before the start of the parse blob

    // that attempt should fail...
    BOOST_CHECK(pos.isString("<test>"))
    BOOST_CHECK(1 == pos.mRow)
    BOOST_CHECK(1 == pos.mColumn)

    // advance to EOL
    pos += (ULONG)(strstr(gParse, "\r\n") - gParse);
    BOOST_CHECK(pos.isString("\r\n"))
    BOOST_CHECK(1 == pos.mRow)
    BOOST_CHECK(pos.mColumn == ((ULONG)(strstr(gParse, "\r\n") - gParse))+1)

    // advance passed the \r which should force the column to 1
    ++pos;
    BOOST_CHECK(pos.isString("\n"))
    BOOST_CHECK(1 == pos.mRow)
    BOOST_CHECK(pos.mColumn == ((ULONG)(strstr(gParse, "\r\n") - gParse))+1)

    ++pos;
    BOOST_CHECK(pos.isString("whatever"))
    BOOST_CHECK(2 == pos.mRow)
    BOOST_CHECK(1 == pos.mColumn)

    --pos;
    BOOST_CHECK(pos.isString("\n"))
    BOOST_CHECK(1 == pos.mRow)
    BOOST_CHECK(pos.mColumn == ((ULONG)(strstr(gParse, "\r\n") - gParse))+1)

    ++pos;
    pos += (ULONG)strlen("whatever dude!");
    BOOST_CHECK(pos.isString("\r"))
    BOOST_CHECK(2 == pos.mRow)
    BOOST_CHECK(pos.mColumn == ((ULONG)strlen("whatever dude!")+1))

    ++pos;
    BOOST_CHECK(pos.isString("WAS MAC EOL"))
    BOOST_CHECK(3 == pos.mRow)
    BOOST_CHECK(1 == pos.mColumn)

    --pos;
    BOOST_CHECK(pos.isString("\r"))
    BOOST_CHECK(2 == pos.mRow)
    BOOST_CHECK(pos.mColumn == ((ULONG)strlen("whatever dude!")+1))

    pos.setSOF();
    pos += 3;
    zsLib::XML::ParserPos pos2(pos - 2);
    BOOST_CHECK(pos2.isString("test>this"))

    zsLib::XML::ParserPos pos3(pos - strlen("es"));
    BOOST_CHECK(pos3.isString("test>this"))

    {int i = 0; ++i;}
  }
};

BOOST_AUTO_TEST_SUITE(zsLibXMLTest)

  BOOST_AUTO_TEST_CASE(TestXML)
  {
    TestXML test;
  }

BOOST_AUTO_TEST_SUITE_END()
