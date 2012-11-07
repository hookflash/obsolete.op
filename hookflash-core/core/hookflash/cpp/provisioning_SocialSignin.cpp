/*
 * Beach Path Inc. - hookflash Client API
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Contributor(s):
 *
 * Goran Tasic   <goran@hookflash.com>
 *
 */

#include <hookflash/provisioning/internal/provisioning_SocialSignin.h>
#include <hookflash/provisioning/hookflash.h>
#include <hookflash/internal/Client.h>

#include <hookflash/stack/message/internal/MessageHelper.h>
#include <hookflash/services/http/internal/hookflashTypes.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Numeric.h>

using namespace hookflash::stack::message::internal;
using namespace zsLib::XML;
using zsLib::String;
using zsLib::ULONG;
//using zsLib::CSTR;
//using zsLib::BYTE;
using zsLib::PUID;
using zsLib::AutoRecursiveLock;
using zsLib::Proxy;
using zsLib::Stringize;

using namespace hookflash::internal;
using namespace hookflash::services::http;
using namespace hookflash::services::http::internal;

//social signin login URLs
//#define URL_TWITTER "http://hfapi.hookflash.me/hybridauth/social_reg_log.php?ID=TW"
//#define URL_LINKEDIN "http://hfapi2.hookflash.me/hybridauth/social_reg_log.php?ID=LI"
//#define URL_FACEBOOK "http://hfapi.hookflash.me/hybridauth/social_reg_log.php?ID=FB"

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }

namespace hookflash
{
  namespace provisioning
  {
    namespace internal
    {
      // Providers
      struct Providers;
      typedef boost::shared_ptr<Providers> ProvidersPtr;
      struct Providers
      {
        static ProvidersPtr create();
        zsLib::String mProviderID;
        zsLib::String mProviderURL;
      };
      ProvidersPtr Providers::create()
      {
        ProvidersPtr pThis(new Providers);
        pThis->mProviderID  = "";
        pThis->mProviderURL = "";
        return pThis;
      }
      typedef std::map<zsLib::String, ProvidersPtr> ProvidersMap;
      // Providers end


      SocialSignin::SocialSignin(zsLib::IMessageQueuePtr queue) :
        zsLib::MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mProviderID(""),
        mProviderURL("")
      {
      }

      void SocialSignin::init()
      {
        AutoRecursiveLock lock(mLock);
        mProviderID = providerToString(mRequestedProvider);
        mHttpMethod = HttpMethod_None;
        step();
      }

      void SocialSignin::step()
      {
        DocumentPtr socialSignin_doc = Document::create();

        zsLib::String id = services::IHelper::randomString(10);
        ElementPtr rootSetRequest = MessageHelper::createElementWithText("request", "Get user's login URL");
        socialSignin_doc->adoptAsLastChild(rootSetRequest);
        rootSetRequest->setAttribute("id", id);

        ULONG length = 0;
        boost::shared_array<char> output = socialSignin_doc->write(&length);
        zsLib::String textOut = output.get();

        zsLib::String loginStr;
        loginStr.append(PROVISIONING_SERVER);
        loginStr.append(METHOD_USER_GET_LOGIN_URL);
        mHttpMethod = HttpMethod_UserGetLoginURL;
        mRequestTypes[id] = mHttpMethod;
        IHttp::post(mThis, loginStr, textOut, (zsLib::ULONG) strlen(textOut.c_str()), "", 30);
      }

      zsLib::String SocialSignin::providerToString(SocialSigninProviders provider)
      {
        switch (provider){
          case SocialSigninProvider_Facebook:
            return "FB"; break;
          case SocialSigninProvider_Twitter:
            return "TW"; break;
          case SocialSigninProvider_LinkedIn:
            return "LI"; break;
          case SocialSigninProvider_Hookflash:
            return "HF"; break;
          default:
            return "";
        }
      }

      ISocialSigninPtr SocialSignin::create(hookflash::provisioning::ISocialSigninDelegatePtr socialSigninDelegate, SocialSigninProviders provider)
      {
        SocialSigninPtr pThis(new SocialSignin(Client::hookflashQueue()));
        pThis->mThis = pThis;
        pThis->mSocialSigninDelegate = Proxy<hookflash::provisioning::ISocialSigninDelegate>::createWeak(Client::guiQueue(), socialSigninDelegate);
        pThis->mRequestedProvider = provider;
        pThis->init();
        return pThis;
      }

      //IHttpQueryDelegate
      void SocialSignin::onHttpComplete(services::http::IHttpQueryPtr query)
      {
      }

      void SocialSignin::onHttpReadDataAvailable(services::http::IHttpQueryPtr query)
      {
        zsLib::String response;
        query->readResultData(response);

        //handle curl network connection down issue
        if(response.size() <1)
        {
          mSocialSigninURLError.errorCode = 2;
          mSocialSigninURLError.errorMsg  = "Network connection is down.";
          return; // ???
        }

        DocumentPtr tmpDoc = Document::create();
        tmpDoc->parse(response);

        zsLib::String idValue;
        ElementPtr rootID;
        try
        {
          rootID = tmpDoc->getFirstChildElementChecked();
          idValue = MessageHelper::getAttributeID(rootID);
          mHttpMethod = mRequestTypes[idValue];
          mRequestTypes.erase(idValue);
        }
        catch(Exceptions::CheckFailed)
        {
        }

        if (mHttpMethod == HttpMethod_UserGetLoginURL)
        {
          zsLib::String idValue, msgValue/*, errorMsg*/;
          // int errorCode;

          mHttpMethod = HttpMethod_None;
          try
          {
            /*
            <response id="1351092499">
            	<idp>
              		<providerID>TW</providerID>
               		<url>http://hfapi2.hookflash.me/hybridauth/social_reg_log.php?ID=TW</url>
              </idp>
              <idp>
                 	<providerID>LI</providerID>
                  <url>http://hfapi2.hookflash.me/hybridauth/social_reg_log.php?ID=LI</url>
              </idp>
              <idp>
              		<providerID>FB</providerID>
                  <url>http://hfapi2.hookflash.me/hybridauth/social_reg_log.php?ID=FB</url>
              </idp>
            </response>
            */

            ProvidersMap providerList;
            ElementPtr root   = tmpDoc->getFirstChildElementChecked();
            ElementPtr idpTag = root->findFirstChildElementChecked("idp");
            while (idpTag)
            {
              ProvidersPtr provider  = Providers::create();
              provider->mProviderID  = MessageHelper::getElementText(idpTag->findFirstChildElementChecked("providerID"));
              provider->mProviderURL = MessageHelper::getElementText(idpTag->findFirstChildElementChecked("url"));
              providerList[provider->mProviderID] = provider;
              idpTag = idpTag->getNextSiblingElement();
            }
            ProvidersMap::iterator found = providerList.find(mProviderID);
            if (found != providerList.end())
              mProviderURL = providerList[mProviderID]->mProviderURL;
            else
              mProviderURL = "";

            if (!mProviderURL.isEmpty())
            {
              ZS_LOG_DEBUG(log("provider URL found") + ", mProviderID=" + mProviderID + ", mProviderURL=" + mProviderURL)
              mSocialSigninURLError.errorCode = 0;
              mSocialSigninURLError.errorMsg  = "Provider URL found.";
            }
            else {
              ZS_LOG_DEBUG(log("provider URL NOT found") + ", mProviderID=" + mProviderID + ", mProviderURL=" + mProviderURL)
              mSocialSigninURLError.errorCode = 1;
              mSocialSigninURLError.errorMsg  = "Provider URL NOT found.";
            }

            try {
              if (mSocialSigninDelegate) {
                mSocialSigninDelegate->onSocialSigninURLReceived(mRequestedProvider, mProviderURL, mSocialSigninURLError);
              }
            }
            catch (Proxy<hookflash::internal::IAccountDelegate>::Exceptions::DelegateGone &) {}
          }
          catch(Exceptions::CheckFailed)
          {
            try
            {
              mSocialSigninURLError.errorCode = zsLib::Numeric<int>(MessageHelper::getElementText(rootID->findFirstChildElementChecked("errorcode")));
              mSocialSigninURLError.errorMsg  = MessageHelper::getElementText(rootID->findFirstChildElementChecked("error"));
              if (mSocialSigninDelegate) {
                mSocialSigninDelegate->onSocialSigninURLReceived(mRequestedProvider, mProviderURL, mSocialSigninURLError);
              }
            }
            catch(Exceptions::CheckFailed) {}
          }
        }
        /*
        else if (mHttpMethod == HttpMethod_None) {}
        */
      }

      zsLib::String SocialSignin::log(const char *message) const
      {
        return String("hookflash::SocialSignin [") + Stringize<PUID>(mID).string() + "] " + message;
      }
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    } // namespace internal

    // ISocialSignin
    ISocialSigninPtr ISocialSignin::create(ISocialSigninDelegatePtr socialSigninDelegate, SocialSigninProviders provider)
    {
      return internal::SocialSignin::create(socialSigninDelegate, provider);
    }
  }
}
