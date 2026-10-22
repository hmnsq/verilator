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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<size_t> m_component = m_dfg.makeUserMap<size_t>();
    സ്ഥിരം std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    size_t m_componentCounter = 1;

    ശൂന്യം colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.size());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        വേണ്ടി (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            എങ്കിൽ (m_component[vtx]) തുടരുക;

            // Start depth first traversal at this vertex
            queue.തള്ളുക_back(&vtx);

            // Depth first traversal
            ചെയ്യുക {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.എടുക്കുക_back();

                // Move on if already visited
                എങ്കിൽ (m_component[item]) തുടരുക;

                // Assign to current component
                m_component[item] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.തള്ളുക_back(&src);
                    തിരികെ അസത്യം;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.തള്ളുക_back(&dst);
                    തിരികെ അസത്യം;
                });
            } ആകുമ്പോൾ (!queue.empty());

            // Done with this component
            ++m_componentCounter;
        }
    }

    template <typename Vertex>
    ശൂന്യം moveVertices(DfgVertex::List<Vertex>& list) {
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : list.unlinkable()) {
            എങ്കിൽ (സ്ഥിരം size_t component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } അല്ലെങ്കിൽ {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, സ്ഥിരം std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        വേണ്ടി (size_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(പുതിയ DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.size() == 0, "'this' DfgGraph should have been emptied");
    }

പൊതു:
    സ്ഥിതം std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, സ്ഥിരം std::string& label) {
        തിരികെ std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(സ്ഥിരം std::string& label) {
    തിരികെ SplitIntoComponents::apply(*ഇത്, label);
}

വർഗ്ഗം ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    സ്ഥിരം std::string m_prefix;  // Component name prefix
    സ്ഥിരം തർക്കം m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<സ്ഥിരം DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    ശൂന്യം addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        എങ്കിൽ (vtx.is<DfgVertexVar>()) തിരികെ;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        എങ്കിൽ (vtxComponentr == component) തിരികെ;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    ശൂന്യം expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            തിരികെ അസത്യം;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            തിരികെ അസത്യം;
        });
    }

    ശൂന്യം expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        എങ്കിൽ (m_doExpensiveChecks) {
            വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        വേണ്ടി (സ്ഥിരം DfgVertexVar& vtx : m_dfg.varVertices()) {
            സ്ഥിരം uint64_t varComponent = m_component.at(vtx);
            എങ്കിൽ (!varComponent) തുടരുക;
            എങ്കിൽ (DfgVertex* സ്ഥിരം srcp = vtx.srcp()) {
                എങ്കിൽ (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            എങ്കിൽ (DfgVertex* സ്ഥിരം defp = vtx.defaultp()) {
                എങ്കിൽ (!defp->is<DfgVertexVar>()) {
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
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
            എങ്കിൽ (സ്ഥിരം uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        എങ്കിൽ (!clonep) {
            എങ്കിൽ (DfgVarPacked* സ്ഥിരം pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = പുതിയ DfgVarPacked{m_dfg, pVtxp->vscp()};
            } അല്ലെങ്കിൽ എങ്കിൽ (DfgVarArray* സ്ഥിരം aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = പുതിയ DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        തിരികെ clonep;
    }

    // Fix edges that cross components
    ശൂന്യം fixEdges(DfgVertexVar& vtx) {
        സ്ഥിരം uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        എങ്കിൽ (DfgVertex* സ്ഥിരം sp = vtx.srcp()) {
            സ്ഥിരം uint64_t srcComponent = m_component.at(sp);
            എങ്കിൽ (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        എങ്കിൽ (DfgVertex* സ്ഥിരം dp = vtx.defaultp()) {
            സ്ഥിരം uint64_t defaultComponent = m_component.at(dp);
            എങ്കിൽ (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            തിരികെ അസത്യം;
        });
        വേണ്ടി (DfgVertex* സ്ഥിരം sinkp : sinkps) {
            സ്ഥിരം uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            എങ്കിൽ (sinkComponent == component) തുടരുക;
            DfgVertex* സ്ഥിരം clonep = getClone(vtx, sinkComponent);
            വേണ്ടി (size_t i = 0; i < sinkp->nInputs(); ++i) {
                എങ്കിൽ (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    template <typename Vertex>
    ശൂന്യം moveVertices(DfgVertex::List<Vertex>& list) {
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            എങ്കിൽ (സ്ഥിരം uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    ശൂന്യം checkEdges(DfgGraph& dfg) സ്ഥിരം {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            എങ്കിൽ (vtx.is<DfgVarPacked>()) തിരികെ;
            സ്ഥിരം uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                എങ്കിൽ (snk.is<DfgVertexVar>()) തിരികെ അസത്യം;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                തിരികെ അസത്യം;
            });
        });
    }

    ശൂന്യം checkGraph(DfgGraph& dfg) സ്ഥിരം {
        // Build set of vertices
        std::unordered_set<സ്ഥിരം DfgVertex*> vertices{dfg.size()};
        dfg.forEachVertex([&](സ്ഥിരം DfgVertex& vtx) { vertices.തിരുകുക(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](സ്ഥിരം DfgVertex& vtx) {
            vtx.foreachSource([&](സ്ഥിരം DfgVertex& src) {
                UASSERT_OBJ(vertices.എണ്ണം(&src), &vtx, "Source vertex not in graph");
                തിരികെ അസത്യം;
            });
            vtx.foreachSink([&](സ്ഥിരം DfgVertex& snk) {
                UASSERT_OBJ(vertices.എണ്ണം(&snk), &snk, "Sink vertex not in graph");
                തിരികെ അസത്യം;
            });
        });
    }

    ശൂന്യം extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        വേണ്ടി (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(പുതിയ DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        സ്ഥിരം DfgVertex* സ്ഥിരം lastp = m_dfg.varVertices().backp();
        വേണ്ടി (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            എങ്കിൽ (&vtx == lastp) നിർത്തുക;
        }

        // Check results for consistency
        എങ്കിൽ (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            വേണ്ടി (സ്ഥിരം auto& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        എങ്കിൽ (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            വേണ്ടി (സ്ഥിരം auto& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, സ്ഥിരം std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        സ്ഥിരം uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        എങ്കിൽ (!nSCCs) തിരികെ;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

പൊതു:
    സ്ഥിതം std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, സ്ഥിരം std::string& label) {
        തിരികെ std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(സ്ഥിരം std::string& label) {
    തിരികെ ExtractCyclicComponents::apply(*ഇത്, label);
}
