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

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <cstddef>
#अन्तर्भाव <tuple>
#अन्तर्भाव <type_traits>

ढाँचा <typename T>
संरचना FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
ढाँचा <typename T>
संरचना FunctionTraits final
    : सार्वजनिक FunctionTraits<decltype(&std::remove_reference<T>::type::चालक())> {};

// Specialization for pointers to member function
ढाँचा <typename T_ClassType, typename T_ReturnType, typename... Args>
संरचना FunctionTraits<T_ReturnType (T_ClassType::*)(Args...) स्थिरांक> VL_NOT_FINAL {
    // Number of arguments
    स्थिर constexpr माप_प्रकार arity = आकार...(Args);

    // Type of result
    using result_type = T_ReturnType;

    // Type of arguments
    ढाँचा <std::माप_प्रकार N>
    संरचना arg final {
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

ढाँचा <typename T_Callable, माप_प्रकार N_Index>
संरचना FunctionArgNoPointerNoCV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::ढाँचा arg<N_Index>::type;
    using T_ArgNoPtr = typename std::remove_pointer<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#endif
