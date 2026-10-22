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

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ <cstddef>
#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <type_traits>

template <typename T>
ସଂରଚନା FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
template <typename T>
ସଂରଚନା FunctionTraits final
    : ସର୍ବସାଧାରଣ FunctionTraits<decltype(&std::remove_reference<T>::type::operator())> {};

// Specialization for pointers to member function
template <typename T_ClassType, typename T_ReturnType, typename... Args>
ସଂରଚନା FunctionTraits<T_ReturnType (T_ClassType::*)(Args...) ଅଚଳ> VL_NOT_FINAL {
    // Number of arguments
    ସ୍ଥିର constexpr size_t arity = ଆକାର...(Args);

    // Type of result
    using result_type = T_ReturnType;

    // Type of arguments
    template <std::size_t N>
    ସଂରଚନା arg final {
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template <typename T_Callable, size_t N_Index>
ସଂରଚନା FunctionArgNoPointerNoCV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::template arg<N_Index>::type;
    using T_ArgNoPtr = typename std::remove_pointer<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#endif
