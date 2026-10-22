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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف ورهايو_Into_Components_2 final {

    // STATE
    Dfg_گراف& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<سائيز_t> m_component = m_dfg.makeUserMap<سائيز_t>();
    ثابت std::string m_اڳياڙي;  // Component name prefix
    std::ويڪٽر<std::unique_ptr<Dfg_گراف>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    سائيز_t m_componentCounter = 1;

    فراغ colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::ويڪٽر<Dfg_راس*> قطار;
        قطار.reserve(m_dfg.سائيز());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            إذا (m_component[vtx]) استمر;

            // Start depth first traversal at this vertex
            قطار.ڌڪو_پٺ(&vtx);

            // Depth first traversal
            افعل {
                // Pop next work item
                Dfg_راس& شي_2 = *قطار.پٺ();
                قطار.ڪڍو_پٺ();

                // Move on if already visited
                إذا (m_component[شي_2]) استمر;

                // Assign to current component
                m_component[شي_2] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                شي_2.foreach_ذريعو([&](Dfg_راس& src) {
                    قطار.ڌڪو_پٺ(&src);
                    أرجع خطأ;
                });
                شي_2.foreachSink([&](Dfg_راس& dst) {
                    قطار.ڌڪو_پٺ(&dst);
                    أرجع خطأ;
                });
            } طالما (!قطار.خالي_جايون());

            // Done with this component
            ++m_componentCounter;
        }
    }

    سانچو <قسم_نالو راس_2>
    فراغ moveVertices(Dfg_راس::List<راس_2>& list) {
        لكل (Dfg_راس* ثابت vtxp : list.unlinkable()) {
            إذا (ثابت سائيز_t component = m_component[vtxp]) {
                m_dfg.remove_راس(*vtxp);
                m_components[component - 1]->جوڙيو_راس(*vtxp);
            } وإلا {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    ورهايو_Into_Components_2(Dfg_گراف& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_اڳياڙي{dfg.name() + (label.خالي_جايون() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        لكل (سائيز_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(جديد Dfg_گراف{m_dfg.ctx(), m_اڳياڙي + cvt_منزل_Str(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.سائيز() == 0, "'this' DfgGraph should have been emptied");
    }

عام:
    ساكن std::ويڪٽر<std::unique_ptr<Dfg_گراف>> apply(Dfg_گراف& dfg, ثابت std::string& label) {
        أرجع std::move(ورهايو_Into_Components_2{dfg, label}.m_components);
    }
};

std::ويڪٽر<std::unique_ptr<Dfg_گراف>> Dfg_گراف::ورهايو_Into_Components(ثابت std::string& label) {
    أرجع ورهايو_Into_Components_2::apply(*هذا, label);
}

صنف ڪڍو_Cyclic_Components_2 final {
    // STATE
    Dfg_گراف& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ثابت std::string m_اڳياڙي;  // Component name prefix
    ثابت منطقي m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::ويڪٽر<std::unique_ptr<Dfg_گراف>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ثابت Dfg_راس_Var*, std::unordered_map<uint64_t, Dfg_راس_Var*>> m_clones;

    // METHODS
    فراغ جوڙيو_راس_And_Expand_Siblings(Dfg_راس& vtx, uint64_t component) {
        // Do not go past a variable, we will partition the graph there
        إذا (vtx.is<Dfg_راس_Var>()) أرجع;
        // Pick up component value reference
        uint64_t& vtxComponentr = m_component.at(vtx);
        // Don't need to recurse if the vertex is already in the same component,
        // it was either marked through an earlier traversal, in which case it
        // was processed recursively, or it will be processed later.
        إذا (vtxComponentr == component) أرجع;
        // Because all cycles are through a variable, we can't reach another SCC.
        UASSERT_OBJ(!vtxComponentr, &vtx, "Cycle without variable involvement");
        // Put this vertex in the component, and continue recursively
        vtxComponentr = component;
        expandSiblings(vtx, component);
    }

    فراغ expandSiblings(Dfg_راس& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) == component, &vtx, "Traversal didn't stop");
        vtx.foreachSink([&](Dfg_راس& v) {
            جوڙيو_راس_And_Expand_Siblings(v, component);
            أرجع خطأ;
        });
        vtx.foreach_ذريعو([&](Dfg_راس& v) {
            جوڙيو_راس_And_Expand_Siblings(v, component);
            أرجع خطأ;
        });
    }

    فراغ expandComponents() {
        // Important fact that we will assume below: There are no path between
        // any two SCCs that do not go through a variable before reaching the
        // destination SCC. That is, to get from one SCC to another, you must
        // go through a variable that is not part of the destination SCC. This
        // holds because no operation vertex can have multiple sinks at this
        // point (constants have no inputs, so they are not in an SCC).
        إذا (m_doExpensiveChecks) {
            لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
                UASSERT_OBJ(!vtx.has_گڻو_Sinks(), &vtx, "Operation has multiple sinks");
            }
        }

        // We will break the graph at variable boundaries, but we want both
        // 'srcp', and 'defaultp' to be in the same component, so for each
        // cyclic variable, put both its 'srcp' and 'defaultp' into the same
        // component if they are not variables themselves. The assertions below
        // must hold because of the assumption above.
        لكل (ثابت Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            ثابت uint64_t varComponent = m_component.at(vtx);
            إذا (!varComponent) استمر;
            إذا (Dfg_راس* ثابت srcp = vtx.srcp()) {
                إذا (!srcp->is<Dfg_راس_Var>()) {
                    uint64_t& srcComponent = m_component.at(srcp);
                    UASSERT_OBJ(!srcComponent || srcComponent == varComponent, srcp,
                                "Cycle through 'srcp' that does not go through variable.");
                    srcComponent = varComponent;
                }
            }
            إذا (Dfg_راس* ثابت defp = vtx.defaultp()) {
                إذا (!defp->is<Dfg_راس_Var>()) {
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
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
            إذا (ثابت uint64_t هدف_Component = m_component.at(vtx)) {
                expandSiblings(vtx, هدف_Component);
            }
        }
    }

    // Retrieve clone of vertex in the given component
    Dfg_راس_Var* getClone(Dfg_راس_Var& vtx, uint64_t component) {
        UASSERT_OBJ(m_component.at(vtx) != component, &vtx, "Vertex is in that component");
        Dfg_راس_Var*& clonep = m_clones[&vtx][component];
        إذا (!clonep) {
            إذا (DfgVarPacked* ثابت pVtxp = vtx.cast<DfgVarPacked>()) {
                clonep = جديد DfgVarPacked{m_dfg, pVtxp->vscp()};
            } وإلا إذا (DfgVarArray* ثابت aVtxp = vtx.cast<DfgVarArray>()) {
                clonep = جديد DfgVarArray{m_dfg, aVtxp->vscp()};
            }
            UASSERT_OBJ(clonep, &vtx, "Unhandled 'DfgVertexVar' sub-type");
            m_component[clonep] = component;
            clonep->tmpForp(vtx.tmpForp());
        }
        أرجع clonep;
    }

    // Fix edges that cross components
    فراغ fix_ڪنارا(Dfg_راس_Var& vtx) {
        ثابت uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        إذا (Dfg_راس* ثابت sp = vtx.srcp()) {
            ثابت uint64_t srcComponent = m_component.at(sp);
            إذا (srcComponent != component) {
                UASSERT_OBJ(sp->is<Dfg_راس_Var>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(خالي_اشارو);
            }
        }
        إذا (Dfg_راس* ثابت dp = vtx.defaultp()) {
            ثابت uint64_t defaultComponent = m_component.at(dp);
            إذا (defaultComponent != component) {
                UASSERT_OBJ(dp->is<Dfg_راس_Var>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(خالي_اشارو);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::ويڪٽر<Dfg_راس*> sinkps;
        vtx.foreachSink([&](Dfg_راس& sink) {
            sinkps.emplace_پٺ(&sink);
            أرجع خطأ;
        });
        لكل (Dfg_راس* ثابت sinkp : sinkps) {
            ثابت uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            إذا (sinkComponent == component) استمر;
            Dfg_راس* ثابت clonep = getClone(vtx, sinkComponent);
            لكل (سائيز_t i = 0; i < sinkp->nInputs(); ++i) {
                إذا (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    سانچو <قسم_نالو راس_2>
    فراغ moveVertices(Dfg_راس::List<راس_2>& list) {
        لكل (Dfg_راس* ثابت vtxp : list.unlinkable()) {
            Dfg_راس& vtx = *vtxp;
            إذا (ثابت uint64_t component = m_component.at(vtx)) {
                m_dfg.remove_راس(vtx);
                m_components[component - 1]->جوڙيو_راس(vtx);
            }
        }
    }

    فراغ check_ڪنارا(Dfg_گراف& dfg) ثابت {
        // Check that edges only cross components at variable boundaries
        dfg.for_Each_راس([&](Dfg_راس& vtx) {
            إذا (vtx.is<DfgVarPacked>()) أرجع;
            ثابت uint64_t component = m_component.at(vtx);
            vtx.foreachSink([&](Dfg_راس& snk) {
                إذا (snk.is<Dfg_راس_Var>()) أرجع خطأ;  // OK to cross at variables
                UASSERT_OBJ(component == m_component.at(snk), &vtx,
                            "Edge crossing components without variable involvement");
                أرجع خطأ;
            });
        });
    }

    فراغ check_گراف(Dfg_گراف& dfg) ثابت {
        // Build set of vertices
        std::unordered_سيٽ<ثابت Dfg_راس*> vertices{dfg.سائيز()};
        dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) { vertices.وجهو(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            vtx.foreach_ذريعو([&](ثابت Dfg_راس& src) {
                UASSERT_OBJ(vertices.ڳڻپ(&src), &vtx, "Source vertex not in graph");
                أرجع خطأ;
            });
            vtx.foreachSink([&](ثابت Dfg_راس& snk) {
                UASSERT_OBJ(vertices.ڳڻپ(&snk), &snk, "Sink vertex not in graph");
                أرجع خطأ;
            });
        });
    }

    فراغ ڪڍو_Components(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        لكل (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(جديد Dfg_گراف{m_dfg.ctx(), m_اڳياڙي + cvt_منزل_Str(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ثابت Dfg_راس* ثابت lastp = m_dfg.varVertices().backp();
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fix_ڪنارا(vtx);
            // Don't iterate clones added during this loop
            إذا (&vtx == lastp) اكسر;
        }

        // Check results for consistency
        إذا (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_ڪنارا(m_dfg);
            لكل (ثابت خودڪار& dfgp : m_components) check_ڪنارا(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        إذا (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_گراف(m_dfg);
            لكل (ثابت خودڪار& dfgp : m_components) check_گراف(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    واضح ڪڍو_Cyclic_Components_2(Dfg_گراف& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_اڳياڙي{dfg.name() + (label.خالي_جايون() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ثابت uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        إذا (!nSCCs) أرجع;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        ڪڍو_Components(nSCCs);
    }

عام:
    ساكن std::ويڪٽر<std::unique_ptr<Dfg_گراف>> apply(Dfg_گراف& dfg, ثابت std::string& label) {
        أرجع std::move(ڪڍو_Cyclic_Components_2{dfg, label}.m_components);
    }
};

std::ويڪٽر<std::unique_ptr<Dfg_گراف>>
Dfg_گراف::ڪڍو_Cyclic_Components(ثابت std::string& label) {
    أرجع ڪڍو_Cyclic_Components_2::apply(*هذا, label);
}
