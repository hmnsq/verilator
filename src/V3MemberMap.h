// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Member names for classes/structs
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
//
// For a class or struct, create a map of names of each member.
// Used for faster lookup to avoid O(n^2) processing times.
//
//*************************************************************************

#ifndef VERILATOR_V3MEMBERMAP_H_
#define VERILATOR_V3MEMBERMAP_H_

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Global.h"

#include <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, AstNode*>;
    using NodeMap = std::map<const AstNode*, MemberMap>;
    NodeMap m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

public:
    void clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    AstNode* findMember(const AstNode* nodep, const std::string& name) {
        auto nit = m_map.find(nodep);
        ပေႃး (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        const auto mit = nit->second.find(name);
        ပေႃး (mit == nit->second.end()) ပွၵ်ႈ nullptr;
        ပွၵ်ႈ mit->second;
    }
    // Insert under the given parent node the child's name
    void insert(const AstNode* nodep, AstNode* childp) {
        auto nit = m_map.find(nodep);
        ပေႃး (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        memberInsert(nit->second, childp, false);
    }

private:
    void scan(const AstNode* nodep) {
        const auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        ပေႃး (const AstClass* const anodep = VN_CAST(nodep, Class)) {
            for (AstNode* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                ပေႃး (const AstScope* const scopep = VN_CAST(itemp, Scope)) {
                    for (AstNode* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        ပေႃး (AstClass::isCacheableChild(blockp)) memberInsert(mmapr, blockp);
                    }
                } ပေႃးဢမ်ႇ {
                    ပေႃး (AstClass::isCacheableChild(itemp)) memberInsert(mmapr, itemp);
                }
            }
        } ပေႃးဢမ်ႇ ပေႃး (const AstIface* const anodep = VN_CAST(nodep, Iface)) {
            for (AstNode* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ပေႃး (const AstScope* const scopep = VN_CAST(itemp, Scope)) {
                    for (AstNode* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        memberInsert(mmapr, blockp);
                    }
                } ပေႃးဢမ်ႇ ပေႃး (!VN_IS(itemp, Always)) {
                    memberInsert(mmapr, itemp);
                }
            }
        } ပေႃးဢမ်ႇ ပေႃး (const AstNodeUOrStructDType* const anodep
                   = VN_CAST(nodep, NodeUOrStructDType)) {
            for (AstNode* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                memberInsert(mmapr, itemp);
            }
        } ပေႃးဢမ်ႇ ပေႃး (const AstPackage* const anodep = VN_CAST(nodep, Package)) {
            for (AstNode* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                memberInsert(mmapr, itemp);
            }
        } ပေႃးဢမ်ႇ {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    void memberInsert(MemberMap& mmapr, AstNode* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        ပေႃး (childp->name().empty()) ပွၵ်ႈ;
        const auto mitPair = mmapr.emplace(childp->name(), childp);
        ပေႃး (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
