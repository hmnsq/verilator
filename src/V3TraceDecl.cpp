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

صنف لار_Adjustor final {
    File_کرښه* ثابت m_flp;  // FileLine used for created nodes
    std::function<فراغ(Ast_غوټه_Stmt*)> m_emit;  // Function called with adjustment statements
    std::ویکټور<std::string> m_ډېری{""};  // Stack of current paths

    ساكن constexpr حرف SEPARATOR = ' ';

عام:
    څرګند لار_Adjustor(File_کرښه* flp, std::function<فراغ(Ast_غوټه_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    فراغ adjust(ثابت string& new_لار, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        طالما (!VString::startsWith(new_لار, m_ډېری.شا())) {
            m_emit(جديد Ast_Trace_راوباسه_مختاړی{m_flp});
            m_ډېری.راوباسه_شا();
        }

        إذا (new_لار == m_ډېری.شا()) أرجع;

        ثابت VTrace_مختاړی_Type وروستی_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_مختاړی_Type::SCOPE_INTERFACE
                : VTrace_مختاړی_Type::SCOPE_MODULE;
        ثابت std::string extra_مختاړی = new_لار.substr(m_ډېری.شا().اندازه());
        اندازه_t begin = 0;
        ثابت اندازه_t وروستی = extra_مختاړی.rfind(SEPARATOR);
        // Move down, one path element at a time
        طالما (ریښتیا) {
            ثابت اندازه_t پای_ټکی = extra_مختاړی.پیدا_کړه(SEPARATOR, begin);
            إذا (پای_ټکی == string::npos) اكسر;
            ثابت string& extra = extra_مختاړی.substr(begin, پای_ټکی - begin);
            إذا (پای_ټکی == وروستی) {
                m_emit(جديد Ast_Trace_ټېل_مختاړی{m_flp, extra, وروستی_Scope_Type});
            } وإلا {
                m_emit(جديد Ast_Trace_ټېل_مختاړی{m_flp, extra, VTrace_مختاړی_Type::SCOPE_MODULE});
            }
            m_ډېری.ټېل_شا(m_ډېری.شا() + extra + SEPARATOR);
            begin = پای_ټکی + 1;
        }
        UASSERT(begin == extra_مختاړی.اندازه(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    فراغ unwind() {
        طالما (m_ډېری.اندازه() > 1) {
            m_emit(جديد Ast_Trace_راوباسه_مختاړی{m_flp});
            m_ډېری.راوباسه_شا();
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
    Ast_سر_Scope* ثابت m_سر_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = تش_اشاره;  // Current scope being visited

    std::ویکټور<AstCFunc*> m_سر_Funcps;  // Top level trace initialization functions
    std::ویکټور<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::سېټ<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    بنية Dtype_Func_کلی final {
        ثابت Ast_غوټه_DType* dtypep;
        VVarType varType;
        منطقي چلوونکی==(ثابت Dtype_Func_کلی& other) ثابت {
            أرجع dtypep == other.dtypep && varType == other.varType;
        }
    };
    بنية Dtype_Func_کلی_Hash final {
        اندازه_t چلوونکی()(ثابت Dtype_Func_کلی& کلی) ثابت {
            أرجع std::hash<ثابت Ast_غوټه_DType*>{}(کلی.dtypep)
                   ^ (std::hash<uint8_t>{}(کلی.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_کلی, AstCFunc*, Dtype_Func_کلی_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = تش_اشاره;  // Current type func
    AstCFunc* m_dtypeSubFuncp = تش_اشاره;  // Current type sub func
    ثابت string m_dtype_دلیلونه{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = تش_اشاره;  // Current type func decl
    V3_Unique_نومونه m_dtype_نومونه{""};  // Unique names for dtype funcs
    منطقي m_skipDtypeFunc = خطأ;  // Don't create a type func
    uint32_t m_دننه_واټن = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحيح m_سر_Func_اندازه = 0;  // Size of the top function currently being built
    صحيح m_sub_Func_اندازه = 0;  // Size of the sub function currently being built
    منطقي m_force_New_ونې_پاڼه = خطأ;  // Force next addToSubFunc to start a fresh leaf
    اندازه_t m_سر_Scope_ریښه_Func_شمېر = 0;  // Top-scope init functions used only for wrapper IOs
    منطقي m_سر_Scope_ریښه_Phase = خطأ;  // Emitting top-scope wrapper IO declarations
    ثابت صحيح m_func_اندازه_حد  // Maximum size of a function
        = v3Global.opt.output_وېشل_CTrace() ? v3Global.opt.output_وېشل_CTrace()
                                           : std::numeric_limits<صحيح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::ویکټور<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_لار_منزل_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::ویکټور<std::tuple<AstScope*, AstCell*, Ast_غوټه_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::ویکټور<std::tuple<AstVarScope*, Ast_غوټه_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    صنف Trace_ثبت final {
        AstVarScope* m_vscp = تش_اشاره;  // AstVarScope under scope being traced
        AstCell* m_cellp = تش_اشاره;  // Sub scope (as AstCell) under scope being traced
        std::string m_لار;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        فراغ init(ثابت std::string& name, Ast_غوټه* nodep, منطقي دننه_سر_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_غوټه::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_لار = vcdName + " ";
                m_name.clear();
            } وإلا إذا (varp->isPrimaryIO()) {
                // Primary IO variable
                m_لار = "$rootio ";
                m_name = vcdName;
            } وإلا {
                // Other Variable
                ثابت اندازه_t pos = vcdName.rfind(' ');
                ثابت اندازه_t لار_Len = pos == std::string::npos ? 0 : pos + 1;
                m_لار = vcdName.substr(0, لار_Len);
                m_name = vcdName.substr(لار_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            إذا (دننه_سر_Scope && !v3Global.opt.libCreate().تشه()) {
                ثابت اندازه_t پیل = m_لار.پیدا_کړه(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(پیل != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string مختاړی = m_لار.substr(0, پیل);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                إذا (مختاړی == "$rootio") أرجع;
                m_لار = m_لار.substr(پیل + 1);
                إذا (v3Global.rootp()->trace_Lib_سر_Name() != مختاړی) m_لار = "$libroot " + m_لار;
            }
        }

    عام:
        څرګند Trace_ثبت(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_سر());
        }
        څرګند Trace_ثبت(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_سر());
        }
        صحيح operatorCompare(ثابت Trace_ثبت& b) ثابت {
            إذا (ثابت صحيح cmp = لار().compare(b.لار())) أرجع cmp < 0;
            إذا (ثابت صحيح cmp = fileline().operatorCompare(b.fileline())) أرجع cmp < 0;
            أرجع name() < b.name();
        }
        AstVarScope* vscp() ثابت { أرجع m_vscp; }
        AstCell* cellp() ثابت { أرجع m_cellp; }
        ثابت std::string& لار() ثابت { أرجع m_لار; }
        فراغ لار(ثابت std::string& لار) { m_لار = لار; }
        ثابت std::string& name() ثابت { أرجع m_name; }
        File_کرښه& fileline() ثابت { أرجع m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::ویکټور<Trace_ثبت> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = تش_اشاره;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_غوټه_بیان* m_traValuep = تش_اشاره;  // Value expression for current signal
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

        ثابت صحيح پلنوالی = recurse_DType_پلنوالی(nodep->varp()->dtypep());
        إذا (v3Global.opt.trace_Max_پلنوالی() && پلنوالی > v3Global.opt.trace_Max_پلنوالی())
            أرجع "Width " + cvt_منزل_Str(پلنوالی) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        إذا (!V3Control::getScopeTraceOn(prettyName)) أرجع "Vlt scope trace_off";
        إذا (!v3Global.opt.traceUnderscore()) {
            إذا (!prettyName.تشه() && prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.پیدا_کړه("._") != string::npos) أرجع "Inlined leading underscore";
        }
        أرجع ""s;
    }

    صحيح recurse_DType_پلنوالی(ثابت Ast_غوټه_DType* nodep) {
        إذا (ثابت Ast_غوټه_Array_DType* adtypep = VN_CAST(nodep, غوټه_Array_DType))
            أرجع recurse_DType_پلنوالی(adtypep->subDTypep()) * adtypep->declRange().elements();
        أرجع nodep->پلنوالی();
    }

    AstCFunc* newCFunc(File_کرښه* flp, ثابت string& name) {
        AstScope* ثابت سر_Scopep = m_سر_Scopep->scopep();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, name, سر_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_بنسټ() + "* tracep");
        funcp->isTrace(ریښتیا);
        funcp->isStatic(خطأ);
        funcp->isLoose(ریښتیا);
        funcp->slow(ریښتیا);
        سر_Scopep->جمع_کړه_Blocksp(funcp);
        أرجع funcp;
    }

    فراغ جمع_کړه_منزل_سر_Func(Ast_غوټه_Stmt* stmtp) {
        إذا (m_سر_Func_اندازه > m_func_اندازه_حد || m_سر_Funcps.تشه()) {
            m_سر_Func_اندازه = 0;
            //
            ثابت string n = cvt_منزل_Str(m_سر_Funcps.اندازه());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_سر_Scopep->fileline(), name);
            m_سر_Funcps.ټېل_شا(funcp);
        }
        m_سر_Funcps.شا()->جمع_کړه_Stmtsp(stmtp);
        m_سر_Func_اندازه += stmtp->غوټه_شمېر();
    }

    فراغ جمع_کړه_منزل_Sub_Func(Ast_غوټه_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        إذا (m_dtypeSubFuncp) {
            إذا (m_sub_Func_اندازه > m_func_اندازه_حد) newDeclSubFunc();
            m_dtypeSubFuncp->جمع_کړه_Stmtsp(stmtp);
            m_sub_Func_اندازه += stmtp->غوټه_شمېر();
            أرجع;
        }
        // Defer trace splitting until V3Trace
        إذا (m_subFuncps.تشه() || m_force_New_ونې_پاڼه) {
            m_force_New_ونې_پاڼه = خطأ;
            File_کرښه* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvt_منزل_Str(m_subFuncps.اندازه());
            ثابت string name
                = m_currScopep == m_سر_Scopep->scopep() && !v3Global.opt.libCreate().تشه()
                      ? (m_سر_Scope_ریښه_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ټېل_شا(funcp);
        }
        m_subFuncps.شا()->جمع_کړه_Stmtsp(stmtp);
        m_sub_Func_اندازه += stmtp->غوټه_شمېر();
    }

    AstTraceDecl* جمع_کړه_Trace_Decl(ثابت VNumRange& arrayRange,
                               صحيح پلنوالی_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = تش_اشاره) {
        VNumRange بټ_Range;
        إذا (پلنوالی_Override) {
            بټ_Range = VNumRange{پلنوالی_Override - 1, 0};
        } وإلا إذا (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            بټ_Range = bdtypep->nrange();
        }
        File_کرښه* ثابت flp = m_traVscp->fileline();
        Ast_غوټه_بیان* valuep = m_traValuep->clone_ونه(خطأ);
        ثابت منطقي valid_دننه_واټن = m_دننه_واټن != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = جديد AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               بټ_Range, arrayRange, dtypeCallp,        valid_دننه_واټن};
        إذا (valid_دننه_واټن) {
            newp->کوډ(m_دننه_واټن);
            إذا (!dtypeCallp) { m_دننه_واټن += newp->کوډ_Inc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(جديد AstCExpr{flp, "__VdtypeVar", m_traVscp->پلنوالی()});
                VL_DO_DANGLING(refp->delete_ونه(), refp);
            });
        } وإلا {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        جمع_کړه_منزل_Sub_Func(newp);

        أرجع newp;
    }

    فراغ جمع_کړه_Ignore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        إذا (debug() > 3 && m_traVscp) std::وتنه << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    فراغ fixupPlaceholder(ثابت std::string& لار, Ast_غوټه_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت خپلکاری it = m_لار_منزل_Scopep.پیدا_کړه(Ast_غوټه::prettyName(لار));
        إذا (it != m_لار_منزل_Scopep.پای_ټکی()) {
            ثابت AstScope* ثابت scopep = it->دوهم;
            File_کرښه* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_غوټه* stmtp = تش_اشاره;
            لكل (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = جديد AstCCall{flp, subFuncp};
                callp->dtype_سېټ_Void();
                callp->argTypes("tracep");
                stmtp = Ast_غوټه::جمع_کړه_بل(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            إذا (stmtp) placeholderp->جمع_کړه_بل_دلته(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_شا();
        VL_DO_DANGLING(placeholderp->delete_ونه(), placeholderp);
    }

    فراغ fixupLibStub(ثابت std::string& لار, Ast_غوټه_Stmt* placeholderp) {
        File_کرښه* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_غوټه::prettyName(لار);
        إذا (!v3Global.opt.libCreate().تشه()) {
            ثابت std::string سر_مختاړی = v3Global.rootp()->trace_Lib_سر_Name() + ".";
            إذا (VString::startsWith(name, سر_مختاړی)) name = name.substr(سر_مختاړی.اندازه());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = جديد AstCStmt{flp};
        initp->جمع_کړه("{\n");
        initp->جمع_کړه("std::string __VlibName = vlSymsp->name();\n");
        initp->جمع_کړه("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->جمع_کړه("__VlibName += ");
        initp->جمع_کړه(جديد AstConst{flp, AstConst::String{}, name});
        initp->جمع_کړه(";\n");
        initp->جمع_کړه("tracep->initLib(__VlibName);\n");
        initp->جمع_کړه("}\n");

        placeholderp->جمع_کړه_بل_دلته(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_شا()->delete_ونه(), placeholderp);
    }

    فراغ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        لكل (ثابت خپلکاری& توکی_2 : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(توکی_2);
            ثابت AstCell* ثابت cellp = std::get<1>(توکی_2);
            Ast_غوټه_Stmt* ثابت placeholderp = std::get<2>(توکی_2);
            ثابت std::string لار = parentp->name() + "__DOT__" + cellp->name();
            إذا (cellp->modp()->verilatorLib()) {
                fixupLibStub(لار, placeholderp);
            } وإلا {
                fixupPlaceholder(لار, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        لكل (ثابت خپلکاری& توکی_2 : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(توکی_2);
            Ast_غوټه_Stmt* ثابت placeholderp = std::get<1>(توکی_2);
            ثابت std::string لار = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(لار, placeholderp);
        }
    }

    فراغ remove_Redundant_مختاړی_ټېل_راوباسه() {
        لكل (ثابت خپلکاری& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            لكل (AstCFunc* ثابت funcp : pair.دوهم) {
                Ast_غوټه* prevp = تش_اشاره;
                Ast_غوټه* currp = funcp->stmtsp();
                طالما (currp) {
                    Ast_غوټه* ثابت nextp = currp->nextp();
                    إذا (VN_IS(prevp, Trace_ټېل_مختاړی) && VN_IS(currp, Trace_راوباسه_مختاړی)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_شا()->delete_ونه(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_شا()->delete_ونه(), currp);
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
        إذا (!m_declUncalledps.تشه()) {  // LCOV_EXCL_START
            لكل (خپلکاری tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    فراغ checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([هذا](ثابت Ast_غوټه* nodep) {
            إذا (ثابت AstTraceDecl* ثابت declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } وإلا إذا (ثابت AstCCall* ثابت ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    منطقي isBasicIO() { أرجع m_traVscp->varp()->isVLIO(); }
    فراغ newDeclSubFunc() {
        File_کرښه* ثابت flp = m_dtypeFuncp->fileline();
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtype_نومونه.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_دلیلونه);
        AstCCall* ثابت subCallp = جديد AstCCall{flp, subFuncp};
        subCallp->dtype_سېټ_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->جمع_کړه_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_اندازه = 0;
    }
    فراغ newDeclFunc(Ast_غوټه_DType* nodep) {
        Ast_غوټه_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_دلیلونه{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_کرښه* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_کلی dtype_کلی{skipTypep, m_traVscp->varp()->varType()};
        خپلکاری pair = m_dtypeFuncs.emplace(dtype_کلی, تش_اشاره);
        AstCFunc** funcpp = &pair.لومړی->دوهم;
        إذا (pair.دوهم) {
            *funcpp = newCFunc(flp, m_dtype_نومونه.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_دلیلونه);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = جديد AstCCall{flp, *funcpp};
        callp->dtype_سېټ_Void();
        callp->argTypes(call_دلیلونه);
        m_dtypeDeclp = جمع_کړه_Trace_Decl(VNumRange{}, skipTypep->پلنوالی(), callp);
        جمع_کړه_منزل_Sub_Func(callp->makeStmt());

        إذا (pair.دوهم) {
            VL_RESTORER(m_دننه_واټن);
            m_دننه_واټن = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_اندازه);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            إذا (AstStructDType* ثابت dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, ریښتیا);
            } وإلا إذا (AstUnpackArrayDType* ثابت dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, ریښتیا);
            } وإلا إذا (AstPackArrayDType* ثابت dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, ریښتیا);
            } وإلا {
                UASSERT_OBJ(خطأ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_دننه_واټن);
        }

        m_dtypeDeclp->کوډ_Inc((*funcpp)->user1());
        m_دننه_واټن += m_dtypeDeclp->کوډ_Inc();
    }
    فراغ declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقي newFunc) {
        string مختاړی_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_کرښه* ثابت flp = nodep->fileline();

        جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_ټېل_مختاړی{flp, مختاړی_Name, VTrace_مختاړی_Type::ARRAY_UNPACKED,
                                            nodep->کیڼ(), nodep->ښی(), !newFunc});

        إذا (VN_IS(nodep->subDTypep()->skip_Ref_منزل_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_منزل_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            إذا (m_traVscp->dtypep()->skip_Ref_منزل_Enump()->isString()) {
                جمع_کړه_Ignore("Unsupported: strings");
            } وإلا {
                m_traName = "";
                جمع_کړه_Trace_Decl(nodep->declRange(), 0);
            }
        } وإلا {
            Ast_غوټه_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();
            // Always iterate left index to right index
            لكل (ثابت صحيح i : nodep->declRange().seq_کیڼ_منزل_ښی()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::منزل_string(i) + ']';
                m_traValuep = m_traValuep->clone_ونه(خطأ);
                m_traValuep = جديد AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ونه(), m_traValuep);
            }
        }

        جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_راوباسه_مختاړی{flp});
    }
    فراغ declPackedArray(AstPackArrayDType* ثابت nodep, منطقي newFunc) {
        string مختاړی_Name(newFunc ? "name" : m_traName);
        Ast_غوټه_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();

        VL_RESTORER_COPY(m_traName);
        File_کرښه* ثابت flp = nodep->fileline();

        جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_ټېل_مختاړی{flp, مختاړی_Name, VTrace_مختاړی_Type::ARRAY_PACKED,
                                            nodep->کیڼ(), nodep->ښی(), !newFunc});

        // Always iterate left index to right index
        لكل (ثابت صحيح i : nodep->declRange().seq_کیڼ_منزل_ښی()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::منزل_string(i) + ']';
            ثابت صحيح lsb = (i - nodep->lo()) * subtypep->پلنوالی();
            m_traValuep = m_traValuep->clone_ونه(خطأ);
            m_traValuep = جديد AstSel{flp, m_traValuep, lsb, subtypep->پلنوالی()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_ونه(), m_traValuep = تش_اشاره);
        }

        جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_راوباسه_مختاړی{flp});
    }
    فراغ declStruct(AstStructDType* ثابت nodep, منطقي newFunc) {
        File_کرښه* ثابت flp = nodep->fileline();
        string مختاړی_Name(newFunc ? "name" : m_traName);
        صحيح nMembers = 0;
        لكل (Ast_غوټه* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        إذا (!nodep->packed()) {
            جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_ټېل_مختاړی{flp, مختاړی_Name,  //
                                                VTrace_مختاړی_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_غوټه_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ونه(خطأ);
                m_traValuep = جديد AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ونه(), m_traValuep);
            }
            جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_راوباسه_مختاړی{flp});
        } وإلا {
            جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_ټېل_مختاړی{flp, مختاړی_Name,  //
                                                VTrace_مختاړی_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_غوټه_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ونه(خطأ);
                m_traValuep = جديد AstSel{flp, m_traValuep, itemp->lsb(), subtypep->پلنوالی()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ونه(), m_traValuep);
            }
            جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_راوباسه_مختاړی{flp});
        }
    }

    // VISITORS
    فراغ لیدنه(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.تشه(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.تشه(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.تشه(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        إذا (nodep->modp()->verilatorLib()) أرجع;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        لكل (Ast_غوټه* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_شا(nodep, cellp);
        }

        إذا (!m_entries.تشه()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_ترتیبول(
                m_entries.begin(), m_entries.پای_ټکی(),
                [](ثابت Trace_ثبت& a, ثابت Trace_ثبت& b) { أرجع a.operatorCompare(b); });

            File_کرښه* ثابت flp = nodep->fileline();
            لار_Adjustor لار_Adjustor_2{flp, [&](Ast_غوټه_Stmt* stmtp) { جمع_کړه_منزل_Sub_Func(stmtp); }};
            ثابت منطقي وېشل_ریښه_Primary_Ios = nodep->is_سر() && !v3Global.opt.libCreate().تشه();
            ثابت خپلکاری emit_ثبت = [&](ثابت Trace_ثبت& ثبت) {
                AstVarScope* ثابت vscp = ثبت.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << ثبت.لار() << "' name='" << ثبت.name() << "' "
                                  << (ثبت.cellp() ? static_cast<Ast_غوټه*>(ثبت.cellp())
                                                    : static_cast<Ast_غوټه*>(ثبت.vscp())));
                لار_Adjustor_2.adjust(ثبت.لار(), ثبت.cellp(), ثبت.vscp());

                m_traName = ثبت.name();

                إذا (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    إذا (!ignoreReason.تشه()) {
                        جمع_کړه_Ignore(ignoreReason);
                    } وإلا {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = جديد AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_منزل_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        إذا (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_ونه(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = تش_اشاره;
                        }
                    }
                } وإلا {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = ثبت.cellp();
                    Ast_غوټه_Stmt* ثابت stmtp = جديد AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    جمع_کړه_منزل_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_شا(nodep, cellp, stmtp);
                }
            };
            إذا (وېشل_ریښه_Primary_Ios) {
                m_سر_Scope_ریښه_Phase = ریښتیا;
                لكل (ثابت Trace_ثبت& ثبت : m_entries) {
                    AstVarScope* ثابت vscp = ثبت.vscp();
                    إذا (!(vscp && vscp->varp()->isPrimaryIO())) استمر;
                    emit_ثبت(ثبت);
                }
                m_سر_Scope_ریښه_Phase = خطأ;
                لار_Adjustor_2.unwind();
                m_سر_Scope_ریښه_Func_شمېر = m_subFuncps.اندازه();
                إذا (m_سر_Scope_ریښه_Func_شمېر) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_اندازه = 0;
                    m_force_New_ونې_پاڼه = ریښتیا;
                }
            }
            لكل (ثابت Trace_ثبت& ثبت : m_entries) {
                AstVarScope* ثابت vscp = ثبت.vscp();
                إذا (وېشل_ریښه_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) استمر;
                emit_ثبت(ثبت);
            }
            لار_Adjustor_2.unwind();
            m_traVscp = تش_اشاره;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string لار = nodep->prettyName();
        m_لار_منزل_Scopep.emplace(لار, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        إذا (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت اندازه_t وروستی_Dot = لار.پیدا_کړه_وروستی_د('.');
            UASSERT_OBJ(وروستی_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << لار);
            ثابت std::string پلار_لار = لار.substr(0, وروستی_Dot + 1);

            لكل (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                إذا (!VString::startsWith(refName, پلار_لار)) استمر;

                // Save the mapping from the path of the reference to the scope
                m_لار_منزل_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                إذا (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_شا()->delete_ونه(), intfRefp);
                }
            }
        }
    }
    فراغ لیدنه(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        إذا (nodep->varp()->is_لنډمهاله() && !nodep->varp()->isTrace()) أرجع;
        إذا (nodep->varp()->isClassMember()) أرجع;
        إذا (nodep->varp()->isFuncLocal()) أرجع;

        // When creating a --lib-create library ...
        إذا (!v3Global.opt.libCreate().تشه()) {
            // Ignore parameters in packages. These will be traced at the top level.
            إذا (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) أرجع;
        }

        // Add to traced signal list
        m_entries.emplace_شا(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    فراغ لیدنه(AstConstDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_دننه_واټن);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ریښتیا;
        iterate(nodep->subDTypep()->skip_Ref_منزل_Enump());
    }
    فراغ لیدنه(AstRefDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_دننه_واټن);
        iterate(nodep->subDTypep()->skip_Ref_منزل_Enump());
    }
    فراغ لیدنه(AstIfaceRefDType* /*nodep*/) override {
        إذا (!m_traVscp) أرجع;
        // Insert a placeholder to be fixed up later
        File_کرښه* ثابت flp = m_traVscp->fileline();
        Ast_غوټه_Stmt* ثابت stmtp
            = جديد AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        جمع_کړه_منزل_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_شا(m_traVscp, stmtp);
    }
    فراغ لیدنه(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;

        إذا (v3Global.opt.traceMaxArray()
            && static_cast<صحيح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            جمع_کړه_Ignore("Wide memory > --trace-max-array ents");
            أرجع;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        إذا (VN_IS(nodep->subDTypep()->skip_Ref_منزل_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_منزل_Enump()->isString()) {
            جمع_کړه_Ignore("Unsupported: strings");
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = ریښتیا;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_دننه_واټن);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, خطأ);
    }
    فراغ لیدنه(AstPackArrayDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جمع_کړه_Trace_Decl(VNumRange{}, nodep->پلنوالی());
            أرجع;
        }

        Ast_غوټه_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        إذا (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            إذا (basicp->is_بټ_Logic() && !basicp->isRanged()) {
                جمع_کړه_Trace_Decl(VNumRange{}, nodep->پلنوالی());
                أرجع;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = ریښتیا;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_دننه_واټن);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, خطأ);
    }
    فراغ لیدنه(AstStructDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جمع_کړه_Trace_Decl(VNumRange{}, nodep->پلنوالی());
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = ریښتیا;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_دننه_واټن);
            newDeclFunc(nodep);
        }
        declStruct(nodep, خطأ);
    }
    فراغ لیدنه(AstUnionDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ریښتیا;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جمع_کړه_Trace_Decl(VNumRange{}, nodep->پلنوالی());
            أرجع;
        }

        VL_RESTORER_COPY(m_traName);
        File_کرښه* ثابت flp = nodep->fileline();

        صحيح nMembers = 0;
        لكل (Ast_غوټه* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        إذا (!nodep->packed()) {
            جمع_کړه_Ignore("Unsupported: Unpacked union");
        } وإلا {
            جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_ټېل_مختاړی{flp, m_traName,  //
                                                VTrace_مختاړی_Type::UNION_PACKED, nMembers});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_غوټه_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            جمع_کړه_منزل_Sub_Func(جديد Ast_Trace_راوباسه_مختاړی{flp});
        }
    }
    فراغ لیدنه(AstBasicDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        إذا (nodep->isString()) {
            جمع_کړه_Ignore("Unsupported: strings");
        } وإلا {
            جمع_کړه_Trace_Decl(VNumRange{}, 0);
        }
    }
    فراغ لیدنه(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    فراغ لیدنه(Ast_غوټه_DType*) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;
        جمع_کړه_Ignore("Unsupported: data type");
    }

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند TraceDeclVisitor(AstNetlist* nodep)
        : m_سر_Scopep{nodep->سر_Scopep()} {
        File_کرښه* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_بل_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.تشه(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_مختاړی_ټېل_راوباسه();

        ثابت std::ویکټور<AstCFunc*>& سر_Scope_Funcps = m_scopeInitFuncps.at(m_سر_Scopep->scopep());
        AstCFunc* ریښه_Funcp = تش_اشاره;
        إذا (!v3Global.opt.libCreate().تشه()) {
            ریښه_Funcp = newCFunc(flp, "trace_init_root");
            ریښه_Funcp->ثبت_ټکی(ریښتیا);
            لكل (اندازه_t i = 0; i < m_سر_Scope_ریښه_Func_شمېر; ++i) {
                AstCCall* ثابت callp = جديد AstCCall{flp, سر_Scope_Funcps.at(i)};
                callp->dtype_سېټ_Void();
                callp->argTypes("tracep");
                ریښه_Funcp->جمع_کړه_Stmtsp(callp->makeStmt());
            }
            إذا (!m_سر_Scope_ریښه_Func_شمېر) ریښه_Funcp->جمع_کړه_Stmtsp(جديد AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        لكل (اندازه_t i = m_سر_Scope_ریښه_Func_شمېر; i < سر_Scope_Funcps.اندازه(); ++i) {
            AstCFunc* ثابت funcp = سر_Scope_Funcps.at(i);
            AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
            callp->dtype_سېټ_Void();
            callp->argTypes("tracep");
            جمع_کړه_منزل_سر_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        إذا (m_سر_Funcps.تشه()) جمع_کړه_منزل_سر_Func(جديد AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        إذا (m_سر_Funcps.اندازه() > 1) {
            AstCFunc* ثابت سر_Funcp = newCFunc(flp, "");
            لكل (AstCFunc* funcp : m_سر_Funcps) {
                AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
                callp->dtype_سېټ_Void();
                callp->argTypes("tracep");
                سر_Funcp->جمع_کړه_Stmtsp(callp->makeStmt());
            }
            m_سر_Funcps.clear();
            m_سر_Funcps.ټېل_شا(سر_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت سر_Funcp = m_سر_Funcps.front();
        سر_Funcp->name("trace_init_top");
        سر_Funcp->ثبت_ټکی(ریښتیا);

        إذا (ریښه_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ریښه_Funcp);
        checkCalls(سر_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::جمع_کړه_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::جمع_کړه_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("tracedecl", 0, dump_ونه_Either_Level() >= 3);
}
