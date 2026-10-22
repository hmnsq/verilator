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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3RTTI_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3RTTI_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
ಮಾದರಿ <typename SubClass, typename SuperClass>
bool is_Instance_ದ(SuperClass* objp) {
    static_assert(std::is_ತಳ_ದ<SuperClass, SubClass>::ಮೌಲ್ಯ, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::ಮೌಲ್ಯ, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::ಮೌಲ್ಯ,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    ಹಿಂತಿರುಗಿ objp->v3_Rtti_Is_Instance_ದ_Impl(SubClass::v3RttiClassId());
}

namespace ಆಂತರಿಕ_2 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
ಮಾದರಿ <typename T>
ಸಾಲೊಳಗೆ bool ಹೊಂದಿಕೆ_Class_Id(uintptr_t id) VL_ಶುದ್ಧ {
    ಹಿಂತಿರುಗಿ id == T::v3RttiClassId() || ಹೊಂದಿಕೆ_Class_Id<typename T::V3_Rtti_ತಳ_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool ಹೊಂದಿಕೆ_Class_Id<ಶೂನ್ಯ>(uintptr_t) VL_ಶುದ್ಧ {
    ಹಿಂತಿರುಗಿ false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#ವ್ಯಾಖ್ಯಾನಿಸು V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    ರಚನೆ RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::ಮೌಲ್ಯ, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    ಮಾದರಿ <typename T> \
    ಮಿತ್ರ bool V3Rtti::ಆಂತರಿಕ_2::ಹೊಂದಿಕೆ_Class_Id(uintptr_t id); \
    ಮಾದರಿ <typename T, typename U> \
    ಮಿತ್ರ bool V3Rtti::is_Instance_ದ(U*); \
    /* Returns unique type ID of the class. */ \
    ಸ್ಥಿರ uintptr_t v3RttiClassId() VL_ಶುದ್ಧ { \
        /* Don't complain this function is unused */ (ಶೂನ್ಯ)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        ಸ್ಥಿರ ಅಕ್ಷರ s_vlrttiClassId; \
        ಹಿಂತಿರುಗಿ reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    ವಾಸ್ತವಿಕ bool v3_Rtti_Is_Instance_ದ_Impl(uintptr_t id) ಸ್ಥಿರಾಂಕ OVERRIDE VL_ಶುದ್ಧ { \
        ಹಿಂತಿರುಗಿ ::V3Rtti::ಆಂತರಿಕ_2::ಹೊಂದಿಕೆ_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#ವ್ಯಾಖ್ಯಾನಿಸು VL_RTTI_IMPL(ThisClass, ನೇರ_ತಳ_Class) \
ಖಾಸಗಿ: \
    using V3_Rtti_ತಳ_Class = ನೇರ_ತಳ_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
ಖಾಸಗಿ:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#ವ್ಯಾಖ್ಯಾನಿಸು VL_RTTI_IMPL_ತಳ(ThisClass) \
ಖಾಸಗಿ: \
    using V3_Rtti_ತಳ_Class = ಶೂನ್ಯ; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*ತಳ*/) \
ಖಾಸಗಿ:

#ಆದರೆ_ಅಂತ್ಯ  // Guard
