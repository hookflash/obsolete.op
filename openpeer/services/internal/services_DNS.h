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

#pragma once

#include <openpeer/services/internal/types.h>
#include <openpeer/services/IDNS.h>

namespace openpeer
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IDNS
      #pragma mark

      class DNS : public IDNS
      {
        friend interaction IDNSFactory;

      protected:
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark DNS => IDNS
        #pragma mark

        static IDNSQueryPtr lookupA(
                                    IDNSDelegatePtr delegate,
                                    const char *name
                                    );

        static IDNSQueryPtr lookupAAAA(
                                       IDNSDelegatePtr delegate,
                                       const char *name
                                       );

        static IDNSQueryPtr lookupAorAAAA(
                                          IDNSDelegatePtr delegate,
                                          const char *name
                                          );

        static IDNSQueryPtr lookupSRV(
                                      IDNSDelegatePtr delegate,
                                      const char *name,
                                      const char *service,
                                      const char *protocol,
                                      WORD defaultPort,
                                      WORD defaultPriority,
                                      WORD defaultWeight,
                                      SRVLookupTypes lookupType
                                      );
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IDNSFactory
      #pragma mark

      interaction IDNSFactory
      {
        typedef IDNS::SRVLookupTypes SRVLookupTypes;

        static IDNSFactory &singleton();

        virtual IDNSQueryPtr lookupA(
                                     IDNSDelegatePtr delegate,
                                     const char *name
                                     );

        virtual IDNSQueryPtr lookupAAAA(
                                        IDNSDelegatePtr delegate,
                                        const char *name
                                        );

        virtual IDNSQueryPtr lookupAorAAAA(
                                           IDNSDelegatePtr delegate,
                                           const char *name
                                           );

        virtual IDNSQueryPtr lookupSRV(
                                       IDNSDelegatePtr delegate,
                                       const char *name,
                                       const char *service,
                                       const char *protocol,
                                       WORD defaultPort,
                                       WORD defaultPriority,
                                       WORD defaultWeight,
                                       SRVLookupTypes lookupType
                                       );
      };
      
    }
  }
}
