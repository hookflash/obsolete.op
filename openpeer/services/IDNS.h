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

#include <openpeer/services/types.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Proxy.h>

#include <list>

namespace openpeer
{
  namespace services
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IDNS
    #pragma mark

    interaction IDNS
    {
      //-----------------------------------------------------------------------
      struct AResult
      {
        typedef std::list<IPAddress> IPAddressList;

        String mName;
        UINT   mTTL;
        IPAddressList mIPAddresses;
      };
      typedef boost::shared_ptr<AResult> AResultPtr;

      typedef AResult AAAAResult;
      typedef AResultPtr AAAAResultPtr;

      //-----------------------------------------------------------------------
      struct SRVResult
      {
        struct SRVRecord
        {
          String mName;
          WORD mPriority;
          WORD mWeight;
          WORD mPort;

          AResultPtr mAResult;
          AAAAResultPtr mAAAAResult;
        };

        typedef boost::shared_ptr<SRVRecord> SRVRecordPtr;
        typedef std::list<SRVRecord> SRVRecordList;

        String mName;
        String mService;
        String mProtocol;
        UINT   mTTL;
        SRVRecordList mRecords;
      };
      typedef boost::shared_ptr<SRVResult> SRVResultPtr;
      typedef std::list<SRVResultPtr> SRVResultList;

      enum SRVLookupTypes {
        SRVLookupType_LookupOnly =               0x0000,                                                                // just do the basic SRV lookup and do not resolve any results or perform any fallback lookups
        SRVLookupType_AutoLookupA =              0x0001,                                                                // if the SRV lookup succeeds, will automatically perform an A-record lookup on the result
        SRVLookupType_AutoLookupAAAA =           0x0002,                                                                // if the SRV lookup succeeds, will automatically perform an AAAA-record lookup on the result
        SRVLookupType_AutoLookupAll =            SRVLookupType_AutoLookupA | SRVLookupType_AutoLookupAAAA,
        SRVLookupType_FallbackToALookup =        0x0004,                                                                // if the SRV lookup fails, will automatically perform an A-record on the name instead
        SRVLookupType_FallbackToAAAALookup =     0x0008,                                                                // if the SRV lookup fails, will automatically perform an A-record on the name instead
        SRVLookupType_FallbackAll =              SRVLookupType_FallbackToALookup | SRVLookupType_FallbackToAAAALookup,
        SRVLookupType_AutoLookupAndFallbackAll = SRVLookupType_AutoLookupAll | SRVLookupType_FallbackAll,
      };

      //-----------------------------------------------------------------------
      // PURPOSE: Perform an A-record lookup using a recursive DNS server.
      static IDNSQueryPtr lookupA(
                                  IDNSDelegatePtr delegate,
                                  const char *name
                                  );

      //-----------------------------------------------------------------------
      // PURPOSE: Perform an AAAA-record lookup using a recursive DNS server.
      static IDNSQueryPtr lookupAAAA(
                                     IDNSDelegatePtr delegate,
                                     const char *name
                                     );

      //-----------------------------------------------------------------------
      // PURPOSE: Perfoms both an A and AAAA lookup on the name passed in
      //          thus simplifying the lookup for a single namd where both
      //          A and AAAA results are possible.
      static IDNSQueryPtr lookupAorAAAA(
                                        IDNSDelegatePtr delegate,
                                        const char *name
                                        );

      //-----------------------------------------------------------------------
      // PURPOSE: Perform an SRV-record lookup with a recursive DNS and
      //          optionally resolves all names found to A or AAAA records and
      //          optionally performs an A or AAAA record lookups if the SRV
      //          fails to resolve.
      // NOTE:    The onLookupFailed event is only called if the SRV record
      //          fails to resolve and any optional fallback A or AAAA
      //          record lookups also fail to resolve. Individual failures
      //          to lookup A or AAAA records within the SRV records would not
      //          cause the onLookupFailed routine to be called.
      static IDNSQueryPtr lookupSRV(
                                    IDNSDelegatePtr delegate,
                                    const char *name,
                                    const char *service,                        // e.g. stun
                                    const char *protocol,                       // e.g. udp
                                    WORD defaultPort = 0,
                                    WORD defaultPriority = 10,
                                    WORD defaultWeight = 0,
                                    SRVLookupTypes lookupType = SRVLookupType_AutoLookupAndFallbackAll
                                    );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates an A result based on a list of IP addresses (only
      //          IPv4 addresses will get returned in the new result and if no
      //          IPv4 addresses exist the result will be NULL)
      static AResultPtr convertIPAddressesToAResult(
                                                    const std::list<IPAddress> &ipAddresses,
                                                    UINT ttl = 3600
                                                    );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates an AAAA result based on a list of IP addresses (only
      //          IPv6 addresses will get returned in the new result and if no
      //          IPv6 addresses exist the result will be NULL)
      static AAAAResultPtr convertIPAddressesToAAAAResult(
                                                          const std::list<IPAddress> &ipAddresses,
                                                          UINT ttl = 3600
                                                          );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates an SRV result type from an A or AAAA result types.
      static SRVResultPtr convertAorAAAAResultToSRVResult(
                                                          const char *service,
                                                          const char *protocol,
                                                          AResultPtr aResult,                      // either aResult or aaaaResult must be set
                                                          AAAAResultPtr aaaaResult,
                                                          WORD defaultPort = 0,                    // if specified, then all IP addresses without a port will automatically get assigned this port
                                                          WORD defaultPriority = 10,
                                                          WORD defaultWeight = 0
                                                          );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates an SRV result type from a list of IP addressses.
      static SRVResultPtr convertIPAddressesToSRVResult(
                                                        const char *service,
                                                        const char *protocol,
                                                        const std::list<IPAddress> &ipAddresses,   // must contain at least one IP address
                                                        WORD defaultPort = 0,                      // if specified, then all IP addresses without a port will automaticlaly get assigned this port
                                                        WORD defaultPriority = 10,
                                                        WORD defaultWeight = 0,
                                                        UINT ttl = 36000
                                                        );

      //-----------------------------------------------------------------------
      // PURPOSE: Creates an SRV result from a list of individual SRV results
      static SRVResultPtr mergeSRVs(const SRVResultList &srvList);

      //-----------------------------------------------------------------------
      // PURPOSE: Extracts the next server in the list to attempt to connect.
      // RETURNS: true if address is found otherwise false.
      // WARNING: This will modfy the SRV record. You should clone any SRV
      //          record where you want to maintain the original values.
      // NOTE:    The caller can safely pass in the address to the same smart
      //          pointer variable for outAResult and outAAAAResult (since the
      //          type is really the same underneath). This will ensure the
      //          caller points to the correct DNS A or AAAA result regardless
      //          if the actual return type was an A or an AAAA result.
      static bool extractNextIP(
                                SRVResultPtr srvResult,
                                IPAddress &outIP,
                                AResultPtr *outAResult = NULL,                // if you care where the original IP address result came from then pass in address of smart pointer to A record (will only fill if came from A result)
                                AAAAResultPtr *outAAAAResult = NULL           // if you care where the original IP address result came from then pass in address of smart pointer to AAAA record (will only fill if came from AAAA result)
                                );

      //-----------------------------------------------------------------------
      // PURPOSE: Clone routines for various return results.
      static AResultPtr cloneA(AResultPtr aResult);
      static AAAAResultPtr cloneAAAA(AAAAResultPtr aaaaResult);
      static SRVResultPtr cloneSRV(SRVResultPtr srvResult);
    };


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IDNSQuery
    #pragma mark

    interaction IDNSQuery
    {
      typedef IDNS::AResult AResult;
      typedef IDNS::AResultPtr AResultPtr;
      typedef IDNS::AAAAResult AAAAResult;
      typedef IDNS::AAAAResultPtr AAAAResultPtr;
      typedef IDNS::SRVResult SRVResult;
      typedef IDNS::SRVResultPtr SRVResultPtr;

      virtual PUID getID() const = 0;

      // PURPOSE: Cancels the DNS query request and prevents any further events
      //          from being fired about this query.
      // NOTE:    This cannot prevent events that have already fired but have
      //          not been processed in the receiving message queue yet.
      virtual void cancel() = 0;

      virtual bool hasResult() const = 0;
      virtual bool isComplete() const = 0;

      virtual AResultPtr getA() const = 0;
      virtual AAAAResultPtr getAAAA() const = 0;
      virtual SRVResultPtr getSRV() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IDNSDelegate
    #pragma mark

    interaction IDNSDelegate
    {
      typedef services::IDNSQueryPtr IDNSQueryPtr;

      virtual void onLookupCompleted(IDNSQueryPtr query) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::services::IDNSDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::services::IDNSQueryPtr, IDNSQueryPtr)
ZS_DECLARE_PROXY_METHOD_1(onLookupCompleted, IDNSQueryPtr)
ZS_DECLARE_PROXY_END()
