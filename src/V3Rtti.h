// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Simple and efficient Run-Time Type Information
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3RTTI_H_
#define VERILATOR_V3RTTI_H_

#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <cstdint>
#অন্তর্ভাব <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
template <typename SubClass, typename SuperClass>
bool isInstanceOf(SuperClass* objp) {
    static_assert(std::is_base_of<SuperClass, SubClass>::মান, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::মান, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::মান,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    ফেরত objp->v3RttiIsInstanceOfImpl(SubClass::v3RttiClassId());
}

namespace internal {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
template <typename T>
অন্তঃরেখ bool matchClassId(uintptr_t id) VL_PURE {
    ফেরত id == T::v3RttiClassId() || matchClassId<typename T::V3RttiBaseClass>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
template <>
অন্তঃরেখ bool matchClassId<শূন্য>(uintptr_t) VL_PURE {
    ফেরত false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#define V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    কাঠামো RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::মান, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    template <typename T> \
    friend bool V3Rtti::internal::matchClassId(uintptr_t id); \
    template <typename T, typename U> \
    friend bool V3Rtti::isInstanceOf(U*); \
    /* Returns unique type ID of the class. */ \
    স্থির uintptr_t v3RttiClassId() VL_PURE { \
        /* Don't complain this function is unused */ (শূন্য)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        স্থির অক্ষর s_vlrttiClassId; \
        ফেরত reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    আভাসী bool v3RttiIsInstanceOfImpl(uintptr_t id) ধ্রুবক OVERRIDE VL_PURE { \
        ফেরত ::V3Rtti::internal::matchClassId<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#define VL_RTTI_IMPL(ThisClass, DirectBaseClass) \
ব্যক্তিগত: \
    using V3RttiBaseClass = DirectBaseClass; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
ব্যক্তিগত:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#define VL_RTTI_IMPL_BASE(ThisClass) \
ব্যক্তিগত: \
    using V3RttiBaseClass = শূন্য; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /* base */) \
ব্যক্তিগত:

#endif  // Guard
