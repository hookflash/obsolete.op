/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2013. Robin Raymond. All rights reserved.
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

#include <zsLib/Proxy.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#ifdef DEBUG
#include <iostream>
#endif //DEBUG

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  namespace internal
  {
#ifdef DEBUG
    class ProxyTracking
    {
    public:
      ProxyTracking()
      {
      }

      ~ProxyTracking()
      {
      }

      static ProxyTracking &singleton()
      {
        static ProxyTracking tracking;
        return tracking;
      }

      void follow(int line, const char *fileName)
      {
        AutoRecursiveLock lock(mLock);

        LocationPair pair(line, fileName);
        LocationCountMap::iterator found = mLocations.find(pair);
        if (found == mLocations.end()) {
          mLocations[pair] = 1;
        } else {
          int &count = (*found).second;
          ++count;
        }
      }

      void unfollow(int line, const char *fileName)
      {
        AutoRecursiveLock lock(mLock);

        LocationPair pair(line, fileName);
        LocationCountMap::iterator found = mLocations.find(pair);
        ZS_THROW_BAD_STATE_IF(found == mLocations.end())

        int &count = (*found).second;
        --count;
        if (0 == count) {
          mLocations.erase(found);
        }
      }

      void output()
      {
        AutoRecursiveLock lock(mLock);
        if (mLocations.size() < 1) return;

        ZS_LOG_BASIC("-------------------------------------------------------------------------------")
        ZS_LOG_BASIC("-------------------------------------------------------------------------------")
        ZS_LOG_BASIC("-------------------------------------------------------------------------------")
        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << "-------------------------------------------------------------------------------\n";

        for (LocationCountMap::iterator iter = mLocations.begin(); iter != mLocations.end(); ++iter) {
          String output = String("FILE=") + (*iter).first.second + " LINE=" + Stringize<int>((*iter).first.first).string() + " COUNT=" + Stringize<int>((*iter).second).string();
          ZS_LOG_WARNING(Basic, output)
          std::cout << output << "\n";
        }

        ZS_LOG_BASIC("-------------------------------------------------------------------------------")
        ZS_LOG_BASIC("-------------------------------------------------------------------------------")
        ZS_LOG_BASIC("-------------------------------------------------------------------------------")
        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << "-------------------------------------------------------------------------------\n";
      }

    private:
      RecursiveLock mLock;

      typedef std::pair<int, const char *> LocationPair;
      typedef std::map<LocationPair, int> LocationCountMap;

      LocationCountMap mLocations;
    };

    class ProxyTrackingGlobalInit
    {
    public:
      ProxyTrackingGlobalInit()
      {
        ProxyTracking::singleton();
      }
    };

    ProxyTrackingGlobalInit gProxyTrackingGlobalInit;
#endif //DEBUG

    static ULONG &getProxyCountGlobal()
    {
      static ULONG total = 0;
      return total;
    }

    void proxyCountIncrement(int line, const char *fileName)
    {
      atomicIncrement(getProxyCountGlobal());
#ifdef DEBUG
      (ProxyTracking::singleton()).follow(line, fileName);
#endif //DEBUG
    }

    void proxyCountDecrement(int line, const char *fileName)
    {
      atomicDecrement(getProxyCountGlobal());
#ifdef DEBUG
      (ProxyTracking::singleton()).unfollow(line, fileName);
#endif //DEBUG
    }
  }

  ULONG proxyGetTotalConstructed()
  {
    return atomicGetValue(internal::getProxyCountGlobal());
  }

  void proxyDump()
  {
#ifdef DEBUG
    (internal::ProxyTracking::singleton()).output();
#endif //DEBUG
  }
}
