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

#include "TestServicePeerContactSession.h"
#include "config.h"
#include "boost_replacement.h"
#include <hookflash/stack/IStack.h>
#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/internal/stack_ServicePeerContactSession.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>
#include <zsLib/Proxy.h>
#include <zsLib/XML.h>

#include <iostream>
#include <fstream>

namespace hookflash { namespace stack { namespace test { ZS_DECLARE_SUBSYSTEM(hookflash_stack_test) } } }

namespace hookflash { namespace stack { namespace test { ZS_IMPLEMENT_SUBSYSTEM(hookflash_stack_test) } } }

//#define HOOKFLASH_MEDIA_ENGINE_DEBUG_LOG_LEVEL
//#define HOOKFLASH_MEDIA_ENGINE_ENABLE_TIMER
#define HOOKFLASH_STACK_TEST_SERVICE_PEER_CONTACT_TIMEOUT_IN_SECONDS (60*2)

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
      ElementPtr createFromString(const String &elementStr)
      {
        if (!elementStr) return ElementPtr();
        
        DocumentPtr doc = Document::createFromParsedJSON(elementStr);
        
        ElementPtr childEl = doc->getFirstChildElement();
        if (!childEl) return ElementPtr();
        
        childEl->orphan();
        return childEl;
      }
      
      //-----------------------------------------------------------------------
      String convertToString(const ElementPtr &element)
      {
        if (!element) return String();
        
        GeneratorPtr generator = Generator::createJSONGenerator();
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
#pragma mark
#pragma mark TestServicePeerContactSession
#pragma mark
      TestServicePeerContactSession::~TestServicePeerContactSession()
      {
        mThisWeak.reset();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestServiceIdentitySession
#pragma mark
      TestServiceIdentitySession::~TestServiceIdentitySession()
      {
        mThisWeak.reset();
      }
      //-----------------------------------------------------------------------
      
      bool TestServiceIdentitySession::isLoginComplete() const
      {
        return true;
      }
      
      void TestServiceIdentitySession::associate(ServicePeerContactSessionPtr peerContact)
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
#pragma mark
#pragma mark TestBootstrappedNetwork
#pragma mark
      
      TestBootstrappedNetwork::~TestBootstrappedNetwork()
      {
        mThisWeak.reset();
      }
      //-----------------------------------------------------------------------

      void TestBootstrappedNetwork::initialize(IBootstrappedNetworkDelegatePtr delegate)
      {
        if(delegate)
        {
          //delegate->onBootstrappedNetworkPreparationCompleted(mThisWeak.lock());
        }
        mCompleted = true;
      }
      
      bool TestBootstrappedNetwork::isPreparationComplete() const
      {
        return true;
      }
      
      bool TestBootstrappedNetwork::wasSuccessful(
                                                  WORD *outErrorCode,
                                                  String *outErrorReason
                                                  ) const
      {
        return true;
      }
      
      bool TestBootstrappedNetwork::isValidSignature(ElementPtr signedElement) const
      {
        return true;
      }
      
      bool TestBootstrappedNetwork::sendServiceMessage(
                                                       const char *serviceType,
                                                       const char *serviceMethodName,
                                                       message::MessagePtr message
                                                       )
      {
        DocumentPtr doc = message->encode();
        ULONG postDataLengthInBytes = 0;
        boost::shared_array<char> postData = doc->writeAsJSON(&postDataLengthInBytes);
        
        
        Duration timeout = Duration(Seconds(HOOKFLASH_STACK_TEST_SERVICE_PEER_CONTACT_TIMEOUT_IN_SECONDS));
        
        TestHTTPQueryPtr query = TestHTTPQuery::create(hookflash::services::internal::HTTPPtr(), mThisWeak.lock(), true, "Bojan's test app", "local", (const BYTE *)postData.get(), postDataLengthInBytes,"", timeout);
        
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
                                     \"url\": \"unstable.hookflash.me\" \
                                     }, \
                                     { \
                                     \"$id\": \"4e6a9ca60d92dfa5e872537f408066d02bdb55f2\", \
                                     \"type\": \"turn\", \
                                     \"version\": \"RFC5766\", \
                                     \"url\": \"unstable.hookflash.me\", \
                                     \"username\": \"a3c6a8742ad3dd73afeb6af35ad4747b38deb3dc\", \
                                     \"password\": \"eb19ac7d43e5f30961198507edaeb8297c579f35\" \
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
        else
        {}
          return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      
#pragma mark
#pragma mark TestFactory
#pragma mark
      
#ifdef USE_FAKE_BOOTSTRAPPED_NETWORK
      //bootstrapped network
      BootstrappedNetworkPtr TestFactory::prepare(
                                                 const char *domain,
                                                 IBootstrappedNetworkDelegatePtr delegate
                                                 )
      {
        TestBootstrappedNetworkPtr pThis(new TestBootstrappedNetwork(IStackForInternal::queueStack()));
        
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
      ServiceIdentitySessionPtr TestFactory::loginWithIdentity(
                                                          IServiceIdentitySessionDelegatePtr delegate,
                                                          const char *redirectAfterLoginCompleteURL,
                                                          const char *identityURI,
                                                          IServiceIdentityPtr provider// required if identity URI does not have domain
                                                          )
      {
        TestServiceIdentitySessionPtr pThis(new TestServiceIdentitySession());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceIdentitySessionPtr TestFactory::loginWithIdentityTBD(
                                                             IServiceIdentitySessionDelegatePtr delegate,
                                                             const char *redirectAfterLoginCompleteURL,
                                                             IServiceIdentityPtr provider,
                                                             const char *legacyIdentityBaseURI
                                                             )
      {
        TestServiceIdentitySessionPtr pThis(new TestServiceIdentitySession());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceIdentitySessionPtr TestFactory::loginWithIdentityBundle(
                                                                IServiceIdentitySessionDelegatePtr delegate,
                                                                const char *redirectAfterLoginCompleteURL,
                                                                ElementPtr signedIdentityBundle
                                                                )
      {
        TestServiceIdentitySessionPtr pThis(new TestServiceIdentitySession());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServiceIdentitySessionPtr TestFactory::relogin(
                                                IServiceIdentitySessionDelegatePtr delegate,
                                                const char *redirectAfterLoginCompleteURL,
                                                IServiceIdentityPtr provider,
                                                const char *identityReloginAccessKey
                                                )
      {
        TestServiceIdentitySessionPtr pThis(new TestServiceIdentitySession());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
#endif //USE_FAKE_IDENTITY_SESSION
      //-----------------------------------------------------------------------
#ifdef USE_FAKE_PEER_CONTACT_SESSION
      //service peer contact session
      ServicePeerContactSessionPtr TestFactory::login(IServicePeerContactSessionDelegatePtr delegate, IServicePeerContactPtr servicePeerContact, IServiceIdentitySessionPtr identitySession)
      {
        TestServicePeerContactSessionPtr pThis(new TestServicePeerContactSession());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
      
      ServicePeerContactSessionPtr TestFactory::relogin(
                                                       IServicePeerContactSessionDelegatePtr delegate,
                                                       IPeerFilesPtr existingPeerFiles
                                                       )
      {
        TestServicePeerContactSessionPtr pThis(new TestServicePeerContactSession());
        pThis->mThisWeak = pThis;
        //pThis->init();
        return pThis;
      }
#endif //USE_FAKE_PEER_CONTACT_SESSION
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------      
#pragma mark
#pragma mark TestCallback
#pragma mark
      TestCallback::TestCallback(zsLib::IMessageQueuePtr queue) :
      MessageQueueAssociator(queue),
      mNetworkDone(false),
      mCount(0)
      {
      }
      
      TestCallback::~TestCallback()
      {
        mThisWeak.reset();
      }
      
      TestCallbackPtr TestCallback::create(IMessageQueuePtr queue)
      {
        TestCallbackPtr pThis(new TestCallback(queue));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
      
      void TestCallback::init()
      {
        mLoginScenario = LoginScenario_None;
        mNetwork = IBootstrappedNetwork::prepare("unstable.hookflash.me", mThisWeak.lock());
      }
      
      void TestCallback::onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork)
      {
        AutoRecursiveLock lock(mLock);
        mNetworkDone = true;
        
        mIdentitySession = hookflash::stack::IServiceIdentitySession::loginWithIdentity(mThisWeak.lock(), "bogus", "bogus");
        
        zsLib::String filePassword, fileText;
        bool ret = readFromFile(filePassword, fileText);
        
        if (ret)
        {
          //relogin scenarion, file successfully loaded
          mLoginScenario = LoginScenario_Relogin;
          mPeerFilesElement = createFromString(fileText);
          mPeerFilePassword = filePassword;
          
          mPeerFilesPtr = IPeerFiles::loadFromElement(mPeerFilePassword.c_str(), mPeerFilesElement);
          
          mPeerContactSession = hookflash::stack::IServicePeerContactSession::relogin(mThisWeak.lock(), mPeerFilesPtr);
        }
        else
        {
          //peer file not found, do Login
          mLoginScenario = LoginScenario_Login;
          mPeerContactSession = hookflash::stack::IServicePeerContactSession::login(mThisWeak.lock(), hookflash::stack::IServicePeerContact::createServicePeerContactFrom(mNetwork), mIdentitySession);
        }
        
        

        

      }
      
      void TestCallback::onServiceIdentitySessionStateChanged(
                                                             IServiceIdentitySessionPtr session,
                                                             IServiceIdentitySession::SessionStates state
                                                             )
      {
      }
      
      void TestCallback::onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame(IServiceIdentitySessionPtr session)
      {
      }
      
      void TestCallback::onServicePeerContactSessionStateChanged(
                                                                IServicePeerContactSessionPtr session,
                                                                hookflash::stack::IServicePeerContactSession::SessionStates state
                                                                )
      {
        if (state == IServicePeerContactSession::SessionState_Ready)
        {
          ElementPtr element = mPeerContactSession->getPeerFiles()->saveToPrivatePeerElement();
          zsLib::String text = convertToString(element);
          
          SecureByteBlockPtr secPwd = mPeerContactSession->getPeerFiles()->getPeerFilePrivate()->getPassword();
          zsLib::String password = IHelper::convertToString((SecureByteBlock)*secPwd.get());
          text = password + "\n" + text;
          
          //first time login, save peer file
          if (LoginScenario_Login == mLoginScenario)
          {
            writeToFile(text);
          }
          
          ++mCount;
        }
      }
      void TestCallback::onServicePeerContactSessionAssociatedIdentitiesChanged(IServicePeerContactSessionPtr session)
      {
      }
#pragma mark
#pragma mark TestHTTPQuery
#pragma mark
      void TestHTTPQuery::writeBody(zsLib::String messageBody)
      {
        mBody.Put((BYTE *)messageBody.c_str(), messageBody.size());
      }
      
      TestHTTPQueryPtr TestHTTPQuery::create(
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
        TestHTTPQueryPtr pThis(new TestHTTPQuery(outer, delegate, isPost, userAgent, url, postData, postDataLengthInBytes, postDataMimeType, timeout));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
    }
  }
}

//service peer contact session
using hookflash::stack::test::TestServicePeerContactSession;
using hookflash::stack::test::TestServicePeerContactSessionPtr;

//service identity session
using hookflash::stack::test::TestServiceIdentitySession;
using hookflash::stack::test::TestServiceIdentitySessionPtr;

//bootstrapped network
using hookflash::stack::test::TestBootstrappedNetwork;
using hookflash::stack::test::TestBootstrappedNetworkPtr;

//delegate callback
using hookflash::stack::test::TestCallback;
using hookflash::stack::test::TestCallbackPtr;

//factory
using hookflash::stack::test::TestFactory;
using hookflash::stack::test::TestFactoryPtr;


void doTestPeerContactSession()
{
  if (!HOOKFLASH_STACK_TEST_DO_PEER_CONTACT_SESSION_TEST) return;
  
  BOOST_INSTALL_LOGGER();
  
  //initialize stack
  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadDelegate(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadStack(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadServices(zsLib::MessageQueueThread::createBasic());
  
  //override factory
  TestFactoryPtr overrideFactory(new TestFactory);
  hookflash::stack::internal::Factory::override(overrideFactory);
  
  //prepare stack
  hookflash::stack::IStack::setup(threadDelegate, threadStack, threadServices, "123456", "Bojan's Test app", "iOS 5.0.3", "iPad 2");
  
  //start test
  TestCallbackPtr testObject = TestCallback::create(thread);
  

  

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
  boost::this_thread::sleep(zsLib::Seconds(10));

  //Dispose created object
  testObject.reset();
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
