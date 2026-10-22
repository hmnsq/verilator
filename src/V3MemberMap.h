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

#که_نه_تعریف_شوی VERILATOR_V3MEMBERMAP_H_
#تعریف_کړه VERILATOR_V3MEMBERMAP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Global.h"

#ضمن <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_غوټه*>;
    using غوټه_Map = std::map<ثابت Ast_غوټه*, MemberMap>;
    غوټه_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    فراغ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_غوټه* پیدا_کړه_Member(ثابت Ast_غوټه* nodep, ثابت std::string& name) {
        خپلکاری nit = m_map.پیدا_کړه(nodep);
        إذا (VL_UNLIKELY(nit == m_map.پای_ټکی())) {
            لټون(nodep);
            nit = m_map.پیدا_کړه(nodep);
        }
        ثابت خپلکاری mit = nit->دوهم.پیدا_کړه(name);
        إذا (mit == nit->دوهم.پای_ټکی()) أرجع nullptr;
        أرجع mit->دوهم;
    }
    // Insert under the given parent node the child's name
    فراغ ورننباسه(ثابت Ast_غوټه* nodep, Ast_غوټه* childp) {
        خپلکاری nit = m_map.پیدا_کړه(nodep);
        إذا (nit == m_map.پای_ټکی()) {
            لټون(nodep);
            nit = m_map.پیدا_کړه(nodep);
        }
        member_ورننباسه(nit->دوهم, childp, false);
    }

خاص:
    فراغ لټون(ثابت Ast_غوټه* nodep) {
        ثابت خپلکاری nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.لومړی->دوهم;
        إذا (ثابت AstClass* ثابت anodep = VN_CAST(nodep, Class)) {
            لكل (Ast_غوټه* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_غوټه* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        إذا (AstClass::is_Cacheable_ماشوم(blockp)) member_ورننباسه(mmapr, blockp);
                    }
                } وإلا {
                    إذا (AstClass::is_Cacheable_ماشوم(itemp)) member_ورننباسه(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت AstIface* ثابت anodep = VN_CAST(nodep, Iface)) {
            لكل (Ast_غوټه* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    لكل (Ast_غوټه* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ورننباسه(mmapr, blockp);
                    }
                } وإلا إذا (!VN_IS(itemp, Always)) {
                    member_ورننباسه(mmapr, itemp);
                }
            }
        } وإلا إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت anodep
                   = VN_CAST(nodep, غوټه_UOr_Struct_DType)) {
            لكل (Ast_غوټه* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ورننباسه(mmapr, itemp);
            }
        } وإلا إذا (ثابت AstPackage* ثابت anodep = VN_CAST(nodep, Package)) {
            لكل (Ast_غوټه* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ورننباسه(mmapr, itemp);
            }
        } وإلا {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    فراغ member_ورننباسه(MemberMap& mmapr, Ast_غوټه* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        إذا (childp->name().تشه()) أرجع;
        ثابت خپلکاری mitPair = mmapr.emplace(childp->name(), childp);
        إذا (VL_UNCOVERABLE(!mitPair.دوهم && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#د_که_پای  // guard
