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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"

#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

template <typename T_ଗଣ୍ଠି, typename T_Data, ପୂର୍ଣ୍ଣସଂଖ୍ୟା N_UserN>
class AstUserAllocatorBase VL_NOT_FINAL {
    static_assert(1 <= N_UserN && N_UserN <= 4, "Wrong user pointer number");
    static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "T_Node must be an AstNode type");

ବ୍ୟକ୍ତିଗତ:
    std::deque<T_Data> m_allocated;

    T_Data* getUserp(ଅଚଳ T_ଗଣ୍ଠି* nodep) ଅଚଳ {
        ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            ଅଚଳ VNUser user = nodep->user1u();
            ଫେରନ୍ତୁ user.to<T_Data*>();
        } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            ଅଚଳ VNUser user = nodep->user2u();
            ଫେରନ୍ତୁ user.to<T_Data*>();
        } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            ଅଚଳ VNUser user = nodep->user3u();
            ଫେରନ୍ତୁ user.to<T_Data*>();
        } ଅନ୍ୟଥା {
            ଅଚଳ VNUser user = nodep->user4u();
            ଫେରନ୍ତୁ user.to<T_Data*>();
        }
    }

    ଶୂନ୍ୟ setUserp(T_ଗଣ୍ଠି* nodep, T_Data* userp) ଅଚଳ {
        ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            nodep->user1u(VNUser{userp});
        } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            nodep->user2u(VNUser{userp});
        } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            nodep->user3u(VNUser{userp});
        } ଅନ୍ୟଥା {
            nodep->user4u(VNUser{userp});
        }
    }

ସୁରକ୍ଷିତ:
    AstUserAllocatorBase() {
        ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 1) {
            VNUser1InUse::check();
        } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 2) {
            VNUser2InUse::check();
        } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (N_UserN == 3) {
            VNUser3InUse::check();
        } ଅନ୍ୟଥା {
            VNUser4InUse::check();
        }
    }

    VL_UNCOPYABLE(AstUserAllocatorBase);

ସର୍ବସାଧାରଣ:
    // Get a reference to the user data. If does not exist, construct it with given arguments.
    template <typename... Args>
    T_Data& operator()(T_ଗଣ୍ଠି* nodep, Args&&... args) {
        T_Data* userp = getUserp(nodep);
        ଯଦି (!userp) {
            m_allocated.emplace_back(std::forward<Args>(args)...);
            userp = &m_allocated.back();
            setUserp(nodep, userp);
        }
        ଫେରନ୍ତୁ *userp;
    }

    // Get a reference to the user data
    T_Data& operator()(ଅଚଳ T_ଗଣ୍ଠି* nodep) ଅଚଳ {
        T_Data* ଅଚଳ userp = getUserp(nodep);
        UASSERT_OBJ(userp, nodep, "Missing User data on const AstNode");
        ଫେରନ୍ତୁ *userp;
    }

    // Get a pointer to the user data if exists, otherwise nullptr
    T_Data* tryGet(ଅଚଳ T_ଗଣ୍ଠି* nodep) ଅଚଳ { ଫେରନ୍ତୁ getUserp(nodep); }

    ଶୂନ୍ୟ clear() { m_allocated.clear(); }
};

// User pointer allocator classes. T_Node is the type of node the allocator should be applied to
// and is there for a bit of extra type safety. T_Data is the type of the data structure
// managed by the allocator.
template <typename T_ଗଣ୍ଠି, typename T_Data>
class AstUser1Allocator final : ସର୍ବସାଧାରଣ AstUserAllocatorBase<T_ଗଣ୍ଠି, T_Data, 1> {};
template <typename T_ଗଣ୍ଠି, typename T_Data>
class AstUser2Allocator final : ସର୍ବସାଧାରଣ AstUserAllocatorBase<T_ଗଣ୍ଠି, T_Data, 2> {};
template <typename T_ଗଣ୍ଠି, typename T_Data>
class AstUser3Allocator final : ସର୍ବସାଧାରଣ AstUserAllocatorBase<T_ଗଣ୍ଠି, T_Data, 3> {};
template <typename T_ଗଣ୍ଠି, typename T_Data>
class AstUser4Allocator final : ସର୍ବସାଧାରଣ AstUserAllocatorBase<T_ଗଣ୍ଠି, T_Data, 4> {};

#endif  // Guard
