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

#ضمن "verilatedos.h"

#ضمن <cstdint>
#ضمن <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
template <typename SubClass, typename SuperClass>
bool isInstanceOf(SuperClass* objp) {
    static_assert(std::is_base_of<SuperClass, SubClass>::قىممەت, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::قىممەت, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::قىممەت,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    أرجع objp->v3RttiIsInstanceOfImpl(SubClass::v3RttiClassId());
}

namespace internal {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
template <typename T>
مضمن bool matchClassId(uintptr_t id) VL_PURE {
    أرجع id == T::v3RttiClassId() || matchClassId<typename T::V3RttiBaseClass>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
template <>
مضمن bool matchClassId<فراغ>(uintptr_t) VL_PURE {
    أرجع false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#define V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    بنية RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::قىممەت, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    template <typename T> \
    friend bool V3Rtti::internal::matchClassId(uintptr_t id); \
    template <typename T, typename U> \
    friend bool V3Rtti::isInstanceOf(U*); \
    /* Returns unique type ID of the class. */ \
    ساكن uintptr_t v3RttiClassId() VL_PURE { \
        /* Don't complain this function is unused */ (فراغ)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        ساكن حرف s_vlrttiClassId; \
        أرجع reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    virtual bool v3RttiIsInstanceOfImpl(uintptr_t id) ثابت OVERRIDE VL_PURE { \
        أرجع ::V3Rtti::internal::matchClassId<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#define VL_RTTI_IMPL(ThisClass, DirectBaseClass) \
خاص: \
    using V3RttiBaseClass = DirectBaseClass; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
خاص:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#define VL_RTTI_IMPL_BASE(ThisClass) \
خاص: \
    using V3RttiBaseClass = فراغ; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /* base */) \
خاص:

#endif  // Guard
