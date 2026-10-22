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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3RTTI_H_
#وصف_ڪريو VERILATOR_V3RTTI_H_

#ضمن "verilatedos.h"

#ضمن <cstdint>
#ضمن <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
سانچو <typename SubClass, typename SuperClass>
bool is_Instance_جو(SuperClass* objp) {
    static_assert(std::is_بنياد_جو<SuperClass, SubClass>::قدر, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::قدر, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::قدر,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    أرجع objp->v3_Rtti_Is_Instance_جو_Impl(SubClass::v3RttiClassId());
}

namespace اندروني_2 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
سانچو <typename T>
مضمن bool ميلاپ_Class_Id(uintptr_t id) VL_خالص {
    أرجع id == T::v3RttiClassId() || ميلاپ_Class_Id<typename T::V3_Rtti_بنياد_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
سانچو <>
مضمن bool ميلاپ_Class_Id<فراغ>(uintptr_t) VL_خالص {
    أرجع false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#وصف_ڪريو V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    بنية RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::قدر, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    سانچو <typename T> \
    دوست bool V3Rtti::اندروني_2::ميلاپ_Class_Id(uintptr_t id); \
    سانچو <typename T, typename U> \
    دوست bool V3Rtti::is_Instance_جو(U*); \
    /* Returns unique type ID of the class. */ \
    ساكن uintptr_t v3RttiClassId() VL_خالص { \
        /* Don't complain this function is unused */ (فراغ)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        ساكن حرف s_vlrttiClassId; \
        أرجع reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    مجازي bool v3_Rtti_Is_Instance_جو_Impl(uintptr_t id) ثابت OVERRIDE VL_خالص { \
        أرجع ::V3Rtti::اندروني_2::ميلاپ_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#وصف_ڪريو VL_RTTI_IMPL(ThisClass, سڌو_بنياد_Class) \
خاص: \
    using V3_Rtti_بنياد_Class = سڌو_بنياد_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
خاص:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#وصف_ڪريو VL_RTTI_IMPL_بنياد(ThisClass) \
خاص: \
    using V3_Rtti_بنياد_Class = فراغ; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*بنياد*/) \
خاص:

#جيڪڏهن_پورو  // Guard
