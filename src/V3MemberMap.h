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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Global.h"

#अन्तर्भाव <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ग्रन्थिः*>;
    using ग्रन्थिः_Map = std::map<ध्रुव Ast_ग्रन्थिः*, MemberMap>;
    ग्रन्थिः_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

सार्वजनिक:
    शून्य clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ग्रन्थिः* findMember(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव std::string& name) {
        auto nit = m_map.find(nodep);
        जइ (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ध्रुव auto mit = nit->second.find(name);
        जइ (mit == nit->second.end()) प्रत्यागम nullptr;
        प्रत्यागम mit->second;
    }
    // Insert under the given parent node the child's name
    शून्य निवेशय(ध्रुव Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* childp) {
        auto nit = m_map.find(nodep);
        जइ (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_निवेशय(nit->second, childp, false);
    }

गुप्त:
    शून्य scan(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        जइ (ध्रुव AstClass* ध्रुव anodep = VN_CAST(nodep, Class)) {
            प्रति (Ast_ग्रन्थिः* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                जइ (ध्रुव AstScope* ध्रुव scopep = VN_CAST(itemp, Scope)) {
                    प्रति (Ast_ग्रन्थिः* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        जइ (AstClass::isCacheableChild(blockp)) member_निवेशय(mmapr, blockp);
                    }
                } अण्णहा {
                    जइ (AstClass::isCacheableChild(itemp)) member_निवेशय(mmapr, itemp);
                }
            }
        } अण्णहा जइ (ध्रुव AstIface* ध्रुव anodep = VN_CAST(nodep, Iface)) {
            प्रति (Ast_ग्रन्थिः* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                जइ (ध्रुव AstScope* ध्रुव scopep = VN_CAST(itemp, Scope)) {
                    प्रति (Ast_ग्रन्थिः* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_निवेशय(mmapr, blockp);
                    }
                } अण्णहा जइ (!VN_IS(itemp, Always)) {
                    member_निवेशय(mmapr, itemp);
                }
            }
        } अण्णहा जइ (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव anodep
                   = VN_CAST(nodep, ग्रन्थिः_UOr_Struct_DType)) {
            प्रति (Ast_ग्रन्थिः* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_निवेशय(mmapr, itemp);
            }
        } अण्णहा जइ (ध्रुव AstPackage* ध्रुव anodep = VN_CAST(nodep, Package)) {
            प्रति (Ast_ग्रन्थिः* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_निवेशय(mmapr, itemp);
            }
        } अण्णहा {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    शून्य member_निवेशय(MemberMap& mmapr, Ast_ग्रन्थिः* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        जइ (childp->name().empty()) प्रत्यागम;
        ध्रुव auto mitPair = mmapr.emplace(childp->name(), childp);
        जइ (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
