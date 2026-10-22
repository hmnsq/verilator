// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Waves tracing
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
// V3TraceDecl's Transformations:
//      Create trace init CFunc
//      For each VarScope
//          If appropriate type of signal, create a TraceDecl
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3TraceDecl.h"

#ضمن "V3Ast.h"
#ضمن "V3Control.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Number.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

#ضمن <cstdint>
#ضمن <functional>
#ضمن <limits>
#ضمن <tuple>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

صنف مسیر_Adjustor final {
    File_سطر* ثابت m_flp;  // FileLine used for created nodes
    std::function<فراغ(Ast_گره_Stmt*)> m_emit;  // Function called with adjustment statements
    std::وکتور<std::string> m_پشته{""};  // Stack of current paths

    ساكن constexpr حرف SEPARATOR = ' ';

عام:
    صریح مسیر_Adjustor(File_سطر* flp, std::function<فراغ(Ast_گره_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    فراغ adjust(ثابت string& new_مسیر, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        طالما (!VString::startsWith(new_مسیر, m_پشته.پشت())) {
            m_emit(جديد Ast_Trace_برداشت_پیشوند{m_flp});
            m_پشته.برداشت_پشت();
        }

        إذا (new_مسیر == m_پشته.پشت()) أرجع;

        ثابت VTrace_پیشوند_Type آخرین_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_پیشوند_Type::SCOPE_INTERFACE
                : VTrace_پیشوند_Type::SCOPE_MODULE;
        ثابت std::string extra_پیشوند = new_مسیر.substr(m_پشته.پشت().اندازه());
        اندازه_t begin = 0;
        ثابت اندازه_t آخرین = extra_پیشوند.rfind(SEPARATOR);
        // Move down, one path element at a time
        طالما (درست) {
            ثابت اندازه_t انتها = extra_پیشوند.پیدا_کن(SEPARATOR, begin);
            إذا (انتها == string::npos) اكسر;
            ثابت string& extra = extra_پیشوند.substr(begin, انتها - begin);
            إذا (انتها == آخرین) {
                m_emit(جديد Ast_Trace_افزودن_پیشوند{m_flp, extra, آخرین_Scope_Type});
            } وإلا {
                m_emit(جديد Ast_Trace_افزودن_پیشوند{m_flp, extra, VTrace_پیشوند_Type::SCOPE_MODULE});
            }
            m_پشته.افزودن_پشت(m_پشته.پشت() + extra + SEPARATOR);
            begin = انتها + 1;
        }
        UASSERT(begin == extra_پیشوند.اندازه(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    فراغ unwind() {
        طالما (m_پشته.اندازه() > 1) {
            m_emit(جديد Ast_Trace_برداشت_پیشوند{m_flp});
            m_پشته.برداشت_پشت();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

صنف TraceDeclVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_بالاترین_Scope* ثابت m_بالاترین_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = اشاره_تهی;  // Current scope being visited

    std::وکتور<AstCFunc*> m_بالاترین_Funcps;  // Top level trace initialization functions
    std::وکتور<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::مجموعه<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    بنية Dtype_Func_کلید final {
        ثابت Ast_گره_DType* dtypep;
        VVarType varType;
        منطقي عملگر==(ثابت Dtype_Func_کلید& other) ثابت {
            أرجع dtypep == other.dtypep && varType == other.varType;
        }
    };
    بنية Dtype_Func_کلید_Hash final {
        اندازه_t عملگر()(ثابت Dtype_Func_کلید& کلید) ثابت {
            أرجع std::hash<ثابت Ast_گره_DType*>{}(کلید.dtypep)
                   ^ (std::hash<uint8_t>{}(کلید.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_کلید, AstCFunc*, Dtype_Func_کلید_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = اشاره_تهی;  // Current type func
    AstCFunc* m_dtypeSubFuncp = اشاره_تهی;  // Current type sub func
    ثابت string m_dtype_آرگومانها{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = اشاره_تهی;  // Current type func decl
    V3_Unique_نامها m_dtype_نامها{""};  // Unique names for dtype funcs
    منطقي m_skipDtypeFunc = خطأ;  // Don't create a type func
    uint32_t m_جابه_جایی = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحيح m_بالاترین_Func_اندازه = 0;  // Size of the top function currently being built
    صحيح m_sub_Func_اندازه = 0;  // Size of the sub function currently being built
    منطقي m_force_New_برگ = خطأ;  // Force next addToSubFunc to start a fresh leaf
    اندازه_t m_بالاترین_Scope_ریشه_Func_شمار = 0;  // Top-scope init functions used only for wrapper IOs
    منطقي m_بالاترین_Scope_ریشه_Phase = خطأ;  // Emitting top-scope wrapper IO declarations
    ثابت صحيح m_func_اندازه_حد  // Maximum size of a function
        = v3Global.opt.output_تقسیم_CTrace() ? v3Global.opt.output_تقسیم_CTrace()
                                           : std::numeric_limits<صحيح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::وکتور<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_مسیر_مقصد_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::وکتور<std::tuple<AstScope*, AstCell*, Ast_گره_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::وکتور<std::tuple<AstVarScope*, Ast_گره_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    صنف Trace_مدخل final {
        AstVarScope* m_vscp = اشاره_تهی;  // AstVarScope under scope being traced
        AstCell* m_cellp = اشاره_تهی;  // Sub scope (as AstCell) under scope being traced
        std::string m_مسیر;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        فراغ init(ثابت std::string& name, Ast_گره* nodep, منطقي درون_بالاترین_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_گره::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_مسیر = vcdName + " ";
                m_name.clear();
            } وإلا إذا (varp->isPrimaryIO()) {
                // Primary IO variable
                m_مسیر = "$rootio ";
                m_name = vcdName;
            } وإلا {
                // Other Variable
                ثابت اندازه_t pos = vcdName.rfind(' ');
                ثابت اندازه_t مسیر_Len = pos == std::string::npos ? 0 : pos + 1;
                m_مسیر = vcdName.substr(0, مسیر_Len);
                m_name = vcdName.substr(مسیر_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            إذا (درون_بالاترین_Scope && !v3Global.opt.libCreate().خالی()) {
                ثابت اندازه_t شروع = m_مسیر.پیدا_کن(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(شروع != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string پیشوند = m_مسیر.substr(0, شروع);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                إذا (پیشوند == "$rootio") أرجع;
                m_مسیر = m_مسیر.substr(شروع + 1);
                إذا (v3Global.rootp()->trace_Lib_بالاترین_Name() != پیشوند) m_مسیر = "$libroot " + m_مسیر;
            }
        }

    عام:
        صریح Trace_مدخل(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_بالاترین());
        }
        صریح Trace_مدخل(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_بالاترین());
        }
        صحيح operatorCompare(ثابت Trace_مدخل& b) ثابت {
            إذا (ثابت صحيح cmp = مسیر().compare(b.مسیر())) أرجع cmp < 0;
            إذا (ثابت صحيح cmp = fileline().operatorCompare(b.fileline())) أرجع cmp < 0;
            أرجع name() < b.name();
        }
        AstVarScope* vscp() ثابت { أرجع m_vscp; }
        AstCell* cellp() ثابت { أرجع m_cellp; }
        ثابت std::string& مسیر() ثابت { أرجع m_مسیر; }
        فراغ مسیر(ثابت std::string& مسیر) { m_مسیر = مسیر; }
        ثابت std::string& name() ثابت { أرجع m_name; }
        File_سطر& fileline() ثابت { أرجع m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::وکتور<Trace_مدخل> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = اشاره_تهی;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_گره_عبارت* m_traValuep = اشاره_تهی;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ثابت AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (!varp->isTrace()) أرجع "Verilator trace_off";
        إذا (!nodep->isTrace()) أرجع "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        إذا (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            أرجع "Automatic variable";

        ثابت صحيح عرض = recurse_DType_عرض(nodep->varp()->dtypep());
        إذا (v3Global.opt.trace_Max_عرض() && عرض > v3Global.opt.trace_Max_عرض())
            أرجع "Width " + cvt_مقصد_Str(عرض) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        إذا (!V3Control::getScopeTraceOn(prettyName)) أرجع "Vlt scope trace_off";
        إذا (!v3Global.opt.traceUnderscore()) {
            إذا (!prettyName.خالی() && prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.پیدا_کن("._") != string::npos) أرجع "Inlined leading underscore";
        }
        أرجع ""s;
    }

    صحيح recurse_DType_عرض(ثابت Ast_گره_DType* nodep) {
        إذا (ثابت Ast_گره_Array_DType* adtypep = VN_CAST(nodep, گره_Array_DType))
            أرجع recurse_DType_عرض(adtypep->subDTypep()) * adtypep->declRange().elements();
        أرجع nodep->عرض();
    }

    AstCFunc* newCFunc(File_سطر* flp, ثابت string& name) {
        AstScope* ثابت بالاترین_Scopep = m_بالاترین_Scopep->scopep();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, name, بالاترین_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_پایه() + "* tracep");
        funcp->isTrace(درست);
        funcp->isStatic(خطأ);
        funcp->isLoose(درست);
        funcp->slow(درست);
        بالاترین_Scopep->جمع_کن_Blocksp(funcp);
        أرجع funcp;
    }

    فراغ جمع_کن_مقصد_بالاترین_Func(Ast_گره_Stmt* stmtp) {
        إذا (m_بالاترین_Func_اندازه > m_func_اندازه_حد || m_بالاترین_Funcps.خالی()) {
            m_بالاترین_Func_اندازه = 0;
            //
            ثابت string n = cvt_مقصد_Str(m_بالاترین_Funcps.اندازه());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_بالاترین_Scopep->fileline(), name);
            m_بالاترین_Funcps.افزودن_پشت(funcp);
        }
        m_بالاترین_Funcps.پشت()->جمع_کن_Stmtsp(stmtp);
        m_بالاترین_Func_اندازه += stmtp->گره_شمار();
    }

    فراغ جمع_کن_مقصد_Sub_Func(Ast_گره_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        إذا (m_dtypeSubFuncp) {
            إذا (m_sub_Func_اندازه > m_func_اندازه_حد) newDeclSubFunc();
            m_dtypeSubFuncp->جمع_کن_Stmtsp(stmtp);
            m_sub_Func_اندازه += stmtp->گره_شمار();
            أرجع;
        }
        // Defer trace splitting until V3Trace
        إذا (m_subFuncps.خالی() || m_force_New_برگ) {
            m_force_New_برگ = خطأ;
            File_سطر* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvt_مقصد_Str(m_subFuncps.اندازه());
            ثابت string name
                = m_currScopep == m_بالاترین_Scopep->scopep() && !v3Global.opt.libCreate().خالی()
                      ? (m_بالاترین_Scope_ریشه_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.افزودن_پشت(funcp);
        }
        m_subFuncps.پشت()->جمع_کن_Stmtsp(stmtp);
        m_sub_Func_اندازه += stmtp->گره_شمار();
    }

    AstTraceDecl* جمع_کن_Trace_Decl(ثابت VNumRange& arrayRange,
                               صحيح عرض_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = اشاره_تهی) {
        VNumRange بیت_Range;
        إذا (عرض_Override) {
            بیت_Range = VNumRange{عرض_Override - 1, 0};
        } وإلا إذا (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            بیت_Range = bdtypep->nrange();
        }
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گره_عبارت* valuep = m_traValuep->clone_درخت(خطأ);
        ثابت منطقي valid_جابه_جایی = m_جابه_جایی != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = جديد AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               بیت_Range, arrayRange, dtypeCallp,        valid_جابه_جایی};
        إذا (valid_جابه_جایی) {
            newp->کد(m_جابه_جایی);
            إذا (!dtypeCallp) { m_جابه_جایی += newp->کد_Inc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(جديد AstCExpr{flp, "__VdtypeVar", m_traVscp->عرض()});
                VL_DO_DANGLING(refp->delete_درخت(), refp);
            });
        } وإلا {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        جمع_کن_مقصد_Sub_Func(newp);

        أرجع newp;
    }

    فراغ جمع_کن_Ignore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        إذا (debug() > 3 && m_traVscp) std::خروجی << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    فراغ fixupPlaceholder(ثابت std::string& مسیر, Ast_گره_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت خودکار it = m_مسیر_مقصد_Scopep.پیدا_کن(Ast_گره::prettyName(مسیر));
        إذا (it != m_مسیر_مقصد_Scopep.انتها()) {
            ثابت AstScope* ثابت scopep = it->دومی;
            File_سطر* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_گره* stmtp = اشاره_تهی;
            لكل (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = جديد AstCCall{flp, subFuncp};
                callp->dtype_مجموعه_Void();
                callp->argTypes("tracep");
                stmtp = Ast_گره::جمع_کن_بعدی(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            إذا (stmtp) placeholderp->جمع_کن_بعدی_اینجا(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_پشت();
        VL_DO_DANGLING(placeholderp->delete_درخت(), placeholderp);
    }

    فراغ fixupLibStub(ثابت std::string& مسیر, Ast_گره_Stmt* placeholderp) {
        File_سطر* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_گره::prettyName(مسیر);
        إذا (!v3Global.opt.libCreate().خالی()) {
            ثابت std::string بالاترین_پیشوند = v3Global.rootp()->trace_Lib_بالاترین_Name() + ".";
            إذا (VString::startsWith(name, بالاترین_پیشوند)) name = name.substr(بالاترین_پیشوند.اندازه());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = جديد AstCStmt{flp};
        initp->جمع_کن("{\n");
        initp->جمع_کن("std::string __VlibName = vlSymsp->name();\n");
        initp->جمع_کن("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->جمع_کن("__VlibName += ");
        initp->جمع_کن(جديد AstConst{flp, AstConst::String{}, name});
        initp->جمع_کن(";\n");
        initp->جمع_کن("tracep->initLib(__VlibName);\n");
        initp->جمع_کن("}\n");

        placeholderp->جمع_کن_بعدی_اینجا(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_پشت()->delete_درخت(), placeholderp);
    }

    فراغ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        لكل (ثابت خودکار& قلم_2 : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(قلم_2);
            ثابت AstCell* ثابت cellp = std::get<1>(قلم_2);
            Ast_گره_Stmt* ثابت placeholderp = std::get<2>(قلم_2);
            ثابت std::string مسیر = parentp->name() + "__DOT__" + cellp->name();
            إذا (cellp->modp()->verilatorLib()) {
                fixupLibStub(مسیر, placeholderp);
            } وإلا {
                fixupPlaceholder(مسیر, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        لكل (ثابت خودکار& قلم_2 : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(قلم_2);
            Ast_گره_Stmt* ثابت placeholderp = std::get<1>(قلم_2);
            ثابت std::string مسیر = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(مسیر, placeholderp);
        }
    }

    فراغ remove_Redundant_پیشوند_افزودن_برداشت() {
        لكل (ثابت خودکار& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            لكل (AstCFunc* ثابت funcp : pair.دومی) {
                Ast_گره* prevp = اشاره_تهی;
                Ast_گره* currp = funcp->stmtsp();
                طالما (currp) {
                    Ast_گره* ثابت nextp = currp->nextp();
                    إذا (VN_IS(prevp, Trace_افزودن_پیشوند) && VN_IS(currp, Trace_برداشت_پیشوند)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_پشت()->delete_درخت(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_پشت()->delete_درخت(), currp);
                    }
                    إذا (!nextp) اكسر;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    فراغ checkCalls(ثابت AstCFunc* funcp) {
        إذا (!v3Global.opt.debugCheck()) أرجع;
        checkCallsRecurse(funcp);
        إذا (!m_declUncalledps.خالی()) {  // LCOV_EXCL_START
            لكل (خودکار tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    فراغ checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([هذا](ثابت Ast_گره* nodep) {
            إذا (ثابت AstTraceDecl* ثابت declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } وإلا إذا (ثابت AstCCall* ثابت ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    منطقي isBasicIO() { أرجع m_traVscp->varp()->isVLIO(); }
    فراغ newDeclSubFunc() {
        File_سطر* ثابت flp = m_dtypeFuncp->fileline();
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtype_نامها.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_آرگومانها);
        AstCCall* ثابت subCallp = جديد AstCCall{flp, subFuncp};
        subCallp->dtype_مجموعه_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->جمع_کن_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_اندازه = 0;
    }
    فراغ newDeclFunc(Ast_گره_DType* nodep) {
        Ast_گره_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_آرگومانها{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_کلید dtype_کلید{skipTypep, m_traVscp->varp()->varType()};
        خودکار pair = m_dtypeFuncs.emplace(dtype_کلید, اشاره_تهی);
        AstCFunc** funcpp = &pair.اولی->دومی;
        إذا (pair.دومی) {
            *funcpp = newCFunc(flp, m_dtype_نامها.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_آرگومانها);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = جديد AstCCall{flp, *funcpp};
        callp->dtype_مجموعه_Void();
        callp->argTypes(call_آرگومانها);
        m_dtypeDeclp = جمع_کن_Trace_Decl(VNumRange{}, skipTypep->عرض(), callp);
        جمع_کن_مقصد_Sub_Func(callp->makeStmt());

        إذا (pair.دومی) {
            VL_RESTORER(m_جابه_جایی);
            m_جابه_جایی = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_اندازه);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            إذا (AstStructDType* ثابت dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, درست);
            } وإلا إذا (AstUnpackArrayDType* ثابت dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, درست);
            } وإلا إذا (AstPackArrayDType* ثابت dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, درست);
            } وإلا {
                UASSERT_OBJ(خطأ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_جابه_جایی);
        }

        m_dtypeDeclp->کد_Inc((*funcpp)->user1());
        m_جابه_جایی += m_dtypeDeclp->کد_Inc();
    }
    فراغ declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقي newFunc) {
        string پیشوند_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_افزودن_پیشوند{flp, پیشوند_Name, VTrace_پیشوند_Type::ARRAY_UNPACKED,
                                            nodep->چپ(), nodep->راست(), !newFunc});

        إذا (VN_IS(nodep->subDTypep()->skip_Ref_مقصد_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_مقصد_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            إذا (m_traVscp->dtypep()->skip_Ref_مقصد_Enump()->isString()) {
                جمع_کن_Ignore("Unsupported: strings");
            } وإلا {
                m_traName = "";
                جمع_کن_Trace_Decl(nodep->declRange(), 0);
            }
        } وإلا {
            Ast_گره_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_مقصد_Enump();
            // Always iterate left index to right index
            لكل (ثابت صحيح i : nodep->declRange().seq_چپ_مقصد_راست()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::مقصد_string(i) + ']';
                m_traValuep = m_traValuep->clone_درخت(خطأ);
                m_traValuep = جديد AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
        }

        جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_برداشت_پیشوند{flp});
    }
    فراغ declPackedArray(AstPackArrayDType* ثابت nodep, منطقي newFunc) {
        string پیشوند_Name(newFunc ? "name" : m_traName);
        Ast_گره_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_مقصد_Enump();

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_افزودن_پیشوند{flp, پیشوند_Name, VTrace_پیشوند_Type::ARRAY_PACKED,
                                            nodep->چپ(), nodep->راست(), !newFunc});

        // Always iterate left index to right index
        لكل (ثابت صحيح i : nodep->declRange().seq_چپ_مقصد_راست()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::مقصد_string(i) + ']';
            ثابت صحيح lsb = (i - nodep->lo()) * subtypep->عرض();
            m_traValuep = m_traValuep->clone_درخت(خطأ);
            m_traValuep = جديد AstSel{flp, m_traValuep, lsb, subtypep->عرض()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_درخت(), m_traValuep = اشاره_تهی);
        }

        جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_برداشت_پیشوند{flp});
    }
    فراغ declStruct(AstStructDType* ثابت nodep, منطقي newFunc) {
        File_سطر* ثابت flp = nodep->fileline();
        string پیشوند_Name(newFunc ? "name" : m_traName);
        صحيح nMembers = 0;
        لكل (Ast_گره* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        إذا (!nodep->packed()) {
            جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_افزودن_پیشوند{flp, پیشوند_Name,  //
                                                VTrace_پیشوند_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_مقصد_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(خطأ);
                m_traValuep = جديد AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_برداشت_پیشوند{flp});
        } وإلا {
            جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_افزودن_پیشوند{flp, پیشوند_Name,  //
                                                VTrace_پیشوند_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_مقصد_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(خطأ);
                m_traValuep = جديد AstSel{flp, m_traValuep, itemp->lsb(), subtypep->عرض()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_برداشت_پیشوند{flp});
        }
    }

    // VISITORS
    فراغ بازدید(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.خالی(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالی(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.خالی(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        إذا (nodep->modp()->verilatorLib()) أرجع;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        لكل (Ast_گره* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_پشت(nodep, cellp);
        }

        إذا (!m_entries.خالی()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_مرتب_سازی(
                m_entries.begin(), m_entries.انتها(),
                [](ثابت Trace_مدخل& a, ثابت Trace_مدخل& b) { أرجع a.operatorCompare(b); });

            File_سطر* ثابت flp = nodep->fileline();
            مسیر_Adjustor مسیر_Adjustor_2{flp, [&](Ast_گره_Stmt* stmtp) { جمع_کن_مقصد_Sub_Func(stmtp); }};
            ثابت منطقي تقسیم_ریشه_Primary_Ios = nodep->is_بالاترین() && !v3Global.opt.libCreate().خالی();
            ثابت خودکار emit_مدخل = [&](ثابت Trace_مدخل& مدخل) {
                AstVarScope* ثابت vscp = مدخل.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << مدخل.مسیر() << "' name='" << مدخل.name() << "' "
                                  << (مدخل.cellp() ? static_cast<Ast_گره*>(مدخل.cellp())
                                                    : static_cast<Ast_گره*>(مدخل.vscp())));
                مسیر_Adjustor_2.adjust(مدخل.مسیر(), مدخل.cellp(), مدخل.vscp());

                m_traName = مدخل.name();

                إذا (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    إذا (!ignoreReason.خالی()) {
                        جمع_کن_Ignore(ignoreReason);
                    } وإلا {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = جديد AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_مقصد_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        إذا (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = اشاره_تهی;
                        }
                    }
                } وإلا {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = مدخل.cellp();
                    Ast_گره_Stmt* ثابت stmtp = جديد AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    جمع_کن_مقصد_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_پشت(nodep, cellp, stmtp);
                }
            };
            إذا (تقسیم_ریشه_Primary_Ios) {
                m_بالاترین_Scope_ریشه_Phase = درست;
                لكل (ثابت Trace_مدخل& مدخل : m_entries) {
                    AstVarScope* ثابت vscp = مدخل.vscp();
                    إذا (!(vscp && vscp->varp()->isPrimaryIO())) استمر;
                    emit_مدخل(مدخل);
                }
                m_بالاترین_Scope_ریشه_Phase = خطأ;
                مسیر_Adjustor_2.unwind();
                m_بالاترین_Scope_ریشه_Func_شمار = m_subFuncps.اندازه();
                إذا (m_بالاترین_Scope_ریشه_Func_شمار) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_اندازه = 0;
                    m_force_New_برگ = درست;
                }
            }
            لكل (ثابت Trace_مدخل& مدخل : m_entries) {
                AstVarScope* ثابت vscp = مدخل.vscp();
                إذا (تقسیم_ریشه_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) استمر;
                emit_مدخل(مدخل);
            }
            مسیر_Adjustor_2.unwind();
            m_traVscp = اشاره_تهی;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string مسیر = nodep->prettyName();
        m_مسیر_مقصد_Scopep.emplace(مسیر, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        إذا (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت اندازه_t آخرین_Dot = مسیر.پیدا_کن_آخرین_از('.');
            UASSERT_OBJ(آخرین_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << مسیر);
            ثابت std::string والد_مسیر = مسیر.substr(0, آخرین_Dot + 1);

            لكل (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                إذا (!VString::startsWith(refName, والد_مسیر)) استمر;

                // Save the mapping from the path of the reference to the scope
                m_مسیر_مقصد_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                إذا (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_پشت()->delete_درخت(), intfRefp);
                }
            }
        }
    }
    فراغ بازدید(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        إذا (nodep->varp()->is_موقت() && !nodep->varp()->isTrace()) أرجع;
        إذا (nodep->varp()->isClassMember()) أرجع;
        إذا (nodep->varp()->isFuncLocal()) أرجع;

        // When creating a --lib-create library ...
        إذا (!v3Global.opt.libCreate().خالی()) {
            // Ignore parameters in packages. These will be traced at the top level.
            إذا (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) أرجع;
        }

        // Add to traced signal list
        m_entries.emplace_پشت(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    فراغ بازدید(AstConstDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_جابه_جایی);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = درست;
        iterate(nodep->subDTypep()->skip_Ref_مقصد_Enump());
    }
    فراغ بازدید(AstRefDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_جابه_جایی);
        iterate(nodep->subDTypep()->skip_Ref_مقصد_Enump());
    }
    فراغ بازدید(AstIfaceRefDType* /*nodep*/) override {
        إذا (!m_traVscp) أرجع;
        // Insert a placeholder to be fixed up later
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گره_Stmt* ثابت stmtp
            = جديد AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        جمع_کن_مقصد_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_پشت(m_traVscp, stmtp);
    }
    فراغ بازدید(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;

        إذا (v3Global.opt.traceMaxArray()
            && static_cast<صحيح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            جمع_کن_Ignore("Wide memory > --trace-max-array ents");
            أرجع;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        إذا (VN_IS(nodep->subDTypep()->skip_Ref_مقصد_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_مقصد_Enump()->isString()) {
            جمع_کن_Ignore("Unsupported: strings");
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = درست;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_جابه_جایی);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, خطأ);
    }
    فراغ بازدید(AstPackArrayDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جمع_کن_Trace_Decl(VNumRange{}, nodep->عرض());
            أرجع;
        }

        Ast_گره_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_مقصد_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        إذا (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            إذا (basicp->is_بیت_Logic() && !basicp->isRanged()) {
                جمع_کن_Trace_Decl(VNumRange{}, nodep->عرض());
                أرجع;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = درست;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_جابه_جایی);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, خطأ);
    }
    فراغ بازدید(AstStructDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جمع_کن_Trace_Decl(VNumRange{}, nodep->عرض());
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = درست;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_جابه_جایی);
            newDeclFunc(nodep);
        }
        declStruct(nodep, خطأ);
    }
    فراغ بازدید(AstUnionDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = درست;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جمع_کن_Trace_Decl(VNumRange{}, nodep->عرض());
            أرجع;
        }

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        صحيح nMembers = 0;
        لكل (Ast_گره* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        إذا (!nodep->packed()) {
            جمع_کن_Ignore("Unsupported: Unpacked union");
        } وإلا {
            جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_افزودن_پیشوند{flp, m_traName,  //
                                                VTrace_پیشوند_Type::UNION_PACKED, nMembers});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گره_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_مقصد_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            جمع_کن_مقصد_Sub_Func(جديد Ast_Trace_برداشت_پیشوند{flp});
        }
    }
    فراغ بازدید(AstBasicDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        إذا (nodep->isString()) {
            جمع_کن_Ignore("Unsupported: strings");
        } وإلا {
            جمع_کن_Trace_Decl(VNumRange{}, 0);
        }
    }
    فراغ بازدید(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    فراغ بازدید(Ast_گره_DType*) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;
        جمع_کن_Ignore("Unsupported: data type");
    }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح TraceDeclVisitor(AstNetlist* nodep)
        : m_بالاترین_Scopep{nodep->بالاترین_Scopep()} {
        File_سطر* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_بعدی_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.خالی(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_پیشوند_افزودن_برداشت();

        ثابت std::وکتور<AstCFunc*>& بالاترین_Scope_Funcps = m_scopeInitFuncps.at(m_بالاترین_Scopep->scopep());
        AstCFunc* ریشه_Funcp = اشاره_تهی;
        إذا (!v3Global.opt.libCreate().خالی()) {
            ریشه_Funcp = newCFunc(flp, "trace_init_root");
            ریشه_Funcp->مدخل_نقطه(درست);
            لكل (اندازه_t i = 0; i < m_بالاترین_Scope_ریشه_Func_شمار; ++i) {
                AstCCall* ثابت callp = جديد AstCCall{flp, بالاترین_Scope_Funcps.at(i)};
                callp->dtype_مجموعه_Void();
                callp->argTypes("tracep");
                ریشه_Funcp->جمع_کن_Stmtsp(callp->makeStmt());
            }
            إذا (!m_بالاترین_Scope_ریشه_Func_شمار) ریشه_Funcp->جمع_کن_Stmtsp(جديد AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        لكل (اندازه_t i = m_بالاترین_Scope_ریشه_Func_شمار; i < بالاترین_Scope_Funcps.اندازه(); ++i) {
            AstCFunc* ثابت funcp = بالاترین_Scope_Funcps.at(i);
            AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
            callp->dtype_مجموعه_Void();
            callp->argTypes("tracep");
            جمع_کن_مقصد_بالاترین_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        إذا (m_بالاترین_Funcps.خالی()) جمع_کن_مقصد_بالاترین_Func(جديد AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        إذا (m_بالاترین_Funcps.اندازه() > 1) {
            AstCFunc* ثابت بالاترین_Funcp = newCFunc(flp, "");
            لكل (AstCFunc* funcp : m_بالاترین_Funcps) {
                AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
                callp->dtype_مجموعه_Void();
                callp->argTypes("tracep");
                بالاترین_Funcp->جمع_کن_Stmtsp(callp->makeStmt());
            }
            m_بالاترین_Funcps.clear();
            m_بالاترین_Funcps.افزودن_پشت(بالاترین_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت بالاترین_Funcp = m_بالاترین_Funcps.front();
        بالاترین_Funcp->name("trace_init_top");
        بالاترین_Funcp->مدخل_نقطه(درست);

        إذا (ریشه_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ریشه_Funcp);
        checkCalls(بالاترین_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::جمع_کن_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::جمع_کن_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("tracedecl", 0, dump_درخت_Either_Level() >= 3);
}
