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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3FUNCTIONTRAITS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3FUNCTIONTRAITS_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <cstddef>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <type_traits>

ಮಾದರಿ <typename T>
ರಚನೆ FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
ಮಾದರಿ <typename T>
ರಚನೆ FunctionTraits final
    : ಸಾರ್ವಜನಿಕ FunctionTraits<decltype(&std::remove_reference<T>::type::ಕ್ರಿಯಾಕಾರಕ())> {};

// Specialization for pointers to member function
ಮಾದರಿ <typename T_ClassType, typename T_ReturnType, typename... ವಾದಗಳು_2>
ರಚನೆ FunctionTraits<T_ReturnType (T_ClassType::*)(ವಾದಗಳು_2...) ಸ್ಥಿರಾಂಕ> VL_NOT_FINAL {
    // Number of arguments
    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t arity = ಗಾತ್ರ...(ವಾದಗಳು_2);

    // Type of result
    using ಫಲಿತಾಂಶ_type = T_ReturnType;

    // Type of arguments
    ಮಾದರಿ <std::ಗಾತ್ರ_t N>
    ರಚನೆ arg final {
        using type = typename std::tuple_element<N, std::tuple<ವಾದಗಳು_2...>>::type;
    };
};

ಮಾದರಿ <typename T_Callable, ಗಾತ್ರ_t N_ಸೂಚ್ಯಂಕ>
ರಚನೆ Function_Arg_No_ಸೂಚಕ_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::ಮಾದರಿ arg<N_ಸೂಚ್ಯಂಕ>::type;
    using T_ArgNoPtr = typename std::remove_ಸೂಚಕ<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#ಆದರೆ_ಅಂತ್ಯ
