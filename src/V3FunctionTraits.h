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

#अगर_अघोषित VERILATOR_V3FUNCTIONTRAITS_H_
#घोषणा VERILATOR_V3FUNCTIONTRAITS_H_

#समावेश "verilatedos.h"

#समावेश <cstddef>
#समावेश <tuple>
#समावेश <type_traits>

ढाँचा <typename T>
काष्ठा FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
ढाँचा <typename T>
काष्ठा FunctionTraits final
    : खुला FunctionTraits<decltype(&std::remove_reference<T>::type::चालक())> {};

// Specialization for pointers to member function
ढाँचा <typename T_ClassType, typename T_ReturnType, typename... Args>
काष्ठा FunctionTraits<T_ReturnType (T_ClassType::*)(Args...) स्थिर> VL_NOT_FINAL {
    // Number of arguments
    अटल constexpr माप_प्रकार arity = माप...(Args);

    // Type of result
    using परिणाम_type = T_ReturnType;

    // Type of arguments
    ढाँचा <std::माप_प्रकार N>
    काष्ठा arg final {
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

ढाँचा <typename T_Callable, माप_प्रकार N_सूचकांक>
काष्ठा FunctionArgNoPointerNoCV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::ढाँचा arg<N_सूचकांक>::type;
    using T_ArgNoPtr = typename std::remove_pointer<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#पूर्ण_अगर
