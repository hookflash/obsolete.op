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

#include <openpeer/services/internal/services_CanonicalXML.h>
#include <zsLib/XML.h>

#include <list>

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }


namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      typedef std::map<String, String> XMLNamespaceMap;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static void calculateNamespaceTo(
                                       ElementPtr element,
                                       XMLNamespaceMap &outNamespaces
                                       )
      {
        if (!element)
          return;

        ElementPtr current = element->getRootElement();
        while (current)
        {
          // walk all the attributes in the namespace for overrides to the namespace map
          {
            AttributePtr attribute = current->getFirstAttribute();
            AttributePtr next;
            for (; attribute; attribute = next) {
              NodePtr tmp = attribute->getNextSibling();
              if (tmp) {
                next = tmp->toAttribute();
              } else {
                next.reset();
              }

              String name = attribute->getName();
              if (name.size() < 5)
                continue;

              if (name.substr(0, 5) == "xmlns") {
                // this is a namespace, override the current namespace

                String value = attribute->getValue();
                if (value.isEmpty()) {
                  XMLNamespaceMap::iterator found = outNamespaces.find(name);
                  if (found != outNamespaces.end())
                    outNamespaces.erase(found);
                } else {
                  outNamespaces[name] = value;
                }
              }
            }
          }

          if (current == element)
            break;  // we are now done

          // calculate the new current root
          ElementPtr found = element;
          ElementPtr parent = found->getParentElement();
          while (parent) {
            if (parent == current)
              break;  // do not redo the already processed parent
            found = parent;
            parent = parent->getParentElement();
          }

          current = found;
        }
      }

      //-----------------------------------------------------------------------
      static void calculateOverrides(
                                     ElementPtr element,
                                     XMLNamespaceMap &namespaces,
                                     XMLNamespaceMap &outOverrides
                                     )
      {
        if (!element)
          return;

        XMLNamespaceMap namespacesToParent = namespaces;

        ElementPtr parent = element->getParentElement();
        if (parent) {
          calculateNamespaceTo(parent, namespacesToParent);
        }

        AttributePtr attribute = element->getFirstAttribute();
        AttributePtr next;
        for (; attribute; attribute = next) {
          NodePtr tmp = attribute->getNextSibling();
          if (tmp) {
            next = tmp->toAttribute();
          } else {
            next.reset();
          }

          String name = attribute->getName();
          if (name.size() < 5)
            continue;

          if (name.substr(0, 5) == "xmlns") {

            // this is a namespace, override the parent namespace
            String value = attribute->getValue();

            XMLNamespaceMap::iterator found = namespacesToParent.find(name);
            if (found == namespacesToParent.end()) {
              // this was not in the parent thus it is an override (if not set to empty)
              if (!value.isEmpty())
                outOverrides[name] = value;
            } else {
              // this was found in the parent, check to see if the value changed
              String parentValue = (*found).second;
              if (parentValue != value)
                outOverrides[name] = value;
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      static void changeAttribtues(
                                   ElementPtr element,
                                   XMLNamespaceMap &overrides
                                   )
      {
        AttributePtr attribute = element->getFirstAttribute();
        AttributePtr next;
        for (; attribute; attribute = next) {
          NodePtr tmp = attribute->getNextSibling();
          if (tmp) {
            next = tmp->toAttribute();
          } else {
            next.reset();
          }

          // first strip out any namespaces

          String name = attribute->getName();
          if (name.size() < 5)
            continue;

          if (name.substr(0, 5) == "xmlns") {
            attribute->orphan();
          }
        }

        // now put back all the new XML element attrbutes namespaces
        for (XMLNamespaceMap::iterator iter = overrides.begin(); iter != overrides.end(); ++iter)
        {
          element->setAttribute((*iter).first, (*iter).second);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MyXMLNSWalkSink
      #pragma mark

      //-----------------------------------------------------------------------
      class MyXMLNSWalkSink : public WalkSink
      {
      public:
        //---------------------------------------------------------------------
        MyXMLNSWalkSink(
                        XMLNamespaceMap &xmlns,
                        ElementPtr &root
                        ) : mRootXMLNS(xmlns), mRoot(root) {}

        virtual bool onElementEnter(ElementPtr inNode) {
          if (inNode == mRoot) return false;  // already processed the root element

          XMLNamespaceMap xmlns;
          calculateOverrides(inNode, mRootXMLNS, xmlns);
          changeAttribtues(inNode, xmlns);
          return false;
        }
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark MyXMLNSWalkSink => (data)
        #pragma mark

        XMLNamespaceMap mRootXMLNS;
        ElementPtr mRoot;
      };

      //---------------------------------------------------------------------
      static bool normizeCompare(AttributePtr v1, AttributePtr v2)
      {
        if (v1->getName().substr(0, 5) == "xmlns") {
          if (v2->getName().substr(0, 5) == "xmlns") {
            return v1->getName() < v2->getName();
          }
          return true;
        } else if (v2->getName().substr(0, 5) == "xmlns") {
          return false;
        }
        return v1->getName() < v2->getName();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MyOrphanerWalkSink
      #pragma mark

      //---------------------------------------------------------------------
      class MyOrphanerWalkSink : public WalkSink
      {
        typedef std::list<AttributePtr> AttributeValueList;

      public:
        virtual bool onElementEnter(ElementPtr inNode) {
          // sort the attributes
          AttributeValueList attributes;
          AttributePtr attribute = inNode->getFirstAttribute();
          while (attribute) {
            NodePtr next = attribute->getNextSibling();
            attributes.push_back(attribute);
            attribute->orphan();
            if (!next)
              break;
            attribute = next->toAttribute();
          }

          // The sort isn't proper because it doesn't take into account the
          // primary key is the URI of the attribute not just the lexical name
          // but will fix this one day... if we ever use name spaces in the
          // attributes! (which we don't right now)

          // sort them - namespaces go first, then other attributes
          attributes.sort(normizeCompare);

          // we orphaned all attributes - now going to put them back in order
          AttributePtr last;
          // now put them all back!
          for (AttributeValueList::iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
            if (!last) {
              inNode->setAttribute(*iter);
              last = (*iter);
            } else {
              last->adoptAsNextSibling(*iter);
              last = (*iter);
            }
          }
          attributes.clear();
          return false;
        }

        //---------------------------------------------------------------------
        virtual bool onComment(CommentPtr inNode)
        {
          inNode->orphan();
          return false;
        }

        //---------------------------------------------------------------------
        virtual bool onDeclarationEnter(DeclarationPtr inNode)
        {
          inNode->orphan();
          return false;
        }

        //---------------------------------------------------------------------
        virtual bool onUnknown(UnknownPtr inNode)
        {
          String value = inNode->getValue();
          if (value.substr(0, 1) == "!") {
            // orphan DTDs
            inNode->orphan();
          }
          return false;
        }
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CanonicalXML
      #pragma mark

      //-----------------------------------------------------------------------
      String CanonicalXML::convert(ElementPtr element)
      {
        if (!element)
          return String();

        XMLNamespaceMap xmlns;
        calculateNamespaceTo(element, xmlns);

        ElementPtr original = element;
        ElementPtr newRoot = (original->clone())->toElementChecked();

        changeAttribtues(newRoot, xmlns);

        MyXMLNSWalkSink nswalker(xmlns, newRoot);
        newRoot->walk(nswalker, Node::NodeType::Element);

        MyOrphanerWalkSink orphanWalker;
        Node::FilterList filter;
        filter.push_back(Node::NodeType::Element);
        filter.push_back(Node::NodeType::Comment);
        filter.push_back(Node::NodeType::Declaration);
        filter.push_back(Node::NodeType::Unknown);

        newRoot->walk(orphanWalker,  &filter);

        DocumentPtr doc = Document::create();
        doc->adoptAsFirstChild(newRoot);

        GeneratorPtr generator = Generator::createXMLGenerator(static_cast<Generator::XMLWriteFlags>(Generator::XMLWriteFlag_ForceElementEndTag | Generator::XMLWriteFlag_NormalizeCDATA | Generator::XMLWriteFlag_EntityEncode0xDInText | Generator::XMLWriteFlag_NormizeAttributeValue));

        boost::shared_array<char> output = generator->write(doc);
        return (CSTR)output.get();
      }
    }

    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    #pragma mark
    #pragma mark ICanonicalXML
    #pragma mark

    //---------------------------------------------------------------------
    String ICanonicalXML::convert(ElementPtr element)
    {
      return internal::CanonicalXML::convert(element);
    }
  }
}
