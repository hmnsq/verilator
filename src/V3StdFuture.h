// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Backports of features from future C++ versions
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3STDFUTURE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3STDFUTURE_H_

#ಅನ್ತರ್ಭಾವ <functional>

namespace vlstd {

// C++17 is_invocable
ಮಾದರಿ <typename F, typename... ವಾದಗಳು_2>
ರಚನೆ is_invocable
    : std::is_constructible<std::function<ಶೂನ್ಯ(ವಾದಗಳು_2...)>,
                            std::reference_wrapper<typename std::remove_reference<F>::type>> {};

// C++17 is_invocable_r
ಮಾದರಿ <typename R, typename F, typename... ವಾದಗಳು_2>
ರಚನೆ is_invocable_r
    : std::is_constructible<std::function<R(ವಾದಗಳು_2...)>,
                            std::reference_wrapper<typename std::remove_reference<F>::type>> {};
};  // namespace vlstd

#ಆದರೆ_ಅಂತ್ಯ  // Guard
