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

#include <openpeer/stack/internal/stack_Diff.h>

#include <zsLib/Log.h>
#include <zsLib/Numeric.h>
#include <zsLib/XML.h>

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Numeric;

      typedef String ElementName;
      typedef int Index;
      typedef String AttributeIDValue;
      typedef std::pair<AttributeIDValue, Index> IndexPair;
      typedef std::pair<ElementName, IndexPair> PathComponent;
      typedef std::list<PathComponent> PathComponentList;
      typedef IDiff::DiffActions DiffActions;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      const char *toActionString(DiffActions action)
      {
        switch (action)
        {
          case IDiff::DiffAction_None:                   return "";
          case IDiff::DiffAction_Replace:                return ".";
          case IDiff::DiffAction_AdoptAsPreviousSibling: return "^";
          case IDiff::DiffAction_AdoptAsNextSibling:     return "v";
          case IDiff::DiffAction_AdoptAsFistChild:       return "fc";
          case IDiff::DiffAction_AdoptAsLastChild:       return "lc";
          case IDiff::DiffAction_Remove:                 return "-";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      DiffActions toAction(const char *str)
      {
        String compare(str ? str : "");
        for (ULONG loop = (ULONG)IDiff::DiffAction_First; loop <= ((ULONG)IDiff::DiffAction_Last); ++loop)
        {
          if (compare == toActionString((DiffActions)loop)) {
            return (DiffActions)loop;
          }
        }
        return IDiff::DiffAction_None;
      }

      //-----------------------------------------------------------------------
      static bool parsePathList(
                                const String &inPath,
                                PathComponentList &outPathList
                                )
      {
        outPathList.clear();

        // scope: process
        String path = inPath;
        while (true)
        {
          // find the "/"...
          size_t slashPos = path.find('/');

          String subPath;
          String attributeIDValue;
          Index index = 0;

          if (slashPos != String::npos) {
            subPath = path.substr(0, slashPos);
            path = path.substr(slashPos+1);
          } else {
            subPath = path;
            path.clear();
          }

          if (subPath.size() < 1) {
            if (path.size() < 1)
              return true;

            continue; // filter out empty subpaths
          }

          // find the "[x]" value at the end of the subpath
          size_t bracketOpenPos = subPath.find('[');
          if (bracketOpenPos != String::npos) {
            size_t bracketClosePos = subPath.find(']');
            if (bracketClosePos == String::npos) return false;
            if (bracketOpenPos > bracketClosePos) return false;

            String indexStr = subPath.substr(bracketOpenPos+1, bracketClosePos - bracketOpenPos - 1);
            subPath = subPath.substr(0, bracketOpenPos);

            if (indexStr.size() < 1) {
              indexStr = "-1";
            }

            if ((*(indexStr.c_str()) == '\"') &&
                (*(indexStr.c_str() + indexStr.size()-1) == '\"') &&
                (indexStr.size() > 1)) {
              // surrounded with quotes thus treat this as an element by "ID" lookup
              attributeIDValue = indexStr.substr(1, indexStr.size()-2);
            } else {
              try {
                index = (int)(Numeric<Index>(indexStr));
              } catch(Numeric<Index>::ValueOutOfRange &) {
                ZS_LOG_WARNING(Detail, "Failed to parse diff path list since index was not a number, index=" + indexStr)
                return false;
              }
            }
          }

          if (subPath.size() < 1) break;  // are we done processing?

          outPathList.push_back(PathComponent(subPath, IndexPair(attributeIDValue, index)));
        }
        return true;
      }

      //-----------------------------------------------------------------------
      static ElementPtr findPath(
                                 DocumentPtr originalDocument,
                                 const PathComponentList &pathList
                                 )
      {
        ElementPtr current;

        for (PathComponentList::const_iterator iter = pathList.begin(); iter != pathList.end(); ++iter)
        {
          const PathComponent &component = (*iter);
          IndexPair indexPair = component.second;
          String attributeIDName = indexPair.first;
          Index index = indexPair.second;

          NodePtr parent = (current ? current->toNode() : originalDocument->toNode());

          if (index >= 0) {
            current = parent->findFirstChildElement(component.first);
          } else {
            current = parent->findLastChildElement(component.first);
          }

          if (attributeIDName.size() > 0) {
            while (true) {
              if (!current) return ElementPtr();
              AttributePtr attributeID = current->findAttribute("id");
              if (attributeID->getValue() == attributeIDName) {
                // found what we were looking for...
                break;
              }
              current = current->findNextSiblingElement(component.first);
            }
          } else {
            if (-1 == index) ++index; // -1 is always last child...

            while (0 != index) {
              if (!current) return ElementPtr();

              if (index >= 0) {
                current = current->findNextSiblingElement(component.first);
                --index;
              } else {
                current = current->findPreviousSiblingElement(component.first);
                ++index;
              }
            }
          }

          if (!current) return ElementPtr();
        }

        return current;
      }

      //-----------------------------------------------------------------------
      static void createDiffDocument(
                                     DocumentPtr &ioDiffDocument,
                                     ElementPtr &outDiffEl
                                     )
      {
        if (ioDiffDocument) {
          outDiffEl = ioDiffDocument->findLastChildElement(OPENPEER_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);
          return;
        }

        DocumentPtr doc = Document::create();
        ElementPtr diffEl = Element::create(OPENPEER_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME);
        doc->adoptAsLastChild(diffEl);

        ioDiffDocument = doc;
        outDiffEl = diffEl;
      }

      //-----------------------------------------------------------------------
      static ElementPtr createDiffXDElement(
                                            ElementPtr path,
                                            DiffActions action
                                            )
      {
        typedef std::list<String> PathList;

        ElementPtr itemEl = Element::create(OPENPEER_STACK_DIFF_DOCUMENT_ITEM_ELEMENT_NAME);
        itemEl->setAttribute("do", toActionString(action));

        // calculate the path...
        PathList paths;

        while (path)
        {
          // find out the index of this element
          bool foundOtherWithID = false;
          bool foundOtherElements = false;

          String id = path->getAttributeValue("id");
          Index index = 0;

          String name = path->getValue();

          ElementPtr prevSibling = path->findPreviousSiblingElement(name);
          while (prevSibling)
          {
            foundOtherElements = true;
            ++index;
            if (foundOtherWithID) continue;

            String sibID = prevSibling->getAttributeValue("id");
            if (sibID == id) foundOtherWithID = true;

            prevSibling = prevSibling->findPreviousSiblingElement(name);
          }

          ElementPtr nextSibling = path->findNextSiblingElement(name);
          while (nextSibling) {
            foundOtherElements = true;
            if (foundOtherWithID) break;
            String sibID = nextSibling->getAttributeValue("id");
            if (sibID == id) {
              foundOtherWithID = true;
              break;
            }

            nextSibling = nextSibling->findNextSiblingElement(name);
          }

          String subPathStr;
          if (foundOtherElements) {
            // will need some kind of index since there are other elements with the same name at this level
            if (foundOtherWithID) {
              // needs to be a numeric index as there are multiple elements with the same ID
              subPathStr = name + "[" + string(index) + "]";
            } else {
              if ((String::npos == id.find('\"')) &&
                  (String::npos == id.find('\''))) {
                // safe to use the ID since it does not include a double quote or single quote in the string
                subPathStr += name + "[\"" + id + "\"]";
              } else {
                // not safe to use the ID since it contains a quote so must use a numeric index instead
                subPathStr = name + "[" + string(index) + "]";
              }
            }
          } else {
            // safe to use the name without any index since its the only element with this name at this level
            subPathStr = name;
          }

          paths.push_front(subPathStr);

          path = path->getParentElement();
        }

        String result;
        for (PathList::iterator iter = paths.begin(); iter != paths.end(); ++iter)
        {
          result += "/" + (*iter);
        }

        itemEl->setAttribute("path", result);
        return itemEl;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Diff
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Diff => IDiff
      #pragma mark

      //-----------------------------------------------------------------------
      bool Diff::process(
                         DocumentPtr originalDocument,
                         DocumentPtr diffDocument
                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!originalDocument)
        ZS_THROW_INVALID_ARGUMENT_IF(!diffDocument)

        // scope: processing the diff document
        {
          ElementPtr diffElem = diffDocument->findFirstChildElement(OPENPEER_STACK_DIFF_DOCUMENT_ROOT_ELEMENT_NAME); // root for diffs elements
          if (!diffElem) return false;

          ElementPtr itemEl = diffElem->findFirstChildElement(OPENPEER_STACK_DIFF_DOCUMENT_ITEM_ELEMENT_NAME);        // diff element
          while (itemEl)
          {
            String pathStr = itemEl->getAttributeValue("path");
            String doStr = itemEl->getAttributeValue("do");

            PathComponentList pathList;
            if (!parsePathList(pathStr, pathList)) {
              ZS_LOG_WARNING(Detail, "Failed to parse path list specified in document, path" + pathStr)
              return false;
            }

            ElementPtr pathElement = findPath(originalDocument, pathList);
            if (!pathElement) {
              ZS_LOG_WARNING(Detail, "Failed to find the path specified in the document" + pathStr)
              return false;
            }

            DiffActions action = DiffAction_None;

            action = toAction(doStr);
            if (IDiff::DiffAction_None == action) {
              if ("a" != doStr) {
                ZS_LOG_WARNING(Detail, "\'do\' attribute is not valid, do=" + doStr)
                return false;
              }
            }

            ZS_LOG_TRACE(String("Performing diff action: ") + (IDiff::DiffAction_None == action ? "attribute" : IDiff::toString(action)))

            switch (action) {
              case IDiff::DiffAction_None: {
                // we are using "NONE" to reprepsent the special case of "attribute"
                ElementPtr setElem = itemEl->findFirstChildElement("set");
                ElementPtr removeElem = itemEl->findFirstChildElement("remove");

                if (setElem) {
                  AttributePtr attrib = setElem->getFirstAttribute();
                  while (attrib) {
                    NodePtr next = attrib->getNextSibling();
                    pathElement->setAttribute(attrib->getName(), attrib->getValue());
                    if (!next) break;
                    attrib = next->toAttribute();
                  }
                }
                if (removeElem) {
                  AttributePtr attrib = removeElem->getFirstAttribute();
                  while (attrib) {
                    NodePtr next = attrib->getNextSibling();
                    pathElement->deleteAttribute(attrib->getName());
                    if (!next) break;
                    attrib = next->toAttribute();
                  }
                }
                break;
              }
              case IDiff::DiffAction_Replace: { // this element
                // keep adopting as next sibling in reverse order of the children
                NodePtr current = itemEl->getLastChild();
                while (current) {
                  NodePtr duplicate = current->clone();
                  pathElement->adoptAsNextSibling(duplicate);
                  current = current->getPreviousSibling();
                }
                pathElement->orphan();  // orphan the path element since it is being replaced
                break;
              }
              case IDiff::DiffAction_AdoptAsPreviousSibling: { // previous sibling
                // keep adopting as previous sibling in normal order of the children
                NodePtr current = itemEl->getFirstChild();
                while (current) {
                  NodePtr duplicate = current->clone();
                  pathElement->adoptAsPreviousSibling(duplicate);
                  current = current->getNextSibling();
                }
                break;
              }
              case IDiff::DiffAction_AdoptAsNextSibling: { // next sibling
                // keep adopting as next sibling in reverse order of the children
                NodePtr current = itemEl->getLastChild();
                while (current) {
                  NodePtr duplicate = current->clone();
                  pathElement->adoptAsNextSibling(duplicate);
                  current = current->getPreviousSibling();
                }
                break;
              }
              case IDiff::DiffAction_AdoptAsFistChild: { // first child
                // keep adopting as first child in reverse order of the children
                NodePtr current = itemEl->getLastChild();
                while (current) {
                  NodePtr duplicate = current->clone();
                  pathElement->adoptAsFirstChild(duplicate);
                  current = current->getPreviousSibling();
                }
                break;
              }
              case IDiff::DiffAction_AdoptAsLastChild: { // last child
                // keep adopting as last child in normal order of the children
                NodePtr current = itemEl->getFirstChild();
                while (current) {
                  NodePtr duplicate = current->clone();
                  pathElement->adoptAsLastChild(duplicate);
                  current = current->getNextSibling();
                }
                break;
              }
              case IDiff::DiffAction_Remove: { // remove
                // remove the element by orphaning it
                pathElement->orphan();
                break;
              }
            }

            itemEl = itemEl->findNextSiblingElement(OPENPEER_STACK_DIFF_DOCUMENT_ITEM_ELEMENT_NAME);
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      void Diff::createDiffs(
                             DiffActions action,
                             DocumentPtr &ioDiffDocument,
                             ElementPtr modifyAtElementPath,
                             bool modifyElementNow,
                             NodePtr newNode,
                             bool useChilrenOfNewNode
                             ) throw (Exceptions::NewNodeIsNotAnElement)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!modifyAtElementPath)

        ElementPtr holder;

        if (DiffAction_Remove != action) {
          if (DiffAction_Replace == action) {
            if (!newNode) {
              // told to replace, but replacing with nothing so do remove action instead
              createDiffs(DiffAction_Remove, ioDiffDocument, modifyAtElementPath, modifyElementNow, NodePtr(), false);
              return;
            }
          } else {
            if (!newNode) {
              ZS_LOG_WARNING(Detail, String("diff processor told to adopt node but no node was specified, action=") + IDiff::toString(action))
              return; // told to adopt in some way but not provided anything to adopt thus this is a NOOP
            }
          }

          if (!useChilrenOfNewNode) {
            // put newNode into a child of an element and call self
            ElementPtr temp = Element::create();
            temp->adoptAsLastChild(newNode);
            createDiffs(action, ioDiffDocument, modifyAtElementPath, modifyElementNow, temp, true);
            return;
          }

          ZS_THROW_CUSTOM_IF(Exceptions::NewNodeIsNotAnElement, !newNode->isElement())

          holder = newNode->toElement();
          if (!holder->hasChildren()) {
            ZS_LOG_WARNING(Detail, String("diff processor told to adopt or replace node but no new node was specified, action=") + IDiff::toString(action))
            return; // this is a NOOP since there are no children to process
          }
        }

        ElementPtr diffEl;
        // definiately will have soemthing to do now...
        createDiffDocument(ioDiffDocument, diffEl);

        ElementPtr itemEl = createDiffXDElement(modifyAtElementPath, action);

        if (holder) {
          // clone the children into the xd element...
          NodePtr child = holder->getFirstChild();
          while (child) {
            itemEl->adoptAsLastChild(child->clone());
            child = child->getNextSibling();
          }
        }

        // applies the change to the original document so document can remain up to date...
        switch (action) {
          case DiffAction_None:                     break;
          case DiffAction_Replace:                  {
            if (modifyElementNow) {
              NodePtr child = holder->getLastChild();
              while (child) {
                NodePtr next = child->getPreviousSibling();
                child->orphan();
                modifyAtElementPath->adoptAsPreviousSibling(child);
                child = next;
              }
              modifyAtElementPath->orphan();
            }
            break;
          }
          case DiffAction_AdoptAsPreviousSibling:   {
            if (modifyElementNow) {
              NodePtr child = holder->getLastChild();
              while (child) {
                NodePtr next = child->getPreviousSibling();
                child->orphan();
                modifyAtElementPath->adoptAsPreviousSibling(child);
                child = next;
              }
            }
            break;
          }
          case DiffAction_AdoptAsNextSibling:       {
            if (modifyElementNow) {
              NodePtr child = holder->getLastChild();
              while (child) {
                NodePtr next = child->getPreviousSibling();
                child->orphan();
                modifyAtElementPath->adoptAsNextSibling(child);
                child = next;
              }
            }
            break;
          }
          case DiffAction_AdoptAsFistChild:         {
            if (modifyElementNow) {
              NodePtr child = holder->getFirstChild();
              while (child) {
                NodePtr next = child->getNextSibling();
                child->orphan();
                modifyAtElementPath->adoptAsLastChild(child);
                child = next;
              }
            }
            break;
          }
          case DiffAction_AdoptAsLastChild:         {
            if (modifyElementNow) {
              NodePtr child = holder->getFirstChild();
              while (child) {
                NodePtr next = child->getNextSibling();
                child->orphan();
                modifyAtElementPath->adoptAsFirstChild(child);
                child = next;
              }
            }
            break;
          }
          case DiffAction_Remove:                   {
            if (modifyElementNow) {
              modifyAtElementPath->orphan();
            }
            break;
          }
        }

        diffEl->adoptAsLastChild(itemEl);
      }

      //-----------------------------------------------------------------------
      void Diff::createDiffsForAttributes(
                                          DocumentPtr &ioDiffDocument,
                                          ElementPtr modifyAtElementPath,
                                          bool modifyElementNow,
                                          ElementPtr setAttributesFromThisElement,
                                          ElementPtr removeAttributesFromThisElement
                                          )
      {
        if ((!setAttributesFromThisElement) &&
            (!removeAttributesFromThisElement)) return;

        bool appliedSet = false;
        bool appliedRemove = false;

        ElementPtr itemEl = createDiffXDElement(modifyAtElementPath, DiffAction_Replace);
        itemEl->setAttribute("do", "a");
        ElementPtr setEl = Element::create("set");
        ElementPtr removeEl = Element::create("remove");

        if (setAttributesFromThisElement) {
          AttributePtr attrib = setAttributesFromThisElement->getFirstAttribute();
          while (attrib) {
            // scope apply set change
            {
              AttributePtr previousAttribute = modifyAtElementPath->findAttribute(attrib->getName());
              if (previousAttribute) {
                if (previousAttribute->getValue() == attrib->getValue()) goto next_set; // this is a NOOP
              }
              setEl->setAttribute(attrib->getName(), attrib->getValue());
              if (modifyElementNow) {
                modifyAtElementPath->setAttribute(attrib->getName(), attrib->getValue());
              }
              appliedSet = true;
            }

          next_set:
            NodePtr next = attrib->getNextSibling();
            if (!next) break;
            attrib = next->toAttribute();
          }
        }

        if (removeAttributesFromThisElement) {
          AttributePtr attrib = removeAttributesFromThisElement->getFirstAttribute();
          while (attrib) {
            // scope apply remove change
            {
              AttributePtr previousAttribute = modifyAtElementPath->findAttribute(attrib->getName());
              if (!previousAttribute) goto next_remove; // this is a NOOP
              removeEl->setAttribute(attrib->getName(), "");
              if (modifyElementNow) {
                previousAttribute->orphan();
              }
              appliedRemove = true;
            }

          next_remove:
            NodePtr next = attrib->getNextSibling();
            if (!next) break;
            attrib = next->toAttribute();
          }
        }

        if ((!appliedSet) &&
            (!appliedRemove)) {
          // This is a NOOP so no changed are needed
          return;
        }

        if (appliedSet) {
          itemEl->adoptAsLastChild(setEl);
        }
        if (appliedRemove) {
          itemEl->adoptAsLastChild(removeEl);
        }

        ElementPtr diffEl;
        createDiffDocument(ioDiffDocument, diffEl);
        diffEl->adoptAsLastChild(itemEl);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IDiff
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IDiff::toString(DiffActions action)
    {
      switch (action)
      {
        case DiffAction_None:                   return "None";
        case DiffAction_Replace:                return "Replace";
        case DiffAction_AdoptAsPreviousSibling: return "Adopt as previous sibling";
        case DiffAction_AdoptAsNextSibling:     return "Adopt as next sibling";
        case DiffAction_AdoptAsFistChild:       return "Adopt as first child";
        case DiffAction_AdoptAsLastChild:       return "Adopt as last child";
        case DiffAction_Remove:                 return "Remove";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    bool IDiff::process(
                        DocumentPtr originalDocument,
                        DocumentPtr diffDocument
                        )
    {
      return internal::Diff::process(originalDocument, diffDocument);
    }

    //-------------------------------------------------------------------------
    void IDiff::createDiffs(
                            DiffActions action,
                            DocumentPtr &ioDiffDocument,
                            ElementPtr modifyAtElementPath,
                            bool modifyElementNow,
                            NodePtr newNode,
                            bool useChilrenOfNewNode
                            ) throw (Exceptions::NewNodeIsNotAnElement)
    {
      return internal::Diff::createDiffs(action, ioDiffDocument, modifyAtElementPath, modifyElementNow, newNode, useChilrenOfNewNode);
    }

    //-------------------------------------------------------------------------
    void IDiff::createDiffsForAttributes(
                                         DocumentPtr &ioDiffDocument,
                                         ElementPtr modifyAtElementPath,
                                         bool modifyElementNow,
                                         ElementPtr setAttributesFromThisElement,
                                         ElementPtr removeAttributesFromThisElement
                                         )
    {
      return internal::Diff::createDiffsForAttributes(ioDiffDocument, modifyAtElementPath, modifyElementNow, setAttributesFromThisElement, removeAttributesFromThisElement);
    }
  }
}
