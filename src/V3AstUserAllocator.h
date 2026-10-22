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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTUSERALLOCATOR_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTUSERALLOCATOR_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

ਸਾਂਚਾ <typename T_ਗੰਢ, typename T_ਅੰਕੜੇ, ਪੂਰਨ N_UserN>
class Ast_User_Allocator_ਅਧਾਰ VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "T_Node must be an AstNode type");

ਨਿਜੀ:
    std::deque<T_ਅੰਕੜੇ> m_allocated;

    T_ਅੰਕੜੇ* getUserp(ਅਚਲ T_ਗੰਢ* nodep) ਅਚਲ {
        ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ਅਚਲ VNUser user = nodep->user1u();
            ਵਾਪਸ user.ਮੰਜ਼ਿਲ<T_ਅੰਕੜੇ*>();
        } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ਅਚਲ VNUser user = nodep->user2u();
            ਵਾਪਸ user.ਮੰਜ਼ਿਲ<T_ਅੰਕੜੇ*>();
        } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ਅਚਲ VNUser user = nodep->user3u();
            ਵਾਪਸ user.ਮੰਜ਼ਿਲ<T_ਅੰਕੜੇ*>();
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ VNUser user = nodep->user4u();
            ਵਾਪਸ user.ਮੰਜ਼ਿਲ<T_ਅੰਕੜੇ*>();
        }
    }

    ਖਾਲੀ ਸੈੱਟ_Userp(T_ਗੰਢ* nodep, T_ਅੰਕੜੇ* userp) ਅਚਲ {
        ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } ਨਹੀਂਤਾਂ {
            nodep->user4u(VNUser{userp});
        }
    }

ਸੁਰੱਖਿਅਤ:
    Ast_User_Allocator_ਅਧਾਰ() {
        ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_ਵਿੱਚ_Use::check();
        } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_ਵਿੱਚ_Use::check();
        } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_ਵਿੱਚ_Use::check();
        } ਨਹੀਂਤਾਂ {
            VNUser4_ਵਿੱਚ_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_ਅਧਾਰ);

ਜਨਤਕ:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    ਸਾਂਚਾ <typename... ਦਲੀਲਾਂ_2>
    T_ਅੰਕੜੇ& ਸੰਚਾਲਕ()(T_ਗੰਢ* nodep, ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        T_ਅੰਕੜੇ* userp = getUserp(nodep);
        ਜੇ (!userp) {
            m_allocated.emplace_ਪਿੱਠ(std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...);
            userp = &m_allocated.ਪਿੱਠ();
            ਸੈੱਟ_Userp(nodep, userp);
        }
        ਵਾਪਸ *userp;
    }

    // Get a reference to the user data
    T_ਅੰਕੜੇ& ਸੰਚਾਲਕ()(ਅਚਲ T_ਗੰਢ* nodep) ਅਚਲ {
        T_ਅੰਕੜੇ* ਅਚਲ userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        ਵਾਪਸ *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_ਅੰਕੜੇ* tryGet(ਅਚਲ T_ਗੰਢ* nodep) ਅਚਲ { ਵਾਪਸ getUserp(nodep); }

    ਖਾਲੀ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
ਸਾਂਚਾ <typename T_ਗੰਢ, typename T_ਅੰਕੜੇ>
class AstUser1Allocator final : ਜਨਤਕ Ast_User_Allocator_ਅਧਾਰ<T_ਗੰਢ, T_ਅੰਕੜੇ, 1> {};
ਸਾਂਚਾ <typename T_ਗੰਢ, typename T_ਅੰਕੜੇ>
class AstUser2Allocator final : ਜਨਤਕ Ast_User_Allocator_ਅਧਾਰ<T_ਗੰਢ, T_ਅੰਕੜੇ, 2> {};
ਸਾਂਚਾ <typename T_ਗੰਢ, typename T_ਅੰਕੜੇ>
class AstUser3Allocator final : ਜਨਤਕ Ast_User_Allocator_ਅਧਾਰ<T_ਗੰਢ, T_ਅੰਕੜੇ, 3> {};
ਸਾਂਚਾ <typename T_ਗੰਢ, typename T_ਅੰਕੜੇ>
class AstUser4Allocator final : ਜਨਤਕ Ast_User_Allocator_ਅਧਾਰ<T_ਗੰਢ, T_ਅੰਕੜੇ, 4> {};

#ਜੇ_ਅੰਤ  // Guard
