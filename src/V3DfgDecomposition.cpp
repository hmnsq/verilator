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

صنف وېشل_Into_Components_2 final {

    // STATE
    Dfg_ګراف& m_dfg;  // The input graph
    // Map from vertices to the weekly connected component they belong to
    DfgUserMap<اندازه_t> m_component = m_dfg.makeUserMap<اندازه_t>();
    ثابت std::string m_مختاړی;  // Component name prefix
    std::ویکټور<std::unique_ptr<Dfg_ګراف>> m_components;  // The extracted components
    // Component counter - starting from 1 as 0 is the default value used as a marker
    اندازه_t m_componentCounter = 1;

    فراغ colorComponents() {
        // Work queue for depth first traversal starting from this vertex
        std::ویکټور<Dfg_راس*> کتار;
        کتار.reserve(m_dfg.اندازه());

        // Any sort of interesting logic must involve a variable, so we only need to iterate them
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // If already assigned this vertex to a component, then continue
            إذا (m_component[vtx]) استمر;

            // Start depth first traversal at this vertex
            کتار.ټېل_شا(&vtx);

            // Depth first traversal
            افعل {
                // Pop next work item
                Dfg_راس& توکی_2 = *کتار.شا();
                کتار.راوباسه_شا();

                // Move on if already visited
                إذا (m_component[توکی_2]) استمر;

                // Assign to current component
                m_component[توکی_2] = m_componentCounter;

                // Enqueue all sources and sinks of this vertex.
                توکی_2.foreach_سرچینه([&](Dfg_راس& src) {
                    کتار.ټېل_شا(&src);
                    أرجع خطأ;
                });
                توکی_2.foreachSink([&](Dfg_راس& dst) {
                    کتار.ټېل_شا(&dst);
                    أرجع خطأ;
                });
            } طالما (!کتار.تشه());

            // Done with this component
            ++m_componentCounter;
        }
    }

    کينډۍ <ډول_نوم راس_2>
    فراغ moveVertices(Dfg_راس::List<راس_2>& list) {
        لكل (Dfg_راس* ثابت vtxp : list.unlinkable()) {
            إذا (ثابت اندازه_t component = m_component[vtxp]) {
                m_dfg.remove_راس(*vtxp);
                m_components[component - 1]->جمع_کړه_راس(*vtxp);
            } وإلا {
                // This vertex is not connected to a variable and is hence unused, remove here
                VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
            }
        }
    }

    وېشل_Into_Components_2(Dfg_ګراف& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_مختاړی{dfg.name() + (label.تشه() ? "" : "-") + label + "-component-"} {
        // Color each component of the graph
        colorComponents();
        // Allocate the component graphs
        m_components.resize(m_componentCounter - 1);
        لكل (اندازه_t i = 1; i < m_componentCounter; ++i) {
            m_components[i - 1].reset(جديد Dfg_ګراف{m_dfg.ctx(), m_مختاړی + cvt_منزل_Str(i - 1)});
        }
        // Move the vertices to the component graphs
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());
        //
        UASSERT(m_dfg.اندازه() == 0, "'this' DfgGraph should have been emptied");
    }

عام:
    ساكن std::ویکټور<std::unique_ptr<Dfg_ګراف>> apply(Dfg_ګراف& dfg, ثابت std::string& label) {
        أرجع std::move(وېشل_Into_Components_2{dfg, label}.m_components);
    }
};

std::ویکټور<std::unique_ptr<Dfg_ګراف>> Dfg_ګراف::وېشل_Into_Components(ثابت std::string& label) {
    أرجع وېشل_Into_Components_2::apply(*هذا, label);
}

صنف ایستل_Cyclic_Components_2 final {
    // STATE
    Dfg_ګراف& m_dfg;  // The input graph
    // Map from vertices to the component they belong to
    DfgUserMap<uint64_t> m_component = m_dfg.makeUserMap<uint64_t>();
    ثابت std::string m_مختاړی;  // Component name prefix
    ثابت منطقي m_doExpensiveChecks = v3Global.opt.debugCheck();
    // The extracted cyclic components
    std::ویکټور<std::unique_ptr<Dfg_ګراف>> m_components;
    // Map from 'variable vertex' -> 'component index' -> 'clone in that component'
    std::unordered_map<ثابت Dfg_راس_Var*, std::unordered_map<uint64_t, Dfg_راس_Var*>> m_clones;

    // METHODS
    فراغ جمع_کړه_راس_And_Expand_Siblings(Dfg_راس& vtx, uint64_t component) {
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
            جمع_کړه_راس_And_Expand_Siblings(v, component);
            أرجع خطأ;
        });
        vtx.foreach_سرچینه([&](Dfg_راس& v) {
            جمع_کړه_راس_And_Expand_Siblings(v, component);
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
                UASSERT_OBJ(!vtx.has_مضرب_Sinks(), &vtx, "Operation has multiple sinks");
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
            إذا (ثابت uint64_t موخه_Component = m_component.at(vtx)) {
                expandSiblings(vtx, موخه_Component);
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
    فراغ fix_څنډې(Dfg_راس_Var& vtx) {
        ثابت uint64_t component = m_component.at(vtx);

        // Fix up srcp and dstp (they must be the same component, or variable)
        إذا (Dfg_راس* ثابت sp = vtx.srcp()) {
            ثابت uint64_t srcComponent = m_component.at(sp);
            إذا (srcComponent != component) {
                UASSERT_OBJ(sp->is<Dfg_راس_Var>(), &vtx, "'srcp' in different component");
                getClone(vtx, srcComponent)->srcp(sp);
                vtx.srcp(تش_اشاره);
            }
        }
        إذا (Dfg_راس* ثابت dp = vtx.defaultp()) {
            ثابت uint64_t defaultComponent = m_component.at(dp);
            إذا (defaultComponent != component) {
                UASSERT_OBJ(dp->is<Dfg_راس_Var>(), &vtx, "'defaultp' in different component");
                getClone(vtx, defaultComponent)->defaultp(dp);
                vtx.defaultp(تش_اشاره);
            }
        }
        // Fix up sinks in a different component to read the clone
        std::ویکټور<Dfg_راس*> sinkps;
        vtx.foreachSink([&](Dfg_راس& sink) {
            sinkps.emplace_شا(&sink);
            أرجع خطأ;
        });
        لكل (Dfg_راس* ثابت sinkp : sinkps) {
            ثابت uint64_t sinkComponent = m_component.at(sinkp);
            // Same component is OK
            إذا (sinkComponent == component) استمر;
            Dfg_راس* ثابت clonep = getClone(vtx, sinkComponent);
            لكل (اندازه_t i = 0; i < sinkp->nInputs(); ++i) {
                إذا (sinkp->inputp(i) == &vtx) sinkp->inputp(i, clonep);
            }
        }
    }

    کينډۍ <ډول_نوم راس_2>
    فراغ moveVertices(Dfg_راس::List<راس_2>& list) {
        لكل (Dfg_راس* ثابت vtxp : list.unlinkable()) {
            Dfg_راس& vtx = *vtxp;
            إذا (ثابت uint64_t component = m_component.at(vtx)) {
                m_dfg.remove_راس(vtx);
                m_components[component - 1]->جمع_کړه_راس(vtx);
            }
        }
    }

    فراغ check_څنډې(Dfg_ګراف& dfg) ثابت {
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

    فراغ check_ګراف(Dfg_ګراف& dfg) ثابت {
        // Build set of vertices
        std::unordered_سېټ<ثابت Dfg_راس*> vertices{dfg.اندازه()};
        dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) { vertices.ورننباسه(&vtx); });

        // Check that each edge connects to a vertex that is within the same graph
        dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            vtx.foreach_سرچینه([&](ثابت Dfg_راس& src) {
                UASSERT_OBJ(vertices.شمېر(&src), &vtx, "Source vertex not in graph");
                أرجع خطأ;
            });
            vtx.foreachSink([&](ثابت Dfg_راس& snk) {
                UASSERT_OBJ(vertices.شمېر(&snk), &snk, "Sink vertex not in graph");
                أرجع خطأ;
            });
        });
    }

    فراغ ایستل_Components(uint32_t nComponents) {
        // Allocate result graphs
        m_components.resize(nComponents);
        لكل (uint32_t i = 0; i < nComponents; ++i) {
            m_components[i].reset(جديد Dfg_ګراف{m_dfg.ctx(), m_مختاړی + cvt_منزل_Str(i)});
        }

        // Fix up edges crossing components (we can only do this at variable boundaries, and the
        // earlier merging of components ensured crossing in fact only happen at variable
        // boundaries). Note that fixing up the edges can create clones of variables. Clones do
        // not need fixing up, so we do not need to iterate them.
        ثابت Dfg_راس* ثابت lastp = m_dfg.varVertices().backp();
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            // Fix up the edges crossing components
            fix_څنډې(vtx);
            // Don't iterate clones added during this loop
            إذا (&vtx == lastp) اكسر;
        }

        // Check results for consistency
        إذا (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_څنډې(m_dfg);
            لكل (ثابت خپلکاری& dfgp : m_components) check_څنډې(*dfgp);
        }

        // Move other vertices to their component graphs
        // After this, vertex states are invalid as we moved the vertices
        moveVertices(m_dfg.varVertices());
        moveVertices(m_dfg.astVertices());
        moveVertices(m_dfg.constVertices());
        moveVertices(m_dfg.opVertices());

        // Check results for consistency
        إذا (VL_UNLIKELY(m_doExpensiveChecks)) {
            check_ګراف(m_dfg);
            لكل (ثابت خپلکاری& dfgp : m_components) check_ګراف(*dfgp);
        }
    }

    // CONSTRUCTOR - entry point
    څرګند ایستل_Cyclic_Components_2(Dfg_ګراف& dfg, ثابت std::string& label)
        : m_dfg{dfg}
        , m_مختاړی{dfg.name() + (label.تشه() ? "" : "-") + label + "-component-"} {
        // Find all the non-trivial SCCs (and trivial cycles) in the graph
        ثابت uint32_t nSCCs = V3DfgPasses::colorStronglyConnectedComponents(dfg, m_component);
        // If the graph was acyclic (which should be the common case), then we are done.
        إذا (!nSCCs) أرجع;
        // Ensure that component boundaries are always at variables, by expanding SCCs
        expandComponents();
        // Extract the components
        ایستل_Components(nSCCs);
    }

عام:
    ساكن std::ویکټور<std::unique_ptr<Dfg_ګراف>> apply(Dfg_ګراف& dfg, ثابت std::string& label) {
        أرجع std::move(ایستل_Cyclic_Components_2{dfg, label}.m_components);
    }
};

std::ویکټور<std::unique_ptr<Dfg_ګراف>>
Dfg_ګراف::ایستل_Cyclic_Components(ثابت std::string& label) {
    أرجع ایستل_Cyclic_Components_2::apply(*هذا, label);
}
