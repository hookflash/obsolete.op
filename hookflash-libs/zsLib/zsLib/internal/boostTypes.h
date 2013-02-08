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

#ifndef ZSLIB_INTERNAL_BOOSTTYPES_H_6f9d2b59fb479e39cbff5e538c98cb1d
#define ZSLIB_INTERNAL_BOOSTTYPES_H_6f9d2b59fb479e39cbff5e538c98cb1d

#pragma once

#ifndef _WIN32
#pragma GCC system_header
#endif //_WIN32

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/tss.hpp>

#pragma warning(push)
#pragma warning(disable:4244)

#include <boost/thread/condition_variable.hpp>

#pragma warning(pop)

#endif //ZSLIB_INTERNAL_BOOSTTYPES_H_6f9d2b59fb479e39cbff5e538c98cb1d
