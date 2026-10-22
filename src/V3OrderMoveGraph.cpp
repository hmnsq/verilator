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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderMoveGraph.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

ಕ್ರಮ_Move_Dom_Scope::DomScopeMap ಕ್ರಮ_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

ಕ್ರಮ_Move_ಶೃಂಗ::ಕ್ರಮ_Move_ಶೃಂಗ(ಕ್ರಮ_Move_ನಕ್ಷೆ& ನಕ್ಷೆ, ಕ್ರಮ_Logic_ಶೃಂಗ* lVtxp,
                                 ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp) VL_MT_DISABLED
    : V3_ನಕ್ಷೆ_ಶೃಂಗ{&ನಕ್ಷೆ},
      m_logicp{lVtxp},
      m_domScope{ಕ್ರಮ_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : ಶೂನ್ಯಸೂಚಕ)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

ವರ್ಗ ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder final {
    // TYPES
    ಬಳಸಿ DomainMap = std::map<ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain*, ಕ್ರಮ_Move_ಶೃಂಗ*>;

    // MEMBERS
    ಕ್ರಮ_ನಕ್ಷೆ& m_ಕ್ರಮ_ನಕ್ಷೆ;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<ಕ್ರಮ_Move_ನಕ್ಷೆ> m_moveGraphp{ಹೊಸ ಕ್ರಮ_Move_ನಕ್ಷೆ{m_ಕ್ರಮ_ನಕ್ಷೆ}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder(ಕ್ರಮ_ನಕ್ಷೆ& ಕ್ರಮ_ನಕ್ಷೆ_2)
        : m_ಕ್ರಮ_ನಕ್ಷೆ{ಕ್ರಮ_ನಕ್ಷೆ_2} {
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
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ಕ್ರಮ_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = vtx.cast<ಕ್ರಮ_Logic_ಶೃಂಗ>()) {
                lvtxp->userp(ಹೊಸ ಕ್ರಮ_Move_ಶೃಂಗ{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // This is an OrderVarVertex
                m_domainMaps.emplace_ಹಿಂಭಾಗ();
                vtx.userp(&m_domainMaps.ಹಿಂಭಾಗ());
            }
        }
        // Build edges between logic vertices
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ಕ್ರಮ_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = vtx.cast<ಕ್ರಮ_Logic_ಶೃಂಗ>()) {
                iterate_Logic_ಶೃಂಗ(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_ಅಂಚುಗಳು_ಮೊತ್ತ(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    ವಾಸ್ತವಿಕ ~ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder);
    VL_UNMOVABLE(ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder);

    // METHODS

    ತಾರ್ಕಿಕ domainsExclusive(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* fromp, ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ತುದಿ) {
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

        ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& fromTriggers = fromp->triggers();
        ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& ಗಮ್ಯ_Triggers = ತುದಿ->triggers();
        ಆದರೆ (fromTriggers.ಗಾತ್ರ_2() != 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ಗಮ್ಯ_Triggers.ಗಾತ್ರ_2() != 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ fromSenItemp = m_ಕ್ರಮ_ನಕ್ಷೆ.trigSenItemp(*fromTriggers.begin());
        ಆದರೆ (!fromSenItemp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Sen_Itemp = m_ಕ್ರಮ_ನಕ್ಷೆ.trigSenItemp(*ಗಮ್ಯ_Triggers.begin());
        ಆದರೆ (!ಗಮ್ಯ_Sen_Itemp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ fromVarrefp = fromSenItemp->varrefp();
        ಆದರೆ (!fromVarrefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Varrefp = ಗಮ್ಯ_Sen_Itemp->varrefp();
        ಆದರೆ (!ಗಮ್ಯ_Varrefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        ಆದರೆ (fromVarrefp->varScopep() != ಗಮ್ಯ_Varrefp->varScopep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಹಿಂತಿರುಗಿ fromSenItemp->ಅಂಚು_Type().exclusive_ಅಂಚು(ಗಮ್ಯ_Sen_Itemp->ಅಂಚು_Type());
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(ಕ್ರಮ_Move_ಶೃಂಗ* srcp, ಕ್ರಮ_Move_ಶೃಂಗ* dstp) {
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_moveGraphp.get(), srcp, dstp, 1};
    }

    ಶೂನ್ಯ iterate_Logic_ಶೃಂಗ(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* lvtxp) {
        ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ domainp = lvtxp->domainp();
        ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lMoveVtxp = static_cast<ಕ್ರಮ_Move_ಶೃಂಗ*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : lvtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (ಅಂಚು.ತೂಕ_2() == 0) ಮುಂದುವರಿ;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = static_cast<ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Var_ಶೃಂಗ*>(ಅಂಚು.ತುದಿ());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = mapp.emplace(domainp, ಶೂನ್ಯಸೂಚಕ);
            // Reference to the mapped OrderMoveVertex
            ಕ್ರಮ_Move_ಶೃಂಗ*& vMoveVtxp = pair.ಮೊದಲ->ಎರಡನೇ;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            ಆದರೆ (pair.ಎರಡನೇ) vMoveVtxp = iterate_Var_ಶೃಂಗ(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            ಆದರೆ (!vMoveVtxp) ಮುಂದುವರಿ;

            // Add this (variable, domain) as dependent of the logic that writes it.
            ಕೂಡಿಸು_ಅಂಚು(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    ಕ್ರಮ_Move_ಶೃಂಗ* iterate_Var_ಶೃಂಗ(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Var_ಶೃಂಗ* vvtxp, ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp) {
        ಕ್ರಮ_Move_ಶೃಂಗ* vMoveVtxp = ಶೂನ್ಯಸೂಚಕ;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vvtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (ಅಂಚು.ತೂಕ_2() == 0) ಮುಂದುವರಿ;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = ಅಂಚು.ತುದಿ()->as<ಕ್ರಮ_Logic_ಶೃಂಗ>();

            // Do not construct dependencies across exclusive domains.
            ಆದರೆ (domainsExclusive(domainp, lVtxp->domainp())) ಮುಂದುವರಿ;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            ಆದರೆ (!vMoveVtxp) vMoveVtxp = ಹೊಸ ಕ್ರಮ_Move_ಶೃಂಗ{*m_moveGraphp, ಶೂನ್ಯಸೂಚಕ, domainp};
            ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lMoveVxp = static_cast<ಕ್ರಮ_Move_ಶೃಂಗ*>(lVtxp->userp());
            ಕೂಡಿಸು_ಅಂಚು(vMoveVtxp, lMoveVxp);
        }
        ಹಿಂತಿರುಗಿ vMoveVtxp;
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<ಕ್ರಮ_Move_ನಕ್ಷೆ> apply(ಕ್ರಮ_ನಕ್ಷೆ& ಕ್ರಮ_ನಕ್ಷೆ_2) {
        ಹಿಂತಿರುಗಿ std::move(ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder{ಕ್ರಮ_ನಕ್ಷೆ_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<ಕ್ರಮ_Move_ನಕ್ಷೆ> ಕ್ರಮ_Move_ನಕ್ಷೆ::build(ಕ್ರಮ_ನಕ್ಷೆ& ಕ್ರಮ_ನಕ್ಷೆ_2) {
    ಹಿಂತಿರುಗಿ ಕ್ರಮ_Move_ನಕ್ಷೆ_Builder::apply(ಕ್ರಮ_ನಕ್ಷೆ_2);
}
