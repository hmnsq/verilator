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

#که_نه_تعریف_شوی VERILATOR_V3RTTI_H_
#تعریف_کړه VERILATOR_V3RTTI_H_

#ضمن "verilatedos.h"

#ضمن <cstdint>
#ضمن <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
کينډۍ <typename SubClass, typename SuperClass>
bool is_Instance_د(SuperClass* objp) {
    static_assert(std::is_بنسټ_د<SuperClass, SubClass>::ارزښت, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::ارزښت, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::ارزښت,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    أرجع objp->v3_Rtti_Is_Instance_د_Impl(SubClass::v3RttiClassId());
}

namespace دننی_2 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
کينډۍ <typename T>
مضمن bool سمون_Class_Id(uintptr_t id) VL_خالص {
    أرجع id == T::v3RttiClassId() || سمون_Class_Id<typename T::V3_Rtti_بنسټ_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
کينډۍ <>
مضمن bool سمون_Class_Id<فراغ>(uintptr_t) VL_خالص {
    أرجع false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#تعریف_کړه V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    بنية RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::ارزښت, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    کينډۍ <typename T> \
    ملګری bool V3Rtti::دننی_2::سمون_Class_Id(uintptr_t id); \
    کينډۍ <typename T, typename U> \
    ملګری bool V3Rtti::is_Instance_د(U*); \
    /* Returns unique type ID of the class. */ \
    ساكن uintptr_t v3RttiClassId() VL_خالص { \
        /* Don't complain this function is unused */ (فراغ)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        ساكن حرف s_vlrttiClassId; \
        أرجع reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    مجازي bool v3_Rtti_Is_Instance_د_Impl(uintptr_t id) ثابت OVERRIDE VL_خالص { \
        أرجع ::V3Rtti::دننی_2::سمون_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#تعریف_کړه VL_RTTI_IMPL(ThisClass, مستقیم_بنسټ_Class) \
خاص: \
    using V3_Rtti_بنسټ_Class = مستقیم_بنسټ_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
خاص:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#تعریف_کړه VL_RTTI_IMPL_بنسټ(ThisClass) \
خاص: \
    using V3_Rtti_بنسټ_Class = فراغ; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*بنسټ*/) \
خاص:

#د_که_پای  // Guard
