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

#ifndef ZSLIB_INTERNAL_EXCEPTION_H_33206cbc33b745eb5c841eca0b34d550
#define ZSLIB_INTERNAL_EXCEPTION_H_33206cbc33b745eb5c841eca0b34d550

#include <zsLib/zsTypes.h>
#include <zsLib/Log.h>
#include <zsLib/String.h>

#define ZS_INTERNAL_THROW_PREFIX(xMessage) \
  ZS_LOG_ERROR(Basic, xMessage)

#define ZS_INTERNAL_DECLARE_EXCEPTION_PROPERTY(xClass, xPropertyType, xPropertyName) \
  xClass &set##xPropertyName(xPropertyType in##xPropertyName) {xPropertyName = in##xPropertyName; return *this;} \
  xPropertyType &get##xPropertyName() const {return xPropertyName;}

#define ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION(xObject) \
  class xObject : public ::zsLib::Exception \
  { \
  public: \
    xObject( \
      const ::zsLib::Subsystem &subsystem, \
      const ::zsLib::String &message, \
      ::zsLib::CSTR function, \
      ::zsLib::CSTR pathName, \
      ::zsLib::ULONG lineNumber \
    ) : Exception(subsystem, message, function, pathName, lineNumber) \
    { \
    } \
  };

#define ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE(xObject, xBase) \
  class xObject : public xBase \
  { \
  public: \
    xObject( \
      const ::zsLib::Subsystem &subsystem, \
      const ::zsLib::String &message, \
      ::zsLib::CSTR function, \
      ::zsLib::CSTR pathName, \
      ::zsLib::ULONG lineNumber \
    ) : xBase(subsystem, message, function, pathName, lineNumber) \
    { \
    } \
  };

#define ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_1(xObject, xBase, xType1) \
  class xObject : public xBase \
  { \
  public: \
    xObject( \
      const ::zsLib::Subsystem &subsystem, \
      const ::zsLib::String &message, \
      ::zsLib::CSTR function, \
      ::zsLib::CSTR pathName, \
      ::zsLib::ULONG lineNumber, \
      xType1 inValue1 \
    ) : xBase(subsystem, message, function, pathName, lineNumber, inValue1) \
    { \
    } \
  };

#define ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_2(xObject, xBase, xType1, xType2) \
  class xObject : public xBase \
  { \
  public: \
    xObject( \
      const ::zsLib::Subsystem &subsystem, \
      const ::zsLib::String &message, \
      ::zsLib::CSTR function, \
      ::zsLib::CSTR pathName, \
      ::zsLib::ULONG lineNumber, \
      xType1 inValue1, \
      xType2 inValue2 \
    ) : xBase(subsystem, message, function, pathName, lineNumber, inValue1, inValue2) \
  { \
  } \
};

#define ZS_INTERNAL_DECLARE_CUSTOM_EXCEPTION_ALT_BASE_WITH_PROPERTIES_3(xObject, xBase, xType1, xType2, xType3) \
  class xObject : public xBase \
  { \
  public: \
    xObject( \
      const ::zsLib::Subsystem &subsystem, \
      const ::zsLib::String &message, \
      ::zsLib::CSTR function, \
      ::zsLib::CSTR pathName, \
      ::zsLib::ULONG lineNumber, \
      xType1 inValue1, \
      xType2 inValue2, \
      xType3 inValue3, \
    ) : xBase(subsystem, message, function, pathName, lineNumber, inValue1, inValue2, inValue3) \
    { \
    } \
  };

#endif //ZSLIB_INTERNAL_EXCEPTION_H_33206cbc33b745eb5c841eca0b34d550

