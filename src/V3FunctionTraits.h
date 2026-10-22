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

#నిర్వచితం_కాకపోతే VERILATOR_V3FUNCTIONTRAITS_H_
#నిర్వచించు VERILATOR_V3FUNCTIONTRAITS_H_

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <cstddef>
#అన్తర్భావ <tuple>
#అన్తర్భావ <type_traits>

మూస <typename T>
నిర్మాణం FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
మూస <typename T>
నిర్మాణం FunctionTraits final
    : బహిరంగ FunctionTraits<decltype(&std::remove_reference<T>::type::క్రియాకారకం())> {};

// Specialization for pointers to member function
మూస <typename T_ClassType, typename T_ReturnType, typename... వాదనలు_2>
నిర్మాణం FunctionTraits<T_ReturnType (T_ClassType::*)(వాదనలు_2...) స్థిరరాశి> VL_NOT_FINAL {
    // Number of arguments
    స్థిర constexpr కొలత_t arity = పరిమాణం...(వాదనలు_2);

    // Type of result
    using ఫలితం_type = T_ReturnType;

    // Type of arguments
    మూస <std::కొలత_t N>
    నిర్మాణం arg final {
        using type = typename std::tuple_element<N, std::tuple<వాదనలు_2...>>::type;
    };
};

మూస <typename T_Callable, కొలత_t N_సూచీ>
నిర్మాణం Function_Arg_No_సూచిక_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::మూస arg<N_సూచీ>::type;
    using T_ArgNoPtr = typename std::remove_సూచిక<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#అయితే_ముగింపు
