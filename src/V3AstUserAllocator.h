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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"

#অন্তর্ভাব <type_traits>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

template <typename T_গ্রন্থি, typename T_তথ্য, পূর্ণসংখ্যা N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "T_Node must be an AstNode type");

ব্যক্তিগত:
    std::deque<T_তথ্য> m_allocated;

    T_তথ্য* getUserp(ধ্রুবক T_গ্রন্থি* nodep) ধ্রুবক {
        যদি VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ধ্রুবক VNUser user = nodep->user1u();
            ফেরত user.to<T_তথ্য*>();
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ধ্রুবক VNUser user = nodep->user2u();
            ফেরত user.to<T_তথ্য*>();
        } অন্যথা যদি VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ধ্রুবক VNUser user = nodep->user3u();
            ফেরত user.to<T_তথ্য*>();
        } অন্যথা {
            ধ্রুবক VNUser user = nodep->user4u();
            ফেরত user.to<T_তথ্য*>();
        }
    }

    শূন্য setUserp(T_গ্রন্থি* nodep, T_তথ্য* userp) ধ্রুবক {
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

সুরক্ষিত:
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

সর্বজনীন:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_তথ্য& operator()(T_গ্রন্থি* nodep, Args&&... args) {
        T_তথ্য* userp = getUserp(nodep);
        যদি (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        ফেরত *userp;
    }

    // Get a reference to the user data
    T_তথ্য& operator()(ধ্রুবক T_গ্রন্থি* nodep) ধ্রুবক {
        T_তথ্য* ধ্রুবক userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        ফেরত *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_তথ্য* tryGet(ধ্রুবক T_গ্রন্থি* nodep) ধ্রুবক { ফেরত getUserp(nodep); }

    শূন্য clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_গ্রন্থি, typename T_তথ্য>
class AstUser1Allocator final : সর্বজনীন AstUserAllocatorBase<T_গ্রন্থি, T_তথ্য, 1> {};
template <typename T_গ্রন্থি, typename T_তথ্য>
class AstUser2Allocator final : সর্বজনীন AstUserAllocatorBase<T_গ্রন্থি, T_তথ্য, 2> {};
template <typename T_গ্রন্থি, typename T_তথ্য>
class AstUser3Allocator final : সর্বজনীন AstUserAllocatorBase<T_গ্রন্থি, T_তথ্য, 3> {};
template <typename T_গ্রন্থি, typename T_তথ্য>
class AstUser4Allocator final : সর্বজনীন AstUserAllocatorBase<T_গ্রন্থি, T_তথ্য, 4> {};

#endif  // Guard
