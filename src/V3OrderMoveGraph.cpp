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

ترتیب_Move_رأس::ترتیب_Move_رأس(ترتیب_Move_گراف& گراف, ترتیب_Logic_رأس* lVtxp,
                                 ثابت ترتیب_Domain* domainp) VL_MT_DISABLED
    : V3_گراف_رأس{&گراف},
      m_logicp{lVtxp},
      m_domScope{ترتیب_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : اشاره_تهی)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

صنف ترتیب_Move_گراف_Builder final {
    // TYPES
    استفاده DomainMap = std::map<ثابت ترتیب_Domain*, ترتیب_Move_رأس*>;

    // MEMBERS
    ترتیب_گراف& m_ترتیب_گراف;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<ترتیب_Move_گراف> m_moveGraphp{جديد ترتیب_Move_گراف{m_ترتیب_گراف}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    صریح ترتیب_Move_گراف_Builder(ترتیب_گراف& ترتیب_گراف_2)
        : m_ترتیب_گراف{ترتیب_گراف_2} {
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
        لكل (V3_گراف_رأس& vtx : m_ترتیب_گراف.vertices()) {
            إذا (ترتیب_Logic_رأس* ثابت lvtxp = vtx.cast<ترتیب_Logic_رأس>()) {
                lvtxp->userp(جديد ترتیب_Move_رأس{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } وإلا {
                // This is an OrderVarVertex
                m_domainMaps.emplace_پشت();
                vtx.userp(&m_domainMaps.پشت());
            }
        }
        // Build edges between logic vertices
        لكل (V3_گراف_رأس& vtx : m_ترتیب_گراف.vertices()) {
            إذا (ترتیب_Logic_رأس* ثابت lvtxp = vtx.cast<ترتیب_Logic_رأس>()) {
                iterate_Logic_رأس(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_یالها_جمع(&V3_گراف_یال::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    مجازی ~ترتیب_Move_گراف_Builder() = افتراضي;
    VL_UNCOPYABLE(ترتیب_Move_گراف_Builder);
    VL_UNMOVABLE(ترتیب_Move_گراف_Builder);

    // METHODS

    منطقي domainsExclusive(ثابت ترتیب_Domain* fromp, ثابت ترتیب_Domain* بالاترین) {
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

        ثابت std::مجموعه<uint32_t>& fromTriggers = fromp->triggers();
        ثابت std::مجموعه<uint32_t>& مقصد_Triggers = بالاترین->triggers();
        إذا (fromTriggers.اندازه() != 1) أرجع خطأ;
        إذا (مقصد_Triggers.اندازه() != 1) أرجع خطأ;
        ثابت Ast_Sen_قلم* ثابت fromSenItemp = m_ترتیب_گراف.trigSenItemp(*fromTriggers.begin());
        إذا (!fromSenItemp) أرجع خطأ;
        ثابت Ast_Sen_قلم* ثابت مقصد_Sen_Itemp = m_ترتیب_گراف.trigSenItemp(*مقصد_Triggers.begin());
        إذا (!مقصد_Sen_Itemp) أرجع خطأ;

        ثابت Ast_گره_Var_Ref* ثابت fromVarrefp = fromSenItemp->varrefp();
        إذا (!fromVarrefp) أرجع خطأ;
        ثابت Ast_گره_Var_Ref* ثابت مقصد_Varrefp = مقصد_Sen_Itemp->varrefp();
        إذا (!مقصد_Varrefp) أرجع خطأ;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        إذا (fromVarrefp->varScopep() != مقصد_Varrefp->varScopep()) أرجع خطأ;

        أرجع fromSenItemp->یال_Type().exclusive_یال(مقصد_Sen_Itemp->یال_Type());
    }

    فراغ جمع_کن_یال(ترتیب_Move_رأس* srcp, ترتیب_Move_رأس* dstp) {
        جديد V3_گراف_یال{m_moveGraphp.get(), srcp, dstp, 1};
    }

    فراغ iterate_Logic_رأس(ثابت ترتیب_Logic_رأس* lvtxp) {
        ثابت ترتیب_Domain* ثابت domainp = lvtxp->domainp();
        ترتیب_Move_رأس* ثابت lMoveVtxp = static_cast<ترتیب_Move_رأس*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        لكل (ثابت V3_گراف_یال& یال : lvtxp->بیرون_یالها()) {
            إذا (یال.وزن_2() == 0) استمر;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ثابت ترتیب_Var_رأس* ثابت vvtxp = static_cast<ثابت ترتیب_Var_رأس*>(یال.بالاترین());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ثابت خودکار pair = mapp.emplace(domainp, اشاره_تهی);
            // Reference to the mapped OrderMoveVertex
            ترتیب_Move_رأس*& vMoveVtxp = pair.اولی->دومی;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            إذا (pair.دومی) vMoveVtxp = iterate_Var_رأس(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            إذا (!vMoveVtxp) استمر;

            // Add this (variable, domain) as dependent of the logic that writes it.
            جمع_کن_یال(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    ترتیب_Move_رأس* iterate_Var_رأس(ثابت ترتیب_Var_رأس* vvtxp, ثابت ترتیب_Domain* domainp) {
        ترتیب_Move_رأس* vMoveVtxp = اشاره_تهی;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        لكل (ثابت V3_گراف_یال& یال : vvtxp->بیرون_یالها()) {
            إذا (یال.وزن_2() == 0) استمر;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ثابت ترتیب_Logic_رأس* ثابت lVtxp = یال.بالاترین()->as<ترتیب_Logic_رأس>();

            // Do not construct dependencies across exclusive domains.
            إذا (domainsExclusive(domainp, lVtxp->domainp())) استمر;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            إذا (!vMoveVtxp) vMoveVtxp = جديد ترتیب_Move_رأس{*m_moveGraphp, اشاره_تهی, domainp};
            ترتیب_Move_رأس* ثابت lMoveVxp = static_cast<ترتیب_Move_رأس*>(lVtxp->userp());
            جمع_کن_یال(vMoveVtxp, lMoveVxp);
        }
        أرجع vMoveVtxp;
    }

عام:
    ساكن std::unique_ptr<ترتیب_Move_گراف> apply(ترتیب_گراف& ترتیب_گراف_2) {
        أرجع std::move(ترتیب_Move_گراف_Builder{ترتیب_گراف_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<ترتیب_Move_گراف> ترتیب_Move_گراف::build(ترتیب_گراف& ترتیب_گراف_2) {
    أرجع ترتیب_Move_گراف_Builder::apply(ترتیب_گراف_2);
}
