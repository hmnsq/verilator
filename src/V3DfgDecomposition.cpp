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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి విభజించు_Into_Components_2 final {

    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<కొలత_t> m_component = m_dfg.makeUserMap<కొలత_t>();
    స్థిరరాశి std::string m_ఉపసర్గ;  // Component name prefix
    std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    కొలత_t m_componentCounter = 1;

    శూన్య colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::సదిశ<Dfg_శీర్షం*> వరుస;
        వరుస.reserve(m_dfg.కొలత());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        కొరకు (Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            ఒకవేళ (m_component[vtx]) కొనసాగు;

            // Start depth first traversal at this vertex
            వరుస.నెట్టు_వెనుక(&vtx);

            // Depth first traversal
            చేయి {
                // Pop next work item
                Dfg_శీర్షం& అంశం_2 = *వరుస.వెనుక();
                వరుస.తీయి_వెనుక();

                // Move on if already visited
                ఒకవేళ (m_component[అంశం_2]) కొనసాగు;

                // Assign to current component
                m_component[అంశం_2] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                అంశం_2.foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                    వరుస.నెట్టు_వెనుక(&src);
                    తిరిగి అబద్ధం;
                });
                అంశం_2.foreachSink([&](Dfg_శీర్షం& dst) {
                    వరుస.నెట్టు_వెనుక(&dst);
                    తిరిగి అబద్ధం;
                });
            } అయితే (!వరుస.ఖాళీలు());

            // Done with this component
            ++m_componentCounter;
        }
    }

    మూస <రకం_పేరు శీర్షం_2>
    శూన్య moveVertices(Dfg_శీర్షం::List<శీర్షం_2>& list) {
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : list.unlinkable()) {
            ఒకవేళ (స్థిరరాశి కొలత_t component = m_component[vtxp]) {
                m_dfg.remove_శీర్షం(*vtxp);
                m_components[component - 1]->కూడు_శీర్షం(*vtxp);
            } లేకపోతే {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    విభజించు_Into_Components_2(Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి std::string& label)
        : m_dfg{dfg}
        , m_ఉపసర్గ{dfg.name() + (label.ఖాళీలు() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        కొరకు (కొలత_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(కొత్త Dfg_లేఖాచిత్రం{m_dfg.ctx(), m_ఉపసర్గ + cvt_గమ్యం_Str(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.కొలత() == 0, "'this' DfgGraph should have been emptied");
    }

బహిరంగ:
    స్థిర std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> apply(Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి std::string& label) {
        తిరిగి std::move(విభజించు_Into_Components_2{dfg, label}.m_components);
    }
};

std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> Dfg_లేఖాచిత్రం::విభజించు_Into_Components(స్థిరరాశి std::string& label) {
    తిరిగి విభజించు_Into_Components_2::apply(*ఇది, label);
}

తరగతి వెలికితీయి_Cyclic_Components_2 final {
    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    స్థిరరాశి std::string m_ఉపసర్గ;  // Component name prefix
    స్థిరరాశి తార్కిక m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<స్థిరరాశి Dfg_శీర్షం_Var*, std::unordered_map<uint64_t, Dfg_శీర్షం_Var*>> m_clones;

    // METHODS
    శూన్య కూడు_శీర్షం_And_Expand_Siblings(Dfg_శీర్షం& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        ఒకవేళ (vtx.is<Dfg_శీర్షం_Var>()) తిరిగి;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        ఒకవేళ (vtxComponentr == component) తిరిగి;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    శూన్య expandSiblings(Dfg_శీర్షం& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](Dfg_శీర్షం& v) {
            కూడు_శీర్షం_And_Expand_Siblings(v, component);
            తిరిగి అబద్ధం;
        });
        vtx.foreach_ఆధారం([&](Dfg_శీర్షం& v) {
            కూడు_శీర్షం_And_Expand_Siblings(v, component);
            తిరిగి అబద్ధం;
        });
    }

    శూన్య expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        ఒకవేళ (m_doExpensiveChecks) {
            కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_గుణిజం_Sinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) {
            స్థిరరాశి uint64_t varComponent = m_component.at(vtx);
            ఒకవేళ (!varComponent) కొనసాగు;
            ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి srcp = vtx.srcp()) {
                ఒకవేళ (!srcp->is<Dfg_శీర్షం_Var>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి defp = vtx.defaultp()) {
                ఒకవేళ (!defp->is<Dfg_శీర్షం_Var>()) {
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
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            ఒకవేళ (స్థిరరాశి uint64_t లక్ష్యం_Component = m_component.at(vtx)) {
                expandSiblings(vtx, లక్ష్యం_Component);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    Dfg_శీర్షం_Var* getClone(Dfg_శీర్షం_Var& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        Dfg_శీర్షం_Var*& clonep = m_clones[&vtx][component];
        ఒకవేళ (!clonep) {
            ఒకవేళ (DfgVarPacked* స్థిరరాశి pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = కొత్త DfgVarPacked{m_dfg, pVtxp->vscp()};
            } లేకపోతే ఒకవేళ (DfgVarArray* స్థిరరాశి aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = కొత్త DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        తిరిగి clonep;
    }

    // Fix edges that cross components
    శూన్య fix_అంచులు(Dfg_శీర్షం_Var& vtx) {
        స్థిరరాశి uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి sp = vtx.srcp()) {
            స్థిరరాశి uint64_t srcComponent = m_component.at(sp);
            ఒకవేళ (srcComponent != component) {
                UASSERT_OBJ(sp->is<Dfg_శీర్షం_Var>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(శూన్యసూచిక);
            }
        }
        ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి dp = vtx.defaultp()) {
            స్థిరరాశి uint64_t defaultComponent = m_component.at(dp);
            ఒకవేళ (defaultComponent != component) {
                UASSERT_OBJ(dp->is<Dfg_శీర్షం_Var>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(శూన్యసూచిక);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::సదిశ<Dfg_శీర్షం*> sinkps;
        vtx.foreachSink([&](Dfg_శీర్షం& sink) {
            sinkps.emplace_వెనుక(&sink);
            తిరిగి అబద్ధం;
        });
        కొరకు (Dfg_శీర్షం* స్థిరరాశి sinkp : sinkps) {
            స్థిరరాశి uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            ఒకవేళ (sinkComponent == component) కొనసాగు;
            Dfg_శీర్షం* స్థిరరాశి clonep = getClone(vtx, sinkComponent);
            కొరకు (కొలత_t i = 0; i < sinkp->nInputs(); ++i) {
                ఒకవేళ (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    మూస <రకం_పేరు శీర్షం_2>
    శూన్య moveVertices(Dfg_శీర్షం::List<శీర్షం_2>& list) {
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : list.unlinkable()) {
            Dfg_శీర్షం& vtx = *vtxp;
            ఒకవేళ (స్థిరరాశి uint64_t component = m_component.at(vtx)) {
                m_dfg.remove_శీర్షం(vtx);
                m_components[component - 1]->కూడు_శీర్షం(vtx);
            }
        }
    }

    శూన్య check_అంచులు(Dfg_లేఖాచిత్రం& dfg) స్థిరరాశి {
        // Check that edges only cross components at variable boundaries
        dfg.for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) {
            ఒకవేళ (vtx.is<DfgVarPacked>()) తిరిగి;
            స్థిరరాశి uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](Dfg_శీర్షం& snk) {
                ఒకవేళ (snk.is<Dfg_శీర్షం_Var>()) తిరిగి అబద్ధం;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                తిరిగి అబద్ధం;
            });
        });
    }

    శూన్య check_లేఖాచిత్రం(Dfg_లేఖాచిత్రం& dfg) స్థిరరాశి {
        // Build set of vertices
        std::unordered_సమితి<స్థిరరాశి Dfg_శీర్షం*> vertices{dfg.కొలత()};
        dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) { vertices.చొప్పించు(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
            vtx.foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
                UASSERT_OBJ(vertices.లెక్క(&src), &vtx, "Source vertex not in graph");
                తిరిగి అబద్ధం;
            });
            vtx.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& snk) {
                UASSERT_OBJ(vertices.లెక్క(&snk), &snk, "Sink vertex not in graph");
                తిరిగి అబద్ధం;
            });
        });
    }

    శూన్య వెలికితీయి_Components(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        కొరకు (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(కొత్త Dfg_లేఖాచిత్రం{m_dfg.ctx(), m_ఉపసర్గ + cvt_గమ్యం_Str(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి lastp = m_dfg.varVertices().backp();
        కొరకు (Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fix_అంచులు(vtx);
            // Don't iterate clones added during this loop
            ఒకవేళ (&vtx == lastp) ఆపు;
        }

        // Check results for consistency
        ఒకవేళ (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_అంచులు(m_dfg);
            కొరకు (స్థిరరాశి స్వయంచాలక& dfgp : m_components) check_అంచులు(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        ఒకవేళ (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_లేఖాచిత్రం(m_dfg);
            కొరకు (స్థిరరాశి స్వయంచాలక& dfgp : m_components) check_లేఖాచిత్రం(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    స్పష్టమైన వెలికితీయి_Cyclic_Components_2(Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి std::string& label)
        : m_dfg{dfg}
        , m_ఉపసర్గ{dfg.name() + (label.ఖాళీలు() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        స్థిరరాశి uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        ఒకవేళ (!nSCCs) తిరిగి;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        వెలికితీయి_Components(nSCCs);
    }

బహిరంగ:
    స్థిర std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> apply(Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి std::string& label) {
        తిరిగి std::move(వెలికితీయి_Cyclic_Components_2{dfg, label}.m_components);
    }
};

std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>>
Dfg_లేఖాచిత్రం::వెలికితీయి_Cyclic_Components(స్థిరరాశి std::string& label) {
    తిరిగి వెలికితీయి_Cyclic_Components_2::apply(*ఇది, label);
}
