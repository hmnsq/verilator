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

#اگر_غیر_تعریف_شدہ VERILATOR_V3FUNCTIONTRAITS_H_
#تعریف_کرو VERILATOR_V3FUNCTIONTRAITS_H_

#شامل "verilatedos.h"

#شامل <cstddef>
#شامل <tuple>
#شامل <type_traits>

سانچہ <typename T>
ساخت FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
سانچہ <typename T>
ساخت FunctionTraits final
    : عوامی FunctionTraits<decltype(&std::remove_reference<T>::type::عامل())> {};

// Specialization for pointers to member function
سانچہ <typename T_ClassType, typename T_ReturnType, typename... دلائل_2>
ساخت FunctionTraits<T_ReturnType (T_ClassType::*)(دلائل_2...) ثابت> VL_NOT_FINAL {
    // Number of arguments
    ساکن constexpr سائز_t arity = حجم...(دلائل_2);

    // Type of result
    using نتیجہ_type = T_ReturnType;

    // Type of arguments
    سانچہ <std::سائز_t N>
    ساخت arg final {
        using type = typename std::tuple_element<N, std::tuple<دلائل_2...>>::type;
    };
};

سانچہ <typename T_Callable, سائز_t N_اشاریہ>
ساخت Function_Arg_No_اشارہ_گر_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::سانچہ arg<N_اشاریہ>::type;
    using T_ArgNoPtr = typename std::remove_اشارہ_گر<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#اگر_ختم
