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

#ifndef VERILATOR_V3FUNCTIONTRAITS_H_
#define VERILATOR_V3FUNCTIONTRAITS_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstddef>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <type_traits>

template <typename T>
ಸಂರಚನಾ FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
template <typename T>
ಸಂರಚನಾ FunctionTraits final
    : ಸಾರ್ವಜನಿಕ FunctionTraits<decltype(&std::remove_reference<T>::type::operator())> {};

// Specialization for pointers to member function
template <typename T_ClassType, typename T_ReturnType, typename... Args>
ಸಂರಚನಾ FunctionTraits<T_ReturnType (T_ClassType::*)(Args...) ಧ್ರುವ> VL_NOT_FINAL {
    // Number of arguments
    ಸ್ಥಿರ constexpr size_t arity = ಪರಿಮಾಣ...(Args);

    // Type of result
    using result_type = T_ReturnType;

    // Type of arguments
    template <std::size_t N>
    ಸಂರಚನಾ arg final {
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template <typename T_Callable, size_t N_Index>
ಸಂರಚನಾ FunctionArgNoPointerNoCV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::template arg<N_Index>::type;
    using T_ArgNoPtr = typename std::remove_pointer<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#endif
