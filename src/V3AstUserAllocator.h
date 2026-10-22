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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTUSERALLOCATOR_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTUSERALLOCATOR_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

ಮಾದರಿ <typename T_ಗಂಟು, typename T_ದತ್ತಾಂಶ, ಪೂರ್ಣಾಂಕ N_UserN>
class Ast_User_Allocator_ತಳ VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "T_Node must be an AstNode type");

ಖಾಸಗಿ:
    std::deque<T_ದತ್ತಾಂಶ> m_allocated;

    T_ದತ್ತಾಂಶ* getUserp(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ಸ್ಥಿರಾಂಕ VNUser user = nodep->user1u();
            ಹಿಂತಿರುಗಿ user.ಗಮ್ಯ<T_ದತ್ತಾಂಶ*>();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ಸ್ಥಿರಾಂಕ VNUser user = nodep->user2u();
            ಹಿಂತಿರುಗಿ user.ಗಮ್ಯ<T_ದತ್ತಾಂಶ*>();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ಸ್ಥಿರಾಂಕ VNUser user = nodep->user3u();
            ಹಿಂತಿರುಗಿ user.ಗಮ್ಯ<T_ದತ್ತಾಂಶ*>();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ VNUser user = nodep->user4u();
            ಹಿಂತಿರುಗಿ user.ಗಮ್ಯ<T_ದತ್ತಾಂಶ*>();
        }
    }

    ಶೂನ್ಯ ಗಣ_Userp(T_ಗಂಟು* nodep, T_ದತ್ತಾಂಶ* userp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->user4u(VNUser{userp});
        }
    }

ಸಂರಕ್ಷಿತ:
    Ast_User_Allocator_ತಳ() {
        ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_ಒಳ_Use::check();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_ಒಳ_Use::check();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_ಒಳ_Use::check();
        } ಇಲ್ಲದಿದ್ದರೆ {
            VNUser4_ಒಳ_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_ತಳ);

ಸಾರ್ವಜನಿಕ:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    ಮಾದರಿ <typename... ವಾದಗಳು_2>
    T_ದತ್ತಾಂಶ& ಕ್ರಿಯಾಕಾರಕ()(T_ಗಂಟು* nodep, ವಾದಗಳು_2&&... ವಾದಗಳು) {
        T_ದತ್ತಾಂಶ* userp = getUserp(nodep);
        ಆದರೆ (!userp) {
            m_allocated.emplace_ಹಿಂಭಾಗ(std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...);
            userp = &m_allocated.ಹಿಂಭಾಗ();
            ಗಣ_Userp(nodep, userp);
        }
        ಹಿಂತಿರುಗಿ *userp;
    }

    // Get a reference to the user data
    T_ದತ್ತಾಂಶ& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
        T_ದತ್ತಾಂಶ* ಸ್ಥಿರಾಂಕ userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        ಹಿಂತಿರುಗಿ *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_ದತ್ತಾಂಶ* tryGet(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ getUserp(nodep); }

    ಶೂನ್ಯ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
ಮಾದರಿ <typename T_ಗಂಟು, typename T_ದತ್ತಾಂಶ>
class AstUser1Allocator final : ಸಾರ್ವಜನಿಕ Ast_User_Allocator_ತಳ<T_ಗಂಟು, T_ದತ್ತಾಂಶ, 1> {};
ಮಾದರಿ <typename T_ಗಂಟು, typename T_ದತ್ತಾಂಶ>
class AstUser2Allocator final : ಸಾರ್ವಜನಿಕ Ast_User_Allocator_ತಳ<T_ಗಂಟು, T_ದತ್ತಾಂಶ, 2> {};
ಮಾದರಿ <typename T_ಗಂಟು, typename T_ದತ್ತಾಂಶ>
class AstUser3Allocator final : ಸಾರ್ವಜನಿಕ Ast_User_Allocator_ತಳ<T_ಗಂಟು, T_ದತ್ತಾಂಶ, 3> {};
ಮಾದರಿ <typename T_ಗಂಟು, typename T_ದತ್ತಾಂಶ>
class AstUser4Allocator final : ಸಾರ್ವಜನಿಕ Ast_User_Allocator_ತಳ<T_ಗಂಟು, T_ದತ್ತಾಂಶ, 4> {};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
