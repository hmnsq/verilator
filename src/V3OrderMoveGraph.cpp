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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3OrderMoveGraph.h"

#అన్తర్భావ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

క్రమం_Move_Dom_Scope::DomScopeMap క్రమం_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

క్రమం_Move_శీర్షం::క్రమం_Move_శీర్షం(క్రమం_Move_లేఖాచిత్రం& లేఖాచిత్రం, క్రమం_Logic_శీర్షం* lVtxp,
                                 స్థిరరాశి క్రమం_Domain* domainp) VL_MT_DISABLED
    : V3_లేఖాచిత్రం_శీర్షం{&లేఖాచిత్రం},
      m_logicp{lVtxp},
      m_domScope{క్రమం_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : శూన్యసూచిక)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

తరగతి క్రమం_Move_లేఖాచిత్రం_Builder final {
    // TYPES
    ఉపయోగించి DomainMap = std::map<స్థిరరాశి క్రమం_Domain*, క్రమం_Move_శీర్షం*>;

    // MEMBERS
    క్రమం_లేఖాచిత్రం& m_క్రమం_లేఖాచిత్రం;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<క్రమం_Move_లేఖాచిత్రం> m_moveGraphp{కొత్త క్రమం_Move_లేఖాచిత్రం{m_క్రమం_లేఖాచిత్రం}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    స్పష్టమైన క్రమం_Move_లేఖాచిత్రం_Builder(క్రమం_లేఖాచిత్రం& క్రమం_లేఖాచిత్రం_2)
        : m_క్రమం_లేఖాచిత్రం{క్రమం_లేఖాచిత్రం_2} {
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
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_క్రమం_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (క్రమం_Logic_శీర్షం* స్థిరరాశి lvtxp = vtx.cast<క్రమం_Logic_శీర్షం>()) {
                lvtxp->userp(కొత్త క్రమం_Move_శీర్షం{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } లేకపోతే {
                // This is an OrderVarVertex
                m_domainMaps.emplace_వెనుక();
                vtx.userp(&m_domainMaps.వెనుక());
            }
        }
        // Build edges between logic vertices
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_క్రమం_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (క్రమం_Logic_శీర్షం* స్థిరరాశి lvtxp = vtx.cast<క్రమం_Logic_శీర్షం>()) {
                iterate_Logic_శీర్షం(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_అంచులు_కూడిక(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    మాయా ~క్రమం_Move_లేఖాచిత్రం_Builder() = అప్రమేయ;
    VL_UNCOPYABLE(క్రమం_Move_లేఖాచిత్రం_Builder);
    VL_UNMOVABLE(క్రమం_Move_లేఖాచిత్రం_Builder);

    // METHODS

    తార్కిక domainsExclusive(స్థిరరాశి క్రమం_Domain* fromp, స్థిరరాశి క్రమం_Domain* శిఖరం) {
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

        స్థిరరాశి std::సమితి<uint32_t>& fromTriggers = fromp->triggers();
        స్థిరరాశి std::సమితి<uint32_t>& గమ్యం_Triggers = శిఖరం->triggers();
        ఒకవేళ (fromTriggers.కొలత() != 1) తిరిగి అబద్ధం;
        ఒకవేళ (గమ్యం_Triggers.కొలత() != 1) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_Sen_అంశం* స్థిరరాశి fromSenItemp = m_క్రమం_లేఖాచిత్రం.trigSenItemp(*fromTriggers.begin());
        ఒకవేళ (!fromSenItemp) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_Sen_అంశం* స్థిరరాశి గమ్యం_Sen_Itemp = m_క్రమం_లేఖాచిత్రం.trigSenItemp(*గమ్యం_Triggers.begin());
        ఒకవేళ (!గమ్యం_Sen_Itemp) తిరిగి అబద్ధం;

        స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి fromVarrefp = fromSenItemp->varrefp();
        ఒకవేళ (!fromVarrefp) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి గమ్యం_Varrefp = గమ్యం_Sen_Itemp->varrefp();
        ఒకవేళ (!గమ్యం_Varrefp) తిరిగి అబద్ధం;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        ఒకవేళ (fromVarrefp->varScopep() != గమ్యం_Varrefp->varScopep()) తిరిగి అబద్ధం;

        తిరిగి fromSenItemp->అంచు_Type().exclusive_అంచు(గమ్యం_Sen_Itemp->అంచు_Type());
    }

    శూన్య కూడు_అంచు(క్రమం_Move_శీర్షం* srcp, క్రమం_Move_శీర్షం* dstp) {
        కొత్త V3_లేఖాచిత్రం_అంచు{m_moveGraphp.get(), srcp, dstp, 1};
    }

    శూన్య iterate_Logic_శీర్షం(స్థిరరాశి క్రమం_Logic_శీర్షం* lvtxp) {
        స్థిరరాశి క్రమం_Domain* స్థిరరాశి domainp = lvtxp->domainp();
        క్రమం_Move_శీర్షం* స్థిరరాశి lMoveVtxp = static_cast<క్రమం_Move_శీర్షం*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : lvtxp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (అంచు.బరువు_2() == 0) కొనసాగు;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            స్థిరరాశి క్రమం_Var_శీర్షం* స్థిరరాశి vvtxp = static_cast<స్థిరరాశి క్రమం_Var_శీర్షం*>(అంచు.శిఖరం());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            స్థిరరాశి స్వయంచాలక pair = mapp.emplace(domainp, శూన్యసూచిక);
            // Reference to the mapped OrderMoveVertex
            క్రమం_Move_శీర్షం*& vMoveVtxp = pair.మొదటి->రెండవ;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            ఒకవేళ (pair.రెండవ) vMoveVtxp = iterate_Var_శీర్షం(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            ఒకవేళ (!vMoveVtxp) కొనసాగు;

            // Add this (variable, domain) as dependent of the logic that writes it.
            కూడు_అంచు(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    క్రమం_Move_శీర్షం* iterate_Var_శీర్షం(స్థిరరాశి క్రమం_Var_శీర్షం* vvtxp, స్థిరరాశి క్రమం_Domain* domainp) {
        క్రమం_Move_శీర్షం* vMoveVtxp = శూన్యసూచిక;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vvtxp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (అంచు.బరువు_2() == 0) కొనసాగు;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            స్థిరరాశి క్రమం_Logic_శీర్షం* స్థిరరాశి lVtxp = అంచు.శిఖరం()->as<క్రమం_Logic_శీర్షం>();

            // Do not construct dependencies across exclusive domains.
            ఒకవేళ (domainsExclusive(domainp, lVtxp->domainp())) కొనసాగు;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            ఒకవేళ (!vMoveVtxp) vMoveVtxp = కొత్త క్రమం_Move_శీర్షం{*m_moveGraphp, శూన్యసూచిక, domainp};
            క్రమం_Move_శీర్షం* స్థిరరాశి lMoveVxp = static_cast<క్రమం_Move_శీర్షం*>(lVtxp->userp());
            కూడు_అంచు(vMoveVtxp, lMoveVxp);
        }
        తిరిగి vMoveVtxp;
    }

బహిరంగ:
    స్థిర std::unique_ptr<క్రమం_Move_లేఖాచిత్రం> apply(క్రమం_లేఖాచిత్రం& క్రమం_లేఖాచిత్రం_2) {
        తిరిగి std::move(క్రమం_Move_లేఖాచిత్రం_Builder{క్రమం_లేఖాచిత్రం_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<క్రమం_Move_లేఖాచిత్రం> క్రమం_Move_లేఖాచిత్రం::build(క్రమం_లేఖాచిత్రం& క్రమం_లేఖాచిత్రం_2) {
    తిరిగి క్రమం_Move_లేఖాచిత్రం_Builder::apply(క్రమం_లేఖాచిత్రం_2);
}
