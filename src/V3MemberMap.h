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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3MEMBERMAP_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3MEMBERMAP_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ಗಂಟು*>;
    using ಗಂಟು_Map = std::map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, MemberMap>;
    ಗಂಟು_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ಗಂಟು* ಕಂಡುಹಿಡಿ_Member(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ std::string& name) {
        ಸ್ವಯಂಚಾಲಿತ nit = m_map.ಕಂಡುಹಿಡಿ(nodep);
        ಆದರೆ (VL_UNLIKELY(nit == m_map.ಕೊನೆ())) {
            ಪರಿಶೀಲನೆ(nodep);
            nit = m_map.ಕಂಡುಹಿಡಿ(nodep);
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mit = nit->ಎರಡನೇ.ಕಂಡುಹಿಡಿ(name);
        ಆದರೆ (mit == nit->ಎರಡನೇ.ಕೊನೆ()) ಹಿಂತಿರುಗಿ nullptr;
        ಹಿಂತಿರುಗಿ mit->ಎರಡನೇ;
    }
    // Insert under the given parent node the child's name
    ಶೂನ್ಯ ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* childp) {
        ಸ್ವಯಂಚಾಲಿತ nit = m_map.ಕಂಡುಹಿಡಿ(nodep);
        ಆದರೆ (nit == m_map.ಕೊನೆ()) {
            ಪರಿಶೀಲನೆ(nodep);
            nit = m_map.ಕಂಡುಹಿಡಿ(nodep);
        }
        member_ಸೇರಿಸು(nit->ಎರಡನೇ, childp, false);
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.ಮೊದಲ->ಎರಡನೇ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, Class)) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_CAST(itemp, Scope)) {
                    ಗಾಗಿ (Ast_ಗಂಟು* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        ಆದರೆ (AstClass::is_Cacheable_ಮಗು(blockp)) member_ಸೇರಿಸು(mmapr, blockp);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (AstClass::is_Cacheable_ಮಗು(itemp)) member_ಸೇರಿಸು(mmapr, itemp);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, Iface)) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_CAST(itemp, Scope)) {
                    ಗಾಗಿ (Ast_ಗಂಟು* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ಸೇರಿಸು(mmapr, blockp);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(itemp, Always)) {
                    member_ಸೇರಿಸು(mmapr, itemp);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ anodep
                   = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ಸೇರಿಸು(mmapr, itemp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackage* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, Package)) {
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ಸೇರಿಸು(mmapr, itemp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    ಶೂನ್ಯ member_ಸೇರಿಸು(MemberMap& mmapr, Ast_ಗಂಟು* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        ಆದರೆ (childp->name().ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mitPair = mmapr.emplace(childp->name(), childp);
        ಆದರೆ (VL_UNCOVERABLE(!mitPair.ಎರಡನೇ && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // guard
