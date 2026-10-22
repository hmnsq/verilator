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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"

#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

template <typename T_முனை, typename T_Data, முழு N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "T_Node must be an AstNode type");

தனிப்பட்ட:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(மாறிலி T_முனை* nodep) மாறிலி {
        எனில் VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            மாறிலி VNUser user = nodep->user1u();
            திரும்பு user.to<T_Data*>();
        } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            மாறிலி VNUser user = nodep->user2u();
            திரும்பு user.to<T_Data*>();
        } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            மாறிலி VNUser user = nodep->user3u();
            திரும்பு user.to<T_Data*>();
        } இல்லையெனில் {
            மாறிலி VNUser user = nodep->user4u();
            திரும்பு user.to<T_Data*>();
        }
    }

    வெறுமை setUserp(T_முனை* nodep, T_Data* userp) மாறிலி {
        எனில் VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } இல்லையெனில் {
            nodep->user4u(VNUser{userp});
        }
    }

பாதுகாக்கப்பட்ட:
    AstUserAllocatorBase() {
        எனில் VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } இல்லையெனில் {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

பொது:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_முனை* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        எனில் (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        திரும்பு *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(மாறிலி T_முனை* nodep) மாறிலி {
        T_Data* மாறிலி userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        திரும்பு *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(மாறிலி T_முனை* nodep) மாறிலி { திரும்பு getUserp(nodep); }

    வெறுமை clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_முனை, typename T_Data>
class AstUser1Allocator final : பொது AstUserAllocatorBase<T_முனை, T_Data, 1> {};
template <typename T_முனை, typename T_Data>
class AstUser2Allocator final : பொது AstUserAllocatorBase<T_முனை, T_Data, 2> {};
template <typename T_முனை, typename T_Data>
class AstUser3Allocator final : பொது AstUserAllocatorBase<T_முனை, T_Data, 3> {};
template <typename T_முனை, typename T_Data>
class AstUser4Allocator final : பொது AstUserAllocatorBase<T_முனை, T_Data, 4> {};

#endif  // Guard
