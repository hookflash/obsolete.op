/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2011. Robin Raymond. All rights reserved.
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

#ifndef ZSLIB_LOG_H_47d9d31085744072b865483d323d7b02
#define ZSLIB_LOG_H_47d9d31085744072b865483d323d7b02

#include <zsLib/internal/Log.h>

// forward declare a subsystem (for use in header files); wrap inside the namespace where it should be defined
#define ZS_DECLARE_FORWARD_SUBSYSTEM(xSubsystem)                        ZS_INTERNAL_DECLARE_FORWARD_SUBSYSTEM(xSubsystem)

// declare a subsystem; wrap inside the namespace where it should be defined
#define ZS_DECLARE_SUBSYSTEM(xSubsystem)                                ZS_INTERNAL_DECLARE_SUBSYSTEM(xSubsystem)

// implement the code for a subsystem; wrap inside the same namespace as all the other declations
#define ZS_IMPLEMENT_SUBSYSTEM(xSubsystem)                              ZS_INTERNAL_IMPLEMENT_SUBSYSTEM(xSubsystem)

// return a reference to the current Subsystem
#define ZS_GET_SUBSYSTEM()                                              ZS_INTERNAL_GET_SUBSYSTEM()

/// get a subsystem that is not part of this subsystem
#define ZS_GET_OTHER_SUBSYSTEM(xNamespace, xSubsystem)                  ZS_INTERNAL_GET_OTHER_SUBSYSTEM(xNamespace, xSubsystem)

#define ZS_GET_LOG_LEVEL()                                              ZS_INTERNAL_GET_LOG_LEVEL()
#define ZS_IS_LOGGING(xLevel)                                           ZS_INTERNAL_IS_LOGGING(xLevel)

#define ZS_LOG_BASIC(xMsg)                                              ZS_INTERNAL_LOG_BASIC(xMsg)
#define ZS_LOG_DETAIL(xMsg)                                             ZS_INTERNAL_LOG_DETAIL(xMsg)
#define ZS_LOG_DEBUG(xMsg)                                              ZS_INTERNAL_LOG_DEBUG(xMsg)
#define ZS_LOG_TRACE(xMsg)                                              ZS_INTERNAL_LOG_TRACE(xMsg)

#define ZS_TRACE()                                                      ZS_INTERNAL_LOG_TRACE("[TRACE]")
#define ZS_TRACE_THIS()                                                 ZS_INTERNAL_LOG_TRACE("[TRACE THIS=" + (::zsLib::Stringize< ::zsLib::PTRNUMBER >((::zsLib::PTRNUMBER)this)).string() + "]")

#define ZS_LOG(xLevel, xMsg)                                            ZS_INTERNAL_LOG(xLevel, xMsg)
#define ZS_LOG_WARNING(xLevel, xMsg)                                    ZS_INTERNAL_LOG_WARNING(xLevel, xMsg)
#define ZS_LOG_ERROR(xLevel, xMsg)                                      ZS_INTERNAL_LOG_ERROR(xLevel, xMsg)
#define ZS_LOG_FATAL(xLevel, xMsg)                                      ZS_INTERNAL_LOG_FATAL(xLevel, xMsg)

#define ZS_LOG_IF(xCond, xLevel, xMsg)                                  ZS_INTERNAL_LOG_IF(xCond, xLevel, xMsg)
#define ZS_LOG_WARNING_IF(xCond, xLevel, xMsg)                          ZS_INTERNAL_LOG_WARNING_IF(xCond, xLevel, xMsg)
#define ZS_LOG_ERROR_IF(xCond, xLevel, xMsg)                            ZS_INTERNAL_LOG_ERROR_IF(xCond, xLevel, xMsg)
#define ZS_LOG_FATAL_IF(xCond, xLevel, xMsg)                            ZS_INTERNAL_LOG_FATAL_IF(xCond, xLevel, xMsg)

namespace zsLib
{
  class Log : public internal::Log
  {
    friend class internal::Log;

  public:
    enum Severity
    {
      Informational,
      Warning,
      Error,
      Fatal
    };

    enum Level
    {
      None,
      Basic,
      Detail,
      Debug,
      Trace
    };

  public:
    ~Log();

    static LogPtr singleton();

    void addListener(ILogDelegatePtr delegate);
    void removeListener(ILogDelegatePtr delegate);

    void notifyNewSubsystem(Subsystem *inSubsystem);

    void log(
             const Subsystem &subsystem,
             Severity severity,
             Level level,
             const String &message,
             CSTR function,
             CSTR filePath,
             ULONG lineNumber
             );

  private:
    Log();
  };

  interaction ILogDelegate
  {
  public:
    // notification that a new subsystem exists
    virtual void onNewSubsystem(zsLib::Subsystem &inSubsystem) {}

    // notification of a log event
    virtual void log(
                     const zsLib::Subsystem &inSubsystem,
                     zsLib::Log::Severity inSeverity,
                     zsLib::Log::Level inLevel,
                     zsLib::CSTR inMessage,
                     zsLib::CSTR inFunction,
                     zsLib::CSTR inFilePath,
                     zsLib::ULONG inLineNumber
                     ) {}
  };

  class Subsystem
  {
  public:
    Subsystem(CSTR inName, Log::Level inLevel = Log::Basic);
    CSTR getName() const {return mSubsystem;}

    void setOutputLevel(Log::Level inLevel);
    Log::Level getOutputLevel() const;

  protected:
    virtual void notifyNewSubsystem();

  private:
    CSTR mSubsystem;
    mutable DWORD mLevel;
  };

} // namespace zsLib

#endif //ZSLIB_LOG_H_47d9d31085744072b865483d323d7b02
