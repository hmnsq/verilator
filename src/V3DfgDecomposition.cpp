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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"

#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<माप_प्रकार> m_component = m_dfg.makeUserMap<माप_प्रकार>();
    स्थिर std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    माप_प्रकार m_component_गणक = 1;

    व्योम colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.आकार());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        क्रम (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            अगर (m_component[vtx]) जारी;

            // Start depth first traversal at this vertex
            queue.धकेलो_back(&vtx);

            // Depth first traversal
            करो {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.निकालो_back();

                // Move on if already visited
                अगर (m_component[item]) जारी;

                // Assign to current component
                m_component[item] = m_component_गणक;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.धकेलो_back(&src);
                    वापस false;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.धकेलो_back(&dst);
                    वापस false;
                });
            } जबतक (!queue.empty());

            // Done with this component
            ++m_component_गणक;
        }
    }

    ढाँचा <typename Vertex>
    व्योम moveVertices(DfgVertex::List<Vertex>& list) {
        क्रम (DfgVertex* स्थिर vtxp : list.unlinkable()) {
            अगर (स्थिर माप_प्रकार component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } अन्यथा {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, स्थिर std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_component_गणक - 1);
        क्रम (माप_प्रकार i = 1; i < m_component_गणक; ++i) {
            m_components[i - 1].reset(नया DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.आकार() == 0, "'this' DfgGraph should have been emptied");
    }

खुला:
    अटल std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, स्थिर std::string& label) {
        वापस std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(स्थिर std::string& label) {
    वापस SplitIntoComponents::apply(*यह, label);
}

श्रेणी ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    स्थिर std::string m_prefix;  // Component name prefix
    स्थिर bool m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<स्थिर DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    व्योम addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        अगर (vtx.is<DfgVertexVar>()) वापस;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        अगर (vtxComponentr == component) वापस;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    व्योम expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            वापस false;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            वापस false;
        });
    }

    व्योम expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        अगर (m_doExpensiveChecks) {
            क्रम (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        क्रम (स्थिर DfgVertexVar& vtx : m_dfg.varVertices()) {
            स्थिर uint64_t varComponent = m_component.at(vtx);
            अगर (!varComponent) जारी;
            अगर (DfgVertex* स्थिर srcp = vtx.srcp()) {
                अगर (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            अगर (DfgVertex* स्थिर defp = vtx.defaultp()) {
                अगर (!defp->is<DfgVertexVar>()) {
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
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) {
            अगर (स्थिर uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        अगर (!clonep) {
            अगर (DfgVarPacked* स्थिर pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = नया DfgVarPacked{m_dfg, pVtxp->vscp()};
            } अन्यथा अगर (DfgVarArray* स्थिर aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = नया DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        वापस clonep;
    }

    // Fix edges that cross components
    व्योम fixEdges(DfgVertexVar& vtx) {
        स्थिर uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        अगर (DfgVertex* स्थिर sp = vtx.srcp()) {
            स्थिर uint64_t srcComponent = m_component.at(sp);
            अगर (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        अगर (DfgVertex* स्थिर dp = vtx.defaultp()) {
            स्थिर uint64_t defaultComponent = m_component.at(dp);
            अगर (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            वापस false;
        });
        क्रम (DfgVertex* स्थिर sinkp : sinkps) {
            स्थिर uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            अगर (sinkComponent == component) जारी;
            DfgVertex* स्थिर clonep = getClone(vtx, sinkComponent);
            क्रम (माप_प्रकार i = 0; i < sinkp->nInputs(); ++i) {
                अगर (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    ढाँचा <typename Vertex>
    व्योम moveVertices(DfgVertex::List<Vertex>& list) {
        क्रम (DfgVertex* स्थिर vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            अगर (स्थिर uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    व्योम checkEdges(DfgGraph& dfg) स्थिर {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            अगर (vtx.is<DfgVarPacked>()) वापस;
            स्थिर uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                अगर (snk.is<DfgVertexVar>()) वापस false;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                वापस false;
            });
        });
    }

    व्योम checkGraph(DfgGraph& dfg) स्थिर {
        // Build set of vertices
        std::unordered_set<स्थिर DfgVertex*> vertices{dfg.आकार()};
        dfg.forEachVertex([&](स्थिर DfgVertex& vtx) { vertices.जोड़ो(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](स्थिर DfgVertex& vtx) {
            vtx.foreachSource([&](स्थिर DfgVertex& src) {
                UASSERT_OBJ(vertices.गिनती(&src), &vtx, "Source vertex not in graph");
                वापस false;
            });
            vtx.foreachSink([&](स्थिर DfgVertex& snk) {
                UASSERT_OBJ(vertices.गिनती(&snk), &snk, "Sink vertex not in graph");
                वापस false;
            });
        });
    }

    व्योम extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        क्रम (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(नया DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        स्थिर DfgVertex* स्थिर lastp = m_dfg.varVertices().backp();
        क्रम (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            अगर (&vtx == lastp) अवरोध;
        }

        // Check results for consistency
        अगर (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            क्रम (स्थिर स्वतः& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        अगर (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            क्रम (स्थिर स्वतः& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, स्थिर std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        स्थिर uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        अगर (!nSCCs) वापस;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

खुला:
    अटल std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, स्थिर std::string& label) {
        वापस std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(स्थिर std::string& label) {
    वापस ExtractCyclicComponents::apply(*यह, label);
}
