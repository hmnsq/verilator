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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी V3ClassGraphVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    मित्र श्रेणी ClassGraphBuilderVisitor;
    क्रमागत Virtualization : uint8_t {
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
    काष्ठा FuncInfo final {
        AstCFunc* स्थिर m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        bool m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* स्थिर cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{false} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    स्थिर AstClass* स्थिर m_classp;  // A class of which the function is a member of

खुला:
    explicit V3ClassGraphVertex(V3Graph* स्थिर graphp, स्थिर AstClass* स्थिर classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = शेष;

    std::string name() स्थिर override { वापस m_classp->name(); }

    व्योम addFunc(AstCFunc* स्थिर cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    व्योम propagateVirt(स्थिर V3ClassGraphVertex& from) {
        क्रम (स्वतः funcs : from.m_funcsVirtualization) {
            अगर (funcs.second.m_virtualization == NON_VIRTUAL) जारी;
            स्वतः& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            अगर (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    स्थिर std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(स्थिर std::string& name) & {
        स्वतः it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        अगर (!info.m_cacheInitialized) {
            अगर (info.m_cfuncp) info.m_cache.जोड़ो(info.m_cfuncp);
            अगर (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                क्रम (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* स्थिर other = edge.शीर्ष()->as<V3ClassGraphVertex>();
                    स्थिर std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.जोड़ो(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = true;
        }
        वापस info.m_cache;
    }
};

स्थिर std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(स्थिर Ast_गाँठ_CCall* स्थिर callp) स्थिर& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    अगर (callp->superReference()) वापस m_emptySet;
    स्थिर स्वतः it = m_memberFuncToClassVertex.find(callp->funcp());
    अगर (it == m_memberFuncToClassVertex.end()) {
        वापस m_emptySet;  // Not under class return empty set
    }
    वापस it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
श्रेणी ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<स्थिर AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(स्थिर AstClass* स्थिर classp) {
        स्वतः it = m_vertexMap.find(classp);
        अगर (it != m_vertexMap.end()) वापस it->second;
        वापस m_vertexMap.emplace(classp, नया V3ClassGraphVertex{m_graphp.लो(), classp})
            .first->second;
    }

    व्योम visit(AstCFunc* स्थिर nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        अगर (!m_classp || nodep->isConstructor()) वापस;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    व्योम visit(AstClassExtends* स्थिर nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        नया V3GraphEdge{m_graphp.लो(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    व्योम visit(Ast_गाँठ_Module* स्थिर nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        अगर (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    व्योम visit(Ast_गाँठ* स्थिर nodep) override { iterateChildrenConst(nodep); }

खुला:
    explicit ClassGraphBuilderVisitor(AstNetlist* स्थिर nodep)
        : m_graphp{नया V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        क्रम (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* स्थिर vertexp = graphVertex.as<V3ClassGraphVertex>();
            क्रम (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* स्थिर successorp = outEdge.शीर्ष()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = शेष;
    std::unique_ptr<V3ClassGraph> takeGraph() && { वापस std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    वापस ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
