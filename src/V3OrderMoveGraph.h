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
//  Move graph builder for ordering
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ORDERMOVEGRAPH_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ORDERMOVEGRAPH_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"

class ಕ್ರಮ_Move_Dom_Scope;
class ಕ್ರಮ_Move_ನಕ್ಷೆ;

//======================================================================
// Vertex types

class ಕ್ರಮ_Move_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Move_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ m_logicp;
    ಕ್ರಮ_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<ಕ್ರಮ_Move_ಶೃಂಗ> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        std::string nm;
        ಆದರೆ (!logicp()) {
            nm = "var";
        } ಇಲ್ಲದಿದ್ದರೆ {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvt_ಗಮ್ಯ_Hex(logicp()->domainp());
            nm += +" s=" + cvt_ಗಮ್ಯ_Hex(logicp()->scopep());
        }
        ಆದರೆ (userp()) nm += "\nu=" + cvt_ಗಮ್ಯ_Hex(userp());
        ಹಿಂತಿರುಗಿ nm;
    }

    V3ListLinks<ಕ್ರಮ_Move_ಶೃಂಗ>& links() { ಹಿಂತಿರುಗಿ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    using List = V3List<ಕ್ರಮ_Move_ಶೃಂಗ, &ಕ್ರಮ_Move_ಶೃಂಗ::links>;

    // CONSTRUCTORS
    ಕ್ರಮ_Move_ಶೃಂಗ(ಕ್ರಮ_Move_ನಕ್ಷೆ& ನಕ್ಷೆ, ಕ್ರಮ_Logic_ಶೃಂಗ* lVtxp,
                    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp) VL_MT_DISABLED;
    ~ಕ್ರಮ_Move_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ಕ್ರಮ_Move_ಶೃಂಗ);

    ಕ್ರಮ_Logic_ಶೃಂಗ* logicp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_logicp; }
    ಕ್ರಮ_Move_Dom_Scope& domScope() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class ಕ್ರಮ_Move_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    ಕ್ರಮ_ನಕ್ಷೆ& m_ಕ್ರಮ_ನಕ್ಷೆ;  // The OrderGraph this move graph was built from

ಸಾರ್ವಜನಿಕ:
    explicit ಕ್ರಮ_Move_ನಕ್ಷೆ(ಕ್ರಮ_ನಕ್ಷೆ& ಕ್ರಮ_ನಕ್ಷೆ_2)
        : m_ಕ್ರಮ_ನಕ್ಷೆ{ಕ್ರಮ_ನಕ್ಷೆ_2} {}

    ಕ್ರಮ_ನಕ್ಷೆ& ಕ್ರಮ_ನಕ್ಷೆ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕ್ರಮ_ನಕ್ಷೆ; }

    // Build an OrderMoveGraph from an OrderGraph
    ಸ್ಥಿರ std::unique_ptr<ಕ್ರಮ_Move_ನಕ್ಷೆ> build(ಕ್ರಮ_ನಕ್ಷೆ&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class ಕ್ರಮ_Move_Dom_Scope final {
    // STATE
    ಕ್ರಮ_Move_ಶೃಂಗ::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<ಕ್ರಮ_Move_Dom_Scope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ m_domainp;  // Domain the vertices belong to
    ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_ಕೀಲಿ final {
        ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ m_domainp;  // Domain the vertices belong to
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // Scope the vertices belong to

    ಸಾರ್ವಜನಿಕ:
        Dom_Scope_Map_ಕೀಲಿ(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp, ಸ್ಥಿರಾಂಕ AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        ರಚನೆ Hash final {
            ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dom_Scope_Map_ಕೀಲಿ& ಕೀಲಿ) ಸ್ಥಿರಾಂಕ {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{ಕೀಲಿ.m_domainp};
                hash += ಕೀಲಿ.m_scopep;
                ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
            }
        };

        ರಚನೆ Equal final {
            bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dom_Scope_Map_ಕೀಲಿ& a, ಸ್ಥಿರಾಂಕ Dom_Scope_Map_ಕೀಲಿ& b) ಸ್ಥಿರಾಂಕ {
                ಹಿಂತಿರುಗಿ a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_ಕೀಲಿ, ಕ್ರಮ_Move_Dom_Scope, Dom_Scope_Map_ಕೀಲಿ::Hash,
                                           Dom_Scope_Map_ಕೀಲಿ::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ಸ್ಥಿರ DomScopeMap s_dsMap;

    V3ListLinks<ಕ್ರಮ_Move_Dom_Scope>& links() { ಹಿಂತಿರುಗಿ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    using List = V3List<ಕ್ರಮ_Move_Dom_Scope, &ಕ್ರಮ_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    ಸ್ಥಿರ ಕ್ರಮ_Move_Dom_Scope& getOrCreate(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp, ಸ್ಥಿರಾಂಕ AstScope* scopep) {
        ಹಿಂತಿರುಗಿ s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .ಮೊದಲ->ಎರಡನೇ;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    ಕ್ರಮ_Move_Dom_Scope(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp, ಸ್ಥಿರಾಂಕ AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~ಕ್ರಮ_Move_Dom_Scope() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ಕ್ರಮ_Move_Dom_Scope);
    VL_UNMOVABLE(ಕ್ರಮ_Move_Dom_Scope);

    // MEMBERS
    ಕ್ರಮ_Move_ಶೃಂಗ::List& readyVertices() { ಹಿಂತಿರುಗಿ m_readyVertices; }
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_domainp; }
    ಸ್ಥಿರಾಂಕ AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }

    bool isOnList() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isOnList; }
    ಶೂನ್ಯ isOnList(bool ಮೌಲ್ಯ) { m_isOnList = ಮೌಲ್ಯ; }
};

//======================================================================
// Serializer for OrderMoveGraph

class ಕ್ರಮ_Move_ನಕ್ಷೆ_Serializer final {
    // STATE
    ಕ್ರಮ_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    ಕ್ರಮ_Move_Dom_Scope* m_ಮುಂದಿನ_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    ಶೂನ್ಯ ready(ಕ್ರಮ_Move_ಶೃಂಗ* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        ಆದರೆ (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            ಕ್ರಮ_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_ಹಿಂಭಾಗ(vtxp);
            // Add the DomScope to the global ready list if not there yet
            ಆದರೆ (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_ಹಿಂಭಾಗ(&domScope);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                // The dependent variable
                ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dVtxp = ಅಂಚು.ತುದಿ()->as<ಕ್ರಮ_Move_ಶೃಂಗ>();
                // Update number of dependencies
                ಸ್ಥಿರಾಂಕ uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                ಆದರೆ (!nDeps) ready(dVtxp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    explicit ಕ್ರಮ_Move_ನಕ್ಷೆ_Serializer(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : move_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ uint32_t nDeps = vtx.ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
            vtx.user(nDeps);
        }
    }
    ~ಕ್ರಮ_Move_ನಕ್ಷೆ_Serializer() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ಕ್ರಮ_Move_ನಕ್ಷೆ_Serializer);

    // Add a seed vertex to the ready lists
    ಶೂನ್ಯ ಕೂಡಿಸು_ಬೀಜ(ಕ್ರಮ_Move_ಶೃಂಗ* vtxp) { ready(vtxp); }

    ಕ್ರಮ_Move_ಶೃಂಗ* get_ಮುಂದಿನ() {
        ಆದರೆ (!m_ಮುಂದಿನ_Dom_Scopep) m_ಮುಂದಿನ_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        ಆದರೆ (!m_ಮುಂದಿನ_Dom_Scopep) ಹಿಂತಿರುಗಿ nullptr;
        ಕ್ರಮ_Move_Dom_Scope& currDomScope = *m_ಮುಂದಿನ_Dom_Scopep;

        ಕ್ರಮ_Move_ಶೃಂಗ::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.ಖಾಲಿಗಳು(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        ಕ್ರಮ_Move_ಶೃಂಗ* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        ಆದರೆ (currReadyList.ಖಾಲಿಗಳು()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            // The dependent variable
            ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dVtxp = ಅಂಚು.ತುದಿ()->as<ಕ್ರಮ_Move_ಶೃಂಗ>();
            // Update number of dependencies
            ಸ್ಥಿರಾಂಕ uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            ಆದರೆ (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        ಆದರೆ (currReadyList.ಖಾಲಿಗಳು()) {
            m_ಮುಂದಿನ_Dom_Scopep = nullptr;
            ಗಾಗಿ (ಕ್ರಮ_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                ಆದರೆ (domScope.domainp() == currDomScope.domainp()) {
                    m_ಮುಂದಿನ_Dom_Scopep = &domScope;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }

        // Finally yield the selected vertex
        ಹಿಂತಿರುಗಿ mVtxp;
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
