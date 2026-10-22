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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3ClassGraph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ V3_Class_ನಕ್ಷೆ_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(V3_Class_ನಕ್ಷೆ_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    ಮಿತ್ರ ವರ್ಗ Class_ನಕ್ಷೆ_Builder_Visitor;
    ಎಣಿಕೆ Virtualization : uint8_t {
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
    ರಚನೆ FuncInfo final {
        AstCFunc* ಸ್ಥಿರಾಂಕ m_cfuncp;  // pointer to a described function
        std::unordered_ಗಣ<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        ತಾರ್ಕಿಕ m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{ಸುಳ್ಳು} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ m_classp;  // A class of which the function is a member of

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ V3_Class_ನಕ್ಷೆ_ಶೃಂಗ(V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ graphp, ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_classp{classp} {}
    ~V3_Class_ನಕ್ಷೆ_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    std::string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_classp->name(); }

    ಶೂನ್ಯ ಕೂಡಿಸು_Func(AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3_Class_ನಕ್ಷೆ_ಶೃಂಗ::VIRTUAL
                                                              : V3_Class_ನಕ್ಷೆ_ಶೃಂಗ::NON_VIRTUAL));
    }
    ಶೂನ್ಯ propagateVirt(ಸ್ಥಿರಾಂಕ V3_Class_ನಕ್ಷೆ_ಶೃಂಗ& from) {
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ funcs : from.m_funcsVirtualization) {
            ಆದರೆ (funcs.ಎರಡನೇ.m_virtualization == NON_VIRTUAL) ಮುಂದುವರಿ;
            ಸ್ವಯಂಚಾಲಿತ& ಶೃಂಗ
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.ಮೊದಲ),
                          std::forward_as_tuple(ಶೂನ್ಯಸೂಚಕ, V3_Class_ನಕ್ಷೆ_ಶೃಂಗ::VIRTUAL_INHERITED))
                      .ಮೊದಲ->ಎರಡನೇ;
            ಆದರೆ (ಶೃಂಗ.m_virtualization == NON_VIRTUAL) ಶೃಂಗ.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*>& getCallPossibleCFuncs(ಸ್ಥಿರಾಂಕ std::string& name) & {
        ಸ್ವಯಂಚಾಲಿತ it = m_funcsVirtualization.ಕಂಡುಹಿಡಿ(name);
        UASSERT_OBJ(it != m_funcsVirtualization.ಕೊನೆ(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->ಎರಡನೇ;
        ಆದರೆ (!info.m_cacheInitialized) {
            ಆದರೆ (info.m_cfuncp) info.m_cache.ಸೇರಿಸು(info.m_cfuncp);
            ಆದರೆ (info.m_virtualization != V3_Class_ನಕ್ಷೆ_ಶೃಂಗ::NON_VIRTUAL) {
                ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    V3_Class_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ other = ಅಂಚು.ತುದಿ()->as<V3_Class_ನಕ್ಷೆ_ಶೃಂಗ>();
                    ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.ಸೇರಿಸು(otherCalls.begin(), otherCalls.ಕೊನೆ());
                }
            }
            info.m_cacheInitialized = ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ info.m_cache;
    }
};

ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*>&
V3_Class_ನಕ್ಷೆ::getCallPossibleCFuncs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ callp) ಸ್ಥಿರಾಂಕ& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    ಆದರೆ (callp->superReference()) ಹಿಂತಿರುಗಿ m_ಖಾಲಿಗಳು_ಗಣ;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_member_Func_ಗಮ್ಯ_Class_ಶೃಂಗ.ಕಂಡುಹಿಡಿ(callp->funcp());
    ಆದರೆ (it == m_member_Func_ಗಮ್ಯ_Class_ಶೃಂಗ.ಕೊನೆ()) {
        ಹಿಂತಿರುಗಿ m_ಖಾಲಿಗಳು_ಗಣ;  // Not under class return empty set
    }
    ಹಿಂತಿರುಗಿ it->ಎರಡನೇ->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
ವರ್ಗ Class_ನಕ್ಷೆ_Builder_Visitor final : VNVisitorConst {
    std::unique_ptr<V3_Class_ನಕ್ಷೆ> m_graphp;  // Graph of classes
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstClass*, V3_Class_ನಕ್ಷೆ_ಶೃಂಗ*>
        m_ಶೃಂಗ_Map;  // Maps classes to their vertices
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Current class
    V3_Class_ನಕ್ಷೆ_ಶೃಂಗ* m_classVertexp = ಶೂನ್ಯಸೂಚಕ;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3_Class_ನಕ್ಷೆ_ಶೃಂಗ* get_Class_ಶೃಂಗ(ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ವಯಂಚಾಲಿತ it = m_ಶೃಂಗ_Map.ಕಂಡುಹಿಡಿ(classp);
        ಆದರೆ (it != m_ಶೃಂಗ_Map.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
        ಹಿಂತಿರುಗಿ m_ಶೃಂಗ_Map.emplace(classp, ಹೊಸ V3_Class_ನಕ್ಷೆ_ಶೃಂಗ{m_graphp.get(), classp})
            .ಮೊದಲ->ಎರಡನೇ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* ಸ್ಥಿರಾಂಕ nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        ಆದರೆ (!m_classp || nodep->isConstructor()) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_member_Func_ಗಮ್ಯ_Class_ಶೃಂಗ.emplace(nodep, m_classVertexp);
        m_classVertexp->ಕೂಡಿಸು_Func(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassExtends* ಸ್ಥಿರಾಂಕ nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{m_graphp.get(),
                        get_Class_ಶೃಂಗ(VN_AS(nodep->ಮಗು_DTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        ಆದರೆ (m_classp) m_classVertexp = get_Class_ಶೃಂಗ(m_classp);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Class_ನಕ್ಷೆ_Builder_Visitor(AstNetlist* ಸ್ಥಿರಾಂಕ nodep)
        : m_graphp{ಹೊಸ V3_Class_ನಕ್ಷೆ} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->ಕ್ರಮ();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& ನಕ್ಷೆ_ಶೃಂಗ : m_graphp->vertices()) {
            V3_Class_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = ನಕ್ಷೆ_ಶೃಂಗ.as<V3_Class_ನಕ್ಷೆ_ಶೃಂಗ>();
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಹೊರಹೋಗು_ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                V3_Class_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ successorp = ಹೊರಹೋಗು_ಅಂಚು.ತುದಿ()->as<V3_Class_ನಕ್ಷೆ_ಶೃಂಗ>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~Class_ನಕ್ಷೆ_Builder_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    std::unique_ptr<V3_Class_ನಕ್ಷೆ> take_ನಕ್ಷೆ() && { ಹಿಂತಿರುಗಿ std::move(m_graphp); }
};

std::unique_ptr<V3_Class_ನಕ್ಷೆ> V3_Class_ನಕ್ಷೆ::build(AstNetlist* netlistp) {
    ಹಿಂತಿರುಗಿ Class_ನಕ್ಷೆ_Builder_Visitor{netlistp}.take_ನಕ್ಷೆ();
}
