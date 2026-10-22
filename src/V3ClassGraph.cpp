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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف V3ClassGraphVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend صنف ClassGraphBuilderVisitor;
    تعداد Virtualization : uint8_t {
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
    بنية FuncInfo final {
        AstCFunc* ثابت m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        منطقي m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ثابت cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{خطأ} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ثابت AstClass* ثابت m_classp;  // A class of which the function is a member of

عام:
    explicit V3ClassGraphVertex(V3Graph* ثابت graphp, ثابت AstClass* ثابت classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = افتراضي;

    std::string name() ثابت override { أرجع m_classp->name(); }

    فراغ addFunc(AstCFunc* ثابت cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    فراغ propagateVirt(ثابت V3ClassGraphVertex& from) {
        لكل (auto funcs : from.m_funcsVirtualization) {
            إذا (funcs.second.m_virtualization == NON_VIRTUAL) استمر;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            إذا (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ثابت std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(ثابت std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        إذا (!info.m_cacheInitialized) {
            إذا (info.m_cfuncp) info.m_cache.سك(info.m_cfuncp);
            إذا (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                لكل (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* ثابت other = edge.كولي()->as<V3ClassGraphVertex>();
                    ثابت std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.سك(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = true;
        }
        أرجع info.m_cache;
    }
};

ثابت std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(ثابت Ast_كلي_CCall* ثابت callp) ثابت& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    إذا (callp->superReference()) أرجع m_emptySet;
    ثابت auto it = m_memberFuncToClassVertex.find(callp->funcp());
    إذا (it == m_memberFuncToClassVertex.end()) {
        أرجع m_emptySet;  // Not under class return empty set
    }
    أرجع it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
صنف ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<ثابت AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(ثابت AstClass* ثابت classp) {
        auto it = m_vertexMap.find(classp);
        إذا (it != m_vertexMap.end()) أرجع it->second;
        أرجع m_vertexMap.emplace(classp, جديد V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    فراغ visit(AstCFunc* ثابت nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        إذا (!m_classp || nodep->isConstructor()) أرجع;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    فراغ visit(AstClassExtends* ثابت nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        جديد V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    فراغ visit(Ast_كلي_Module* ثابت nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        إذا (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    فراغ visit(Ast_كلي* ثابت nodep) override { iterateChildrenConst(nodep); }

عام:
    explicit ClassGraphBuilderVisitor(AstNetlist* ثابت nodep)
        : m_graphp{جديد V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        لكل (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* ثابت vertexp = graphVertex.as<V3ClassGraphVertex>();
            لكل (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* ثابت successorp = outEdge.كولي()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = افتراضي;
    std::unique_ptr<V3ClassGraph> takeGraph() && { أرجع std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    أرجع ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
