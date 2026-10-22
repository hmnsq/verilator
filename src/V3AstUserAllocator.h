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

#यदि_अपरिभाषितम् VERILATOR_V3ASTUSERALLOCATOR_H_
#परिभाषय VERILATOR_V3ASTUSERALLOCATOR_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

#अन्तर्भाव <type_traits>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

प्रतिरूपम् <typename T_ग्रन्थिः, typename T_दत्त, पूर्णाङ्क N_UserN>
class Ast_User_Allocator_आधारः VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_आधारः_सम्बन्धि<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "T_Node must be an AstNode type");

गुप्त:
    std::deque<T_दत्त> m_allocated;

    T_दत्त* getUserp(ध्रुव T_ग्रन्थिः* nodep) ध्रुव {
        यदि VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ध्रुव VNUser user = nodep->user1u();
            प्रत्यागम user.गन्तव्यम्<T_दत्त*>();
        } अन्यथा यदि VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ध्रुव VNUser user = nodep->user2u();
            प्रत्यागम user.गन्तव्यम्<T_दत्त*>();
        } अन्यथा यदि VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ध्रुव VNUser user = nodep->user3u();
            प्रत्यागम user.गन्तव्यम्<T_दत्त*>();
        } अन्यथा {
            ध्रुव VNUser user = nodep->user4u();
            प्रत्यागम user.गन्तव्यम्<T_दत्त*>();
        }
    }

    शून्य समुच्चयः_Userp(T_ग्रन्थिः* nodep, T_दत्त* userp) ध्रुव {
        यदि VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } अन्यथा यदि VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } अन्यथा यदि VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } अन्यथा {
            nodep->user4u(VNUser{userp});
        }
    }

रक्षित:
    Ast_User_Allocator_आधारः() {
        यदि VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1_अन्तर्_Use::check();
        } अन्यथा यदि VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2_अन्तर्_Use::check();
        } अन्यथा यदि VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3_अन्तर्_Use::check();
        } अन्यथा {
            VNUser4_अन्तर्_Use::check();
        }
    }

    VL_UNCOPYABLE(Ast_User_Allocator_आधारः);

सार्वजनिक:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    प्रतिरूपम् <typename... तर्काः_2>
    T_दत्त& संक्रियकः()(T_ग्रन्थिः* nodep, तर्काः_2&&... तर्काः) {
        T_दत्त* userp = getUserp(nodep);
        यदि (!userp) {
            m_allocated.emplace_पृष्ठभागः(std::forward<तर्काः_2>(तर्काः)...);
            userp = &m_allocated.पृष्ठभागः();
            समुच्चयः_Userp(nodep, userp);
        }
        प्रत्यागम *userp;
    }

    // Get a reference to the user data
    T_दत्त& संक्रियकः()(ध्रुव T_ग्रन्थिः* nodep) ध्रुव {
        T_दत्त* ध्रुव userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        प्रत्यागम *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_दत्त* tryGet(ध्रुव T_ग्रन्थिः* nodep) ध्रुव { प्रत्यागम getUserp(nodep); }

    शून्य clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
प्रतिरूपम् <typename T_ग्रन्थिः, typename T_दत्त>
class AstUser1Allocator final : सार्वजनिक Ast_User_Allocator_आधारः<T_ग्रन्थिः, T_दत्त, 1> {};
प्रतिरूपम् <typename T_ग्रन्थिः, typename T_दत्त>
class AstUser2Allocator final : सार्वजनिक Ast_User_Allocator_आधारः<T_ग्रन्थिः, T_दत्त, 2> {};
प्रतिरूपम् <typename T_ग्रन्थिः, typename T_दत्त>
class AstUser3Allocator final : सार्वजनिक Ast_User_Allocator_आधारः<T_ग्रन्थिः, T_दत्त, 3> {};
प्रतिरूपम् <typename T_ग्रन्थिः, typename T_दत्त>
class AstUser4Allocator final : सार्वजनिक Ast_User_Allocator_आधारः<T_ग्रन्थिः, T_दत्त, 4> {};

#यदि_अन्तः  // Guard
