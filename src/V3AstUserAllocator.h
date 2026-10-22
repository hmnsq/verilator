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

#اگر_تعریف_نشده VERILATOR_V3ASTUSERALLOCATOR_H_
#تعریف_کن VERILATOR_V3ASTUSERALLOCATOR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

#ضمن <type_traits>
#ضمن <utility>
#ضمن <vector>

قالب <typename T_گره, typename T_داده, صحيح N_UserN>
class Ast_User_Allocator_پایه VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_پایه_از<Ast_گره, T_گره>::مقدار, "T_Node must be an AstNode type");

خاص:
    std::deque<T_داده> m_allocated;

    T_داده* getUserp(ثابت T_گره* nodep) ثابت {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ثابت VNUser user = nodep->user1u();
            أرجع user.مقصد<T_داده*>();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ثابت VNUser user = nodep->user2u();
            أرجع user.مقصد<T_داده*>();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ثابت VNUser user = nodep->user3u();
            أرجع user.مقصد<T_داده*>();
        } وإلا {
            ثابت VNUser user = nodep->user4u();
            أرجع user.مقصد<T_داده*>();
        }
    }

    فراغ مجموعه_Userp(T_گره* nodep, T_داده* userp) ثابت {
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
    Ast_User_Allocator_پایه() {
        إذا VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_درون_Use::check();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_درون_Use::check();
        } وإلا إذا VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_درون_Use::check();
        } وإلا {
            VNUser4_درون_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_پایه);

عام:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    قالب <typename... آرگومانها_2>
    T_داده& عملگر()(T_گره* nodep, آرگومانها_2&&... آرگومانها) {
        T_داده* userp = getUserp(nodep);
        إذا (!userp) {
            m_allocated.emplace_پشت(std::forward<آرگومانها_2>(آرگومانها)...);
            userp = &m_allocated.پشت();
            مجموعه_Userp(nodep, userp);
        }
        أرجع *userp;
    }

    // Get a reference to the user data
    T_داده& عملگر()(ثابت T_گره* nodep) ثابت {
        T_داده* ثابت userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        أرجع *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_داده* tryGet(ثابت T_گره* nodep) ثابت { أرجع getUserp(nodep); }

    فراغ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
قالب <typename T_گره, typename T_داده>
class AstUser1Allocator final : عام Ast_User_Allocator_پایه<T_گره, T_داده, 1> {};
قالب <typename T_گره, typename T_داده>
class AstUser2Allocator final : عام Ast_User_Allocator_پایه<T_گره, T_داده, 2> {};
قالب <typename T_گره, typename T_داده>
class AstUser3Allocator final : عام Ast_User_Allocator_پایه<T_گره, T_داده, 3> {};
قالب <typename T_گره, typename T_داده>
class AstUser4Allocator final : عام Ast_User_Allocator_پایه<T_گره, T_داده, 4> {};

#پایان_اگر  // Guard
