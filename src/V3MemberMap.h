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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Global.h"

#அந்தர்பாவ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_முனை*>;
    using முனை_Map = std::map<மாறிலி Ast_முனை*, MemberMap>;
    முனை_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

பொது:
    வெறுமை clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_முனை* findMember(மாறிலி Ast_முனை* nodep, மாறிலி std::string& name) {
        auto nit = m_map.find(nodep);
        எனில் (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        மாறிலி auto mit = nit->second.find(name);
        எனில் (mit == nit->second.end()) திரும்பு nullptr;
        திரும்பு mit->second;
    }
    // Insert under the given parent node the child's name
    வெறுமை செருகு(மாறிலி Ast_முனை* nodep, Ast_முனை* childp) {
        auto nit = m_map.find(nodep);
        எனில் (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_செருகு(nit->second, childp, false);
    }

தனிப்பட்ட:
    வெறுமை scan(மாறிலி Ast_முனை* nodep) {
        மாறிலி auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        எனில் (மாறிலி AstClass* மாறிலி anodep = VN_CAST(nodep, Class)) {
            க்கு (Ast_முனை* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                எனில் (மாறிலி AstScope* மாறிலி scopep = VN_CAST(itemp, Scope)) {
                    க்கு (Ast_முனை* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        எனில் (AstClass::isCacheableChild(blockp)) member_செருகு(mmapr, blockp);
                    }
                } இல்லையெனில் {
                    எனில் (AstClass::isCacheableChild(itemp)) member_செருகு(mmapr, itemp);
                }
            }
        } இல்லையெனில் எனில் (மாறிலி AstIface* மாறிலி anodep = VN_CAST(nodep, Iface)) {
            க்கு (Ast_முனை* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                எனில் (மாறிலி AstScope* மாறிலி scopep = VN_CAST(itemp, Scope)) {
                    க்கு (Ast_முனை* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_செருகு(mmapr, blockp);
                    }
                } இல்லையெனில் எனில் (!VN_IS(itemp, Always)) {
                    member_செருகு(mmapr, itemp);
                }
            }
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி anodep
                   = VN_CAST(nodep, முனை_UOr_Struct_DType)) {
            க்கு (Ast_முனை* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_செருகு(mmapr, itemp);
            }
        } இல்லையெனில் எனில் (மாறிலி AstPackage* மாறிலி anodep = VN_CAST(nodep, Package)) {
            க்கு (Ast_முனை* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_செருகு(mmapr, itemp);
            }
        } இல்லையெனில் {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    வெறுமை member_செருகு(MemberMap& mmapr, Ast_முனை* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        எனில் (childp->name().empty()) திரும்பு;
        மாறிலி auto mitPair = mmapr.emplace(childp->name(), childp);
        எனில் (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
