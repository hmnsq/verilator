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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

رده SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<size_t> m_component = m_dfg.makeUserMap<size_t>();
    ثابت std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    size_t m_componentCounter = 1;

    تهی colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.size());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        برای (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            اگر (m_component[vtx]) ادامه;

            // Start depth first traversal at this vertex
            queue.افزودن_back(&vtx);

            // Depth first traversal
            انجامبده {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.برداشت_back();

                // Move on if already visited
                اگر (m_component[item]) ادامه;

                // Assign to current component
                m_component[item] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.افزودن_back(&src);
                    بازگشت نادرست;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.افزودن_back(&dst);
                    بازگشت نادرست;
                });
            } وقتی (!queue.empty());

            // Done with this component
            ++m_componentCounter;
        }
    }

    template <typename Vertex>
    تهی moveVertices(DfgVertex::List<Vertex>& list) {
        برای (DfgVertex* ثابت vtxp : list.unlinkable()) {
            اگر (ثابت size_t component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } وگرنه {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        برای (size_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(جدید DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.size() == 0, "'this' DfgGraph should have been emptied");
    }

عمومی:
    ایستا std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ثابت std::string& label) {
        بازگشت std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(ثابت std::string& label) {
    بازگشت SplitIntoComponents::apply(*این, label);
}

رده ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ثابت std::string m_prefix;  // Component name prefix
    ثابت منطقی m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ثابت DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    تهی addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        اگر (vtx.is<DfgVertexVar>()) بازگشت;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        اگر (vtxComponentr == component) بازگشت;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    تهی expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            بازگشت نادرست;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            بازگشت نادرست;
        });
    }

    تهی expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        اگر (m_doExpensiveChecks) {
            برای (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        برای (ثابت DfgVertexVar& vtx : m_dfg.varVertices()) {
            ثابت uint64_t varComponent = m_component.at(vtx);
            اگر (!varComponent) ادامه;
            اگر (DfgVertex* ثابت srcp = vtx.srcp()) {
                اگر (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            اگر (DfgVertex* ثابت defp = vtx.defaultp()) {
                اگر (!defp->is<DfgVertexVar>()) {
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
        برای (DfgVertex& vtx : m_dfg.opVertices()) {
            اگر (ثابت uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        اگر (!clonep) {
            اگر (DfgVarPacked* ثابت pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = جدید DfgVarPacked{m_dfg, pVtxp->vscp()};
            } وگرنه اگر (DfgVarArray* ثابت aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = جدید DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        بازگشت clonep;
    }

    // Fix edges that cross components
    تهی fixEdges(DfgVertexVar& vtx) {
        ثابت uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        اگر (DfgVertex* ثابت sp = vtx.srcp()) {
            ثابت uint64_t srcComponent = m_component.at(sp);
            اگر (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        اگر (DfgVertex* ثابت dp = vtx.defaultp()) {
            ثابت uint64_t defaultComponent = m_component.at(dp);
            اگر (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            بازگشت نادرست;
        });
        برای (DfgVertex* ثابت sinkp : sinkps) {
            ثابت uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            اگر (sinkComponent == component) ادامه;
            DfgVertex* ثابت clonep = getClone(vtx, sinkComponent);
            برای (size_t i = 0; i < sinkp->nInputs(); ++i) {
                اگر (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    template <typename Vertex>
    تهی moveVertices(DfgVertex::List<Vertex>& list) {
        برای (DfgVertex* ثابت vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            اگر (ثابت uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    تهی checkEdges(DfgGraph& dfg) ثابت {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            اگر (vtx.is<DfgVarPacked>()) بازگشت;
            ثابت uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                اگر (snk.is<DfgVertexVar>()) بازگشت نادرست;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                بازگشت نادرست;
            });
        });
    }

    تهی checkGraph(DfgGraph& dfg) ثابت {
        // Build set of vertices
        std::unordered_set<ثابت DfgVertex*> vertices{dfg.size()};
        dfg.forEachVertex([&](ثابت DfgVertex& vtx) { vertices.درج(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](ثابت DfgVertex& vtx) {
            vtx.foreachSource([&](ثابت DfgVertex& src) {
                UASSERT_OBJ(vertices.شمار(&src), &vtx, "Source vertex not in graph");
                بازگشت نادرست;
            });
            vtx.foreachSink([&](ثابت DfgVertex& snk) {
                UASSERT_OBJ(vertices.شمار(&snk), &snk, "Sink vertex not in graph");
                بازگشت نادرست;
            });
        });
    }

    تهی extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        برای (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(جدید DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ثابت DfgVertex* ثابت lastp = m_dfg.varVertices().backp();
        برای (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            اگر (&vtx == lastp) بشکن;
        }

        // Check results for consistency
        اگر (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            برای (ثابت auto& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        اگر (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            برای (ثابت auto& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ثابت uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        اگر (!nSCCs) بازگشت;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

عمومی:
    ایستا std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ثابت std::string& label) {
        بازگشت std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(ثابت std::string& label) {
    بازگشت ExtractCyclicComponents::apply(*این, label);
}
