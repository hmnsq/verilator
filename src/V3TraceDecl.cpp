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

صنف رستو_Adjustor final {
    File_سٽ* ثابت m_flp;  // FileLine used for created nodes
    std::function<فراغ(Ast_ڳنڍ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::ويڪٽر<std::string> m_ڍير{""};  // Stack of current paths

    ساكن constexpr حرف SEPARATOR = ' ';

عام:
    واضح رستو_Adjustor(File_سٽ* flp, std::function<فراغ(Ast_ڳنڍ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    فراغ adjust(ثابت string& new_رستو, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        طالما (!VString::startsWith(new_رستو, m_ڍير.پٺ())) {
            m_emit(جديد Ast_Trace_ڪڍو_اڳياڙي{m_flp});
            m_ڍير.ڪڍو_پٺ();
        }

        إذا (new_رستو == m_ڍير.پٺ()) أرجع;

        ثابت VTrace_اڳياڙي_Type آخري_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_اڳياڙي_Type::SCOPE_INTERFACE
                : VTrace_اڳياڙي_Type::SCOPE_MODULE;
        ثابت std::string extra_اڳياڙي = new_رستو.substr(m_ڍير.پٺ().سائيز());
        سائيز_t begin = 0;
        ثابت سائيز_t آخري = extra_اڳياڙي.rfind(SEPARATOR);
        // Move down, one path element at a time
        طالما (سچ) {
            ثابت سائيز_t پڇاڙي = extra_اڳياڙي.ڳوليو(SEPARATOR, begin);
            إذا (پڇاڙي == string::npos) اكسر;
            ثابت string& extra = extra_اڳياڙي.substr(begin, پڇاڙي - begin);
            إذا (پڇاڙي == آخري) {
                m_emit(جديد Ast_Trace_ڌڪو_اڳياڙي{m_flp, extra, آخري_Scope_Type});
            } وإلا {
                m_emit(جديد Ast_Trace_ڌڪو_اڳياڙي{m_flp, extra, VTrace_اڳياڙي_Type::SCOPE_MODULE});
            }
            m_ڍير.ڌڪو_پٺ(m_ڍير.پٺ() + extra + SEPARATOR);
            begin = پڇاڙي + 1;
        }
        UASSERT(begin == extra_اڳياڙي.سائيز(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    فراغ unwind() {
        طالما (m_ڍير.سائيز() > 1) {
            m_emit(جديد Ast_Trace_ڪڍو_اڳياڙي{m_flp});
            m_ڍير.ڪڍو_پٺ();
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
    Ast_چوٽي_Scope* ثابت m_چوٽي_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = خالي_اشارو;  // Current scope being visited

    std::ويڪٽر<AstCFunc*> m_چوٽي_Funcps;  // Top level trace initialization functions
    std::ويڪٽر<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::سيٽ<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    بنية Dtype_Func_ڪنجي final {
        ثابت Ast_ڳنڍ_DType* dtypep;
        VVarType varType;
        منطقي عامل==(ثابت Dtype_Func_ڪنجي& other) ثابت {
            أرجع dtypep == other.dtypep && varType == other.varType;
        }
    };
    بنية Dtype_Func_ڪنجي_Hash final {
        سائيز_t عامل()(ثابت Dtype_Func_ڪنجي& ڪنجي) ثابت {
            أرجع std::hash<ثابت Ast_ڳنڍ_DType*>{}(ڪنجي.dtypep)
                   ^ (std::hash<uint8_t>{}(ڪنجي.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ڪنجي, AstCFunc*, Dtype_Func_ڪنجي_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = خالي_اشارو;  // Current type func
    AstCFunc* m_dtypeSubFuncp = خالي_اشارو;  // Current type sub func
    ثابت string m_dtype_دليلون{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = خالي_اشارو;  // Current type func decl
    V3_Unique_نالا m_dtype_نالا{""};  // Unique names for dtype funcs
    منطقي m_skipDtypeFunc = خطأ;  // Don't create a type func
    uint32_t m_وٿي = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحيح m_چوٽي_Func_سائيز = 0;  // Size of the top function currently being built
    صحيح m_sub_Func_سائيز = 0;  // Size of the sub function currently being built
    منطقي m_force_New_پن = خطأ;  // Force next addToSubFunc to start a fresh leaf
    سائيز_t m_چوٽي_Scope_پاڙ_Func_ڳڻپ = 0;  // Top-scope init functions used only for wrapper IOs
    منطقي m_چوٽي_Scope_پاڙ_Phase = خطأ;  // Emitting top-scope wrapper IO declarations
    ثابت صحيح m_func_سائيز_حد_بندي  // Maximum size of a function
        = v3Global.opt.output_ورهايو_CTrace() ? v3Global.opt.output_ورهايو_CTrace()
                                           : std::numeric_limits<صحيح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::ويڪٽر<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_رستو_منزل_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::ويڪٽر<std::tuple<AstScope*, AstCell*, Ast_ڳنڍ_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::ويڪٽر<std::tuple<AstVarScope*, Ast_ڳنڍ_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    صنف Trace_داخلا final {
        AstVarScope* m_vscp = خالي_اشارو;  // AstVarScope under scope being traced
        AstCell* m_cellp = خالي_اشارو;  // Sub scope (as AstCell) under scope being traced
        std::string m_رستو;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        فراغ init(ثابت std::string& name, Ast_ڳنڍ* nodep, منطقي اندريون_چوٽي_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_ڳنڍ::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_رستو = vcdName + " ";
                m_name.clear();
            } وإلا إذا (varp->isPrimaryIO()) {
                // Primary IO variable
                m_رستو = "$rootio ";
                m_name = vcdName;
            } وإلا {
                // Other Variable
                ثابت سائيز_t pos = vcdName.rfind(' ');
                ثابت سائيز_t رستو_Len = pos == std::string::npos ? 0 : pos + 1;
                m_رستو = vcdName.substr(0, رستو_Len);
                m_name = vcdName.substr(رستو_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            إذا (اندريون_چوٽي_Scope && !v3Global.opt.libCreate().خالي_جايون()) {
                ثابت سائيز_t شروعات = m_رستو.ڳوليو(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(شروعات != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string اڳياڙي = m_رستو.substr(0, شروعات);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                إذا (اڳياڙي == "$rootio") أرجع;
                m_رستو = m_رستو.substr(شروعات + 1);
                إذا (v3Global.rootp()->trace_Lib_چوٽي_Name() != اڳياڙي) m_رستو = "$libroot " + m_رستو;
            }
        }

    عام:
        واضح Trace_داخلا(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_چوٽي());
        }
        واضح Trace_داخلا(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_چوٽي());
        }
        صحيح operatorCompare(ثابت Trace_داخلا& b) ثابت {
            إذا (ثابت صحيح cmp = رستو().compare(b.رستو())) أرجع cmp < 0;
            إذا (ثابت صحيح cmp = fileline().operatorCompare(b.fileline())) أرجع cmp < 0;
            أرجع name() < b.name();
        }
        AstVarScope* vscp() ثابت { أرجع m_vscp; }
        AstCell* cellp() ثابت { أرجع m_cellp; }
        ثابت std::string& رستو() ثابت { أرجع m_رستو; }
        فراغ رستو(ثابت std::string& رستو) { m_رستو = رستو; }
        ثابت std::string& name() ثابت { أرجع m_name; }
        File_سٽ& fileline() ثابت { أرجع m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::ويڪٽر<Trace_داخلا> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = خالي_اشارو;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ڳنڍ_اظهار* m_traValuep = خالي_اشارو;  // Value expression for current signal
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

        ثابت صحيح ويڪر = recurse_DType_ويڪر(nodep->varp()->dtypep());
        إذا (v3Global.opt.trace_Max_ويڪر() && ويڪر > v3Global.opt.trace_Max_ويڪر())
            أرجع "Width " + cvt_منزل_Str(ويڪر) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        إذا (!V3Control::getScopeTraceOn(prettyName)) أرجع "Vlt scope trace_off";
        إذا (!v3Global.opt.traceUnderscore()) {
            إذا (!prettyName.خالي_جايون() && prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.ڳوليو("._") != string::npos) أرجع "Inlined leading underscore";
        }
        أرجع ""s;
    }

    صحيح recurse_DType_ويڪر(ثابت Ast_ڳنڍ_DType* nodep) {
        إذا (ثابت Ast_ڳنڍ_Array_DType* adtypep = VN_CAST(nodep, ڳنڍ_Array_DType))
            أرجع recurse_DType_ويڪر(adtypep->subDTypep()) * adtypep->declRange().elements();
        أرجع nodep->ويڪر();
    }

    AstCFunc* newCFunc(File_سٽ* flp, ثابت string& name) {
        AstScope* ثابت چوٽي_Scopep = m_چوٽي_Scopep->scopep();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, name, چوٽي_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_بنياد() + "* tracep");
        funcp->isTrace(سچ);
        funcp->isStatic(خطأ);
        funcp->isLoose(سچ);
        funcp->slow(سچ);
        چوٽي_Scopep->جوڙيو_Blocksp(funcp);
        أرجع funcp;
    }

    فراغ جوڙيو_منزل_چوٽي_Func(Ast_ڳنڍ_Stmt* stmtp) {
        إذا (m_چوٽي_Func_سائيز > m_func_سائيز_حد_بندي || m_چوٽي_Funcps.خالي_جايون()) {
            m_چوٽي_Func_سائيز = 0;
            //
            ثابت string n = cvt_منزل_Str(m_چوٽي_Funcps.سائيز());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_چوٽي_Scopep->fileline(), name);
            m_چوٽي_Funcps.ڌڪو_پٺ(funcp);
        }
        m_چوٽي_Funcps.پٺ()->جوڙيو_Stmtsp(stmtp);
        m_چوٽي_Func_سائيز += stmtp->ڳنڍ_ڳڻپ();
    }

    فراغ جوڙيو_منزل_Sub_Func(Ast_ڳنڍ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        إذا (m_dtypeSubFuncp) {
            إذا (m_sub_Func_سائيز > m_func_سائيز_حد_بندي) newDeclSubFunc();
            m_dtypeSubFuncp->جوڙيو_Stmtsp(stmtp);
            m_sub_Func_سائيز += stmtp->ڳنڍ_ڳڻپ();
            أرجع;
        }
        // Defer trace splitting until V3Trace
        إذا (m_subFuncps.خالي_جايون() || m_force_New_پن) {
            m_force_New_پن = خطأ;
            File_سٽ* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvt_منزل_Str(m_subFuncps.سائيز());
            ثابت string name
                = m_currScopep == m_چوٽي_Scopep->scopep() && !v3Global.opt.libCreate().خالي_جايون()
                      ? (m_چوٽي_Scope_پاڙ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ڌڪو_پٺ(funcp);
        }
        m_subFuncps.پٺ()->جوڙيو_Stmtsp(stmtp);
        m_sub_Func_سائيز += stmtp->ڳنڍ_ڳڻپ();
    }

    AstTraceDecl* جوڙيو_Trace_Decl(ثابت VNumRange& arrayRange,
                               صحيح ويڪر_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = خالي_اشارو) {
        VNumRange ٻٽو_انگ_Range;
        إذا (ويڪر_Override) {
            ٻٽو_انگ_Range = VNumRange{ويڪر_Override - 1, 0};
        } وإلا إذا (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            ٻٽو_انگ_Range = bdtypep->nrange();
        }
        File_سٽ* ثابت flp = m_traVscp->fileline();
        Ast_ڳنڍ_اظهار* valuep = m_traValuep->clone_وڻ(خطأ);
        ثابت منطقي valid_وٿي = m_وٿي != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = جديد AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               ٻٽو_انگ_Range, arrayRange, dtypeCallp,        valid_وٿي};
        إذا (valid_وٿي) {
            newp->ڪوڊ(m_وٿي);
            إذا (!dtypeCallp) { m_وٿي += newp->ڪوڊ_Inc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(جديد AstCExpr{flp, "__VdtypeVar", m_traVscp->ويڪر()});
                VL_DO_DANGLING(refp->delete_وڻ(), refp);
            });
        } وإلا {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        جوڙيو_منزل_Sub_Func(newp);

        أرجع newp;
    }

    فراغ جوڙيو_Ignore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        إذا (debug() > 3 && m_traVscp) std::ٻاهر << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    فراغ fixupPlaceholder(ثابت std::string& رستو, Ast_ڳنڍ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت خودڪار it = m_رستو_منزل_Scopep.ڳوليو(Ast_ڳنڍ::prettyName(رستو));
        إذا (it != m_رستو_منزل_Scopep.پڇاڙي()) {
            ثابت AstScope* ثابت scopep = it->ٻيو;
            File_سٽ* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ڳنڍ* stmtp = خالي_اشارو;
            لكل (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = جديد AstCCall{flp, subFuncp};
                callp->dtype_سيٽ_Void();
                callp->argTypes("tracep");
                stmtp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            إذا (stmtp) placeholderp->جوڙيو_ايندڙ_هتي(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_پٺ();
        VL_DO_DANGLING(placeholderp->delete_وڻ(), placeholderp);
    }

    فراغ fixupLibStub(ثابت std::string& رستو, Ast_ڳنڍ_Stmt* placeholderp) {
        File_سٽ* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ڳنڍ::prettyName(رستو);
        إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            ثابت std::string چوٽي_اڳياڙي = v3Global.rootp()->trace_Lib_چوٽي_Name() + ".";
            إذا (VString::startsWith(name, چوٽي_اڳياڙي)) name = name.substr(چوٽي_اڳياڙي.سائيز());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = جديد AstCStmt{flp};
        initp->جوڙيو("{\n");
        initp->جوڙيو("std::string __VlibName = vlSymsp->name();\n");
        initp->جوڙيو("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->جوڙيو("__VlibName += ");
        initp->جوڙيو(جديد AstConst{flp, AstConst::String{}, name});
        initp->جوڙيو(";\n");
        initp->جوڙيو("tracep->initLib(__VlibName);\n");
        initp->جوڙيو("}\n");

        placeholderp->جوڙيو_ايندڙ_هتي(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_پٺ()->delete_وڻ(), placeholderp);
    }

    فراغ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        لكل (ثابت خودڪار& شي_2 : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(شي_2);
            ثابت AstCell* ثابت cellp = std::get<1>(شي_2);
            Ast_ڳنڍ_Stmt* ثابت placeholderp = std::get<2>(شي_2);
            ثابت std::string رستو = parentp->name() + "__DOT__" + cellp->name();
            إذا (cellp->modp()->verilatorLib()) {
                fixupLibStub(رستو, placeholderp);
            } وإلا {
                fixupPlaceholder(رستو, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        لكل (ثابت خودڪار& شي_2 : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(شي_2);
            Ast_ڳنڍ_Stmt* ثابت placeholderp = std::get<1>(شي_2);
            ثابت std::string رستو = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(رستو, placeholderp);
        }
    }

    فراغ remove_Redundant_اڳياڙي_ڌڪو_ڪڍو() {
        لكل (ثابت خودڪار& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            لكل (AstCFunc* ثابت funcp : pair.ٻيو) {
                Ast_ڳنڍ* prevp = خالي_اشارو;
                Ast_ڳنڍ* currp = funcp->stmtsp();
                طالما (currp) {
                    Ast_ڳنڍ* ثابت nextp = currp->nextp();
                    إذا (VN_IS(prevp, Trace_ڌڪو_اڳياڙي) && VN_IS(currp, Trace_ڪڍو_اڳياڙي)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_پٺ()->delete_وڻ(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_پٺ()->delete_وڻ(), currp);
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
        إذا (!m_declUncalledps.خالي_جايون()) {  // LCOV_EXCL_START
            لكل (خودڪار tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    فراغ checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([هذا](ثابت Ast_ڳنڍ* nodep) {
            إذا (ثابت AstTraceDecl* ثابت declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } وإلا إذا (ثابت AstCCall* ثابت ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    منطقي isBasicIO() { أرجع m_traVscp->varp()->isVLIO(); }
    فراغ newDeclSubFunc() {
        File_سٽ* ثابت flp = m_dtypeFuncp->fileline();
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtype_نالا.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_دليلون);
        AstCCall* ثابت subCallp = جديد AstCCall{flp, subFuncp};
        subCallp->dtype_سيٽ_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->جوڙيو_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_سائيز = 0;
    }
    فراغ newDeclFunc(Ast_ڳنڍ_DType* nodep) {
        Ast_ڳنڍ_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_دليلون{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_سٽ* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_ڪنجي dtype_ڪنجي{skipTypep, m_traVscp->varp()->varType()};
        خودڪار pair = m_dtypeFuncs.emplace(dtype_ڪنجي, خالي_اشارو);
        AstCFunc** funcpp = &pair.پهريون->ٻيو;
        إذا (pair.ٻيو) {
            *funcpp = newCFunc(flp, m_dtype_نالا.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_دليلون);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = جديد AstCCall{flp, *funcpp};
        callp->dtype_سيٽ_Void();
        callp->argTypes(call_دليلون);
        m_dtypeDeclp = جوڙيو_Trace_Decl(VNumRange{}, skipTypep->ويڪر(), callp);
        جوڙيو_منزل_Sub_Func(callp->makeStmt());

        إذا (pair.ٻيو) {
            VL_RESTORER(m_وٿي);
            m_وٿي = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_سائيز);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            إذا (AstStructDType* ثابت dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, سچ);
            } وإلا إذا (AstUnpackArrayDType* ثابت dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, سچ);
            } وإلا إذا (AstPackArrayDType* ثابت dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, سچ);
            } وإلا {
                UASSERT_OBJ(خطأ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_وٿي);
        }

        m_dtypeDeclp->ڪوڊ_Inc((*funcpp)->user1());
        m_وٿي += m_dtypeDeclp->ڪوڊ_Inc();
    }
    فراغ declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقي newFunc) {
        string اڳياڙي_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_سٽ* ثابت flp = nodep->fileline();

        جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڌڪو_اڳياڙي{flp, اڳياڙي_Name, VTrace_اڳياڙي_Type::ARRAY_UNPACKED,
                                            nodep->کاٻي(), nodep->ساڄي(), !newFunc});

        إذا (VN_IS(nodep->subDTypep()->skip_Ref_منزل_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_منزل_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            إذا (m_traVscp->dtypep()->skip_Ref_منزل_Enump()->isString()) {
                جوڙيو_Ignore("Unsupported: strings");
            } وإلا {
                m_traName = "";
                جوڙيو_Trace_Decl(nodep->declRange(), 0);
            }
        } وإلا {
            Ast_ڳنڍ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();
            // Always iterate left index to right index
            لكل (ثابت صحيح i : nodep->declRange().seq_کاٻو_منزل_ساڄو()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::منزل_string(i) + ']';
                m_traValuep = m_traValuep->clone_وڻ(خطأ);
                m_traValuep = جديد AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_وڻ(), m_traValuep);
            }
        }

        جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڪڍو_اڳياڙي{flp});
    }
    فراغ declPackedArray(AstPackArrayDType* ثابت nodep, منطقي newFunc) {
        string اڳياڙي_Name(newFunc ? "name" : m_traName);
        Ast_ڳنڍ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();

        VL_RESTORER_COPY(m_traName);
        File_سٽ* ثابت flp = nodep->fileline();

        جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڌڪو_اڳياڙي{flp, اڳياڙي_Name, VTrace_اڳياڙي_Type::ARRAY_PACKED,
                                            nodep->کاٻي(), nodep->ساڄي(), !newFunc});

        // Always iterate left index to right index
        لكل (ثابت صحيح i : nodep->declRange().seq_کاٻو_منزل_ساڄو()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::منزل_string(i) + ']';
            ثابت صحيح lsb = (i - nodep->lo()) * subtypep->ويڪر();
            m_traValuep = m_traValuep->clone_وڻ(خطأ);
            m_traValuep = جديد AstSel{flp, m_traValuep, lsb, subtypep->ويڪر()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_وڻ(), m_traValuep = خالي_اشارو);
        }

        جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڪڍو_اڳياڙي{flp});
    }
    فراغ declStruct(AstStructDType* ثابت nodep, منطقي newFunc) {
        File_سٽ* ثابت flp = nodep->fileline();
        string اڳياڙي_Name(newFunc ? "name" : m_traName);
        صحيح nMembers = 0;
        لكل (Ast_ڳنڍ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        إذا (!nodep->packed()) {
            جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڌڪو_اڳياڙي{flp, اڳياڙي_Name,  //
                                                VTrace_اڳياڙي_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ڳنڍ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_وڻ(خطأ);
                m_traValuep = جديد AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_وڻ(), m_traValuep);
            }
            جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڪڍو_اڳياڙي{flp});
        } وإلا {
            جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڌڪو_اڳياڙي{flp, اڳياڙي_Name,  //
                                                VTrace_اڳياڙي_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ڳنڍ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_وڻ(خطأ);
                m_traValuep = جديد AstSel{flp, m_traValuep, itemp->lsb(), subtypep->ويڪر()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_وڻ(), m_traValuep);
            }
            جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڪڍو_اڳياڙي{flp});
        }
    }

    // VISITORS
    فراغ گهمو(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.خالي_جايون(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالي_جايون(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.خالي_جايون(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        إذا (nodep->modp()->verilatorLib()) أرجع;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        لكل (Ast_ڳنڍ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_پٺ(nodep, cellp);
        }

        إذا (!m_entries.خالي_جايون()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_ترتيب_ڏيو(
                m_entries.begin(), m_entries.پڇاڙي(),
                [](ثابت Trace_داخلا& a, ثابت Trace_داخلا& b) { أرجع a.operatorCompare(b); });

            File_سٽ* ثابت flp = nodep->fileline();
            رستو_Adjustor رستو_Adjustor_2{flp, [&](Ast_ڳنڍ_Stmt* stmtp) { جوڙيو_منزل_Sub_Func(stmtp); }};
            ثابت منطقي ورهايو_پاڙ_Primary_Ios = nodep->is_چوٽي() && !v3Global.opt.libCreate().خالي_جايون();
            ثابت خودڪار emit_داخلا = [&](ثابت Trace_داخلا& داخلا_2) {
                AstVarScope* ثابت vscp = داخلا_2.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << داخلا_2.رستو() << "' name='" << داخلا_2.name() << "' "
                                  << (داخلا_2.cellp() ? static_cast<Ast_ڳنڍ*>(داخلا_2.cellp())
                                                    : static_cast<Ast_ڳنڍ*>(داخلا_2.vscp())));
                رستو_Adjustor_2.adjust(داخلا_2.رستو(), داخلا_2.cellp(), داخلا_2.vscp());

                m_traName = داخلا_2.name();

                إذا (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    إذا (!ignoreReason.خالي_جايون()) {
                        جوڙيو_Ignore(ignoreReason);
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
                            VL_DO_DANGLING(m_traValuep->delete_وڻ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = خالي_اشارو;
                        }
                    }
                } وإلا {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = داخلا_2.cellp();
                    Ast_ڳنڍ_Stmt* ثابت stmtp = جديد AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    جوڙيو_منزل_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_پٺ(nodep, cellp, stmtp);
                }
            };
            إذا (ورهايو_پاڙ_Primary_Ios) {
                m_چوٽي_Scope_پاڙ_Phase = سچ;
                لكل (ثابت Trace_داخلا& داخلا_2 : m_entries) {
                    AstVarScope* ثابت vscp = داخلا_2.vscp();
                    إذا (!(vscp && vscp->varp()->isPrimaryIO())) استمر;
                    emit_داخلا(داخلا_2);
                }
                m_چوٽي_Scope_پاڙ_Phase = خطأ;
                رستو_Adjustor_2.unwind();
                m_چوٽي_Scope_پاڙ_Func_ڳڻپ = m_subFuncps.سائيز();
                إذا (m_چوٽي_Scope_پاڙ_Func_ڳڻپ) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_سائيز = 0;
                    m_force_New_پن = سچ;
                }
            }
            لكل (ثابت Trace_داخلا& داخلا_2 : m_entries) {
                AstVarScope* ثابت vscp = داخلا_2.vscp();
                إذا (ورهايو_پاڙ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) استمر;
                emit_داخلا(داخلا_2);
            }
            رستو_Adjustor_2.unwind();
            m_traVscp = خالي_اشارو;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string رستو = nodep->prettyName();
        m_رستو_منزل_Scopep.emplace(رستو, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        إذا (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت سائيز_t آخري_Dot = رستو.ڳوليو_آخري_جو('.');
            UASSERT_OBJ(آخري_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << رستو);
            ثابت std::string والد_رستو = رستو.substr(0, آخري_Dot + 1);

            لكل (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                إذا (!VString::startsWith(refName, والد_رستو)) استمر;

                // Save the mapping from the path of the reference to the scope
                m_رستو_منزل_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                إذا (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_پٺ()->delete_وڻ(), intfRefp);
                }
            }
        }
    }
    فراغ گهمو(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        إذا (nodep->varp()->is_عارضي() && !nodep->varp()->isTrace()) أرجع;
        إذا (nodep->varp()->isClassMember()) أرجع;
        إذا (nodep->varp()->isFuncLocal()) أرجع;

        // When creating a --lib-create library ...
        إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            // Ignore parameters in packages. These will be traced at the top level.
            إذا (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) أرجع;
        }

        // Add to traced signal list
        m_entries.emplace_پٺ(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    فراغ گهمو(AstConstDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_وٿي);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = سچ;
        iterate(nodep->subDTypep()->skip_Ref_منزل_Enump());
    }
    فراغ گهمو(AstRefDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_وٿي);
        iterate(nodep->subDTypep()->skip_Ref_منزل_Enump());
    }
    فراغ گهمو(AstIfaceRefDType* /*nodep*/) override {
        إذا (!m_traVscp) أرجع;
        // Insert a placeholder to be fixed up later
        File_سٽ* ثابت flp = m_traVscp->fileline();
        Ast_ڳنڍ_Stmt* ثابت stmtp
            = جديد AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        جوڙيو_منزل_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_پٺ(m_traVscp, stmtp);
    }
    فراغ گهمو(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;

        إذا (v3Global.opt.traceMaxArray()
            && static_cast<صحيح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            جوڙيو_Ignore("Wide memory > --trace-max-array ents");
            أرجع;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        إذا (VN_IS(nodep->subDTypep()->skip_Ref_منزل_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_منزل_Enump()->isString()) {
            جوڙيو_Ignore("Unsupported: strings");
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = سچ;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_وٿي);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, خطأ);
    }
    فراغ گهمو(AstPackArrayDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڙيو_Trace_Decl(VNumRange{}, nodep->ويڪر());
            أرجع;
        }

        Ast_ڳنڍ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        إذا (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            إذا (basicp->is_ٻٽو_انگ_Logic() && !basicp->isRanged()) {
                جوڙيو_Trace_Decl(VNumRange{}, nodep->ويڪر());
                أرجع;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = سچ;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_وٿي);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, خطأ);
    }
    فراغ گهمو(AstStructDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڙيو_Trace_Decl(VNumRange{}, nodep->ويڪر());
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = سچ;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_وٿي);
            newDeclFunc(nodep);
        }
        declStruct(nodep, خطأ);
    }
    فراغ گهمو(AstUnionDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = سچ;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڙيو_Trace_Decl(VNumRange{}, nodep->ويڪر());
            أرجع;
        }

        VL_RESTORER_COPY(m_traName);
        File_سٽ* ثابت flp = nodep->fileline();

        صحيح nMembers = 0;
        لكل (Ast_ڳنڍ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        إذا (!nodep->packed()) {
            جوڙيو_Ignore("Unsupported: Unpacked union");
        } وإلا {
            جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڌڪو_اڳياڙي{flp, m_traName,  //
                                                VTrace_اڳياڙي_Type::UNION_PACKED, nMembers});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ڳنڍ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            جوڙيو_منزل_Sub_Func(جديد Ast_Trace_ڪڍو_اڳياڙي{flp});
        }
    }
    فراغ گهمو(AstBasicDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        إذا (nodep->isString()) {
            جوڙيو_Ignore("Unsupported: strings");
        } وإلا {
            جوڙيو_Trace_Decl(VNumRange{}, 0);
        }
    }
    فراغ گهمو(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    فراغ گهمو(Ast_ڳنڍ_DType*) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;
        جوڙيو_Ignore("Unsupported: data type");
    }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TraceDeclVisitor(AstNetlist* nodep)
        : m_چوٽي_Scopep{nodep->چوٽي_Scopep()} {
        File_سٽ* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_ايندڙ_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.خالي_جايون(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_اڳياڙي_ڌڪو_ڪڍو();

        ثابت std::ويڪٽر<AstCFunc*>& چوٽي_Scope_Funcps = m_scopeInitFuncps.at(m_چوٽي_Scopep->scopep());
        AstCFunc* پاڙ_Funcp = خالي_اشارو;
        إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            پاڙ_Funcp = newCFunc(flp, "trace_init_root");
            پاڙ_Funcp->داخلا_نقطو(سچ);
            لكل (سائيز_t i = 0; i < m_چوٽي_Scope_پاڙ_Func_ڳڻپ; ++i) {
                AstCCall* ثابت callp = جديد AstCCall{flp, چوٽي_Scope_Funcps.at(i)};
                callp->dtype_سيٽ_Void();
                callp->argTypes("tracep");
                پاڙ_Funcp->جوڙيو_Stmtsp(callp->makeStmt());
            }
            إذا (!m_چوٽي_Scope_پاڙ_Func_ڳڻپ) پاڙ_Funcp->جوڙيو_Stmtsp(جديد AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        لكل (سائيز_t i = m_چوٽي_Scope_پاڙ_Func_ڳڻپ; i < چوٽي_Scope_Funcps.سائيز(); ++i) {
            AstCFunc* ثابت funcp = چوٽي_Scope_Funcps.at(i);
            AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
            callp->dtype_سيٽ_Void();
            callp->argTypes("tracep");
            جوڙيو_منزل_چوٽي_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        إذا (m_چوٽي_Funcps.خالي_جايون()) جوڙيو_منزل_چوٽي_Func(جديد AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        إذا (m_چوٽي_Funcps.سائيز() > 1) {
            AstCFunc* ثابت چوٽي_Funcp = newCFunc(flp, "");
            لكل (AstCFunc* funcp : m_چوٽي_Funcps) {
                AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
                callp->dtype_سيٽ_Void();
                callp->argTypes("tracep");
                چوٽي_Funcp->جوڙيو_Stmtsp(callp->makeStmt());
            }
            m_چوٽي_Funcps.clear();
            m_چوٽي_Funcps.ڌڪو_پٺ(چوٽي_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت چوٽي_Funcp = m_چوٽي_Funcps.front();
        چوٽي_Funcp->name("trace_init_top");
        چوٽي_Funcp->داخلا_نقطو(سچ);

        إذا (پاڙ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(پاڙ_Funcp);
        checkCalls(چوٽي_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::جوڙيو_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::جوڙيو_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("tracedecl", 0, dump_وڻ_Either_Level() >= 3);
}
