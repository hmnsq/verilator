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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTUSERALLOCATOR_H_
#تعریف_کرو VERILATOR_V3ASTUSERALLOCATOR_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"

#شامل <type_traits>
#شامل <utility>
#شامل <vector>

سانچہ <typename T_گرہ, typename T_ڈیٹا, صحیح N_UserN>
class Ast_User_Allocator_بنیاد VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "T_Node must be an AstNode type");

نجی:
    std::deque<T_ڈیٹا> m_allocated;

    T_ڈیٹا* getUserp(ثابت T_گرہ* nodep) ثابت {
        اگر VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ثابت VNUser user = nodep->user1u();
            واپس user.مقصد<T_ڈیٹا*>();
        } ورنہ اگر VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ثابت VNUser user = nodep->user2u();
            واپس user.مقصد<T_ڈیٹا*>();
        } ورنہ اگر VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ثابت VNUser user = nodep->user3u();
            واپس user.مقصد<T_ڈیٹا*>();
        } ورنہ {
            ثابت VNUser user = nodep->user4u();
            واپس user.مقصد<T_ڈیٹا*>();
        }
    }

    خالی سیٹ_Userp(T_گرہ* nodep, T_ڈیٹا* userp) ثابت {
        اگر VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } ورنہ اگر VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } ورنہ اگر VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } ورنہ {
            nodep->user4u(VNUser{userp});
        }
    }

محفوظ:
    Ast_User_Allocator_بنیاد() {
        اگر VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_اندر_Use::check();
        } ورنہ اگر VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_اندر_Use::check();
        } ورنہ اگر VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_اندر_Use::check();
        } ورنہ {
            VNUser4_اندر_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_بنیاد);

عوامی:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    سانچہ <typename... دلائل_2>
    T_ڈیٹا& عامل()(T_گرہ* nodep, دلائل_2&&... دلائل) {
        T_ڈیٹا* userp = getUserp(nodep);
        اگر (!userp) {
            m_allocated.emplace_عقب(std::forward<دلائل_2>(دلائل)...);
            userp = &m_allocated.عقب();
            سیٹ_Userp(nodep, userp);
        }
        واپس *userp;
    }

    // Get a reference to the user data
    T_ڈیٹا& عامل()(ثابت T_گرہ* nodep) ثابت {
        T_ڈیٹا* ثابت userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        واپس *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_ڈیٹا* tryGet(ثابت T_گرہ* nodep) ثابت { واپس getUserp(nodep); }

    خالی clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
سانچہ <typename T_گرہ, typename T_ڈیٹا>
class AstUser1Allocator final : عوامی Ast_User_Allocator_بنیاد<T_گرہ, T_ڈیٹا, 1> {};
سانچہ <typename T_گرہ, typename T_ڈیٹا>
class AstUser2Allocator final : عوامی Ast_User_Allocator_بنیاد<T_گرہ, T_ڈیٹا, 2> {};
سانچہ <typename T_گرہ, typename T_ڈیٹا>
class AstUser3Allocator final : عوامی Ast_User_Allocator_بنیاد<T_گرہ, T_ڈیٹا, 3> {};
سانچہ <typename T_گرہ, typename T_ڈیٹا>
class AstUser4Allocator final : عوامی Ast_User_Allocator_بنیاد<T_گرہ, T_ڈیٹا, 4> {};

#اگر_ختم  // Guard
