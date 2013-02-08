
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

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Exception.h>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{

  namespace internal
  {
    class GlobalLog;
    typedef boost::shared_ptr<GlobalLog> GlobalLogPtr;

    class GlobalLog
    {
    private:
      GlobalLog() {mLog = zsLib::Log::create();}

    public:
      static GlobalLogPtr create() {return GlobalLogPtr(new GlobalLog);}

      LogPtr mLog;
    };

    static GlobalLogPtr &getGlobalLog()
    {
      static GlobalLogPtr log = GlobalLog::create();
      return log;
    }

    LogPtr Log::create()
    {
      return LogPtr(new zsLib::Log);
    }
  } // internal;

  Subsystem::Subsystem(CSTR inName, Log::Level inLevel) :
    mSubsystem(inName),
    mLevel((ULONG)inLevel)
  {
  }

  void Subsystem::notifyNewSubsystem()
  {
    Log::singleton()->notifyNewSubsystem(this);
  }

  void Subsystem::setOutputLevel(Log::Level inLevel)
  {
    ULONG value = (ULONG)inLevel;
    atomicSetValue32(mLevel, value);
  }

  Log::Level Subsystem::getOutputLevel() const
  {
    return (Log::Level)atomicGetValue32(mLevel);
  }

  Log::Log()
  {
  }

  Log::~Log()
  {
  }

  LogPtr Log::singleton()
  {
    return internal::getGlobalLog()->mLog;
  }

  void Log::addListener(ILogDelegatePtr delegate)
  {
    SubsystemList notifyList;
    // copy the list but notify without the lock
    {
      AutoRecursiveLock lock(mLock);
      mListeners.push_back(delegate);
      notifyList = mSubsystems;
    }

    for (SubsystemList::iterator iter = notifyList.begin(); iter != notifyList.end(); ++iter)
    {
      delegate->onNewSubsystem(
                               *(*iter)
                               );
    }
  }

  void Log::removeListener(ILogDelegatePtr delegate)
  {
    AutoRecursiveLock lock(mLock);
    for (ListenerList::iterator iter = mListeners.begin(); iter != mListeners.end(); ++iter)
    {
      if (delegate.get() == (*iter).get())
      {
        mListeners.erase(iter);
        return;
      }
    }
    ZS_THROW_INVALID_ARGUMENT("cound not remove log listener as it was not found")
  }

  void Log::notifyNewSubsystem(Subsystem *inSubsystem)
  {
    ListenerList notifyList;

    // scope: remember the subsystem
    {
      AutoRecursiveLock lock(mLock);
      mSubsystems.push_back(inSubsystem);
      notifyList = mListeners;
    }

    for (ListenerList::iterator iter = notifyList.begin(); iter != notifyList.end(); ++iter)
    {
      (*iter)->onNewSubsystem(
                              *inSubsystem
                              );
    }
  }

  void Log::log(
                const Subsystem &inSubsystem,
                Severity inSeverity,
                Level inLevel,
                const String &inMessage,
                CSTR inFunction,
                CSTR inFilePath,
                ULONG inLineNumber
                )
  {
    if (inLevel > inSubsystem.getOutputLevel())
      return;

    AutoRecursiveLock lock(mLock);
    for (ListenerList::iterator iter = mListeners.begin(); iter != mListeners.end(); ++iter)
    {
      (*iter)->log(
                   inSubsystem,
                   inSeverity,
                   inLevel,
                   inMessage.c_str(),
                   inFunction,
                   inFilePath,
                   inLineNumber
                   );
    }
  }

} // namespace zsLib
