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

#यदि_अपरिभाषितम् VERILATOR_V3RTTI_H_
#परिभाषय VERILATOR_V3RTTI_H_

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <cstdint>
#अन्तर्भाव <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
प्रतिरूपम् <typename SubClass, typename SuperClass>
bool is_Instance_सम्बन्धि(SuperClass* objp) {
    static_assert(std::is_आधारः_सम्बन्धि<SuperClass, SubClass>::मानम्, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::मानम्, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::मानम्,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    प्रत्यागम objp->v3_Rtti_Is_Instance_सम्बन्धि_Impl(SubClass::v3RttiClassId());
}

namespace आन्तरिकम्_2 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
प्रतिरूपम् <typename T>
अन्तःस्थ bool मेलनम्_Class_Id(uintptr_t id) VL_विशुद्धम् {
    प्रत्यागम id == T::v3RttiClassId() || मेलनम्_Class_Id<typename T::V3_Rtti_आधारः_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
प्रतिरूपम् <>
अन्तःस्थ bool मेलनम्_Class_Id<शून्य>(uintptr_t) VL_विशुद्धम् {
    प्रत्यागम false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#परिभाषय V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    संरचना RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::मानम्, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    प्रतिरूपम् <typename T> \
    मित्रम् bool V3Rtti::आन्तरिकम्_2::मेलनम्_Class_Id(uintptr_t id); \
    प्रतिरूपम् <typename T, typename U> \
    मित्रम् bool V3Rtti::is_Instance_सम्बन्धि(U*); \
    /* Returns unique type ID of the class. */ \
    स्थिर uintptr_t v3RttiClassId() VL_विशुद्धम् { \
        /* Don't complain this function is unused */ (शून्य)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        स्थिर वर्ण s_vlrttiClassId; \
        प्रत्यागम reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    आभास bool v3_Rtti_Is_Instance_सम्बन्धि_Impl(uintptr_t id) ध्रुव OVERRIDE VL_विशुद्धम् { \
        प्रत्यागम ::V3Rtti::आन्तरिकम्_2::मेलनम्_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#परिभाषय VL_RTTI_IMPL(ThisClass, प्रत्यक्षम्_आधारः_Class) \
गुप्त: \
    using V3_Rtti_आधारः_Class = प्रत्यक्षम्_आधारः_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
गुप्त:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#परिभाषय VL_RTTI_IMPL_आधारः(ThisClass) \
गुप्त: \
    using V3_Rtti_आधारः_Class = शून्य; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*आधारः*/) \
गुप्त:

#यदि_अन्तः  // Guard
