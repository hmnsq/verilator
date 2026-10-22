// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Block code ordering
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
//  OrderMoveGraph implementation and related
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderMoveGraph.h"

#ضمن "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

ترتیب_Move_Dom_Scope::DomScopeMap ترتیب_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

ترتیب_Move_راس::ترتیب_Move_راس(ترتیب_Move_ګراف& ګراف, ترتیب_Logic_راس* lVtxp,
                                 ثابت ترتیب_Domain* domainp) VL_MT_DISABLED
    : V3_ګراف_راس{&ګراف},
      m_logicp{lVtxp},
      m_domScope{ترتیب_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : تش_اشاره)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

صنف ترتیب_Move_ګراف_Builder final {
    // TYPES
    کارول DomainMap = std::map<ثابت ترتیب_Domain*, ترتیب_Move_راس*>;

    // MEMBERS
    ترتیب_ګراف& m_ترتیب_ګراف;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<ترتیب_Move_ګراف> m_moveGraphp{جديد ترتیب_Move_ګراف{m_ترتیب_ګراف}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    څرګند ترتیب_Move_ګراف_Builder(ترتیب_ګراف& ترتیب_ګراف_2)
        : m_ترتیب_ګراف{ترتیب_ګراف_2} {
        // How this works:
        //  - Create a OrderMoveVertex for each OrderLogicVertex.
        //  - Following each OrderLogicVertex, search forward in the context of its domain
        //    - If we encounter another OrderLogicVertex in non-exclusive domain, make the
        //      OrderMoveVertex->OrderMoveVertex edge.
        //    - If we encounter an OrderVarVertex, make a Vertex for the (OrderVarVertex, domain)
        //      pair and continue to search forward in the context of the same domain.  Unless we
        //      already created that pair, in which case, we've already done the forward search,
        //      so stop.

        // For each logic vertex, make a OrderMoveVertex, for each variable vertex, allocate
        // storage
        لكل (V3_ګراف_راس& vtx : m_ترتیب_ګراف.vertices()) {
            إذا (ترتیب_Logic_راس* ثابت lvtxp = vtx.cast<ترتیب_Logic_راس>()) {
                lvtxp->userp(جديد ترتیب_Move_راس{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } وإلا {
                // This is an OrderVarVertex
                m_domainMaps.emplace_شا();
                vtx.userp(&m_domainMaps.شا());
            }
        }
        // Build edges between logic vertices
        لكل (V3_ګراف_راس& vtx : m_ترتیب_ګراف.vertices()) {
            إذا (ترتیب_Logic_راس* ثابت lvtxp = vtx.cast<ترتیب_Logic_راس>()) {
                iterate_Logic_راس(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_څنډې_مجموعه(&V3_ګراف_څنډه::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    مجازي ~ترتیب_Move_ګراف_Builder() = افتراضي;
    VL_UNCOPYABLE(ترتیب_Move_ګراف_Builder);
    VL_UNMOVABLE(ترتیب_Move_ګراف_Builder);

    // METHODS

    منطقي domainsExclusive(ثابت ترتیب_Domain* fromp, ثابت ترتیب_Domain* سر) {
        // Return 'true' if we can prove that both 'from' and 'to' cannot both
        // be active on the same evaluation, or false if we can't prove this.
        //
        // This detects the case of 'always @(posedge clk)'
        // and 'always @(negedge clk)' being exclusive.
        //
        // Are there any other cases we need to handle? Maybe not,
        // because these are not exclusive:
        //   always @(posedge A or posedge B)
        //   always @(negedge A)
        //
        // ... unless you know more about A and B, which sounds hard.

        ثابت std::سېټ<uint32_t>& fromTriggers = fromp->triggers();
        ثابت std::سېټ<uint32_t>& منزل_Triggers = سر->triggers();
        إذا (fromTriggers.اندازه() != 1) أرجع خطأ;
        إذا (منزل_Triggers.اندازه() != 1) أرجع خطأ;
        ثابت Ast_Sen_توکی* ثابت fromSenItemp = m_ترتیب_ګراف.trigSenItemp(*fromTriggers.begin());
        إذا (!fromSenItemp) أرجع خطأ;
        ثابت Ast_Sen_توکی* ثابت منزل_Sen_Itemp = m_ترتیب_ګراف.trigSenItemp(*منزل_Triggers.begin());
        إذا (!منزل_Sen_Itemp) أرجع خطأ;

        ثابت Ast_غوټه_Var_Ref* ثابت fromVarrefp = fromSenItemp->varrefp();
        إذا (!fromVarrefp) أرجع خطأ;
        ثابت Ast_غوټه_Var_Ref* ثابت منزل_Varrefp = منزل_Sen_Itemp->varrefp();
        إذا (!منزل_Varrefp) أرجع خطأ;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        إذا (fromVarrefp->varScopep() != منزل_Varrefp->varScopep()) أرجع خطأ;

        أرجع fromSenItemp->څنډه_Type().exclusive_څنډه(منزل_Sen_Itemp->څنډه_Type());
    }

    فراغ جمع_کړه_څنډه(ترتیب_Move_راس* srcp, ترتیب_Move_راس* dstp) {
        جديد V3_ګراف_څنډه{m_moveGraphp.get(), srcp, dstp, 1};
    }

    فراغ iterate_Logic_راس(ثابت ترتیب_Logic_راس* lvtxp) {
        ثابت ترتیب_Domain* ثابت domainp = lvtxp->domainp();
        ترتیب_Move_راس* ثابت lMoveVtxp = static_cast<ترتیب_Move_راس*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        لكل (ثابت V3_ګراف_څنډه& څنډه : lvtxp->بهر_څنډې()) {
            إذا (څنډه.وزن_2() == 0) استمر;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ثابت ترتیب_Var_راس* ثابت vvtxp = static_cast<ثابت ترتیب_Var_راس*>(څنډه.سر());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ثابت خپلکاری pair = mapp.emplace(domainp, تش_اشاره);
            // Reference to the mapped OrderMoveVertex
            ترتیب_Move_راس*& vMoveVtxp = pair.لومړی->دوهم;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            إذا (pair.دوهم) vMoveVtxp = iterate_Var_راس(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            إذا (!vMoveVtxp) استمر;

            // Add this (variable, domain) as dependent of the logic that writes it.
            جمع_کړه_څنډه(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    ترتیب_Move_راس* iterate_Var_راس(ثابت ترتیب_Var_راس* vvtxp, ثابت ترتیب_Domain* domainp) {
        ترتیب_Move_راس* vMoveVtxp = تش_اشاره;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        لكل (ثابت V3_ګراف_څنډه& څنډه : vvtxp->بهر_څنډې()) {
            إذا (څنډه.وزن_2() == 0) استمر;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ثابت ترتیب_Logic_راس* ثابت lVtxp = څنډه.سر()->as<ترتیب_Logic_راس>();

            // Do not construct dependencies across exclusive domains.
            إذا (domainsExclusive(domainp, lVtxp->domainp())) استمر;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            إذا (!vMoveVtxp) vMoveVtxp = جديد ترتیب_Move_راس{*m_moveGraphp, تش_اشاره, domainp};
            ترتیب_Move_راس* ثابت lMoveVxp = static_cast<ترتیب_Move_راس*>(lVtxp->userp());
            جمع_کړه_څنډه(vMoveVtxp, lMoveVxp);
        }
        أرجع vMoveVtxp;
    }

عام:
    ساكن std::unique_ptr<ترتیب_Move_ګراف> apply(ترتیب_ګراف& ترتیب_ګراف_2) {
        أرجع std::move(ترتیب_Move_ګراف_Builder{ترتیب_ګراف_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<ترتیب_Move_ګراف> ترتیب_Move_ګراف::build(ترتیب_ګراف& ترتیب_ګراف_2) {
    أرجع ترتیب_Move_ګراف_Builder::apply(ترتیب_ګراف_2);
}
