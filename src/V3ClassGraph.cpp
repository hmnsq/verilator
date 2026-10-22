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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

رده V3ClassGraphVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend رده ClassGraphBuilderVisitor;
    شمارش Virtualization : uint8_t {
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
    ساختار FuncInfo final {
        AstCFunc* ثابت m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        منطقی m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ثابت cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{نادرست} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ثابت AstClass* ثابت m_classp;  // A class of which the function is a member of

عمومی:
    explicit V3ClassGraphVertex(V3Graph* ثابت graphp, ثابت AstClass* ثابت classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = پیشفرض;

    std::string name() ثابت override { بازگشت m_classp->name(); }

    تهی addFunc(AstCFunc* ثابت cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    تهی propagateVirt(ثابت V3ClassGraphVertex& from) {
        برای (auto funcs : from.m_funcsVirtualization) {
            اگر (funcs.second.m_virtualization == NON_VIRTUAL) ادامه;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            اگر (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ثابت std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(ثابت std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        اگر (!info.m_cacheInitialized) {
            اگر (info.m_cfuncp) info.m_cache.درج(info.m_cfuncp);
            اگر (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                برای (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* ثابت other = edge.بالاترین()->as<V3ClassGraphVertex>();
                    ثابت std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.درج(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = درست;
        }
        بازگشت info.m_cache;
    }
};

ثابت std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(ثابت Ast_گره_CCall* ثابت callp) ثابت& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    اگر (callp->superReference()) بازگشت m_emptySet;
    ثابت auto it = m_memberFuncToClassVertex.find(callp->funcp());
    اگر (it == m_memberFuncToClassVertex.end()) {
        بازگشت m_emptySet;  // Not under class return empty set
    }
    بازگشت it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
رده ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<ثابت AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(ثابت AstClass* ثابت classp) {
        auto it = m_vertexMap.find(classp);
        اگر (it != m_vertexMap.end()) بازگشت it->second;
        بازگشت m_vertexMap.emplace(classp, جدید V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    تهی visit(AstCFunc* ثابت nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        اگر (!m_classp || nodep->isConstructor()) بازگشت;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    تهی visit(AstClassExtends* ثابت nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        جدید V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    تهی visit(Ast_گره_Module* ثابت nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        اگر (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    تهی visit(Ast_گره* ثابت nodep) override { iterateChildrenConst(nodep); }

عمومی:
    explicit ClassGraphBuilderVisitor(AstNetlist* ثابت nodep)
        : m_graphp{جدید V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        برای (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* ثابت vertexp = graphVertex.as<V3ClassGraphVertex>();
            برای (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* ثابت successorp = outEdge.بالاترین()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = پیشفرض;
    std::unique_ptr<V3ClassGraph> takeGraph() && { بازگشت std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    بازگشت ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
