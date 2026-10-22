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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு V3ClassGraphVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(V3ClassGraphVertex, V3GraphVertex)
    friend வகுப்பு ClassGraphBuilderVisitor;
    எண்ணிக்கை Virtualization : uint8_t {
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
    கட்டமைப்பு FuncInfo final {
        AstCFunc* மாறிலி m_cfuncp;  // pointer to a described function
        std::unordered_set<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        தருக்க m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* மாறிலி cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{பொய்} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    மாறிலி AstClass* மாறிலி m_classp;  // A class of which the function is a member of

பொது:
    explicit V3ClassGraphVertex(V3Graph* மாறிலி graphp, மாறிலி AstClass* மாறிலி classp)
        : V3GraphVertex{graphp}
        , m_classp{classp} {}
    ~V3ClassGraphVertex() override = இயல்பு;

    std::string name() மாறிலி override { திரும்பு m_classp->name(); }

    வெறுமை addFunc(AstCFunc* மாறிலி cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3ClassGraphVertex::VIRTUAL
                                                              : V3ClassGraphVertex::NON_VIRTUAL));
    }
    வெறுமை propagateVirt(மாறிலி V3ClassGraphVertex& from) {
        க்கு (auto funcs : from.m_funcsVirtualization) {
            எனில் (funcs.second.m_virtualization == NON_VIRTUAL) தொடர்;
            auto& vertex
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.first),
                          std::forward_as_tuple(nullptr, V3ClassGraphVertex::VIRTUAL_INHERITED))
                      .first->second;
            எனில் (vertex.m_virtualization == NON_VIRTUAL) vertex.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    மாறிலி std::unordered_set<AstCFunc*>& getCallPossibleCFuncs(மாறிலி std::string& name) & {
        auto it = m_funcsVirtualization.find(name);
        UASSERT_OBJ(it != m_funcsVirtualization.end(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->second;
        எனில் (!info.m_cacheInitialized) {
            எனில் (info.m_cfuncp) info.m_cache.செருகு(info.m_cfuncp);
            எனில் (info.m_virtualization != V3ClassGraphVertex::NON_VIRTUAL) {
                க்கு (V3GraphEdge& edge : outEdges()) {
                    V3ClassGraphVertex* மாறிலி other = edge.உச்சி()->as<V3ClassGraphVertex>();
                    மாறிலி std::unordered_set<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.செருகு(otherCalls.begin(), otherCalls.end());
                }
            }
            info.m_cacheInitialized = உண்மை;
        }
        திரும்பு info.m_cache;
    }
};

மாறிலி std::unordered_set<AstCFunc*>&
V3ClassGraph::getCallPossibleCFuncs(மாறிலி Ast_முனை_CCall* மாறிலி callp) மாறிலி& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    எனில் (callp->superReference()) திரும்பு m_emptySet;
    மாறிலி auto it = m_memberFuncToClassVertex.find(callp->funcp());
    எனில் (it == m_memberFuncToClassVertex.end()) {
        திரும்பு m_emptySet;  // Not under class return empty set
    }
    திரும்பு it->second->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
வகுப்பு ClassGraphBuilderVisitor final : VNVisitorConst {
    std::unique_ptr<V3ClassGraph> m_graphp;  // Graph of classes
    std::unordered_map<மாறிலி AstClass*, V3ClassGraphVertex*>
        m_vertexMap;  // Maps classes to their vertices
    AstClass* m_classp = nullptr;  // Current class
    V3ClassGraphVertex* m_classVertexp = nullptr;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3ClassGraphVertex* getClassVertex(மாறிலி AstClass* மாறிலி classp) {
        auto it = m_vertexMap.find(classp);
        எனில் (it != m_vertexMap.end()) திரும்பு it->second;
        திரும்பு m_vertexMap.emplace(classp, புதிய V3ClassGraphVertex{m_graphp.get(), classp})
            .first->second;
    }

    வெறுமை visit(AstCFunc* மாறிலி nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        எனில் (!m_classp || nodep->isConstructor()) திரும்பு;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_memberFuncToClassVertex.emplace(nodep, m_classVertexp);
        m_classVertexp->addFunc(nodep);
    }
    வெறுமை visit(AstClassExtends* மாறிலி nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        புதிய V3GraphEdge{m_graphp.get(),
                        getClassVertex(VN_AS(nodep->childDTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    வெறுமை visit(Ast_முனை_Module* மாறிலி nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        எனில் (m_classp) m_classVertexp = getClassVertex(m_classp);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_முனை* மாறிலி nodep) override { iterateChildrenConst(nodep); }

பொது:
    explicit ClassGraphBuilderVisitor(AstNetlist* மாறிலி nodep)
        : m_graphp{புதிய V3ClassGraph} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->order();
        க்கு (V3GraphVertex& graphVertex : m_graphp->vertices()) {
            V3ClassGraphVertex* மாறிலி vertexp = graphVertex.as<V3ClassGraphVertex>();
            க்கு (V3GraphEdge& outEdge : vertexp->outEdges()) {
                V3ClassGraphVertex* மாறிலி successorp = outEdge.உச்சி()->as<V3ClassGraphVertex>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~ClassGraphBuilderVisitor() override = இயல்பு;
    std::unique_ptr<V3ClassGraph> takeGraph() && { திரும்பு std::move(m_graphp); }
};

std::unique_ptr<V3ClassGraph> V3ClassGraph::build(AstNetlist* netlistp) {
    திரும்பு ClassGraphBuilderVisitor{netlistp}.takeGraph();
}
