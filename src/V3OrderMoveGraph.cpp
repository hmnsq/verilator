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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3OrderMoveGraph.h"

#شامل "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

ترتیب_Move_Dom_Scope::DomScopeMap ترتیب_Move_Dom_Scope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

ترتیب_Move_راس::ترتیب_Move_راس(ترتیب_Move_گراف& گراف, ترتیب_Logic_راس* lVtxp,
                                 ثابت ترتیب_Domain* domainp) VL_MT_DISABLED
    : V3_گراف_راس{&گراف},
      m_logicp{lVtxp},
      m_domScope{ترتیب_Move_Dom_Scope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : خالی_اشاریہ)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

جماعت ترتیب_Move_گراف_Builder final {
    // TYPES
    استعمال DomainMap = std::map<ثابت ترتیب_Domain*, ترتیب_Move_راس*>;

    // MEMBERS
    ترتیب_گراف& m_ترتیب_گراف;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<ترتیب_Move_گراف> m_moveGraphp{نیا ترتیب_Move_گراف{m_ترتیب_گراف}};
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
        برائے (V3_گراف_راس& vtx : m_ترتیب_گراف.vertices()) {
            اگر (ترتیب_Logic_راس* ثابت lvtxp = vtx.cast<ترتیب_Logic_راس>()) {
                lvtxp->userp(نیا ترتیب_Move_راس{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } ورنہ {
                // This is an OrderVarVertex
                m_domainMaps.emplace_عقب();
                vtx.userp(&m_domainMaps.عقب());
            }
        }
        // Build edges between logic vertices
        برائے (V3_گراف_راس& vtx : m_ترتیب_گراف.vertices()) {
            اگر (ترتیب_Logic_راس* ثابت lvtxp = vtx.cast<ترتیب_Logic_راس>()) {
                iterate_Logic_راس(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_کنارے_مجموع(&V3_گراف_کنارہ::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    مجازی ~ترتیب_Move_گراف_Builder() = طےشدہ;
    VL_UNCOPYABLE(ترتیب_Move_گراف_Builder);
    VL_UNMOVABLE(ترتیب_Move_گراف_Builder);

    // METHODS

    منطقی domainsExclusive(ثابت ترتیب_Domain* fromp, ثابت ترتیب_Domain* چوٹی) {
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

        ثابت std::سیٹ<uint32_t>& fromTriggers = fromp->triggers();
        ثابت std::سیٹ<uint32_t>& مقصد_Triggers = چوٹی->triggers();
        اگر (fromTriggers.سائز() != 1) واپس جھوٹ;
        اگر (مقصد_Triggers.سائز() != 1) واپس جھوٹ;
        ثابت Ast_Sen_شے* ثابت fromSenItemp = m_ترتیب_گراف.trigSenItemp(*fromTriggers.begin());
        اگر (!fromSenItemp) واپس جھوٹ;
        ثابت Ast_Sen_شے* ثابت مقصد_Sen_Itemp = m_ترتیب_گراف.trigSenItemp(*مقصد_Triggers.begin());
        اگر (!مقصد_Sen_Itemp) واپس جھوٹ;

        ثابت Ast_گرہ_Var_Ref* ثابت fromVarrefp = fromSenItemp->varrefp();
        اگر (!fromVarrefp) واپس جھوٹ;
        ثابت Ast_گرہ_Var_Ref* ثابت مقصد_Varrefp = مقصد_Sen_Itemp->varrefp();
        اگر (!مقصد_Varrefp) واپس جھوٹ;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        اگر (fromVarrefp->varScopep() != مقصد_Varrefp->varScopep()) واپس جھوٹ;

        واپس fromSenItemp->کنارہ_Type().exclusive_کنارہ(مقصد_Sen_Itemp->کنارہ_Type());
    }

    خالی جوڑو_کنارہ(ترتیب_Move_راس* srcp, ترتیب_Move_راس* dstp) {
        نیا V3_گراف_کنارہ{m_moveGraphp.get(), srcp, dstp, 1};
    }

    خالی iterate_Logic_راس(ثابت ترتیب_Logic_راس* lvtxp) {
        ثابت ترتیب_Domain* ثابت domainp = lvtxp->domainp();
        ترتیب_Move_راس* ثابت lMoveVtxp = static_cast<ترتیب_Move_راس*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        برائے (ثابت V3_گراف_کنارہ& کنارہ : lvtxp->برآمد_کنارے()) {
            اگر (کنارہ.وزن_2() == 0) جاری;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ثابت ترتیب_Var_راس* ثابت vvtxp = static_cast<ثابت ترتیب_Var_راس*>(کنارہ.چوٹی());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ثابت خودکار pair = mapp.emplace(domainp, خالی_اشاریہ);
            // Reference to the mapped OrderMoveVertex
            ترتیب_Move_راس*& vMoveVtxp = pair.پہلا->دوسرا;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            اگر (pair.دوسرا) vMoveVtxp = iterate_Var_راس(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            اگر (!vMoveVtxp) جاری;

            // Add this (variable, domain) as dependent of the logic that writes it.
            جوڑو_کنارہ(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    ترتیب_Move_راس* iterate_Var_راس(ثابت ترتیب_Var_راس* vvtxp, ثابت ترتیب_Domain* domainp) {
        ترتیب_Move_راس* vMoveVtxp = خالی_اشاریہ;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        برائے (ثابت V3_گراف_کنارہ& کنارہ : vvtxp->برآمد_کنارے()) {
            اگر (کنارہ.وزن_2() == 0) جاری;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ثابت ترتیب_Logic_راس* ثابت lVtxp = کنارہ.چوٹی()->as<ترتیب_Logic_راس>();

            // Do not construct dependencies across exclusive domains.
            اگر (domainsExclusive(domainp, lVtxp->domainp())) جاری;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            اگر (!vMoveVtxp) vMoveVtxp = نیا ترتیب_Move_راس{*m_moveGraphp, خالی_اشاریہ, domainp};
            ترتیب_Move_راس* ثابت lMoveVxp = static_cast<ترتیب_Move_راس*>(lVtxp->userp());
            جوڑو_کنارہ(vMoveVtxp, lMoveVxp);
        }
        واپس vMoveVtxp;
    }

عوامی:
    ساکن std::unique_ptr<ترتیب_Move_گراف> apply(ترتیب_گراف& ترتیب_گراف_2) {
        واپس std::move(ترتیب_Move_گراف_Builder{ترتیب_گراف_2}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<ترتیب_Move_گراف> ترتیب_Move_گراف::build(ترتیب_گراف& ترتیب_گراف_2) {
    واپس ترتیب_Move_گراف_Builder::apply(ترتیب_گراف_2);
}
