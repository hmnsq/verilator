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

#اگر_غیر_تعریف_شدہ VERILATOR_V3MEMBERMAP_H_
#تعریف_کرو VERILATOR_V3MEMBERMAP_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Global.h"

#شامل <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_گرہ*>;
    using گرہ_Map = std::map<ثابت Ast_گرہ*, MemberMap>;
    گرہ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

عوامی:
    خالی clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_گرہ* ڈھونڈو_Member(ثابت Ast_گرہ* nodep, ثابت std::string& name) {
        خودکار nit = m_map.ڈھونڈو(nodep);
        اگر (VL_UNLIKELY(nit == m_map.انتہا())) {
            جائزہ(nodep);
            nit = m_map.ڈھونڈو(nodep);
        }
        ثابت خودکار mit = nit->دوسرا.ڈھونڈو(name);
        اگر (mit == nit->دوسرا.انتہا()) واپس nullptr;
        واپس mit->دوسرا;
    }
    // Insert under the given parent node the child's name
    خالی داخل_کرو(ثابت Ast_گرہ* nodep, Ast_گرہ* childp) {
        خودکار nit = m_map.ڈھونڈو(nodep);
        اگر (nit == m_map.انتہا()) {
            جائزہ(nodep);
            nit = m_map.ڈھونڈو(nodep);
        }
        member_داخل_کرو(nit->دوسرا, childp, false);
    }

نجی:
    خالی جائزہ(ثابت Ast_گرہ* nodep) {
        ثابت خودکار nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.پہلا->دوسرا;
        اگر (ثابت AstClass* ثابت anodep = VN_CAST(nodep, Class)) {
            برائے (Ast_گرہ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                اگر (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    برائے (Ast_گرہ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        اگر (AstClass::is_Cacheable_بچہ(blockp)) member_داخل_کرو(mmapr, blockp);
                    }
                } ورنہ {
                    اگر (AstClass::is_Cacheable_بچہ(itemp)) member_داخل_کرو(mmapr, itemp);
                }
            }
        } ورنہ اگر (ثابت AstIface* ثابت anodep = VN_CAST(nodep, Iface)) {
            برائے (Ast_گرہ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                اگر (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    برائے (Ast_گرہ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_داخل_کرو(mmapr, blockp);
                    }
                } ورنہ اگر (!VN_IS(itemp, Always)) {
                    member_داخل_کرو(mmapr, itemp);
                }
            }
        } ورنہ اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت anodep
                   = VN_CAST(nodep, گرہ_UOr_Struct_DType)) {
            برائے (Ast_گرہ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_داخل_کرو(mmapr, itemp);
            }
        } ورنہ اگر (ثابت AstPackage* ثابت anodep = VN_CAST(nodep, Package)) {
            برائے (Ast_گرہ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_داخل_کرو(mmapr, itemp);
            }
        } ورنہ {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    خالی member_داخل_کرو(MemberMap& mmapr, Ast_گرہ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        اگر (childp->name().خالی_خانے()) واپس;
        ثابت خودکار mitPair = mmapr.emplace(childp->name(), childp);
        اگر (VL_UNCOVERABLE(!mitPair.دوسرا && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#اگر_ختم  // guard
