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

#اگر_غیر_تعریف_شدہ VERILATOR_V3RTTI_H_
#تعریف_کرو VERILATOR_V3RTTI_H_

#شامل "verilatedos.h"

#شامل <cstdint>
#شامل <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
سانچہ <typename SubClass, typename SuperClass>
bool is_Instance_کا(SuperClass* objp) {
    static_assert(std::is_بنیاد_کا<SuperClass, SubClass>::قدر, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::قدر, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::قدر,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    واپس objp->v3_Rtti_Is_Instance_کا_Impl(SubClass::v3RttiClassId());
}

namespace اندرونی_3 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
سانچہ <typename T>
اندرونی bool مطابقت_Class_Id(uintptr_t id) VL_خالص {
    واپس id == T::v3RttiClassId() || مطابقت_Class_Id<typename T::V3_Rtti_بنیاد_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
سانچہ <>
اندرونی bool مطابقت_Class_Id<خالی>(uintptr_t) VL_خالص {
    واپس false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#تعریف_کرو V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    ساخت RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::قدر, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    سانچہ <typename T> \
    دوست bool V3Rtti::اندرونی_3::مطابقت_Class_Id(uintptr_t id); \
    سانچہ <typename T, typename U> \
    دوست bool V3Rtti::is_Instance_کا(U*); \
    /* Returns unique type ID of the class. */ \
    ساکن uintptr_t v3RttiClassId() VL_خالص { \
        /* Don't complain this function is unused */ (خالی)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        ساکن حرف s_vlrttiClassId; \
        واپس reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    مجازی bool v3_Rtti_Is_Instance_کا_Impl(uintptr_t id) ثابت OVERRIDE VL_خالص { \
        واپس ::V3Rtti::اندرونی_3::مطابقت_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#تعریف_کرو VL_RTTI_IMPL(ThisClass, براہ_راست_بنیاد_Class) \
نجی: \
    using V3_Rtti_بنیاد_Class = براہ_راست_بنیاد_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
نجی:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#تعریف_کرو VL_RTTI_IMPL_بنیاد(ThisClass) \
نجی: \
    using V3_Rtti_بنیاد_Class = خالی; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*بنیاد*/) \
نجی:

#اگر_ختم  // Guard
