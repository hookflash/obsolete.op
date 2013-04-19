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

#include <zsLib/RegEx.h>

#ifdef __QNX__
#include "pcre.h"
#else
#include <boost/regex.hpp>
#endif

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  RegEx::RegEx(const String& regularExpression) : mRegularExpression(regularExpression) {
  }

  bool RegEx::hasMatch(const String& stringToSearch)
  {
#ifdef __QNX__
    const char *errorMsg;
    int errorOffset;
    pcre* re = pcre_compile(mRegularExpression.c_str(), 0, &errorMsg, &errorOffset, 0);
    if(!re) {
      ZS_THROW_CUSTOM(Exceptions::ParseError, ("Failed to parse regular expression: " + mRegularExpression).c_str())
    }
    pcre_extra* study = pcre_study(re, 0, &errorMsg);

    int ovector[30];
    int rc = pcre_exec(re, study, stringToSearch.c_str(), stringToSearch.size(), 0, 0, ovector, 30);
    pcre_free(re);
    if(study != NULL) {
      pcre_free(study);
    }

    if(rc <= 0) {
      return false;
    }
    return true;

#else

    const boost::regex e(mRegularExpression);
    if (!boost::regex_match(stringToSearch, e)) {
      return false;
    }
    return true;
#endif
  }
}
