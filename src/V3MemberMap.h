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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Global.h"

#شامل <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_گره*>;
    using گره_Map = std::map<ثابت Ast_گره*, MemberMap>;
    گره_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

عمومی:
    تهی clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_گره* findMember(ثابت Ast_گره* nodep, ثابت std::string& name) {
        auto nit = m_map.find(nodep);
        اگر (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ثابت auto mit = nit->second.find(name);
        اگر (mit == nit->second.end()) بازگشت nullptr;
        بازگشت mit->second;
    }
    // Insert under the given parent node the child's name
    تهی درج(ثابت Ast_گره* nodep, Ast_گره* childp) {
        auto nit = m_map.find(nodep);
        اگر (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_درج(nit->second, childp, false);
    }

خصوصی:
    تهی scan(ثابت Ast_گره* nodep) {
        ثابت auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        اگر (ثابت AstClass* ثابت anodep = VN_CAST(nodep, Class)) {
            برای (Ast_گره* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                اگر (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    برای (Ast_گره* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        اگر (AstClass::isCacheableChild(blockp)) member_درج(mmapr, blockp);
                    }
                } وگرنه {
                    اگر (AstClass::isCacheableChild(itemp)) member_درج(mmapr, itemp);
                }
            }
        } وگرنه اگر (ثابت AstIface* ثابت anodep = VN_CAST(nodep, Iface)) {
            برای (Ast_گره* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                اگر (ثابت AstScope* ثابت scopep = VN_CAST(itemp, Scope)) {
                    برای (Ast_گره* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_درج(mmapr, blockp);
                    }
                } وگرنه اگر (!VN_IS(itemp, Always)) {
                    member_درج(mmapr, itemp);
                }
            }
        } وگرنه اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت anodep
                   = VN_CAST(nodep, گره_UOr_Struct_DType)) {
            برای (Ast_گره* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_درج(mmapr, itemp);
            }
        } وگرنه اگر (ثابت AstPackage* ثابت anodep = VN_CAST(nodep, Package)) {
            برای (Ast_گره* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_درج(mmapr, itemp);
            }
        } وگرنه {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    تهی member_درج(MemberMap& mmapr, Ast_گره* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        اگر (childp->name().empty()) بازگشت;
        ثابت auto mitPair = mmapr.emplace(childp->name(), childp);
        اگر (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
