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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTUSERALLOCATOR_H_
#నిర్వచించు VERILATOR_V3ASTUSERALLOCATOR_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"

#అన్తర్భావ <type_traits>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

మూస <typename T_కణుపు, typename T_దత్తాంశం, పూర్ణాంకం N_UserN>
class Ast_User_Allocator_పునాది VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "T_Node must be an AstNode type");

అంతరంగ:
    std::deque<T_దత్తాంశం> m_allocated;

    T_దత్తాంశం* getUserp(స్థిరరాశి T_కణుపు* nodep) స్థిరరాశి {
        ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            స్థిరరాశి VNUser user = nodep->user1u();
            తిరిగి user.గమ్యం<T_దత్తాంశం*>();
        } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            స్థిరరాశి VNUser user = nodep->user2u();
            తిరిగి user.గమ్యం<T_దత్తాంశం*>();
        } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            స్థిరరాశి VNUser user = nodep->user3u();
            తిరిగి user.గమ్యం<T_దత్తాంశం*>();
        } లేకపోతే {
            స్థిరరాశి VNUser user = nodep->user4u();
            తిరిగి user.గమ్యం<T_దత్తాంశం*>();
        }
    }

    శూన్య సమితి_Userp(T_కణుపు* nodep, T_దత్తాంశం* userp) స్థిరరాశి {
        ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } లేకపోతే {
            nodep->user4u(VNUser{userp});
        }
    }

రక్షిత:
    Ast_User_Allocator_పునాది() {
        ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_లోపల_Use::check();
        } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_లోపల_Use::check();
        } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_లోపల_Use::check();
        } లేకపోతే {
            VNUser4_లోపల_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_పునాది);

బహిరంగ:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    మూస <typename... వాదనలు_2>
    T_దత్తాంశం& క్రియాకారకం()(T_కణుపు* nodep, వాదనలు_2&&... వాదనలు) {
        T_దత్తాంశం* userp = getUserp(nodep);
        ఒకవేళ (!userp) {
            m_allocated.emplace_వెనుక(std::forward<వాదనలు_2>(వాదనలు)...);
            userp = &m_allocated.వెనుక();
            సమితి_Userp(nodep, userp);
        }
        తిరిగి *userp;
    }

    // Get a reference to the user data
    T_దత్తాంశం& క్రియాకారకం()(స్థిరరాశి T_కణుపు* nodep) స్థిరరాశి {
        T_దత్తాంశం* స్థిరరాశి userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        తిరిగి *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_దత్తాంశం* tryGet(స్థిరరాశి T_కణుపు* nodep) స్థిరరాశి { తిరిగి getUserp(nodep); }

    శూన్య clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
మూస <typename T_కణుపు, typename T_దత్తాంశం>
class AstUser1Allocator final : బహిరంగ Ast_User_Allocator_పునాది<T_కణుపు, T_దత్తాంశం, 1> {};
మూస <typename T_కణుపు, typename T_దత్తాంశం>
class AstUser2Allocator final : బహిరంగ Ast_User_Allocator_పునాది<T_కణుపు, T_దత్తాంశం, 2> {};
మూస <typename T_కణుపు, typename T_దత్తాంశం>
class AstUser3Allocator final : బహిరంగ Ast_User_Allocator_పునాది<T_కణుపు, T_దత్తాంశం, 3> {};
మూస <typename T_కణుపు, typename T_దత్తాంశం>
class AstUser4Allocator final : బహిరంగ Ast_User_Allocator_పునాది<T_కణుపు, T_దత్తాంశం, 4> {};

#అయితే_ముగింపు  // Guard
