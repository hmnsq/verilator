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

#جے_بنا_تعریف VERILATOR_V3FUNCTIONTRAITS_H_
#تعریف_کرو VERILATOR_V3FUNCTIONTRAITS_H_

#ضمن "verilatedos.h"

#ضمن <cstddef>
#ضمن <tuple>
#ضمن <type_traits>

سانچا <typename T>
بنية FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
سانچا <typename T>
بنية FunctionTraits final
    : عام FunctionTraits<decltype(&std::remove_reference<T>::type::عامل())> {};

// Specialization for pointers to member function
سانچا <typename T_ClassType, typename T_ReturnType, typename... دلیلاں_2>
بنية FunctionTraits<T_ReturnType (T_ClassType::*)(دلیلاں_2...) ثابت> VL_NOT_FINAL {
    // Number of arguments
    ساكن constexpr ناپ_t arity = حجم...(دلیلاں_2);

    // Type of result
    using نتیجہ_type = T_ReturnType;

    // Type of arguments
    سانچا <std::ناپ_t N>
    بنية arg final {
        using type = typename std::tuple_element<N, std::tuple<دلیلاں_2...>>::type;
    };
};

سانچا <typename T_Callable, ناپ_t N_اشاریہ>
بنية Function_Arg_No_اشارہ_گر_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::سانچا arg<N_اشاریہ>::type;
    using T_ArgNoPtr = typename std::remove_اشارہ_گر<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#جے_انت
