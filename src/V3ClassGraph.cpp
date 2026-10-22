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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ V3ClassGraphVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend ଶ୍ରେଣୀ ClassGraphBuilderVisitor;
    ଗଣନା Virtualization : uint8_t {
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
    ସଂରଚନା FuncInfo final {
        AstCFunc* ଅଚଳ m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        ତାର୍କିକ m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ଅଚଳ cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{ମିଥ୍ୟା} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ଅଚଳ AstClass* ଅଚଳ m_classp;  // A class of which the function is a member of

ସର୍ବସାଧାରଣ:
    explicit V3ClassGraphVertex(V3Graph* ଅଚଳ graphp, ଅଚଳ AstClass* ଅଚଳ classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    std::string name() ଅଚଳ override { ଫେରନ୍ତୁ m_classp->name(); }

    ଶୂନ୍ୟ addFunc(AstCFunc* ଅଚଳ cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    ଶୂନ୍ୟ propagateVirt(ଅଚଳ V3ClassGraphVertex& from) {
        ପାଇଁ (auto funcs : from.m_funcsVirtualization) {
            ଯଦି (funcs.second.m_virtualization == NON_VIRTUAL) ଜାରି;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            ଯଦି (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ଅଚଳ std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(ଅଚଳ std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        ଯଦି (!info.m_cacheInitialized) {
            ଯଦି (info.m_cfuncp) info.m_cache.ଭର୍ତ୍ତି(info.m_cfuncp);
            ଯଦି (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                ପାଇଁ (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* ଅଚଳ other = edge.ଶୀର୍ଷ()->as<V3ClassGraphVertex>();
                    ଅଚଳ std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.ଭର୍ତ୍ତି(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ info.m_cache;
    }
};

ଅଚଳ std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(ଅଚଳ Ast_ଗଣ୍ଠି_CCall* ଅଚଳ callp) ଅଚଳ& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    ଯଦି (callp->superReference()) ଫେରନ୍ତୁ m_emptySet;
    ଅଚଳ auto it = m_memberFuncToClassVertex.find(callp->funcp());
    ଯଦି (it == m_memberFuncToClassVertex.end()) {
        ଫେରନ୍ତୁ m_emptySet;  // Not under class return empty set
    }
    ଫେରନ୍ତୁ it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
ଶ୍ରେଣୀ ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<ଅଚଳ AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(ଅଚଳ AstClass* ଅଚଳ classp) {
        auto it = m_vertexMap.find(classp);
        ଯଦି (it != m_vertexMap.end()) ଫେରନ୍ତୁ it->second;
        ଫେରନ୍ତୁ m_vertexMap.emplace(classp, ନୂତନ V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    ଶୂନ୍ୟ visit(AstCFunc* ଅଚଳ nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        ଯଦି (!m_classp || nodep->isConstructor()) ଫେରନ୍ତୁ;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    ଶୂନ୍ୟ visit(AstClassExtends* ଅଚଳ nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        ନୂତନ V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* ଅଚଳ nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        ଯଦି (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit ClassGraphBuilderVisitor(AstNetlist* ଅଚଳ nodep)
        : m_graphp{ନୂତନ V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        ପାଇଁ (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* ଅଚଳ vertexp = graphVertex.as<V3ClassGraphVertex>();
            ପାଇଁ (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* ଅଚଳ successorp = outEdge.ଶୀର୍ଷ()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    std::unique_ptr<V3ClassGraph> takeGraph() && { ଫେରନ୍ତୁ std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    ଫେରନ୍ତୁ ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
