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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderMoveGraph.h"

#अन्तर्भाव "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

क्रमः_Move_Dom_Scope::DomScopeMap क्रमः_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

क्रमः_Move_Vertex::क्रमः_Move_Vertex(क्रमः_Move_Graph& graph, क्रमः_Logic_Vertex* lVtxp,
                                 ध्रुव क्रमः_Domain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{क्रमः_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : शून्यसूचकः)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

वर्ग क्रमः_Move_Graph_Builder final {
    // TYPES
    उपयुज्य DomainMap = std::map<ध्रुव क्रमः_Domain*, क्रमः_Move_Vertex*>;

    // MEMBERS
    क्रमः_Graph& m_क्रमः_Graph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<क्रमः_Move_Graph> m_moveGraphp{नूतन क्रमः_Move_Graph{m_क्रमः_Graph}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    स्पष्ट क्रमः_Move_Graph_Builder(क्रमः_Graph& क्रमः_Graph_2)
        : m_क्रमः_Graph{क्रमः_Graph_2} {
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
        प्रति (V3GraphVertex& vtx : m_क्रमः_Graph.vertices()) {
            यदि (क्रमः_Logic_Vertex* ध्रुव lvtxp = vtx.cast<क्रमः_Logic_Vertex>()) {
                lvtxp->userp(नूतन क्रमः_Move_Vertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } अन्यथा {
                // This is an OrderVarVertex
                m_domainMaps.emplace_पृष्ठभागः();
                vtx.userp(&m_domainMaps.पृष्ठभागः());
            }
        }
        // Build edges between logic vertices
        प्रति (V3GraphVertex& vtx : m_क्रमः_Graph.vertices()) {
            यदि (क्रमः_Logic_Vertex* ध्रुव lvtxp = vtx.cast<क्रमः_Logic_Vertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_धाराः_योग(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    आभास ~क्रमः_Move_Graph_Builder() = स्वभाव;
    VL_UNCOPYABLE(क्रमः_Move_Graph_Builder);
    VL_UNMOVABLE(क्रमः_Move_Graph_Builder);

    // METHODS

    तार्किक domainsExclusive(ध्रुव क्रमः_Domain* fromp, ध्रुव क्रमः_Domain* शिखरम्) {
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

        ध्रुव std::समुच्चयः<uint32_t>& fromTriggers = fromp->triggers();
        ध्रुव std::समुच्चयः<uint32_t>& गन्तव्यम्_Triggers = शिखरम्->triggers();
        यदि (fromTriggers.आकार() != 1) प्रत्यागम असत्य;
        यदि (गन्तव्यम्_Triggers.आकार() != 1) प्रत्यागम असत्य;
        ध्रुव AstSenItem* ध्रुव fromSenItemp = m_क्रमः_Graph.trigSenItemp(*fromTriggers.begin());
        यदि (!fromSenItemp) प्रत्यागम असत्य;
        ध्रुव AstSenItem* ध्रुव गन्तव्यम्_Sen_Itemp = m_क्रमः_Graph.trigSenItemp(*गन्तव्यम्_Triggers.begin());
        यदि (!गन्तव्यम्_Sen_Itemp) प्रत्यागम असत्य;

        ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव fromVarrefp = fromSenItemp->varrefp();
        यदि (!fromVarrefp) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव गन्तव्यम्_Varrefp = गन्तव्यम्_Sen_Itemp->varrefp();
        यदि (!गन्तव्यम्_Varrefp) प्रत्यागम असत्य;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        यदि (fromVarrefp->varScopep() != गन्तव्यम्_Varrefp->varScopep()) प्रत्यागम असत्य;

        प्रत्यागम fromSenItemp->edgeType().exclusiveEdge(गन्तव्यम्_Sen_Itemp->edgeType());
    }

    शून्य सङ्कलय_Edge(क्रमः_Move_Vertex* srcp, क्रमः_Move_Vertex* dstp) {
        नूतन V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    शून्य iterateLogicVertex(ध्रुव क्रमः_Logic_Vertex* lvtxp) {
        ध्रुव क्रमः_Domain* ध्रुव domainp = lvtxp->domainp();
        क्रमः_Move_Vertex* ध्रुव lMoveVtxp = static_cast<क्रमः_Move_Vertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        प्रति (ध्रुव V3GraphEdge& edge : lvtxp->बहिर्गमनम्_धाराः()) {
            यदि (edge.weight() == 0) अनुवर्त;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ध्रुव क्रमः_Var_Vertex* ध्रुव vvtxp = static_cast<ध्रुव क्रमः_Var_Vertex*>(edge.शिखरम्());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ध्रुव स्वचालित pair = mapp.emplace(domainp, शून्यसूचकः);
            // Reference to the mapped OrderMoveVertex
            क्रमः_Move_Vertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            यदि (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            यदि (!vMoveVtxp) अनुवर्त;

            // Add this (variable, domain) as dependent of the logic that writes it.
            सङ्कलय_Edge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    क्रमः_Move_Vertex* iterateVarVertex(ध्रुव क्रमः_Var_Vertex* vvtxp, ध्रुव क्रमः_Domain* domainp) {
        क्रमः_Move_Vertex* vMoveVtxp = शून्यसूचकः;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        प्रति (ध्रुव V3GraphEdge& edge : vvtxp->बहिर्गमनम्_धाराः()) {
            यदि (edge.weight() == 0) अनुवर्त;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ध्रुव क्रमः_Logic_Vertex* ध्रुव lVtxp = edge.शिखरम्()->as<क्रमः_Logic_Vertex>();

            // Do not construct dependencies across exclusive domains.
            यदि (domainsExclusive(domainp, lVtxp->domainp())) अनुवर्त;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            यदि (!vMoveVtxp) vMoveVtxp = नूतन क्रमः_Move_Vertex{*m_moveGraphp, शून्यसूचकः, domainp};
            क्रमः_Move_Vertex* ध्रुव lMoveVxp = static_cast<क्रमः_Move_Vertex*>(lVtxp->userp());
            सङ्कलय_Edge(vMoveVtxp, lMoveVxp);
        }
        प्रत्यागम vMoveVtxp;
    }

सार्वजनिक:
    स्थिर std::unique_ptr<क्रमः_Move_Graph> apply(क्रमः_Graph& क्रमः_Graph_2) {
        प्रत्यागम std::move(क्रमः_Move_Graph_Builder{क्रमः_Graph_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<क्रमः_Move_Graph> क्रमः_Move_Graph::build(क्रमः_Graph& क्रमः_Graph_2) {
    प्रत्यागम क्रमः_Move_Graph_Builder::apply(क्रमः_Graph_2);
}
