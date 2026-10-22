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

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <cstdint>
#अन्तर्भाव <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
template <typename SubClass, typename SuperClass>
bool isInstanceOf(SuperClass* objp) {
    static_assert(std::is_base_of<SuperClass, SubClass>::मूल्य, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::मूल्य, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::मूल्य,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    प्रत्यागम objp->v3RttiIsInstanceOfImpl(SubClass::v3RttiClassId());
}

namespace internal {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
template <typename T>
अन्तःस्थ bool matchClassId(uintptr_t id) VL_PURE {
    प्रत्यागम id == T::v3RttiClassId() || matchClassId<typename T::V3RttiBaseClass>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
template <>
अन्तःस्थ bool matchClassId<शून्य>(uintptr_t) VL_PURE {
    प्रत्यागम false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#define V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    संरचना RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::मूल्य, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    template <typename T> \
    friend bool V3Rtti::internal::matchClassId(uintptr_t id); \
    template <typename T, typename U> \
    friend bool V3Rtti::isInstanceOf(U*); \
    /* Returns unique type ID of the class. */ \
    स्थिर uintptr_t v3RttiClassId() VL_PURE { \
        /* Don't complain this function is unused */ (शून्य)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        स्थिर वर्ण s_vlrttiClassId; \
        प्रत्यागम reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    आभास bool v3RttiIsInstanceOfImpl(uintptr_t id) ध्रुव OVERRIDE VL_PURE { \
        प्रत्यागम ::V3Rtti::internal::matchClassId<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#define VL_RTTI_IMPL(ThisClass, DirectBaseClass) \
गुप्त: \
    using V3RttiBaseClass = DirectBaseClass; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
गुप्त:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#define VL_RTTI_IMPL_BASE(ThisClass) \
गुप्त: \
    using V3RttiBaseClass = शून्य; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /* base */) \
गुप्त:

#endif  // Guard
