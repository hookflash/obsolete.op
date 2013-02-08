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

#pragma once

#if !defined(ZSLIB_INTERNAL_LOG_H_bae05c798f0d8589029db417219553a9)
#define ZSLIB_INTERNAL_LOG_H_bae05c798f0d8589029db417219553a9

#include <zsLib/types.h>
#include <zsLib/Stringize.h>
#include <list>

namespace zsLib
{
  class String;

  class Subsystem;

  interaction ILogDelegate;
  typedef boost::shared_ptr<ILogDelegate> ILogDelegatePtr;

  class Log;
  typedef boost::shared_ptr<Log> LogPtr;
  typedef boost::weak_ptr<Log> LogWeakPtr;

  namespace internal
  {
    class Log
    {
    public:
      static LogPtr create();

    protected:
      RecursiveLock mLock;
      typedef std::list<ILogDelegatePtr> ListenerList;
      ListenerList mListeners;

      typedef std::list<Subsystem *> SubsystemList;
      SubsystemList mSubsystems;
    };
  }
}


#define ZS_INTERNAL_DECLARE_FORWARD_SUBSYSTEM(xSubsystem) \
  ::zsLib::Subsystem &get##xSubsystem##Subsystem();       \

#define ZS_INTERNAL_DECLARE_SUBSYSTEM(xSubsystem)         \
  ::zsLib::Subsystem &get##xSubsystem##Subsystem();       \
  static ::zsLib::Subsystem &getCurrentSubsystem() {      \
    return get##xSubsystem##Subsystem();                  \
  }

#define ZS_INTERNAL_IMPLEMENT_SUBSYSTEM(xSubsystem) \
  class xSubsystem##Subsystem : public ::zsLib::Subsystem \
  {                                                       \
  public:                                                 \
    xSubsystem##Subsystem(::zsLib::CSTR inName = #xSubsystem) : ::zsLib::Subsystem(inName) {notifyNewSubsystem();} \
  };                                                      \
  ::zsLib::Subsystem &get##xSubsystem##Subsystem() {      \
    static xSubsystem##Subsystem subsystem;               \
    return subsystem;                                     \
    }

#define ZS_INTERNAL_GET_SUBSYSTEM()                                     (getCurrentSubsystem())
#define ZS_INTERNAL_GET_OTHER_SUBSYSTEM(xNamespace, xSubsystem)         (xNamespace::get##xSubsystem##Subsystem())

#define ZS_INTERNAL_FUNCTION_FILE_LINE                                  __FUNCTION__, __FILE__, __LINE__

#define ZS_INTERNAL_GET_LOG_LEVEL()                                     ((ZS_GET_SUBSYSTEM()).getOutputLevel())
#define ZS_INTERNAL_IS_LOGGING(xLevel)                                  (((ZS_GET_SUBSYSTEM()).getOutputLevel()) >= ::zsLib::Log::xLevel)

#define ZS_INTERNAL_LOG_BASIC(xMsg)                                     if (ZS_INTERNAL_IS_LOGGING(Basic))  {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Informational, ::zsLib::Log::Basic, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}
#define ZS_INTERNAL_LOG_DETAIL(xMsg)                                    if (ZS_INTERNAL_IS_LOGGING(Detail)) {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Informational, ::zsLib::Log::Detail, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}
#define ZS_INTERNAL_LOG_DEBUG(xMsg)                                     if (ZS_INTERNAL_IS_LOGGING(Debug))  {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Informational, ::zsLib::Log::Debug, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}
#define ZS_INTERNAL_LOG_TRACE(xMsg)                                     if (ZS_INTERNAL_IS_LOGGING(Trace))  {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Informational, ::zsLib::Log::Trace, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}

#define ZS_INTERNAL_LOG(xLevel, xMsg)                                   if (ZS_INTERNAL_IS_LOGGING(xLevel)) {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Informational, ::zsLib::Log::xLevel, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}
#define ZS_INTERNAL_LOG_WARNING(xLevel, xMsg)                           if (ZS_INTERNAL_IS_LOGGING(xLevel)) {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Warning, ::zsLib::Log::xLevel, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}
#define ZS_INTERNAL_LOG_ERROR(xLevel, xMsg)                             if (ZS_INTERNAL_IS_LOGGING(xLevel)) {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Error, ::zsLib::Log::xLevel, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}
#define ZS_INTERNAL_LOG_FATAL(xLevel, xMsg)                             if (ZS_INTERNAL_IS_LOGGING(xLevel)) {(::zsLib::Log::singleton())->log(ZS_GET_SUBSYSTEM(), ::zsLib::Log::Fatal, ::zsLib::Log::xLevel, ::zsLib::String(xMsg), ZS_INTERNAL_FUNCTION_FILE_LINE);}

#define ZS_INTERNAL_LOG_IF(xCond, xLevel, xMsg)                         if ((xCond) && (ZS_INTERNAL_IS_LOGGING(xLevel))) {ZS_INTERNAL_LOG(xLevel, xMsg)}
#define ZS_INTERNAL_LOG_WARNING_IF(xCond, xLevel, xMsg)                 if ((xCond) && (ZS_INTERNAL_IS_LOGGING(xLevel))) {ZS_INTERNAL_LOG_WARNING(xLevel, xMsg)}
#define ZS_INTERNAL_LOG_ERROR_IF(xCond, xLevel, xMsg)                   if ((xCond) && (ZS_INTERNAL_IS_LOGGING(xLevel))) {ZS_INTERNAL_LOG_ERROR(xLevel, xMsg)}
#define ZS_INTERNAL_LOG_FATAL_IF(xCond, xLevel, xMsg)                   if ((xCond) && (ZS_INTERNAL_IS_LOGGING(xLevel))) {ZS_INTERNAL_LOG_FATAL(xLevel, xMsg)}

#endif //ZS_INTERNAL_LOG_H_bae05c798f0d8589029db417219553a9

