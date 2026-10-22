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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"

#شامل <type_traits>
#شامل <utility>
#شامل <vector>

template <typename T_گره, typename T_Data, صحیح N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_گره, T_گره>::مقدار, "T_Node must be an AstNode type");

خصوصی:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(ثابت T_گره* nodep) ثابت {
        اگر VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ثابت VNUser user = nodep->user1u();
            بازگشت user.to<T_Data*>();
        } وگرنه اگر VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ثابت VNUser user = nodep->user2u();
            بازگشت user.to<T_Data*>();
        } وگرنه اگر VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ثابت VNUser user = nodep->user3u();
            بازگشت user.to<T_Data*>();
        } وگرنه {
            ثابت VNUser user = nodep->user4u();
            بازگشت user.to<T_Data*>();
        }
    }

    تهی setUserp(T_گره* nodep, T_Data* userp) ثابت {
        اگر VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } وگرنه اگر VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } وگرنه اگر VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } وگرنه {
            nodep->user4u(VNUser{userp});
        }
    }

محافظتشده:
    AstUserAllocatorBase() {
        اگر VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } وگرنه اگر VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } وگرنه اگر VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } وگرنه {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

عمومی:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_گره* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        اگر (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        بازگشت *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(ثابت T_گره* nodep) ثابت {
        T_Data* ثابت userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        بازگشت *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(ثابت T_گره* nodep) ثابت { بازگشت getUserp(nodep); }

    تهی clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_گره, typename T_Data>
class AstUser1Allocator final : عمومی AstUserAllocatorBase<T_گره, T_Data, 1> {};
template <typename T_گره, typename T_Data>
class AstUser2Allocator final : عمومی AstUserAllocatorBase<T_گره, T_Data, 2> {};
template <typename T_گره, typename T_Data>
class AstUser3Allocator final : عمومی AstUserAllocatorBase<T_گره, T_Data, 3> {};
template <typename T_گره, typename T_Data>
class AstUser4Allocator final : عمومی AstUserAllocatorBase<T_گره, T_Data, 4> {};

#endif  // Guard
