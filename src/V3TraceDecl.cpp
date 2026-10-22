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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3TraceDecl.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Number.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <cstdint>
#అన్తర్భావ <functional>
#అన్తర్భావ <limits>
#అన్తర్భావ <tuple>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

తరగతి మార్గం_Adjustor final {
    File_పంక్తి* స్థిరరాశి m_flp;  // FileLine used for created nodes
    std::function<శూన్య(Ast_కణుపు_Stmt*)> m_emit;  // Function called with adjustment statements
    std::సదిశ<std::string> m_దొంతర{""};  // Stack of current paths

    స్థిర constexpr అక్షరం SEPARATOR = ' ';

బహిరంగ:
    స్పష్టమైన మార్గం_Adjustor(File_పంక్తి* flp, std::function<శూన్య(Ast_కణుపు_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    శూన్య adjust(స్థిరరాశి string& new_మార్గం, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        అయితే (!VString::startsWith(new_మార్గం, m_దొంతర.వెనుక())) {
            m_emit(కొత్త Ast_Trace_తీయి_ఉపసర్గ{m_flp});
            m_దొంతర.తీయి_వెనుక();
        }

        ఒకవేళ (new_మార్గం == m_దొంతర.వెనుక()) తిరిగి;

        స్థిరరాశి VTrace_ఉపసర్గ_Type చివరి_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_ఉపసర్గ_Type::SCOPE_INTERFACE
                : VTrace_ఉపసర్గ_Type::SCOPE_MODULE;
        స్థిరరాశి std::string extra_ఉపసర్గ = new_మార్గం.substr(m_దొంతర.వెనుక().కొలత());
        కొలత_t begin = 0;
        స్థిరరాశి కొలత_t చివరి = extra_ఉపసర్గ.rfind(SEPARATOR);
        // Move down, one path element at a time
        అయితే (నిజం) {
            స్థిరరాశి కొలత_t చివర = extra_ఉపసర్గ.కనుగొను(SEPARATOR, begin);
            ఒకవేళ (చివర == string::npos) ఆపు;
            స్థిరరాశి string& extra = extra_ఉపసర్గ.substr(begin, చివర - begin);
            ఒకవేళ (చివర == చివరి) {
                m_emit(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{m_flp, extra, చివరి_Scope_Type});
            } లేకపోతే {
                m_emit(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{m_flp, extra, VTrace_ఉపసర్గ_Type::SCOPE_MODULE});
            }
            m_దొంతర.నెట్టు_వెనుక(m_దొంతర.వెనుక() + extra + SEPARATOR);
            begin = చివర + 1;
        }
        UASSERT(begin == extra_ఉపసర్గ.కొలత(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    శూన్య unwind() {
        అయితే (m_దొంతర.కొలత() > 1) {
            m_emit(కొత్త Ast_Trace_తీయి_ఉపసర్గ{m_flp});
            m_దొంతర.తీయి_వెనుక();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

తరగతి TraceDeclVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_శిఖరం_Scope* స్థిరరాశి m_శిఖరం_Scopep;  // The singleton AstTopScope
    స్థిరరాశి AstScope* m_currScopep = శూన్యసూచిక;  // Current scope being visited

    std::సదిశ<AstCFunc*> m_శిఖరం_Funcps;  // Top level trace initialization functions
    std::సదిశ<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::సమితి<స్థిరరాశి AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    నిర్మాణం Dtype_Func_కీలకం final {
        స్థిరరాశి Ast_కణుపు_DType* dtypep;
        VVarType varType;
        తార్కిక క్రియాకారకం==(స్థిరరాశి Dtype_Func_కీలకం& other) స్థిరరాశి {
            తిరిగి dtypep == other.dtypep && varType == other.varType;
        }
    };
    నిర్మాణం Dtype_Func_కీలకం_Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dtype_Func_కీలకం& కీలకం) స్థిరరాశి {
            తిరిగి std::hash<స్థిరరాశి Ast_కణుపు_DType*>{}(కీలకం.dtypep)
                   ^ (std::hash<uint8_t>{}(కీలకం.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_కీలకం, AstCFunc*, Dtype_Func_కీలకం_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = శూన్యసూచిక;  // Current type func
    AstCFunc* m_dtypeSubFuncp = శూన్యసూచిక;  // Current type sub func
    స్థిరరాశి string m_dtype_వాదనలు{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = శూన్యసూచిక;  // Current type func decl
    V3_Unique_పేర్లు m_dtype_పేర్లు{""};  // Unique names for dtype funcs
    తార్కిక m_skipDtypeFunc = అబద్ధం;  // Don't create a type func
    uint32_t m_స్థానభ్రంశం = std::numeric_limits<uint32_t>::max();  // Offset for types
    పూర్ణాంకం m_శిఖరం_Func_కొలత = 0;  // Size of the top function currently being built
    పూర్ణాంకం m_sub_Func_కొలత = 0;  // Size of the sub function currently being built
    తార్కిక m_force_New_ఆకు = అబద్ధం;  // Force next addToSubFunc to start a fresh leaf
    కొలత_t m_శిఖరం_Scope_మూలం_Func_లెక్క = 0;  // Top-scope init functions used only for wrapper IOs
    తార్కిక m_శిఖరం_Scope_మూలం_Phase = అబద్ధం;  // Emitting top-scope wrapper IO declarations
    స్థిరరాశి పూర్ణాంకం m_func_కొలత_హద్దు  // Maximum size of a function
        = v3Global.opt.output_విభజించు_CTrace() ? v3Global.opt.output_విభజించు_CTrace()
                                           : std::numeric_limits<పూర్ణాంకం>::max();
    // Trace init functions to for each scope
    std::unordered_map<స్థిరరాశి AstScope*, std::సదిశ<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, స్థిరరాశి AstScope*> m_మార్గం_గమ్యం_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::సదిశ<std::tuple<AstScope*, AstCell*, Ast_కణుపు_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::సదిశ<std::tuple<AstVarScope*, Ast_కణుపు_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    తరగతి Trace_నమోదు final {
        AstVarScope* m_vscp = శూన్యసూచిక;  // AstVarScope under scope being traced
        AstCell* m_cellp = శూన్యసూచిక;  // Sub scope (as AstCell) under scope being traced
        std::string m_మార్గం;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        శూన్య init(స్థిరరాశి std::string& name, Ast_కణుపు* nodep, తార్కిక లోపల_శిఖరం_Scope) {
            // Compute path in hierarchy and item name
            స్థిరరాశి std::string& vcdName = Ast_కణుపు::vcdName(name);
            AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
            ఒకవేళ (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_మార్గం = vcdName + " ";
                m_name.clear();
            } లేకపోతే ఒకవేళ (varp->isPrimaryIO()) {
                // Primary IO variable
                m_మార్గం = "$rootio ";
                m_name = vcdName;
            } లేకపోతే {
                // Other Variable
                స్థిరరాశి కొలత_t pos = vcdName.rfind(' ');
                స్థిరరాశి కొలత_t మార్గం_Len = pos == std::string::npos ? 0 : pos + 1;
                m_మార్గం = vcdName.substr(0, మార్గం_Len);
                m_name = vcdName.substr(మార్గం_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            ఒకవేళ (లోపల_శిఖరం_Scope && !v3Global.opt.libCreate().ఖాళీలు()) {
                స్థిరరాశి కొలత_t ప్రారంభం = m_మార్గం.కనుగొను(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(ప్రారంభం != std::string::npos, nodep, "No prefix with --lib-create");
                స్థిరరాశి std::string ఉపసర్గ = m_మార్గం.substr(0, ప్రారంభం);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                ఒకవేళ (ఉపసర్గ == "$rootio") తిరిగి;
                m_మార్గం = m_మార్గం.substr(ప్రారంభం + 1);
                ఒకవేళ (v3Global.rootp()->trace_Lib_శిఖరం_Name() != ఉపసర్గ) m_మార్గం = "$libroot " + m_మార్గం;
            }
        }

    బహిరంగ:
        స్పష్టమైన Trace_నమోదు(స్థిరరాశి AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_శిఖరం());
        }
        స్పష్టమైన Trace_నమోదు(స్థిరరాశి AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_శిఖరం());
        }
        పూర్ణాంకం operatorCompare(స్థిరరాశి Trace_నమోదు& b) స్థిరరాశి {
            ఒకవేళ (స్థిరరాశి పూర్ణాంకం cmp = మార్గం().compare(b.మార్గం())) తిరిగి cmp < 0;
            ఒకవేళ (స్థిరరాశి పూర్ణాంకం cmp = fileline().operatorCompare(b.fileline())) తిరిగి cmp < 0;
            తిరిగి name() < b.name();
        }
        AstVarScope* vscp() స్థిరరాశి { తిరిగి m_vscp; }
        AstCell* cellp() స్థిరరాశి { తిరిగి m_cellp; }
        స్థిరరాశి std::string& మార్గం() స్థిరరాశి { తిరిగి m_మార్గం; }
        శూన్య మార్గం(స్థిరరాశి std::string& మార్గం) { m_మార్గం = మార్గం; }
        స్థిరరాశి std::string& name() స్థిరరాశి { తిరిగి m_name; }
        File_పంక్తి& fileline() స్థిరరాశి { తిరిగి m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::సదిశ<Trace_నమోదు> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = శూన్యసూచిక;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_కణుపు_సమాసం* m_traValuep = శూన్యసూచిక;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(స్థిరరాశి AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        స్థిరరాశి AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (!varp->isTrace()) తిరిగి "Verilator trace_off";
        ఒకవేళ (!nodep->isTrace()) తిరిగి "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        ఒకవేళ (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            తిరిగి "Automatic variable";

        స్థిరరాశి పూర్ణాంకం వెడల్పు = recurse_DType_వెడల్పు(nodep->varp()->dtypep());
        ఒకవేళ (v3Global.opt.trace_Max_వెడల్పు() && వెడల్పు > v3Global.opt.trace_Max_వెడల్పు())
            తిరిగి "Width " + cvt_గమ్యం_Str(వెడల్పు) + " > --trace-max-width";

        స్థిరరాశి string prettyName = nodep->prettyName();
        ఒకవేళ (!V3Control::getScopeTraceOn(prettyName)) తిరిగి "Vlt scope trace_off";
        ఒకవేళ (!v3Global.opt.traceUnderscore()) {
            ఒకవేళ (!prettyName.ఖాళీలు() && prettyName[0] == '_') తిరిగి "Leading underscore";
            ఒకవేళ (prettyName.కనుగొను("._") != string::npos) తిరిగి "Inlined leading underscore";
        }
        తిరిగి ""s;
    }

    పూర్ణాంకం recurse_DType_వెడల్పు(స్థిరరాశి Ast_కణుపు_DType* nodep) {
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* adtypep = VN_CAST(nodep, కణుపు_Array_DType))
            తిరిగి recurse_DType_వెడల్పు(adtypep->subDTypep()) * adtypep->declRange().elements();
        తిరిగి nodep->వెడల్పు();
    }

    AstCFunc* newCFunc(File_పంక్తి* flp, స్థిరరాశి string& name) {
        AstScope* స్థిరరాశి శిఖరం_Scopep = m_శిఖరం_Scopep->scopep();
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{flp, name, శిఖరం_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_పునాది() + "* tracep");
        funcp->isTrace(నిజం);
        funcp->isStatic(అబద్ధం);
        funcp->isLoose(నిజం);
        funcp->slow(నిజం);
        శిఖరం_Scopep->కూడు_Blocksp(funcp);
        తిరిగి funcp;
    }

    శూన్య కూడు_గమ్యం_శిఖరం_Func(Ast_కణుపు_Stmt* stmtp) {
        ఒకవేళ (m_శిఖరం_Func_కొలత > m_func_కొలత_హద్దు || m_శిఖరం_Funcps.ఖాళీలు()) {
            m_శిఖరం_Func_కొలత = 0;
            //
            స్థిరరాశి string n = cvt_గమ్యం_Str(m_శిఖరం_Funcps.కొలత());
            స్థిరరాశి string name{"trace_init_top__" + n};
            AstCFunc* స్థిరరాశి funcp = newCFunc(m_శిఖరం_Scopep->fileline(), name);
            m_శిఖరం_Funcps.నెట్టు_వెనుక(funcp);
        }
        m_శిఖరం_Funcps.వెనుక()->కూడు_Stmtsp(stmtp);
        m_శిఖరం_Func_కొలత += stmtp->కణుపు_లెక్క();
    }

    శూన్య కూడు_గమ్యం_Sub_Func(Ast_కణుపు_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        ఒకవేళ (m_dtypeSubFuncp) {
            ఒకవేళ (m_sub_Func_కొలత > m_func_కొలత_హద్దు) newDeclSubFunc();
            m_dtypeSubFuncp->కూడు_Stmtsp(stmtp);
            m_sub_Func_కొలత += stmtp->కణుపు_లెక్క();
            తిరిగి;
        }
        // Defer trace splitting until V3Trace
        ఒకవేళ (m_subFuncps.ఖాళీలు() || m_force_New_ఆకు) {
            m_force_New_ఆకు = అబద్ధం;
            File_పంక్తి* స్థిరరాశి flp = m_currScopep->fileline();
            స్థిరరాశి string n = cvt_గమ్యం_Str(m_subFuncps.కొలత());
            స్థిరరాశి string name
                = m_currScopep == m_శిఖరం_Scopep->scopep() && !v3Global.opt.libCreate().ఖాళీలు()
                      ? (m_శిఖరం_Scope_మూలం_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* స్థిరరాశి funcp = newCFunc(flp, name);
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.నెట్టు_వెనుక(funcp);
        }
        m_subFuncps.వెనుక()->కూడు_Stmtsp(stmtp);
        m_sub_Func_కొలత += stmtp->కణుపు_లెక్క();
    }

    AstTraceDecl* కూడు_Trace_Decl(స్థిరరాశి VNumRange& arrayRange,
                               పూర్ణాంకం వెడల్పు_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* స్థిరరాశి dtypeCallp = శూన్యసూచిక) {
        VNumRange బిట్టు_Range;
        ఒకవేళ (వెడల్పు_Override) {
            బిట్టు_Range = VNumRange{వెడల్పు_Override - 1, 0};
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = m_traValuep->dtypep()->basicp()) {
            బిట్టు_Range = bdtypep->nrange();
        }
        File_పంక్తి* స్థిరరాశి flp = m_traVscp->fileline();
        Ast_కణుపు_సమాసం* valuep = m_traValuep->clone_వృక్షం(అబద్ధం);
        స్థిరరాశి తార్కిక valid_స్థానభ్రంశం = m_స్థానభ్రంశం != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* స్థిరరాశి newp
            = కొత్త AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               బిట్టు_Range, arrayRange, dtypeCallp,        valid_స్థానభ్రంశం};
        ఒకవేళ (valid_స్థానభ్రంశం) {
            newp->సంకేతం_2(m_స్థానభ్రంశం);
            ఒకవేళ (!dtypeCallp) { m_స్థానభ్రంశం += newp->సంకేతం_Inc(); }
            valuep->foreach([&](AstVarRef* స్థిరరాశి refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(కొత్త AstCExpr{flp, "__VdtypeVar", m_traVscp->వెడల్పు()});
                VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
            });
        } లేకపోతే {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        కూడు_గమ్యం_Sub_Func(newp);

        తిరిగి newp;
    }

    శూన్య కూడు_Ignore(స్థిరరాశి string& why) {
        ++m_statIgnSigs;
        స్థిరరాశి std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        ఒకవేళ (debug() > 3 && m_traVscp) std::బయటకు << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    శూన్య fixupPlaceholder(స్థిరరాశి std::string& మార్గం, Ast_కణుపు_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        స్థిరరాశి స్వయంచాలక it = m_మార్గం_గమ్యం_Scopep.కనుగొను(Ast_కణుపు::prettyName(మార్గం));
        ఒకవేళ (it != m_మార్గం_గమ్యం_Scopep.చివర()) {
            స్థిరరాశి AstScope* స్థిరరాశి scopep = it->రెండవ;
            File_పంక్తి* స్థిరరాశి flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_కణుపు* stmtp = శూన్యసూచిక;
            కొరకు (AstCFunc* స్థిరరాశి subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, subFuncp};
                callp->dtype_సమితి_Void();
                callp->argTypes("tracep");
                stmtp = Ast_కణుపు::కూడు_తదుపరి(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            ఒకవేళ (stmtp) placeholderp->కూడు_తదుపరి_ఇక్కడ(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_వెనుక();
        VL_DO_DANGLING(placeholderp->delete_వృక్షం(), placeholderp);
    }

    శూన్య fixupLibStub(స్థిరరాశి std::string& మార్గం, Ast_కణుపు_Stmt* placeholderp) {
        File_పంక్తి* స్థిరరాశి flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_కణుపు::prettyName(మార్గం);
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            స్థిరరాశి std::string శిఖరం_ఉపసర్గ = v3Global.rootp()->trace_Lib_శిఖరం_Name() + ".";
            ఒకవేళ (VString::startsWith(name, శిఖరం_ఉపసర్గ)) name = name.substr(శిఖరం_ఉపసర్గ.కొలత());
        }

        // Call the initialization function for the library instance
        AstCStmt* స్థిరరాశి initp = కొత్త AstCStmt{flp};
        initp->కూడు("{\n");
        initp->కూడు("std::string __VlibName = vlSymsp->name();\n");
        initp->కూడు("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->కూడు("__VlibName += ");
        initp->కూడు(కొత్త AstConst{flp, AstConst::String{}, name});
        initp->కూడు(";\n");
        initp->కూడు("tracep->initLib(__VlibName);\n");
        initp->కూడు("}\n");

        placeholderp->కూడు_తదుపరి_ఇక్కడ(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_వెనుక()->delete_వృక్షం(), placeholderp);
    }

    శూన్య fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        కొరకు (స్థిరరాశి స్వయంచాలక& అంశం_2 : m_cellInitPlaceholders) {
            స్థిరరాశి AstScope* స్థిరరాశి parentp = std::get<0>(అంశం_2);
            స్థిరరాశి AstCell* స్థిరరాశి cellp = std::get<1>(అంశం_2);
            Ast_కణుపు_Stmt* స్థిరరాశి placeholderp = std::get<2>(అంశం_2);
            స్థిరరాశి std::string మార్గం = parentp->name() + "__DOT__" + cellp->name();
            ఒకవేళ (cellp->modp()->verilatorLib()) {
                fixupLibStub(మార్గం, placeholderp);
            } లేకపోతే {
                fixupPlaceholder(మార్గం, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        కొరకు (స్థిరరాశి స్వయంచాలక& అంశం_2 : m_ifaceRefInitPlaceholders) {
            స్థిరరాశి AstVarScope* స్థిరరాశి vscp = std::get<0>(అంశం_2);
            Ast_కణుపు_Stmt* స్థిరరాశి placeholderp = std::get<1>(అంశం_2);
            స్థిరరాశి std::string మార్గం = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(మార్గం, placeholderp);
        }
    }

    శూన్య remove_Redundant_ఉపసర్గ_నెట్టు_తీయి() {
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            కొరకు (AstCFunc* స్థిరరాశి funcp : pair.రెండవ) {
                Ast_కణుపు* prevp = శూన్యసూచిక;
                Ast_కణుపు* currp = funcp->stmtsp();
                అయితే (currp) {
                    Ast_కణుపు* స్థిరరాశి nextp = currp->nextp();
                    ఒకవేళ (VN_IS(prevp, Trace_నెట్టు_ఉపసర్గ) && VN_IS(currp, Trace_తీయి_ఉపసర్గ)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_వెనుక()->delete_వృక్షం(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_వెనుక()->delete_వృక్షం(), currp);
                    }
                    ఒకవేళ (!nextp) ఆపు;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    శూన్య checkCalls(స్థిరరాశి AstCFunc* funcp) {
        ఒకవేళ (!v3Global.opt.debugCheck()) తిరిగి;
        checkCallsRecurse(funcp);
        ఒకవేళ (!m_declUncalledps.ఖాళీలు()) {  // LCOV_EXCL_START
            కొరకు (స్వయంచాలక tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    శూన్య checkCallsRecurse(స్థిరరాశి AstCFunc* funcp) {
        funcp->foreach([ఇది](స్థిరరాశి Ast_కణుపు* nodep) {
            ఒకవేళ (స్థిరరాశి AstTraceDecl* స్థిరరాశి declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstCCall* స్థిరరాశి ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    తార్కిక isBasicIO() { తిరిగి m_traVscp->varp()->isVLIO(); }
    శూన్య newDeclSubFunc() {
        File_పంక్తి* స్థిరరాశి flp = m_dtypeFuncp->fileline();
        AstCFunc* స్థిరరాశి subFuncp = newCFunc(flp, m_dtype_పేర్లు.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_వాదనలు);
        AstCCall* స్థిరరాశి subCallp = కొత్త AstCCall{flp, subFuncp};
        subCallp->dtype_సమితి_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->కూడు_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_కొలత = 0;
    }
    శూన్య newDeclFunc(Ast_కణుపు_DType* nodep) {
        Ast_కణుపు_DType* స్థిరరాశి skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_వాదనలు{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_పంక్తి* స్థిరరాశి flp = skipTypep->fileline();

        స్థిరరాశి Dtype_Func_కీలకం dtype_కీలకం{skipTypep, m_traVscp->varp()->varType()};
        స్వయంచాలక pair = m_dtypeFuncs.emplace(dtype_కీలకం, శూన్యసూచిక);
        AstCFunc** funcpp = &pair.మొదటి->రెండవ;
        ఒకవేళ (pair.రెండవ) {
            *funcpp = newCFunc(flp, m_dtype_పేర్లు.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_వాదనలు);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, *funcpp};
        callp->dtype_సమితి_Void();
        callp->argTypes(call_వాదనలు);
        m_dtypeDeclp = కూడు_Trace_Decl(VNumRange{}, skipTypep->వెడల్పు(), callp);
        కూడు_గమ్యం_Sub_Func(callp->makeStmt());

        ఒకవేళ (pair.రెండవ) {
            VL_RESTORER(m_స్థానభ్రంశం);
            m_స్థానభ్రంశం = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_కొలత);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            ఒకవేళ (AstStructDType* స్థిరరాశి dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, నిజం);
            } లేకపోతే ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, నిజం);
            } లేకపోతే ఒకవేళ (AstPackArrayDType* స్థిరరాశి dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, నిజం);
            } లేకపోతే {
                UASSERT_OBJ(అబద్ధం, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_స్థానభ్రంశం);
        }

        m_dtypeDeclp->సంకేతం_Inc((*funcpp)->user1());
        m_స్థానభ్రంశం += m_dtypeDeclp->సంకేతం_Inc();
    }
    శూన్య declUnpackedArray(AstUnpackArrayDType* స్థిరరాశి nodep, తార్కిక newFunc) {
        string ఉపసర్గ_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{flp, ఉపసర్గ_Name, VTrace_ఉపసర్గ_Type::ARRAY_UNPACKED,
                                            nodep->ఎడమ(), nodep->కుడి(), !newFunc});

        ఒకవేళ (VN_IS(nodep->subDTypep()->skip_Ref_గమ్యం_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_గమ్యం_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            ఒకవేళ (m_traVscp->dtypep()->skip_Ref_గమ్యం_Enump()->isString()) {
                కూడు_Ignore("Unsupported: strings");
            } లేకపోతే {
                m_traName = "";
                కూడు_Trace_Decl(nodep->declRange(), 0);
            }
        } లేకపోతే {
            Ast_కణుపు_DType* స్థిరరాశి subtypep = nodep->subDTypep()->skip_Ref_గమ్యం_Enump();
            // Always iterate left index to right index
            కొరకు (స్థిరరాశి పూర్ణాంకం i : nodep->declRange().seq_ఎడమ_గమ్యం_కుడి()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::గమ్యం_string(i) + ']';
                m_traValuep = m_traValuep->clone_వృక్షం(అబద్ధం);
                m_traValuep = కొత్త AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_వృక్షం(), m_traValuep);
            }
        }

        కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_తీయి_ఉపసర్గ{flp});
    }
    శూన్య declPackedArray(AstPackArrayDType* స్థిరరాశి nodep, తార్కిక newFunc) {
        string ఉపసర్గ_Name(newFunc ? "name" : m_traName);
        Ast_కణుపు_DType* స్థిరరాశి subtypep = nodep->subDTypep()->skip_Ref_గమ్యం_Enump();

        VL_RESTORER_COPY(m_traName);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{flp, ఉపసర్గ_Name, VTrace_ఉపసర్గ_Type::ARRAY_PACKED,
                                            nodep->ఎడమ(), nodep->కుడి(), !newFunc});

        // Always iterate left index to right index
        కొరకు (స్థిరరాశి పూర్ణాంకం i : nodep->declRange().seq_ఎడమ_గమ్యం_కుడి()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::గమ్యం_string(i) + ']';
            స్థిరరాశి పూర్ణాంకం lsb = (i - nodep->lo()) * subtypep->వెడల్పు();
            m_traValuep = m_traValuep->clone_వృక్షం(అబద్ధం);
            m_traValuep = కొత్త AstSel{flp, m_traValuep, lsb, subtypep->వెడల్పు()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_వృక్షం(), m_traValuep = శూన్యసూచిక);
        }

        కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_తీయి_ఉపసర్గ{flp});
    }
    శూన్య declStruct(AstStructDType* స్థిరరాశి nodep, తార్కిక newFunc) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        string ఉపసర్గ_Name(newFunc ? "name" : m_traName);
        పూర్ణాంకం nMembers = 0;
        కొరకు (Ast_కణుపు* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        ఒకవేళ (!nodep->packed()) {
            కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{flp, ఉపసర్గ_Name,  //
                                                VTrace_ఉపసర్గ_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            కొరకు (స్థిరరాశి AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_కణుపు_DType* స్థిరరాశి subtypep = itemp->subDTypep()->skip_Ref_గమ్యం_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_వృక్షం(అబద్ధం);
                m_traValuep = కొత్త AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_వృక్షం(), m_traValuep);
            }
            కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_తీయి_ఉపసర్గ{flp});
        } లేకపోతే {
            కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{flp, ఉపసర్గ_Name,  //
                                                VTrace_ఉపసర్గ_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            కొరకు (స్థిరరాశి AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_కణుపు_DType* స్థిరరాశి subtypep = itemp->subDTypep()->skip_Ref_గమ్యం_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_వృక్షం(అబద్ధం);
                m_traValuep = కొత్త AstSel{flp, m_traValuep, itemp->lsb(), subtypep->వెడల్పు()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_వృక్షం(), m_traValuep);
            }
            కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_తీయి_ఉపసర్గ{flp});
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.ఖాళీలు(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.ఖాళీలు(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.ఖాళీలు(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        ఒకవేళ (nodep->modp()->verilatorLib()) తిరిగి;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        కొరకు (Ast_కణుపు* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstCell* స్థిరరాశి cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_వెనుక(nodep, cellp);
        }

        ఒకవేళ (!m_entries.ఖాళీలు()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_క్రమీకరించు(
                m_entries.begin(), m_entries.చివర(),
                [](స్థిరరాశి Trace_నమోదు& a, స్థిరరాశి Trace_నమోదు& b) { తిరిగి a.operatorCompare(b); });

            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            మార్గం_Adjustor మార్గం_Adjustor_2{flp, [&](Ast_కణుపు_Stmt* stmtp) { కూడు_గమ్యం_Sub_Func(stmtp); }};
            స్థిరరాశి తార్కిక విభజించు_మూలం_Primary_Ios = nodep->is_శిఖరం() && !v3Global.opt.libCreate().ఖాళీలు();
            స్థిరరాశి స్వయంచాలక emit_నమోదు = [&](స్థిరరాశి Trace_నమోదు& నమోదు_2) {
                AstVarScope* స్థిరరాశి vscp = నమోదు_2.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << నమోదు_2.మార్గం() << "' name='" << నమోదు_2.name() << "' "
                                  << (నమోదు_2.cellp() ? static_cast<Ast_కణుపు*>(నమోదు_2.cellp())
                                                    : static_cast<Ast_కణుపు*>(నమోదు_2.vscp())));
                మార్గం_Adjustor_2.adjust(నమోదు_2.మార్గం(), నమోదు_2.cellp(), నమోదు_2.vscp());

                m_traName = నమోదు_2.name();

                ఒకవేళ (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    స్థిరరాశి string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    ఒకవేళ (!ignoreReason.ఖాళీలు()) {
                        కూడు_Ignore(ignoreReason);
                    } లేకపోతే {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = కొత్త AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_గమ్యం_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        ఒకవేళ (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_వృక్షం(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = శూన్యసూచిక;
                        }
                    }
                } లేకపోతే {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* స్థిరరాశి cellp = నమోదు_2.cellp();
                    Ast_కణుపు_Stmt* స్థిరరాశి stmtp = కొత్త AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    కూడు_గమ్యం_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_వెనుక(nodep, cellp, stmtp);
                }
            };
            ఒకవేళ (విభజించు_మూలం_Primary_Ios) {
                m_శిఖరం_Scope_మూలం_Phase = నిజం;
                కొరకు (స్థిరరాశి Trace_నమోదు& నమోదు_2 : m_entries) {
                    AstVarScope* స్థిరరాశి vscp = నమోదు_2.vscp();
                    ఒకవేళ (!(vscp && vscp->varp()->isPrimaryIO())) కొనసాగు;
                    emit_నమోదు(నమోదు_2);
                }
                m_శిఖరం_Scope_మూలం_Phase = అబద్ధం;
                మార్గం_Adjustor_2.unwind();
                m_శిఖరం_Scope_మూలం_Func_లెక్క = m_subFuncps.కొలత();
                ఒకవేళ (m_శిఖరం_Scope_మూలం_Func_లెక్క) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_కొలత = 0;
                    m_force_New_ఆకు = నిజం;
                }
            }
            కొరకు (స్థిరరాశి Trace_నమోదు& నమోదు_2 : m_entries) {
                AstVarScope* స్థిరరాశి vscp = నమోదు_2.vscp();
                ఒకవేళ (విభజించు_మూలం_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) కొనసాగు;
                emit_నమోదు(నమోదు_2);
            }
            మార్గం_Adjustor_2.unwind();
            m_traVscp = శూన్యసూచిక;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        స్థిరరాశి std::string మార్గం = nodep->prettyName();
        m_మార్గం_గమ్యం_Scopep.emplace(మార్గం, nodep);

        // Save the hierarchical names of interface references that reference this scope
        స్థిరరాశి AstCell* స్థిరరాశి cellp = nodep->aboveCellp();
        ఒకవేళ (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            స్థిరరాశి కొలత_t చివరి_Dot = మార్గం.కనుగొను_చివరి_యొక్క('.');
            UASSERT_OBJ(చివరి_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << మార్గం);
            స్థిరరాశి std::string మాతృ_మార్గం = మార్గం.substr(0, చివరి_Dot + 1);

            కొరకు (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                స్థిరరాశి std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                ఒకవేళ (!VString::startsWith(refName, మాతృ_మార్గం)) కొనసాగు;

                // Save the mapping from the path of the reference to the scope
                m_మార్గం_గమ్యం_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                ఒకవేళ (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_వెనుక()->delete_వృక్షం(), intfRefp);
                }
            }
        }
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        ఒకవేళ (nodep->varp()->is_తాత్కాలిక() && !nodep->varp()->isTrace()) తిరిగి;
        ఒకవేళ (nodep->varp()->isClassMember()) తిరిగి;
        ఒకవేళ (nodep->varp()->isFuncLocal()) తిరిగి;

        // When creating a --lib-create library ...
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            // Ignore parameters in packages. These will be traced at the top level.
            ఒకవేళ (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) తిరిగి;
        }

        // Add to traced signal list
        m_entries.emplace_వెనుక(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    శూన్య సందర్శించు(AstConstDType* nodep) override {
        ఒకవేళ (!m_traVscp) తిరిగి;
        VL_RESTORER(m_స్థానభ్రంశం);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = నిజం;
        iterate(nodep->subDTypep()->skip_Ref_గమ్యం_Enump());
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        ఒకవేళ (!m_traVscp) తిరిగి;
        VL_RESTORER(m_స్థానభ్రంశం);
        iterate(nodep->subDTypep()->skip_Ref_గమ్యం_Enump());
    }
    శూన్య సందర్శించు(AstIfaceRefDType* /*nodep*/) override {
        ఒకవేళ (!m_traVscp) తిరిగి;
        // Insert a placeholder to be fixed up later
        File_పంక్తి* స్థిరరాశి flp = m_traVscp->fileline();
        Ast_కణుపు_Stmt* స్థిరరాశి stmtp
            = కొత్త AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        కూడు_గమ్యం_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_వెనుక(m_traVscp, stmtp);
    }
    శూన్య సందర్శించు(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        ఒకవేళ (!m_traVscp) తిరిగి;

        ఒకవేళ (v3Global.opt.traceMaxArray()
            && static_cast<పూర్ణాంకం>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            కూడు_Ignore("Wide memory > --trace-max-array ents");
            తిరిగి;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        ఒకవేళ (VN_IS(nodep->subDTypep()->skip_Ref_గమ్యం_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_గమ్యం_Enump()->isString()) {
            కూడు_Ignore("Unsupported: strings");
            తిరిగి;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ఒకవేళ (isBasicIO()) m_skipDtypeFunc = నిజం;

        ఒకవేళ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_స్థానభ్రంశం);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, అబద్ధం);
    }
    శూన్య సందర్శించు(AstPackArrayDType* nodep) override {
        ఒకవేళ (!m_traVscp) తిరిగి;

        ఒకవేళ (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            కూడు_Trace_Decl(VNumRange{}, nodep->వెడల్పు());
            తిరిగి;
        }

        Ast_కణుపు_DType* స్థిరరాశి subtypep = nodep->subDTypep()->skip_Ref_గమ్యం_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        ఒకవేళ (AstBasicDType* స్థిరరాశి basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            ఒకవేళ (basicp->is_బిట్టు_Logic() && !basicp->isRanged()) {
                కూడు_Trace_Decl(VNumRange{}, nodep->వెడల్పు());
                తిరిగి;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ఒకవేళ (isBasicIO()) m_skipDtypeFunc = నిజం;

        ఒకవేళ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_స్థానభ్రంశం);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, అబద్ధం);
    }
    శూన్య సందర్శించు(AstStructDType* nodep) override {
        ఒకవేళ (!m_traVscp) తిరిగి;

        ఒకవేళ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            కూడు_Trace_Decl(VNumRange{}, nodep->వెడల్పు());
            తిరిగి;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ఒకవేళ (isBasicIO()) m_skipDtypeFunc = నిజం;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        ఒకవేళ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_స్థానభ్రంశం);
            newDeclFunc(nodep);
        }
        declStruct(nodep, అబద్ధం);
    }
    శూన్య సందర్శించు(AstUnionDType* nodep) override {
        ఒకవేళ (!m_traVscp) తిరిగి;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = నిజం;

        ఒకవేళ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            కూడు_Trace_Decl(VNumRange{}, nodep->వెడల్పు());
            తిరిగి;
        }

        VL_RESTORER_COPY(m_traName);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        పూర్ణాంకం nMembers = 0;
        కొరకు (Ast_కణుపు* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        ఒకవేళ (!nodep->packed()) {
            కూడు_Ignore("Unsupported: Unpacked union");
        } లేకపోతే {
            కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_నెట్టు_ఉపసర్గ{flp, m_traName,  //
                                                VTrace_ఉపసర్గ_Type::UNION_PACKED, nMembers});
            కొరకు (స్థిరరాశి AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_కణుపు_DType* స్థిరరాశి subtypep = itemp->subDTypep()->skip_Ref_గమ్యం_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            కూడు_గమ్యం_Sub_Func(కొత్త Ast_Trace_తీయి_ఉపసర్గ{flp});
        }
    }
    శూన్య సందర్శించు(AstBasicDType* nodep) override {
        ఒకవేళ (!m_traVscp) తిరిగి;
        ఒకవేళ (nodep->isString()) {
            కూడు_Ignore("Unsupported: strings");
        } లేకపోతే {
            కూడు_Trace_Decl(VNumRange{}, 0);
        }
    }
    శూన్య సందర్శించు(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    శూన్య సందర్శించు(Ast_కణుపు_DType*) override {
        // Note more specific dtypes above
        ఒకవేళ (!m_traVscp) తిరిగి;
        కూడు_Ignore("Unsupported: data type");
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TraceDeclVisitor(AstNetlist* nodep)
        : m_శిఖరం_Scopep{nodep->శిఖరం_Scopep()} {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_తదుపరి_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.ఖాళీలు(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_ఉపసర్గ_నెట్టు_తీయి();

        స్థిరరాశి std::సదిశ<AstCFunc*>& శిఖరం_Scope_Funcps = m_scopeInitFuncps.at(m_శిఖరం_Scopep->scopep());
        AstCFunc* మూలం_Funcp = శూన్యసూచిక;
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            మూలం_Funcp = newCFunc(flp, "trace_init_root");
            మూలం_Funcp->నమోదు_బిందువు(నిజం);
            కొరకు (కొలత_t i = 0; i < m_శిఖరం_Scope_మూలం_Func_లెక్క; ++i) {
                AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, శిఖరం_Scope_Funcps.at(i)};
                callp->dtype_సమితి_Void();
                callp->argTypes("tracep");
                మూలం_Funcp->కూడు_Stmtsp(callp->makeStmt());
            }
            ఒకవేళ (!m_శిఖరం_Scope_మూలం_Func_లెక్క) మూలం_Funcp->కూడు_Stmtsp(కొత్త AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        కొరకు (కొలత_t i = m_శిఖరం_Scope_మూలం_Func_లెక్క; i < శిఖరం_Scope_Funcps.కొలత(); ++i) {
            AstCFunc* స్థిరరాశి funcp = శిఖరం_Scope_Funcps.at(i);
            AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp};
            callp->dtype_సమితి_Void();
            callp->argTypes("tracep");
            కూడు_గమ్యం_శిఖరం_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        ఒకవేళ (m_శిఖరం_Funcps.ఖాళీలు()) కూడు_గమ్యం_శిఖరం_Func(కొత్త AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        ఒకవేళ (m_శిఖరం_Funcps.కొలత() > 1) {
            AstCFunc* స్థిరరాశి శిఖరం_Funcp = newCFunc(flp, "");
            కొరకు (AstCFunc* funcp : m_శిఖరం_Funcps) {
                AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp};
                callp->dtype_సమితి_Void();
                callp->argTypes("tracep");
                శిఖరం_Funcp->కూడు_Stmtsp(callp->makeStmt());
            }
            m_శిఖరం_Funcps.clear();
            m_శిఖరం_Funcps.నెట్టు_వెనుక(శిఖరం_Funcp);
        }

        // Set name of top level function
        AstCFunc* స్థిరరాశి శిఖరం_Funcp = m_శిఖరం_Funcps.front();
        శిఖరం_Funcp->name("trace_init_top");
        శిఖరం_Funcp->నమోదు_బిందువు(నిజం);

        ఒకవేళ (మూలం_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(మూలం_Funcp);
        checkCalls(శిఖరం_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::కూడు_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::కూడు_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

శూన్య V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("tracedecl", 0, dump_వృక్షం_Either_Level() >= 3);
}
