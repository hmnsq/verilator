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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<size_t> m_component = m_dfg.makeUserMap<size_t>();
    மாறிலி std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    size_t m_componentCounter = 1;

    வெறுமை colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.size());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        க்கு (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            எனில் (m_component[vtx]) தொடர்;

            // Start depth first traversal at this vertex
            queue.தள்ளு_back(&vtx);

            // Depth first traversal
            செய் {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.எடு_back();

                // Move on if already visited
                எனில் (m_component[item]) தொடர்;

                // Assign to current component
                m_component[item] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.தள்ளு_back(&src);
                    திரும்பு பொய்;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.தள்ளு_back(&dst);
                    திரும்பு பொய்;
                });
            } போது (!queue.empty());

            // Done with this component
            ++m_componentCounter;
        }
    }

    template <typename Vertex>
    வெறுமை moveVertices(DfgVertex::List<Vertex>& list) {
        க்கு (DfgVertex* மாறிலி vtxp : list.unlinkable()) {
            எனில் (மாறிலி size_t component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } இல்லையெனில் {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, மாறிலி std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        க்கு (size_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(புதிய DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.size() == 0, "'this' DfgGraph should have been emptied");
    }

பொது:
    நிலையான std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, மாறிலி std::string& label) {
        திரும்பு std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(மாறிலி std::string& label) {
    திரும்பு SplitIntoComponents::apply(*இது, label);
}

வகுப்பு ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    மாறிலி std::string m_prefix;  // Component name prefix
    மாறிலி தருக்க m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<மாறிலி DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    வெறுமை addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        எனில் (vtx.is<DfgVertexVar>()) திரும்பு;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        எனில் (vtxComponentr == component) திரும்பு;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    வெறுமை expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            திரும்பு பொய்;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            திரும்பு பொய்;
        });
    }

    வெறுமை expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        எனில் (m_doExpensiveChecks) {
            க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        க்கு (மாறிலி DfgVertexVar& vtx : m_dfg.varVertices()) {
            மாறிலி uint64_t varComponent = m_component.at(vtx);
            எனில் (!varComponent) தொடர்;
            எனில் (DfgVertex* மாறிலி srcp = vtx.srcp()) {
                எனில் (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            எனில் (DfgVertex* மாறிலி defp = vtx.defaultp()) {
                எனில் (!defp->is<DfgVertexVar>()) {
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
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
            எனில் (மாறிலி uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        எனில் (!clonep) {
            எனில் (DfgVarPacked* மாறிலி pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = புதிய DfgVarPacked{m_dfg, pVtxp->vscp()};
            } இல்லையெனில் எனில் (DfgVarArray* மாறிலி aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = புதிய DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        திரும்பு clonep;
    }

    // Fix edges that cross components
    வெறுமை fixEdges(DfgVertexVar& vtx) {
        மாறிலி uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        எனில் (DfgVertex* மாறிலி sp = vtx.srcp()) {
            மாறிலி uint64_t srcComponent = m_component.at(sp);
            எனில் (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        எனில் (DfgVertex* மாறிலி dp = vtx.defaultp()) {
            மாறிலி uint64_t defaultComponent = m_component.at(dp);
            எனில் (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            திரும்பு பொய்;
        });
        க்கு (DfgVertex* மாறிலி sinkp : sinkps) {
            மாறிலி uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            எனில் (sinkComponent == component) தொடர்;
            DfgVertex* மாறிலி clonep = getClone(vtx, sinkComponent);
            க்கு (size_t i = 0; i < sinkp->nInputs(); ++i) {
                எனில் (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    template <typename Vertex>
    வெறுமை moveVertices(DfgVertex::List<Vertex>& list) {
        க்கு (DfgVertex* மாறிலி vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            எனில் (மாறிலி uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    வெறுமை checkEdges(DfgGraph& dfg) மாறிலி {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            எனில் (vtx.is<DfgVarPacked>()) திரும்பு;
            மாறிலி uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                எனில் (snk.is<DfgVertexVar>()) திரும்பு பொய்;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                திரும்பு பொய்;
            });
        });
    }

    வெறுமை checkGraph(DfgGraph& dfg) மாறிலி {
        // Build set of vertices
        std::unordered_set<மாறிலி DfgVertex*> vertices{dfg.size()};
        dfg.forEachVertex([&](மாறிலி DfgVertex& vtx) { vertices.செருகு(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](மாறிலி DfgVertex& vtx) {
            vtx.foreachSource([&](மாறிலி DfgVertex& src) {
                UASSERT_OBJ(vertices.மொத்தம்(&src), &vtx, "Source vertex not in graph");
                திரும்பு பொய்;
            });
            vtx.foreachSink([&](மாறிலி DfgVertex& snk) {
                UASSERT_OBJ(vertices.மொத்தம்(&snk), &snk, "Sink vertex not in graph");
                திரும்பு பொய்;
            });
        });
    }

    வெறுமை extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        க்கு (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(புதிய DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        மாறிலி DfgVertex* மாறிலி lastp = m_dfg.varVertices().backp();
        க்கு (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            எனில் (&vtx == lastp) நிறுத்து;
        }

        // Check results for consistency
        எனில் (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            க்கு (மாறிலி auto& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        எனில் (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            க்கு (மாறிலி auto& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, மாறிலி std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        மாறிலி uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        எனில் (!nSCCs) திரும்பு;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

பொது:
    நிலையான std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, மாறிலி std::string& label) {
        திரும்பு std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(மாறிலி std::string& label) {
    திரும்பு ExtractCyclicComponents::apply(*இது, label);
}
