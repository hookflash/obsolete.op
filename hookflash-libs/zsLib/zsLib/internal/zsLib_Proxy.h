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

#ifndef ZSLIB_INTERNAL_PROXY_H_a1792950ebd2df4b616a6b341965c42d
#define ZSLIB_INTERNAL_PROXY_H_a1792950ebd2df4b616a6b341965c42d

#include <zsLib/types.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/internal/zsLib_ProxyPack.h>

#define ZS_LIB_NO_CONST
#define ZS_LIB_CONST const

namespace zsLib
{
  template <typename XINTERFACE>
  class Proxy;

  ZS_DECLARE_FORWARD_SUBSYSTEM(zsLib)

  namespace internal
  {
    void proxyCountIncrement(int line, const char *fileName);
    void proxyCountDecrement(int line, const char *fileName);

    template <typename XINTERFACE>
    class Proxy : public XINTERFACE
    {
    public:
      Proxy(IMessageQueuePtr queue, boost::shared_ptr<XINTERFACE> delegate, int line, const char *fileName) : mQueue(queue), mDelegate(delegate), mNoop(false), mIgnoreMethodCall(false), mLine(line), mFileName(fileName) {proxyCountIncrement(mLine, mFileName);}
      Proxy(IMessageQueuePtr queue, boost::weak_ptr<XINTERFACE> delegateWeakPtr, int line, const char *fileName) : mQueue(queue), mWeakDelegate(delegateWeakPtr), mNoop(false), mIgnoreMethodCall(false), mLine(line), mFileName(fileName) {proxyCountIncrement(mLine, mFileName);}
      Proxy(IMessageQueuePtr queue, bool throwsDelegateGone, int line, const char *fileName) : mQueue(queue), mNoop(true), mIgnoreMethodCall(!throwsDelegateGone), mLine(line), mFileName(fileName) {proxyCountIncrement(mLine, mFileName);}
      ~Proxy() {proxyCountDecrement(mLine, mFileName);}

      boost::shared_ptr<XINTERFACE> getDelegate() const
      {
        if (mDelegate) {
          return mDelegate;
        }
        boost::shared_ptr<XINTERFACE> result = mWeakDelegate.lock();
        if (!result) {throwDelegateGone(typeid(XINTERFACE).name());}
        return result;
      }

      boost::shared_ptr<XINTERFACE> getDelegate(bool throwDelegateGone) const
      {
        if (throwDelegateGone) return getDelegate();
        if (mDelegate) {
          return mDelegate;
        }
        return mWeakDelegate.lock();
      }

      IMessageQueuePtr getQueue() const
      {
        return mQueue;
      }

      bool isNoop() const {return mNoop;}

      bool ignoreMethodCall() const {return mIgnoreMethodCall;}

    protected:
      virtual void throwDelegateGone(const char *name) const = 0;

    protected:
      IMessageQueuePtr mQueue;
      boost::shared_ptr<XINTERFACE> mDelegate;
      boost::weak_ptr<XINTERFACE> mWeakDelegate;
      int mLine;
      const char *mFileName;
      bool mNoop;
      bool mIgnoreMethodCall;
    };
  }
}

#define ZS_INTERNAL_DECLARE_PROXY_BEGIN(xInterface)                                                           \
namespace zsLib                                                                                               \
{                                                                                                             \
  template<>                                                                                                  \
  class Proxy<xInterface> : public internal::Proxy<xInterface>                                                \
  {                                                                                                           \
  public:                                                                                                     \
    struct Exceptions                                                                                         \
    {                                                                                                         \
      ZS_DECLARE_CUSTOM_EXCEPTION_ALT_BASE(DelegateGone, ProxyBase::Exceptions::DelegateGone)                 \
    };                                                                                                        \
    typedef xInterface                    Delegate;                                                           \
    typedef boost::shared_ptr<xInterface> DelegatePtr;                                                        \
    typedef boost::weak_ptr<xInterface>   DelegateWeakPtr;                                                    \
    typedef Proxy<xInterface>             ProxyType;                                                          \
    typedef boost::shared_ptr<ProxyType>  ProxyPtr;                                                           \
                                                                                                              \
  private:                                                                                                    \
    Proxy(IMessageQueuePtr queue, DelegatePtr delegate, int line, const char *fileName) : internal::Proxy<xInterface>(queue, delegate, line, fileName) {}     \
    Proxy(IMessageQueuePtr queue, DelegateWeakPtr delegate, int line, const char *fileName) : internal::Proxy<xInterface>(queue, delegate, line, fileName) {} \
    Proxy(IMessageQueuePtr queue, bool throwsDelegateGone, int line, const char *fileName) : internal::Proxy<xInterface>(queue, throwsDelegateGone, line, fileName) {} \
                                                                                                              \
  public:                                                                                                     \
    static DelegatePtr create(DelegatePtr delegate, bool throwDelegateGone = false, int line = __LINE__, const char *fileName = __FILE__)   \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return delegate;                                                                                      \
                                                                                                              \
      IMessageQueuePtr queue;                                                                                 \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      if (proxy) {                                                                                            \
        if (proxy->isNoop())                                                                                  \
          return delegate;                                                                                    \
        delegate = proxy->getDelegate(throwDelegateGone);                                                     \
        if (!delegate)                                                                                        \
          return delegate;                                                                                    \
        queue = proxy->getQueue();                                                                            \
      }                                                                                                       \
                                                                                                              \
      MessageQueueAssociator *associator = dynamic_cast<MessageQueueAssociator *>(delegate.get());            \
      if (associator)                                                                                         \
        queue =  associator->getAssociatedMessageQueue();                                                     \
                                                                                                              \
      if (!queue)                                                                                             \
        return delegate;                                                                                      \
                                                                                                              \
      return ProxyPtr(new ProxyType(queue, delegate, line, fileName));                                        \
    }                                                                                                         \
                                                                                                              \
    static DelegatePtr create(IMessageQueuePtr queue, DelegatePtr delegate, bool throwDelegateGone = false, int line = __LINE__, const char *fileName = __FILE__)   \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return delegate;                                                                                      \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      if (proxy) {                                                                                            \
        if (proxy->isNoop())                                                                                  \
          return delegate;                                                                                    \
        delegate = proxy->getDelegate(throwDelegateGone);                                                     \
        if (!delegate)                                                                                        \
          return delegate;                                                                                    \
        if (!queue)                                                                                           \
          queue = proxy->getQueue();                                                                          \
      }                                                                                                       \
                                                                                                              \
      MessageQueueAssociator *associator = dynamic_cast<MessageQueueAssociator *>(delegate.get());            \
      if (associator)                                                                                         \
        queue = (associator->getAssociatedMessageQueue() ? associator->getAssociatedMessageQueue() : queue);  \
                                                                                                              \
      if (!queue) {                                                                                           \
        return delegate;                                                                                      \
      }                                                                                                       \
                                                                                                              \
      return ProxyPtr(new ProxyType(queue, delegate, line, fileName));                                        \
    }                                                                                                         \
                                                                                                              \
    static DelegatePtr createWeak(DelegatePtr delegate, bool throwDelegateGone = false, int line = __LINE__, const char *fileName = __FILE__)                       \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return delegate;                                                                                      \
                                                                                                              \
      IMessageQueuePtr queue;                                                                                 \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      if (proxy) {                                                                                            \
        if (proxy->isNoop())                                                                                  \
          return delegate;                                                                                    \
        delegate = proxy->getDelegate(throwDelegateGone);                                                     \
        if (!delegate)                                                                                        \
          return delegate;                                                                                    \
        queue = proxy->getQueue();                                                                            \
      }                                                                                                       \
                                                                                                              \
      MessageQueueAssociator *associator = dynamic_cast<MessageQueueAssociator *>(delegate.get());            \
      if (associator)                                                                                         \
        queue =  associator->getAssociatedMessageQueue();                                                     \
                                                                                                              \
      if (!queue)                                                                                             \
        return delegate;                                                                                      \
                                                                                                              \
      return ProxyPtr(new ProxyType(queue, DelegateWeakPtr(delegate), line, fileName));                       \
    }                                                                                                         \
                                                                                                              \
    static DelegatePtr createWeak(IMessageQueuePtr queue, DelegatePtr delegate, bool throwDelegateGone = false, int line = __LINE__, const char *fileName = __FILE__) \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return delegate;                                                                                      \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      if (proxy) {                                                                                            \
        if (proxy->isNoop())                                                                                  \
          return delegate;                                                                                    \
        delegate = proxy->getDelegate(throwDelegateGone);                                                     \
        if (!delegate)                                                                                        \
          return delegate;                                                                                    \
        if (!queue)                                                                                           \
          queue = proxy->getQueue();                                                                          \
      }                                                                                                       \
                                                                                                              \
      MessageQueueAssociator *associator = dynamic_cast<MessageQueueAssociator *>(delegate.get());            \
      if (associator)                                                                                         \
        queue = (associator->getAssociatedMessageQueue() ? associator->getAssociatedMessageQueue() : queue);  \
                                                                                                              \
      if (!queue) {                                                                                           \
        return delegate;                                                                                      \
      }                                                                                                       \
                                                                                                              \
      return ProxyPtr(new ProxyType(queue, DelegateWeakPtr(delegate), line, fileName));                       \
    }                                                                                                         \
                                                                                                              \
    static DelegatePtr createNoop(IMessageQueuePtr queue, bool throwsDelegateGone = false, int line = __LINE__, const char *fileName = __FILE__) \
    {                                                                                                         \
      if (!queue) {                                                                                           \
        return DelegatePtr();                                                                                 \
      }                                                                                                       \
                                                                                                              \
      return ProxyPtr(new ProxyType(queue, throwsDelegateGone, line, fileName));                              \
    }                                                                                                         \
                                                                                                              \
    static bool isProxy(DelegatePtr delegate)                                                                 \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return false;                                                                                         \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      return (proxy ? true : false);                                                                          \
    }                                                                                                         \
                                                                                                              \
    static DelegatePtr original(DelegatePtr delegate, bool throwDelegateGone = false)                         \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return delegate;                                                                                      \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      if (proxy) {                                                                                            \
        if (proxy->isNoop())                                                                                  \
          return delegate;                                                                                    \
        return proxy->getDelegate(throwDelegateGone);                                                         \
      }                                                                                                       \
      return delegate;                                                                                        \
    }                                                                                                         \
                                                                                                              \
    static IMessageQueuePtr getAssociatedMessageQueue(DelegatePtr delegate)                                   \
    {                                                                                                         \
      if (!delegate)                                                                                          \
        return IMessageQueuePtr();                                                                            \
                                                                                                              \
      ProxyType *proxy = dynamic_cast<ProxyType *>(delegate.get());                                           \
      if (proxy) {                                                                                            \
        return proxy->getQueue();                                                                             \
      }                                                                                                       \
                                                                                                              \
      MessageQueueAssociator *associator = dynamic_cast<MessageQueueAssociator *>(delegate.get());            \
      if (associator)                                                                                         \
        return associator->getAssociatedMessageQueue();                                                       \
                                                                                                              \
      return IMessageQueuePtr();                                                                              \
    }                                                                                                         \
                                                                                                              \
    void throwDelegateGone(const char *name) const                                                            \
    {                                                                                                         \
      throw Exceptions::DelegateGone(ZS_GET_OTHER_SUBSYSTEM(::zsLib, zsLib), ::zsLib::String("Proxy points to destroyed delegate: ") + name, __FUNCTION__, __FILE__, __LINE__); \
    }


#define ZS_INTERNAL_DECLARE_PROXY_END()                                                                       \
  };                                                                                                          \
}

#define ZS_INTERNAL_DECLARE_PROXY_TYPEDEF(xOriginalType, xTypeAlias)                                          \
    typedef xOriginalType xTypeAlias;


#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_0(xConst, xMethod)                                              \
    virtual void xMethod() xConst {                                                                           \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod();                                                                               \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_1(xConst, xMethod,t1)                                           \
    virtual void xMethod(t1 v1) xConst {                                                                      \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1);                                                                             \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_2(xConst, xMethod,t1,t2)                                        \
    virtual void xMethod(t1 v1, t2 v2) xConst {                                                               \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2);                                                                          \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_3(xConst, xMethod,t1,t2,t3)                                     \
    virtual void xMethod(t1 v1,t2 v2,t3 v3) xConst {                                                          \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3);                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_4(xConst, xMethod,t1,t2,t3,t4)                                  \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4) xConst {                                                    \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4);                                                                    \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_5(xConst, xMethod,t1,t2,t3,t4,t5)                               \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5) xConst {                                              \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5);                                                                 \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_6(xConst, xMethod,t1,t2,t3,t4,t5,t6)                            \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6) xConst {                                        \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6);                                                              \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_7(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7)                         \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7) xConst {                                  \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7);                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_8(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8)                      \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8) xConst {                            \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8);                                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_9(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9)                   \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9) xConst {                      \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9);                                                     \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_10(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10)              \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10) xConst {              \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10);                                                 \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_11(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11)          \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11) xConst {      \
      if (ignoreMethodCall()) return;                                                                         \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11);                                             \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_12(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12)            \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12) xConst {    \
      if (ignoreMethodCall()) return;                                                                               \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12);                                               \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_13(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13)                                                                                                      \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13) xConst {                                                                                          \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13);                                                                                                                                         \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_14(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14)                                                                                                  \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14) xConst {                                                                                  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14);                                                                                                                                     \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_15(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15)                                                                                              \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15) xConst {                                                                          \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15);                                                                                                                                 \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_16(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16)                                                                                          \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16) xConst {                                                                  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16);                                                                                                                             \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_17(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17)                                                                                      \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17) xConst {                                                          \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17);                                                                                                                         \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_18(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18)                                                                                  \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18) xConst {                                                  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18);                                                                                                                     \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_19(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19)                                                                              \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19) xConst {                                          \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19);                                                                                                                 \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_20(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20)                                                                          \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20) xConst {                                  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20);                                                                                                             \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_21(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21)                                                                      \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21) xConst {                          \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21);                                                                                                         \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_22(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22)                                                                  \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22) xConst {                  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22);                                                                                                     \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_23(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23)                                                              \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23) xConst {          \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23);                                                                                                 \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_24(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24)                                                          \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24) xConst {  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24);                                                                                             \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_25(xConst, xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24,t25)                                                              \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24,t25 v25) xConst {  \
      if (ignoreMethodCall()) return;                                                                                                                                                                             \
      getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25);                                                                                                 \
    }


#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_0(xConst, xMethod,r1)                                  \
    virtual r1 xMethod() xConst {                                                                           \
      return getDelegate()->xMethod();                                                                      \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_1(xConst, xMethod,r1,t1)                               \
    virtual r1 xMethod(t1 v1) xConst {                                                                      \
      return getDelegate()->xMethod(v1);                                                                    \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_2(xConst, xMethod,r1,t1,t2)                            \
    virtual r1 xMethod(t1 v1, t2 v2) xConst {                                                               \
      return getDelegate()->xMethod(v1,v2);                                                                 \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_3(xConst, xMethod,r1,t1,t2,t3)                         \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3) xConst {                                                          \
      return getDelegate()->xMethod(v1,v2,v3);                                                              \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_4(xConst, xMethod,r1,t1,t2,t3,t4)                      \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4) xConst {                                                    \
      return getDelegate()->xMethod(v1,v2,v3,v4);                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_5(xConst, xMethod,r1,t1,t2,t3,t4,t5)                   \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5) xConst {                                              \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5);                                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_6(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6)                \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6) xConst {                                        \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6);                                                     \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_7(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7)             \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7) xConst {                                  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7);                                                  \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_8(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8)          \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8) xConst {                            \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8);                                               \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_9(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9)       \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9) xConst {                      \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9);                                            \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_10(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10)  \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10) xConst {              \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10);                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_11(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11)      \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11) xConst {              \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11);                                            \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_12(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12)  \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12) xConst {      \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12);                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_13(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13)                                                                                          \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13) xConst {                                                                                          \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13);                                                                                                                                \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_14(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14)                                                                                      \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14) xConst {                                                                                  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14);                                                                                                                            \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_15(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15)                                                                                  \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15) xConst {                                                                          \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15);                                                                                                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_16(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16)                                                                              \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16) xConst {                                                                  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16);                                                                                                                    \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_17(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17)                                                                          \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17) xConst {                                                          \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17);                                                                                                                \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_18(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18)                                                                      \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18) xConst {                                                  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18);                                                                                                            \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_19(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19)                                                                  \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19) xConst {                                          \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19);                                                                                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_20(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20)                                                              \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20) xConst {                                  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20);                                                                                                    \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_21(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21)                                                          \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21) xConst {                          \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21);                                                                                                \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_22(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22)                                                      \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22) xConst {                  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22);                                                                                            \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_23(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23)                                                  \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23) xConst {          \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23);                                                                                        \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_24(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24)                                              \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24) xConst {  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24);                                                                                    \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_SYNC_RETURN_25(xConst, xMethod,r1,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24,t25)                                                  \
    virtual r1 xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24,t25 v25) xConst {  \
      return getDelegate()->xMethod(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25);                                                                                        \
    }


#define ZS_INTERNAL_DECLARE_PROXY_METHOD_0(xMethod)                                                                                                 \
    class Stub_0_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
    public:                                                                                                                                         \
      Stub_0_##xMethod(DelegatePtr delegate) : mDelegate(delegate) { }                                                                              \
      ~Stub_0_##xMethod() { }                                                                                                                       \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod();                                                                                                                       \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod() {                                                                                                                        \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_0_##xMethod> stub(new Stub_0_##xMethod(getDelegate()));                                                                \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_1(xMethod,t1)                                                                                              \
    class Stub_1_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1;                                                                                                                                        \
    public:                                                                                                                                         \
      Stub_1_##xMethod(DelegatePtr delegate,t1 v1) : mDelegate(delegate) {                                                                          \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_1_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1);                                                                                                                     \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1) {                                                                                                                   \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_1_##xMethod> stub(new Stub_1_##xMethod(getDelegate(),v1));                                                             \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_2(xMethod,t1,t2)                                                                                           \
    class Stub_2_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2;                                                                                                                                 \
    public:                                                                                                                                         \
      Stub_2_##xMethod(DelegatePtr delegate,t1 v1,t2 v2) : mDelegate(delegate) {                                                                    \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_2_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2);                                                                                                                  \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2) {                                                                                                             \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_2_##xMethod> stub(new Stub_2_##xMethod(getDelegate(),v1,v2));                                                          \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_3(xMethod,t1,t2,t3)                                                                                        \
    class Stub_3_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3;                                                                                                                          \
    public:                                                                                                                                         \
      Stub_3_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3) : mDelegate(delegate) {                                                              \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_3_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3);                                                                                                               \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3) {                                                                                                       \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_3_##xMethod> stub(new Stub_3_##xMethod(getDelegate(),v1,v2,v3));                                                       \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_4(xMethod,t1,t2,t3,t4)                                                                                     \
    class Stub_4_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4;                                                                                                                   \
    public:                                                                                                                                         \
      Stub_4_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4) : mDelegate(delegate) {                                                        \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_4_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4);                                                                                                            \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4) {                                                                                                 \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_4_##xMethod> stub(new Stub_4_##xMethod(getDelegate(),v1,v2,v3,v4));                                                    \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_5(xMethod,t1,t2,t3,t4,t5)                                                                                  \
    class Stub_5_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5;                                                                                                            \
    public:                                                                                                                                         \
      Stub_5_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5) : mDelegate(delegate) {                                                  \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_5_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5);                                                                                                         \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5) {                                                                                           \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_5_##xMethod> stub(new Stub_5_##xMethod(getDelegate(),v1,v2,v3,v4,v5));                                                 \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_6(xMethod,t1,t2,t3,t4,t5,t6)                                                                               \
    class Stub_6_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6;                                                                                                     \
    public:                                                                                                                                         \
      Stub_6_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6) : mDelegate(delegate) {                                            \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_6_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6);                                                                                                      \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6) {                                                                                     \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_6_##xMethod> stub(new Stub_6_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6));                                              \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_7(xMethod,t1,t2,t3,t4,t5,t6,t7)                                                                            \
    class Stub_7_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7;                                                                                              \
    public:                                                                                                                                         \
      Stub_7_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7) : mDelegate(delegate) {                                      \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
        internal::ProxyPack<t7>(m7, v7);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_7_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
        internal::ProxyClean<t7>(m7);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7);                                                                                                   \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7) {                                                                               \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_7_##xMethod> stub(new Stub_7_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7));                                           \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_8(xMethod,t1,t2,t3,t4,t5,t6,t7,t8)                                                                         \
    class Stub_8_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8;                                                                                       \
    public:                                                                                                                                         \
      Stub_8_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8) : mDelegate(delegate) {                                \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
        internal::ProxyPack<t7>(m7, v7);                                                                                                            \
        internal::ProxyPack<t8>(m8, v8);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_8_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
        internal::ProxyClean<t7>(m7);                                                                                                               \
        internal::ProxyClean<t8>(m8);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8);                                                                                                \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8) {                                                                         \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_8_##xMethod> stub(new Stub_8_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8));                                        \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_9(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9)                                                                      \
    class Stub_9_##xMethod : public IMessageQueueMessage                                                                                            \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9;                                                                                \
    public:                                                                                                                                         \
      Stub_9_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9) : mDelegate(delegate) {                          \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
        internal::ProxyPack<t7>(m7, v7);                                                                                                            \
        internal::ProxyPack<t8>(m8, v8);                                                                                                            \
        internal::ProxyPack<t9>(m9, v9);                                                                                                            \
      }                                                                                                                                             \
      ~Stub_9_##xMethod() {                                                                                                                         \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
        internal::ProxyClean<t7>(m7);                                                                                                               \
        internal::ProxyClean<t8>(m8);                                                                                                               \
        internal::ProxyClean<t9>(m9);                                                                                                               \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9);                                                                                             \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9) {                                                                   \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_9_##xMethod> stub(new Stub_9_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9));                                     \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_10(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10)                                                                 \
    class Stub_10_##xMethod : public IMessageQueueMessage                                                                                           \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10;                                                                       \
    public:                                                                                                                                         \
      Stub_10_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10) : mDelegate(delegate) {                 \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
        internal::ProxyPack<t7>(m7, v7);                                                                                                            \
        internal::ProxyPack<t8>(m8, v8);                                                                                                            \
        internal::ProxyPack<t9>(m9, v9);                                                                                                            \
        internal::ProxyPack<t10>(m10, v10);                                                                                                         \
      }                                                                                                                                             \
      ~Stub_10_##xMethod() {                                                                                                                        \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
        internal::ProxyClean<t7>(m7);                                                                                                               \
        internal::ProxyClean<t8>(m8);                                                                                                               \
        internal::ProxyClean<t9>(m9);                                                                                                               \
        internal::ProxyClean<t10>(m10);                                                                                                             \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10);                                                                                         \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10) {                                                           \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_10_##xMethod> stub(new Stub_10_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10));                               \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_11(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11)                                                             \
    class Stub_11_##xMethod : public IMessageQueueMessage                                                                                           \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11;                                                              \
    public:                                                                                                                                         \
      Stub_11_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11) : mDelegate(delegate) {         \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
        internal::ProxyPack<t7>(m7, v7);                                                                                                            \
        internal::ProxyPack<t8>(m8, v8);                                                                                                            \
        internal::ProxyPack<t9>(m9, v9);                                                                                                            \
        internal::ProxyPack<t10>(m10, v10);                                                                                                         \
        internal::ProxyPack<t11>(m11, v11);                                                                                                         \
      }                                                                                                                                             \
      ~Stub_11_##xMethod() {                                                                                                                        \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
        internal::ProxyClean<t7>(m7);                                                                                                               \
        internal::ProxyClean<t8>(m8);                                                                                                               \
        internal::ProxyClean<t9>(m9);                                                                                                               \
        internal::ProxyClean<t10>(m10);                                                                                                             \
        internal::ProxyClean<t11>(m11);                                                                                                             \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11);                                                                                     \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11) {                                                   \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_11_##xMethod> stub(new Stub_11_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11));                           \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_12(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12)                                                         \
    class Stub_12_##xMethod : public IMessageQueueMessage                                                                                           \
    {                                                                                                                                               \
    private:                                                                                                                                        \
      DelegatePtr mDelegate;                                                                                                                        \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12;                                                     \
    public:                                                                                                                                         \
      Stub_12_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                            \
        internal::ProxyPack<t2>(m2, v2);                                                                                                            \
        internal::ProxyPack<t3>(m3, v3);                                                                                                            \
        internal::ProxyPack<t4>(m4, v4);                                                                                                            \
        internal::ProxyPack<t5>(m5, v5);                                                                                                            \
        internal::ProxyPack<t6>(m6, v6);                                                                                                            \
        internal::ProxyPack<t7>(m7, v7);                                                                                                            \
        internal::ProxyPack<t8>(m8, v8);                                                                                                            \
        internal::ProxyPack<t9>(m9, v9);                                                                                                            \
        internal::ProxyPack<t10>(m10, v10);                                                                                                         \
        internal::ProxyPack<t11>(m11, v11);                                                                                                         \
        internal::ProxyPack<t12>(m12, v12);                                                                                                         \
      }                                                                                                                                             \
      ~Stub_12_##xMethod() {                                                                                                                        \
        internal::ProxyClean<t1>(m1);                                                                                                               \
        internal::ProxyClean<t2>(m2);                                                                                                               \
        internal::ProxyClean<t3>(m3);                                                                                                               \
        internal::ProxyClean<t4>(m4);                                                                                                               \
        internal::ProxyClean<t5>(m5);                                                                                                               \
        internal::ProxyClean<t6>(m6);                                                                                                               \
        internal::ProxyClean<t7>(m7);                                                                                                               \
        internal::ProxyClean<t8>(m8);                                                                                                               \
        internal::ProxyClean<t9>(m9);                                                                                                               \
        internal::ProxyClean<t10>(m10);                                                                                                             \
        internal::ProxyClean<t11>(m11);                                                                                                             \
        internal::ProxyClean<t12>(m12);                                                                                                             \
      }                                                                                                                                             \
                                                                                                                                                    \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                 \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                  \
      virtual void processMessage() {                                                                                                               \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12);                                                                                 \
      }                                                                                                                                             \
    };                                                                                                                                              \
                                                                                                                                                    \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12) {                                           \
      if (ignoreMethodCall()) return;                                                                                                               \
      boost::shared_ptr<Stub_12_##xMethod> stub(new Stub_12_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12));                       \
      mQueue->post(stub);                                                                                                                           \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_13(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13)                                                                                                                 \
    class Stub_13_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13;                                                                                                        \
    public:                                                                                                                                                                                                     \
      Stub_13_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13) : mDelegate(delegate) {                                                     \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_13_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13);                                                                                                                                         \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13) {                                                                                               \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_13_##xMethod> stub(new Stub_13_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13));                                                                               \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_14(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14)                                                                                                             \
    class Stub_14_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14;                                                                                               \
    public:                                                                                                                                                                                                     \
      Stub_14_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14) : mDelegate(delegate) {                                             \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_14_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14);                                                                                                                                     \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14) {                                                                                       \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_14_##xMethod> stub(new Stub_14_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14));                                                                           \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_15(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15)                                                                                                         \
    class Stub_15_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15;                                                                                      \
    public:                                                                                                                                                                                                     \
      Stub_15_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15) : mDelegate(delegate) {                                     \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_15_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15);                                                                                                                                 \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15) {                                                                               \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_15_##xMethod> stub(new Stub_15_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15));                                                                       \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_16(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16)                                                                                                     \
    class Stub_16_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16;                                                                             \
    public:                                                                                                                                                                                                     \
      Stub_16_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16) : mDelegate(delegate) {                             \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_16_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16);                                                                                                                             \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16) {                                                                       \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_16_##xMethod> stub(new Stub_16_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16));                                                                   \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_17(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17)                                                                                                 \
    class Stub_17_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17;                                                                    \
    public:                                                                                                                                                                                                     \
      Stub_17_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17) : mDelegate(delegate) {                     \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_17_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17);                                                                                                                         \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17) {                                                               \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_17_##xMethod> stub(new Stub_17_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17));                                                               \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_18(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18)                                                                                             \
    class Stub_18_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18;                                                           \
    public:                                                                                                                                                                                                     \
      Stub_18_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18) : mDelegate(delegate) {             \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_18_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18);                                                                                                                     \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18) {                                                       \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_18_##xMethod> stub(new Stub_18_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18));                                                           \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_19(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19)                                                                                         \
    class Stub_19_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19;                                                  \
    public:                                                                                                                                                                                                     \
      Stub_19_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19) : mDelegate(delegate) {     \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_19_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19);                                                                                                                 \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19) {                                               \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_19_##xMethod> stub(new Stub_19_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19));                                                       \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_20(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20)                                                                                     \
    class Stub_20_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19; t20 m20;                                         \
    public:                                                                                                                                                                                                     \
      Stub_20_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
        internal::ProxyPack<t20>(m20, v20);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_20_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
        internal::ProxyClean<t20>(m20);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20);                                                                                                             \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20) {                                       \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_20_##xMethod> stub(new Stub_20_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20));                                                   \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_21(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21)                                                                                 \
    class Stub_21_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19; t20 m20; t21 m21;                                \
    public:                                                                                                                                                                                                     \
      Stub_21_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
        internal::ProxyPack<t20>(m20, v20);                                                                                                                                                                     \
        internal::ProxyPack<t21>(m21, v21);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_21_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
        internal::ProxyClean<t20>(m20);                                                                                                                                                                         \
        internal::ProxyClean<t21>(m21);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21);                                                                                                         \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21) {                               \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_21_##xMethod> stub(new Stub_21_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21));                                               \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_22(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22)                                                                             \
    class Stub_22_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19; t20 m20; t21 m21; t22 m22;                       \
    public:                                                                                                                                                                                                     \
      Stub_22_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
        internal::ProxyPack<t20>(m20, v20);                                                                                                                                                                     \
        internal::ProxyPack<t21>(m21, v21);                                                                                                                                                                     \
        internal::ProxyPack<t22>(m22, v22);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_22_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
        internal::ProxyClean<t20>(m20);                                                                                                                                                                         \
        internal::ProxyClean<t21>(m21);                                                                                                                                                                         \
        internal::ProxyClean<t22>(m22);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22);                                                                                                     \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22) {                       \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_22_##xMethod> stub(new Stub_22_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22));                                           \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_23(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23)                                                                         \
    class Stub_23_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19; t20 m20; t21 m21; t22 m22; t23 m23;              \
    public:                                                                                                                                                                                                     \
      Stub_23_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
        internal::ProxyPack<t20>(m20, v20);                                                                                                                                                                     \
        internal::ProxyPack<t21>(m21, v21);                                                                                                                                                                     \
        internal::ProxyPack<t22>(m22, v22);                                                                                                                                                                     \
        internal::ProxyPack<t23>(m23, v23);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_23_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
        internal::ProxyClean<t20>(m20);                                                                                                                                                                         \
        internal::ProxyClean<t21>(m21);                                                                                                                                                                         \
        internal::ProxyClean<t22>(m22);                                                                                                                                                                         \
        internal::ProxyClean<t23>(m23);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23);                                                                                                 \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23) {               \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_23_##xMethod> stub(new Stub_23_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23));                                       \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_24(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24)                                                                     \
    class Stub_24_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19; t20 m20; t21 m21; t22 m22; t23 m23; t24 m24;     \
    public:                                                                                                                                                                                                     \
      Stub_24_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
        internal::ProxyPack<t20>(m20, v20);                                                                                                                                                                     \
        internal::ProxyPack<t21>(m21, v21);                                                                                                                                                                     \
        internal::ProxyPack<t22>(m22, v22);                                                                                                                                                                     \
        internal::ProxyPack<t23>(m23, v23);                                                                                                                                                                     \
        internal::ProxyPack<t24>(m24, v24);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_24_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
        internal::ProxyClean<t20>(m20);                                                                                                                                                                         \
        internal::ProxyClean<t21>(m21);                                                                                                                                                                         \
        internal::ProxyClean<t22>(m22);                                                                                                                                                                         \
        internal::ProxyClean<t23>(m23);                                                                                                                                                                         \
        internal::ProxyClean<t24>(m24);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24);                                                                                             \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24) {       \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_24_##xMethod> stub(new Stub_24_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24));                                   \
      mQueue->post(stub);                                                                                                                                                                                       \
    }

#define ZS_INTERNAL_DECLARE_PROXY_METHOD_25(xMethod,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24,t25)                                                                 \
    class Stub_25_##xMethod : public IMessageQueueMessage                                                                                                                                                       \
    {                                                                                                                                                                                                           \
    private:                                                                                                                                                                                                    \
      DelegatePtr mDelegate;                                                                                                                                                                                    \
      t1 m1; t2 m2; t3 m3; t4 m4; t5 m5; t6 m6; t7 m7; t8 m8; t9 m9; t10 m10; t11 m11; t12 m12; t13 m13; t14 m14; t15 m15; t16 m16; t17 m17; t18 m18; t19 m19; t20 m20; t21 m21; t22 m22; t23 m23; t24 m24; t25 m25;  \
    public:                                                                                                                                                                                                           \
      Stub_25_##xMethod(DelegatePtr delegate,t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24,t25 v25) : mDelegate(delegate) { \
        internal::ProxyPack<t1>(m1, v1);                                                                                                                                                                        \
        internal::ProxyPack<t2>(m2, v2);                                                                                                                                                                        \
        internal::ProxyPack<t3>(m3, v3);                                                                                                                                                                        \
        internal::ProxyPack<t4>(m4, v4);                                                                                                                                                                        \
        internal::ProxyPack<t5>(m5, v5);                                                                                                                                                                        \
        internal::ProxyPack<t6>(m6, v6);                                                                                                                                                                        \
        internal::ProxyPack<t7>(m7, v7);                                                                                                                                                                        \
        internal::ProxyPack<t8>(m8, v8);                                                                                                                                                                        \
        internal::ProxyPack<t9>(m9, v9);                                                                                                                                                                        \
        internal::ProxyPack<t10>(m10, v10);                                                                                                                                                                     \
        internal::ProxyPack<t11>(m11, v11);                                                                                                                                                                     \
        internal::ProxyPack<t12>(m12, v12);                                                                                                                                                                     \
        internal::ProxyPack<t13>(m13, v13);                                                                                                                                                                     \
        internal::ProxyPack<t14>(m14, v14);                                                                                                                                                                     \
        internal::ProxyPack<t15>(m15, v15);                                                                                                                                                                     \
        internal::ProxyPack<t16>(m16, v16);                                                                                                                                                                     \
        internal::ProxyPack<t17>(m17, v17);                                                                                                                                                                     \
        internal::ProxyPack<t18>(m18, v18);                                                                                                                                                                     \
        internal::ProxyPack<t19>(m19, v19);                                                                                                                                                                     \
        internal::ProxyPack<t20>(m20, v20);                                                                                                                                                                     \
        internal::ProxyPack<t21>(m21, v21);                                                                                                                                                                     \
        internal::ProxyPack<t22>(m22, v22);                                                                                                                                                                     \
        internal::ProxyPack<t23>(m23, v23);                                                                                                                                                                     \
        internal::ProxyPack<t24>(m24, v24);                                                                                                                                                                     \
        internal::ProxyPack<t25>(m25, v25);                                                                                                                                                                     \
      }                                                                                                                                                                                                         \
      ~Stub_25_##xMethod() {                                                                                                                                                                                    \
        internal::ProxyClean<t1>(m1);                                                                                                                                                                           \
        internal::ProxyClean<t2>(m2);                                                                                                                                                                           \
        internal::ProxyClean<t3>(m3);                                                                                                                                                                           \
        internal::ProxyClean<t4>(m4);                                                                                                                                                                           \
        internal::ProxyClean<t5>(m5);                                                                                                                                                                           \
        internal::ProxyClean<t6>(m6);                                                                                                                                                                           \
        internal::ProxyClean<t7>(m7);                                                                                                                                                                           \
        internal::ProxyClean<t8>(m8);                                                                                                                                                                           \
        internal::ProxyClean<t9>(m9);                                                                                                                                                                           \
        internal::ProxyClean<t10>(m10);                                                                                                                                                                         \
        internal::ProxyClean<t11>(m11);                                                                                                                                                                         \
        internal::ProxyClean<t12>(m12);                                                                                                                                                                         \
        internal::ProxyClean<t13>(m13);                                                                                                                                                                         \
        internal::ProxyClean<t14>(m14);                                                                                                                                                                         \
        internal::ProxyClean<t15>(m15);                                                                                                                                                                         \
        internal::ProxyClean<t16>(m16);                                                                                                                                                                         \
        internal::ProxyClean<t17>(m17);                                                                                                                                                                         \
        internal::ProxyClean<t18>(m18);                                                                                                                                                                         \
        internal::ProxyClean<t19>(m19);                                                                                                                                                                         \
        internal::ProxyClean<t20>(m20);                                                                                                                                                                         \
        internal::ProxyClean<t21>(m21);                                                                                                                                                                         \
        internal::ProxyClean<t22>(m22);                                                                                                                                                                         \
        internal::ProxyClean<t23>(m23);                                                                                                                                                                         \
        internal::ProxyClean<t24>(m24);                                                                                                                                                                         \
        internal::ProxyClean<t25>(m25);                                                                                                                                                                         \
      }                                                                                                                                                                                                         \
                                                                                                                                                                                                                \
      virtual const char *getDelegateName() const {return typeid(Delegate).name();}                                                                                                                             \
      virtual const char *getMethodName() const {return #xMethod;}                                                                                                                                              \
      virtual void processMessage() {                                                                                                                                                                           \
        mDelegate->xMethod(m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15,m16,m17,m18,m19,m20,m21,m22,m23,m24,m25);                                                                                         \
      }                                                                                                                                                                                                         \
    };                                                                                                                                                                                                          \
                                                                                                                                                                                                                \
    virtual void xMethod(t1 v1,t2 v2,t3 v3,t4 v4,t5 v5,t6 v6,t7 v7,t8 v8,t9 v9,t10 v10,t11 v11,t12 v12,t13 v13,t14 v14,t15 v15,t16 v16,t17 v17,t18 v18,t19 v19,t20 v20,t21 v21,t22 v22,t23 v23,t24 v24,t25 v25) { \
      if (ignoreMethodCall()) return;                                                                                                                                                                           \
      boost::shared_ptr<Stub_25_##xMethod> stub(new Stub_25_##xMethod(getDelegate(),v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25));                               \
      mQueue->post(stub);                                                                                                                                                                                       \
    }


#endif //ZSLIB_INTERNAL_PROXY_H_a1792950ebd2df4b616a6b341965c42d
