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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Global.h"

#અન્તર્ભાવ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ગાંઠ*>;
    using ગાંઠ_Map = std::map<અચલ Ast_ગાંઠ*, MemberMap>;
    ગાંઠ_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

જાહેર:
    શૂન્ય clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ગાંઠ* findMember(અચલ Ast_ગાંઠ* nodep, અચલ std::string& name) {
        auto nit = m_map.find(nodep);
        જો (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        અચલ auto mit = nit->second.find(name);
        જો (mit == nit->second.end()) પાછા nullptr;
        પાછા mit->second;
    }
    // Insert under the given parent node the child's name
    શૂન્ય ઉમેરો(અચલ Ast_ગાંઠ* nodep, Ast_ગાંઠ* childp) {
        auto nit = m_map.find(nodep);
        જો (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_ઉમેરો(nit->second, childp, false);
    }

ખાનગી:
    શૂન્ય scan(અચલ Ast_ગાંઠ* nodep) {
        અચલ auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        જો (અચલ AstClass* અચલ anodep = VN_CAST(nodep, Class)) {
            માટે (Ast_ગાંઠ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                જો (અચલ AstScope* અચલ scopep = VN_CAST(itemp, Scope)) {
                    માટે (Ast_ગાંઠ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        જો (AstClass::isCacheableChild(blockp)) member_ઉમેરો(mmapr, blockp);
                    }
                } નહીંતર {
                    જો (AstClass::isCacheableChild(itemp)) member_ઉમેરો(mmapr, itemp);
                }
            }
        } નહીંતર જો (અચલ AstIface* અચલ anodep = VN_CAST(nodep, Iface)) {
            માટે (Ast_ગાંઠ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                જો (અચલ AstScope* અચલ scopep = VN_CAST(itemp, Scope)) {
                    માટે (Ast_ગાંઠ* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ઉમેરો(mmapr, blockp);
                    }
                } નહીંતર જો (!VN_IS(itemp, Always)) {
                    member_ઉમેરો(mmapr, itemp);
                }
            }
        } નહીંતર જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ anodep
                   = VN_CAST(nodep, ગાંઠ_UOr_Struct_DType)) {
            માટે (Ast_ગાંઠ* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ઉમેરો(mmapr, itemp);
            }
        } નહીંતર જો (અચલ AstPackage* અચલ anodep = VN_CAST(nodep, Package)) {
            માટે (Ast_ગાંઠ* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ઉમેરો(mmapr, itemp);
            }
        } નહીંતર {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    શૂન્ય member_ઉમેરો(MemberMap& mmapr, Ast_ગાંઠ* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        જો (childp->name().empty()) પાછા;
        અચલ auto mitPair = mmapr.emplace(childp->name(), childp);
        જો (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
