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

#నిర్వచితం_కాకపోతే VERILATOR_V3MEMBERMAP_H_
#నిర్వచించు VERILATOR_V3MEMBERMAP_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Global.h"

#అన్తర్భావ <map>

//######################################################################

class VMemberMap final {
    // MEMBERS
    using MemberMap = std::map<std::string, Ast_కణుపు*>;
    using కణుపు_Map = std::map<స్థిరరాశి Ast_కణుపు*, MemberMap>;
    కణుపు_Map m_map;  // Map of nodes being tracked

    VL_DEFINE_DEBUG_FUNCTIONS;

బహిరంగ:
    శూన్య clear() { m_map.clear(); }
    // Find 'name' under 'nodep', caching nodep's children if needed
    Ast_కణుపు* కనుగొను_Member(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి std::string& name) {
        స్వయంచాలక nit = m_map.కనుగొను(nodep);
        ఒకవేళ (VL_UNLIKELY(nit == m_map.చివర())) {
            పరిశీలన(nodep);
            nit = m_map.కనుగొను(nodep);
        }
        స్థిరరాశి స్వయంచాలక mit = nit->రెండవ.కనుగొను(name);
        ఒకవేళ (mit == nit->రెండవ.చివర()) తిరిగి nullptr;
        తిరిగి mit->రెండవ;
    }
    // Insert under the given parent node the child's name
    శూన్య చొప్పించు(స్థిరరాశి Ast_కణుపు* nodep, Ast_కణుపు* childp) {
        స్వయంచాలక nit = m_map.కనుగొను(nodep);
        ఒకవేళ (nit == m_map.చివర()) {
            పరిశీలన(nodep);
            nit = m_map.కనుగొను(nodep);
        }
        member_చొప్పించు(nit->రెండవ, childp, false);
    }

అంతరంగ:
    శూన్య పరిశీలన(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి స్వయంచాలక nitPair = m_map.emplace(nodep, MemberMap{});
        MemberMap& mmapr = nitPair.మొదటి->రెండవ;
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి anodep = VN_CAST(nodep, Class)) {
            కొరకు (Ast_కణుపు* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                ఒకవేళ (స్థిరరాశి AstScope* స్థిరరాశి scopep = VN_CAST(itemp, Scope)) {
                    కొరకు (Ast_కణుపు* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        ఒకవేళ (AstClass::is_Cacheable_బిడ్డ(blockp)) member_చొప్పించు(mmapr, blockp);
                    }
                } లేకపోతే {
                    ఒకవేళ (AstClass::is_Cacheable_బిడ్డ(itemp)) member_చొప్పించు(mmapr, itemp);
                }
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstIface* స్థిరరాశి anodep = VN_CAST(nodep, Iface)) {
            కొరకు (Ast_కణుపు* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ఒకవేళ (స్థిరరాశి AstScope* స్థిరరాశి scopep = VN_CAST(itemp, Scope)) {
                    కొరకు (Ast_కణుపు* blockp = scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                        member_చొప్పించు(mmapr, blockp);
                    }
                } లేకపోతే ఒకవేళ (!VN_IS(itemp, Always)) {
                    member_చొప్పించు(mmapr, itemp);
                }
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి anodep
                   = VN_CAST(nodep, కణుపు_UOr_Struct_DType)) {
            కొరకు (Ast_కణుపు* itemp = anodep->membersp(); itemp; itemp = itemp->nextp()) {
                member_చొప్పించు(mmapr, itemp);
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstPackage* స్థిరరాశి anodep = VN_CAST(nodep, Package)) {
            కొరకు (Ast_కణుపు* itemp = anodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                member_చొప్పించు(mmapr, itemp);
            }
        } లేకపోతే {
            nodep->v3fatalSrc("Unsupported node type");
        }
    }
    శూన్య member_చొప్పించు(MemberMap& mmapr, Ast_కణుపు* childp, bool warn = true) {
        // Skip nodes without names (e.g., initial blocks, anonymous statements)
        // These can't be looked up by name anyway
        ఒకవేళ (childp->name().ఖాళీలు()) తిరిగి;
        స్థిరరాశి స్వయంచాలక mitPair = mmapr.emplace(childp->name(), childp);
        ఒకవేళ (VL_UNCOVERABLE(!mitPair.రెండవ && warn)) {
            // Probably an internal error, but we'll make it user friendly if happens
            childp->v3error("Duplicate declaration of member name: " << childp->prettyNameQ());
        }
    }
};

#అయితే_ముగింపు  // guard
