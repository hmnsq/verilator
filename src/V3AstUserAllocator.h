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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

template <typename T_ಗಂಟು, typename T_Data, ಪೂರ್ಣಾಙ್ಕ N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "T_Node must be an AstNode type");

ಗುಪ್ತ:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(ಧ್ರುವ T_ಗಂಟು* nodep) ಧ್ರುವ {
        ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ಧ್ರುವ VNUser user = nodep->user1u();
            ಪ್ರತ್ಯಾಗಮ user.to<T_Data*>();
        } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ಧ್ರುವ VNUser user = nodep->user2u();
            ಪ್ರತ್ಯಾಗಮ user.to<T_Data*>();
        } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ಧ್ರುವ VNUser user = nodep->user3u();
            ಪ್ರತ್ಯಾಗಮ user.to<T_Data*>();
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ VNUser user = nodep->user4u();
            ಪ್ರತ್ಯಾಗಮ user.to<T_Data*>();
        }
    }

    ಶೂನ್ಯ setUserp(T_ಗಂಟು* nodep, T_Data* userp) ಧ್ರುವ {
        ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } ಅನ್ಯಥಾ {
            nodep->user4u(VNUser{userp});
        }
    }

ರಕ್ಷಿತ:
    AstUserAllocatorBase() {
        ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } ಅನ್ಯಥಾ {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

ಸಾರ್ವಜನಿಕ:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_ಗಂಟು* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        ಯದಿ (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        ಪ್ರತ್ಯಾಗಮ *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(ಧ್ರುವ T_ಗಂಟು* nodep) ಧ್ರುವ {
        T_Data* ಧ್ರುವ userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        ಪ್ರತ್ಯಾಗಮ *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(ಧ್ರುವ T_ಗಂಟು* nodep) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ getUserp(nodep); }

    ಶೂನ್ಯ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_ಗಂಟು, typename T_Data>
class AstUser1Allocator final : ಸಾರ್ವಜನಿಕ AstUserAllocatorBase<T_ಗಂಟು, T_Data, 1> {};
template <typename T_ಗಂಟು, typename T_Data>
class AstUser2Allocator final : ಸಾರ್ವಜನಿಕ AstUserAllocatorBase<T_ಗಂಟು, T_Data, 2> {};
template <typename T_ಗಂಟು, typename T_Data>
class AstUser3Allocator final : ಸಾರ್ವಜನಿಕ AstUserAllocatorBase<T_ಗಂಟು, T_Data, 3> {};
template <typename T_ಗಂಟು, typename T_Data>
class AstUser4Allocator final : ಸಾರ್ವಜನಿಕ AstUserAllocatorBase<T_ಗಂಟು, T_Data, 4> {};

#endif  // Guard
