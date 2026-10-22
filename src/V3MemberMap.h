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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Global.h"

#അന്തര്ഭാവ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_കണ്ണി*>;
    using കണ്ണി_Map = std::map<സ്ഥിരം Ast_കണ്ണി*, MemberMap>;
    കണ്ണി_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

പൊതു:
    ശൂന്യം clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_കണ്ണി* findMember(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം std::string& name) {
        auto nit = m_map.find(nodep);
        എങ്കിൽ (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        സ്ഥിരം auto mit = nit->second.find(name);
        എങ്കിൽ (mit == nit->second.end()) തിരികെ nullptr;
        തിരികെ mit->second;
    }
    // Insert under the given parent node the child's name
    ശൂന്യം തിരുകുക(സ്ഥിരം Ast_കണ്ണി* nodep, Ast_കണ്ണി* childp) {
        auto nit = m_map.find(nodep);
        എങ്കിൽ (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_തിരുകുക(nit->second, childp, false);
    }

സ്വകാര്യ:
    ശൂന്യം scan(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം anodep = VN_CAST(nodep, Class)) {
            വേണ്ടി (Ast_കണ്ണി* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                എങ്കിൽ (സ്ഥിരം AstScope* സ്ഥിരം scopep = VN_CAST(itemp, Scope)) {
                    വേണ്ടി (Ast_കണ്ണി* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        എങ്കിൽ (AstClass::isCacheableChild(blockp)) member_തിരുകുക(mmapr, blockp);
                    }
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (AstClass::isCacheableChild(itemp)) member_തിരുകുക(mmapr, itemp);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstIface* സ്ഥിരം anodep = VN_CAST(nodep, Iface)) {
            വേണ്ടി (Ast_കണ്ണി* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                എങ്കിൽ (സ്ഥിരം AstScope* സ്ഥിരം scopep = VN_CAST(itemp, Scope)) {
                    വേണ്ടി (Ast_കണ്ണി* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_തിരുകുക(mmapr, blockp);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(itemp, Always)) {
                    member_തിരുകുക(mmapr, itemp);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം anodep
                   = VN_CAST(nodep, കണ്ണി_UOr_Struct_DType)) {
            വേണ്ടി (Ast_കണ്ണി* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_തിരുകുക(mmapr, itemp);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPackage* സ്ഥിരം anodep = VN_CAST(nodep, Package)) {
            വേണ്ടി (Ast_കണ്ണി* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_തിരുകുക(mmapr, itemp);
            }
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    ശൂന്യം member_തിരുകുക(MemberMap& mmapr, Ast_കണ്ണി* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        എങ്കിൽ (childp->name().empty()) തിരികെ;
        സ്ഥിരം auto mitPair = mmapr.emplace(childp->name(), childp);
        എങ്കിൽ (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
