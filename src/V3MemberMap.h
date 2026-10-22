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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3MEMBERMAP_H_
#وصف_ڪريو VERILATOR_V3MEMBERMAP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Global.h"

#ضمن <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ڳنڍ*>;
    using ڳنڍ_Map = std::map<ثابت Ast_ڳنڍ*, MemberMap>;
    ڳنڍ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    فراغ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ڳنڍ* ڳوليو_Member(ثابت Ast_ڳنڍ* nodep, ثابت std::string& name) {
        خودڪار nit = m_map.ڳوليو(nodep);
        إذا (VL_UNLIKELY(nit == m_map.پڇاڙي())) {
            جاچ(nodep);
            nit = m_map.ڳوليو(nodep);
        }
        ثابت خودڪار mit = nit->ٻيو.ڳوليو(name);
        إذا (mit == nit->ٻيو.پڇاڙي()) أرجع nullptr;
        أرجع mit->ٻيو;
    }
    // Insert under the given parent node the child's name
    فراغ وجهو(ثابت Ast_ڳنڍ* nodep, Ast_ڳنڍ* childp) {
        خودڪار nit = m_map.ڳوليو(nodep);
        إذا (nit == m_map.پڇاڙي()) {
            جاچ(nodep);
            nit = m_map.ڳوليو(nodep);
        }
        member_وجهو(nit->ٻيو, childp, false);
    }

خاص:
    فراغ جاچ(ثابت Ast_ڳنڍ* nodep) {
        ثابت خودڪار nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.پهريون->ٻيو;
        إذا (ثابت AstClass* ثابت anodep = VN_CAST(nodep, Class)) {
            لكل (Ast_ڳنڍ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_ڳنڍ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        إذا (AstClass::is_Cacheable_ٻار(blockp)) member_وجهو(mmapr, blockp);
                    }
                } وإلا {
                    إذا (AstClass::is_Cacheable_ٻار(itemp)) member_وجهو(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت AstIface* ثابت anodep = VN_CAST(nodep, Iface)) {
            لكل (Ast_ڳنڍ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_ڳنڍ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_وجهو(mmapr, blockp);
                    }
                } وإلا إذا (!VN_IS(itemp, Always)) {
                    member_وجهو(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت anodep
                   = VN_CAST(nodep, ڳنڍ_UOr_Struct_DType)) {
            لكل (Ast_ڳنڍ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_وجهو(mmapr, itemp);
            }
        } وإلا إذا (ثابت AstPackage* ثابت anodep = VN_CAST(nodep, Package)) {
            لكل (Ast_ڳنڍ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_وجهو(mmapr, itemp);
            }
        } وإلا {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    فراغ member_وجهو(MemberMap& mmapr, Ast_ڳنڍ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        إذا (childp->name().خالي_جايون()) أرجع;
        ثابت خودڪار mitPair = mmapr.emplace(childp->name(), childp);
        إذا (VL_UNCOVERABLE(!mitPair.ٻيو && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#جيڪڏهن_پورو  // guard
