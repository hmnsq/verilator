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

جماعت V3_Class_گراف_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(V3_Class_گراف_راس, V3_گراف_راس)
    دوست جماعت Class_گراف_Builder_Visitor;
    شمار Virtualization : uint8_t {
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
    ساخت FuncInfo final {
        AstCFunc* ثابت m_cfuncp;  // pointer to a described function
        std::unordered_سیٹ<AstCFunc*>
            m_cache;  // set of functions that may be called due to a virtual call
        Virtualization m_virtualization;  // Whether function is virtual (see Virtualization)
        منطقی m_cacheInitialized;  // true when m_cache stores a valid value
        FuncInfo(AstCFunc* ثابت cfuncp, Virtualization virtualization)
            : m_cfuncp{cfuncp}
            , m_virtualization{virtualization}
            , m_cacheInitialized{جھوٹ} {}
    };
    std::unordered_map<std::string, FuncInfo>
        m_funcsVirtualization;  // Map from function names to their description
    ثابت AstClass* ثابت m_classp;  // A class of which the function is a member of

عوامی:
    صریح V3_Class_گراف_راس(V3_گراف* ثابت graphp, ثابت AstClass* ثابت classp)
        : V3_گراف_راس{graphp}
        , m_classp{classp} {}
    ~V3_Class_گراف_راس() override = طےشدہ;

    std::string name() ثابت override { واپس m_classp->name(); }

    خالی جوڑو_Func(AstCFunc* ثابت cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3_Class_گراف_راس::VIRTUAL
                                                              : V3_Class_گراف_راس::NON_VIRTUAL));
    }
    خالی propagateVirt(ثابت V3_Class_گراف_راس& from) {
        برائے (خودکار funcs : from.m_funcsVirtualization) {
            اگر (funcs.دوسرا.m_virtualization == NON_VIRTUAL) جاری;
            خودکار& راس
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.پہلا),
                          std::forward_as_tuple(خالی_اشاریہ, V3_Class_گراف_راس::VIRTUAL_INHERITED))
                      .پہلا->دوسرا;
            اگر (راس.m_virtualization == NON_VIRTUAL) راس.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ثابت std::unordered_سیٹ<AstCFunc*>& getCallPossibleCFuncs(ثابت std::string& name) & {
        خودکار it = m_funcsVirtualization.ڈھونڈو(name);
        UASSERT_OBJ(it != m_funcsVirtualization.انتہا(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->دوسرا;
        اگر (!info.m_cacheInitialized) {
            اگر (info.m_cfuncp) info.m_cache.داخل_کرو(info.m_cfuncp);
            اگر (info.m_virtualization != V3_Class_گراف_راس::NON_VIRTUAL) {
                برائے (V3_گراف_کنارہ& کنارہ : برآمد_کنارے()) {
                    V3_Class_گراف_راس* ثابت other = کنارہ.چوٹی()->as<V3_Class_گراف_راس>();
                    ثابت std::unordered_سیٹ<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.داخل_کرو(otherCalls.begin(), otherCalls.انتہا());
                }
            }
            info.m_cacheInitialized = سچ;
        }
        واپس info.m_cache;
    }
};

ثابت std::unordered_سیٹ<AstCFunc*>&
V3_Class_گراف::getCallPossibleCFuncs(ثابت Ast_گرہ_CCall* ثابت callp) ثابت& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    اگر (callp->superReference()) واپس m_خالی_خانے_سیٹ;
    ثابت خودکار it = m_member_Func_مقصد_Class_راس.ڈھونڈو(callp->funcp());
    اگر (it == m_member_Func_مقصد_Class_راس.انتہا()) {
        واپس m_خالی_خانے_سیٹ;  // Not under class return empty set
    }
    واپس it->دوسرا->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
جماعت Class_گراف_Builder_Visitor final : VNVisitorConst {
    std::unique_ptr<V3_Class_گراف> m_graphp;  // Graph of classes
    std::unordered_map<ثابت AstClass*, V3_Class_گراف_راس*>
        m_راس_Map;  // Maps classes to their vertices
    AstClass* m_classp = خالی_اشاریہ;  // Current class
    V3_Class_گراف_راس* m_classVertexp = خالی_اشاریہ;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3_Class_گراف_راس* get_Class_راس(ثابت AstClass* ثابت classp) {
        خودکار it = m_راس_Map.ڈھونڈو(classp);
        اگر (it != m_راس_Map.انتہا()) واپس it->دوسرا;
        واپس m_راس_Map.emplace(classp, نیا V3_Class_گراف_راس{m_graphp.get(), classp})
            .پہلا->دوسرا;
    }

    خالی دورہ(AstCFunc* ثابت nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        اگر (!m_classp || nodep->isConstructor()) واپس;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_member_Func_مقصد_Class_راس.emplace(nodep, m_classVertexp);
        m_classVertexp->جوڑو_Func(nodep);
    }
    خالی دورہ(AstClassExtends* ثابت nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        نیا V3_گراف_کنارہ{m_graphp.get(),
                        get_Class_راس(VN_AS(nodep->بچہ_DTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    خالی دورہ(Ast_گرہ_Module* ثابت nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        اگر (m_classp) m_classVertexp = get_Class_راس(m_classp);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ* ثابت nodep) override { iterateChildrenConst(nodep); }

عوامی:
    صریح Class_گراف_Builder_Visitor(AstNetlist* ثابت nodep)
        : m_graphp{نیا V3_Class_گراف} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->ترتیب();
        برائے (V3_گراف_راس& گراف_راس : m_graphp->vertices()) {
            V3_Class_گراف_راس* ثابت vertexp = گراف_راس.as<V3_Class_گراف_راس>();
            برائے (V3_گراف_کنارہ& برآمد_کنارہ : vertexp->برآمد_کنارے()) {
                V3_Class_گراف_راس* ثابت successorp = برآمد_کنارہ.چوٹی()->as<V3_Class_گراف_راس>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~Class_گراف_Builder_Visitor() override = طےشدہ;
    std::unique_ptr<V3_Class_گراف> take_گراف() && { واپس std::move(m_graphp); }
};

std::unique_ptr<V3_Class_گراف> V3_Class_گراف::build(AstNetlist* netlistp) {
    واپس Class_گراف_Builder_Visitor{netlistp}.take_گراف();
}
