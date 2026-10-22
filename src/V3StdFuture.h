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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3STDFUTURE_H_
#وصف_ڪريو VERILATOR_V3STDFUTURE_H_

#ضمن <functional>

namespace vlstd {

// C++17 is_invocable
سانچو <typename F, typename... دليلون_2>
بنية is_invocable
    : std::is_constructible<std::function<فراغ(دليلون_2...)>,
                            std::reference_wrapper<typename std::remove_reference<F>::type>> {};

// C++17 is_invocable_r
سانچو <typename R, typename F, typename... دليلون_2>
بنية is_invocable_r
    : std::is_constructible<std::function<R(دليلون_2...)>,
                            std::reference_wrapper<typename std::remove_reference<F>::type>> {};
};  // namespace vlstd

#جيڪڏهن_پورو  // Guard
