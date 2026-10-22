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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ಗಂಟು*>;
    using ಗಂಟು_Map = std::map<ಧ್ರುವ Ast_ಗಂಟು*, MemberMap>;
    ಗಂಟು_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ಗಂಟು* findMember(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ std::string& name) {
        auto nit = m_map.find(nodep);
        ಯದಿ (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ಧ್ರುವ auto mit = nit->second.find(name);
        ಯದಿ (mit == nit->second.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ mit->second;
    }
    // Insert under the given parent node the child's name
    ಶೂನ್ಯ ಸೇರಿಸು(ಧ್ರುವ Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* childp) {
        auto nit = m_map.find(nodep);
        ಯದಿ (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_ಸೇರಿಸು(nit->second, childp, false);
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ scan(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ anodep = VN_CAST(nodep, Class)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                ಯದಿ (ಧ್ರುವ AstScope* ಧ್ರುವ scopep = VN_CAST(itemp, Scope)) {
                    ಪ್ರತಿ (Ast_ಗಂಟು* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        ಯದಿ (AstClass::isCacheableChild(blockp)) member_ಸೇರಿಸು(mmapr, blockp);
                    }
                } ಅನ್ಯಥಾ {
                    ಯದಿ (AstClass::isCacheableChild(itemp)) member_ಸೇರಿಸು(mmapr, itemp);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstIface* ಧ್ರುವ anodep = VN_CAST(nodep, Iface)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ಯದಿ (ಧ್ರುವ AstScope* ಧ್ರುವ scopep = VN_CAST(itemp, Scope)) {
                    ಪ್ರತಿ (Ast_ಗಂಟು* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ಸೇರಿಸು(mmapr, blockp);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(itemp, Always)) {
                    member_ಸೇರಿಸು(mmapr, itemp);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ anodep
                   = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ಸೇರಿಸು(mmapr, itemp);
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPackage* ಧ್ರುವ anodep = VN_CAST(nodep, Package)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ಸೇರಿಸು(mmapr, itemp);
            }
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    ಶೂನ್ಯ member_ಸೇರಿಸು(MemberMap& mmapr, Ast_ಗಂಟು* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        ಯದಿ (childp->name().empty()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ auto mitPair = mmapr.emplace(childp->name(), childp);
        ಯದಿ (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
