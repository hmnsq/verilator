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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Global.h"

#অন্তর্ভাব <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_গ্রন্থি*>;
    using গ্রন্থি_Map = std::map<ধ্রুবক Ast_গ্রন্থি*, MemberMap>;
    গ্রন্থি_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

সর্বজনীন:
    শূন্য clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_গ্রন্থি* findMember(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক std::string& name) {
        auto nit = m_map.find(nodep);
        যদি (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ধ্রুবক auto mit = nit->second.find(name);
        যদি (mit == nit->second.end()) ফেরত nullptr;
        ফেরত mit->second;
    }
    // Insert under the given parent node the child's name
    শূন্য ঢোকাও(ধ্রুবক Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* childp) {
        auto nit = m_map.find(nodep);
        যদি (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_ঢোকাও(nit->second, childp, false);
    }

ব্যক্তিগত:
    শূন্য scan(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ধ্রুবক auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        যদি (ধ্রুবক AstClass* ধ্রুবক anodep = VN_CAST(nodep, Class)) {
            জন্য (Ast_গ্রন্থি* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                যদি (ধ্রুবক AstScope* ধ্রুবক scopep = VN_CAST(itemp, Scope)) {
                    জন্য (Ast_গ্রন্থি* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        যদি (AstClass::isCacheableChild(blockp)) member_ঢোকাও(mmapr, blockp);
                    }
                } অন্যথা {
                    যদি (AstClass::isCacheableChild(itemp)) member_ঢোকাও(mmapr, itemp);
                }
            }
        } অন্যথা যদি (ধ্রুবক AstIface* ধ্রুবক anodep = VN_CAST(nodep, Iface)) {
            জন্য (Ast_গ্রন্থি* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                যদি (ধ্রুবক AstScope* ধ্রুবক scopep = VN_CAST(itemp, Scope)) {
                    জন্য (Ast_গ্রন্থি* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ঢোকাও(mmapr, blockp);
                    }
                } অন্যথা যদি (!VN_IS(itemp, Always)) {
                    member_ঢোকাও(mmapr, itemp);
                }
            }
        } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক anodep
                   = VN_CAST(nodep, গ্রন্থি_UOr_Struct_DType)) {
            জন্য (Ast_গ্রন্থি* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ঢোকাও(mmapr, itemp);
            }
        } অন্যথা যদি (ধ্রুবক AstPackage* ধ্রুবক anodep = VN_CAST(nodep, Package)) {
            জন্য (Ast_গ্রন্থি* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ঢোকাও(mmapr, itemp);
            }
        } অন্যথা {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    শূন্য member_ঢোকাও(MemberMap& mmapr, Ast_গ্রন্থি* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        যদি (childp->name().empty()) ফেরত;
        ধ্রুবক auto mitPair = mmapr.emplace(childp->name(), childp);
        যদি (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
