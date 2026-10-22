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

#अगर_अघोषित VERILATOR_V3MEMBERMAP_H_
#घोषणा VERILATOR_V3MEMBERMAP_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Global.h"

#समावेश <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_गाँठ*>;
    using गाँठ_Map = std::map<स्थिर Ast_गाँठ*, MemberMap>;
    गाँठ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

खुला:
    व्योम clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_गाँठ* findMember(स्थिर Ast_गाँठ* nodep, स्थिर std::string& name) {
        स्वतः nit = m_map.find(nodep);
        अगर (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        स्थिर स्वतः mit = nit->second.find(name);
        अगर (mit == nit->second.end()) वापस nullptr;
        वापस mit->second;
    }
    // Insert under the given parent node the child's name
    व्योम जोड़ो(स्थिर Ast_गाँठ* nodep, Ast_गाँठ* childp) {
        स्वतः nit = m_map.find(nodep);
        अगर (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_जोड़ो(nit->second, childp, false);
    }

निजी:
    व्योम scan(स्थिर Ast_गाँठ* nodep) {
        स्थिर स्वतः nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        अगर (स्थिर AstClass* स्थिर anodep = VN_CAST(nodep, Class)) {
            क्रम (Ast_गाँठ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                अगर (स्थिर AstScope* स्थिर scopep = VN_CAST(itemp, Scope)) {
                    क्रम (Ast_गाँठ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        अगर (AstClass::isCacheableChild(blockp)) member_जोड़ो(mmapr, blockp);
                    }
                } अन्यथा {
                    अगर (AstClass::isCacheableChild(itemp)) member_जोड़ो(mmapr, itemp);
                }
            }
        } अन्यथा अगर (स्थिर AstIface* स्थिर anodep = VN_CAST(nodep, Iface)) {
            क्रम (Ast_गाँठ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                अगर (स्थिर AstScope* स्थिर scopep = VN_CAST(itemp, Scope)) {
                    क्रम (Ast_गाँठ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_जोड़ो(mmapr, blockp);
                    }
                } अन्यथा अगर (!VN_IS(itemp, Always)) {
                    member_जोड़ो(mmapr, itemp);
                }
            }
        } अन्यथा अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर anodep
                   = VN_CAST(nodep, गाँठ_UOr_Struct_DType)) {
            क्रम (Ast_गाँठ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_जोड़ो(mmapr, itemp);
            }
        } अन्यथा अगर (स्थिर AstPackage* स्थिर anodep = VN_CAST(nodep, Package)) {
            क्रम (Ast_गाँठ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_जोड़ो(mmapr, itemp);
            }
        } अन्यथा {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    व्योम member_जोड़ो(MemberMap& mmapr, Ast_गाँठ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        अगर (childp->name().empty()) वापस;
        स्थिर स्वतः mitPair = mmapr.emplace(childp->name(), childp);
        अगर (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#पूर्ण_अगर  // guard
