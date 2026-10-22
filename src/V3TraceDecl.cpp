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

صنف رستہ_Adjustor final {
    File_سطر* ثابت m_flp;  // FileLine used for created nodes
    std::function<فراغ(Ast_گرہ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::سمتیہ<std::string> m_ڈھیر{""};  // Stack of current paths

    ساكن constexpr حرف SEPARATOR = ' ';

عام:
    واضح رستہ_Adjustor(File_سطر* flp, std::function<فراغ(Ast_گرہ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    فراغ adjust(ثابت string& new_رستہ, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        طالما (!VString::startsWith(new_رستہ, m_ڈھیر.پٹھ())) {
            m_emit(جديد Ast_Trace_نکالو_اگیتر{m_flp});
            m_ڈھیر.نکالو_پٹھ();
        }

        إذا (new_رستہ == m_ڈھیر.پٹھ()) أرجع;

        ثابت VTrace_اگیتر_Type آخری_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_اگیتر_Type::SCOPE_INTERFACE
                : VTrace_اگیتر_Type::SCOPE_MODULE;
        ثابت std::string extra_اگیتر = new_رستہ.substr(m_ڈھیر.پٹھ().ناپ());
        ناپ_t begin = 0;
        ثابت ناپ_t آخری = extra_اگیتر.rfind(SEPARATOR);
        // Move down, one path element at a time
        طالما (سچ) {
            ثابت ناپ_t سرا = extra_اگیتر.لبھو(SEPARATOR, begin);
            إذا (سرا == string::npos) اكسر;
            ثابت string& extra = extra_اگیتر.substr(begin, سرا - begin);
            إذا (سرا == آخری) {
                m_emit(جديد Ast_Trace_ڈالو_اگیتر{m_flp, extra, آخری_Scope_Type});
            } وإلا {
                m_emit(جديد Ast_Trace_ڈالو_اگیتر{m_flp, extra, VTrace_اگیتر_Type::SCOPE_MODULE});
            }
            m_ڈھیر.ڈالو_پٹھ(m_ڈھیر.پٹھ() + extra + SEPARATOR);
            begin = سرا + 1;
        }
        UASSERT(begin == extra_اگیتر.ناپ(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    فراغ unwind() {
        طالما (m_ڈھیر.ناپ() > 1) {
            m_emit(جديد Ast_Trace_نکالو_اگیتر{m_flp});
            m_ڈھیر.نکالو_پٹھ();
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
    Ast_چوٹی_Scope* ثابت m_چوٹی_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = خالی_سوچک;  // Current scope being visited

    std::سمتیہ<AstCFunc*> m_چوٹی_Funcps;  // Top level trace initialization functions
    std::سمتیہ<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::سیٹ<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    بنية Dtype_Func_کلید final {
        ثابت Ast_گرہ_DType* dtypep;
        VVarType varType;
        منطقي عامل==(ثابت Dtype_Func_کلید& other) ثابت {
            أرجع dtypep == other.dtypep && varType == other.varType;
        }
    };
    بنية Dtype_Func_کلید_Hash final {
        ناپ_t عامل()(ثابت Dtype_Func_کلید& کلید) ثابت {
            أرجع std::hash<ثابت Ast_گرہ_DType*>{}(کلید.dtypep)
                   ^ (std::hash<uint8_t>{}(کلید.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_کلید, AstCFunc*, Dtype_Func_کلید_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = خالی_سوچک;  // Current type func
    AstCFunc* m_dtypeSubFuncp = خالی_سوچک;  // Current type sub func
    ثابت string m_dtype_دلیلاں{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = خالی_سوچک;  // Current type func decl
    V3_Unique_ناں m_dtype_ناں{""};  // Unique names for dtype funcs
    منطقي m_skipDtypeFunc = خطأ;  // Don't create a type func
    uint32_t m_وتھ = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحيح m_چوٹی_Func_ناپ = 0;  // Size of the top function currently being built
    صحيح m_sub_Func_ناپ = 0;  // Size of the sub function currently being built
    منطقي m_force_New_پتی = خطأ;  // Force next addToSubFunc to start a fresh leaf
    ناپ_t m_چوٹی_Scope_جڑ_Func_گنتی = 0;  // Top-scope init functions used only for wrapper IOs
    منطقي m_چوٹی_Scope_جڑ_Phase = خطأ;  // Emitting top-scope wrapper IO declarations
    ثابت صحيح m_func_ناپ_حد_بندی  // Maximum size of a function
        = v3Global.opt.output_ونڈو_CTrace() ? v3Global.opt.output_ونڈو_CTrace()
                                           : std::numeric_limits<صحيح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::سمتیہ<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_رستہ_منزل_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::سمتیہ<std::tuple<AstScope*, AstCell*, Ast_گرہ_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::سمتیہ<std::tuple<AstVarScope*, Ast_گرہ_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    صنف Trace_اندراج final {
        AstVarScope* m_vscp = خالی_سوچک;  // AstVarScope under scope being traced
        AstCell* m_cellp = خالی_سوچک;  // Sub scope (as AstCell) under scope being traced
        std::string m_رستہ;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        فراغ init(ثابت std::string& name, Ast_گرہ* nodep, منطقي وچ_چوٹی_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_گرہ::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_رستہ = vcdName + " ";
                m_name.clear();
            } وإلا إذا (varp->isPrimaryIO()) {
                // Primary IO variable
                m_رستہ = "$rootio ";
                m_name = vcdName;
            } وإلا {
                // Other Variable
                ثابت ناپ_t pos = vcdName.rfind(' ');
                ثابت ناپ_t رستہ_Len = pos == std::string::npos ? 0 : pos + 1;
                m_رستہ = vcdName.substr(0, رستہ_Len);
                m_name = vcdName.substr(رستہ_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            إذا (وچ_چوٹی_Scope && !v3Global.opt.libCreate().خالی_تھاواں()) {
                ثابت ناپ_t شروع = m_رستہ.لبھو(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(شروع != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string اگیتر = m_رستہ.substr(0, شروع);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                إذا (اگیتر == "$rootio") أرجع;
                m_رستہ = m_رستہ.substr(شروع + 1);
                إذا (v3Global.rootp()->trace_Lib_چوٹی_Name() != اگیتر) m_رستہ = "$libroot " + m_رستہ;
            }
        }

    عام:
        واضح Trace_اندراج(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_چوٹی());
        }
        واضح Trace_اندراج(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_چوٹی());
        }
        صحيح operatorCompare(ثابت Trace_اندراج& b) ثابت {
            إذا (ثابت صحيح cmp = رستہ().compare(b.رستہ())) أرجع cmp < 0;
            إذا (ثابت صحيح cmp = fileline().operatorCompare(b.fileline())) أرجع cmp < 0;
            أرجع name() < b.name();
        }
        AstVarScope* vscp() ثابت { أرجع m_vscp; }
        AstCell* cellp() ثابت { أرجع m_cellp; }
        ثابت std::string& رستہ() ثابت { أرجع m_رستہ; }
        فراغ رستہ(ثابت std::string& رستہ) { m_رستہ = رستہ; }
        ثابت std::string& name() ثابت { أرجع m_name; }
        File_سطر& fileline() ثابت { أرجع m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::سمتیہ<Trace_اندراج> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = خالی_سوچک;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_گرہ_اظہاریہ* m_traValuep = خالی_سوچک;  // Value expression for current signal
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

        ثابت صحيح چوڑائی = recurse_DType_چوڑائی(nodep->varp()->dtypep());
        إذا (v3Global.opt.trace_Max_چوڑائی() && چوڑائی > v3Global.opt.trace_Max_چوڑائی())
            أرجع "Width " + cvt_منزل_Str(چوڑائی) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        إذا (!V3Control::getScopeTraceOn(prettyName)) أرجع "Vlt scope trace_off";
        إذا (!v3Global.opt.traceUnderscore()) {
            إذا (!prettyName.خالی_تھاواں() && prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.لبھو("._") != string::npos) أرجع "Inlined leading underscore";
        }
        أرجع ""s;
    }

    صحيح recurse_DType_چوڑائی(ثابت Ast_گرہ_DType* nodep) {
        إذا (ثابت Ast_گرہ_Array_DType* adtypep = VN_CAST(nodep, گرہ_Array_DType))
            أرجع recurse_DType_چوڑائی(adtypep->subDTypep()) * adtypep->declRange().elements();
        أرجع nodep->چوڑائی();
    }

    AstCFunc* newCFunc(File_سطر* flp, ثابت string& name) {
        AstScope* ثابت چوٹی_Scopep = m_چوٹی_Scopep->scopep();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, name, چوٹی_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_ادھار() + "* tracep");
        funcp->isTrace(سچ);
        funcp->isStatic(خطأ);
        funcp->isLoose(سچ);
        funcp->slow(سچ);
        چوٹی_Scopep->جوڑو_Blocksp(funcp);
        أرجع funcp;
    }

    فراغ جوڑو_منزل_چوٹی_Func(Ast_گرہ_Stmt* stmtp) {
        إذا (m_چوٹی_Func_ناپ > m_func_ناپ_حد_بندی || m_چوٹی_Funcps.خالی_تھاواں()) {
            m_چوٹی_Func_ناپ = 0;
            //
            ثابت string n = cvt_منزل_Str(m_چوٹی_Funcps.ناپ());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_چوٹی_Scopep->fileline(), name);
            m_چوٹی_Funcps.ڈالو_پٹھ(funcp);
        }
        m_چوٹی_Funcps.پٹھ()->جوڑو_Stmtsp(stmtp);
        m_چوٹی_Func_ناپ += stmtp->گرہ_گنتی();
    }

    فراغ جوڑو_منزل_Sub_Func(Ast_گرہ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        إذا (m_dtypeSubFuncp) {
            إذا (m_sub_Func_ناپ > m_func_ناپ_حد_بندی) newDeclSubFunc();
            m_dtypeSubFuncp->جوڑو_Stmtsp(stmtp);
            m_sub_Func_ناپ += stmtp->گرہ_گنتی();
            أرجع;
        }
        // Defer trace splitting until V3Trace
        إذا (m_subFuncps.خالی_تھاواں() || m_force_New_پتی) {
            m_force_New_پتی = خطأ;
            File_سطر* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvt_منزل_Str(m_subFuncps.ناپ());
            ثابت string name
                = m_currScopep == m_چوٹی_Scopep->scopep() && !v3Global.opt.libCreate().خالی_تھاواں()
                      ? (m_چوٹی_Scope_جڑ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->جوڑو_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ڈالو_پٹھ(funcp);
        }
        m_subFuncps.پٹھ()->جوڑو_Stmtsp(stmtp);
        m_sub_Func_ناپ += stmtp->گرہ_گنتی();
    }

    AstTraceDecl* جوڑو_Trace_Decl(ثابت VNumRange& arrayRange,
                               صحيح چوڑائی_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = خالی_سوچک) {
        VNumRange ثنائی_ہندسہ_Range;
        إذا (چوڑائی_Override) {
            ثنائی_ہندسہ_Range = VNumRange{چوڑائی_Override - 1, 0};
        } وإلا إذا (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            ثنائی_ہندسہ_Range = bdtypep->nrange();
        }
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گرہ_اظہاریہ* valuep = m_traValuep->clone_درخت(خطأ);
        ثابت منطقي valid_وتھ = m_وتھ != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = جديد AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               ثنائی_ہندسہ_Range, arrayRange, dtypeCallp,        valid_وتھ};
        إذا (valid_وتھ) {
            newp->کوڈ(m_وتھ);
            إذا (!dtypeCallp) { m_وتھ += newp->کوڈ_Inc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(جديد AstCExpr{flp, "__VdtypeVar", m_traVscp->چوڑائی()});
                VL_DO_DANGLING(refp->delete_درخت(), refp);
            });
        } وإلا {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        جوڑو_منزل_Sub_Func(newp);

        أرجع newp;
    }

    فراغ جوڑو_Ignore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        إذا (debug() > 3 && m_traVscp) std::باہر << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    فراغ fixupPlaceholder(ثابت std::string& رستہ, Ast_گرہ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت خودکار it = m_رستہ_منزل_Scopep.لبھو(Ast_گرہ::prettyName(رستہ));
        إذا (it != m_رستہ_منزل_Scopep.سرا()) {
            ثابت AstScope* ثابت scopep = it->دوجا;
            File_سطر* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_گرہ* stmtp = خالی_سوچک;
            لكل (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = جديد AstCCall{flp, subFuncp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                stmtp = Ast_گرہ::جوڑو_اگلا(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            إذا (stmtp) placeholderp->جوڑو_اگلا_ایتھے(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_پٹھ();
        VL_DO_DANGLING(placeholderp->delete_درخت(), placeholderp);
    }

    فراغ fixupLibStub(ثابت std::string& رستہ, Ast_گرہ_Stmt* placeholderp) {
        File_سطر* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_گرہ::prettyName(رستہ);
        إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            ثابت std::string چوٹی_اگیتر = v3Global.rootp()->trace_Lib_چوٹی_Name() + ".";
            إذا (VString::startsWith(name, چوٹی_اگیتر)) name = name.substr(چوٹی_اگیتر.ناپ());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = جديد AstCStmt{flp};
        initp->جوڑو("{\n");
        initp->جوڑو("std::string __VlibName = vlSymsp->name();\n");
        initp->جوڑو("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->جوڑو("__VlibName += ");
        initp->جوڑو(جديد AstConst{flp, AstConst::String{}, name});
        initp->جوڑو(";\n");
        initp->جوڑو("tracep->initLib(__VlibName);\n");
        initp->جوڑو("}\n");

        placeholderp->جوڑو_اگلا_ایتھے(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_پٹھ()->delete_درخت(), placeholderp);
    }

    فراغ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        لكل (ثابت خودکار& شے_2 : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(شے_2);
            ثابت AstCell* ثابت cellp = std::get<1>(شے_2);
            Ast_گرہ_Stmt* ثابت placeholderp = std::get<2>(شے_2);
            ثابت std::string رستہ = parentp->name() + "__DOT__" + cellp->name();
            إذا (cellp->modp()->verilatorLib()) {
                fixupLibStub(رستہ, placeholderp);
            } وإلا {
                fixupPlaceholder(رستہ, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        لكل (ثابت خودکار& شے_2 : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(شے_2);
            Ast_گرہ_Stmt* ثابت placeholderp = std::get<1>(شے_2);
            ثابت std::string رستہ = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(رستہ, placeholderp);
        }
    }

    فراغ remove_Redundant_اگیتر_ڈالو_نکالو() {
        لكل (ثابت خودکار& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            لكل (AstCFunc* ثابت funcp : pair.دوجا) {
                Ast_گرہ* prevp = خالی_سوچک;
                Ast_گرہ* currp = funcp->stmtsp();
                طالما (currp) {
                    Ast_گرہ* ثابت nextp = currp->nextp();
                    إذا (VN_IS(prevp, Trace_ڈالو_اگیتر) && VN_IS(currp, Trace_نکالو_اگیتر)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_پٹھ()->delete_درخت(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_پٹھ()->delete_درخت(), currp);
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
        إذا (!m_declUncalledps.خالی_تھاواں()) {  // LCOV_EXCL_START
            لكل (خودکار tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    فراغ checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([هذا](ثابت Ast_گرہ* nodep) {
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
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtype_ناں.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_دلیلاں);
        AstCCall* ثابت subCallp = جديد AstCCall{flp, subFuncp};
        subCallp->dtype_سیٹ_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->جوڑو_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_ناپ = 0;
    }
    فراغ newDeclFunc(Ast_گرہ_DType* nodep) {
        Ast_گرہ_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_دلیلاں{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_کلید dtype_کلید{skipTypep, m_traVscp->varp()->varType()};
        خودکار pair = m_dtypeFuncs.emplace(dtype_کلید, خالی_سوچک);
        AstCFunc** funcpp = &pair.پہلا->دوجا;
        إذا (pair.دوجا) {
            *funcpp = newCFunc(flp, m_dtype_ناں.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_دلیلاں);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = جديد AstCCall{flp, *funcpp};
        callp->dtype_سیٹ_Void();
        callp->argTypes(call_دلیلاں);
        m_dtypeDeclp = جوڑو_Trace_Decl(VNumRange{}, skipTypep->چوڑائی(), callp);
        جوڑو_منزل_Sub_Func(callp->makeStmt());

        إذا (pair.دوجا) {
            VL_RESTORER(m_وتھ);
            m_وتھ = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_ناپ);
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
            m_dtypeFuncp->user1(m_وتھ);
        }

        m_dtypeDeclp->کوڈ_Inc((*funcpp)->user1());
        m_وتھ += m_dtypeDeclp->کوڈ_Inc();
    }
    فراغ declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقي newFunc) {
        string اگیتر_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        جوڑو_منزل_Sub_Func(جديد Ast_Trace_ڈالو_اگیتر{flp, اگیتر_Name, VTrace_اگیتر_Type::ARRAY_UNPACKED,
                                            nodep->کھبے(), nodep->سجے(), !newFunc});

        إذا (VN_IS(nodep->subDTypep()->skip_Ref_منزل_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_منزل_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            إذا (m_traVscp->dtypep()->skip_Ref_منزل_Enump()->isString()) {
                جوڑو_Ignore("Unsupported: strings");
            } وإلا {
                m_traName = "";
                جوڑو_Trace_Decl(nodep->declRange(), 0);
            }
        } وإلا {
            Ast_گرہ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();
            // Always iterate left index to right index
            لكل (ثابت صحيح i : nodep->declRange().seq_بایاں_منزل_دایاں()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::منزل_string(i) + ']';
                m_traValuep = m_traValuep->clone_درخت(خطأ);
                m_traValuep = جديد AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
        }

        جوڑو_منزل_Sub_Func(جديد Ast_Trace_نکالو_اگیتر{flp});
    }
    فراغ declPackedArray(AstPackArrayDType* ثابت nodep, منطقي newFunc) {
        string اگیتر_Name(newFunc ? "name" : m_traName);
        Ast_گرہ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        جوڑو_منزل_Sub_Func(جديد Ast_Trace_ڈالو_اگیتر{flp, اگیتر_Name, VTrace_اگیتر_Type::ARRAY_PACKED,
                                            nodep->کھبے(), nodep->سجے(), !newFunc});

        // Always iterate left index to right index
        لكل (ثابت صحيح i : nodep->declRange().seq_بایاں_منزل_دایاں()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::منزل_string(i) + ']';
            ثابت صحيح lsb = (i - nodep->lo()) * subtypep->چوڑائی();
            m_traValuep = m_traValuep->clone_درخت(خطأ);
            m_traValuep = جديد AstSel{flp, m_traValuep, lsb, subtypep->چوڑائی()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_درخت(), m_traValuep = خالی_سوچک);
        }

        جوڑو_منزل_Sub_Func(جديد Ast_Trace_نکالو_اگیتر{flp});
    }
    فراغ declStruct(AstStructDType* ثابت nodep, منطقي newFunc) {
        File_سطر* ثابت flp = nodep->fileline();
        string اگیتر_Name(newFunc ? "name" : m_traName);
        صحيح nMembers = 0;
        لكل (Ast_گرہ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        إذا (!nodep->packed()) {
            جوڑو_منزل_Sub_Func(جديد Ast_Trace_ڈالو_اگیتر{flp, اگیتر_Name,  //
                                                VTrace_اگیتر_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(خطأ);
                m_traValuep = جديد AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            جوڑو_منزل_Sub_Func(جديد Ast_Trace_نکالو_اگیتر{flp});
        } وإلا {
            جوڑو_منزل_Sub_Func(جديد Ast_Trace_ڈالو_اگیتر{flp, اگیتر_Name,  //
                                                VTrace_اگیتر_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(خطأ);
                m_traValuep = جديد AstSel{flp, m_traValuep, itemp->lsb(), subtypep->چوڑائی()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            جوڑو_منزل_Sub_Func(جديد Ast_Trace_نکالو_اگیتر{flp});
        }
    }

    // VISITORS
    فراغ ویکھو(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.خالی_تھاواں(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالی_تھاواں(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.خالی_تھاواں(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        إذا (nodep->modp()->verilatorLib()) أرجع;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        لكل (Ast_گرہ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_پٹھ(nodep, cellp);
        }

        إذا (!m_entries.خالی_تھاواں()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_ترتیب_دیو(
                m_entries.begin(), m_entries.سرا(),
                [](ثابت Trace_اندراج& a, ثابت Trace_اندراج& b) { أرجع a.operatorCompare(b); });

            File_سطر* ثابت flp = nodep->fileline();
            رستہ_Adjustor رستہ_Adjustor_2{flp, [&](Ast_گرہ_Stmt* stmtp) { جوڑو_منزل_Sub_Func(stmtp); }};
            ثابت منطقي ونڈو_جڑ_Primary_Ios = nodep->is_چوٹی() && !v3Global.opt.libCreate().خالی_تھاواں();
            ثابت خودکار emit_اندراج = [&](ثابت Trace_اندراج& اندراج) {
                AstVarScope* ثابت vscp = اندراج.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << اندراج.رستہ() << "' name='" << اندراج.name() << "' "
                                  << (اندراج.cellp() ? static_cast<Ast_گرہ*>(اندراج.cellp())
                                                    : static_cast<Ast_گرہ*>(اندراج.vscp())));
                رستہ_Adjustor_2.adjust(اندراج.رستہ(), اندراج.cellp(), اندراج.vscp());

                m_traName = اندراج.name();

                إذا (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    إذا (!ignoreReason.خالی_تھاواں()) {
                        جوڑو_Ignore(ignoreReason);
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
                            VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = خالی_سوچک;
                        }
                    }
                } وإلا {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = اندراج.cellp();
                    Ast_گرہ_Stmt* ثابت stmtp = جديد AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    جوڑو_منزل_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_پٹھ(nodep, cellp, stmtp);
                }
            };
            إذا (ونڈو_جڑ_Primary_Ios) {
                m_چوٹی_Scope_جڑ_Phase = سچ;
                لكل (ثابت Trace_اندراج& اندراج : m_entries) {
                    AstVarScope* ثابت vscp = اندراج.vscp();
                    إذا (!(vscp && vscp->varp()->isPrimaryIO())) استمر;
                    emit_اندراج(اندراج);
                }
                m_چوٹی_Scope_جڑ_Phase = خطأ;
                رستہ_Adjustor_2.unwind();
                m_چوٹی_Scope_جڑ_Func_گنتی = m_subFuncps.ناپ();
                إذا (m_چوٹی_Scope_جڑ_Func_گنتی) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_ناپ = 0;
                    m_force_New_پتی = سچ;
                }
            }
            لكل (ثابت Trace_اندراج& اندراج : m_entries) {
                AstVarScope* ثابت vscp = اندراج.vscp();
                إذا (ونڈو_جڑ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) استمر;
                emit_اندراج(اندراج);
            }
            رستہ_Adjustor_2.unwind();
            m_traVscp = خالی_سوچک;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string رستہ = nodep->prettyName();
        m_رستہ_منزل_Scopep.emplace(رستہ, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        إذا (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت ناپ_t آخری_Dot = رستہ.لبھو_آخری_دا('.');
            UASSERT_OBJ(آخری_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << رستہ);
            ثابت std::string ماپا_رستہ = رستہ.substr(0, آخری_Dot + 1);

            لكل (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                إذا (!VString::startsWith(refName, ماپا_رستہ)) استمر;

                // Save the mapping from the path of the reference to the scope
                m_رستہ_منزل_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                إذا (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_پٹھ()->delete_درخت(), intfRefp);
                }
            }
        }
    }
    فراغ ویکھو(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        إذا (nodep->varp()->is_عارضی() && !nodep->varp()->isTrace()) أرجع;
        إذا (nodep->varp()->isClassMember()) أرجع;
        إذا (nodep->varp()->isFuncLocal()) أرجع;

        // When creating a --lib-create library ...
        إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            // Ignore parameters in packages. These will be traced at the top level.
            إذا (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) أرجع;
        }

        // Add to traced signal list
        m_entries.emplace_پٹھ(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    فراغ ویکھو(AstConstDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_وتھ);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = سچ;
        iterate(nodep->subDTypep()->skip_Ref_منزل_Enump());
    }
    فراغ ویکھو(AstRefDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_وتھ);
        iterate(nodep->subDTypep()->skip_Ref_منزل_Enump());
    }
    فراغ ویکھو(AstIfaceRefDType* /*nodep*/) override {
        إذا (!m_traVscp) أرجع;
        // Insert a placeholder to be fixed up later
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گرہ_Stmt* ثابت stmtp
            = جديد AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        جوڑو_منزل_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_پٹھ(m_traVscp, stmtp);
    }
    فراغ ویکھو(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;

        إذا (v3Global.opt.traceMaxArray()
            && static_cast<صحيح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            جوڑو_Ignore("Wide memory > --trace-max-array ents");
            أرجع;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        إذا (VN_IS(nodep->subDTypep()->skip_Ref_منزل_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_منزل_Enump()->isString()) {
            جوڑو_Ignore("Unsupported: strings");
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = سچ;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_وتھ);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, خطأ);
    }
    فراغ ویکھو(AstPackArrayDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
            أرجع;
        }

        Ast_گرہ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_منزل_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        إذا (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            إذا (basicp->is_ثنائی_ہندسہ_Logic() && !basicp->isRanged()) {
                جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
                أرجع;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = سچ;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_وتھ);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, خطأ);
    }
    فراغ ویکھو(AstStructDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
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
            VL_RESTORER(m_وتھ);
            newDeclFunc(nodep);
        }
        declStruct(nodep, خطأ);
    }
    فراغ ویکھو(AstUnionDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = سچ;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
            أرجع;
        }

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        صحيح nMembers = 0;
        لكل (Ast_گرہ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        إذا (!nodep->packed()) {
            جوڑو_Ignore("Unsupported: Unpacked union");
        } وإلا {
            جوڑو_منزل_Sub_Func(جديد Ast_Trace_ڈالو_اگیتر{flp, m_traName,  //
                                                VTrace_اگیتر_Type::UNION_PACKED, nMembers});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_منزل_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            جوڑو_منزل_Sub_Func(جديد Ast_Trace_نکالو_اگیتر{flp});
        }
    }
    فراغ ویکھو(AstBasicDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        إذا (nodep->isString()) {
            جوڑو_Ignore("Unsupported: strings");
        } وإلا {
            جوڑو_Trace_Decl(VNumRange{}, 0);
        }
    }
    فراغ ویکھو(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    فراغ ویکھو(Ast_گرہ_DType*) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;
        جوڑو_Ignore("Unsupported: data type");
    }

    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TraceDeclVisitor(AstNetlist* nodep)
        : m_چوٹی_Scopep{nodep->چوٹی_Scopep()} {
        File_سطر* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_اگلا_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.خالی_تھاواں(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_اگیتر_ڈالو_نکالو();

        ثابت std::سمتیہ<AstCFunc*>& چوٹی_Scope_Funcps = m_scopeInitFuncps.at(m_چوٹی_Scopep->scopep());
        AstCFunc* جڑ_Funcp = خالی_سوچک;
        إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            جڑ_Funcp = newCFunc(flp, "trace_init_root");
            جڑ_Funcp->اندراج_نقطہ(سچ);
            لكل (ناپ_t i = 0; i < m_چوٹی_Scope_جڑ_Func_گنتی; ++i) {
                AstCCall* ثابت callp = جديد AstCCall{flp, چوٹی_Scope_Funcps.at(i)};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                جڑ_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
            إذا (!m_چوٹی_Scope_جڑ_Func_گنتی) جڑ_Funcp->جوڑو_Stmtsp(جديد AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        لكل (ناپ_t i = m_چوٹی_Scope_جڑ_Func_گنتی; i < چوٹی_Scope_Funcps.ناپ(); ++i) {
            AstCFunc* ثابت funcp = چوٹی_Scope_Funcps.at(i);
            AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
            callp->dtype_سیٹ_Void();
            callp->argTypes("tracep");
            جوڑو_منزل_چوٹی_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        إذا (m_چوٹی_Funcps.خالی_تھاواں()) جوڑو_منزل_چوٹی_Func(جديد AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        إذا (m_چوٹی_Funcps.ناپ() > 1) {
            AstCFunc* ثابت چوٹی_Funcp = newCFunc(flp, "");
            لكل (AstCFunc* funcp : m_چوٹی_Funcps) {
                AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                چوٹی_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
            m_چوٹی_Funcps.clear();
            m_چوٹی_Funcps.ڈالو_پٹھ(چوٹی_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت چوٹی_Funcp = m_چوٹی_Funcps.front();
        چوٹی_Funcp->name("trace_init_top");
        چوٹی_Funcp->اندراج_نقطہ(سچ);

        إذا (جڑ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(جڑ_Funcp);
        checkCalls(چوٹی_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::جوڑو_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::جوڑو_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("tracedecl", 0, dump_درخت_Either_Level() >= 3);
}
