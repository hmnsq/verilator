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

#جے_بنا_تعریف VERILATOR_V3ASTUSERALLOCATOR_H_
#تعریف_کرو VERILATOR_V3ASTUSERALLOCATOR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

#ضمن <type_traits>
#ضمن <utility>
#ضمن <vector>

سانچا <typename T_گرہ, typename T_انکڑے, صحيح N_UserN>
class Ast_User_Allocator_ادھار VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "T_Node must be an AstNode type");

خاص:
    std::deque<T_انکڑے> m_allocated;

    T_انکڑے* getUserp(ثابت T_گرہ* nodep) ثابت {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ثابت VNUser user = nodep->user1u();
            أرجع user.منزل<T_انکڑے*>();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ثابت VNUser user = nodep->user2u();
            أرجع user.منزل<T_انکڑے*>();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ثابت VNUser user = nodep->user3u();
            أرجع user.منزل<T_انکڑے*>();
        } وإلا {
            ثابت VNUser user = nodep->user4u();
            أرجع user.منزل<T_انکڑے*>();
        }
    }

    فراغ سیٹ_Userp(T_گرہ* nodep, T_انکڑے* userp) ثابت {
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
    Ast_User_Allocator_ادھار() {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_وچ_Use::check();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_وچ_Use::check();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_وچ_Use::check();
        } وإلا {
            VNUser4_وچ_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_ادھار);

عام:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    سانچا <typename... دلیلاں_2>
    T_انکڑے& عامل()(T_گرہ* nodep, دلیلاں_2&&... دلیلاں) {
        T_انکڑے* userp = getUserp(nodep);
        إذا (!userp) {
            m_allocated.emplace_پٹھ(std::forward<دلیلاں_2>(دلیلاں)...);
            userp = &m_allocated.پٹھ();
            سیٹ_Userp(nodep, userp);
        }
        أرجع *userp;
    }

    // Get a reference to the user data
    T_انکڑے& عامل()(ثابت T_گرہ* nodep) ثابت {
        T_انکڑے* ثابت userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        أرجع *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_انکڑے* tryGet(ثابت T_گرہ* nodep) ثابت { أرجع getUserp(nodep); }

    فراغ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
سانچا <typename T_گرہ, typename T_انکڑے>
class AstUser1Allocator final : عام Ast_User_Allocator_ادھار<T_گرہ, T_انکڑے, 1> {};
سانچا <typename T_گرہ, typename T_انکڑے>
class AstUser2Allocator final : عام Ast_User_Allocator_ادھار<T_گرہ, T_انکڑے, 2> {};
سانچا <typename T_گرہ, typename T_انکڑے>
class AstUser3Allocator final : عام Ast_User_Allocator_ادھار<T_گرہ, T_انکڑے, 3> {};
سانچا <typename T_گرہ, typename T_انکڑے>
class AstUser4Allocator final : عام Ast_User_Allocator_ادھار<T_گرہ, T_انکڑے, 4> {};

#جے_انت  // Guard
