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

#నిర్వచితం_కాకపోతే VERILATOR_V3RTTI_H_
#నిర్వచించు VERILATOR_V3RTTI_H_

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <cstdint>
#అన్తర్భావ <type_traits>

namespace V3Rtti {

// Use 'V3Rtti::isInstanceOf<T>(objp)' to check if 'objp' is of subtype 'T' (downcasting)
మూస <typename SubClass, typename SuperClass>
bool is_Instance_యొక్క(SuperClass* objp) {
    static_assert(std::is_పునాది_యొక్క<SuperClass, SubClass>::విలువ, "Impossible 'isInstanceOf'");
    static_assert(!std::is_same<SuperClass, SubClass>::విలువ, "Useless 'isInstanceOf'");
    static_assert(std::is_same<  //
                      typename std::remove_cv<SubClass>::type,  //
                      typename SubClass::V3RttiThisClass  //
                      >::విలువ,
                  "Missing VL_RTTI_IMPL(...) in 'SubClass'");
    తిరిగి objp->v3_Rtti_Is_Instance_యొక్క_Impl(SubClass::v3RttiClassId());
}

namespace అంతర్గత_2 {

// Returns true iff 'id' equals the 'rttiClassId()' of either 'T', or any of its base classes
మూస <typename T>
అంతఃవరుస bool సరిపోలిక_Class_Id(uintptr_t id) VL_శుద్ధ {
    తిరిగి id == T::v3RttiClassId() || సరిపోలిక_Class_Id<typename T::V3_Rtti_పునాది_Class>(id);
}

// Specialization for the root class, which have 'void' for 'V3RttiBaseClass'
మూస <>
అంతఃవరుస bool సరిపోలిక_Class_Id<శూన్య>(uintptr_t) VL_శుద్ధ {
    తిరిగి false;
}

}  // namespace internal

}  // namespace V3Rtti

// Common code used by VL_RTTI_COMMON_IMPL and VL_RTTI_COMMON_IMPL_BASE.
#నిర్వచించు V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, OVERRIDE) \
    using V3RttiThisClass = ThisClass; \
    /* A type used only for implementation of the static_assert below. */ \
    నిర్మాణం RttiUniqueTypeForThisClass {}; \
    static_assert( \
        std::is_same<RttiUniqueTypeForThisClass, ThisClass::RttiUniqueTypeForThisClass>::విలువ, \
        "'ThisClass' argument (" #ThisClass ") does not match the class name"); \
    /* The implementation can pick up the private members */ \
    మూస <typename T> \
    మిత్ర bool V3Rtti::అంతర్గత_2::సరిపోలిక_Class_Id(uintptr_t id); \
    మూస <typename T, typename U> \
    మిత్ర bool V3Rtti::is_Instance_యొక్క(U*); \
    /* Returns unique type ID of the class. */ \
    స్థిర uintptr_t v3RttiClassId() VL_శుద్ధ { \
        /* Don't complain this function is unused */ (శూన్య)&v3RttiClassId; \
        /* The address of this static variable is used as the unique class type ID. */ \
        స్థిర అక్షరం s_vlrttiClassId; \
        తిరిగి reinterpret_cast<uintptr_t>(&s_vlrttiClassId); \
    } \
    /* Dispatch to matchClassId with the correct sub type (this type) */ \
    మాయా bool v3_Rtti_Is_Instance_యొక్క_Impl(uintptr_t id) స్థిరరాశి OVERRIDE VL_శుద్ధ { \
        తిరిగి ::V3Rtti::అంతర్గత_2::సరిపోలిక_Class_Id<ThisClass>(id); \
    }

// Call this macro at the beginning of class definition if the class derives
// from a class with VL_RTTI_IMPL or VL_RTTI_IMPL_BASE calls.
#నిర్వచించు VL_RTTI_IMPL(ThisClass, ప్రత్యక్ష_పునాది_Class) \
అంతరంగ: \
    using V3_Rtti_పునాది_Class = ప్రత్యక్ష_పునాది_Class; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, override) \
అంతరంగ:

// Call this macro at the beginning of a base class to implement class type
// usable with V3Rtti::InstanceOf
#నిర్వచించు VL_RTTI_IMPL_పునాది(ThisClass) \
అంతరంగ: \
    using V3_Rtti_పునాది_Class = శూన్య; \
    V3RTTIINTERNAL_VL_RTTI_COMMON_IMPL(ThisClass, /*పునాది*/) \
అంతరంగ:

#అయితే_ముగింపు  // Guard
