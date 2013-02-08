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

#ifndef ZSLIB_EXCEPTION_H_128581a1508c405cbdac46b96074b91b
#define ZSLIB_EXCEPTION_H_128581a1508c405cbdac46b96074b91b

#include <zsLib/internal/zsLib_Exception.h>

namespace zsLib
{
  class Exception : public std::exception
  {
  public:
    Exception(
              const Subsystem &subsystem,
              const String &message,
              CSTR function,
              CSTR filePath,
              ULONG lineNumber
              ) :
      mSubsystem(subsystem),
      mMessage(message),
      mFunction(function),
      mLineNumber(lineNumber)
    {
    }
    ~Exception() throw() {}

    virtual const char* what() const throw() {return mMessage;}

    const Subsystem &getSubsystem() const {return mSubsystem;}
    const String &getMessage() const {return mMessage;}
    CSTR getFunction() const {return mFunction;}
    CSTR getFilePath() const {return mFilePath;}
    ULONG getLineNumber() const {return mLineNumber;}

  private:
    const Subsystem &mSubsystem;
    String mMessage;
    CSTR mFunction;
    CSTR mFilePath;
    ULONG mLineNumber;
  };
}

#define ZS_DECLARE_CUSTOM_EXCEPTION(xObject)                                                          ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION(xObject)
#define ZS_DECLARE_CUSTOM_EXCEPTION_ALT_BASE(xObject, xBase)                                          ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE(xObject, xBase)
#define ZS_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_1(xObject, xBase, xType1)                 ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_1(xObject, xBase, xType1)
#define ZS_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_2(xObject, xBase, xType1, xType2)         ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_2(xObject, xBase, xType1, xType2)
#define ZS_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_3(xObject, xBase, xType1, xType2, xType3) ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_3(xObject, xBase, xType1, xType2, xType3)

namespace zsLib
{
  struct Exceptions
  {
    ZS_DECLARE_CUSTOM_EXCEPTION(InvalidArgument)
    ZS_DECLARE_CUSTOM_EXCEPTION(BadState)
    ZS_DECLARE_CUSTOM_EXCEPTION(ResourceError)
    ZS_DECLARE_CUSTOM_EXCEPTION(UnexpectedError)
    ZS_DECLARE_CUSTOM_EXCEPTION(InvalidUsage)
    ZS_DECLARE_CUSTOM_EXCEPTION(InvalidAssumption)
    ZS_DECLARE_CUSTOM_EXCEPTION(NotImplemented)
  };
} // namespace zsLib

#define ZS_THROW_INVALID_ARGUMENT(xMessage)                                                     {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::InvalidArgument(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_BAD_STATE(xMessage)                                                            {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::BadState(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_RESOURCE_ERROR(xMessage)                                                       {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::ResourceError(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_UNEXPECTED_ERROR(xMessage)                                                     {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::UnexpectedError(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_INVALID_USAGE(xMessage)                                                        {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::InvalidUsage(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_INVALID_ASSUMPTION(xMessage)                                                   {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::InvalidAssumption(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_NOT_IMPLEMENTED(xMessage)                                                      {ZS_INTERNAL_THROW_PREFIX(xMessage) throw ::zsLib::Exceptions::NotImplemented(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_CUSTOM(xObject, xMessage)                                                      {ZS_INTERNAL_THROW_PREFIX(xMessage) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__);}
#define ZS_THROW_CUSTOM_PROPERTIES_1(xObject, xValue1, xMessage)                                {ZS_INTERNAL_THROW_PREFIX(xMessage) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__, xValue1);}
#define ZS_THROW_CUSTOM_PROPERTIES_2(xObject, xValue1, xValue2, xMessage)                       {ZS_INTERNAL_THROW_PREFIX(xMessage) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__, xValue1, xValue2);}
#define ZS_THROW_CUSTOM_PROPERTIES_3(xObject, xValue1, xValue2, xValue3, xMessage)              {ZS_INTERNAL_THROW_PREFIX(xMessage) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(xMessage), __FUNCTION__, __FILE__, __LINE__, xValue1, xValue2, xValue3);}

#define ZS_THROW_INVALID_ARGUMENT_IF(xExperssion)                                               {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::InvalidArgument(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_BAD_STATE_IF(xExperssion)                                                      {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::BadState(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_RESOURCE_ERROR_IF(xExperssion)                                                 {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::ResourceError(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_UNEXPECTED_ERROR_IF(xExperssion)                                               {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::UnexpectedError(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_INVALID_USAGE_IF(xExperssion)                                                  {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::InvalidUsage(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_INVALID_ASSUMPTION_IF(xExperssion)                                             {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::InvalidAssumption(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_NOT_IMPLEMENTED_IF(xExperssion)                                                {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::NotImplemented(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_CUSTOM_IF(xObject, xExperssion)                                                {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_CUSTOM_PROPERTIES_1_IF(xObject, xExperssion, xValue1)                          {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__, xValue1);}}
#define ZS_THROW_CUSTOM_PROPERTIES_2_IF(xObject, xExperssion, xValue1, xValue2)                 {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__, xValue1, xValue2);}}
#define ZS_THROW_CUSTOM_PROPERTIES_3_IF(xObject, xExperssion, xValue1, xValue2, xValue3)        {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(#xExperssion), __FUNCTION__, __FILE__, __LINE__, xValue1, xValue2, xValue3);}}

#define ZS_THROW_INVALID_ARGUMENT_MSG_IF(xExperssion, xMessage)                                 {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::InvalidArgument(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (#xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_BAD_STATE_MSG_IF(xExperssion, xMessage)                                        {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::BadState(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_RESOURCE_ERROR_MSG_IF(xExperssion, xMessage)                                   {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::ResourceError(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_UNEXPECTED_ERROR_MSG_IF(xExperssion, xMessage)                                 {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::UnexpectedError(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_INVALID_USAGE_MSG_IF(xExperssion, xMessage)                                    {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::InvalidUsage(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_INVALID_ASSUMPTION_MSG_IF(xExperssion, xMessage)                               {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::InvalidAssumption(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_NOT_IMPLEMENTED_MSG_IF(xExperssion, xMessage)                                  {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw ::zsLib::Exceptions::NotImplemented(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_CUSTOM_MSG_IF(xObject, xExperssion, xMessage)                                  {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__);}}
#define ZS_THROW_CUSTOM_MSG_PROPERTIES_1_IF(xObject, xExperssion, xValue1, xMessage)            {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__, xValue1);}}
#define ZS_THROW_CUSTOM_MSG_PROPERTIES_2_IF(xObject, xExperssion, xValue1, xValue2, xMessage)   {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__, xValue1, xValue2);}}
#define ZS_THROW_CUSTOM_MSG_PROPERTIES_3_IF(xObject, xExperssion, xValue1, xValue2, xValue3, xMessage)   {if (xExperssion) {ZS_INTERNAL_THROW_PREFIX(#xExperssion) throw xObject(ZS_GET_SUBSYSTEM(), ::zsLib::String(::zsLib::String(#xExperssion) + (xMessage)), __FUNCTION__, __FILE__, __LINE__, xValue1, xValue2, xValue3);}}

#endif //ZSLIB_EXCEPTION_H_128581a1508c405cbdac46b96074b91b
