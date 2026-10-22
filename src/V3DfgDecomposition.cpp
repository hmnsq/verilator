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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ ਵੰਡੋ_Into_Components_2 final {

    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<ਮਾਪ_t> m_component = m_dfg.makeUserMap<ਮਾਪ_t>();
    ਅਚਲ std::string m_ਅਗੇਤਰ;  // Component name prefix
    std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    ਮਾਪ_t m_componentCounter = 1;

    ਖਾਲੀ colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> ਕਤਾਰ;
        ਕਤਾਰ.reserve(m_dfg.ਮਾਪ());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            ਜੇ (m_component[vtx]) ਜਾਰੀ;

            // Start depth first traversal at this vertex
            ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(&vtx);

            // Depth first traversal
            ਕਰੋ {
                // Pop next work item
                Dfg_ਸ਼ੀਰਸ਼& ਵਸਤੂ_3 = *ਕਤਾਰ.ਪਿੱਠ();
                ਕਤਾਰ.ਕੱਢੋ_ਪਿੱਠ();

                // Move on if already visited
                ਜੇ (m_component[ਵਸਤੂ_3]) ਜਾਰੀ;

                // Assign to current component
                m_component[ਵਸਤੂ_3] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                ਵਸਤੂ_3.foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                    ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(&src);
                    ਵਾਪਸ ਝੂਠ;
                });
                ਵਸਤੂ_3.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
                    ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(&dst);
                    ਵਾਪਸ ਝੂਠ;
                });
            } ਜਦੋਂ (!ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ());

            // Done with this component
            ++m_componentCounter;
        }
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    ਖਾਲੀ moveVertices(Dfg_ਸ਼ੀਰਸ਼::List<ਸ਼ੀਰਸ਼_2>& list) {
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : list.unlinkable()) {
            ਜੇ (ਅਚਲ ਮਾਪ_t component = m_component[vtxp]) {
                m_dfg.remove_ਸ਼ੀਰਸ਼(*vtxp);
                m_components[component - 1]->ਜੋੜੋ_ਸ਼ੀਰਸ਼(*vtxp);
            } ਨਹੀਂਤਾਂ {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    ਵੰਡੋ_Into_Components_2(Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ std::string& label)
        : m_dfg{dfg}
        , m_ਅਗੇਤਰ{dfg.name() + (label.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        ਲਈ (ਮਾਪ_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(ਨਵਾਂ Dfg_ਗ੍ਰਾਫ{m_dfg.ctx(), m_ਅਗੇਤਰ + cvt_ਮੰਜ਼ਿਲ_Str(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.ਮਾਪ() == 0, "'this' DfgGraph should have been emptied");
    }

ਜਨਤਕ:
    ਸਥਿਰ std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> apply(Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ std::string& label) {
        ਵਾਪਸ std::move(ਵੰਡੋ_Into_Components_2{dfg, label}.m_components);
    }
};

std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> Dfg_ਗ੍ਰਾਫ::ਵੰਡੋ_Into_Components(ਅਚਲ std::string& label) {
    ਵਾਪਸ ਵੰਡੋ_Into_Components_2::apply(*ਇਹ, label);
}

ਵਰਗ ਕੱਢੋ_Cyclic_Components_2 final {
    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ਅਚਲ std::string m_ਅਗੇਤਰ;  // Component name prefix
    ਅਚਲ ਤਾਰਕਿਕ m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var*, std::unordered_map<uint64_t, Dfg_ਸ਼ੀਰਸ਼_Var*>> m_clones;

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਸ਼ੀਰਸ਼_And_Expand_Siblings(Dfg_ਸ਼ੀਰਸ਼& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        ਜੇ (vtx.is<Dfg_ਸ਼ੀਰਸ਼_Var>()) ਵਾਪਸ;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        ਜੇ (vtxComponentr == component) ਵਾਪਸ;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    ਖਾਲੀ expandSiblings(Dfg_ਸ਼ੀਰਸ਼& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& v) {
            ਜੋੜੋ_ਸ਼ੀਰਸ਼_And_Expand_Siblings(v, component);
            ਵਾਪਸ ਝੂਠ;
        });
        vtx.foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& v) {
            ਜੋੜੋ_ਸ਼ੀਰਸ਼_And_Expand_Siblings(v, component);
            ਵਾਪਸ ਝੂਠ;
        });
    }

    ਖਾਲੀ expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        ਜੇ (m_doExpensiveChecks) {
            ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_ਗੁਣਜ_Sinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) {
            ਅਚਲ uint64_t varComponent = m_component.at(vtx);
            ਜੇ (!varComponent) ਜਾਰੀ;
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtx.srcp()) {
                ਜੇ (!srcp->is<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ defp = vtx.defaultp()) {
                ਜੇ (!defp->is<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
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
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            ਜੇ (ਅਚਲ uint64_t ਨਿਸ਼ਾਨਾ_Component = m_component.at(vtx)) {
                expandSiblings(vtx, ਨਿਸ਼ਾਨਾ_Component);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    Dfg_ਸ਼ੀਰਸ਼_Var* getClone(Dfg_ਸ਼ੀਰਸ਼_Var& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        Dfg_ਸ਼ੀਰਸ਼_Var*& clonep = m_clones[&vtx][component];
        ਜੇ (!clonep) {
            ਜੇ (DfgVarPacked* ਅਚਲ pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = ਨਵਾਂ DfgVarPacked{m_dfg, pVtxp->vscp()};
            } ਨਹੀਂਤਾਂ ਜੇ (DfgVarArray* ਅਚਲ aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = ਨਵਾਂ DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        ਵਾਪਸ clonep;
    }

    // Fix edges that cross components
    ਖਾਲੀ fix_ਕਿਨਾਰੇ(Dfg_ਸ਼ੀਰਸ਼_Var& vtx) {
        ਅਚਲ uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ sp = vtx.srcp()) {
            ਅਚਲ uint64_t srcComponent = m_component.at(sp);
            ਜੇ (srcComponent != component) {
                UASSERT_OBJ(sp->is<Dfg_ਸ਼ੀਰਸ਼_Var>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(ਖਾਲੀ_ਸੂਚਕ);
            }
        }
        ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ dp = vtx.defaultp()) {
            ਅਚਲ uint64_t defaultComponent = m_component.at(dp);
            ਜੇ (defaultComponent != component) {
                UASSERT_OBJ(dp->is<Dfg_ਸ਼ੀਰਸ਼_Var>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(ਖਾਲੀ_ਸੂਚਕ);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> sinkps;
        vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
            sinkps.emplace_ਪਿੱਠ(&sink);
            ਵਾਪਸ ਝੂਠ;
        });
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ sinkp : sinkps) {
            ਅਚਲ uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            ਜੇ (sinkComponent == component) ਜਾਰੀ;
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ clonep = getClone(vtx, sinkComponent);
            ਲਈ (ਮਾਪ_t i = 0; i < sinkp->nInputs(); ++i) {
                ਜੇ (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    ਖਾਲੀ moveVertices(Dfg_ਸ਼ੀਰਸ਼::List<ਸ਼ੀਰਸ਼_2>& list) {
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : list.unlinkable()) {
            Dfg_ਸ਼ੀਰਸ਼& vtx = *vtxp;
            ਜੇ (ਅਚਲ uint64_t component = m_component.at(vtx)) {
                m_dfg.remove_ਸ਼ੀਰਸ਼(vtx);
                m_components[component - 1]->ਜੋੜੋ_ਸ਼ੀਰਸ਼(vtx);
            }
        }
    }

    ਖਾਲੀ check_ਕਿਨਾਰੇ(Dfg_ਗ੍ਰਾਫ& dfg) ਅਚਲ {
        // Check that edges only cross components at variable boundaries
        dfg.for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਜੇ (vtx.is<DfgVarPacked>()) ਵਾਪਸ;
            ਅਚਲ uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& snk) {
                ਜੇ (snk.is<Dfg_ਸ਼ੀਰਸ਼_Var>()) ਵਾਪਸ ਝੂਠ;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                ਵਾਪਸ ਝੂਠ;
            });
        });
    }

    ਖਾਲੀ check_ਗ੍ਰਾਫ(Dfg_ਗ੍ਰਾਫ& dfg) ਅਚਲ {
        // Build set of vertices
        std::unordered_ਸੈੱਟ<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*> vertices{dfg.ਮਾਪ()};
        dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) { vertices.ਪਾਓ(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
            vtx.foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
                UASSERT_OBJ(vertices.ਗਿਣਤੀ_2(&src), &vtx, "Source vertex not in graph");
                ਵਾਪਸ ਝੂਠ;
            });
            vtx.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& snk) {
                UASSERT_OBJ(vertices.ਗਿਣਤੀ_2(&snk), &snk, "Sink vertex not in graph");
                ਵਾਪਸ ਝੂਠ;
            });
        });
    }

    ਖਾਲੀ ਕੱਢੋ_Components(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        ਲਈ (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(ਨਵਾਂ Dfg_ਗ੍ਰਾਫ{m_dfg.ctx(), m_ਅਗੇਤਰ + cvt_ਮੰਜ਼ਿਲ_Str(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lastp = m_dfg.varVertices().backp();
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fix_ਕਿਨਾਰੇ(vtx);
            // Don't iterate clones added during this loop
            ਜੇ (&vtx == lastp) ਰੋਕੋ;
        }

        // Check results for consistency
        ਜੇ (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_ਕਿਨਾਰੇ(m_dfg);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& dfgp : m_components) check_ਕਿਨਾਰੇ(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        ਜੇ (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_ਗ੍ਰਾਫ(m_dfg);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& dfgp : m_components) check_ਗ੍ਰਾਫ(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    ਸਪਸ਼ਟ ਕੱਢੋ_Cyclic_Components_2(Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ std::string& label)
        : m_dfg{dfg}
        , m_ਅਗੇਤਰ{dfg.name() + (label.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ਅਚਲ uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        ਜੇ (!nSCCs) ਵਾਪਸ;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        ਕੱਢੋ_Components(nSCCs);
    }

ਜਨਤਕ:
    ਸਥਿਰ std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> apply(Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ std::string& label) {
        ਵਾਪਸ std::move(ਕੱਢੋ_Cyclic_Components_2{dfg, label}.m_components);
    }
};

std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>>
Dfg_ਗ੍ਰਾਫ::ਕੱਢੋ_Cyclic_Components(ਅਚਲ std::string& label) {
    ਵਾਪਸ ਕੱਢੋ_Cyclic_Components_2::apply(*ਇਹ, label);
}
