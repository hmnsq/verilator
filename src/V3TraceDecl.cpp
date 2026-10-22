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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3TraceDecl.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Number.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstdint>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

ਵਰਗ ਰਸਤਾ_Adjustor final {
    File_ਸਤਰ* ਅਚਲ m_flp;  // FileLine used for created nodes
    std::function<ਖਾਲੀ(Ast_ਗੰਢ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::ਸਦਿਸ਼<std::string> m_ਢੇਰ{""};  // Stack of current paths

    ਸਥਿਰ constexpr ਅੱਖਰ SEPARATOR = ' ';

ਜਨਤਕ:
    ਸਪਸ਼ਟ ਰਸਤਾ_Adjustor(File_ਸਤਰ* flp, std::function<ਖਾਲੀ(Ast_ਗੰਢ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    ਖਾਲੀ adjust(ਅਚਲ string& new_ਰਸਤਾ, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        ਜਦੋਂ (!VString::startsWith(new_ਰਸਤਾ, m_ਢੇਰ.ਪਿੱਠ())) {
            m_emit(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{m_flp});
            m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
        }

        ਜੇ (new_ਰਸਤਾ == m_ਢੇਰ.ਪਿੱਠ()) ਵਾਪਸ;

        ਅਚਲ VTrace_ਅਗੇਤਰ_Type ਆਖਰੀ_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_ਅਗੇਤਰ_Type::SCOPE_INTERFACE
                : VTrace_ਅਗੇਤਰ_Type::SCOPE_MODULE;
        ਅਚਲ std::string extra_ਅਗੇਤਰ = new_ਰਸਤਾ.substr(m_ਢੇਰ.ਪਿੱਠ().ਮਾਪ());
        ਮਾਪ_t begin = 0;
        ਅਚਲ ਮਾਪ_t ਆਖਰੀ = extra_ਅਗੇਤਰ.rfind(SEPARATOR);
        // Move down, one path element at a time
        ਜਦੋਂ (ਸੱਚ) {
            ਅਚਲ ਮਾਪ_t ਸਿਰਾ = extra_ਅਗੇਤਰ.ਲੱਭੋ(SEPARATOR, begin);
            ਜੇ (ਸਿਰਾ == string::npos) ਰੋਕੋ;
            ਅਚਲ string& extra = extra_ਅਗੇਤਰ.substr(begin, ਸਿਰਾ - begin);
            ਜੇ (ਸਿਰਾ == ਆਖਰੀ) {
                m_emit(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{m_flp, extra, ਆਖਰੀ_Scope_Type});
            } ਨਹੀਂਤਾਂ {
                m_emit(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{m_flp, extra, VTrace_ਅਗੇਤਰ_Type::SCOPE_MODULE});
            }
            m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(m_ਢੇਰ.ਪਿੱਠ() + extra + SEPARATOR);
            begin = ਸਿਰਾ + 1;
        }
        UASSERT(begin == extra_ਅਗੇਤਰ.ਮਾਪ(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    ਖਾਲੀ unwind() {
        ਜਦੋਂ (m_ਢੇਰ.ਮਾਪ() > 1) {
            m_emit(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{m_flp});
            m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

ਵਰਗ TraceDeclVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_ਸਿਖਰ_Scope* ਅਚਲ m_ਸਿਖਰ_Scopep;  // The singleton AstTopScope
    ਅਚਲ AstScope* m_currScopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope being visited

    std::ਸਦਿਸ਼<AstCFunc*> m_ਸਿਖਰ_Funcps;  // Top level trace initialization functions
    std::ਸਦਿਸ਼<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::ਸੈੱਟ<ਅਚਲ AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ਬਣਤਰ Dtype_Func_ਕੁੰਜੀ final {
        ਅਚਲ Ast_ਗੰਢ_DType* dtypep;
        VVarType varType;
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ==(ਅਚਲ Dtype_Func_ਕੁੰਜੀ& other) ਅਚਲ {
            ਵਾਪਸ dtypep == other.dtypep && varType == other.varType;
        }
    };
    ਬਣਤਰ Dtype_Func_ਕੁੰਜੀ_Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dtype_Func_ਕੁੰਜੀ& ਕੁੰਜੀ) ਅਚਲ {
            ਵਾਪਸ std::hash<ਅਚਲ Ast_ਗੰਢ_DType*>{}(ਕੁੰਜੀ.dtypep)
                   ^ (std::hash<uint8_t>{}(ਕੁੰਜੀ.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ਕੁੰਜੀ, AstCFunc*, Dtype_Func_ਕੁੰਜੀ_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current type func
    AstCFunc* m_dtypeSubFuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current type sub func
    ਅਚਲ string m_dtype_ਦਲੀਲਾਂ{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = ਖਾਲੀ_ਸੂਚਕ;  // Current type func decl
    V3_Unique_ਨਾਂ m_dtype_ਨਾਂ{""};  // Unique names for dtype funcs
    ਤਾਰਕਿਕ m_skipDtypeFunc = ਝੂਠ;  // Don't create a type func
    uint32_t m_ਵਿੱਥ = std::numeric_limits<uint32_t>::max();  // Offset for types
    ਪੂਰਨ m_ਸਿਖਰ_Func_ਮਾਪ = 0;  // Size of the top function currently being built
    ਪੂਰਨ m_sub_Func_ਮਾਪ = 0;  // Size of the sub function currently being built
    ਤਾਰਕਿਕ m_force_New_ਪੱਤੀ = ਝੂਠ;  // Force next addToSubFunc to start a fresh leaf
    ਮਾਪ_t m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Func_ਗਿਣਤੀ = 0;  // Top-scope init functions used only for wrapper IOs
    ਤਾਰਕਿਕ m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Phase = ਝੂਠ;  // Emitting top-scope wrapper IO declarations
    ਅਚਲ ਪੂਰਨ m_func_ਮਾਪ_ਹੱਦਬੰਦੀ  // Maximum size of a function
        = v3Global.opt.output_ਵੰਡੋ_CTrace() ? v3Global.opt.output_ਵੰਡੋ_CTrace()
                                           : std::numeric_limits<ਪੂਰਨ>::max();
    // Trace init functions to for each scope
    std::unordered_map<ਅਚਲ AstScope*, std::ਸਦਿਸ਼<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ਅਚਲ AstScope*> m_ਰਸਤਾ_ਮੰਜ਼ਿਲ_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::ਸਦਿਸ਼<std::tuple<AstScope*, AstCell*, Ast_ਗੰਢ_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::ਸਦਿਸ਼<std::tuple<AstVarScope*, Ast_ਗੰਢ_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    ਵਰਗ Trace_ਇੰਦਰਾਜ final {
        AstVarScope* m_vscp = ਖਾਲੀ_ਸੂਚਕ;  // AstVarScope under scope being traced
        AstCell* m_cellp = ਖਾਲੀ_ਸੂਚਕ;  // Sub scope (as AstCell) under scope being traced
        std::string m_ਰਸਤਾ;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        ਖਾਲੀ init(ਅਚਲ std::string& name, Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ ਵਿੱਚ_ਸਿਖਰ_Scope) {
            // Compute path in hierarchy and item name
            ਅਚਲ std::string& vcdName = Ast_ਗੰਢ::vcdName(name);
            AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
            ਜੇ (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_ਰਸਤਾ = vcdName + " ";
                m_name.clear();
            } ਨਹੀਂਤਾਂ ਜੇ (varp->isPrimaryIO()) {
                // Primary IO variable
                m_ਰਸਤਾ = "$rootio ";
                m_name = vcdName;
            } ਨਹੀਂਤਾਂ {
                // Other Variable
                ਅਚਲ ਮਾਪ_t pos = vcdName.rfind(' ');
                ਅਚਲ ਮਾਪ_t ਰਸਤਾ_Len = pos == std::string::npos ? 0 : pos + 1;
                m_ਰਸਤਾ = vcdName.substr(0, ਰਸਤਾ_Len);
                m_name = vcdName.substr(ਰਸਤਾ_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            ਜੇ (ਵਿੱਚ_ਸਿਖਰ_Scope && !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
                ਅਚਲ ਮਾਪ_t ਸ਼ੁਰੂ = m_ਰਸਤਾ.ਲੱਭੋ(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(ਸ਼ੁਰੂ != std::string::npos, nodep, "No prefix with --lib-create");
                ਅਚਲ std::string ਅਗੇਤਰ = m_ਰਸਤਾ.substr(0, ਸ਼ੁਰੂ);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                ਜੇ (ਅਗੇਤਰ == "$rootio") ਵਾਪਸ;
                m_ਰਸਤਾ = m_ਰਸਤਾ.substr(ਸ਼ੁਰੂ + 1);
                ਜੇ (v3Global.rootp()->trace_Lib_ਸਿਖਰ_Name() != ਅਗੇਤਰ) m_ਰਸਤਾ = "$libroot " + m_ਰਸਤਾ;
            }
        }

    ਜਨਤਕ:
        ਸਪਸ਼ਟ Trace_ਇੰਦਰਾਜ(ਅਚਲ AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_ਸਿਖਰ());
        }
        ਸਪਸ਼ਟ Trace_ਇੰਦਰਾਜ(ਅਚਲ AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_ਸਿਖਰ());
        }
        ਪੂਰਨ operatorCompare(ਅਚਲ Trace_ਇੰਦਰਾਜ& b) ਅਚਲ {
            ਜੇ (ਅਚਲ ਪੂਰਨ cmp = ਰਸਤਾ().compare(b.ਰਸਤਾ())) ਵਾਪਸ cmp < 0;
            ਜੇ (ਅਚਲ ਪੂਰਨ cmp = fileline().operatorCompare(b.fileline())) ਵਾਪਸ cmp < 0;
            ਵਾਪਸ name() < b.name();
        }
        AstVarScope* vscp() ਅਚਲ { ਵਾਪਸ m_vscp; }
        AstCell* cellp() ਅਚਲ { ਵਾਪਸ m_cellp; }
        ਅਚਲ std::string& ਰਸਤਾ() ਅਚਲ { ਵਾਪਸ m_ਰਸਤਾ; }
        ਖਾਲੀ ਰਸਤਾ(ਅਚਲ std::string& ਰਸਤਾ) { m_ਰਸਤਾ = ਰਸਤਾ; }
        ਅਚਲ std::string& name() ਅਚਲ { ਵਾਪਸ m_name; }
        File_ਸਤਰ& fileline() ਅਚਲ { ਵਾਪਸ m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::ਸਦਿਸ਼<Trace_ਇੰਦਰਾਜ> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = ਖਾਲੀ_ਸੂਚਕ;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_traValuep = ਖਾਲੀ_ਸੂਚਕ;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ਅਚਲ AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ਅਚਲ AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (!varp->isTrace()) ਵਾਪਸ "Verilator trace_off";
        ਜੇ (!nodep->isTrace()) ਵਾਪਸ "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        ਜੇ (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            ਵਾਪਸ "Automatic variable";

        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = recurse_DType_ਚੌੜਾਈ(nodep->varp()->dtypep());
        ਜੇ (v3Global.opt.trace_Max_ਚੌੜਾਈ() && ਚੌੜਾਈ > v3Global.opt.trace_Max_ਚੌੜਾਈ())
            ਵਾਪਸ "Width " + cvt_ਮੰਜ਼ਿਲ_Str(ਚੌੜਾਈ) + " > --trace-max-width";

        ਅਚਲ string prettyName = nodep->prettyName();
        ਜੇ (!V3Control::getScopeTraceOn(prettyName)) ਵਾਪਸ "Vlt scope trace_off";
        ਜੇ (!v3Global.opt.traceUnderscore()) {
            ਜੇ (!prettyName.ਖਾਲੀ_ਥਾਵਾਂ() && prettyName[0] == '_') ਵਾਪਸ "Leading underscore";
            ਜੇ (prettyName.ਲੱਭੋ("._") != string::npos) ਵਾਪਸ "Inlined leading underscore";
        }
        ਵਾਪਸ ""s;
    }

    ਪੂਰਨ recurse_DType_ਚੌੜਾਈ(ਅਚਲ Ast_ਗੰਢ_DType* nodep) {
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* adtypep = VN_CAST(nodep, ਗੰਢ_Array_DType))
            ਵਾਪਸ recurse_DType_ਚੌੜਾਈ(adtypep->subDTypep()) * adtypep->declRange().elements();
        ਵਾਪਸ nodep->ਚੌੜਾਈ();
    }

    AstCFunc* newCFunc(File_ਸਤਰ* flp, ਅਚਲ string& name) {
        AstScope* ਅਚਲ ਸਿਖਰ_Scopep = m_ਸਿਖਰ_Scopep->scopep();
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{flp, name, ਸਿਖਰ_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep");
        funcp->isTrace(ਸੱਚ);
        funcp->isStatic(ਝੂਠ);
        funcp->isLoose(ਸੱਚ);
        funcp->slow(ਸੱਚ);
        ਸਿਖਰ_Scopep->ਜੋੜੋ_Blocksp(funcp);
        ਵਾਪਸ funcp;
    }

    ਖਾਲੀ ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਸਿਖਰ_Func(Ast_ਗੰਢ_Stmt* stmtp) {
        ਜੇ (m_ਸਿਖਰ_Func_ਮਾਪ > m_func_ਮਾਪ_ਹੱਦਬੰਦੀ || m_ਸਿਖਰ_Funcps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_ਸਿਖਰ_Func_ਮਾਪ = 0;
            //
            ਅਚਲ string n = cvt_ਮੰਜ਼ਿਲ_Str(m_ਸਿਖਰ_Funcps.ਮਾਪ());
            ਅਚਲ string name{"trace_init_top__" + n};
            AstCFunc* ਅਚਲ funcp = newCFunc(m_ਸਿਖਰ_Scopep->fileline(), name);
            m_ਸਿਖਰ_Funcps.ਧੱਕੋ_ਪਿੱਠ(funcp);
        }
        m_ਸਿਖਰ_Funcps.ਪਿੱਠ()->ਜੋੜੋ_Stmtsp(stmtp);
        m_ਸਿਖਰ_Func_ਮਾਪ += stmtp->ਗੰਢ_ਗਿਣਤੀ();
    }

    ਖਾਲੀ ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(Ast_ਗੰਢ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        ਜੇ (m_dtypeSubFuncp) {
            ਜੇ (m_sub_Func_ਮਾਪ > m_func_ਮਾਪ_ਹੱਦਬੰਦੀ) newDeclSubFunc();
            m_dtypeSubFuncp->ਜੋੜੋ_Stmtsp(stmtp);
            m_sub_Func_ਮਾਪ += stmtp->ਗੰਢ_ਗਿਣਤੀ();
            ਵਾਪਸ;
        }
        // Defer trace splitting until V3Trace
        ਜੇ (m_subFuncps.ਖਾਲੀ_ਥਾਵਾਂ() || m_force_New_ਪੱਤੀ) {
            m_force_New_ਪੱਤੀ = ਝੂਠ;
            File_ਸਤਰ* ਅਚਲ flp = m_currScopep->fileline();
            ਅਚਲ string n = cvt_ਮੰਜ਼ਿਲ_Str(m_subFuncps.ਮਾਪ());
            ਅਚਲ string name
                = m_currScopep == m_ਸਿਖਰ_Scopep->scopep() && !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()
                      ? (m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ਅਚਲ funcp = newCFunc(flp, name);
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ਧੱਕੋ_ਪਿੱਠ(funcp);
        }
        m_subFuncps.ਪਿੱਠ()->ਜੋੜੋ_Stmtsp(stmtp);
        m_sub_Func_ਮਾਪ += stmtp->ਗੰਢ_ਗਿਣਤੀ();
    }

    AstTraceDecl* ਜੋੜੋ_Trace_Decl(ਅਚਲ VNumRange& arrayRange,
                               ਪੂਰਨ ਚੌੜਾਈ_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ਅਚਲ dtypeCallp = ਖਾਲੀ_ਸੂਚਕ) {
        VNumRange ਦੋਆਧਾਰੀ_ਅੰਕ_Range;
        ਜੇ (ਚੌੜਾਈ_Override) {
            ਦੋਆਧਾਰੀ_ਅੰਕ_Range = VNumRange{ਚੌੜਾਈ_Override - 1, 0};
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = m_traValuep->dtypep()->basicp()) {
            ਦੋਆਧਾਰੀ_ਅੰਕ_Range = bdtypep->nrange();
        }
        File_ਸਤਰ* ਅਚਲ flp = m_traVscp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = m_traValuep->clone_ਰੁੱਖ(ਝੂਠ);
        ਅਚਲ ਤਾਰਕਿਕ valid_ਵਿੱਥ = m_ਵਿੱਥ != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ਅਚਲ newp
            = ਨਵਾਂ AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               ਦੋਆਧਾਰੀ_ਅੰਕ_Range, arrayRange, dtypeCallp,        valid_ਵਿੱਥ};
        ਜੇ (valid_ਵਿੱਥ) {
            newp->ਕੋਡ(m_ਵਿੱਥ);
            ਜੇ (!dtypeCallp) { m_ਵਿੱਥ += newp->ਕੋਡ_Inc(); }
            valuep->foreach([&](AstVarRef* ਅਚਲ refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(ਨਵਾਂ AstCExpr{flp, "__VdtypeVar", m_traVscp->ਚੌੜਾਈ()});
                VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
            });
        } ਨਹੀਂਤਾਂ {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(newp);

        ਵਾਪਸ newp;
    }

    ਖਾਲੀ ਜੋੜੋ_Ignore(ਅਚਲ string& why) {
        ++m_statIgnSigs;
        ਅਚਲ std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        ਜੇ (debug() > 3 && m_traVscp) std::ਬਾਹਰ << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    ਖਾਲੀ fixupPlaceholder(ਅਚਲ std::string& ਰਸਤਾ, Ast_ਗੰਢ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਰਸਤਾ_ਮੰਜ਼ਿਲ_Scopep.ਲੱਭੋ(Ast_ਗੰਢ::prettyName(ਰਸਤਾ));
        ਜੇ (it != m_ਰਸਤਾ_ਮੰਜ਼ਿਲ_Scopep.ਸਿਰਾ()) {
            ਅਚਲ AstScope* ਅਚਲ scopep = it->ਦੂਜਾ;
            File_ਸਤਰ* ਅਚਲ flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ਗੰਢ* stmtp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (AstCFunc* ਅਚਲ subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, subFuncp};
                callp->dtype_ਸੈੱਟ_Void();
                callp->argTypes("tracep");
                stmtp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            ਜੇ (stmtp) placeholderp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(placeholderp->delete_ਰੁੱਖ(), placeholderp);
    }

    ਖਾਲੀ fixupLibStub(ਅਚਲ std::string& ਰਸਤਾ, Ast_ਗੰਢ_Stmt* placeholderp) {
        File_ਸਤਰ* ਅਚਲ flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ਗੰਢ::prettyName(ਰਸਤਾ);
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ std::string ਸਿਖਰ_ਅਗੇਤਰ = v3Global.rootp()->trace_Lib_ਸਿਖਰ_Name() + ".";
            ਜੇ (VString::startsWith(name, ਸਿਖਰ_ਅਗੇਤਰ)) name = name.substr(ਸਿਖਰ_ਅਗੇਤਰ.ਮਾਪ());
        }

        // Call the initialization function for the library instance
        AstCStmt* ਅਚਲ initp = ਨਵਾਂ AstCStmt{flp};
        initp->ਜੋੜੋ("{\n");
        initp->ਜੋੜੋ("std::string __VlibName = vlSymsp->name();\n");
        initp->ਜੋੜੋ("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->ਜੋੜੋ("__VlibName += ");
        initp->ਜੋੜੋ(ਨਵਾਂ AstConst{flp, AstConst::String{}, name});
        initp->ਜੋੜੋ(";\n");
        initp->ਜੋੜੋ("tracep->initLib(__VlibName);\n");
        initp->ਜੋੜੋ("}\n");

        placeholderp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), placeholderp);
    }

    ਖਾਲੀ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਸਤੂ_3 : m_cellInitPlaceholders) {
            ਅਚਲ AstScope* ਅਚਲ parentp = std::get<0>(ਵਸਤੂ_3);
            ਅਚਲ AstCell* ਅਚਲ cellp = std::get<1>(ਵਸਤੂ_3);
            Ast_ਗੰਢ_Stmt* ਅਚਲ placeholderp = std::get<2>(ਵਸਤੂ_3);
            ਅਚਲ std::string ਰਸਤਾ = parentp->name() + "__DOT__" + cellp->name();
            ਜੇ (cellp->modp()->verilatorLib()) {
                fixupLibStub(ਰਸਤਾ, placeholderp);
            } ਨਹੀਂਤਾਂ {
                fixupPlaceholder(ਰਸਤਾ, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਸਤੂ_3 : m_ifaceRefInitPlaceholders) {
            ਅਚਲ AstVarScope* ਅਚਲ vscp = std::get<0>(ਵਸਤੂ_3);
            Ast_ਗੰਢ_Stmt* ਅਚਲ placeholderp = std::get<1>(ਵਸਤੂ_3);
            ਅਚਲ std::string ਰਸਤਾ = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(ਰਸਤਾ, placeholderp);
        }
    }

    ਖਾਲੀ remove_Redundant_ਅਗੇਤਰ_ਧੱਕੋ_ਕੱਢੋ() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            ਲਈ (AstCFunc* ਅਚਲ funcp : pair.ਦੂਜਾ) {
                Ast_ਗੰਢ* prevp = ਖਾਲੀ_ਸੂਚਕ;
                Ast_ਗੰਢ* currp = funcp->stmtsp();
                ਜਦੋਂ (currp) {
                    Ast_ਗੰਢ* ਅਚਲ nextp = currp->nextp();
                    ਜੇ (VN_IS(prevp, Trace_ਧੱਕੋ_ਅਗੇਤਰ) && VN_IS(currp, Trace_ਕੱਢੋ_ਅਗੇਤਰ)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), currp);
                    }
                    ਜੇ (!nextp) ਰੋਕੋ;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    ਖਾਲੀ checkCalls(ਅਚਲ AstCFunc* funcp) {
        ਜੇ (!v3Global.opt.debugCheck()) ਵਾਪਸ;
        checkCallsRecurse(funcp);
        ਜੇ (!m_declUncalledps.ਖਾਲੀ_ਥਾਵਾਂ()) {  // LCOV_EXCL_START
            ਲਈ (ਸਵੈਚਾਲਿਤ tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    ਖਾਲੀ checkCallsRecurse(ਅਚਲ AstCFunc* funcp) {
        funcp->foreach([ਇਹ](ਅਚਲ Ast_ਗੰਢ* nodep) {
            ਜੇ (ਅਚਲ AstTraceDecl* ਅਚਲ declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstCCall* ਅਚਲ ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    ਤਾਰਕਿਕ isBasicIO() { ਵਾਪਸ m_traVscp->varp()->isVLIO(); }
    ਖਾਲੀ newDeclSubFunc() {
        File_ਸਤਰ* ਅਚਲ flp = m_dtypeFuncp->fileline();
        AstCFunc* ਅਚਲ subFuncp = newCFunc(flp, m_dtype_ਨਾਂ.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_ਦਲੀਲਾਂ);
        AstCCall* ਅਚਲ subCallp = ਨਵਾਂ AstCCall{flp, subFuncp};
        subCallp->dtype_ਸੈੱਟ_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->ਜੋੜੋ_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_ਮਾਪ = 0;
    }
    ਖਾਲੀ newDeclFunc(Ast_ਗੰਢ_DType* nodep) {
        Ast_ਗੰਢ_DType* ਅਚਲ skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_ਦਲੀਲਾਂ{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_ਸਤਰ* ਅਚਲ flp = skipTypep->fileline();

        ਅਚਲ Dtype_Func_ਕੁੰਜੀ dtype_ਕੁੰਜੀ{skipTypep, m_traVscp->varp()->varType()};
        ਸਵੈਚਾਲਿਤ pair = m_dtypeFuncs.emplace(dtype_ਕੁੰਜੀ, ਖਾਲੀ_ਸੂਚਕ);
        AstCFunc** funcpp = &pair.ਪਹਿਲਾ->ਦੂਜਾ;
        ਜੇ (pair.ਦੂਜਾ) {
            *funcpp = newCFunc(flp, m_dtype_ਨਾਂ.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_ਦਲੀਲਾਂ);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, *funcpp};
        callp->dtype_ਸੈੱਟ_Void();
        callp->argTypes(call_ਦਲੀਲਾਂ);
        m_dtypeDeclp = ਜੋੜੋ_Trace_Decl(VNumRange{}, skipTypep->ਚੌੜਾਈ(), callp);
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(callp->makeStmt());

        ਜੇ (pair.ਦੂਜਾ) {
            VL_RESTORER(m_ਵਿੱਥ);
            m_ਵਿੱਥ = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_ਮਾਪ);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            ਜੇ (AstStructDType* ਅਚਲ dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (AstUnpackArrayDType* ਅਚਲ dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (AstPackArrayDType* ਅਚਲ dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, ਸੱਚ);
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(ਝੂਠ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_ਵਿੱਥ);
        }

        m_dtypeDeclp->ਕੋਡ_Inc((*funcpp)->user1());
        m_ਵਿੱਥ += m_dtypeDeclp->ਕੋਡ_Inc();
    }
    ਖਾਲੀ declUnpackedArray(AstUnpackArrayDType* ਅਚਲ nodep, ਤਾਰਕਿਕ newFunc) {
        string ਅਗੇਤਰ_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{flp, ਅਗੇਤਰ_Name, VTrace_ਅਗੇਤਰ_Type::ARRAY_UNPACKED,
                                            nodep->ਖੱਬੇ(), nodep->ਸੱਜੇ(), !newFunc});

        ਜੇ (VN_IS(nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            ਜੇ (m_traVscp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump()->isString()) {
                ਜੋੜੋ_Ignore("Unsupported: strings");
            } ਨਹੀਂਤਾਂ {
                m_traName = "";
                ਜੋੜੋ_Trace_Decl(nodep->declRange(), 0);
            }
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_DType* ਅਚਲ subtypep = nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
            // Always iterate left index to right index
            ਲਈ (ਅਚਲ ਪੂਰਨ i : nodep->declRange().seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::ਮੰਜ਼ਿਲ_string(i) + ']';
                m_traValuep = m_traValuep->clone_ਰੁੱਖ(ਝੂਠ);
                m_traValuep = ਨਵਾਂ AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ਰੁੱਖ(), m_traValuep);
            }
        }

        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{flp});
    }
    ਖਾਲੀ declPackedArray(AstPackArrayDType* ਅਚਲ nodep, ਤਾਰਕਿਕ newFunc) {
        string ਅਗੇਤਰ_Name(newFunc ? "name" : m_traName);
        Ast_ਗੰਢ_DType* ਅਚਲ subtypep = nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();

        VL_RESTORER_COPY(m_traName);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{flp, ਅਗੇਤਰ_Name, VTrace_ਅਗੇਤਰ_Type::ARRAY_PACKED,
                                            nodep->ਖੱਬੇ(), nodep->ਸੱਜੇ(), !newFunc});

        // Always iterate left index to right index
        ਲਈ (ਅਚਲ ਪੂਰਨ i : nodep->declRange().seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::ਮੰਜ਼ਿਲ_string(i) + ']';
            ਅਚਲ ਪੂਰਨ lsb = (i - nodep->lo()) * subtypep->ਚੌੜਾਈ();
            m_traValuep = m_traValuep->clone_ਰੁੱਖ(ਝੂਠ);
            m_traValuep = ਨਵਾਂ AstSel{flp, m_traValuep, lsb, subtypep->ਚੌੜਾਈ()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_ਰੁੱਖ(), m_traValuep = ਖਾਲੀ_ਸੂਚਕ);
        }

        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{flp});
    }
    ਖਾਲੀ declStruct(AstStructDType* ਅਚਲ nodep, ਤਾਰਕਿਕ newFunc) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        string ਅਗੇਤਰ_Name(newFunc ? "name" : m_traName);
        ਪੂਰਨ nMembers = 0;
        ਲਈ (Ast_ਗੰਢ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        ਜੇ (!nodep->packed()) {
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{flp, ਅਗੇਤਰ_Name,  //
                                                VTrace_ਅਗੇਤਰ_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            ਲਈ (ਅਚਲ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ਗੰਢ_DType* ਅਚਲ subtypep = itemp->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ਰੁੱਖ(ਝੂਠ);
                m_traValuep = ਨਵਾਂ AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ਰੁੱਖ(), m_traValuep);
            }
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{flp});
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{flp, ਅਗੇਤਰ_Name,  //
                                                VTrace_ਅਗੇਤਰ_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            ਲਈ (ਅਚਲ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ਗੰਢ_DType* ਅਚਲ subtypep = itemp->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ਰੁੱਖ(ਝੂਠ);
                m_traValuep = ਨਵਾਂ AstSel{flp, m_traValuep, itemp->lsb(), subtypep->ਚੌੜਾਈ()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ਰੁੱਖ(), m_traValuep);
            }
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{flp});
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        ਜੇ (nodep->modp()->verilatorLib()) ਵਾਪਸ;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstCell* ਅਚਲ cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_ਪਿੱਠ(nodep, cellp);
        }

        ਜੇ (!m_entries.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_ਛਾਂਟੋ(
                m_entries.begin(), m_entries.ਸਿਰਾ(),
                [](ਅਚਲ Trace_ਇੰਦਰਾਜ& a, ਅਚਲ Trace_ਇੰਦਰਾਜ& b) { ਵਾਪਸ a.operatorCompare(b); });

            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            ਰਸਤਾ_Adjustor ਰਸਤਾ_Adjustor_2{flp, [&](Ast_ਗੰਢ_Stmt* stmtp) { ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(stmtp); }};
            ਅਚਲ ਤਾਰਕਿਕ ਵੰਡੋ_ਜੜ੍ਹ_Primary_Ios = nodep->is_ਸਿਖਰ() && !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ();
            ਅਚਲ ਸਵੈਚਾਲਿਤ emit_ਇੰਦਰਾਜ = [&](ਅਚਲ Trace_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ) {
                AstVarScope* ਅਚਲ vscp = ਇੰਦਰਾਜ.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << ਇੰਦਰਾਜ.ਰਸਤਾ() << "' name='" << ਇੰਦਰਾਜ.name() << "' "
                                  << (ਇੰਦਰਾਜ.cellp() ? static_cast<Ast_ਗੰਢ*>(ਇੰਦਰਾਜ.cellp())
                                                    : static_cast<Ast_ਗੰਢ*>(ਇੰਦਰਾਜ.vscp())));
                ਰਸਤਾ_Adjustor_2.adjust(ਇੰਦਰਾਜ.ਰਸਤਾ(), ਇੰਦਰਾਜ.cellp(), ਇੰਦਰਾਜ.vscp());

                m_traName = ਇੰਦਰਾਜ.name();

                ਜੇ (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ਅਚਲ string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    ਜੇ (!ignoreReason.ਖਾਲੀ_ਥਾਵਾਂ()) {
                        ਜੋੜੋ_Ignore(ignoreReason);
                    } ਨਹੀਂਤਾਂ {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = ਨਵਾਂ AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        ਜੇ (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_ਰੁੱਖ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = ਖਾਲੀ_ਸੂਚਕ;
                        }
                    }
                } ਨਹੀਂਤਾਂ {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ਅਚਲ cellp = ਇੰਦਰਾਜ.cellp();
                    Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = ਨਵਾਂ AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_ਪਿੱਠ(nodep, cellp, stmtp);
                }
            };
            ਜੇ (ਵੰਡੋ_ਜੜ੍ਹ_Primary_Ios) {
                m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Phase = ਸੱਚ;
                ਲਈ (ਅਚਲ Trace_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ : m_entries) {
                    AstVarScope* ਅਚਲ vscp = ਇੰਦਰਾਜ.vscp();
                    ਜੇ (!(vscp && vscp->varp()->isPrimaryIO())) ਜਾਰੀ;
                    emit_ਇੰਦਰਾਜ(ਇੰਦਰਾਜ);
                }
                m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Phase = ਝੂਠ;
                ਰਸਤਾ_Adjustor_2.unwind();
                m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Func_ਗਿਣਤੀ = m_subFuncps.ਮਾਪ();
                ਜੇ (m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Func_ਗਿਣਤੀ) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_ਮਾਪ = 0;
                    m_force_New_ਪੱਤੀ = ਸੱਚ;
                }
            }
            ਲਈ (ਅਚਲ Trace_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ : m_entries) {
                AstVarScope* ਅਚਲ vscp = ਇੰਦਰਾਜ.vscp();
                ਜੇ (ਵੰਡੋ_ਜੜ੍ਹ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) ਜਾਰੀ;
                emit_ਇੰਦਰਾਜ(ਇੰਦਰਾਜ);
            }
            ਰਸਤਾ_Adjustor_2.unwind();
            m_traVscp = ਖਾਲੀ_ਸੂਚਕ;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ਅਚਲ std::string ਰਸਤਾ = nodep->prettyName();
        m_ਰਸਤਾ_ਮੰਜ਼ਿਲ_Scopep.emplace(ਰਸਤਾ, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ਅਚਲ AstCell* ਅਚਲ cellp = nodep->aboveCellp();
        ਜੇ (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ਅਚਲ ਮਾਪ_t ਆਖਰੀ_Dot = ਰਸਤਾ.ਲੱਭੋ_ਆਖਰੀ_ਦਾ('.');
            UASSERT_OBJ(ਆਖਰੀ_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << ਰਸਤਾ);
            ਅਚਲ std::string ਮਾਪਾ_ਰਸਤਾ = ਰਸਤਾ.substr(0, ਆਖਰੀ_Dot + 1);

            ਲਈ (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ਅਚਲ std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                ਜੇ (!VString::startsWith(refName, ਮਾਪਾ_ਰਸਤਾ)) ਜਾਰੀ;

                // Save the mapping from the path of the reference to the scope
                m_ਰਸਤਾ_ਮੰਜ਼ਿਲ_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                ਜੇ (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), intfRefp);
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        ਜੇ (nodep->varp()->is_ਆਰਜ਼ੀ() && !nodep->varp()->isTrace()) ਵਾਪਸ;
        ਜੇ (nodep->varp()->isClassMember()) ਵਾਪਸ;
        ਜੇ (nodep->varp()->isFuncLocal()) ਵਾਪਸ;

        // When creating a --lib-create library ...
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Ignore parameters in packages. These will be traced at the top level.
            ਜੇ (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) ਵਾਪਸ;
        }

        // Add to traced signal list
        m_entries.emplace_ਪਿੱਠ(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    ਖਾਲੀ ਦੇਖੋ(AstConstDType* nodep) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਥ);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ਸੱਚ;
        iterate(nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump());
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਥ);
        iterate(nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump());
    }
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* /*nodep*/) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;
        // Insert a placeholder to be fixed up later
        File_ਸਤਰ* ਅਚਲ flp = m_traVscp->fileline();
        Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp
            = ਨਵਾਂ AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_ਪਿੱਠ(m_traVscp, stmtp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        ਜੇ (!m_traVscp) ਵਾਪਸ;

        ਜੇ (v3Global.opt.traceMaxArray()
            && static_cast<ਪੂਰਨ>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            ਜੋੜੋ_Ignore("Wide memory > --trace-max-array ents");
            ਵਾਪਸ;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        ਜੇ (VN_IS(nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump()->isString()) {
            ਜੋੜੋ_Ignore("Unsupported: strings");
            ਵਾਪਸ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ਜੇ (isBasicIO()) m_skipDtypeFunc = ਸੱਚ;

        ਜੇ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_ਵਿੱਥ);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPackArrayDType* nodep) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;

        ਜੇ (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            ਜੋੜੋ_Trace_Decl(VNumRange{}, nodep->ਚੌੜਾਈ());
            ਵਾਪਸ;
        }

        Ast_ਗੰਢ_DType* ਅਚਲ subtypep = nodep->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        ਜੇ (AstBasicDType* ਅਚਲ basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            ਜੇ (basicp->is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic() && !basicp->isRanged()) {
                ਜੋੜੋ_Trace_Decl(VNumRange{}, nodep->ਚੌੜਾਈ());
                ਵਾਪਸ;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ਜੇ (isBasicIO()) m_skipDtypeFunc = ਸੱਚ;

        ਜੇ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_ਵਿੱਥ);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructDType* nodep) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;

        ਜੇ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            ਜੋੜੋ_Trace_Decl(VNumRange{}, nodep->ਚੌੜਾਈ());
            ਵਾਪਸ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ਜੇ (isBasicIO()) m_skipDtypeFunc = ਸੱਚ;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        ਜੇ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_ਵਿੱਥ);
            newDeclFunc(nodep);
        }
        declStruct(nodep, ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnionDType* nodep) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ਸੱਚ;

        ਜੇ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            ਜੋੜੋ_Trace_Decl(VNumRange{}, nodep->ਚੌੜਾਈ());
            ਵਾਪਸ;
        }

        VL_RESTORER_COPY(m_traName);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        ਪੂਰਨ nMembers = 0;
        ਲਈ (Ast_ਗੰਢ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        ਜੇ (!nodep->packed()) {
            ਜੋੜੋ_Ignore("Unsupported: Unpacked union");
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ{flp, m_traName,  //
                                                VTrace_ਅਗੇਤਰ_Type::UNION_PACKED, nMembers});
            ਲਈ (ਅਚਲ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ਗੰਢ_DType* ਅਚਲ subtypep = itemp->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Sub_Func(ਨਵਾਂ Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ{flp});
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstBasicDType* nodep) override {
        ਜੇ (!m_traVscp) ਵਾਪਸ;
        ਜੇ (nodep->isString()) {
            ਜੋੜੋ_Ignore("Unsupported: strings");
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Trace_Decl(VNumRange{}, 0);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType*) override {
        // Note more specific dtypes above
        ਜੇ (!m_traVscp) ਵਾਪਸ;
        ਜੋੜੋ_Ignore("Unsupported: data type");
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TraceDeclVisitor(AstNetlist* nodep)
        : m_ਸਿਖਰ_Scopep{nodep->ਸਿਖਰ_Scopep()} {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_ਅਗਲਾ_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_ਅਗੇਤਰ_ਧੱਕੋ_ਕੱਢੋ();

        ਅਚਲ std::ਸਦਿਸ਼<AstCFunc*>& ਸਿਖਰ_Scope_Funcps = m_scopeInitFuncps.at(m_ਸਿਖਰ_Scopep->scopep());
        AstCFunc* ਜੜ੍ਹ_Funcp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਜੜ੍ਹ_Funcp = newCFunc(flp, "trace_init_root");
            ਜੜ੍ਹ_Funcp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(ਸੱਚ);
            ਲਈ (ਮਾਪ_t i = 0; i < m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Func_ਗਿਣਤੀ; ++i) {
                AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, ਸਿਖਰ_Scope_Funcps.at(i)};
                callp->dtype_ਸੈੱਟ_Void();
                callp->argTypes("tracep");
                ਜੜ੍ਹ_Funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
            }
            ਜੇ (!m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Func_ਗਿਣਤੀ) ਜੜ੍ਹ_Funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        ਲਈ (ਮਾਪ_t i = m_ਸਿਖਰ_Scope_ਜੜ੍ਹ_Func_ਗਿਣਤੀ; i < ਸਿਖਰ_Scope_Funcps.ਮਾਪ(); ++i) {
            AstCFunc* ਅਚਲ funcp = ਸਿਖਰ_Scope_Funcps.at(i);
            AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp};
            callp->dtype_ਸੈੱਟ_Void();
            callp->argTypes("tracep");
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਸਿਖਰ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        ਜੇ (m_ਸਿਖਰ_Funcps.ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਸਿਖਰ_Func(ਨਵਾਂ AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        ਜੇ (m_ਸਿਖਰ_Funcps.ਮਾਪ() > 1) {
            AstCFunc* ਅਚਲ ਸਿਖਰ_Funcp = newCFunc(flp, "");
            ਲਈ (AstCFunc* funcp : m_ਸਿਖਰ_Funcps) {
                AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp};
                callp->dtype_ਸੈੱਟ_Void();
                callp->argTypes("tracep");
                ਸਿਖਰ_Funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
            }
            m_ਸਿਖਰ_Funcps.clear();
            m_ਸਿਖਰ_Funcps.ਧੱਕੋ_ਪਿੱਠ(ਸਿਖਰ_Funcp);
        }

        // Set name of top level function
        AstCFunc* ਅਚਲ ਸਿਖਰ_Funcp = m_ਸਿਖਰ_Funcps.front();
        ਸਿਖਰ_Funcp->name("trace_init_top");
        ਸਿਖਰ_Funcp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(ਸੱਚ);

        ਜੇ (ਜੜ੍ਹ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ਜੜ੍ਹ_Funcp);
        checkCalls(ਸਿਖਰ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::ਜੋੜੋ_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

ਖਾਲੀ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("tracedecl", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
