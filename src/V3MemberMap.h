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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Global.h"

#অন্তর্ভাৱ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_গ্ৰন্থি*>;
    using গ্ৰন্থি_Map = std::map<ধ্রুৱ Ast_গ্ৰন্থি*, MemberMap>;
    গ্ৰন্থি_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

সার্ৱজনিক:
    শূন্য clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_গ্ৰন্থি* findMember(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ std::string& name) {
        auto nit = m_map.find(nodep);
        যদি (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ধ্রুৱ auto mit = nit->second.find(name);
        যদি (mit == nit->second.end()) ঘূৰাই nullptr;
        ঘূৰাই mit->second;
    }
    // Insert under the given parent node the child's name
    শূন্য ঢোকাও(ধ্রুৱ Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* childp) {
        auto nit = m_map.find(nodep);
        যদি (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_ঢোকাও(nit->second, childp, false);
    }

গুপ্ত:
    শূন্য scan(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        ধ্রুৱ auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        যদি (ধ্রুৱ AstClass* ধ্রুৱ anodep = VN_CAST(nodep, Class)) {
            বাবে (Ast_গ্ৰন্থি* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                যদি (ধ্রুৱ AstScope* ধ্রুৱ scopep = VN_CAST(itemp, Scope)) {
                    বাবে (Ast_গ্ৰন্থি* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        যদি (AstClass::isCacheableChild(blockp)) member_ঢোকাও(mmapr, blockp);
                    }
                } অন্যথা {
                    যদি (AstClass::isCacheableChild(itemp)) member_ঢোকাও(mmapr, itemp);
                }
            }
        } অন্যথা যদি (ধ্রুৱ AstIface* ধ্রুৱ anodep = VN_CAST(nodep, Iface)) {
            বাবে (Ast_গ্ৰন্থি* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                যদি (ধ্রুৱ AstScope* ধ্রুৱ scopep = VN_CAST(itemp, Scope)) {
                    বাবে (Ast_গ্ৰন্থি* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ঢোকাও(mmapr, blockp);
                    }
                } অন্যথা যদি (!VN_IS(itemp, Always)) {
                    member_ঢোকাও(mmapr, itemp);
                }
            }
        } অন্যথা যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ anodep
                   = VN_CAST(nodep, গ্ৰন্থি_UOr_Struct_DType)) {
            বাবে (Ast_গ্ৰন্থি* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ঢোকাও(mmapr, itemp);
            }
        } অন্যথা যদি (ধ্রুৱ AstPackage* ধ্রুৱ anodep = VN_CAST(nodep, Package)) {
            বাবে (Ast_গ্ৰন্থি* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ঢোকাও(mmapr, itemp);
            }
        } অন্যথা {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    শূন্য member_ঢোকাও(MemberMap& mmapr, Ast_গ্ৰন্থি* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        যদি (childp->name().empty()) ঘূৰাই;
        ধ্রুৱ auto mitPair = mmapr.emplace(childp->name(), childp);
        যদি (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
