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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্রেণী SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<আকার_t> m_component = m_dfg.makeUserMap<আকার_t>();
    ধ্রুবক std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    আকার_t m_component_গণক = 1;

    শূন্য colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.আকার_2());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        জন্য (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            যদি (m_component[vtx]) চালু;

            // Start depth first traversal at this vertex
            queue.ঠেলো_back(&vtx);

            // Depth first traversal
            করো {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.বের_করো_back();

                // Move on if already visited
                যদি (m_component[item]) চালু;

                // Assign to current component
                m_component[item] = m_component_গণক;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.ঠেলো_back(&src);
                    ফেরত মিথ্যা;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.ঠেলো_back(&dst);
                    ফেরত মিথ্যা;
                });
            } যখন (!queue.empty());

            // Done with this component
            ++m_component_গণক;
        }
    }

    template <typename Vertex>
    শূন্য moveVertices(DfgVertex::List<Vertex>& list) {
        জন্য (DfgVertex* ধ্রুবক vtxp : list.unlinkable()) {
            যদি (ধ্রুবক আকার_t component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } অন্যথা {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, ধ্রুবক std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_component_গণক - 1);
        জন্য (আকার_t i = 1; i < m_component_গণক; ++i) {
            m_components[i - 1].reset(নতুন DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.আকার_2() == 0, "'this' DfgGraph should have been emptied");
    }

সর্বজনীন:
    স্থির std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ধ্রুবক std::string& label) {
        ফেরত std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(ধ্রুবক std::string& label) {
    ফেরত SplitIntoComponents::apply(*এই, label);
}

শ্রেণী ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ধ্রুবক std::string m_prefix;  // Component name prefix
    ধ্রুবক যৌক্তিক m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ধ্রুবক DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    শূন্য addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        যদি (vtx.is<DfgVertexVar>()) ফেরত;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        যদি (vtxComponentr == component) ফেরত;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    শূন্য expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            ফেরত মিথ্যা;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            ফেরত মিথ্যা;
        });
    }

    শূন্য expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        যদি (m_doExpensiveChecks) {
            জন্য (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        জন্য (ধ্রুবক DfgVertexVar& vtx : m_dfg.varVertices()) {
            ধ্রুবক uint64_t varComponent = m_component.at(vtx);
            যদি (!varComponent) চালু;
            যদি (DfgVertex* ধ্রুবক srcp = vtx.srcp()) {
                যদি (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            যদি (DfgVertex* ধ্রুবক defp = vtx.defaultp()) {
                যদি (!defp->is<DfgVertexVar>()) {
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
        জন্য (DfgVertex& vtx : m_dfg.opVertices()) {
            যদি (ধ্রুবক uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        যদি (!clonep) {
            যদি (DfgVarPacked* ধ্রুবক pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = নতুন DfgVarPacked{m_dfg, pVtxp->vscp()};
            } অন্যথা যদি (DfgVarArray* ধ্রুবক aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = নতুন DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        ফেরত clonep;
    }

    // Fix edges that cross components
    শূন্য fixEdges(DfgVertexVar& vtx) {
        ধ্রুবক uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        যদি (DfgVertex* ধ্রুবক sp = vtx.srcp()) {
            ধ্রুবক uint64_t srcComponent = m_component.at(sp);
            যদি (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        যদি (DfgVertex* ধ্রুবক dp = vtx.defaultp()) {
            ধ্রুবক uint64_t defaultComponent = m_component.at(dp);
            যদি (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            ফেরত মিথ্যা;
        });
        জন্য (DfgVertex* ধ্রুবক sinkp : sinkps) {
            ধ্রুবক uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            যদি (sinkComponent == component) চালু;
            DfgVertex* ধ্রুবক clonep = getClone(vtx, sinkComponent);
            জন্য (আকার_t i = 0; i < sinkp->nInputs(); ++i) {
                যদি (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    template <typename Vertex>
    শূন্য moveVertices(DfgVertex::List<Vertex>& list) {
        জন্য (DfgVertex* ধ্রুবক vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            যদি (ধ্রুবক uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    শূন্য checkEdges(DfgGraph& dfg) ধ্রুবক {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            যদি (vtx.is<DfgVarPacked>()) ফেরত;
            ধ্রুবক uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                যদি (snk.is<DfgVertexVar>()) ফেরত মিথ্যা;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                ফেরত মিথ্যা;
            });
        });
    }

    শূন্য checkGraph(DfgGraph& dfg) ধ্রুবক {
        // Build set of vertices
        std::unordered_set<ধ্রুবক DfgVertex*> vertices{dfg.আকার_2()};
        dfg.forEachVertex([&](ধ্রুবক DfgVertex& vtx) { vertices.ঢোকাও(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](ধ্রুবক DfgVertex& vtx) {
            vtx.foreachSource([&](ধ্রুবক DfgVertex& src) {
                UASSERT_OBJ(vertices.গুনতি(&src), &vtx, "Source vertex not in graph");
                ফেরত মিথ্যা;
            });
            vtx.foreachSink([&](ধ্রুবক DfgVertex& snk) {
                UASSERT_OBJ(vertices.গুনতি(&snk), &snk, "Sink vertex not in graph");
                ফেরত মিথ্যা;
            });
        });
    }

    শূন্য extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        জন্য (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(নতুন DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ধ্রুবক DfgVertex* ধ্রুবক lastp = m_dfg.varVertices().backp();
        জন্য (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            যদি (&vtx == lastp) বিরতি;
        }

        // Check results for consistency
        যদি (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            জন্য (ধ্রুবক auto& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        যদি (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            জন্য (ধ্রুবক auto& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, ধ্রুবক std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ধ্রুবক uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        যদি (!nSCCs) ফেরত;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

সর্বজনীন:
    স্থির std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ধ্রুবক std::string& label) {
        ফেরত std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(ধ্রুবক std::string& label) {
    ফেরত ExtractCyclicComponents::apply(*এই, label);
}
