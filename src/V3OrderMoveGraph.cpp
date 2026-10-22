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

ترتيب_Move_Dom_Scope::DomScopeMap ترتيب_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

ترتيب_Move_راس::ترتيب_Move_راس(ترتيب_Move_گراف& گراف, ترتيب_Logic_راس* lVtxp,
                                 ثابت ترتيب_Domain* domainp) VL_MT_DISABLED
    : V3_گراف_راس{&گراف},
      m_logicp{lVtxp},
      m_domScope{ترتيب_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : خالي_اشارو)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

صنف ترتيب_Move_گراف_Builder final {
    // TYPES
    استعمال DomainMap = std::map<ثابت ترتيب_Domain*, ترتيب_Move_راس*>;

    // MEMBERS
    ترتيب_گراف& m_ترتيب_گراف;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<ترتيب_Move_گراف> m_moveGraphp{جديد ترتيب_Move_گراف{m_ترتيب_گراف}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    واضح ترتيب_Move_گراف_Builder(ترتيب_گراف& ترتيب_گراف_2)
        : m_ترتيب_گراف{ترتيب_گراف_2} {
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
        لكل (V3_گراف_راس& vtx : m_ترتيب_گراف.vertices()) {
            إذا (ترتيب_Logic_راس* ثابت lvtxp = vtx.cast<ترتيب_Logic_راس>()) {
                lvtxp->userp(جديد ترتيب_Move_راس{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } وإلا {
                // This is an OrderVarVertex
                m_domainMaps.emplace_پٺ();
                vtx.userp(&m_domainMaps.پٺ());
            }
        }
        // Build edges between logic vertices
        لكل (V3_گراف_راس& vtx : m_ترتيب_گراف.vertices()) {
            إذا (ترتيب_Logic_راس* ثابت lvtxp = vtx.cast<ترتيب_Logic_راس>()) {
                iterate_Logic_راس(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_ڪنارا_جوڙ(&V3_گراف_ڪنارو::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    مجازي ~ترتيب_Move_گراف_Builder() = افتراضي;
    VL_UNCOPYABLE(ترتيب_Move_گراف_Builder);
    VL_UNMOVABLE(ترتيب_Move_گراف_Builder);

    // METHODS

    منطقي domainsExclusive(ثابت ترتيب_Domain* fromp, ثابت ترتيب_Domain* چوٽي) {
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

        ثابت std::سيٽ<uint32_t>& fromTriggers = fromp->triggers();
        ثابت std::سيٽ<uint32_t>& منزل_Triggers = چوٽي->triggers();
        إذا (fromTriggers.سائيز() != 1) أرجع خطأ;
        إذا (منزل_Triggers.سائيز() != 1) أرجع خطأ;
        ثابت Ast_Sen_شي* ثابت fromSenItemp = m_ترتيب_گراف.trigSenItemp(*fromTriggers.begin());
        إذا (!fromSenItemp) أرجع خطأ;
        ثابت Ast_Sen_شي* ثابت منزل_Sen_Itemp = m_ترتيب_گراف.trigSenItemp(*منزل_Triggers.begin());
        إذا (!منزل_Sen_Itemp) أرجع خطأ;

        ثابت Ast_ڳنڍ_Var_Ref* ثابت fromVarrefp = fromSenItemp->varrefp();
        إذا (!fromVarrefp) أرجع خطأ;
        ثابت Ast_ڳنڍ_Var_Ref* ثابت منزل_Varrefp = منزل_Sen_Itemp->varrefp();
        إذا (!منزل_Varrefp) أرجع خطأ;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        إذا (fromVarrefp->varScopep() != منزل_Varrefp->varScopep()) أرجع خطأ;

        أرجع fromSenItemp->ڪنارو_Type().exclusive_ڪنارو(منزل_Sen_Itemp->ڪنارو_Type());
    }

    فراغ جوڙيو_ڪنارو(ترتيب_Move_راس* srcp, ترتيب_Move_راس* dstp) {
        جديد V3_گراف_ڪنارو{m_moveGraphp.get(), srcp, dstp, 1};
    }

    فراغ iterate_Logic_راس(ثابت ترتيب_Logic_راس* lvtxp) {
        ثابت ترتيب_Domain* ثابت domainp = lvtxp->domainp();
        ترتيب_Move_راس* ثابت lMoveVtxp = static_cast<ترتيب_Move_راس*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : lvtxp->ٻاهريون_ڪنارا()) {
            إذا (ڪنارو.وزن_2() == 0) استمر;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ثابت ترتيب_Var_راس* ثابت vvtxp = static_cast<ثابت ترتيب_Var_راس*>(ڪنارو.چوٽي());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ثابت خودڪار pair = mapp.emplace(domainp, خالي_اشارو);
            // Reference to the mapped OrderMoveVertex
            ترتيب_Move_راس*& vMoveVtxp = pair.پهريون->ٻيو;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            إذا (pair.ٻيو) vMoveVtxp = iterate_Var_راس(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            إذا (!vMoveVtxp) استمر;

            // Add this (variable, domain) as dependent of the logic that writes it.
            جوڙيو_ڪنارو(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    ترتيب_Move_راس* iterate_Var_راس(ثابت ترتيب_Var_راس* vvtxp, ثابت ترتيب_Domain* domainp) {
        ترتيب_Move_راس* vMoveVtxp = خالي_اشارو;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vvtxp->ٻاهريون_ڪنارا()) {
            إذا (ڪنارو.وزن_2() == 0) استمر;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ثابت ترتيب_Logic_راس* ثابت lVtxp = ڪنارو.چوٽي()->as<ترتيب_Logic_راس>();

            // Do not construct dependencies across exclusive domains.
            إذا (domainsExclusive(domainp, lVtxp->domainp())) استمر;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            إذا (!vMoveVtxp) vMoveVtxp = جديد ترتيب_Move_راس{*m_moveGraphp, خالي_اشارو, domainp};
            ترتيب_Move_راس* ثابت lMoveVxp = static_cast<ترتيب_Move_راس*>(lVtxp->userp());
            جوڙيو_ڪنارو(vMoveVtxp, lMoveVxp);
        }
        أرجع vMoveVtxp;
    }

عام:
    ساكن std::unique_ptr<ترتيب_Move_گراف> apply(ترتيب_گراف& ترتيب_گراف_2) {
        أرجع std::move(ترتيب_Move_گراف_Builder{ترتيب_گراف_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<ترتيب_Move_گراف> ترتيب_Move_گراف::build(ترتيب_گراف& ترتيب_گراف_2) {
    أرجع ترتيب_Move_گراف_Builder::apply(ترتيب_گراف_2);
}
