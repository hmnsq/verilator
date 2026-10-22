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

جماعت بانٹو_Into_Components_2 final {

    // STATE
    Dfg_گراف& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<سائز_t> m_component = m_dfg.makeUserMap<سائز_t>();
    ثابت std::string m_سابقہ;  // Component name prefix
    std::سمتیہ<std::unique_ptr<Dfg_گراف>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    سائز_t m_component_شمار = 1;

    خالی colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::سمتیہ<Dfg_راس*> قطار;
        قطار.reserve(m_dfg.سائز());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        برائے (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            اگر (m_component[vtx]) جاری;

            // Start depth first traversal at this vertex
            قطار.ڈالو_عقب(&vtx);

            // Depth first traversal
            کرو {
                // Pop next work item
                Dfg_راس& شے_3 = *قطار.عقب();
                قطار.نکالو_عقب();

                // Move on if already visited
                اگر (m_component[شے_3]) جاری;

                // Assign to current component
                m_component[شے_3] = m_component_شمار;

                // Enqueue all sources and sinks of this vertex.
                شے_3.foreach_ماخذ([&](Dfg_راس& src) {
                    قطار.ڈالو_عقب(&src);
                    واپس جھوٹ;
                });
                شے_3.foreachSink([&](Dfg_راس& dst) {
                    قطار.ڈالو_عقب(&dst);
                    واپس جھوٹ;
                });
            } جب (!قطار.خالی_خانے());

            // Done with this component
            ++m_component_شمار;
        }
    }

    سانچہ <قسم_نام راس_2>
    خالی moveVertices(Dfg_راس::List<راس_2>& list) {
        برائے (Dfg_راس* ثابت vtxp : list.unlinkable()) {
            اگر (ثابت سائز_t component = m_component[vtxp]) {
                m_dfg.remove_راس(*vtxp);
                m_components[component - 1]->جوڑو_راس(*vtxp);
            } ورنہ {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    بانٹو_Into_Components_2(Dfg_گراف& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_سابقہ{dfg.name() + (label.خالی_خانے() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_component_شمار - 1);
        برائے (سائز_t i = 1; i < m_component_شمار; ++i) {
            m_components[i - 1].reset(نیا Dfg_گراف{m_dfg.ctx(), m_سابقہ + cvt_مقصد_Str(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.سائز() == 0, "'this' DfgGraph should have been emptied");
    }

عوامی:
    ساکن std::سمتیہ<std::unique_ptr<Dfg_گراف>> apply(Dfg_گراف& dfg, ثابت std::string& label) {
        واپس std::move(بانٹو_Into_Components_2{dfg, label}.m_components);
    }
};

std::سمتیہ<std::unique_ptr<Dfg_گراف>> Dfg_گراف::بانٹو_Into_Components(ثابت std::string& label) {
    واپس بانٹو_Into_Components_2::apply(*یہ, label);
}

جماعت نکالو_Cyclic_Components_2 final {
    // STATE
    Dfg_گراف& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ثابت std::string m_سابقہ;  // Component name prefix
    ثابت منطقی m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::سمتیہ<std::unique_ptr<Dfg_گراف>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ثابت Dfg_راس_Var*, std::unordered_map<uint64_t, Dfg_راس_Var*>> m_clones;

    // METHODS
    خالی جوڑو_راس_And_Expand_Siblings(Dfg_راس& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        اگر (vtx.is<Dfg_راس_Var>()) واپس;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        اگر (vtxComponentr == component) واپس;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    خالی expandSiblings(Dfg_راس& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](Dfg_راس& v) {
            جوڑو_راس_And_Expand_Siblings(v, component);
            واپس جھوٹ;
        });
        vtx.foreach_ماخذ([&](Dfg_راس& v) {
            جوڑو_راس_And_Expand_Siblings(v, component);
            واپس جھوٹ;
        });
    }

    خالی expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        اگر (m_doExpensiveChecks) {
            برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_مضرب_Sinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        برائے (ثابت Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            ثابت uint64_t varComponent = m_component.at(vtx);
            اگر (!varComponent) جاری;
            اگر (Dfg_راس* ثابت srcp = vtx.srcp()) {
                اگر (!srcp->is<Dfg_راس_Var>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            اگر (Dfg_راس* ثابت defp = vtx.defaultp()) {
                اگر (!defp->is<Dfg_راس_Var>()) {
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
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
            اگر (ثابت uint64_t ہدف_Component = m_component.at(vtx)) {
                expandSiblings(vtx, ہدف_Component);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    Dfg_راس_Var* getClone(Dfg_راس_Var& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        Dfg_راس_Var*& clonep = m_clones[&vtx][component];
        اگر (!clonep) {
            اگر (DfgVarPacked* ثابت pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = نیا DfgVarPacked{m_dfg, pVtxp->vscp()};
            } ورنہ اگر (DfgVarArray* ثابت aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = نیا DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        واپس clonep;
    }

    // Fix edges that cross components
    خالی fix_کنارے(Dfg_راس_Var& vtx) {
        ثابت uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        اگر (Dfg_راس* ثابت sp = vtx.srcp()) {
            ثابت uint64_t srcComponent = m_component.at(sp);
            اگر (srcComponent != component) {
                UASSERT_OBJ(sp->is<Dfg_راس_Var>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(خالی_اشاریہ);
            }
        }
        اگر (Dfg_راس* ثابت dp = vtx.defaultp()) {
            ثابت uint64_t defaultComponent = m_component.at(dp);
            اگر (defaultComponent != component) {
                UASSERT_OBJ(dp->is<Dfg_راس_Var>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(خالی_اشاریہ);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::سمتیہ<Dfg_راس*> sinkps;
        vtx.foreachSink([&](Dfg_راس& sink) {
            sinkps.emplace_عقب(&sink);
            واپس جھوٹ;
        });
        برائے (Dfg_راس* ثابت sinkp : sinkps) {
            ثابت uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            اگر (sinkComponent == component) جاری;
            Dfg_راس* ثابت clonep = getClone(vtx, sinkComponent);
            برائے (سائز_t i = 0; i < sinkp->nInputs(); ++i) {
                اگر (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    سانچہ <قسم_نام راس_2>
    خالی moveVertices(Dfg_راس::List<راس_2>& list) {
        برائے (Dfg_راس* ثابت vtxp : list.unlinkable()) {
            Dfg_راس& vtx = *vtxp;
            اگر (ثابت uint64_t component = m_component.at(vtx)) {
                m_dfg.remove_راس(vtx);
                m_components[component - 1]->جوڑو_راس(vtx);
            }
        }
    }

    خالی check_کنارے(Dfg_گراف& dfg) ثابت {
        // Check that edges only cross components at variable boundaries
        dfg.for_Each_راس([&](Dfg_راس& vtx) {
            اگر (vtx.is<DfgVarPacked>()) واپس;
            ثابت uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](Dfg_راس& snk) {
                اگر (snk.is<Dfg_راس_Var>()) واپس جھوٹ;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                واپس جھوٹ;
            });
        });
    }

    خالی check_گراف(Dfg_گراف& dfg) ثابت {
        // Build set of vertices
        std::unordered_سیٹ<ثابت Dfg_راس*> vertices{dfg.سائز()};
        dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) { vertices.داخل_کرو(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            vtx.foreach_ماخذ([&](ثابت Dfg_راس& src) {
                UASSERT_OBJ(vertices.گنتی(&src), &vtx, "Source vertex not in graph");
                واپس جھوٹ;
            });
            vtx.foreachSink([&](ثابت Dfg_راس& snk) {
                UASSERT_OBJ(vertices.گنتی(&snk), &snk, "Sink vertex not in graph");
                واپس جھوٹ;
            });
        });
    }

    خالی نکالو_Components(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        برائے (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(نیا Dfg_گراف{m_dfg.ctx(), m_سابقہ + cvt_مقصد_Str(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ثابت Dfg_راس* ثابت lastp = m_dfg.varVertices().backp();
        برائے (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fix_کنارے(vtx);
            // Don't iterate clones added during this loop
            اگر (&vtx == lastp) توڑو;
        }

        // Check results for consistency
        اگر (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_کنارے(m_dfg);
            برائے (ثابت خودکار& dfgp : m_components) check_کنارے(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        اگر (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_گراف(m_dfg);
            برائے (ثابت خودکار& dfgp : m_components) check_گراف(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    صریح نکالو_Cyclic_Components_2(Dfg_گراف& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_سابقہ{dfg.name() + (label.خالی_خانے() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ثابت uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        اگر (!nSCCs) واپس;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        نکالو_Components(nSCCs);
    }

عوامی:
    ساکن std::سمتیہ<std::unique_ptr<Dfg_گراف>> apply(Dfg_گراف& dfg, ثابت std::string& label) {
        واپس std::move(نکالو_Cyclic_Components_2{dfg, label}.m_components);
    }
};

std::سمتیہ<std::unique_ptr<Dfg_گراف>>
Dfg_گراف::نکالو_Cyclic_Components(ثابت std::string& label) {
    واپس نکالو_Cyclic_Components_2::apply(*یہ, label);
}
