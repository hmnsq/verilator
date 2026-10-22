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

ವರ್ಗ SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<size_t> m_component = m_dfg.makeUserMap<size_t>();
    ಧ್ರುವ std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    size_t m_componentCounter = 1;

    ಶೂನ್ಯ colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.size());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        ಪ್ರತಿ (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            ಯದಿ (m_component[vtx]) ಅನುವರ್ತ;

            // Start depth first traversal at this vertex
            queue.ತಳ್ಳು_back(&vtx);

            // Depth first traversal
            ಕುರು {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.ತೆಗೆ_back();

                // Move on if already visited
                ಯದಿ (m_component[item]) ಅನುವರ್ತ;

                // Assign to current component
                m_component[item] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.ತಳ್ಳು_back(&src);
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.ತಳ್ಳು_back(&dst);
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                });
            } ಯಾವತ್ (!queue.empty());

            // Done with this component
            ++m_componentCounter;
        }
    }

    template <typename Vertex>
    ಶೂನ್ಯ moveVertices(DfgVertex::List<Vertex>& list) {
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : list.unlinkable()) {
            ಯದಿ (ಧ್ರುವ size_t component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } ಅನ್ಯಥಾ {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, ಧ್ರುವ std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        ಪ್ರತಿ (size_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(ನೂತನ DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.size() == 0, "'this' DfgGraph should have been emptied");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ಧ್ರುವ std::string& label) {
        ಪ್ರತ್ಯಾಗಮ std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(ಧ್ರುವ std::string& label) {
    ಪ್ರತ್ಯಾಗಮ SplitIntoComponents::apply(*ಏತತ್, label);
}

ವರ್ಗ ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ಧ್ರುವ std::string m_prefix;  // Component name prefix
    ಧ್ರುವ ತಾರ್ಕಿಕ m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ಧ್ರುವ DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    ಶೂನ್ಯ addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        ಯದಿ (vtx.is<DfgVertexVar>()) ಪ್ರತ್ಯಾಗಮ;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        ಯದಿ (vtxComponentr == component) ಪ್ರತ್ಯಾಗಮ;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    ಶೂನ್ಯ expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    ಶೂನ್ಯ expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        ಯದಿ (m_doExpensiveChecks) {
            ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        ಪ್ರತಿ (ಧ್ರುವ DfgVertexVar& vtx : m_dfg.varVertices()) {
            ಧ್ರುವ uint64_t varComponent = m_component.at(vtx);
            ಯದಿ (!varComponent) ಅನುವರ್ತ;
            ಯದಿ (DfgVertex* ಧ್ರುವ srcp = vtx.srcp()) {
                ಯದಿ (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            ಯದಿ (DfgVertex* ಧ್ರುವ defp = vtx.defaultp()) {
                ಯದಿ (!defp->is<DfgVertexVar>()) {
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
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) {
            ಯದಿ (ಧ್ರುವ uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        ಯದಿ (!clonep) {
            ಯದಿ (DfgVarPacked* ಧ್ರುವ pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = ನೂತನ DfgVarPacked{m_dfg, pVtxp->vscp()};
            } ಅನ್ಯಥಾ ಯದಿ (DfgVarArray* ಧ್ರುವ aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = ನೂತನ DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        ಪ್ರತ್ಯಾಗಮ clonep;
    }

    // Fix edges that cross components
    ಶೂನ್ಯ fixEdges(DfgVertexVar& vtx) {
        ಧ್ರುವ uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        ಯದಿ (DfgVertex* ಧ್ರುವ sp = vtx.srcp()) {
            ಧ್ರುವ uint64_t srcComponent = m_component.at(sp);
            ಯದಿ (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        ಯದಿ (DfgVertex* ಧ್ರುವ dp = vtx.defaultp()) {
            ಧ್ರುವ uint64_t defaultComponent = m_component.at(dp);
            ಯದಿ (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ sinkp : sinkps) {
            ಧ್ರುವ uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            ಯದಿ (sinkComponent == component) ಅನುವರ್ತ;
            DfgVertex* ಧ್ರುವ clonep = getClone(vtx, sinkComponent);
            ಪ್ರತಿ (size_t i = 0; i < sinkp->nInputs(); ++i) {
                ಯದಿ (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    template <typename Vertex>
    ಶೂನ್ಯ moveVertices(DfgVertex::List<Vertex>& list) {
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            ಯದಿ (ಧ್ರುವ uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    ಶೂನ್ಯ checkEdges(DfgGraph& dfg) ಧ್ರುವ {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            ಯದಿ (vtx.is<DfgVarPacked>()) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                ಯದಿ (snk.is<DfgVertexVar>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        });
    }

    ಶೂನ್ಯ checkGraph(DfgGraph& dfg) ಧ್ರುವ {
        // Build set of vertices
        std::unordered_set<ಧ್ರುವ DfgVertex*> vertices{dfg.size()};
        dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) { vertices.ಸೇರಿಸು(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
            vtx.foreachSource([&](ಧ್ರುವ DfgVertex& src) {
                UASSERT_OBJ(vertices.ಎಣಿಕೆ(&src), &vtx, "Source vertex not in graph");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            vtx.foreachSink([&](ಧ್ರುವ DfgVertex& snk) {
                UASSERT_OBJ(vertices.ಎಣಿಕೆ(&snk), &snk, "Sink vertex not in graph");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        });
    }

    ಶೂನ್ಯ extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        ಪ್ರತಿ (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(ನೂತನ DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ಧ್ರುವ DfgVertex* ಧ್ರುವ lastp = m_dfg.varVertices().backp();
        ಪ್ರತಿ (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            ಯದಿ (&vtx == lastp) ಭಙ್ಗ;
        }

        // Check results for consistency
        ಯದಿ (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            ಪ್ರತಿ (ಧ್ರುವ auto& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        ಯದಿ (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            ಪ್ರತಿ (ಧ್ರುವ auto& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, ಧ್ರುವ std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ಧ್ರುವ uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        ಯದಿ (!nSCCs) ಪ್ರತ್ಯಾಗಮ;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ಧ್ರುವ std::string& label) {
        ಪ್ರತ್ಯಾಗಮ std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(ಧ್ರುವ std::string& label) {
    ಪ್ರತ್ಯಾಗಮ ExtractCyclicComponents::apply(*ಏತತ್, label);
}
