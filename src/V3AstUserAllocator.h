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

#که_نه_تعریف_شوی VERILATOR_V3ASTUSERALLOCATOR_H_
#تعریف_کړه VERILATOR_V3ASTUSERALLOCATOR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

#ضمن <type_traits>
#ضمن <utility>
#ضمن <vector>

کينډۍ <typename T_غوټه, typename T_معلومات, صحيح N_UserN>
class Ast_User_Allocator_بنسټ VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "T_Node must be an AstNode type");

خاص:
    std::deque<T_معلومات> m_allocated;

    T_معلومات* getUserp(ثابت T_غوټه* nodep) ثابت {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ثابت VNUser user = nodep->user1u();
            أرجع user.منزل<T_معلومات*>();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ثابت VNUser user = nodep->user2u();
            أرجع user.منزل<T_معلومات*>();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ثابت VNUser user = nodep->user3u();
            أرجع user.منزل<T_معلومات*>();
        } وإلا {
            ثابت VNUser user = nodep->user4u();
            أرجع user.منزل<T_معلومات*>();
        }
    }

    فراغ سېټ_Userp(T_غوټه* nodep, T_معلومات* userp) ثابت {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } وإلا {
            nodep->user4u(VNUser{userp});
        }
    }

محمي:
    Ast_User_Allocator_بنسټ() {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_دننه_Use::check();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_دننه_Use::check();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_دننه_Use::check();
        } وإلا {
            VNUser4_دننه_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_بنسټ);

عام:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    کينډۍ <typename... دلیلونه_2>
    T_معلومات& چلوونکی()(T_غوټه* nodep, دلیلونه_2&&... دلیلونه) {
        T_معلومات* userp = getUserp(nodep);
        إذا (!userp) {
            m_allocated.emplace_شا(std::forward<دلیلونه_2>(دلیلونه)...);
            userp = &m_allocated.شا();
            سېټ_Userp(nodep, userp);
        }
        أرجع *userp;
    }

    // Get a reference to the user data
    T_معلومات& چلوونکی()(ثابت T_غوټه* nodep) ثابت {
        T_معلومات* ثابت userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        أرجع *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_معلومات* tryGet(ثابت T_غوټه* nodep) ثابت { أرجع getUserp(nodep); }

    فراغ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
کينډۍ <typename T_غوټه, typename T_معلومات>
class AstUser1Allocator final : عام Ast_User_Allocator_بنسټ<T_غوټه, T_معلومات, 1> {};
کينډۍ <typename T_غوټه, typename T_معلومات>
class AstUser2Allocator final : عام Ast_User_Allocator_بنسټ<T_غوټه, T_معلومات, 2> {};
کينډۍ <typename T_غوټه, typename T_معلومات>
class AstUser3Allocator final : عام Ast_User_Allocator_بنسټ<T_غوټه, T_معلومات, 3> {};
کينډۍ <typename T_غوټه, typename T_معلومات>
class AstUser4Allocator final : عام Ast_User_Allocator_بنسټ<T_غوټه, T_معلومات, 4> {};

#د_که_پای  // Guard
