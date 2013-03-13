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

#include "helpers.h"

using namespace zsLib;

namespace hookflash
{
  namespace stack
  {
    namespace test
    {
#pragma mark
#pragma mark XML helpers
#pragma mark
      //-----------------------------------------------------------------------
      XML::ElementPtr createFromString(const String &elementStr)
      {
        if (!elementStr) return XML::ElementPtr();
        
        XML::DocumentPtr doc = XML::Document::createFromParsedJSON(elementStr);
        
        XML::ElementPtr childEl = doc->getFirstChildElement();
        if (!childEl) return XML::ElementPtr();
        
        childEl->orphan();
        return childEl;
      }
      
      //-----------------------------------------------------------------------
      String convertToString(const XML::ElementPtr &element)
      {
        if (!element) return String();
        
        XML::GeneratorPtr generator = XML::Generator::createJSONGenerator();
        boost::shared_array<char> output = generator->write(element);
        
        return output.get();
      }
      
      bool writeToFile(zsLib::String text)
      {
        std::ofstream myfile ("/tmp/peerfile.txt");
        if (myfile.is_open())
        {
          myfile << text;
          myfile.close();
          return true;
        }
        else
        {
          std::cout << "Unable to open file";
          return false;
        }
      }
      bool readFromFile(String &outPassword, String &outText)
      {
        zsLib::String line;
        std::ifstream myfile ("/tmp/peerfile.txt");
        if (myfile.is_open())
        {
          int i = 0;
          while ( myfile.good() )
          {
            getline (myfile,line);
            if (i == 0)
            {
              outPassword = line;
              std::cout << line << std::endl;
            }
            else{
              outText += line;
              std::cout << line << std::endl;
            }
            ++i;
          }
          myfile.close();
          return true;
        }
        
        else
        {
          std::cout << "Unable to open file";
          return false;
        }
      }
    } // namespace test
  } // namespace stack
} // namespace hookflash
