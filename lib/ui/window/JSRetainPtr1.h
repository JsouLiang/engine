/*
 * Copyright (C) 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSRetainPtr1_h
#define JSRetainPtr1_h

#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSStringRef.h>
#include <algorithm>

inline void JSRetain(JSStringRef string) { JSStringRetain(string); }
inline void JSRelease(JSStringRef string) { JSStringRelease(string); }
inline void JSRetain(JSGlobalContextRef context) { JSGlobalContextRetain(context); }
inline void JSRelease(JSGlobalContextRef context) { JSGlobalContextRelease(context); }

enum AdoptTag { Adopt };

template<typename T> class JSRetainPtr1 {
public:
    JSRetainPtr1() : m_ptr(0) { }
    JSRetainPtr1(T ptr) : m_ptr(ptr) { if (ptr) JSRetain(ptr); }
    JSRetainPtr1(AdoptTag, T ptr) : m_ptr(ptr) { }
    JSRetainPtr1(const JSRetainPtr1&);
    template<typename U> JSRetainPtr1(const JSRetainPtr1<U>&);
    ~JSRetainPtr1();
    
    T get() const { return m_ptr; }
    
    void clear();
    T leakRef();

    T operator->() const { return m_ptr; }
    
    bool operator!() const { return !m_ptr; }

    // This conversion operator allows implicit conversion to bool but not to other integer types.
    typedef T JSRetainPtr1::*UnspecifiedBoolType;
    operator UnspecifiedBoolType() const { return m_ptr ? &JSRetainPtr1::m_ptr : 0; }
    
    JSRetainPtr1& operator=(const JSRetainPtr1&);
    template<typename U> JSRetainPtr1& operator=(const JSRetainPtr1<U>&);
    JSRetainPtr1& operator=(T);
    template<typename U> JSRetainPtr1& operator=(U*);

    void adopt(T);
    
    void swap(JSRetainPtr1&);

private:
    T m_ptr;
};

template<typename T> inline JSRetainPtr1<T>::JSRetainPtr1(const JSRetainPtr1& o)
    : m_ptr(o.m_ptr)
{
    if (m_ptr)
        JSRetain(m_ptr);
}

template<typename T> template<typename U> inline JSRetainPtr1<T>::JSRetainPtr1(const JSRetainPtr1<U>& o)
    : m_ptr(o.get())
{
    if (m_ptr)
        JSRetain(m_ptr);
}

template<typename T> inline JSRetainPtr1<T>::~JSRetainPtr1()
{
    if (m_ptr)
        JSRelease(m_ptr);
}

template<typename T> inline void JSRetainPtr1<T>::clear()
{
    if (T ptr = m_ptr) {
        m_ptr = 0;
        JSRelease(ptr);
    }
}

template<typename T> inline T JSRetainPtr1<T>::leakRef()
{
    T ptr = m_ptr;
    m_ptr = 0;
    return ptr;
}

template<typename T> inline JSRetainPtr1<T>& JSRetainPtr1<T>::operator=(const JSRetainPtr1<T>& o)
{
    T optr = o.get();
    if (optr)
        JSRetain(optr);
    T ptr = m_ptr;
    m_ptr = optr;
    if (ptr)
        JSRelease(ptr);
    return *this;
}

template<typename T> template<typename U> inline JSRetainPtr1<T>& JSRetainPtr1<T>::operator=(const JSRetainPtr1<U>& o)
{
    T optr = o.get();
    if (optr)
        JSRetain(optr);
    T ptr = m_ptr;
    m_ptr = optr;
    if (ptr)
        JSRelease(ptr);
    return *this;
}

template<typename T> inline JSRetainPtr1<T>& JSRetainPtr1<T>::operator=(T optr)
{
    if (optr)
        JSRetain(optr);
    T ptr = m_ptr;
    m_ptr = optr;
    if (ptr)
        JSRelease(ptr);
    return *this;
}

template<typename T> inline void JSRetainPtr1<T>::adopt(T optr)
{
    T ptr = m_ptr;
    m_ptr = optr;
    if (ptr)
        JSRelease(ptr);
}

template<typename T> template<typename U> inline JSRetainPtr1<T>& JSRetainPtr1<T>::operator=(U* optr)
{
    if (optr)
        JSRetain(optr);
    T ptr = m_ptr;
    m_ptr = optr;
    if (ptr)
        JSRelease(ptr);
    return *this;
}

template<typename T> inline void JSRetainPtr1<T>::swap(JSRetainPtr1<T>& o)
{
    std::swap(m_ptr, o.m_ptr);
}

template<typename T> inline void swap(JSRetainPtr1<T>& a, JSRetainPtr1<T>& b)
{
    a.swap(b);
}

template<typename T, typename U> inline bool operator==(const JSRetainPtr1<T>& a, const JSRetainPtr1<U>& b)
{ 
    return a.get() == b.get(); 
}

template<typename T, typename U> inline bool operator==(const JSRetainPtr1<T>& a, U* b)
{ 
    return a.get() == b; 
}

template<typename T, typename U> inline bool operator==(T* a, const JSRetainPtr1<U>& b) 
{
    return a == b.get(); 
}

template<typename T, typename U> inline bool operator!=(const JSRetainPtr1<T>& a, const JSRetainPtr1<U>& b)
{ 
    return a.get() != b.get(); 
}

template<typename T, typename U> inline bool operator!=(const JSRetainPtr1<T>& a, U* b)
{
    return a.get() != b; 
}

template<typename T, typename U> inline bool operator!=(T* a, const JSRetainPtr1<U>& b)
{ 
    return a != b.get(); 
}


#endif // JSRetainPtr_h
