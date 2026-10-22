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

صنف V3_Class_گراف_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(V3_Class_گراف_رأس, V3_گراف_رأس)
    دوست صنف Class_گراف_Builder_Visitor;
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
        std::unordered_مجموعه<AstCFunc*>
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
    صریح V3_Class_گراف_رأس(V3_گراف* ثابت graphp, ثابت AstClass* ثابت classp)
        : V3_گراف_رأس{graphp}
        , m_classp{classp} {}
    ~V3_Class_گراف_رأس() override = افتراضي;

    std::string name() ثابت override { أرجع m_classp->name(); }

    فراغ جمع_کن_Func(AstCFunc* ثابت cfuncp) {
        m_funcsVirtualization.emplace(
            std::piecewise_construct, std::forward_as_tuple(cfuncp->name()),
            std::forward_as_tuple(cfuncp, cfuncp->isVirtual() ? V3_Class_گراف_رأس::VIRTUAL
                                                              : V3_Class_گراف_رأس::NON_VIRTUAL));
    }
    فراغ propagateVirt(ثابت V3_Class_گراف_رأس& from) {
        لكل (خودکار funcs : from.m_funcsVirtualization) {
            إذا (funcs.دومی.m_virtualization == NON_VIRTUAL) استمر;
            خودکار& رأس
                = m_funcsVirtualization
                      .emplace(
                          std::piecewise_construct, std::forward_as_tuple(funcs.اولی),
                          std::forward_as_tuple(اشاره_تهی, V3_Class_گراف_رأس::VIRTUAL_INHERITED))
                      .اولی->دومی;
            إذا (رأس.m_virtualization == NON_VIRTUAL) رأس.m_virtualization = VIRTUAL_IMPLICIT;
        }
    }
    ثابت std::unordered_مجموعه<AstCFunc*>& getCallPossibleCFuncs(ثابت std::string& name) & {
        خودکار it = m_funcsVirtualization.پیدا_کن(name);
        UASSERT_OBJ(it != m_funcsVirtualization.انتها(), m_classp,
                    "Unexpected call - CFunc with name '" << name << "' not found");
        FuncInfo& info = it->دومی;
        إذا (!info.m_cacheInitialized) {
            إذا (info.m_cfuncp) info.m_cache.درج(info.m_cfuncp);
            إذا (info.m_virtualization != V3_Class_گراف_رأس::NON_VIRTUAL) {
                لكل (V3_گراف_یال& یال : بیرون_یالها()) {
                    V3_Class_گراف_رأس* ثابت other = یال.بالاترین()->as<V3_Class_گراف_رأس>();
                    ثابت std::unordered_مجموعه<AstCFunc*> otherCalls
                        = other->getCallPossibleCFuncs(name);
                    info.m_cache.درج(otherCalls.begin(), otherCalls.انتها());
                }
            }
            info.m_cacheInitialized = درست;
        }
        أرجع info.m_cache;
    }
};

ثابت std::unordered_مجموعه<AstCFunc*>&
V3_Class_گراف::getCallPossibleCFuncs(ثابت Ast_گره_CCall* ثابت callp) ثابت& {
    // super references shall have AstNodeCCall::funcp() pointing to the only possible
    // AstCFunc that may be called
    إذا (callp->superReference()) أرجع m_خالی_مجموعه;
    ثابت خودکار it = m_member_Func_مقصد_Class_رأس.پیدا_کن(callp->funcp());
    إذا (it == m_member_Func_مقصد_Class_رأس.انتها()) {
        أرجع m_خالی_مجموعه;  // Not under class return empty set
    }
    أرجع it->دومی->getCallPossibleCFuncs(callp->funcp()->name());
}

// Build a Class graph and returns it in a V3ClassGraph
صنف Class_گراف_Builder_Visitor final : VNVisitorConst {
    std::unique_ptr<V3_Class_گراف> m_graphp;  // Graph of classes
    std::unordered_map<ثابت AstClass*, V3_Class_گراف_رأس*>
        m_رأس_Map;  // Maps classes to their vertices
    AstClass* m_classp = اشاره_تهی;  // Current class
    V3_Class_گراف_رأس* m_classVertexp = اشاره_تهی;  // vertex corresponding to a current class -
                                                   // getClassVertex(m_classp) == m_classVertexp

    V3_Class_گراف_رأس* get_Class_رأس(ثابت AstClass* ثابت classp) {
        خودکار it = m_رأس_Map.پیدا_کن(classp);
        إذا (it != m_رأس_Map.انتها()) أرجع it->دومی;
        أرجع m_رأس_Map.emplace(classp, جديد V3_Class_گراف_رأس{m_graphp.get(), classp})
            .اولی->دومی;
    }

    فراغ بازدید(AstCFunc* ثابت nodep) override {
        // Constructors have explicitly added super call (added by V3LinkDot if not added by user),
        // therefore assume that no virtualization happens - only explicit calls
        إذا (!m_classp || nodep->isConstructor()) أرجع;
        UASSERT_OBJ(m_classVertexp, nodep,
                    "If function is under class m_classVertexp shall be set");
        m_graphp->m_member_Func_مقصد_Class_رأس.emplace(nodep, m_classVertexp);
        m_classVertexp->جمع_کن_Func(nodep);
    }
    فراغ بازدید(AstClassExtends* ثابت nodep) override {
        UASSERT_OBJ(m_classVertexp, nodep,
                    "m_classVertexp shall be set while visiting AstClassExtends");
        جديد V3_گراف_یال{m_graphp.get(),
                        get_Class_رأس(VN_AS(nodep->فرزند_DTypep(), ClassRefDType)->classp()),
                        m_classVertexp, 1};
    }
    فراغ بازدید(Ast_گره_Module* ثابت nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_classVertexp);
        m_classp = VN_CAST(nodep, Class);
        إذا (m_classp) m_classVertexp = get_Class_رأس(m_classp);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره* ثابت nodep) override { iterateChildrenConst(nodep); }

عام:
    صریح Class_گراف_Builder_Visitor(AstNetlist* ثابت nodep)
        : m_graphp{جديد V3_Class_گراف} {
        iterateConst(nodep);

        // Propagate virtualization
        m_graphp->ترتیب();
        لكل (V3_گراف_رأس& گراف_رأس : m_graphp->vertices()) {
            V3_Class_گراف_رأس* ثابت vertexp = گراف_رأس.as<V3_Class_گراف_رأس>();
            لكل (V3_گراف_یال& بیرون_یال : vertexp->بیرون_یالها()) {
                V3_Class_گراف_رأس* ثابت successorp = بیرون_یال.بالاترین()->as<V3_Class_گراف_رأس>();
                successorp->propagateVirt(*vertexp);
            }
        }
    }
    ~Class_گراف_Builder_Visitor() override = افتراضي;
    std::unique_ptr<V3_Class_گراف> take_گراف() && { أرجع std::move(m_graphp); }
};

std::unique_ptr<V3_Class_گراف> V3_Class_گراف::build(AstNetlist* netlistp) {
    أرجع Class_گراف_Builder_Visitor{netlistp}.take_گراف();
}
