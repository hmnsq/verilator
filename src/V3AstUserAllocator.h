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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"

#অন্তর্ভাৱ <type_traits>
#অন্তর্ভাৱ <utility>
#অন্তর্ভাৱ <vector>

template <typename T_গ্ৰন্থি, typename T_Data, পূর্ণাঙ্ক N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_গ্ৰন্থি, T_গ্ৰন্থি>::মান, "T_Node must be an AstNode type");

গুপ্ত:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(ধ্রুৱ T_গ্ৰন্থি* nodep) ধ্রুৱ {
        যদি VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ধ্রুৱ VNUser user = nodep->user1u();
            ঘূৰাই user.to<T_Data*>();
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ধ্রুৱ VNUser user = nodep->user2u();
            ঘূৰাই user.to<T_Data*>();
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ধ্রুৱ VNUser user = nodep->user3u();
            ঘূৰাই user.to<T_Data*>();
        } অন্যথা {
            ধ্রুৱ VNUser user = nodep->user4u();
            ঘূৰাই user.to<T_Data*>();
        }
    }

    শূন্য setUserp(T_গ্ৰন্থি* nodep, T_Data* userp) ধ্রুৱ {
        যদি VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } অন্যথা {
            nodep->user4u(VNUser{userp});
        }
    }

রক্ষিত:
    AstUserAllocatorBase() {
        যদি VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } অন্যথা {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

সার্ৱজনিক:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_গ্ৰন্থি* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        যদি (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        ঘূৰাই *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(ধ্রুৱ T_গ্ৰন্থি* nodep) ধ্রুৱ {
        T_Data* ধ্রুৱ userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        ঘূৰাই *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(ধ্রুৱ T_গ্ৰন্থি* nodep) ধ্রুৱ { ঘূৰাই getUserp(nodep); }

    শূন্য clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_গ্ৰন্থি, typename T_Data>
class AstUser1Allocator final : সার্ৱজনিক AstUserAllocatorBase<T_গ্ৰন্থি, T_Data, 1> {};
template <typename T_গ্ৰন্থি, typename T_Data>
class AstUser2Allocator final : সার্ৱজনিক AstUserAllocatorBase<T_গ্ৰন্থি, T_Data, 2> {};
template <typename T_গ্ৰন্থি, typename T_Data>
class AstUser3Allocator final : সার্ৱজনিক AstUserAllocatorBase<T_গ্ৰন্থি, T_Data, 3> {};
template <typename T_গ্ৰন্থি, typename T_Data>
class AstUser4Allocator final : সার্ৱজনিক AstUserAllocatorBase<T_গ্ৰন্থি, T_Data, 4> {};

#endif  // Guard
