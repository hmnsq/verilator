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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"

#અન્તર્ભાવ <type_traits>
#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

template <typename T_ગાંઠ, typename T_Data, પૂર્ણાંક N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_ગાંઠ, T_ગાંઠ>::કિંમત, "T_Node must be an AstNode type");

ખાનગી:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(અચલ T_ગાંઠ* nodep) અચલ {
        જો VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            અચલ VNUser user = nodep->user1u();
            પાછા user.to<T_Data*>();
        } નહીંતર જો VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            અચલ VNUser user = nodep->user2u();
            પાછા user.to<T_Data*>();
        } નહીંતર જો VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            અચલ VNUser user = nodep->user3u();
            પાછા user.to<T_Data*>();
        } નહીંતર {
            અચલ VNUser user = nodep->user4u();
            પાછા user.to<T_Data*>();
        }
    }

    શૂન્ય setUserp(T_ગાંઠ* nodep, T_Data* userp) અચલ {
        જો VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } નહીંતર જો VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } નહીંતર જો VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } નહીંતર {
            nodep->user4u(VNUser{userp});
        }
    }

સુરક્ષિત:
    AstUserAllocatorBase() {
        જો VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } નહીંતર જો VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } નહીંતર જો VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } નહીંતર {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

જાહેર:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_ગાંઠ* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        જો (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        પાછા *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(અચલ T_ગાંઠ* nodep) અચલ {
        T_Data* અચલ userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        પાછા *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(અચલ T_ગાંઠ* nodep) અચલ { પાછા getUserp(nodep); }

    શૂન્ય clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_ગાંઠ, typename T_Data>
class AstUser1Allocator final : જાહેર AstUserAllocatorBase<T_ગાંઠ, T_Data, 1> {};
template <typename T_ગાંઠ, typename T_Data>
class AstUser2Allocator final : જાહેર AstUserAllocatorBase<T_ગાંઠ, T_Data, 2> {};
template <typename T_ગાંઠ, typename T_Data>
class AstUser3Allocator final : જાહેર AstUserAllocatorBase<T_ગાંઠ, T_Data, 3> {};
template <typename T_ગાંઠ, typename T_Data>
class AstUser4Allocator final : જાહેર AstUserAllocatorBase<T_ગાંઠ, T_Data, 4> {};

#endif  // Guard
