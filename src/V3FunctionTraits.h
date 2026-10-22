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

#اگر_تعریف_نشده VERILATOR_V3FUNCTIONTRAITS_H_
#تعریف_کن VERILATOR_V3FUNCTIONTRAITS_H_

#ضمن "verilatedos.h"

#ضمن <cstddef>
#ضمن <tuple>
#ضمن <type_traits>

قالب <typename T>
بنية FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
قالب <typename T>
بنية FunctionTraits final
    : عام FunctionTraits<decltype(&std::remove_reference<T>::type::عملگر())> {};

// Specialization for pointers to member function
قالب <typename T_ClassType, typename T_ReturnType, typename... آرگومانها_2>
بنية FunctionTraits<T_ReturnType (T_ClassType::*)(آرگومانها_2...) ثابت> VL_NOT_FINAL {
    // Number of arguments
    ساكن constexpr اندازه_t arity = حجم...(آرگومانها_2);

    // Type of result
    using نتیجه_type = T_ReturnType;

    // Type of arguments
    قالب <std::اندازه_t N>
    بنية arg final {
        using type = typename std::tuple_element<N, std::tuple<آرگومانها_2...>>::type;
    };
};

قالب <typename T_Callable, اندازه_t N_شاخص>
بنية Function_Arg_No_اشاره_گر_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::قالب arg<N_شاخص>::type;
    using T_ArgNoPtr = typename std::remove_اشاره_گر<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#پایان_اگر
