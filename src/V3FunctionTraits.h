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

#यदि_अपरिभाषितम् VERILATOR_V3FUNCTIONTRAITS_H_
#परिभाषय VERILATOR_V3FUNCTIONTRAITS_H_

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <cstddef>
#अन्तर्भाव <tuple>
#अन्तर्भाव <type_traits>

प्रतिरूपम् <typename T>
संरचना FunctionTraits;

// For generic types, directly use the result of the signature of its 'operator()'
प्रतिरूपम् <typename T>
संरचना FunctionTraits final
    : सार्वजनिक FunctionTraits<decltype(&std::remove_reference<T>::type::संक्रियकः())> {};

// Specialization for pointers to member function
प्रतिरूपम् <typename T_ClassType, typename T_ReturnType, typename... तर्काः_2>
संरचना FunctionTraits<T_ReturnType (T_ClassType::*)(तर्काः_2...) ध्रुव> VL_NOT_FINAL {
    // Number of arguments
    स्थिर constexpr आकार_t arity = परिमाण...(तर्काः_2);

    // Type of result
    using परिणाम_type = T_ReturnType;

    // Type of arguments
    प्रतिरूपम् <std::आकार_t N>
    संरचना arg final {
        using type = typename std::tuple_element<N, std::tuple<तर्काः_2...>>::type;
    };
};

प्रतिरूपम् <typename T_Callable, आकार_t N_सूचकांक>
संरचना FunctionArgNoPointerNoCV final {
    using Traits = FunctionTraits<T_Callable>;
    using T_Arg = typename Traits::प्रतिरूपम् arg<N_सूचकांक>::type;
    using T_ArgNoPtr = typename std::remove_pointer<T_Arg>::type;
    using type = typename std::remove_cv<T_ArgNoPtr>::type;
};

#यदि_अन्तः
