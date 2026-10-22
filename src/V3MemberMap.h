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

#جے_بنا_تعریف VERILATOR_V3MEMBERMAP_H_
#تعریف_کرو VERILATOR_V3MEMBERMAP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Global.h"

#ضمن <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_گرہ*>;
    using گرہ_Map = std::map<ثابت Ast_گرہ*, MemberMap>;
    گرہ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    فراغ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_گرہ* لبھو_Member(ثابت Ast_گرہ* nodep, ثابت std::string& name) {
        خودکار nit = m_map.لبھو(nodep);
        إذا (VL_UNLIKELY(nit == m_map.سرا())) {
            جانچ(nodep);
            nit = m_map.لبھو(nodep);
        }
        ثابت خودکار mit = nit->دوجا.لبھو(name);
        إذا (mit == nit->دوجا.سرا()) أرجع nullptr;
        أرجع mit->دوجا;
    }
    // Insert under the given parent node the child's name
    فراغ داخل_کرو(ثابت Ast_گرہ* nodep, Ast_گرہ* childp) {
        خودکار nit = m_map.لبھو(nodep);
        إذا (nit == m_map.سرا()) {
            جانچ(nodep);
            nit = m_map.لبھو(nodep);
        }
        member_داخل_کرو(nit->دوجا, childp, false);
    }

خاص:
    فراغ جانچ(ثابت Ast_گرہ* nodep) {
        ثابت خودکار nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.پہلا->دوجا;
        إذا (ثابت AstClass* ثابت anodep = VN_CAST(nodep, Class)) {
            لكل (Ast_گرہ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_گرہ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        إذا (AstClass::is_Cacheable_بچہ(blockp)) member_داخل_کرو(mmapr, blockp);
                    }
                } وإلا {
                    إذا (AstClass::is_Cacheable_بچہ(itemp)) member_داخل_کرو(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت AstIface* ثابت anodep = VN_CAST(nodep, Iface)) {
            لكل (Ast_گرہ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_گرہ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_داخل_کرو(mmapr, blockp);
                    }
                } وإلا إذا (!VN_IS(itemp, Always)) {
                    member_داخل_کرو(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت anodep
                   = VN_CAST(nodep, گرہ_UOr_Struct_DType)) {
            لكل (Ast_گرہ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_داخل_کرو(mmapr, itemp);
            }
        } وإلا إذا (ثابت AstPackage* ثابت anodep = VN_CAST(nodep, Package)) {
            لكل (Ast_گرہ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_داخل_کرو(mmapr, itemp);
            }
        } وإلا {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    فراغ member_داخل_کرو(MemberMap& mmapr, Ast_گرہ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        إذا (childp->name().خالی_تھاواں()) أرجع;
        ثابت خودکار mitPair = mmapr.emplace(childp->name(), childp);
        إذا (VL_UNCOVERABLE(!mitPair.دوجا && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#جے_انت  // guard
