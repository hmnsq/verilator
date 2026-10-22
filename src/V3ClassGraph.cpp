// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Virtual function calls - function finder
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
// Builds a graph of inheritance (implements & extends)
// Allows for lookups of what AstCFunc may be called
// due to a virtual function call
//
// Current limitation: This only may be called after V3Task
// when AstNodeFTasks are substituted with AstCFuncs
//
//*************************************************************************
// TODO:
//  - Probably in future this phase shall have a check for overriding
//    virtual functions with static functions - currently
//    there is no such check - it shall happen before V3Task
//    since it moves static functions outside classes
//  - This shall check for error that is currently emitted in V3LinkDot
//    `Class 'Derived' implements 'Base' but is missing implementation
//     for 'function' (IEEE 1800-2023 8.26)`
//    Sometimes this error is emitted and it is a false positive
//    since the following snipped is correct but Verilator emits an error:
//
//      class Base;
//        virtual task foo();
//        endtask
//      endclass
//
//      interface class Iface;
//        pure virtual task foo();
//      endclass
//
//      class Derived extends Base implements Iface;
//        // This is legal as it is but Verilator fails
//      endclass
//
//    Mentioned `IEEE 1800-2023 8.26` says:
//    `When an interface class is implemented by a class, the required
//     implementations of interface class methods may be
//     provided by inherited virtual method implementations.`
//    issue: #6908
//
//  Therefore, this shall probably become a full phase
//  and check for those errors somewhere before V3Task
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ V3ClassGraphVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend ವರ್ಗ ClassGraphBuilderVisitor;
    ಗಣನಾ Virtualization : uint8_t {
        NON_VIRTUAL = 0,  // Not a virtual member function
        VIRTUAL_IMPLICIT,  // virtual keyword is not next to a function definition the ancestor
                           // class has such virtual function therefore, it is treated as an
                           // virtual
        VIRTUAL,  // function explicitly defined as virtual with a keyword
        VIRTUAL_INHERITED,  // Function not declared in a current class however, it is inherited
                            // and it is virtual
        // Note: non-virtual inherited function may not be described with this enum - no need for
        // now as well as static functions
    };
    ಸಂರಚನಾ FuncInfo final {
        AstCFunc* ಧ್ರುವ m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        ತಾರ್ಕಿಕ m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ಧ್ರುವ cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{ಅಸತ್ಯ} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ಧ್ರುವ AstClass* ಧ್ರುವ m_classp;  // A class of which the function is a member of

ಸಾರ್ವಜನಿಕ:
    explicit V3ClassGraphVertex(V3Graph* ಧ್ರುವ graphp, ಧ್ರುವ AstClass* ಧ್ರುವ classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = ಸ್ವಭಾವ;

    std::string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_classp->name(); }

    ಶೂನ್ಯ addFunc(AstCFunc* ಧ್ರುವ cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    ಶೂನ್ಯ propagateVirt(ಧ್ರುವ V3ClassGraphVertex& from) {
        ಪ್ರತಿ (auto funcs : from.m_funcsVirtualization) {
            ಯದಿ (funcs.second.m_virtualization == NON_VIRTUAL) ಅನುವರ್ತ;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            ಯದಿ (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ಧ್ರುವ std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(ಧ್ರುವ std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        ಯದಿ (!info.m_cacheInitialized) {
            ಯದಿ (info.m_cfuncp) info.m_cache.ಸೇರಿಸು(info.m_cfuncp);
            ಯದಿ (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                ಪ್ರತಿ (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* ಧ್ರುವ other = edge.ತುದಿ()->as<V3ClassGraphVertex>();
                    ಧ್ರುವ std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.ಸೇರಿಸು(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ info.m_cache;
    }
};

ಧ್ರುವ std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(ಧ್ರುವ Ast_ಗಂಟು_CCall* ಧ್ರುವ callp) ಧ್ರುವ& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    ಯದಿ (callp->superReference()) ಪ್ರತ್ಯಾಗಮ m_emptySet;
    ಧ್ರುವ auto it = m_memberFuncToClassVertex.find(callp->funcp());
    ಯದಿ (it == m_memberFuncToClassVertex.end()) {
        ಪ್ರತ್ಯಾಗಮ m_emptySet;  // Not under class return empty set
    }
    ಪ್ರತ್ಯಾಗಮ it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
ವರ್ಗ ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<ಧ್ರುವ AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(ಧ್ರುವ AstClass* ಧ್ರುವ classp) {
        auto it = m_vertexMap.find(classp);
        ಯದಿ (it != m_vertexMap.end()) ಪ್ರತ್ಯಾಗಮ it->second;
        ಪ್ರತ್ಯಾಗಮ m_vertexMap.emplace(classp, ನೂತನ V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    ಶೂನ್ಯ visit(AstCFunc* ಧ್ರುವ nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        ಯದಿ (!m_classp || nodep->isConstructor()) ಪ್ರತ್ಯಾಗಮ;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    ಶೂನ್ಯ visit(AstClassExtends* ಧ್ರುವ nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        ನೂತನ V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* ಧ್ರುವ nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        ಯದಿ (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* ಧ್ರುವ nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit ClassGraphBuilderVisitor(AstNetlist* ಧ್ರುವ nodep)
        : m_graphp{ನೂತನ V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        ಪ್ರತಿ (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* ಧ್ರುವ vertexp = graphVertex.as<V3ClassGraphVertex>();
            ಪ್ರತಿ (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* ಧ್ರುವ successorp = outEdge.ತುದಿ()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = ಸ್ವಭಾವ;
    std::unique_ptr<V3ClassGraph> takeGraph() && { ಪ್ರತ್ಯಾಗಮ std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    ಪ್ರತ್ಯಾಗಮ ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
