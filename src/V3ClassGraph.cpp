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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग V3ClassGraphVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend वर्ग ClassGraphBuilderVisitor;
    गणना Virtualization : uint8_t {
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
    संरचना FuncInfo final {
        AstCFunc* ध्रुव m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        तार्किक m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ध्रुव cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{असत्य} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ध्रुव AstClass* ध्रुव m_classp;  // A class of which the function is a member of

सार्वजनिक:
    explicit V3ClassGraphVertex(V3Graph* ध्रुव graphp, ध्रुव AstClass* ध्रुव classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = स्वभाव;

    std::string name() ध्रुव override { प्रत्यागम m_classp->name(); }

    शून्य addFunc(AstCFunc* ध्रुव cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    शून्य propagateVirt(ध्रुव V3ClassGraphVertex& from) {
        प्रति (auto funcs : from.m_funcsVirtualization) {
            यदि (funcs.second.m_virtualization == NON_VIRTUAL) अनुवर्त;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            यदि (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ध्रुव std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(ध्रुव std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        यदि (!info.m_cacheInitialized) {
            यदि (info.m_cfuncp) info.m_cache.जोड़ो(info.m_cfuncp);
            यदि (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                प्रति (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* ध्रुव other = edge.शीर्ष()->as<V3ClassGraphVertex>();
                    ध्रुव std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.जोड़ो(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = सत्य;
        }
        प्रत्यागम info.m_cache;
    }
};

ध्रुव std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(ध्रुव Ast_गाँठ_CCall* ध्रुव callp) ध्रुव& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    यदि (callp->superReference()) प्रत्यागम m_emptySet;
    ध्रुव auto it = m_memberFuncToClassVertex.find(callp->funcp());
    यदि (it == m_memberFuncToClassVertex.end()) {
        प्रत्यागम m_emptySet;  // Not under class return empty set
    }
    प्रत्यागम it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
वर्ग ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<ध्रुव AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(ध्रुव AstClass* ध्रुव classp) {
        auto it = m_vertexMap.find(classp);
        यदि (it != m_vertexMap.end()) प्रत्यागम it->second;
        प्रत्यागम m_vertexMap.emplace(classp, नूतन V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    शून्य visit(AstCFunc* ध्रुव nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        यदि (!m_classp || nodep->isConstructor()) प्रत्यागम;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    शून्य visit(AstClassExtends* ध्रुव nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        नूतन V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    शून्य visit(Ast_गाँठ_Module* ध्रुव nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        यदि (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठ* ध्रुव nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit ClassGraphBuilderVisitor(AstNetlist* ध्रुव nodep)
        : m_graphp{नूतन V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        प्रति (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* ध्रुव vertexp = graphVertex.as<V3ClassGraphVertex>();
            प्रति (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* ध्रुव successorp = outEdge.शीर्ष()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = स्वभाव;
    std::unique_ptr<V3ClassGraph> takeGraph() && { प्रत्यागम std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    प्रत्यागम ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
