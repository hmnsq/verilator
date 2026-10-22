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
    using MemberMap = std::map<std::string, Ast_गाठ*>;
    using गाठ_Map = std::map<स्थिरांक Ast_गाठ*, MemberMap>;
    गाठ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

सार्वजनिक:
    शून्य clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_गाठ* findMember(स्थिरांक Ast_गाठ* nodep, स्थिरांक std::string& name) {
        स्वतः nit = m_map.find(nodep);
        जर (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        स्थिरांक स्वतः mit = nit->second.find(name);
        जर (mit == nit->second.end()) परत nullptr;
        परत mit->second;
    }
    // Insert under the given parent node the child's name
    शून्य घाला(स्थिरांक Ast_गाठ* nodep, Ast_गाठ* childp) {
        स्वतः nit = m_map.find(nodep);
        जर (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_घाला(nit->second, childp, false);
    }

खाजगी:
    शून्य scan(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक स्वतः nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        जर (स्थिरांक AstClass* स्थिरांक anodep = VN_CAST(nodep, Class)) {
            साठी (Ast_गाठ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                जर (स्थिरांक AstScope* स्थिरांक scopep = VN_CAST(itemp, Scope)) {
                    साठी (Ast_गाठ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        जर (AstClass::isCacheableChild(blockp)) member_घाला(mmapr, blockp);
                    }
                } अन्यथा {
                    जर (AstClass::isCacheableChild(itemp)) member_घाला(mmapr, itemp);
                }
            }
        } अन्यथा जर (स्थिरांक AstIface* स्थिरांक anodep = VN_CAST(nodep, Iface)) {
            साठी (Ast_गाठ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                जर (स्थिरांक AstScope* स्थिरांक scopep = VN_CAST(itemp, Scope)) {
                    साठी (Ast_गाठ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_घाला(mmapr, blockp);
                    }
                } अन्यथा जर (!VN_IS(itemp, Always)) {
                    member_घाला(mmapr, itemp);
                }
            }
        } अन्यथा जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक anodep
                   = VN_CAST(nodep, गाठ_UOr_Struct_DType)) {
            साठी (Ast_गाठ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_घाला(mmapr, itemp);
            }
        } अन्यथा जर (स्थिरांक AstPackage* स्थिरांक anodep = VN_CAST(nodep, Package)) {
            साठी (Ast_गाठ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_घाला(mmapr, itemp);
            }
        } अन्यथा {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    शून्य member_घाला(MemberMap& mmapr, Ast_गाठ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        जर (childp->name().empty()) परत;
        स्थिरांक स्वतः mitPair = mmapr.emplace(childp->name(), childp);
        जर (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
