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

#अगर_अघोषित VERILATOR_V3ASTUSERALLOCATOR_H_
#घोषणा VERILATOR_V3ASTUSERALLOCATOR_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"

#समावेश <type_traits>
#समावेश <utility>
#समावेश <vector>

ढाँचा <typename T_गाँठ, typename T_तथ्य, पूर्णांक N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "T_Node must be an AstNode type");

निजी:
    std::deque<T_तथ्य> m_allocated;

    T_तथ्य* getUserp(स्थिर T_गाँठ* nodep) स्थिर {
        अगर VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            स्थिर VNUser user = nodep->user1u();
            वापस user.to<T_तथ्य*>();
        } अन्यथा अगर VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            स्थिर VNUser user = nodep->user2u();
            वापस user.to<T_तथ्य*>();
        } अन्यथा अगर VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            स्थिर VNUser user = nodep->user3u();
            वापस user.to<T_तथ्य*>();
        } अन्यथा {
            स्थिर VNUser user = nodep->user4u();
            वापस user.to<T_तथ्य*>();
        }
    }

    व्योम setUserp(T_गाँठ* nodep, T_तथ्य* userp) स्थिर {
        अगर VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } अन्यथा अगर VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } अन्यथा अगर VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } अन्यथा {
            nodep->user4u(VNUser{userp});
        }
    }

रक्षित:
    AstUserAllocatorBase() {
        अगर VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } अन्यथा अगर VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } अन्यथा अगर VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } अन्यथा {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

खुला:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    ढाँचा <typename... Args>
    T_तथ्य& चालक()(T_गाँठ* nodep, Args&&... args) {
        T_तथ्य* userp = getUserp(nodep);
        अगर (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        वापस *userp;
    }

    // Get a reference to the user data
    T_तथ्य& चालक()(स्थिर T_गाँठ* nodep) स्थिर {
        T_तथ्य* स्थिर userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        वापस *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_तथ्य* tryGet(स्थिर T_गाँठ* nodep) स्थिर { वापस getUserp(nodep); }

    व्योम clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
ढाँचा <typename T_गाँठ, typename T_तथ्य>
class AstUser1Allocator final : खुला AstUserAllocatorBase<T_गाँठ, T_तथ्य, 1> {};
ढाँचा <typename T_गाँठ, typename T_तथ्य>
class AstUser2Allocator final : खुला AstUserAllocatorBase<T_गाँठ, T_तथ्य, 2> {};
ढाँचा <typename T_गाँठ, typename T_तथ्य>
class AstUser3Allocator final : खुला AstUserAllocatorBase<T_गाँठ, T_तथ्य, 3> {};
ढाँचा <typename T_गाँठ, typename T_तथ्य>
class AstUser4Allocator final : खुला AstUserAllocatorBase<T_गाँठ, T_तथ्य, 4> {};

#पूर्ण_अगर  // Guard
