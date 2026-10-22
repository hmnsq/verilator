// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Utility to hang advanced data structures of
// AstNode::user*p() pointers with automatic memory management.
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

#ifndef VERILATOR_V3ASTUSERALLOCATOR_H_
#define VERILATOR_V3ASTUSERALLOCATOR_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"

#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

template <typename T_കണ്ണി, typename T_Data, പൂർണ്ണസംഖ്യ N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "T_Node must be an AstNode type");

സ്വകാര്യ:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(സ്ഥിരം T_കണ്ണി* nodep) സ്ഥിരം {
        എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            സ്ഥിരം VNUser user = nodep->user1u();
            തിരികെ user.to<T_Data*>();
        } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            സ്ഥിരം VNUser user = nodep->user2u();
            തിരികെ user.to<T_Data*>();
        } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            സ്ഥിരം VNUser user = nodep->user3u();
            തിരികെ user.to<T_Data*>();
        } അല്ലെങ്കിൽ {
            സ്ഥിരം VNUser user = nodep->user4u();
            തിരികെ user.to<T_Data*>();
        }
    }

    ശൂന്യം setUserp(T_കണ്ണി* nodep, T_Data* userp) സ്ഥിരം {
        എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } അല്ലെങ്കിൽ {
            nodep->user4u(VNUser{userp});
        }
    }

സംരക്ഷിത:
    AstUserAllocatorBase() {
        എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } അല്ലെങ്കിൽ {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

പൊതു:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_കണ്ണി* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        എങ്കിൽ (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        തിരികെ *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(സ്ഥിരം T_കണ്ണി* nodep) സ്ഥിരം {
        T_Data* സ്ഥിരം userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        തിരികെ *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(സ്ഥിരം T_കണ്ണി* nodep) സ്ഥിരം { തിരികെ getUserp(nodep); }

    ശൂന്യം clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_കണ്ണി, typename T_Data>
class AstUser1Allocator final : പൊതു AstUserAllocatorBase<T_കണ്ണി, T_Data, 1> {};
template <typename T_കണ്ണി, typename T_Data>
class AstUser2Allocator final : പൊതു AstUserAllocatorBase<T_കണ്ണി, T_Data, 2> {};
template <typename T_കണ്ണി, typename T_Data>
class AstUser3Allocator final : പൊതു AstUserAllocatorBase<T_കണ്ണി, T_Data, 3> {};
template <typename T_കണ്ണി, typename T_Data>
class AstUser4Allocator final : പൊതു AstUserAllocatorBase<T_കണ്ണി, T_Data, 4> {};

#endif  // Guard
