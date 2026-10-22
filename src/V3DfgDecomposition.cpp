// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: DfgGraph decomposition algorithms
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
//  Algorithms that take a DfgGraph and decompose it into multiple DfgGraphs.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ ವಿಭಜಿಸು_Into_Components_2 final {

    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<ಗಾತ್ರ_t> m_component = m_dfg.makeUserMap<ಗಾತ್ರ_t>();
    ಸ್ಥಿರಾಂಕ std::string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Component name prefix
    std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    ಗಾತ್ರ_t m_componentCounter = 1;

    ಶೂನ್ಯ colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::ಸದಿಶ<Dfg_ಶೃಂಗ*> ಸರತಿ;
        ಸರತಿ.reserve(m_dfg.ಗಾತ್ರ_2());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            ಆದರೆ (m_component[vtx]) ಮುಂದುವರಿ;

            // Start depth first traversal at this vertex
            ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);

            // Depth first traversal
            ಮಾಡು {
                // Pop next work item
                Dfg_ಶೃಂಗ& ಅಂಶ_2 = *ಸರತಿ.ಹಿಂಭಾಗ();
                ಸರತಿ.ತೆಗೆ_ಹಿಂಭಾಗ();

                // Move on if already visited
                ಆದರೆ (m_component[ಅಂಶ_2]) ಮುಂದುವರಿ;

                // Assign to current component
                m_component[ಅಂಶ_2] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                ಅಂಶ_2.foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                    ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&src);
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                });
                ಅಂಶ_2.foreachSink([&](Dfg_ಶೃಂಗ& dst) {
                    ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&dst);
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                });
            } ಆಗ (!ಸರತಿ.ಖಾಲಿಗಳು());

            // Done with this component
            ++m_componentCounter;
        }
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    ಶೂನ್ಯ moveVertices(Dfg_ಶೃಂಗ::List<ಶೃಂಗ_2>& list) {
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : list.unlinkable()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t component = m_component[vtxp]) {
                m_dfg.remove_ಶೃಂಗ(*vtxp);
                m_components[component - 1]->ಕೂಡಿಸು_ಶೃಂಗ(*vtxp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    ವಿಭಜಿಸು_Into_Components_2(Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ std::string& label)
        : m_dfg{dfg}
        , m_ಪೂರ್ವಪ್ರತ್ಯಯ{dfg.name() + (label.ಖಾಲಿಗಳು() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(ಹೊಸ Dfg_ನಕ್ಷೆ{m_dfg.ctx(), m_ಪೂರ್ವಪ್ರತ್ಯಯ + cvt_ಗಮ್ಯ_Str(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.ಗಾತ್ರ_2() == 0, "'this' DfgGraph should have been emptied");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> apply(Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ std::string& label) {
        ಹಿಂತಿರುಗಿ std::move(ವಿಭಜಿಸು_Into_Components_2{dfg, label}.m_components);
    }
};

std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> Dfg_ನಕ್ಷೆ::ವಿಭಜಿಸು_Into_Components(ಸ್ಥಿರಾಂಕ std::string& label) {
    ಹಿಂತಿರುಗಿ ವಿಭಜಿಸು_Into_Components_2::apply(*ಇದು, label);
}

ವರ್ಗ ಹೊರತೆಗೆ_Cyclic_Components_2 final {
    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ಸ್ಥಿರಾಂಕ std::string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Component name prefix
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var*, std::unordered_map<uint64_t, Dfg_ಶೃಂಗ_Var*>> m_clones;

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ಶೃಂಗ_And_Expand_Siblings(Dfg_ಶೃಂಗ& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        ಆದರೆ (vtx.is<Dfg_ಶೃಂಗ_Var>()) ಹಿಂತಿರುಗಿ;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        ಆದರೆ (vtxComponentr == component) ಹಿಂತಿರುಗಿ;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    ಶೂನ್ಯ expandSiblings(Dfg_ಶೃಂಗ& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](Dfg_ಶೃಂಗ& v) {
            ಕೂಡಿಸು_ಶೃಂಗ_And_Expand_Siblings(v, component);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        vtx.foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& v) {
            ಕೂಡಿಸು_ಶೃಂಗ_And_Expand_Siblings(v, component);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    ಶೂನ್ಯ expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        ಆದರೆ (m_doExpensiveChecks) {
            ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_ಅಪವರ್ತ್ಯ_Sinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) {
            ಸ್ಥಿರಾಂಕ uint64_t varComponent = m_component.at(vtx);
            ಆದರೆ (!varComponent) ಮುಂದುವರಿ;
            ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtx.srcp()) {
                ಆದರೆ (!srcp->is<Dfg_ಶೃಂಗ_Var>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ defp = vtx.defaultp()) {
                ಆದರೆ (!defp->is<Dfg_ಶೃಂಗ_Var>()) {
                    uint64_t& defComponent = m_component.at(defp);
                    UASSERT_OBJ(!defComponent || defComponent == varComponent, defp,
                                "Cycle through 'defaultp' that does not go through variable");
                    defComponent = varComponent;
                }
            }
        }
        // To ensure all component boundaries are at variables, expand
        // components to include all reachable non-variable vertices. Constants
        // are reachable from their sinks, and Ast refs cannot be in an SCC,
        // so only need to process op vertices. We do this by starting a DFS
        // from each vertex that is part of a component and add all reachable
        // non-variable vertices to the same.
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t ಗುರಿ_Component = m_component.at(vtx)) {
                expandSiblings(vtx, ಗುರಿ_Component);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    Dfg_ಶೃಂಗ_Var* getClone(Dfg_ಶೃಂಗ_Var& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        Dfg_ಶೃಂಗ_Var*& clonep = m_clones[&vtx][component];
        ಆದರೆ (!clonep) {
            ಆದರೆ (DfgVarPacked* ಸ್ಥಿರಾಂಕ pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = ಹೊಸ DfgVarPacked{m_dfg, pVtxp->vscp()};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgVarArray* ಸ್ಥಿರಾಂಕ aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = ಹೊಸ DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        ಹಿಂತಿರುಗಿ clonep;
    }

    // Fix edges that cross components
    ಶೂನ್ಯ fix_ಅಂಚುಗಳು(Dfg_ಶೃಂಗ_Var& vtx) {
        ಸ್ಥಿರಾಂಕ uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sp = vtx.srcp()) {
            ಸ್ಥಿರಾಂಕ uint64_t srcComponent = m_component.at(sp);
            ಆದರೆ (srcComponent != component) {
                UASSERT_OBJ(sp->is<Dfg_ಶೃಂಗ_Var>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(ಶೂನ್ಯಸೂಚಕ);
            }
        }
        ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dp = vtx.defaultp()) {
            ಸ್ಥಿರಾಂಕ uint64_t defaultComponent = m_component.at(dp);
            ಆದರೆ (defaultComponent != component) {
                UASSERT_OBJ(dp->is<Dfg_ಶೃಂಗ_Var>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(ಶೂನ್ಯಸೂಚಕ);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::ಸದಿಶ<Dfg_ಶೃಂಗ*> sinkps;
        vtx.foreachSink([&](Dfg_ಶೃಂಗ& sink) {
            sinkps.emplace_ಹಿಂಭಾಗ(&sink);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkp : sinkps) {
            ಸ್ಥಿರಾಂಕ uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            ಆದರೆ (sinkComponent == component) ಮುಂದುವರಿ;
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ clonep = getClone(vtx, sinkComponent);
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < sinkp->nInputs(); ++i) {
                ಆದರೆ (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    ಶೂನ್ಯ moveVertices(Dfg_ಶೃಂಗ::List<ಶೃಂಗ_2>& list) {
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : list.unlinkable()) {
            Dfg_ಶೃಂಗ& vtx = *vtxp;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t component = m_component.at(vtx)) {
                m_dfg.remove_ಶೃಂಗ(vtx);
                m_components[component - 1]->ಕೂಡಿಸು_ಶೃಂಗ(vtx);
            }
        }
    }

    ಶೂನ್ಯ check_ಅಂಚುಗಳು(Dfg_ನಕ್ಷೆ& dfg) ಸ್ಥಿರಾಂಕ {
        // Check that edges only cross components at variable boundaries
        dfg.for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) {
            ಆದರೆ (vtx.is<DfgVarPacked>()) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](Dfg_ಶೃಂಗ& snk) {
                ಆದರೆ (snk.is<Dfg_ಶೃಂಗ_Var>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        });
    }

    ಶೂನ್ಯ check_ನಕ್ಷೆ(Dfg_ನಕ್ಷೆ& dfg) ಸ್ಥಿರಾಂಕ {
        // Build set of vertices
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*> vertices{dfg.ಗಾತ್ರ_2()};
        dfg.for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) { vertices.ಸೇರಿಸು(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
            vtx.foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
                UASSERT_OBJ(vertices.ಎಣಿಕೆ_2(&src), &vtx, "Source vertex not in graph");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            vtx.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& snk) {
                UASSERT_OBJ(vertices.ಎಣಿಕೆ_2(&snk), &snk, "Sink vertex not in graph");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        });
    }

    ಶೂನ್ಯ ಹೊರತೆಗೆ_Components(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        ಗಾಗಿ (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(ಹೊಸ Dfg_ನಕ್ಷೆ{m_dfg.ctx(), m_ಪೂರ್ವಪ್ರತ್ಯಯ + cvt_ಗಮ್ಯ_Str(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lastp = m_dfg.varVertices().backp();
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fix_ಅಂಚುಗಳು(vtx);
            // Don't iterate clones added during this loop
            ಆದರೆ (&vtx == lastp) ನಿಲ್ಲಿಸು;
        }

        // Check results for consistency
        ಆದರೆ (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_ಅಂಚುಗಳು(m_dfg);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& dfgp : m_components) check_ಅಂಚುಗಳು(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        ಆದರೆ (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_ನಕ್ಷೆ(m_dfg);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& dfgp : m_components) check_ನಕ್ಷೆ(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    ಸ್ಪಷ್ಟ ಹೊರತೆಗೆ_Cyclic_Components_2(Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ std::string& label)
        : m_dfg{dfg}
        , m_ಪೂರ್ವಪ್ರತ್ಯಯ{dfg.name() + (label.ಖಾಲಿಗಳು() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ಸ್ಥಿರಾಂಕ uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        ಆದರೆ (!nSCCs) ಹಿಂತಿರುಗಿ;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        ಹೊರತೆಗೆ_Components(nSCCs);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> apply(Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ std::string& label) {
        ಹಿಂತಿರುಗಿ std::move(ಹೊರತೆಗೆ_Cyclic_Components_2{dfg, label}.m_components);
    }
};

std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>>
Dfg_ನಕ್ಷೆ::ಹೊರತೆಗೆ_Cyclic_Components(ಸ್ಥಿರಾಂಕ std::string& label) {
    ಹಿಂತಿರುಗಿ ಹೊರತೆಗೆ_Cyclic_Components_2::apply(*ಇದು, label);
}
