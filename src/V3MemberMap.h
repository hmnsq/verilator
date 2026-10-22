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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"

#ଅନ୍ତର୍ଭାଵ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_ଗଣ୍ଠି*>;
    using ଗଣ୍ଠି_Map = std::map<ଅଚଳ Ast_ଗଣ୍ଠି*, MemberMap>;
    ଗଣ୍ଠି_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_ଗଣ୍ଠି* findMember(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ std::string& name) {
        auto nit = m_map.find(nodep);
        ଯଦି (VL_UNLIKELY(nit == m_map.end())) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        ଅଚଳ auto mit = nit->second.find(name);
        ଯଦି (mit == nit->second.end()) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ mit->second;
    }
    // Insert under the given parent node the child's name
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* childp) {
        auto nit = m_map.find(nodep);
        ଯଦି (nit == m_map.end()) {
            scan(nodep);
            nit = m_map.find(nodep);
        }
        member_ଭର୍ତ୍ତି(nit->second, childp, false);
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ scan(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ auto nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.first->second;
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ anodep = VN_CAST(nodep, Class)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                ଯଦି (ଅଚଳ AstScope* ଅଚଳ scopep = VN_CAST(itemp, Scope)) {
                    ପାଇଁ (Ast_ଗଣ୍ଠି* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        ଯଦି (AstClass::isCacheableChild(blockp)) member_ଭର୍ତ୍ତି(mmapr, blockp);
                    }
                } ଅନ୍ୟଥା {
                    ଯଦି (AstClass::isCacheableChild(itemp)) member_ଭର୍ତ୍ତି(mmapr, itemp);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstIface* ଅଚଳ anodep = VN_CAST(nodep, Iface)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ଯଦି (ଅଚଳ AstScope* ଅଚଳ scopep = VN_CAST(itemp, Scope)) {
                    ପାଇଁ (Ast_ଗଣ୍ଠି* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_ଭର୍ତ୍ତି(mmapr, blockp);
                    }
                } ଅନ୍ୟଥା ଯଦି (!VN_IS(itemp, Always)) {
                    member_ଭର୍ତ୍ତି(mmapr, itemp);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ anodep
                   = VN_CAST(nodep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_ଭର୍ତ୍ତି(mmapr, itemp);
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPackage* ଅଚଳ anodep = VN_CAST(nodep, Package)) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_ଭର୍ତ୍ତି(mmapr, itemp);
            }
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    ଶୂନ୍ୟ member_ଭର୍ତ୍ତି(MemberMap& mmapr, Ast_ଗଣ୍ଠି* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        ଯଦି (childp->name().empty()) ଫେରନ୍ତୁ;
        ଅଚଳ auto mitPair = mmapr.emplace(childp->name(), childp);
        ଯଦି (VL_UNCOVERABLE(!mitPair.second && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#endif  // guard
