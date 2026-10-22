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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

#अन्तर्भाव <type_traits>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

template <typename T_ग्रन्थिः, typename T_Data, पूर्णाङ्क N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "T_Node must be an AstNode type");

गुप्त:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(ध्रुव T_ग्रन्थिः* nodep) ध्रुव {
        जइ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ध्रुव VNUser user = nodep->user1u();
            प्रत्यागम user.to<T_Data*>();
        } अण्णहा जइ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ध्रुव VNUser user = nodep->user2u();
            प्रत्यागम user.to<T_Data*>();
        } अण्णहा जइ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ध्रुव VNUser user = nodep->user3u();
            प्रत्यागम user.to<T_Data*>();
        } अण्णहा {
            ध्रुव VNUser user = nodep->user4u();
            प्रत्यागम user.to<T_Data*>();
        }
    }

    शून्य setUserp(T_ग्रन्थिः* nodep, T_Data* userp) ध्रुव {
        जइ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } अण्णहा जइ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } अण्णहा जइ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } अण्णहा {
            nodep->user4u(VNUser{userp});
        }
    }

रक्षित:
    AstUserAllocatorBase() {
        जइ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } अण्णहा जइ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } अण्णहा जइ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } अण्णहा {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

सार्वजनिक:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_ग्रन्थिः* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        जइ (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        प्रत्यागम *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(ध्रुव T_ग्रन्थिः* nodep) ध्रुव {
        T_Data* ध्रुव userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        प्रत्यागम *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(ध्रुव T_ग्रन्थिः* nodep) ध्रुव { प्रत्यागम getUserp(nodep); }

    शून्य clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_ग्रन्थिः, typename T_Data>
class AstUser1Allocator final : सार्वजनिक AstUserAllocatorBase<T_ग्रन्थिः, T_Data, 1> {};
template <typename T_ग्रन्थिः, typename T_Data>
class AstUser2Allocator final : सार्वजनिक AstUserAllocatorBase<T_ग्रन्थिः, T_Data, 2> {};
template <typename T_ग्रन्थिः, typename T_Data>
class AstUser3Allocator final : सार्वजनिक AstUserAllocatorBase<T_ग्रन्थिः, T_Data, 3> {};
template <typename T_ग्रन्थिः, typename T_Data>
class AstUser4Allocator final : सार्वजनिक AstUserAllocatorBase<T_ग्रन्थिः, T_Data, 4> {};

#endif  // Guard
