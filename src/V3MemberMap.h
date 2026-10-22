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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3MEMBERMAP_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3MEMBERMAP_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ਗੰਢ*>;
    using ਗੰਢ_Map = std::map<ਅਚਲ Ast_ਗੰਢ*, MemberMap>;
    ਗੰਢ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

ਜਨਤਕ:
    ਖਾਲੀ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ਗੰਢ* ਲੱਭੋ_Member(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ std::string& name) {
        ਸਵੈਚਾਲਿਤ nit = m_map.ਲੱਭੋ(nodep);
        ਜੇ (VL_UNLIKELY(nit == m_map.ਸਿਰਾ())) {
            ਜਾਂਚ(nodep);
            nit = m_map.ਲੱਭੋ(nodep);
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ mit = nit->ਦੂਜਾ.ਲੱਭੋ(name);
        ਜੇ (mit == nit->ਦੂਜਾ.ਸਿਰਾ()) ਵਾਪਸ nullptr;
        ਵਾਪਸ mit->ਦੂਜਾ;
    }
    // Insert under the given parent node the child's name
    ਖਾਲੀ ਪਾਓ(ਅਚਲ Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* childp) {
        ਸਵੈਚਾਲਿਤ nit = m_map.ਲੱਭੋ(nodep);
        ਜੇ (nit == m_map.ਸਿਰਾ()) {
            ਜਾਂਚ(nodep);
            nit = m_map.ਲੱਭੋ(nodep);
        }
        member_ਪਾਓ(nit->ਦੂਜਾ, childp, false);
    }

ਨਿਜੀ:
    ਖਾਲੀ ਜਾਂਚ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.ਪਹਿਲਾ->ਦੂਜਾ;
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ anodep = VN_CAST(nodep, Class)) {
            ਲਈ (Ast_ਗੰਢ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                ਜੇ (ਅਚਲ AstScope* ਅਚਲ scopep = VN_CAST(itemp, Scope)) {
                    ਲਈ (Ast_ਗੰਢ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        ਜੇ (AstClass::is_Cacheable_ਬੱਚਾ(blockp)) member_ਪਾਓ(mmapr, blockp);
                    }
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (AstClass::is_Cacheable_ਬੱਚਾ(itemp)) member_ਪਾਓ(mmapr, itemp);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstIface* ਅਚਲ anodep = VN_CAST(nodep, Iface)) {
            ਲਈ (Ast_ਗੰਢ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ਜੇ (ਅਚਲ AstScope* ਅਚਲ scopep = VN_CAST(itemp, Scope)) {
                    ਲਈ (Ast_ਗੰਢ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ਪਾਓ(mmapr, blockp);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(itemp, Always)) {
                    member_ਪਾਓ(mmapr, itemp);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ anodep
                   = VN_CAST(nodep, ਗੰਢ_UOr_Struct_DType)) {
            ਲਈ (Ast_ਗੰਢ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ਪਾਓ(mmapr, itemp);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstPackage* ਅਚਲ anodep = VN_CAST(nodep, Package)) {
            ਲਈ (Ast_ਗੰਢ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ਪਾਓ(mmapr, itemp);
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    ਖਾਲੀ member_ਪਾਓ(MemberMap& mmapr, Ast_ਗੰਢ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        ਜੇ (childp->name().ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ mitPair = mmapr.emplace(childp->name(), childp);
        ਜੇ (VL_UNCOVERABLE(!mitPair.ਦੂਜਾ && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#ਜੇ_ਅੰਤ  // guard
