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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి V3_Class_లేఖాచిత్రం_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(V3_Class_లేఖాచిత్రం_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    మిత్ర తరగతి Class_లేఖాచిత్రం_Builder_Visitor;
    గణన Virtualization : uint8_t {
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
    నిర్మాణం FuncInfo final {
        AstCFunc* స్థిరరాశి m_cfuncp;  // pointer to a described function
        std::unordered_సమితి<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        తార్కిక m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* స్థిరరాశి cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{అబద్ధం} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    స్థిరరాశి AstClass* స్థిరరాశి m_classp;  // A class of which the function is a member of

బహిరంగ:
    స్పష్టమైన V3_Class_లేఖాచిత్రం_శీర్షం(V3_లేఖాచిత్రం* స్థిరరాశి graphp, స్థిరరాశి AstClass* స్థిరరాశి classp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_classp{classp} {}
    ~V3_Class_లేఖాచిత్రం_శీర్షం() override = అప్రమేయ;

    std::string name() స్థిరరాశి override { తిరిగి m_classp->name(); }

    శూన్య కూడు_Func(AstCFunc* స్థిరరాశి cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3_Class_లేఖాచిత్రం_శీర్షం::VIRTUAL
                                                              : V3_Class_లేఖాచిత్రం_శీర్షం::NON_VIRTUAL));
    }
    శూన్య propagateVirt(స్థిరరాశి V3_Class_లేఖాచిత్రం_శీర్షం& from) {
        కొరకు (స్వయంచాలక funcs : from.m_funcsVirtualization) {
            ఒకవేళ (funcs.రెండవ.m_virtualization == NON_VIRTUAL) కొనసాగు;
            స్వయంచాలక& శీర్షం
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.మొదటి),
                          std::forward_as_tuple(శూన్యసూచిక, V3_Class_లేఖాచిత్రం_శీర్షం::VIRTUAL_INHERITED))
                      .మొదటి->రెండవ;
            ఒకవేళ (శీర్షం.m_virtualization == NON_VIRTUAL) శీర్షం.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    స్థిరరాశి std::unordered_సమితి<AstCFunc*>& getCallPossibleCFuncs(స్థిరరాశి std::string& name) & {
        స్వయంచాలక it = m_funcsVirtualization.కనుగొను(name);
        UASSERT_OBJ(it != m_funcsVirtualization.చివర(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->రెండవ;
        ఒకవేళ (!info.m_cacheInitialized) {
            ఒకవేళ (info.m_cfuncp) info.m_cache.చొప్పించు(info.m_cfuncp);
            ఒకవేళ (info.m_virtualization != V3_Class_లేఖాచిత్రం_శీర్షం::NON_VIRTUAL) {
                కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : బహిర్గతం_అంచులు()) {
                    V3_Class_లేఖాచిత్రం_శీర్షం* స్థిరరాశి other = అంచు.శిఖరం()->as<V3_Class_లేఖాచిత్రం_శీర్షం>();
                    స్థిరరాశి std::unordered_సమితి<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.చొప్పించు(otherCalls.begin(), otherCalls.చివర());
                }
            }
            info.m_cacheInitialized = నిజం;
        }
        తిరిగి info.m_cache;
    }
};

స్థిరరాశి std::unordered_సమితి<AstCFunc*>&
V3_Class_లేఖాచిత్రం::getCallPossibleCFuncs(స్థిరరాశి Ast_కణుపు_CCall* స్థిరరాశి callp) స్థిరరాశి& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    ఒకవేళ (callp->superReference()) తిరిగి m_ఖాళీలు_సమితి;
    స్థిరరాశి స్వయంచాలక it = m_member_Func_గమ్యం_Class_శీర్షం.కనుగొను(callp->funcp());
    ఒకవేళ (it == m_member_Func_గమ్యం_Class_శీర్షం.చివర()) {
        తిరిగి m_ఖాళీలు_సమితి;  // Not under class return empty set
    }
    తిరిగి it->రెండవ->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
తరగతి Class_లేఖాచిత్రం_Builder_Visitor final : VNVisitorConst {
    std::unique_ptr<V3_Class_లేఖాచిత్రం> m_graphp;  // Graph of classes
    std::unordered_map<స్థిరరాశి AstClass*, V3_Class_లేఖాచిత్రం_శీర్షం*>
        m_శీర్షం_Map;  // Maps classes to their vertices
    AstClass* m_classp = శూన్యసూచిక;  // Current class
    V3_Class_లేఖాచిత్రం_శీర్షం* m_classVertexp = శూన్యసూచిక;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3_Class_లేఖాచిత్రం_శీర్షం* get_Class_శీర్షం(స్థిరరాశి AstClass* స్థిరరాశి classp) {
        స్వయంచాలక it = m_శీర్షం_Map.కనుగొను(classp);
        ఒకవేళ (it != m_శీర్షం_Map.చివర()) తిరిగి it->రెండవ;
        తిరిగి m_శీర్షం_Map.emplace(classp, కొత్త V3_Class_లేఖాచిత్రం_శీర్షం{m_graphp.get(), classp})
            .మొదటి->రెండవ;
    }

    శూన్య సందర్శించు(AstCFunc* స్థిరరాశి nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        ఒకవేళ (!m_classp || nodep->isConstructor()) తిరిగి;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_member_Func_గమ్యం_Class_శీర్షం.emplace(nodep, m_classVertexp);
        m_classVertexp->కూడు_Func(nodep);
    }
    శూన్య సందర్శించు(AstClassExtends* స్థిరరాశి nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        కొత్త V3_లేఖాచిత్రం_అంచు{m_graphp.get(),
                        get_Class_శీర్షం(VN_AS(nodep->బిడ్డ_DTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* స్థిరరాశి nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        ఒకవేళ (m_classp) m_classVertexp = get_Class_శీర్షం(m_classp);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* స్థిరరాశి nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్పష్టమైన Class_లేఖాచిత్రం_Builder_Visitor(AstNetlist* స్థిరరాశి nodep)
        : m_graphp{కొత్త V3_Class_లేఖాచిత్రం} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->క్రమం();
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& లేఖాచిత్రం_శీర్షం : m_graphp->vertices()) {
            V3_Class_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertexp = లేఖాచిత్రం_శీర్షం.as<V3_Class_లేఖాచిత్రం_శీర్షం>();
            కొరకు (V3_లేఖాచిత్రం_అంచు& బహిర్గతం_అంచు : vertexp->బహిర్గతం_అంచులు()) {
                V3_Class_లేఖాచిత్రం_శీర్షం* స్థిరరాశి successorp = బహిర్గతం_అంచు.శిఖరం()->as<V3_Class_లేఖాచిత్రం_శీర్షం>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~Class_లేఖాచిత్రం_Builder_Visitor() override = అప్రమేయ;
    std::unique_ptr<V3_Class_లేఖాచిత్రం> take_లేఖాచిత్రం() && { తిరిగి std::move(m_graphp); }
};

std::unique_ptr<V3_Class_లేఖాచిత్రం> V3_Class_లేఖాచిత్రం::build(AstNetlist* netlistp) {
    తిరిగి Class_లేఖాచిత్రం_Builder_Visitor{netlistp}.take_లేఖాచిత్రం();
}
