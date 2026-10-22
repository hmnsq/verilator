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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMoveGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

ਕ੍ਰਮ_Move_Dom_Scope::DomScopeMap ਕ੍ਰਮ_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* lVtxp,
                                 ਅਚਲ ਕ੍ਰਮ_Domain* domainp) VL_MT_DISABLED
    : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{&ਗ੍ਰਾਫ},
      m_logicp{lVtxp},
      m_domScope{ਕ੍ਰਮ_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : ਖਾਲੀ_ਸੂਚਕ)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

ਵਰਗ ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder final {
    // TYPES
    ਵਰਤੋਂ DomainMap = std::map<ਅਚਲ ਕ੍ਰਮ_Domain*, ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼*>;

    // MEMBERS
    ਕ੍ਰਮ_ਗ੍ਰਾਫ& m_ਕ੍ਰਮ_ਗ੍ਰਾਫ;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ> m_moveGraphp{ਨਵਾਂ ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ{m_ਕ੍ਰਮ_ਗ੍ਰਾਫ}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਕ੍ਰਮ_ਗ੍ਰਾਫ_2)
        : m_ਕ੍ਰਮ_ਗ੍ਰਾਫ{ਕ੍ਰਮ_ਗ੍ਰਾਫ_2} {
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
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਕ੍ਰਮ_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = vtx.cast<ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼>()) {
                lvtxp->userp(ਨਵਾਂ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } ਨਹੀਂਤਾਂ {
                // This is an OrderVarVertex
                m_domainMaps.emplace_ਪਿੱਠ();
                vtx.userp(&m_domainMaps.ਪਿੱਠ());
            }
        }
        // Build edges between logic vertices
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਕ੍ਰਮ_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = vtx.cast<ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼>()) {
                iterate_Logic_ਸ਼ੀਰਸ਼(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_ਕਿਨਾਰੇ_ਜੋੜ(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    ਆਭਾਸੀ ~ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder() = ਮੂਲ;
    VL_UNCOPYABLE(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder);
    VL_UNMOVABLE(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder);

    // METHODS

    ਤਾਰਕਿਕ domainsExclusive(ਅਚਲ ਕ੍ਰਮ_Domain* fromp, ਅਚਲ ਕ੍ਰਮ_Domain* ਸਿਖਰ) {
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

        ਅਚਲ std::ਸੈੱਟ<uint32_t>& fromTriggers = fromp->triggers();
        ਅਚਲ std::ਸੈੱਟ<uint32_t>& ਮੰਜ਼ਿਲ_Triggers = ਸਿਖਰ->triggers();
        ਜੇ (fromTriggers.ਮਾਪ() != 1) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ਮੰਜ਼ਿਲ_Triggers.ਮਾਪ() != 1) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_Sen_ਵਸਤੂ* ਅਚਲ fromSenItemp = m_ਕ੍ਰਮ_ਗ੍ਰਾਫ.trigSenItemp(*fromTriggers.begin());
        ਜੇ (!fromSenItemp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_Sen_ਵਸਤੂ* ਅਚਲ ਮੰਜ਼ਿਲ_Sen_Itemp = m_ਕ੍ਰਮ_ਗ੍ਰਾਫ.trigSenItemp(*ਮੰਜ਼ਿਲ_Triggers.begin());
        ਜੇ (!ਮੰਜ਼ਿਲ_Sen_Itemp) ਵਾਪਸ ਝੂਠ;

        ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ fromVarrefp = fromSenItemp->varrefp();
        ਜੇ (!fromVarrefp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ ਮੰਜ਼ਿਲ_Varrefp = ਮੰਜ਼ਿਲ_Sen_Itemp->varrefp();
        ਜੇ (!ਮੰਜ਼ਿਲ_Varrefp) ਵਾਪਸ ਝੂਠ;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        ਜੇ (fromVarrefp->varScopep() != ਮੰਜ਼ਿਲ_Varrefp->varScopep()) ਵਾਪਸ ਝੂਠ;

        ਵਾਪਸ fromSenItemp->ਕਿਨਾਰਾ_Type().exclusive_ਕਿਨਾਰਾ(ਮੰਜ਼ਿਲ_Sen_Itemp->ਕਿਨਾਰਾ_Type());
    }

    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* srcp, ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* dstp) {
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_moveGraphp.get(), srcp, dstp, 1};
    }

    ਖਾਲੀ iterate_Logic_ਸ਼ੀਰਸ਼(ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* lvtxp) {
        ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ domainp = lvtxp->domainp();
        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ lMoveVtxp = static_cast<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : lvtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() == 0) ਜਾਰੀ;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ਅਚਲ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = static_cast<ਅਚਲ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.ਸਿਖਰ());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = mapp.emplace(domainp, ਖਾਲੀ_ਸੂਚਕ);
            // Reference to the mapped OrderMoveVertex
            ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼*& vMoveVtxp = pair.ਪਹਿਲਾ->ਦੂਜਾ;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            ਜੇ (pair.ਦੂਜਾ) vMoveVtxp = iterate_Var_ਸ਼ੀਰਸ਼(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            ਜੇ (!vMoveVtxp) ਜਾਰੀ;

            // Add this (variable, domain) as dependent of the logic that writes it.
            ਜੋੜੋ_ਕਿਨਾਰਾ(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* iterate_Var_ਸ਼ੀਰਸ਼(ਅਚਲ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* vvtxp, ਅਚਲ ਕ੍ਰਮ_Domain* domainp) {
        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* vMoveVtxp = ਖਾਲੀ_ਸੂਚਕ;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vvtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() == 0) ਜਾਰੀ;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼>();

            // Do not construct dependencies across exclusive domains.
            ਜੇ (domainsExclusive(domainp, lVtxp->domainp())) ਜਾਰੀ;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            ਜੇ (!vMoveVtxp) vMoveVtxp = ਨਵਾਂ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼{*m_moveGraphp, ਖਾਲੀ_ਸੂਚਕ, domainp};
            ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ lMoveVxp = static_cast<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼*>(lVtxp->userp());
            ਜੋੜੋ_ਕਿਨਾਰਾ(vMoveVtxp, lMoveVxp);
        }
        ਵਾਪਸ vMoveVtxp;
    }

ਜਨਤਕ:
    ਸਥਿਰ std::unique_ptr<ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ> apply(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਕ੍ਰਮ_ਗ੍ਰਾਫ_2) {
        ਵਾਪਸ std::move(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder{ਕ੍ਰਮ_ਗ੍ਰਾਫ_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ> ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ::build(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਕ੍ਰਮ_ਗ੍ਰਾਫ_2) {
    ਵਾਪਸ ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Builder::apply(ਕ੍ਰਮ_ਗ੍ਰਾਫ_2);
}
