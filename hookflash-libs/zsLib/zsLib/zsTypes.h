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

#ifndef ZSLIB_ZSTYPES_H_53fa543745f508d26ae5baeec4bf3770
#define ZSLIB_ZSTYPES_H_53fa543745f508d26ae5baeec4bf3770

#include <zsLib/internal/zsTypes.h>

namespace zsLib
{
  typedef boost::thread Thread;
  typedef boost::shared_ptr<Thread> ThreadPtr;

  typedef boost::mutex Lock;
  typedef boost::recursive_mutex RecursiveLock;
  typedef boost::lock_guard<Lock> AutoLock;
  typedef boost::lock_guard<RecursiveLock> AutoRecursiveLock;

  typedef boost::posix_time::ptime Time;
  typedef boost::posix_time::time_duration Duration;
  typedef boost::posix_time::seconds Seconds;
  typedef boost::posix_time::minutes Minutes;
  typedef boost::posix_time::hours Hours;
  typedef boost::posix_time::milliseconds Milliseconds;
  typedef boost::posix_time::microseconds Microseconds;
#if defined(BOOST_DATE_TIME_HAS_NANOSECONDS)
  typedef boost::posix_time::nanoseconds Nanoseconds;
#endif
  typedef boost::posix_time::time_period TimePeriod;
  typedef boost::posix_time::time_iterator TimeIterator;

  typedef CHAR ZsDeclareCHAR;
  typedef UCHAR ZsDeclareUCHAR;
  typedef SHORT ZsDeclareSHORT;
  typedef USHORT ZsDeclareUSHORT;
  typedef INT ZsDeclareINT;
  typedef UINT ZsDeclareUINT;
  typedef ULONG ZsDeclareULONG;
  typedef LONGLONG ZsDeclareLONGLONG;
  typedef ULONGLONG ZsDeclareULONGLONG;

  typedef FLOAT ZsDeclareFLOAT;
  typedef DOUBLE ZsDeclareDOUBLE;

  typedef BYTE ZsDeclareBYTE;
  typedef WORD ZsDeclareWORD;
  typedef DWORD ZsDeclareDWORD;
  typedef QWORD ZsDeclareQWORD;

  typedef PTRNUMBER ZsDeclarePTRNUMBER;
  typedef USERPARAM ZsDeclareUSERPARAM;

  typedef PUID ZsDeclarePUID;
  typedef UUID ZsDeclareUUID;

  typedef TCHAR ZsDeclareTCHAR;
  typedef WCHAR ZsDeclareWCHAR;

  typedef STR ZsDeclareSTR;
  typedef CSTR ZsDeclareCSTR;
  typedef TSTR ZsDeclareTSTR;
  typedef CTSTR ZsDeclareCTSTR;
  typedef WSTR ZsDeclareWSTR;
  typedef CWSTR ZsDeclareCWSTR;

  class Event;
  typedef boost::shared_ptr<Event> EventPtr;
  typedef boost::weak_ptr<Event> EventWeakPtr;

  class Exception;
  typedef boost::shared_ptr<Exception> ExceptionPtr;
  typedef boost::weak_ptr<Exception> ExceptionWeakPtr;

  class IPAddress;
  typedef boost::shared_ptr<IPAddress> IPAddressPtr;
  typedef boost::weak_ptr<IPAddress> IPAddressWeakPtr;

  class IPAddress;
  typedef boost::shared_ptr<IPAddress> IPAddressPtr;
  typedef boost::weak_ptr<IPAddress> IPAddressWeakPtr;

  interaction ILogDelegate;
  typedef boost::shared_ptr<ILogDelegate> ILogDelegatePtr;
  typedef boost::weak_ptr<ILogDelegate> ILogDelegateWeakPtr;

  class Log;
  typedef boost::shared_ptr<Log> LogPtr;
  typedef boost::weak_ptr<Log> LogWeakPtr;

  interaction IMessageQueueMessage;
  typedef boost::shared_ptr<IMessageQueueMessage> IMessageQueueMessagePtr;
  typedef boost::weak_ptr<IMessageQueueMessage> IMessageQueueMessageWeakPtr;

  interaction IMessageQueueNotify;
  typedef boost::shared_ptr<IMessageQueueNotify> IMessageQueueNotifyPtr;
  typedef boost::weak_ptr<IMessageQueueNotify> IMessageQueueNotifyWeakPtr;

  interaction IMessageQueue;
  typedef boost::shared_ptr<IMessageQueue> IMessageQueuePtr;
  typedef boost::weak_ptr<IMessageQueue> IMessageQueueWeakPtr;

  class MessageQueue;
  typedef boost::shared_ptr<MessageQueue> MessageQueuePtr;
  typedef boost::weak_ptr<MessageQueue> MessageQueueWeakPtr;

  interaction IMessageQueueThread;
  typedef boost::shared_ptr<IMessageQueueThread> IMessageQueueThreadPtr;
  typedef boost::weak_ptr<IMessageQueueThread> IMessageQueueThreadWeakPtr;

  class MessageQueueThread;
  typedef boost::shared_ptr<MessageQueueThread> MessageQueueThreadPtr;
  typedef boost::weak_ptr<MessageQueueThread> MessageQueueThreadWeakPtr;

  interaction ISocket;
  typedef boost::shared_ptr<ISocket> ISocketPtr;
  typedef boost::weak_ptr<ISocket> ISocketWeakPtr;

  interaction ISocketDelegate;
  typedef boost::shared_ptr<ISocketDelegate> ISocketDelegatePtr;
  typedef boost::weak_ptr<ISocketDelegate> ISocketDelegateWeakPtr;

  class Socket;
  typedef boost::shared_ptr<Socket> SocketPtr;
  typedef boost::weak_ptr<Socket> SocketWeakPtr;

  class String;
  typedef boost::shared_ptr<String> StringPtr;
  typedef boost::weak_ptr<String> StringWeakPtr;

  interaction ITimerDelegate;
  typedef boost::shared_ptr<ITimerDelegate> ITimerDelegatePtr;
  typedef boost::weak_ptr<ITimerDelegate> ITimerDelegateWeakPtr;

  class Timer;
  typedef boost::shared_ptr<Timer> TimerPtr;
  typedef boost::weak_ptr<Timer> TimerWeakPtr;

  namespace XML
  {
    class Node;
    typedef boost::shared_ptr<Node> NodePtr;
    typedef boost::weak_ptr<Node> NodeWeakPtr;

    class Document;
    typedef boost::shared_ptr<Document> DocumentPtr;
    typedef boost::weak_ptr<Document> DocumentWeakPtr;

    class Element;
    typedef boost::shared_ptr<Element> ElementPtr;
    typedef boost::weak_ptr<Element> ElementWeakPtr;

    class Attribute;
    typedef boost::shared_ptr<Attribute> AttributePtr;
    typedef boost::weak_ptr<Attribute> AttributeWeakPtr;

    class Text;
    typedef boost::shared_ptr<Text> TextPtr;
    typedef boost::weak_ptr<Text> TextWeakPtr;

    class Comment;
    typedef boost::shared_ptr<Comment> CommentPtr;
    typedef boost::weak_ptr<Comment> CommentWeakPtr;

    class Declaration;
    typedef boost::shared_ptr<Declaration> DeclarationPtr;
    typedef boost::weak_ptr<Declaration> DeclarationWeakPtr;

    class Unknown;
    typedef boost::shared_ptr<Unknown> UnknownPtr;
    typedef boost::weak_ptr<Unknown> UnknownWeakPtr;

    class ParserPos;
    class ParserWarning;
  }

} // namespace zsLib

#endif //ZSLIB_ZSTYPES_H_53fa543745f508d26ae5baeec4bf3770
