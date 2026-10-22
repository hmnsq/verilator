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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ V3ClassGraphVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend વર્ગ ClassGraphBuilderVisitor;
    ગણના Virtualization : uint8_t {
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
    સંરચના FuncInfo final {
        AstCFunc* અચલ m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        તાર્કિક m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* અચલ cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{ખોટું} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    અચલ AstClass* અચલ m_classp;  // A class of which the function is a member of

જાહેર:
    explicit V3ClassGraphVertex(V3Graph* અચલ graphp, અચલ AstClass* અચલ classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = મૂળભૂત;

    std::string name() અચલ override { પાછા m_classp->name(); }

    શૂન્ય addFunc(AstCFunc* અચલ cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    શૂન્ય propagateVirt(અચલ V3ClassGraphVertex& from) {
        માટે (auto funcs : from.m_funcsVirtualization) {
            જો (funcs.second.m_virtualization == NON_VIRTUAL) ચાલુ;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            જો (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    અચલ std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(અચલ std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        જો (!info.m_cacheInitialized) {
            જો (info.m_cfuncp) info.m_cache.ઉમેરો(info.m_cfuncp);
            જો (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                માટે (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* અચલ other = edge.ટોચ()->as<V3ClassGraphVertex>();
                    અચલ std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.ઉમેરો(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = સાચું;
        }
        પાછા info.m_cache;
    }
};

અચલ std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(અચલ Ast_ગાંઠ_CCall* અચલ callp) અચલ& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    જો (callp->superReference()) પાછા m_emptySet;
    અચલ auto it = m_memberFuncToClassVertex.find(callp->funcp());
    જો (it == m_memberFuncToClassVertex.end()) {
        પાછા m_emptySet;  // Not under class return empty set
    }
    પાછા it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
વર્ગ ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<અચલ AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(અચલ AstClass* અચલ classp) {
        auto it = m_vertexMap.find(classp);
        જો (it != m_vertexMap.end()) પાછા it->second;
        પાછા m_vertexMap.emplace(classp, નવું V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    શૂન્ય visit(AstCFunc* અચલ nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        જો (!m_classp || nodep->isConstructor()) પાછા;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    શૂન્ય visit(AstClassExtends* અચલ nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        નવું V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* અચલ nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        જો (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* અચલ nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit ClassGraphBuilderVisitor(AstNetlist* અચલ nodep)
        : m_graphp{નવું V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        માટે (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* અચલ vertexp = graphVertex.as<V3ClassGraphVertex>();
            માટે (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* અચલ successorp = outEdge.ટોચ()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = મૂળભૂત;
    std::unique_ptr<V3ClassGraph> takeGraph() && { પાછા std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    પાછા ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
