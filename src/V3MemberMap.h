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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Global.h"

#ضمن <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_كلي*>;
    using كلي_Map = std::map<ثابت Ast_كلي*, MemberMap>;
    كلي_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    فراغ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_كلي* findMember(ثابت Ast_كلي* nodep, ثابت std::string& name) {
        auto nit = m_map.find(nodep);
        إذا (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ثابت auto mit = nit->second.find(name);
        إذا (mit == nit->second.end()) أرجع nullptr;
        أرجع mit->second;
    }
    // Insert under the given parent node the child's name
    فراغ سك(ثابت Ast_كلي* nodep, Ast_كلي* childp) {
        auto nit = m_map.find(nodep);
        إذا (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_سك(nit->second, childp, false);
    }

خاص:
    فراغ scan(ثابت Ast_كلي* nodep) {
        ثابت auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        إذا (ثابت AstClass* ثابت anodep = VN_CAST(nodep, Class)) {
            لكل (Ast_كلي* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_كلي* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        إذا (AstClass::isCacheableChild(blockp)) member_سك(mmapr, blockp);
                    }
                } وإلا {
                    إذا (AstClass::isCacheableChild(itemp)) member_سك(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت AstIface* ثابت anodep = VN_CAST(nodep, Iface)) {
            لكل (Ast_كلي* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_كلي* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_سك(mmapr, blockp);
                    }
                } وإلا إذا (!VN_IS(itemp, Always)) {
                    member_سك(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت Ast_كلي_UOr_Struct_DType* ثابت anodep
                   = VN_CAST(nodep, كلي_UOr_Struct_DType)) {
            لكل (Ast_كلي* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_سك(mmapr, itemp);
            }
        } وإلا إذا (ثابت AstPackage* ثابت anodep = VN_CAST(nodep, Package)) {
            لكل (Ast_كلي* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_سك(mmapr, itemp);
            }
        } وإلا {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    فراغ member_سك(MemberMap& mmapr, Ast_كلي* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        إذا (childp->name().empty()) أرجع;
        ثابت auto mitPair = mmapr.emplace(childp->name(), childp);
        إذا (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
