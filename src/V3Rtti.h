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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3RTTI_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3RTTI_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstdint>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
ਸਾਂਚਾ <typename SubClass, typename SuperClass>
bool is_Instance_ਦਾ(SuperClass* objp) {
    static_assert(std::is_ਅਧਾਰ_ਦਾ<SuperClass, SubClass>::ਮੁੱਲ, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::ਮੁੱਲ, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::ਮੁੱਲ,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    ਵਾਪਸ objp->v3_Rtti_Is_Instance_ਦਾ_Impl(SubClass::v3RttiClassId());
}

namespace ਅੰਦਰੂਨੀ_2 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
ਸਾਂਚਾ <typename T>
ਸਤਰਅੰਦਰ bool ਮੇਲ_Class_Id(uintptr_t id) VL_ਸ਼ੁੱਧ {
    ਵਾਪਸ id == T::v3RttiClassId() || ਮੇਲ_Class_Id<typename T::V3_Rtti_ਅਧਾਰ_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool ਮੇਲ_Class_Id<ਖਾਲੀ>(uintptr_t) VL_ਸ਼ੁੱਧ {
    ਵਾਪਸ false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    ਬਣਤਰ RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::ਮੁੱਲ, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    ਸਾਂਚਾ <typename T> \
    ਮਿੱਤਰ bool V3Rtti::ਅੰਦਰੂਨੀ_2::ਮੇਲ_Class_Id(uintptr_t id); \
    ਸਾਂਚਾ <typename T, typename U> \
    ਮਿੱਤਰ bool V3Rtti::is_Instance_ਦਾ(U*); \
    /* Returns unique type ID of the class. */ \
    ਸਥਿਰ uintptr_t v3RttiClassId() VL_ਸ਼ੁੱਧ { \
        /* Don't complain this function is unused */ (ਖਾਲੀ)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        ਸਥਿਰ ਅੱਖਰ s_vlrttiClassId; \
        ਵਾਪਸ reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    ਆਭਾਸੀ bool v3_Rtti_Is_Instance_ਦਾ_Impl(uintptr_t id) ਅਚਲ OVERRIDE VL_ਸ਼ੁੱਧ { \
        ਵਾਪਸ ::V3Rtti::ਅੰਦਰੂਨੀ_2::ਮੇਲ_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_RTTI_IMPL(ThisClass, ਸਿੱਧਾ_ਅਧਾਰ_Class) \
ਨਿਜੀ: \
    using V3_Rtti_ਅਧਾਰ_Class = ਸਿੱਧਾ_ਅਧਾਰ_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
ਨਿਜੀ:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_RTTI_IMPL_ਅਧਾਰ(ThisClass) \
ਨਿਜੀ: \
    using V3_Rtti_ਅਧਾਰ_Class = ਖਾਲੀ; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*ਅਧਾਰ*/) \
ਨਿਜੀ:

#ਜੇ_ਅੰਤ  // Guard
