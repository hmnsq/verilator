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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ V3ClassGraphVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend বর্গ ClassGraphBuilderVisitor;
    গণনা Virtualization : uint8_t {
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
    সংরচনা FuncInfo final {
        AstCFunc* ধ্রুব m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        তার্কিক m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ধ্রুব cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{অসত্য} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ধ্রুব AstClass* ধ্রুব m_classp;  // A class of which the function is a member of

সার্বজনিক:
    explicit V3ClassGraphVertex(V3Graph* ধ্রুব graphp, ধ্রুব AstClass* ধ্রুব classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = স্বভাব;

    std::string name() ধ্রুব override { প্রত্যাগম m_classp->name(); }

    শূন্য addFunc(AstCFunc* ধ্রুব cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    শূন্য propagateVirt(ধ্রুব V3ClassGraphVertex& from) {
        প্রতি (auto funcs : from.m_funcsVirtualization) {
            যদি (funcs.second.m_virtualization == NON_VIRTUAL) অনুবর্ত;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            যদি (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ধ্রুব std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(ধ্রুব std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        যদি (!info.m_cacheInitialized) {
            যদি (info.m_cfuncp) info.m_cache.ঢোকাও(info.m_cfuncp);
            যদি (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                প্রতি (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* ধ্রুব other = edge.শীর্ষ()->as<V3ClassGraphVertex>();
                    ধ্রুব std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.ঢোকাও(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = সত্য;
        }
        প্রত্যাগম info.m_cache;
    }
};

ধ্রুব std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(ধ্রুব Ast_গ্রন্থি_CCall* ধ্রুব callp) ধ্রুব& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    যদি (callp->superReference()) প্রত্যাগম m_emptySet;
    ধ্রুব auto it = m_memberFuncToClassVertex.find(callp->funcp());
    যদি (it == m_memberFuncToClassVertex.end()) {
        প্রত্যাগম m_emptySet;  // Not under class return empty set
    }
    প্রত্যাগম it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
বর্গ ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<ধ্রুব AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(ধ্রুব AstClass* ধ্রুব classp) {
        auto it = m_vertexMap.find(classp);
        যদি (it != m_vertexMap.end()) প্রত্যাগম it->second;
        প্রত্যাগম m_vertexMap.emplace(classp, নূতন V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    শূন্য visit(AstCFunc* ধ্রুব nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        যদি (!m_classp || nodep->isConstructor()) প্রত্যাগম;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    শূন্য visit(AstClassExtends* ধ্রুব nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        নূতন V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    শূন্য visit(Ast_গ্রন্থি_Module* ধ্রুব nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        যদি (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* ধ্রুব nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    explicit ClassGraphBuilderVisitor(AstNetlist* ধ্রুব nodep)
        : m_graphp{নূতন V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        প্রতি (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* ধ্রুব vertexp = graphVertex.as<V3ClassGraphVertex>();
            প্রতি (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* ধ্রুব successorp = outEdge.শীর্ষ()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = স্বভাব;
    std::unique_ptr<V3ClassGraph> takeGraph() && { প্রত্যাগম std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    প্রত্যাগম ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
