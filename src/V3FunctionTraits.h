// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Function traits for metaprogramming
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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3FUNCTIONTRAITS_H_
#وصف_ڪريو VERILATOR_V3FUNCTIONTRAITS_H_

#ضمن "verilatedos.h"

#ضمن <cstddef>
#ضمن <tuple>
#ضمن <type_traits>

سانچو <typename T>
بنية FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
سانچو <typename T>
بنية FunctionTraits final
    : عام FunctionTraits<decltype(&std::remove_reference<T>::type::عامل())> {};

// Specialization for pointers to member function
سانچو <typename T_ClassType, typename T_ReturnType, typename... دليلون_2>
بنية FunctionTraits<T_ReturnType (T_ClassType::*)(دليلون_2...) ثابت> VL_NOT_FINAL {
    // Number of arguments
    ساكن constexpr سائيز_t arity = حجم...(دليلون_2);

    // Type of result
    using نتيجو_type = T_ReturnType;

    // Type of arguments
    سانچو <std::سائيز_t N>
    بنية arg final {
        using type = typename std::tuple_element<N, std::tuple<دليلون_2...>>::type;
    };
};

سانچو <typename T_Callable, سائيز_t N_اشاريو>
بنية Function_Arg_No_اشارو_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::سانچو arg<N_اشاريو>::type;
    using T_ArgNoPtr = typename std::remove_اشارو<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#جيڪڏهن_پورو
