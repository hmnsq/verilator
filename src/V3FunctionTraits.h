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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3FUNCTIONTRAITS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3FUNCTIONTRAITS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstddef>
#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>

ਸਾਂਚਾ <typename T>
ਬਣਤਰ FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
ਸਾਂਚਾ <typename T>
ਬਣਤਰ FunctionTraits final
    : ਜਨਤਕ FunctionTraits<decltype(&std::remove_reference<T>::type::ਸੰਚਾਲਕ())> {};

// Specialization for pointers to member function
ਸਾਂਚਾ <typename T_ClassType, typename T_ReturnType, typename... ਦਲੀਲਾਂ_2>
ਬਣਤਰ FunctionTraits<T_ReturnType (T_ClassType::*)(ਦਲੀਲਾਂ_2...) ਅਚਲ> VL_NOT_FINAL {
    // Number of arguments
    ਸਥਿਰ constexpr ਮਾਪ_t arity = ਆਕਾਰ...(ਦਲੀਲਾਂ_2);

    // Type of result
    using ਨਤੀਜਾ_type = T_ReturnType;

    // Type of arguments
    ਸਾਂਚਾ <std::ਮਾਪ_t N>
    ਬਣਤਰ arg final {
        using type = typename std::tuple_element<N, std::tuple<ਦਲੀਲਾਂ_2...>>::type;
    };
};

ਸਾਂਚਾ <typename T_Callable, ਮਾਪ_t N_ਸੂਚਕਾਂਕ>
ਬਣਤਰ Function_Arg_No_ਸੂਚਕ_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::ਸਾਂਚਾ arg<N_ਸੂਚਕਾਂਕ>::type;
    using T_ArgNoPtr = typename std::remove_ਸੂਚਕ<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#ਜੇ_ਅੰਤ
