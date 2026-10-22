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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3TraceDecl.h"

#شامل "V3Ast.h"
#شامل "V3Control.h"
#شامل "V3EmitCBase.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Number.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

#شامل <cstdint>
#شامل <functional>
#شامل <limits>
#شامل <tuple>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

جماعت راستہ_Adjustor final {
    File_سطر* ثابت m_flp;  // FileLine used for created nodes
    std::function<خالی(Ast_گرہ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::سمتیہ<std::string> m_ڈھیر{""};  // Stack of current paths

    ساکن constexpr حرف SEPARATOR = ' ';

عوامی:
    صریح راستہ_Adjustor(File_سطر* flp, std::function<خالی(Ast_گرہ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    خالی adjust(ثابت string& new_راستہ, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        جب (!VString::startsWith(new_راستہ, m_ڈھیر.عقب())) {
            m_emit(نیا Ast_Trace_نکالو_سابقہ{m_flp});
            m_ڈھیر.نکالو_عقب();
        }

        اگر (new_راستہ == m_ڈھیر.عقب()) واپس;

        ثابت VTrace_سابقہ_Type آخری_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_سابقہ_Type::SCOPE_INTERFACE
                : VTrace_سابقہ_Type::SCOPE_MODULE;
        ثابت std::string extra_سابقہ = new_راستہ.substr(m_ڈھیر.عقب().سائز());
        سائز_t begin = 0;
        ثابت سائز_t آخری = extra_سابقہ.rfind(SEPARATOR);
        // Move down, one path element at a time
        جب (سچ) {
            ثابت سائز_t انتہا = extra_سابقہ.ڈھونڈو(SEPARATOR, begin);
            اگر (انتہا == string::npos) توڑو;
            ثابت string& extra = extra_سابقہ.substr(begin, انتہا - begin);
            اگر (انتہا == آخری) {
                m_emit(نیا Ast_Trace_ڈالو_سابقہ{m_flp, extra, آخری_Scope_Type});
            } ورنہ {
                m_emit(نیا Ast_Trace_ڈالو_سابقہ{m_flp, extra, VTrace_سابقہ_Type::SCOPE_MODULE});
            }
            m_ڈھیر.ڈالو_عقب(m_ڈھیر.عقب() + extra + SEPARATOR);
            begin = انتہا + 1;
        }
        UASSERT(begin == extra_سابقہ.سائز(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    خالی unwind() {
        جب (m_ڈھیر.سائز() > 1) {
            m_emit(نیا Ast_Trace_نکالو_سابقہ{m_flp});
            m_ڈھیر.نکالو_عقب();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

جماعت TraceDeclVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_چوٹی_Scope* ثابت m_چوٹی_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = خالی_اشاریہ;  // Current scope being visited

    std::سمتیہ<AstCFunc*> m_چوٹی_Funcps;  // Top level trace initialization functions
    std::سمتیہ<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::سیٹ<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ساخت Dtype_Func_کلید final {
        ثابت Ast_گرہ_DType* dtypep;
        VVarType varType;
        منطقی عامل==(ثابت Dtype_Func_کلید& other) ثابت {
            واپس dtypep == other.dtypep && varType == other.varType;
        }
    };
    ساخت Dtype_Func_کلید_Hash final {
        سائز_t عامل()(ثابت Dtype_Func_کلید& کلید) ثابت {
            واپس std::hash<ثابت Ast_گرہ_DType*>{}(کلید.dtypep)
                   ^ (std::hash<uint8_t>{}(کلید.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_کلید, AstCFunc*, Dtype_Func_کلید_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = خالی_اشاریہ;  // Current type func
    AstCFunc* m_dtypeSubFuncp = خالی_اشاریہ;  // Current type sub func
    ثابت string m_dtype_دلائل{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = خالی_اشاریہ;  // Current type func decl
    V3_Unique_نام m_dtype_نام{""};  // Unique names for dtype funcs
    منطقی m_skipDtypeFunc = جھوٹ;  // Don't create a type func
    uint32_t m_انحراف = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحیح m_چوٹی_Func_سائز = 0;  // Size of the top function currently being built
    صحیح m_sub_Func_سائز = 0;  // Size of the sub function currently being built
    منطقی m_force_New_برگ = جھوٹ;  // Force next addToSubFunc to start a fresh leaf
    سائز_t m_چوٹی_Scope_جڑ_Func_گنتی = 0;  // Top-scope init functions used only for wrapper IOs
    منطقی m_چوٹی_Scope_جڑ_Phase = جھوٹ;  // Emitting top-scope wrapper IO declarations
    ثابت صحیح m_func_سائز_حد_بندی  // Maximum size of a function
        = v3Global.opt.output_بانٹو_CTrace() ? v3Global.opt.output_بانٹو_CTrace()
                                           : std::numeric_limits<صحیح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::سمتیہ<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_راستہ_مقصد_Scopep;
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
    جماعت Trace_اندراج final {
        AstVarScope* m_vscp = خالی_اشاریہ;  // AstVarScope under scope being traced
        AstCell* m_cellp = خالی_اشاریہ;  // Sub scope (as AstCell) under scope being traced
        std::string m_راستہ;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        خالی init(ثابت std::string& name, Ast_گرہ* nodep, منطقی اندر_چوٹی_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_گرہ::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_راستہ = vcdName + " ";
                m_name.clear();
            } ورنہ اگر (varp->isPrimaryIO()) {
                // Primary IO variable
                m_راستہ = "$rootio ";
                m_name = vcdName;
            } ورنہ {
                // Other Variable
                ثابت سائز_t pos = vcdName.rfind(' ');
                ثابت سائز_t راستہ_Len = pos == std::string::npos ? 0 : pos + 1;
                m_راستہ = vcdName.substr(0, راستہ_Len);
                m_name = vcdName.substr(راستہ_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            اگر (اندر_چوٹی_Scope && !v3Global.opt.libCreate().خالی_خانے()) {
                ثابت سائز_t آغاز = m_راستہ.ڈھونڈو(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(آغاز != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string سابقہ = m_راستہ.substr(0, آغاز);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                اگر (سابقہ == "$rootio") واپس;
                m_راستہ = m_راستہ.substr(آغاز + 1);
                اگر (v3Global.rootp()->trace_Lib_چوٹی_Name() != سابقہ) m_راستہ = "$libroot " + m_راستہ;
            }
        }

    عوامی:
        صریح Trace_اندراج(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_چوٹی());
        }
        صریح Trace_اندراج(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_چوٹی());
        }
        صحیح operatorCompare(ثابت Trace_اندراج& b) ثابت {
            اگر (ثابت صحیح cmp = راستہ().compare(b.راستہ())) واپس cmp < 0;
            اگر (ثابت صحیح cmp = fileline().operatorCompare(b.fileline())) واپس cmp < 0;
            واپس name() < b.name();
        }
        AstVarScope* vscp() ثابت { واپس m_vscp; }
        AstCell* cellp() ثابت { واپس m_cellp; }
        ثابت std::string& راستہ() ثابت { واپس m_راستہ; }
        خالی راستہ(ثابت std::string& راستہ) { m_راستہ = راستہ; }
        ثابت std::string& name() ثابت { واپس m_name; }
        File_سطر& fileline() ثابت { واپس m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::سمتیہ<Trace_اندراج> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = خالی_اشاریہ;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_گرہ_اظہاریہ* m_traValuep = خالی_اشاریہ;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ثابت AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ثابت AstVar* ثابت varp = nodep->varp();
        اگر (!varp->isTrace()) واپس "Verilator trace_off";
        اگر (!nodep->isTrace()) واپس "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        اگر (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            واپس "Automatic variable";

        ثابت صحیح چوڑائی = recurse_DType_چوڑائی(nodep->varp()->dtypep());
        اگر (v3Global.opt.trace_Max_چوڑائی() && چوڑائی > v3Global.opt.trace_Max_چوڑائی())
            واپس "Width " + cvt_مقصد_Str(چوڑائی) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        اگر (!V3Control::getScopeTraceOn(prettyName)) واپس "Vlt scope trace_off";
        اگر (!v3Global.opt.traceUnderscore()) {
            اگر (!prettyName.خالی_خانے() && prettyName[0] == '_') واپس "Leading underscore";
            اگر (prettyName.ڈھونڈو("._") != string::npos) واپس "Inlined leading underscore";
        }
        واپس ""s;
    }

    صحیح recurse_DType_چوڑائی(ثابت Ast_گرہ_DType* nodep) {
        اگر (ثابت Ast_گرہ_Array_DType* adtypep = VN_CAST(nodep, گرہ_Array_DType))
            واپس recurse_DType_چوڑائی(adtypep->subDTypep()) * adtypep->declRange().elements();
        واپس nodep->چوڑائی();
    }

    AstCFunc* newCFunc(File_سطر* flp, ثابت string& name) {
        AstScope* ثابت چوٹی_Scopep = m_چوٹی_Scopep->scopep();
        AstCFunc* ثابت funcp = نیا AstCFunc{flp, name, چوٹی_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_بنیاد() + "* tracep");
        funcp->isTrace(سچ);
        funcp->isStatic(جھوٹ);
        funcp->isLoose(سچ);
        funcp->slow(سچ);
        چوٹی_Scopep->جوڑو_Blocksp(funcp);
        واپس funcp;
    }

    خالی جوڑو_مقصد_چوٹی_Func(Ast_گرہ_Stmt* stmtp) {
        اگر (m_چوٹی_Func_سائز > m_func_سائز_حد_بندی || m_چوٹی_Funcps.خالی_خانے()) {
            m_چوٹی_Func_سائز = 0;
            //
            ثابت string n = cvt_مقصد_Str(m_چوٹی_Funcps.سائز());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_چوٹی_Scopep->fileline(), name);
            m_چوٹی_Funcps.ڈالو_عقب(funcp);
        }
        m_چوٹی_Funcps.عقب()->جوڑو_Stmtsp(stmtp);
        m_چوٹی_Func_سائز += stmtp->گرہ_گنتی();
    }

    خالی جوڑو_مقصد_Sub_Func(Ast_گرہ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        اگر (m_dtypeSubFuncp) {
            اگر (m_sub_Func_سائز > m_func_سائز_حد_بندی) newDeclSubFunc();
            m_dtypeSubFuncp->جوڑو_Stmtsp(stmtp);
            m_sub_Func_سائز += stmtp->گرہ_گنتی();
            واپس;
        }
        // Defer trace splitting until V3Trace
        اگر (m_subFuncps.خالی_خانے() || m_force_New_برگ) {
            m_force_New_برگ = جھوٹ;
            File_سطر* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvt_مقصد_Str(m_subFuncps.سائز());
            ثابت string name
                = m_currScopep == m_چوٹی_Scopep->scopep() && !v3Global.opt.libCreate().خالی_خانے()
                      ? (m_چوٹی_Scope_جڑ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->جوڑو_Stmtsp(نیا AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ڈالو_عقب(funcp);
        }
        m_subFuncps.عقب()->جوڑو_Stmtsp(stmtp);
        m_sub_Func_سائز += stmtp->گرہ_گنتی();
    }

    AstTraceDecl* جوڑو_Trace_Decl(ثابت VNumRange& arrayRange,
                               صحیح چوڑائی_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = خالی_اشاریہ) {
        VNumRange ثنائی_ہندسہ_Range;
        اگر (چوڑائی_Override) {
            ثنائی_ہندسہ_Range = VNumRange{چوڑائی_Override - 1, 0};
        } ورنہ اگر (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            ثنائی_ہندسہ_Range = bdtypep->nrange();
        }
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گرہ_اظہاریہ* valuep = m_traValuep->clone_درخت(جھوٹ);
        ثابت منطقی valid_انحراف = m_انحراف != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = نیا AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               ثنائی_ہندسہ_Range, arrayRange, dtypeCallp,        valid_انحراف};
        اگر (valid_انحراف) {
            newp->رمز(m_انحراف);
            اگر (!dtypeCallp) { m_انحراف += newp->رمز_Inc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(نیا AstCExpr{flp, "__VdtypeVar", m_traVscp->چوڑائی()});
                VL_DO_DANGLING(refp->delete_درخت(), refp);
            });
        } ورنہ {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        جوڑو_مقصد_Sub_Func(newp);

        واپس newp;
    }

    خالی جوڑو_Ignore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        اگر (debug() > 3 && m_traVscp) std::خارج << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    خالی fixupPlaceholder(ثابت std::string& راستہ, Ast_گرہ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت خودکار it = m_راستہ_مقصد_Scopep.ڈھونڈو(Ast_گرہ::prettyName(راستہ));
        اگر (it != m_راستہ_مقصد_Scopep.انتہا()) {
            ثابت AstScope* ثابت scopep = it->دوسرا;
            File_سطر* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_گرہ* stmtp = خالی_اشاریہ;
            برائے (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = نیا AstCCall{flp, subFuncp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                stmtp = Ast_گرہ::جوڑو_اگلا(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            اگر (stmtp) placeholderp->جوڑو_اگلا_یہاں(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_عقب();
        VL_DO_DANGLING(placeholderp->delete_درخت(), placeholderp);
    }

    خالی fixupLibStub(ثابت std::string& راستہ, Ast_گرہ_Stmt* placeholderp) {
        File_سطر* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_گرہ::prettyName(راستہ);
        اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            ثابت std::string چوٹی_سابقہ = v3Global.rootp()->trace_Lib_چوٹی_Name() + ".";
            اگر (VString::startsWith(name, چوٹی_سابقہ)) name = name.substr(چوٹی_سابقہ.سائز());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = نیا AstCStmt{flp};
        initp->جوڑو("{\n");
        initp->جوڑو("std::string __VlibName = vlSymsp->name();\n");
        initp->جوڑو("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->جوڑو("__VlibName += ");
        initp->جوڑو(نیا AstConst{flp, AstConst::String{}, name});
        initp->جوڑو(";\n");
        initp->جوڑو("tracep->initLib(__VlibName);\n");
        initp->جوڑو("}\n");

        placeholderp->جوڑو_اگلا_یہاں(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_عقب()->delete_درخت(), placeholderp);
    }

    خالی fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        برائے (ثابت خودکار& شے_3 : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(شے_3);
            ثابت AstCell* ثابت cellp = std::get<1>(شے_3);
            Ast_گرہ_Stmt* ثابت placeholderp = std::get<2>(شے_3);
            ثابت std::string راستہ = parentp->name() + "__DOT__" + cellp->name();
            اگر (cellp->modp()->verilatorLib()) {
                fixupLibStub(راستہ, placeholderp);
            } ورنہ {
                fixupPlaceholder(راستہ, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        برائے (ثابت خودکار& شے_3 : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(شے_3);
            Ast_گرہ_Stmt* ثابت placeholderp = std::get<1>(شے_3);
            ثابت std::string راستہ = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(راستہ, placeholderp);
        }
    }

    خالی remove_Redundant_سابقہ_ڈالو_نکالو() {
        برائے (ثابت خودکار& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            برائے (AstCFunc* ثابت funcp : pair.دوسرا) {
                Ast_گرہ* prevp = خالی_اشاریہ;
                Ast_گرہ* currp = funcp->stmtsp();
                جب (currp) {
                    Ast_گرہ* ثابت nextp = currp->nextp();
                    اگر (VN_IS(prevp, Trace_ڈالو_سابقہ) && VN_IS(currp, Trace_نکالو_سابقہ)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_عقب()->delete_درخت(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_عقب()->delete_درخت(), currp);
                    }
                    اگر (!nextp) توڑو;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    خالی checkCalls(ثابت AstCFunc* funcp) {
        اگر (!v3Global.opt.debugCheck()) واپس;
        checkCallsRecurse(funcp);
        اگر (!m_declUncalledps.خالی_خانے()) {  // LCOV_EXCL_START
            برائے (خودکار tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    خالی checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([یہ](ثابت Ast_گرہ* nodep) {
            اگر (ثابت AstTraceDecl* ثابت declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } ورنہ اگر (ثابت AstCCall* ثابت ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    منطقی isBasicIO() { واپس m_traVscp->varp()->isVLIO(); }
    خالی newDeclSubFunc() {
        File_سطر* ثابت flp = m_dtypeFuncp->fileline();
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtype_نام.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_دلائل);
        AstCCall* ثابت subCallp = نیا AstCCall{flp, subFuncp};
        subCallp->dtype_سیٹ_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->جوڑو_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_سائز = 0;
    }
    خالی newDeclFunc(Ast_گرہ_DType* nodep) {
        Ast_گرہ_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_دلائل{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_کلید dtype_کلید{skipTypep, m_traVscp->varp()->varType()};
        خودکار pair = m_dtypeFuncs.emplace(dtype_کلید, خالی_اشاریہ);
        AstCFunc** funcpp = &pair.پہلا->دوسرا;
        اگر (pair.دوسرا) {
            *funcpp = newCFunc(flp, m_dtype_نام.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_دلائل);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = نیا AstCCall{flp, *funcpp};
        callp->dtype_سیٹ_Void();
        callp->argTypes(call_دلائل);
        m_dtypeDeclp = جوڑو_Trace_Decl(VNumRange{}, skipTypep->چوڑائی(), callp);
        جوڑو_مقصد_Sub_Func(callp->makeStmt());

        اگر (pair.دوسرا) {
            VL_RESTORER(m_انحراف);
            m_انحراف = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_سائز);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            اگر (AstStructDType* ثابت dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, سچ);
            } ورنہ اگر (AstUnpackArrayDType* ثابت dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, سچ);
            } ورنہ اگر (AstPackArrayDType* ثابت dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, سچ);
            } ورنہ {
                UASSERT_OBJ(جھوٹ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_انحراف);
        }

        m_dtypeDeclp->رمز_Inc((*funcpp)->user1());
        m_انحراف += m_dtypeDeclp->رمز_Inc();
    }
    خالی declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقی newFunc) {
        string سابقہ_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        جوڑو_مقصد_Sub_Func(نیا Ast_Trace_ڈالو_سابقہ{flp, سابقہ_Name, VTrace_سابقہ_Type::ARRAY_UNPACKED,
                                            nodep->بائیں(), nodep->دائیں(), !newFunc});

        اگر (VN_IS(nodep->subDTypep()->skip_Ref_مقصد_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_مقصد_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            اگر (m_traVscp->dtypep()->skip_Ref_مقصد_Enump()->isString()) {
                جوڑو_Ignore("Unsupported: strings");
            } ورنہ {
                m_traName = "";
                جوڑو_Trace_Decl(nodep->declRange(), 0);
            }
        } ورنہ {
            Ast_گرہ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_مقصد_Enump();
            // Always iterate left index to right index
            برائے (ثابت صحیح i : nodep->declRange().seq_بایاں_مقصد_دایاں()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::مقصد_string(i) + ']';
                m_traValuep = m_traValuep->clone_درخت(جھوٹ);
                m_traValuep = نیا AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
        }

        جوڑو_مقصد_Sub_Func(نیا Ast_Trace_نکالو_سابقہ{flp});
    }
    خالی declPackedArray(AstPackArrayDType* ثابت nodep, منطقی newFunc) {
        string سابقہ_Name(newFunc ? "name" : m_traName);
        Ast_گرہ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_مقصد_Enump();

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        جوڑو_مقصد_Sub_Func(نیا Ast_Trace_ڈالو_سابقہ{flp, سابقہ_Name, VTrace_سابقہ_Type::ARRAY_PACKED,
                                            nodep->بائیں(), nodep->دائیں(), !newFunc});

        // Always iterate left index to right index
        برائے (ثابت صحیح i : nodep->declRange().seq_بایاں_مقصد_دایاں()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::مقصد_string(i) + ']';
            ثابت صحیح lsb = (i - nodep->lo()) * subtypep->چوڑائی();
            m_traValuep = m_traValuep->clone_درخت(جھوٹ);
            m_traValuep = نیا AstSel{flp, m_traValuep, lsb, subtypep->چوڑائی()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_درخت(), m_traValuep = خالی_اشاریہ);
        }

        جوڑو_مقصد_Sub_Func(نیا Ast_Trace_نکالو_سابقہ{flp});
    }
    خالی declStruct(AstStructDType* ثابت nodep, منطقی newFunc) {
        File_سطر* ثابت flp = nodep->fileline();
        string سابقہ_Name(newFunc ? "name" : m_traName);
        صحیح nMembers = 0;
        برائے (Ast_گرہ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        اگر (!nodep->packed()) {
            جوڑو_مقصد_Sub_Func(نیا Ast_Trace_ڈالو_سابقہ{flp, سابقہ_Name,  //
                                                VTrace_سابقہ_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            برائے (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_مقصد_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(جھوٹ);
                m_traValuep = نیا AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            جوڑو_مقصد_Sub_Func(نیا Ast_Trace_نکالو_سابقہ{flp});
        } ورنہ {
            جوڑو_مقصد_Sub_Func(نیا Ast_Trace_ڈالو_سابقہ{flp, سابقہ_Name,  //
                                                VTrace_سابقہ_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            برائے (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_مقصد_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_درخت(جھوٹ);
                m_traValuep = نیا AstSel{flp, m_traValuep, itemp->lsb(), subtypep->چوڑائی()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
            }
            جوڑو_مقصد_Sub_Func(نیا Ast_Trace_نکالو_سابقہ{flp});
        }
    }

    // VISITORS
    خالی دورہ(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.خالی_خانے(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.خالی_خانے(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.خالی_خانے(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        اگر (nodep->modp()->verilatorLib()) واپس;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        برائے (Ast_گرہ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_عقب(nodep, cellp);
        }

        اگر (!m_entries.خالی_خانے()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_ترتیب_دو(
                m_entries.begin(), m_entries.انتہا(),
                [](ثابت Trace_اندراج& a, ثابت Trace_اندراج& b) { واپس a.operatorCompare(b); });

            File_سطر* ثابت flp = nodep->fileline();
            راستہ_Adjustor راستہ_Adjustor_2{flp, [&](Ast_گرہ_Stmt* stmtp) { جوڑو_مقصد_Sub_Func(stmtp); }};
            ثابت منطقی بانٹو_جڑ_Primary_Ios = nodep->is_چوٹی() && !v3Global.opt.libCreate().خالی_خانے();
            ثابت خودکار emit_اندراج = [&](ثابت Trace_اندراج& اندراج_2) {
                AstVarScope* ثابت vscp = اندراج_2.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << اندراج_2.راستہ() << "' name='" << اندراج_2.name() << "' "
                                  << (اندراج_2.cellp() ? static_cast<Ast_گرہ*>(اندراج_2.cellp())
                                                    : static_cast<Ast_گرہ*>(اندراج_2.vscp())));
                راستہ_Adjustor_2.adjust(اندراج_2.راستہ(), اندراج_2.cellp(), اندراج_2.vscp());

                m_traName = اندراج_2.name();

                اگر (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    اگر (!ignoreReason.خالی_خانے()) {
                        جوڑو_Ignore(ignoreReason);
                    } ورنہ {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = نیا AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_مقصد_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        اگر (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_درخت(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = خالی_اشاریہ;
                        }
                    }
                } ورنہ {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = اندراج_2.cellp();
                    Ast_گرہ_Stmt* ثابت stmtp = نیا AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    جوڑو_مقصد_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_عقب(nodep, cellp, stmtp);
                }
            };
            اگر (بانٹو_جڑ_Primary_Ios) {
                m_چوٹی_Scope_جڑ_Phase = سچ;
                برائے (ثابت Trace_اندراج& اندراج_2 : m_entries) {
                    AstVarScope* ثابت vscp = اندراج_2.vscp();
                    اگر (!(vscp && vscp->varp()->isPrimaryIO())) جاری;
                    emit_اندراج(اندراج_2);
                }
                m_چوٹی_Scope_جڑ_Phase = جھوٹ;
                راستہ_Adjustor_2.unwind();
                m_چوٹی_Scope_جڑ_Func_گنتی = m_subFuncps.سائز();
                اگر (m_چوٹی_Scope_جڑ_Func_گنتی) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_سائز = 0;
                    m_force_New_برگ = سچ;
                }
            }
            برائے (ثابت Trace_اندراج& اندراج_2 : m_entries) {
                AstVarScope* ثابت vscp = اندراج_2.vscp();
                اگر (بانٹو_جڑ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) جاری;
                emit_اندراج(اندراج_2);
            }
            راستہ_Adjustor_2.unwind();
            m_traVscp = خالی_اشاریہ;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string راستہ = nodep->prettyName();
        m_راستہ_مقصد_Scopep.emplace(راستہ, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        اگر (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت سائز_t آخری_Dot = راستہ.ڈھونڈو_آخری_کا('.');
            UASSERT_OBJ(آخری_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << راستہ);
            ثابت std::string والد_راستہ = راستہ.substr(0, آخری_Dot + 1);

            برائے (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                اگر (!VString::startsWith(refName, والد_راستہ)) جاری;

                // Save the mapping from the path of the reference to the scope
                m_راستہ_مقصد_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                اگر (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_عقب()->delete_درخت(), intfRefp);
                }
            }
        }
    }
    خالی دورہ(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        اگر (nodep->varp()->is_عارضی() && !nodep->varp()->isTrace()) واپس;
        اگر (nodep->varp()->isClassMember()) واپس;
        اگر (nodep->varp()->isFuncLocal()) واپس;

        // When creating a --lib-create library ...
        اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            // Ignore parameters in packages. These will be traced at the top level.
            اگر (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) واپس;
        }

        // Add to traced signal list
        m_entries.emplace_عقب(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    خالی دورہ(AstConstDType* nodep) override {
        اگر (!m_traVscp) واپس;
        VL_RESTORER(m_انحراف);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = سچ;
        iterate(nodep->subDTypep()->skip_Ref_مقصد_Enump());
    }
    خالی دورہ(AstRefDType* nodep) override {
        اگر (!m_traVscp) واپس;
        VL_RESTORER(m_انحراف);
        iterate(nodep->subDTypep()->skip_Ref_مقصد_Enump());
    }
    خالی دورہ(AstIfaceRefDType* /*nodep*/) override {
        اگر (!m_traVscp) واپس;
        // Insert a placeholder to be fixed up later
        File_سطر* ثابت flp = m_traVscp->fileline();
        Ast_گرہ_Stmt* ثابت stmtp
            = نیا AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        جوڑو_مقصد_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_عقب(m_traVscp, stmtp);
    }
    خالی دورہ(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        اگر (!m_traVscp) واپس;

        اگر (v3Global.opt.traceMaxArray()
            && static_cast<صحیح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            جوڑو_Ignore("Wide memory > --trace-max-array ents");
            واپس;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        اگر (VN_IS(nodep->subDTypep()->skip_Ref_مقصد_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_مقصد_Enump()->isString()) {
            جوڑو_Ignore("Unsupported: strings");
            واپس;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        اگر (isBasicIO()) m_skipDtypeFunc = سچ;

        اگر (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_انحراف);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, جھوٹ);
    }
    خالی دورہ(AstPackArrayDType* nodep) override {
        اگر (!m_traVscp) واپس;

        اگر (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
            واپس;
        }

        Ast_گرہ_DType* ثابت subtypep = nodep->subDTypep()->skip_Ref_مقصد_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        اگر (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            اگر (basicp->is_ثنائی_ہندسہ_Logic() && !basicp->isRanged()) {
                جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
                واپس;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        اگر (isBasicIO()) m_skipDtypeFunc = سچ;

        اگر (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_انحراف);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, جھوٹ);
    }
    خالی دورہ(AstStructDType* nodep) override {
        اگر (!m_traVscp) واپس;

        اگر (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
            واپس;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        اگر (isBasicIO()) m_skipDtypeFunc = سچ;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        اگر (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_انحراف);
            newDeclFunc(nodep);
        }
        declStruct(nodep, جھوٹ);
    }
    خالی دورہ(AstUnionDType* nodep) override {
        اگر (!m_traVscp) واپس;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = سچ;

        اگر (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            جوڑو_Trace_Decl(VNumRange{}, nodep->چوڑائی());
            واپس;
        }

        VL_RESTORER_COPY(m_traName);
        File_سطر* ثابت flp = nodep->fileline();

        صحیح nMembers = 0;
        برائے (Ast_گرہ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        اگر (!nodep->packed()) {
            جوڑو_Ignore("Unsupported: Unpacked union");
        } ورنہ {
            جوڑو_مقصد_Sub_Func(نیا Ast_Trace_ڈالو_سابقہ{flp, m_traName,  //
                                                VTrace_سابقہ_Type::UNION_PACKED, nMembers});
            برائے (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_گرہ_DType* ثابت subtypep = itemp->subDTypep()->skip_Ref_مقصد_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            جوڑو_مقصد_Sub_Func(نیا Ast_Trace_نکالو_سابقہ{flp});
        }
    }
    خالی دورہ(AstBasicDType* nodep) override {
        اگر (!m_traVscp) واپس;
        اگر (nodep->isString()) {
            جوڑو_Ignore("Unsupported: strings");
        } ورنہ {
            جوڑو_Trace_Decl(VNumRange{}, 0);
        }
    }
    خالی دورہ(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    خالی دورہ(Ast_گرہ_DType*) override {
        // Note more specific dtypes above
        اگر (!m_traVscp) واپس;
        جوڑو_Ignore("Unsupported: data type");
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح TraceDeclVisitor(AstNetlist* nodep)
        : m_چوٹی_Scopep{nodep->چوٹی_Scopep()} {
        File_سطر* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_اگلا_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.خالی_خانے(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_سابقہ_ڈالو_نکالو();

        ثابت std::سمتیہ<AstCFunc*>& چوٹی_Scope_Funcps = m_scopeInitFuncps.at(m_چوٹی_Scopep->scopep());
        AstCFunc* جڑ_Funcp = خالی_اشاریہ;
        اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            جڑ_Funcp = newCFunc(flp, "trace_init_root");
            جڑ_Funcp->اندراج_نقطہ(سچ);
            برائے (سائز_t i = 0; i < m_چوٹی_Scope_جڑ_Func_گنتی; ++i) {
                AstCCall* ثابت callp = نیا AstCCall{flp, چوٹی_Scope_Funcps.at(i)};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                جڑ_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
            اگر (!m_چوٹی_Scope_جڑ_Func_گنتی) جڑ_Funcp->جوڑو_Stmtsp(نیا AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        برائے (سائز_t i = m_چوٹی_Scope_جڑ_Func_گنتی; i < چوٹی_Scope_Funcps.سائز(); ++i) {
            AstCFunc* ثابت funcp = چوٹی_Scope_Funcps.at(i);
            AstCCall* ثابت callp = نیا AstCCall{flp, funcp};
            callp->dtype_سیٹ_Void();
            callp->argTypes("tracep");
            جوڑو_مقصد_چوٹی_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        اگر (m_چوٹی_Funcps.خالی_خانے()) جوڑو_مقصد_چوٹی_Func(نیا AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        اگر (m_چوٹی_Funcps.سائز() > 1) {
            AstCFunc* ثابت چوٹی_Funcp = newCFunc(flp, "");
            برائے (AstCFunc* funcp : m_چوٹی_Funcps) {
                AstCCall* ثابت callp = نیا AstCCall{flp, funcp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                چوٹی_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
            m_چوٹی_Funcps.clear();
            m_چوٹی_Funcps.ڈالو_عقب(چوٹی_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت چوٹی_Funcp = m_چوٹی_Funcps.front();
        چوٹی_Funcp->name("trace_init_top");
        چوٹی_Funcp->اندراج_نقطہ(سچ);

        اگر (جڑ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(جڑ_Funcp);
        checkCalls(چوٹی_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::جوڑو_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::جوڑو_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

خالی V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("tracedecl", 0, dump_درخت_Either_Level() >= 3);
}
