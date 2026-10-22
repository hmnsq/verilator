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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग SplitIntoComponents final {

    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<माप_प्रकार> m_component = m_dfg.makeUserMap<माप_प्रकार>();
    ध्रुव std::string m_prefix;  // Component name prefix
    std::vector<std::unique_ptr<DfgGraph>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    माप_प्रकार m_componentCounter = 1;

    शून्य colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::vector<DfgVertex*> queue;
        queue.reserve(m_dfg.size());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        प्रति (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            यदि (m_component[vtx]) अनुवर्त;

            // Start depth first traversal at this vertex
            queue.धकेलो_back(&vtx);

            // Depth first traversal
            कुरु {
                // Pop next work item
                DfgVertex& item = *queue.back();
                queue.निकालो_back();

                // Move on if already visited
                यदि (m_component[item]) अनुवर्त;

                // Assign to current component
                m_component[item] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                item.foreachSource([&](DfgVertex& src) {
                    queue.धकेलो_back(&src);
                    प्रत्यागम असत्य;
                });
                item.foreachSink([&](DfgVertex& dst) {
                    queue.धकेलो_back(&dst);
                    प्रत्यागम असत्य;
                });
            } यावत् (!queue.empty());

            // Done with this component
            ++m_componentCounter;
        }
    }

    ढाँचा <typename Vertex>
    शून्य moveVertices(DfgVertex::List<Vertex>& list) {
        प्रति (DfgVertex* ध्रुव vtxp : list.unlinkable()) {
            यदि (ध्रुव माप_प्रकार component = m_component[vtxp]) {
                m_dfg.removeVertex(*vtxp);
                m_components[component - 1]->addVertex(*vtxp);
            } अन्यथा {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    SplitIntoComponents(DfgGraph& dfg, ध्रुव std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        प्रति (माप_प्रकार i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(नूतन DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.size() == 0, "'this' DfgGraph should have been emptied");
    }

सार्वजनिक:
    स्थिर std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ध्रुव std::string& label) {
        प्रत्यागम std::move(SplitIntoComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>> DfgGraph::splitIntoComponents(ध्रुव std::string& label) {
    प्रत्यागम SplitIntoComponents::apply(*एतत्, label);
}

वर्ग ExtractCyclicComponents final {
    // STATE
    DfgGraph& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ध्रुव std::string m_prefix;  // Component name prefix
    ध्रुव तार्किक m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::vector<std::unique_ptr<DfgGraph>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ध्रुव DfgVertexVar*, std::unordered_map<uint64_t, DfgVertexVar*>> m_clones;

    // METHODS
    शून्य addVertexAndExpandSiblings(DfgVertex& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        यदि (vtx.is<DfgVertexVar>()) प्रत्यागम;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        यदि (vtxComponentr == component) प्रत्यागम;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    शून्य expandSiblings(DfgVertex& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            प्रत्यागम असत्य;
        });
        vtx.foreachSource([&](DfgVertex& v) {
            addVertexAndExpandSiblings(v, component);
            प्रत्यागम असत्य;
        });
    }

    शून्य expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        यदि (m_doExpensiveChecks) {
            प्रति (DfgVertex& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.hasMultipleSinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        प्रति (ध्रुव DfgVertexVar& vtx : m_dfg.varVertices()) {
            ध्रुव uint64_t varComponent = m_component.at(vtx);
            यदि (!varComponent) अनुवर्त;
            यदि (DfgVertex* ध्रुव srcp = vtx.srcp()) {
                यदि (!srcp->is<DfgVertexVar>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            यदि (DfgVertex* ध्रुव defp = vtx.defaultp()) {
                यदि (!defp->is<DfgVertexVar>()) {
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
        प्रति (DfgVertex& vtx : m_dfg.opVertices()) {
            यदि (ध्रुव uint64_t targetComponent = m_component.at(vtx)) {
                expandSiblings(vtx, targetComponent);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    DfgVertexVar* getClone(DfgVertexVar& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        DfgVertexVar*& clonep = m_clones[&vtx][component];
        यदि (!clonep) {
            यदि (DfgVarPacked* ध्रुव pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = नूतन DfgVarPacked{m_dfg, pVtxp->vscp()};
            } अन्यथा यदि (DfgVarArray* ध्रुव aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = नूतन DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        प्रत्यागम clonep;
    }

    // Fix edges that cross components
    शून्य fixEdges(DfgVertexVar& vtx) {
        ध्रुव uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        यदि (DfgVertex* ध्रुव sp = vtx.srcp()) {
            ध्रुव uint64_t srcComponent = m_component.at(sp);
            यदि (srcComponent != component) {
                UASSERT_OBJ(sp->is<DfgVertexVar>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(nullptr);
            }
        }
        यदि (DfgVertex* ध्रुव dp = vtx.defaultp()) {
            ध्रुव uint64_t defaultComponent = m_component.at(dp);
            यदि (defaultComponent != component) {
                UASSERT_OBJ(dp->is<DfgVertexVar>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(nullptr);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::vector<DfgVertex*> sinkps;
        vtx.foreachSink([&](DfgVertex& sink) {
            sinkps.emplace_back(&sink);
            प्रत्यागम असत्य;
        });
        प्रति (DfgVertex* ध्रुव sinkp : sinkps) {
            ध्रुव uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            यदि (sinkComponent == component) अनुवर्त;
            DfgVertex* ध्रुव clonep = getClone(vtx, sinkComponent);
            प्रति (माप_प्रकार i = 0; i < sinkp->nInputs(); ++i) {
                यदि (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    ढाँचा <typename Vertex>
    शून्य moveVertices(DfgVertex::List<Vertex>& list) {
        प्रति (DfgVertex* ध्रुव vtxp : list.unlinkable()) {
            DfgVertex& vtx = *vtxp;
            यदि (ध्रुव uint64_t component = m_component.at(vtx)) {
                m_dfg.removeVertex(vtx);
                m_components[component - 1]->addVertex(vtx);
            }
        }
    }

    शून्य checkEdges(DfgGraph& dfg) ध्रुव {
        // Check that edges only cross components at variable boundaries
        dfg.forEachVertex([&](DfgVertex& vtx) {
            यदि (vtx.is<DfgVarPacked>()) प्रत्यागम;
            ध्रुव uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](DfgVertex& snk) {
                यदि (snk.is<DfgVertexVar>()) प्रत्यागम असत्य;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                प्रत्यागम असत्य;
            });
        });
    }

    शून्य checkGraph(DfgGraph& dfg) ध्रुव {
        // Build set of vertices
        std::unordered_set<ध्रुव DfgVertex*> vertices{dfg.size()};
        dfg.forEachVertex([&](ध्रुव DfgVertex& vtx) { vertices.जोड़ो(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.forEachVertex([&](ध्रुव DfgVertex& vtx) {
            vtx.foreachSource([&](ध्रुव DfgVertex& src) {
                UASSERT_OBJ(vertices.गिनती(&src), &vtx, "Source vertex not in graph");
                प्रत्यागम असत्य;
            });
            vtx.foreachSink([&](ध्रुव DfgVertex& snk) {
                UASSERT_OBJ(vertices.गिनती(&snk), &snk, "Sink vertex not in graph");
                प्रत्यागम असत्य;
            });
        });
    }

    शून्य extractComponents(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        प्रति (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(नूतन DfgGraph{m_dfg.ctx(), m_prefix + cvtToStr(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ध्रुव DfgVertex* ध्रुव lastp = m_dfg.varVertices().backp();
        प्रति (DfgVertexVar& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fixEdges(vtx);
            // Don't iterate clones added during this loop
            यदि (&vtx == lastp) भङ्ग;
        }

        // Check results for consistency
        यदि (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkEdges(m_dfg);
            प्रति (ध्रुव स्वतः& dfgp : m_components) checkEdges(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        यदि (VL_UNLIKELY(m_doExpensiveChecks)) {
            checkGraph(m_dfg);
            प्रति (ध्रुव स्वतः& dfgp : m_components) checkGraph(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    explicit ExtractCyclicComponents(DfgGraph& dfg, ध्रुव std::string& label)
        : m_dfg{dfg}
        , m_prefix{dfg.name() + (label.empty() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ध्रुव uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        यदि (!nSCCs) प्रत्यागम;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        extractComponents(nSCCs);
    }

सार्वजनिक:
    स्थिर std::vector<std::unique_ptr<DfgGraph>> apply(DfgGraph& dfg, ध्रुव std::string& label) {
        प्रत्यागम std::move(ExtractCyclicComponents{dfg, label}.m_components);
    }
};

std::vector<std::unique_ptr<DfgGraph>>
DfgGraph::extractCyclicComponents(ध्रुव std::string& label) {
    प्रत्यागम ExtractCyclicComponents::apply(*एतत्, label);
}
