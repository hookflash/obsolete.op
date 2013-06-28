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

#if 0

#include "TestAccount.h"
#include "config.h"
#include "boost_replacement.h"
#include "helpers.h"
#include <openpeer/stack/IStack.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/internal/stack_ServiceLockboxSession.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>
#include <zsLib/Proxy.h>
#include <zsLib/XML.h>

#include <iostream>
#include <fstream>

namespace openpeer { namespace stack { namespace test { ZS_DECLARE_SUBSYSTEM(openpeer_stack_test) } } }

namespace openpeer { namespace stack { namespace test { ZS_IMPLEMENT_SUBSYSTEM(openpeer_stack_test) } } }

//#define OPENPEER_MEDIA_ENGINE_DEBUG_LOG_LEVEL
//#define OPENPEER_MEDIA_ENGINE_ENABLE_TIMER
#define OPENPEER_STACK_TEST_ACCOUNT_TIMEOUT_IN_SECONDS (60*2)

namespace openpeer
{
  namespace stack
  {
    namespace test
    {
//#pragma mark
//#pragma mark XML helpers
//#pragma mark
//      //-----------------------------------------------------------------------
//      ElementPtr createFromString(const String &elementStr)
//      {
//        if (!elementStr) return ElementPtr();
//        
//        DocumentPtr doc = Document::createFromParsedJSON(elementStr);
//        
//        ElementPtr childEl = doc->getFirstChildElement();
//        if (!childEl) return ElementPtr();
//        
//        childEl->orphan();
//        return childEl;
//      }
//      
//      //-----------------------------------------------------------------------
//      String convertToString(const ElementPtr &element)
//      {
//        if (!element) return String();
//        
//        GeneratorPtr generator = Generator::createJSONGenerator();
//        boost::shared_array<char> output = generator->write(element);
//        
//        return output.get();
//      }
//      
//      bool writeToFile(zsLib::String text)
//      {
//        std::ofstream myfile ("/tmp/peerfile.txt");
//        if (myfile.is_open())
//        {
//          myfile << text;
//          myfile.close();
//          return true;
//        }
//        else
//        {
//          std::cout << "Unable to open file";
//          return false;
//        }
//      }
//      bool readFromFile(String &outPassword, String &outText)
//      {
//        zsLib::String line;
//        std::ifstream myfile ("/tmp/peerfile.txt");
//        if (myfile.is_open())
//        {
//          int i = 0;
//          while ( myfile.good() )
//          {
//            getline (myfile,line);
//            if (i == 0)
//            {
//              outPassword = line;
//              std::cout << line << std::endl;
//            }
//            else{
//              outText += line;
//              std::cout << line << std::endl;
//            }
//            ++i;
//          }
//          myfile.close();
//          return true;
//        }
//        
//        else
//        {
//          std::cout << "Unable to open file";
//          return false;
//        }
//      }
#pragma mark
#pragma mark TestServiceLockboxSessionForAccount
#pragma mark
      TestServiceLockboxSessionForAccount::~TestServiceLockboxSessionForAccount()
      {
        mThisWeak.reset();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestAccount
#pragma mark
      TestAccount::~TestAccount()
      {
        mThisWeak.reset();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestAccountFinderForAccount
#pragma mark
      TestAccountFinderForAccount::~TestAccountFinderForAccount()
      {
        mThisWeak.reset();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestRUDPICESocketForAccount
#pragma mark
      TestRUDPICESocketForAccount::~TestRUDPICESocketForAccount()
      {
        mThisWeak.reset();
      }
      void TestRUDPICESocketForAccount::shutdown()
      {
        setState(RUDPICESocketState_Shutdown);
      }
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestServiceIdentitySessionForAccount
#pragma mark
      TestServiceIdentitySessionForAccount::~TestServiceIdentitySessionForAccount()
      {
        mThisWeak.reset();
      }
      //-----------------------------------------------------------------------
      
      bool TestServiceIdentitySessionForAccount::isLoginComplete() const
      {
        return true;
      }
      
      void TestServiceIdentitySessionForAccount::associate(ServiceLockboxSessionPtr peerContact)
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestBootstrappedNetworkForAccount
#pragma mark
      
      TestBootstrappedNetworkForAccount::~TestBootstrappedNetworkForAccount()
      {
        mThisWeak.reset();
      }
      //-----------------------------------------------------------------------
      
      void TestBootstrappedNetworkForAccount::initialize(IBootstrappedNetworkDelegatePtr delegate)
      {
        if(delegate)
        {
          //delegate->onBootstrappedNetworkPreparationCompleted(mThisWeak.lock());
        }
        mCompleted = true;
      }
      
      bool TestBootstrappedNetworkForAccount::isPreparationComplete() const
      {
        return true;
      }
      
      bool TestBootstrappedNetworkForAccount::wasSuccessful(
                                                  WORD *outErrorCode,
                                                  String *outErrorReason
                                                  ) const
      {
        return true;
      }
      
      bool TestBootstrappedNetworkForAccount::isValidSignature(ElementPtr signedElement) const
      {
        return true;
      }
      
      bool TestBootstrappedNetworkForAccount::sendServiceMessage(
                                                       const char *serviceType,
                                                       const char *serviceMethodName,
                                                       message::MessagePtr message
                                                       )
      {
        DocumentPtr doc = message->encode();
        ULONG postDataLengthInBytes = 0;
        boost::shared_array<char> postData = doc->writeAsJSON(&postDataLengthInBytes);
        
        
        Duration timeout = Duration(Seconds(OPENPEER_STACK_TEST_ACCOUNT_TIMEOUT_IN_SECONDS));
        
        TestHTTPQueryForAccountPtr query = TestHTTPQueryForAccount::create(openpeer::services::internal::HTTPPtr(), mThisWeak.lock(), true, "Bojan's test app", "local", (const BYTE *)postData.get(), postDataLengthInBytes,"", timeout);
        
        mPendingRequests[query] = message;
        
        if (!strcmp(serviceMethodName,"peer-contact-login"))
        {
          zsLib::String quote = "\"";
          zsLib::String ptr = "{ \
          \"result\": { \
          \"$domain\": \"unstable.hookflash.me\", \
          \"$id\":" + quote + message->messageID() + quote + ", \
          \"$handler\": \"peer-contact\", \
          \"$method\": \"peer-contact-login\", \
          \"$epoch\": 1362050609, \
          \"contactUserID\": \"1\", \
          \"contactAccessToken\": \"8ae0513dd08a0d2e3c0db8221c80ab2f\", \
          \"contactAccessSecret\": \"60ab2197b52939f29c2c1f25cac5ce173c16b840\", \
          \"contactAccessExpires\": 1362137009, \
          \"peerFilesRegenerate\": \"0\" \
          } \
          }";
          query->writeBody(ptr);
          //query->mBody.Put((BYTE *)ptr.c_str(), ptr.size());
          
          onHTTPCompleted(query);
        }
        else if (!strcmp(serviceMethodName, "signed-salt-get"))
        {
          zsLib::String quote = "\"";
          zsLib::String ptr = "{ \
          \"result\": { \
          \"$domain\": \"unstable.hookflash.me\", \
          \"$id\":" + quote + message->messageID() + quote + ", \
          \"$handler\": \"peer-salt\", \
          \"$method\": \"signed-salt-get\", \
          \"$epoch\": 1362131157, \
          \"salts\": { \
          \"saltBundle\": { \
          \"salt\": { \
          \"$id\": \"c5f568521a24c5baf618d1ada3b8e300fdc963e0\", \
          \"#text\": \"432d09beaed2b7bd392a73c8c3dddf86098981bc\" \
          }, \
          \"signature\": { \
          \"reference\": \"#c5f568521a24c5baf618d1ada3b8e300fdc963e0\", \
          \"algorithm\": \"http://openpeer.org/2012/12/14/jsonsig#rsa-sha1\", \
          \"digestValue\": \"0a0f158931d9e6abe2e3d99a7841242762a86feb\", \
          \"digestSigned\": \"by5doZ5jRK12qi4lIFUdeHpYR7ta3AUesWQX0Odr9nUL9MsdJTyccLeZMXFt2dcQKtfIyzOwFkpUbQuH7IFB4JLgPnGkJW/WfEggGxisTSDr+CYi3NU0hStWDvC+m6OLXjQAOc0PeI3ketUSXcEiNukkOvxuBlflbE0Zf7/zXejG+9L6Ve/0z3eKsJ487gyjhPhGxgLoGb1G6+3jWNvBubUqVhYac3hVMvI95zIkZg44T25gnuEwKfXpkfKRZRptbuk1Dq6StA52ZBKn1xQM8z3akPj9CPLcSTW3Rb+CiG3tgyRxBl7nYBaJeGDWNxtF0B9ttBY46AV69ugrdJlRbA==\", \
          \"key\": { \
          \"$id\": \"5d4e02f0800c0f354a72b2983914ca409ce6cf0c\", \
          \"domain\": \"unstable.hookflash.me\", \
          \"service\": \"salt\" \
          } \
          } \
          } \
          } \
          } \
          }";
          
          query->writeBody(ptr);
          
          onHTTPCompleted(query);
        }
        else if (!strcmp(serviceMethodName, "private-peer-file-set"))
        {
          zsLib::String quote = "\"";
          zsLib::String ptr = "{ \
          \"result\": { \
          \"$domain\": \"unstable.hookflash.me\", \
          \"$id\":" + quote + message->messageID() + quote + ", \
          \"$handler\": \"peer-contact\", \
          \"$method\": \"private-peer-file-set\", \
          \"$epoch\": 1362131170 \
          }  \
          }";
          
          query->writeBody(ptr);
          
          onHTTPCompleted(query);
          /*
           {
           "result": {
           "$domain": "provider.com",
           "$id": "abd23",
           "$handler": "peer-contact", "$method": "private-peer-file-set", "$epoch": 439439493
           } }
           */
        }
        else if (!strcmp(serviceMethodName, "peer-contact-services-get"))
        {
          zsLib::String quote = "\"";
          zsLib::String ptr = "{ \
          \"result\": { \
          \"$domain\": \"unstable.hookflash.me\", \
          \"$id\":" + quote + message->messageID() + quote + ", \
          \"$handler\": \"peer-contact\", \
          \"$method\": \"peer-contact-services-get\", \
          \"$epoch\": 1362392604, \
          \"services\": { \
          \"service\": [ \
          { \
          \"$id\": \"5e6a9ca60d92dfa5e872537f408066d02bdb55f85e6a9ca6\", \
          \"type\": \"stun\", \
          \"version\": \"RFC5389\", \
          \"methods\": { \
          \"method\": { \
            \"name\": \"stun\", \
            \"uri\": \"174.129.95.12\" \
            \"username\": \"toto\", \
            \"password\": \"toto\" \
          } \
          } \
          }, \
          { \
          \"$id\": \"4e6a9ca60d92dfa5e872537f408066d02bdb55f2\", \
          \"type\": \"turn\", \
          \"version\": \"RFC5766\", \
          \"methods\": { \
          \"method\": { \
          \"name\": \"turn\", \
          \"uri\": \"174.129.95.12\" \
          \"username\": \"toto\", \
          \"password\": \"toto\" \
          } \
          } \
          } \
          ] \
          } \
          } \
          }";
          query->writeBody(ptr);
          
          onHTTPCompleted(query);
          
          /*
           {
              "result": {
                  "$domain": "unstable.hookflash.me",
                  "$id": "380dc742ef",
                  "$handler": "peer-contact",
                  "$method": "peer-contact-services-get",
                  "$epoch": 1362392604,
                  "services": {
                      "service": [
                          {
                              "$id": "5e6a9ca60d92dfa5e872537f408066d02bdb55f8\r\n5e6a9ca6",
                              "type": "stun",
                              "version": "RFC5389",
                              "url": "unstable.hookflash.me"
                          },
                          {
                              "$id": "4e6a9ca60d92dfa5e872537f408066d02bdb55f2\r\n",
                              "type": "turn",
                              "version": "RFC5766\r\n",
                              "url": "unstable.hookflash.me",
                              "username": "a3c6a8742ad3dd73afeb6af35ad4747b38deb3dc",
                              "password": "eb19ac7d43e5f30961198507edaeb8297c579f35"
                          }
                      ]
                  }
              }
           }
           */
        }
        else if (!strcmp(serviceMethodName, "peer-contact-identity-associate"))
        {
          zsLib::String quote = "\"";
          zsLib::String ptr = "{ \
          \"result\": { \
          \"$domain\": \"unstable.hookflash.me\", \
          \"$id\":" + quote + message->messageID() + quote + ", \
          \"$handler\": \"peer-contact\", \
          \"$method\": \"peer-contact-identity-associate\", \
          \"$epoch\": 1362395414, \
          \"identities\": { \
          \"identity\": { \
          \"uri\": \"c5d0994f9b334503f98815765cbb8130331b61c2\", \
          \"provider\": \"unstable.hookflash.me\", \
          \"identityReloginAccessKeyEncrypted\": \"90f2ceec6d854c23db937c271c9819ef0dde5cc3\" \
          } \
          } \
          } \
          }";
          query->writeBody(ptr);
          
          onHTTPCompleted(query);
          
          /*
           {
              "result": {
                  "$domain": "unstable.hookflash.me",
                  "$id": "dc4333193f",
                  "$handler": "peer-contact",
                  "$method": "peer-contact-identity-associate",
                  "$epoch": 1362395414,
                  "identities": {
                      "identity": {
                          "uri": "c5d0994f9b334503f98815765cbb8130331b61c2",
                          "provider": "unstable.hookflash.me",
                          "identityReloginAccessKeyEncrypted": "90f2ceec6d854c23db937c271c9819ef0dde5cc3"
                      }
                  }
              }
           }
           */
        }
        else if(!strcmp(serviceMethodName, "finders-get"))
        {
          zsLib::String quote = "\"";
          zsLib::String ptr = "{ \
            \"result\": { \
              \"$domain\": \"unstable.hookflash.me\", \
              \"$id\":" + quote + message->messageID() + quote + ", \
              \"$handler\": \"bootstrapped-finder\", \
              \"$method\": \"finders-get\", \
              \"$epoch\": 1362659300, \
              \"finders\": { \
                \"finderBundle\": { \
                  \"finder\": { \
                    \"$id\": \"9e64fdea1b972c442a70e02c7764e0a513bb4caf\", \
                    \"transport\": \"rudp/udp\", \
                    \"srv\": \"unstable.hookflash.me\", \
                    \"key\": { \
                      \"x509Data\": \"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2mNnwojn9L0CIpSuDt/TGebRaQk08kxlFRaC5wwlhisOtf98P1X2+ZBBJtRETOR5CwyB9UMa1SFr4mi0iMsjqYcLAL80DpdvNWkKZi61dgU8260tr7PBw1byw2iRwOXLPEd9irWrsDO+F/fbKsM21DkKnRs6Cjk+GcgCnNYTtoAt9mO1UPhwRfU+UtDL3X7I7xf0PlytKaDvuqid38jWccgWdBAKHpkzUmr2goF6T2802OAmAUToP8UMPnpQ5L7GZMT4D3C/AkjzJveqAXJA9OVGdFcY5ztHLxChQyfkyuajQbxrrUpMzpFMEXsAieZxYucazwT3nsXsx05jRQJQmQIDAQAB\" \
                      }, \
                    \"priority\": 0, \
                    \"weight\": 30, \
                    \"region\": \"1\", \
                    \"created\": 1362659297, \
                    \"expires\": 1394195297 \
                    }, \
                  \"signature\": { \
                    \"reference\": \"#9e64fdea1b972c442a70e02c7764e0a513bb4caf\", \
                    \"algorithm\": \"http://openpeer.org/2012/12/14/jsonsig#rsa-sha1\", \
                    \"digestValue\": \"a3909a82809443ebf599c233575e3ef65909f019\", \
                    \"digestSigned\": \"H/Q0daS/OmxC1tidKx7ORmy/q4ErHlHsMKXS/hD+aP94GJYw6pXP6V+T6hvR+UDH4dnbZ/slZSzXtMN0r/Of27ih6mNwnVWapp04rCwH9xJPrDWgTy6ZXQ8eTDiNXbvHUtmeKoPBl40p4eEys5G9pg4fgEMGY9eGt4Vh2XGiyHm0AM1tSwa+3SYzHw9HouRTYN1ipKfpkdaGwbYHXkuRX6OcJRJhoe2diOK0LgQrTLL2l0wL6WiJDDkot5/M5RV2xn/VJ6icbK/2TCLBZWRFFGKe+aolcEc3RWGP5hS23zkg9rX0ngqVMgtrZr9qr4iAVicAeIgJSuoKkRm2Kc2VzA==\", \
                    \"key\": { \
                      \"$id\": \"8d6062d23e84e9cd5f18134ba9531a48bd45fbab\", \
                      \"domain\": \"unstable.hookflash.me\", \
                      \"service\": \"bootstrapped-finders\" \
                      } \
                    } \
                  } \
                } \
              } \
        }";
          query->writeBody(ptr);
          
          onHTTPCompleted(query);
          /*
           {
              "result": {
                  "$domain": "unstable.hookflash.me",
                  "$id": "18b419889c",
                  "$handler": "bootstrapper-finder",
                  "$method": "finders-get",
                  "$epoch": 1362659300,
                  "finders": {
                      "finderBundle": {
                          "finder": {
                              "$id": "9e64fdea1b972c442a70e02c7764e0a513bb4caf",
                              "transport": "rudp/udp",
                              "srv": "finders test",
                              "key": {
                                  "x509Data": "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2mNnwojn9L0CIpSuDt/TGebRaQk08kxlFRaC5wwlhisOtf98P1X2+ZBBJtRETOR5CwyB9UMa1SFr4mi0iMsjqYcLAL80DpdvNWkKZi61dgU8260tr7PBw1byw2iRwOXLPEd9irWrsDO+F/fbKsM21DkKnRs6Cjk+GcgCnNYTtoAt9mO1UPhwRfU+UtDL3X7I7xf0PlytKaDvuqid38jWccgWdBAKHpkzUmr2goF6T2802OAmAUToP8UMPnpQ5L7GZMT4D3C/AkjzJveqAXJA9OVGdFcY5ztHLxChQyfkyuajQbxrrUpMzpFMEXsAieZxYucazwT3nsXsx05jRQJQmQIDAQAB"
                              },
                              "priority": 0,
                              "weight": 30,
                              "region": "1",
                              "created": 1362659297,
                              "expires": 1394195297
                          },
                          "signature": {
                              "reference": "#9e64fdea1b972c442a70e02c7764e0a513bb4caf",
                              "algorithm": "http://openpeer.org/2012/12/14/jsonsig#rsa-sha1",
                              "digestValue": "a3909a82809443ebf599c233575e3ef65909f019",
                              "digestSigned": "H/Q0daS/OmxC1tidKx7ORmy/q4ErHlHsMKXS/hD+aP94GJYw6pXP6V+T6hvR+UDH4dnbZ/slZSzXtMN0r/Of27ih6mNwnVWapp04rCwH9xJPrDWgTy6ZXQ8eTDiNXbvHUtmeKoPBl40p4eEys5G9pg4fgEMGY9eGt4Vh2XGiyHm0AM1tSwa+3SYzHw9HouRTYN1ipKfpkdaGwbYHXkuRX6OcJRJhoe2diOK0LgQrTLL2l0wL6WiJDDkot5/M5RV2xn/VJ6icbK/2TCLBZWRFFGKe+aolcEc3RWGP5hS23zkg9rX0ngqVMgtrZr9qr4iAVicAeIgJSuoKkRm2Kc2VzA==",
                              "key": {
                                  "$id": "8d6062d23e84e9cd5f18134ba9531a48bd45fbab",
                                  "domain": "unstable.hookflash.me",
                                  "service": "bootstrapped-finders"
                              }
                          }
                      }
                  }
              }
           }
           */
        }
        else
        {}
        return true;
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      
#pragma mark
#pragma mark TestFactoryForAccount
#pragma mark
      
#ifdef USE_FAKE_BOOTSTRAPPED_NETWORK
      //bootstrapped network
      BootstrappedNetworkPtr TestFactoryForAccount::prepare(
                                                  const char *domain,
                                                  IBootstrappedNetworkDelegatePtr delegate
                                                  )
      {
        TestBootstrappedNetworkForAccountPtr pThis(new TestBootstrappedNetworkForAccount(IStackForInternal::queueStack()));
        
        BootstrappedNetworkManagerPtr manager = IBootstrappedNetworkManagerForBootstrappedNetwork::singleton();
        ZS_THROW_BAD_STATE_IF(!manager)
        
        pThis->mThisWeak = pThis;
        
        AutoRecursiveLock lock(pThis->getLock());
        
        BootstrappedNetworkPtr useThis = manager->forBootstrappedNetwork().findExistingOrUse(pThis);
        
        if (delegate) {
          manager->forBootstrappedNetwork().registerDelegate(useThis, delegate);
        }
        
        pThis->mThisWeak = pThis;
        pThis->mCompleted = true;
        return useThis;
      }
      
#endif //USE_FAKE_BOOTSTRAPPED_NETWORK
      //-----------------------------------------------------------------------
#ifdef USE_FAKE_IDENTITY_SESSION
      //service identity session
      ServiceIdentitySessionPtr TestFactoryForAccount::loginWithIdentity(
                                                               IServiceIdentitySessionDelegatePtr delegate,
                                                               const char *redirectAfterLoginCompleteURL,
                                                               const char *identityURI,
                                                               IServiceIdentityPtr provider// required if identity URI does not have domain
                                                               )
      {
        TestServiceIdentitySessionForAccountPtr pThis(new TestServiceIdentitySessionForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceIdentitySessionPtr TestFactoryForAccount::loginWithIdentityTBD(
                                                                  IServiceIdentitySessionDelegatePtr delegate,
                                                                  const char *redirectAfterLoginCompleteURL,
                                                                  IServiceIdentityPtr provider,
                                                                  const char *legacyIdentityBaseURI
                                                                  )
      {
        TestServiceIdentitySessionForAccountPtr pThis(new TestServiceIdentitySessionForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceIdentitySessionPtr TestFactoryForAccount::loginWithIdentityBundle(
                                                                     IServiceIdentitySessionDelegatePtr delegate,
                                                                     const char *redirectAfterLoginCompleteURL,
                                                                     ElementPtr signedIdentityBundle
                                                                     )
      {
        TestServiceIdentitySessionForAccountPtr pThis(new TestServiceIdentitySessionForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceIdentitySessionPtr TestFactoryForAccount::relogin(
                                                     IServiceIdentitySessionDelegatePtr delegate,
                                                     const char *redirectAfterLoginCompleteURL,
                                                     IServiceIdentityPtr provider,
                                                     const char *identityReloginAccessKey
                                                     )
      {
        TestServiceIdentitySessionForAccountPtr pThis(new TestServiceIdentitySessionForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
#endif //USE_FAKE_IDENTITY_SESSION
      //-----------------------------------------------------------------------
#ifdef USE_FAKE_PEER_CONTACT_SESSION
      //service peer contact session
      ServiceLockboxSessionPtr TestFactoryForAccount::login(IServiceLockboxSessionDelegatePtr delegate, IServiceLockboxPtr ServiceLockbox, IServiceIdentitySessionPtr identitySession)
      {
        TestServiceLockboxSessionForAccountPtr pThis(new TestServiceLockboxSessionForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceLockboxSessionPtr TestFactoryForAccount::relogin(
                                                        IServiceLockboxSessionDelegatePtr delegate,
                                                        IPeerFilesPtr existingPeerFiles
                                                        )
      {
        TestServiceLockboxSessionForAccountPtr pThis(new TestServiceLockboxSessionForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
#endif //USE_FAKE_PEER_CONTACT_SESSION
#ifdef USE_FAKE_ACOUNT
      AccountPtr TestFactoryForAccount::create(
                                IAccountDelegatePtr delegate,
                                IServiceLockboxSessionPtr peerContactSession
                                )
      {
        TestAccountPtr pThis(new TestAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
#endif //USE_FAKE_ACOUNT
      
      AccountFinderPtr TestFactoryForAccount::create(
                              IAccountFinderDelegatePtr delegate,
                              AccountPtr outer
                              )
      {
        TestAccountFinderForAccountPtr pThis(new TestAccountFinderForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init();
        pThis->setState(IAccount::AccountState_Ready);
        return pThis;
      }
      
      //RUDPICE Socket
      RUDPICESocketPtr TestServicesFactoryForAccount::create(
                              IMessageQueuePtr queue,
                              IRUDPICESocketDelegatePtr delegate,
                              const char *turnServer,
                              const char *turnServerUsername,
                              const char *turnServerPassword,
                              const char *stunServer,
                              WORD port
                              )
      {
        TestRUDPICESocketForAccountPtr pThis(new TestRUDPICESocketForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init(turnServer, turnServerUsername, turnServerPassword, stunServer, port);
        pThis->setState(openpeer::services::IRUDPICESocket::RUDPICESocketState_Ready);
        return pThis;
      }
      
      RUDPICESocketPtr TestServicesFactoryForAccount::create(
                              IMessageQueuePtr queue,
                              IRUDPICESocketDelegatePtr delegate,
                              IDNS::SRVResultPtr srvTURNUDP,
                              IDNS::SRVResultPtr srvTURNTCP,
                              const char *turnServerUsername,
                              const char *turnServerPassword,
                              IDNS::SRVResultPtr srvSTUN,
                              WORD port
                              )
      {
        TestRUDPICESocketForAccountPtr pThis(new TestRUDPICESocketForAccount());
        pThis->mThisWeak = pThis;
        //pThis->init(turnServer, turnServerUsername, turnServerPassword, stunServer, port);
        pThis->setState(openpeer::services::IRUDPICESocket::RUDPICESocketState_Ready);
        return pThis;
      }
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestCallbackForAccount
#pragma mark
      TestCallbackForAccount::TestCallbackForAccount(zsLib::IMessageQueuePtr queue) :
      MessageQueueAssociator(queue),
      mNetworkDone(false),
      mCount(0)
      {
      }
      
      TestCallbackForAccount::~TestCallbackForAccount()
      {
        if(mNetwork)
          mNetwork.reset();

        if(mPeerContactSession)
          mPeerContactSession.reset();

        if(mIdentitySession)
          mIdentitySession.reset();

        if(mAccount) {
          mAccount->shutdown();
          mAccount.reset();
        }
        
        mThisWeak.reset();
      }
      
      TestCallbackForAccountPtr TestCallbackForAccount::create(IMessageQueuePtr queue)
      {
        TestCallbackForAccountPtr pThis(new TestCallbackForAccount(queue));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
      
      void TestCallbackForAccount::init()
      {
        mLoginScenario = LoginScenario_None;
        mNetwork = IBootstrappedNetwork::prepare("unstable.hookflash.me", mThisWeak.lock());
      }
      
      void TestCallbackForAccount::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        AutoRecursiveLock lock(mLock);
        mNetworkDone = true;
        
        mIdentitySession = openpeer::stack::IServiceIdentitySession::loginWithIdentity(mThisWeak.lock(), "bogus", "bogus");
        
        zsLib::String filePassword, fileText;
        bool ret = readFromFile(filePassword, fileText);
        
        if (ret)
        {
          //relogin scenarion, file successfully loaded
          mLoginScenario = LoginScenario_Relogin;
          mPeerFilesElement = createFromString(fileText);
          mPeerFilePassword = filePassword;
          
          mPeerFilesPtr = IPeerFiles::loadFromElement(mPeerFilePassword.c_str(), mPeerFilesElement);
          
          mPeerContactSession = openpeer::stack::IServiceLockboxSession::relogin(mThisWeak.lock(), mPeerFilesPtr);
        }
        else
        {
          //peer file not found, do Login
          mLoginScenario = LoginScenario_Login;
          mPeerContactSession = openpeer::stack::IServiceLockboxSession::login(mThisWeak.lock(), openpeer::stack::IServiceLockbox::createServiceLockboxFrom(mNetwork), mIdentitySession);
        }
        
        
        
        
        
      }
      
      void TestCallbackForAccount::onServiceIdentitySessionStateChanged(
                                                              IServiceIdentitySessionPtr session,
                                                              IServiceIdentitySession::SessionStates state
                                                              )
      {
      }
      
      void TestCallbackForAccount::onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session)
      {
      }
      
      void TestCallbackForAccount::onServiceLockboxSessionStateChanged(
                                                                 IServiceLockboxSessionPtr session,
                                                                 openpeer::stack::IServiceLockboxSession::SessionStates state
                                                                 )
      {
        if (state == IServiceLockboxSession::SessionState_Ready)
        {
          ElementPtr element = mPeerContactSession->getPeerFiles()->saveToPrivatePeerElement();
          zsLib::String text = convertToString(element);
          
          SecureByteBlockPtr secPwd = mPeerContactSession->getPeerFiles()->getPeerFilePrivate()->getPassword();
          zsLib::String password = IHelper::convertToString((SecureByteBlock)*secPwd.get());
          text = password + "\n" + text;
          
          //first time login, save peer file
          if (LoginScenario_Login == mLoginScenario)
          {
            writeToFile(text, "/tmp/peerfile.txt");
          }
          //prerequisites for account creation DONE
          mAccount = IAccount::create(mThisWeak.lock(), mPeerContactSession);
          //++mCount;
        }
      }
      void TestCallbackForAccount::onServiceLockboxSessionAssociatedIdentitiesChanged(IServiceLockboxSessionPtr session)
      {
      }
      
      void TestCallbackForAccount::onAccountStateChanged(
                                 IAccountPtr account,
                                 openpeer::stack::internal::Account::AccountStates state
                                 )
      {
        if (state == Account::AccountState_Ready)
        {
          ++mCount;
        }
      }
#pragma mark
#pragma mark TestHTTPQueryForAccount
#pragma mark
      void TestHTTPQueryForAccount::writeBody(zsLib::String messageBody)
      {
        mBody.Put((BYTE *)messageBody.c_str(), messageBody.size());
      }
      
      TestHTTPQueryForAccountPtr TestHTTPQueryForAccount::create(
                                             services::internal::HTTPPtr outer,
                                             services::IHTTPQueryDelegatePtr delegate,
                                             bool isPost,
                                             const char *userAgent,
                                             const char *url,
                                             const BYTE *postData,
                                             ULONG postDataLengthInBytes,
                                             const char *postDataMimeType,
                                             Duration timeout
                                             )
      {
        TestHTTPQueryForAccountPtr pThis(new TestHTTPQueryForAccount(outer, delegate, isPost, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
    }
  }
}

//service peer contact session
using openpeer::stack::test::TestServiceLockboxSessionForAccount;
using openpeer::stack::test::TestServiceLockboxSessionForAccountPtr;

//service identity session
using openpeer::stack::test::TestServiceIdentitySessionForAccount;
using openpeer::stack::test::TestServiceIdentitySessionForAccountPtr;

//bootstrapped network
using openpeer::stack::test::TestBootstrappedNetworkForAccount;
using openpeer::stack::test::TestBootstrappedNetworkForAccountPtr;

//delegate callback
using openpeer::stack::test::TestCallbackForAccount;
using openpeer::stack::test::TestCallbackForAccountPtr;

//factory
using openpeer::stack::test::TestFactoryForAccount;
using openpeer::stack::test::TestFactoryForAccountPtr;

//services factory
using openpeer::stack::test::TestServicesFactoryForAccount;
using openpeer::stack::test::TestServicesFactoryForAccountPtr;


void doTestAccount()
{
  if (!OPENPEER_STACK_TEST_DO_ACCOUNT_TEST) return;
  
  BOOST_INSTALL_LOGGER();
  
  //initialize stack
  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadDelegate(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadStack(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadServices(zsLib::MessageQueueThread::createBasic());
  
  //override factory
  TestFactoryForAccountPtr overrideFactory(new TestFactoryForAccount);
  openpeer::stack::internal::Factory::override(overrideFactory);
  
  //override services factory
  TestServicesFactoryForAccountPtr overrideServicesFactory(new TestServicesFactoryForAccount);
  openpeer::services::internal::Factory::override(overrideServicesFactory);
  
  //prepare stack
  openpeer::stack::IStack::setup(threadDelegate, threadStack, threadServices, "123456", "Bojan's Test app", "iOS 5.0.3", "iPad 2");
  
  //start test
  TestCallbackForAccountPtr testObject = TestCallbackForAccount::create(thread);
  
  
  
  
  std::cout << "WAITING:      Waiting for stack test to complete (max wait is 60 seconds).\n";
  
  zsLib::ULONG expectingProcessed = 0;
  
  // count from each object
  expectingProcessed += (testObject ? 1 : 0);
  
  // check to see if all test routines have completed
  {
    zsLib::ULONG lastProcessed = 0;
    zsLib::ULONG totalWait = 0;
    do
    {
      zsLib::ULONG totalProcessed = 0;
      if (totalProcessed != lastProcessed) {
        lastProcessed = totalProcessed;
        std::cout << "WAITING:      [" << totalProcessed << "\n";
      }
      
      // tally up count from each object
      totalProcessed += testObject->mCount;
      
      if (totalProcessed < expectingProcessed) {
        ++totalWait;
        boost::this_thread::sleep(zsLib::Seconds(1));
      }
      else
        break;
    } while (totalWait < (60)); // max three minutes
    BOOST_CHECK(totalWait < (60));
  }
  
  std::cout << "\n\nWAITING:      All tests have finished. Waiting for 'bogus' events to process (10 second wait).\n";
  //boost::this_thread::sleep(zsLib::Seconds(10));
  
  //Dispose created object
  testObject.reset();
  
  boost::this_thread::sleep(zsLib::Seconds(10));
  // wait for shutdown
  {
    zsLib::ULONG count = 0;
    do
    {
      count = 0;
      
      count += thread->getTotalUnprocessedMessages();
      count += threadDelegate->getTotalUnprocessedMessages();
      count += threadStack->getTotalUnprocessedMessages();
      count += threadServices->getTotalUnprocessedMessages();
      if (0 != count)
        boost::this_thread::yield();
    } while (count > 0);
    
    thread->waitForShutdown();
  }
  BOOST_UNINSTALL_LOGGER()
  zsLib::proxyDump();
  BOOST_EQUAL(zsLib::proxyGetTotalConstructed(), 0);
}

#endif //0
