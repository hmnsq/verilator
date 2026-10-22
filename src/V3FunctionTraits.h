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

#که_نه_تعریف_شوی VERILATOR_V3FUNCTIONTRAITS_H_
#تعریف_کړه VERILATOR_V3FUNCTIONTRAITS_H_

#ضمن "verilatedos.h"

#ضمن <cstddef>
#ضمن <tuple>
#ضمن <type_traits>

کينډۍ <typename T>
بنية FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
کينډۍ <typename T>
بنية FunctionTraits final
    : عام FunctionTraits<decltype(&std::remove_reference<T>::type::چلوونکی())> {};

// Specialization for pointers to member function
کينډۍ <typename T_ClassType, typename T_ReturnType, typename... دلیلونه_2>
بنية FunctionTraits<T_ReturnType (T_ClassType::*)(دلیلونه_2...) ثابت> VL_NOT_FINAL {
    // Number of arguments
    ساكن constexpr اندازه_t arity = حجم...(دلیلونه_2);

    // Type of result
    using پایله_type = T_ReturnType;

    // Type of arguments
    کينډۍ <std::اندازه_t N>
    بنية arg final {
        using type = typename std::tuple_element<N, std::tuple<دلیلونه_2...>>::type;
    };
};

کينډۍ <typename T_Callable, اندازه_t N_شاخص>
بنية Function_Arg_No_اشاره_ګر_No_CV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::کينډۍ arg<N_شاخص>::type;
    using T_ArgNoPtr = typename std::remove_اشاره_ګر<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#د_که_پای
