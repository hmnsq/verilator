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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    ਮਿੱਤਰ ਵਰਗ Class_ਗ੍ਰਾਫ_Builder_Visitor;
    ਗਿਣਤੀ Virtualization : uint8_t {
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
    ਬਣਤਰ FuncInfo final {
        AstCFunc* ਅਚਲ m_cfuncp;  // pointer to a described function
        std::unordered_ਸੈੱਟ<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        ਤਾਰਕਿਕ m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ਅਚਲ cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{ਝੂਠ} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ਅਚਲ AstClass* ਅਚਲ m_classp;  // A class of which the function is a member of

ਜਨਤਕ:
    ਸਪਸ਼ਟ V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* ਅਚਲ graphp, ਅਚਲ AstClass* ਅਚਲ classp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_classp{classp} {}
    ~V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    std::string name() ਅਚਲ override { ਵਾਪਸ m_classp->name(); }

    ਖਾਲੀ ਜੋੜੋ_Func(AstCFunc* ਅਚਲ cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VIRTUAL
                                                              : V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::NON_VIRTUAL));
    }
    ਖਾਲੀ propagateVirt(ਅਚਲ V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& from) {
        ਲਈ (ਸਵੈਚਾਲਿਤ funcs : from.m_funcsVirtualization) {
            ਜੇ (funcs.ਦੂਜਾ.m_virtualization == NON_VIRTUAL) ਜਾਰੀ;
            ਸਵੈਚਾਲਿਤ& ਸ਼ੀਰਸ਼
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.ਪਹਿਲਾ),
                          std::forward_as_tuple(ਖਾਲੀ_ਸੂਚਕ, V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VIRTUAL_INHERITED))
                      .ਪਹਿਲਾ->ਦੂਜਾ;
            ਜੇ (ਸ਼ੀਰਸ਼.m_virtualization == NON_VIRTUAL) ਸ਼ੀਰਸ਼.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*>& getCallPossibleCFuncs(ਅਚਲ std::string& name) & {
        ਸਵੈਚਾਲਿਤ it = m_funcsVirtualization.ਲੱਭੋ(name);
        UASSERT_OBJ(it != m_funcsVirtualization.ਸਿਰਾ(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->ਦੂਜਾ;
        ਜੇ (!info.m_cacheInitialized) {
            ਜੇ (info.m_cfuncp) info.m_cache.ਪਾਓ(info.m_cfuncp);
            ਜੇ (info.m_virtualization != V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::NON_VIRTUAL) {
                ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ other = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼>();
                    ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.ਪਾਓ(otherCalls.begin(), otherCalls.ਸਿਰਾ());
                }
            }
            info.m_cacheInitialized = ਸੱਚ;
        }
        ਵਾਪਸ info.m_cache;
    }
};

ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*>&
V3_Class_ਗ੍ਰਾਫ::getCallPossibleCFuncs(ਅਚਲ Ast_ਗੰਢ_CCall* ਅਚਲ callp) ਅਚਲ& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    ਜੇ (callp->superReference()) ਵਾਪਸ m_ਖਾਲੀ_ਥਾਵਾਂ_ਸੈੱਟ;
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_member_Func_ਮੰਜ਼ਿਲ_Class_ਸ਼ੀਰਸ਼.ਲੱਭੋ(callp->funcp());
    ਜੇ (it == m_member_Func_ਮੰਜ਼ਿਲ_Class_ਸ਼ੀਰਸ਼.ਸਿਰਾ()) {
        ਵਾਪਸ m_ਖਾਲੀ_ਥਾਵਾਂ_ਸੈੱਟ;  // Not under class return empty set
    }
    ਵਾਪਸ it->ਦੂਜਾ->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
ਵਰਗ Class_ਗ੍ਰਾਫ_Builder_Visitor final : VNVisitorConst {
    std::unique_ptr<V3_Class_ਗ੍ਰਾਫ> m_graphp;  // Graph of classes
    std::unordered_map<ਅਚਲ AstClass*, V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>
        m_ਸ਼ੀਰਸ਼_Map;  // Maps classes to their vertices
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Current class
    V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* m_classVertexp = ਖਾਲੀ_ਸੂਚਕ;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* get_Class_ਸ਼ੀਰਸ਼(ਅਚਲ AstClass* ਅਚਲ classp) {
        ਸਵੈਚਾਲਿਤ it = m_ਸ਼ੀਰਸ਼_Map.ਲੱਭੋ(classp);
        ਜੇ (it != m_ਸ਼ੀਰਸ਼_Map.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
        ਵਾਪਸ m_ਸ਼ੀਰਸ਼_Map.emplace(classp, ਨਵਾਂ V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{m_graphp.get(), classp})
            .ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* ਅਚਲ nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        ਜੇ (!m_classp || nodep->isConstructor()) ਵਾਪਸ;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_member_Func_ਮੰਜ਼ਿਲ_Class_ਸ਼ੀਰਸ਼.emplace(nodep, m_classVertexp);
        m_classVertexp->ਜੋੜੋ_Func(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassExtends* ਅਚਲ nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{m_graphp.get(),
                        get_Class_ਸ਼ੀਰਸ਼(VN_AS(nodep->ਬੱਚਾ_DTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* ਅਚਲ nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        ਜੇ (m_classp) m_classVertexp = get_Class_ਸ਼ੀਰਸ਼(m_classp);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* ਅਚਲ nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ Class_ਗ੍ਰਾਫ_Builder_Visitor(AstNetlist* ਅਚਲ nodep)
        : m_graphp{ਨਵਾਂ V3_Class_ਗ੍ਰਾਫ} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->ਕ੍ਰਮ();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ : m_graphp->vertices()) {
            V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼.as<V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼>();
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਬਾਹਰਲਾ_ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ successorp = ਬਾਹਰਲਾ_ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~Class_ਗ੍ਰਾਫ_Builder_Visitor() override = ਮੂਲ;
    std::unique_ptr<V3_Class_ਗ੍ਰਾਫ> take_ਗ੍ਰਾਫ() && { ਵਾਪਸ std::move(m_graphp); }
};

std::unique_ptr<V3_Class_ਗ੍ਰਾਫ> V3_Class_ਗ੍ਰਾਫ::build(AstNetlist* netlistp) {
    ਵਾਪਸ Class_ਗ੍ਰਾਫ_Builder_Visitor{netlistp}.take_ਗ੍ਰਾਫ();
}
