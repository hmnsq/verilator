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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3TraceDecl.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Number.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

ವರ್ಗ ಮಾರ್ಗ_Adjustor final {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ m_flp;  // FileLine used for created nodes
    std::function<ಶೂನ್ಯ(Ast_ಗಂಟು_Stmt*)> m_emit;  // Function called with adjustment statements
    std::ಸದಿಶ<std::string> m_ರಾಶಿ{""};  // Stack of current paths

    ಸ್ಥಿರ constexpr ಅಕ್ಷರ SEPARATOR = ' ';

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ಮಾರ್ಗ_Adjustor(File_ಸಾಲು* flp, std::function<ಶೂನ್ಯ(Ast_ಗಂಟು_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    ಶೂನ್ಯ adjust(ಸ್ಥಿರಾಂಕ string& new_ಮಾರ್ಗ, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        ಆಗ (!VString::startsWith(new_ಮಾರ್ಗ, m_ರಾಶಿ.ಹಿಂಭಾಗ())) {
            m_emit(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{m_flp});
            m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        }

        ಆದರೆ (new_ಮಾರ್ಗ == m_ರಾಶಿ.ಹಿಂಭಾಗ()) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type ಕೊನೆಯ_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::SCOPE_INTERFACE
                : VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::SCOPE_MODULE;
        ಸ್ಥಿರಾಂಕ std::string extra_ಪೂರ್ವಪ್ರತ್ಯಯ = new_ಮಾರ್ಗ.substr(m_ರಾಶಿ.ಹಿಂಭಾಗ().ಗಾತ್ರ_2());
        ಗಾತ್ರ_t begin = 0;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆಯ = extra_ಪೂರ್ವಪ್ರತ್ಯಯ.rfind(SEPARATOR);
        // Move down, one path element at a time
        ಆಗ (ನಿಜ) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆ = extra_ಪೂರ್ವಪ್ರತ್ಯಯ.ಕಂಡುಹಿಡಿ(SEPARATOR, begin);
            ಆದರೆ (ಕೊನೆ == string::npos) ನಿಲ್ಲಿಸು;
            ಸ್ಥಿರಾಂಕ string& extra = extra_ಪೂರ್ವಪ್ರತ್ಯಯ.substr(begin, ಕೊನೆ - begin);
            ಆದರೆ (ಕೊನೆ == ಕೊನೆಯ) {
                m_emit(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{m_flp, extra, ಕೊನೆಯ_Scope_Type});
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_emit(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{m_flp, extra, VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::SCOPE_MODULE});
            }
            m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(m_ರಾಶಿ.ಹಿಂಭಾಗ() + extra + SEPARATOR);
            begin = ಕೊನೆ + 1;
        }
        UASSERT(begin == extra_ಪೂರ್ವಪ್ರತ್ಯಯ.ಗಾತ್ರ_2(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    ಶೂನ್ಯ unwind() {
        ಆಗ (m_ರಾಶಿ.ಗಾತ್ರ_2() > 1) {
            m_emit(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{m_flp});
            m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

ವರ್ಗ TraceDeclVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ m_ತುದಿ_Scopep;  // The singleton AstTopScope
    ಸ್ಥಿರಾಂಕ AstScope* m_currScopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope being visited

    std::ಸದಿಶ<AstCFunc*> m_ತುದಿ_Funcps;  // Top level trace initialization functions
    std::ಸದಿಶ<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ರಚನೆ Dtype_Func_ಕೀಲಿ final {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep;
        VVarType varType;
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ Dtype_Func_ಕೀಲಿ& other) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ dtypep == other.dtypep && varType == other.varType;
        }
    };
    ರಚನೆ Dtype_Func_ಕೀಲಿ_Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dtype_Func_ಕೀಲಿ& ಕೀಲಿ) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ std::hash<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>{}(ಕೀಲಿ.dtypep)
                   ^ (std::hash<uint8_t>{}(ಕೀಲಿ.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ಕೀಲಿ, AstCFunc*, Dtype_Func_ಕೀಲಿ_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = ಶೂನ್ಯಸೂಚಕ;  // Current type func
    AstCFunc* m_dtypeSubFuncp = ಶೂನ್ಯಸೂಚಕ;  // Current type sub func
    ಸ್ಥಿರಾಂಕ string m_dtype_ವಾದಗಳು{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = ಶೂನ್ಯಸೂಚಕ;  // Current type func decl
    V3_Unique_ಹೆಸರುಗಳು m_dtype_ಹೆಸರುಗಳು{""};  // Unique names for dtype funcs
    ತಾರ್ಕಿಕ m_skipDtypeFunc = ಸುಳ್ಳು;  // Don't create a type func
    uint32_t m_ಸ್ಥಾನಾಂತರ = std::numeric_limits<uint32_t>::max();  // Offset for types
    ಪೂರ್ಣಾಂಕ m_ತುದಿ_Func_ಗಾತ್ರ = 0;  // Size of the top function currently being built
    ಪೂರ್ಣಾಂಕ m_sub_Func_ಗಾತ್ರ = 0;  // Size of the sub function currently being built
    ತಾರ್ಕಿಕ m_force_New_ಎಲೆ = ಸುಳ್ಳು;  // Force next addToSubFunc to start a fresh leaf
    ಗಾತ್ರ_t m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ = 0;  // Top-scope init functions used only for wrapper IOs
    ತಾರ್ಕಿಕ m_ತುದಿ_Scope_ಬೇರು_Phase = ಸುಳ್ಳು;  // Emitting top-scope wrapper IO declarations
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_func_ಗಾತ್ರ_ಎಲ್ಲೆ  // Maximum size of a function
        = v3Global.opt.output_ವಿಭಜಿಸು_CTrace() ? v3Global.opt.output_ವಿಭಜಿಸು_CTrace()
                                           : std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
    // Trace init functions to for each scope
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstScope*, std::ಸದಿಶ<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ಸ್ಥಿರಾಂಕ AstScope*> m_ಮಾರ್ಗ_ಗಮ್ಯ_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::ಸದಿಶ<std::tuple<AstScope*, AstCell*, Ast_ಗಂಟು_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::ಸದಿಶ<std::tuple<AstVarScope*, Ast_ಗಂಟು_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    ವರ್ಗ Trace_ನಮೂದು final {
        AstVarScope* m_vscp = ಶೂನ್ಯಸೂಚಕ;  // AstVarScope under scope being traced
        AstCell* m_cellp = ಶೂನ್ಯಸೂಚಕ;  // Sub scope (as AstCell) under scope being traced
        std::string m_ಮಾರ್ಗ;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        ಶೂನ್ಯ init(ಸ್ಥಿರಾಂಕ std::string& name, Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ಒಳ_ತುದಿ_Scope) {
            // Compute path in hierarchy and item name
            ಸ್ಥಿರಾಂಕ std::string& vcdName = Ast_ಗಂಟು::vcdName(name);
            AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
            ಆದರೆ (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_ಮಾರ್ಗ = vcdName + " ";
                m_name.clear();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isPrimaryIO()) {
                // Primary IO variable
                m_ಮಾರ್ಗ = "$rootio ";
                m_name = vcdName;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Other Variable
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pos = vcdName.rfind(' ');
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮಾರ್ಗ_Len = pos == std::string::npos ? 0 : pos + 1;
                m_ಮಾರ್ಗ = vcdName.substr(0, ಮಾರ್ಗ_Len);
                m_name = vcdName.substr(ಮಾರ್ಗ_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            ಆದರೆ (ಒಳ_ತುದಿ_Scope && !v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಆರಂಭ = m_ಮಾರ್ಗ.ಕಂಡುಹಿಡಿ(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(ಆರಂಭ != std::string::npos, nodep, "No prefix with --lib-create");
                ಸ್ಥಿರಾಂಕ std::string ಪೂರ್ವಪ್ರತ್ಯಯ = m_ಮಾರ್ಗ.substr(0, ಆರಂಭ);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ == "$rootio") ಹಿಂತಿರುಗಿ;
                m_ಮಾರ್ಗ = m_ಮಾರ್ಗ.substr(ಆರಂಭ + 1);
                ಆದರೆ (v3Global.rootp()->trace_Lib_ತುದಿ_Name() != ಪೂರ್ವಪ್ರತ್ಯಯ) m_ಮಾರ್ಗ = "$libroot " + m_ಮಾರ್ಗ;
            }
        }

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ Trace_ನಮೂದು(ಸ್ಥಿರಾಂಕ AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_ತುದಿ());
        }
        ಸ್ಪಷ್ಟ Trace_ನಮೂದು(ಸ್ಥಿರಾಂಕ AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_ತುದಿ());
        }
        ಪೂರ್ಣಾಂಕ operatorCompare(ಸ್ಥಿರಾಂಕ Trace_ನಮೂದು& b) ಸ್ಥಿರಾಂಕ {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ cmp = ಮಾರ್ಗ().compare(b.ಮಾರ್ಗ())) ಹಿಂತಿರುಗಿ cmp < 0;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ cmp = fileline().operatorCompare(b.fileline())) ಹಿಂತಿರುಗಿ cmp < 0;
            ಹಿಂತಿರುಗಿ name() < b.name();
        }
        AstVarScope* vscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp; }
        AstCell* cellp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cellp; }
        ಸ್ಥಿರಾಂಕ std::string& ಮಾರ್ಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮಾರ್ಗ; }
        ಶೂನ್ಯ ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ std::string& ಮಾರ್ಗ) { m_ಮಾರ್ಗ = ಮಾರ್ಗ; }
        ಸ್ಥಿರಾಂಕ std::string& name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
        File_ಸಾಲು& fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::ಸದಿಶ<Trace_ನಮೂದು> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = ಶೂನ್ಯಸೂಚಕ;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_traValuep = ಶೂನ್ಯಸೂಚಕ;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ಸ್ಥಿರಾಂಕ AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (!varp->isTrace()) ಹಿಂತಿರುಗಿ "Verilator trace_off";
        ಆದರೆ (!nodep->isTrace()) ಹಿಂತಿರುಗಿ "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        ಆದರೆ (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            ಹಿಂತಿರುಗಿ "Automatic variable";

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = recurse_DType_ಅಗಲ(nodep->varp()->dtypep());
        ಆದರೆ (v3Global.opt.trace_Max_ಅಗಲ() && ಅಗಲ > v3Global.opt.trace_Max_ಅಗಲ())
            ಹಿಂತಿರುಗಿ "Width " + cvt_ಗಮ್ಯ_Str(ಅಗಲ) + " > --trace-max-width";

        ಸ್ಥಿರಾಂಕ string prettyName = nodep->prettyName();
        ಆದರೆ (!V3Control::getScopeTraceOn(prettyName)) ಹಿಂತಿರುಗಿ "Vlt scope trace_off";
        ಆದರೆ (!v3Global.opt.traceUnderscore()) {
            ಆದರೆ (!prettyName.ಖಾಲಿಗಳು() && prettyName[0] == '_') ಹಿಂತಿರುಗಿ "Leading underscore";
            ಆದರೆ (prettyName.ಕಂಡುಹಿಡಿ("._") != string::npos) ಹಿಂತಿರುಗಿ "Inlined leading underscore";
        }
        ಹಿಂತಿರುಗಿ ""s;
    }

    ಪೂರ್ಣಾಂಕ recurse_DType_ಅಗಲ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* adtypep = VN_CAST(nodep, ಗಂಟು_Array_DType))
            ಹಿಂತಿರುಗಿ recurse_DType_ಅಗಲ(adtypep->subDTypep()) * adtypep->declRange().elements();
        ಹಿಂತಿರುಗಿ nodep->ಅಗಲ();
    }

    AstCFunc* newCFunc(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ string& name) {
        AstScope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = m_ತುದಿ_Scopep->scopep();
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{flp, name, ತುದಿ_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_ತಳ() + "* tracep");
        funcp->isTrace(ನಿಜ);
        funcp->isStatic(ಸುಳ್ಳು);
        funcp->isLoose(ನಿಜ);
        funcp->slow(ನಿಜ);
        ತುದಿ_Scopep->ಕೂಡಿಸು_Blocksp(funcp);
        ಹಿಂತಿರುಗಿ funcp;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಗಮ್ಯ_ತುದಿ_Func(Ast_ಗಂಟು_Stmt* stmtp) {
        ಆದರೆ (m_ತುದಿ_Func_ಗಾತ್ರ > m_func_ಗಾತ್ರ_ಎಲ್ಲೆ || m_ತುದಿ_Funcps.ಖಾಲಿಗಳು()) {
            m_ತುದಿ_Func_ಗಾತ್ರ = 0;
            //
            ಸ್ಥಿರಾಂಕ string n = cvt_ಗಮ್ಯ_Str(m_ತುದಿ_Funcps.ಗಾತ್ರ_2());
            ಸ್ಥಿರಾಂಕ string name{"trace_init_top__" + n};
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = newCFunc(m_ತುದಿ_Scopep->fileline(), name);
            m_ತುದಿ_Funcps.ತಳ್ಳು_ಹಿಂಭಾಗ(funcp);
        }
        m_ತುದಿ_Funcps.ಹಿಂಭಾಗ()->ಕೂಡಿಸು_Stmtsp(stmtp);
        m_ತುದಿ_Func_ಗಾತ್ರ += stmtp->ಗಂಟು_ಎಣಿಕೆ();
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(Ast_ಗಂಟು_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        ಆದರೆ (m_dtypeSubFuncp) {
            ಆದರೆ (m_sub_Func_ಗಾತ್ರ > m_func_ಗಾತ್ರ_ಎಲ್ಲೆ) newDeclSubFunc();
            m_dtypeSubFuncp->ಕೂಡಿಸು_Stmtsp(stmtp);
            m_sub_Func_ಗಾತ್ರ += stmtp->ಗಂಟು_ಎಣಿಕೆ();
            ಹಿಂತಿರುಗಿ;
        }
        // Defer trace splitting until V3Trace
        ಆದರೆ (m_subFuncps.ಖಾಲಿಗಳು() || m_force_New_ಎಲೆ) {
            m_force_New_ಎಲೆ = ಸುಳ್ಳು;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_currScopep->fileline();
            ಸ್ಥಿರಾಂಕ string n = cvt_ಗಮ್ಯ_Str(m_subFuncps.ಗಾತ್ರ_2());
            ಸ್ಥಿರಾಂಕ string name
                = m_currScopep == m_ತುದಿ_Scopep->scopep() && !v3Global.opt.libCreate().ಖಾಲಿಗಳು()
                      ? (m_ತುದಿ_Scope_ಬೇರು_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = newCFunc(flp, name);
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ತಳ್ಳು_ಹಿಂಭಾಗ(funcp);
        }
        m_subFuncps.ಹಿಂಭಾಗ()->ಕೂಡಿಸು_Stmtsp(stmtp);
        m_sub_Func_ಗಾತ್ರ += stmtp->ಗಂಟು_ಎಣಿಕೆ();
    }

    AstTraceDecl* ಕೂಡಿಸು_Trace_Decl(ಸ್ಥಿರಾಂಕ VNumRange& arrayRange,
                               ಪೂರ್ಣಾಂಕ ಅಗಲ_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ಸ್ಥಿರಾಂಕ dtypeCallp = ಶೂನ್ಯಸೂಚಕ) {
        VNumRange ಬಿಟ್_Range;
        ಆದರೆ (ಅಗಲ_Override) {
            ಬಿಟ್_Range = VNumRange{ಅಗಲ_Override - 1, 0};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = m_traValuep->dtypep()->basicp()) {
            ಬಿಟ್_Range = bdtypep->nrange();
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_traVscp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = m_traValuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ valid_ಸ್ಥಾನಾಂತರ = m_ಸ್ಥಾನಾಂತರ != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               ಬಿಟ್_Range, arrayRange, dtypeCallp,        valid_ಸ್ಥಾನಾಂತರ};
        ಆದರೆ (valid_ಸ್ಥಾನಾಂತರ) {
            newp->ಸಂಕೇತ_2(m_ಸ್ಥಾನಾಂತರ);
            ಆದರೆ (!dtypeCallp) { m_ಸ್ಥಾನಾಂತರ += newp->ಸಂಕೇತ_Inc(); }
            valuep->foreach([&](AstVarRef* ಸ್ಥಿರಾಂಕ refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(ಹೊಸ AstCExpr{flp, "__VdtypeVar", m_traVscp->ಅಗಲ()});
                VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
            });
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(newp);

        ಹಿಂತಿರುಗಿ newp;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Ignore(ಸ್ಥಿರಾಂಕ string& why) {
        ++m_statIgnSigs;
        ಸ್ಥಿರಾಂಕ std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        ಆದರೆ (debug() > 3 && m_traVscp) std::ಹೊರಗೆ << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    ಶೂನ್ಯ fixupPlaceholder(ಸ್ಥಿರಾಂಕ std::string& ಮಾರ್ಗ, Ast_ಗಂಟು_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಮಾರ್ಗ_ಗಮ್ಯ_Scopep.ಕಂಡುಹಿಡಿ(Ast_ಗಂಟು::prettyName(ಮಾರ್ಗ));
        ಆದರೆ (it != m_ಮಾರ್ಗ_ಗಮ್ಯ_Scopep.ಕೊನೆ()) {
            ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = it->ಎರಡನೇ;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ಗಂಟು* stmtp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, subFuncp};
                callp->dtype_ಗಣ_Void();
                callp->argTypes("tracep");
                stmtp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            ಆದರೆ (stmtp) placeholderp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(placeholderp->delete_ವೃಕ್ಷ(), placeholderp);
    }

    ಶೂನ್ಯ fixupLibStub(ಸ್ಥಿರಾಂಕ std::string& ಮಾರ್ಗ, Ast_ಗಂಟು_Stmt* placeholderp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ಗಂಟು::prettyName(ಮಾರ್ಗ);
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ std::string ತುದಿ_ಪೂರ್ವಪ್ರತ್ಯಯ = v3Global.rootp()->trace_Lib_ತುದಿ_Name() + ".";
            ಆದರೆ (VString::startsWith(name, ತುದಿ_ಪೂರ್ವಪ್ರತ್ಯಯ)) name = name.substr(ತುದಿ_ಪೂರ್ವಪ್ರತ್ಯಯ.ಗಾತ್ರ_2());
        }

        // Call the initialization function for the library instance
        AstCStmt* ಸ್ಥಿರಾಂಕ initp = ಹೊಸ AstCStmt{flp};
        initp->ಕೂಡಿಸು("{\n");
        initp->ಕೂಡಿಸು("std::string __VlibName = vlSymsp->name();\n");
        initp->ಕೂಡಿಸು("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->ಕೂಡಿಸು("__VlibName += ");
        initp->ಕೂಡಿಸು(ಹೊಸ AstConst{flp, AstConst::String{}, name});
        initp->ಕೂಡಿಸು(";\n");
        initp->ಕೂಡಿಸು("tracep->initLib(__VlibName);\n");
        initp->ಕೂಡಿಸು("}\n");

        placeholderp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), placeholderp);
    }

    ಶೂನ್ಯ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಶ_2 : m_cellInitPlaceholders) {
            ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ parentp = std::get<0>(ಅಂಶ_2);
            ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = std::get<1>(ಅಂಶ_2);
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ placeholderp = std::get<2>(ಅಂಶ_2);
            ಸ್ಥಿರಾಂಕ std::string ಮಾರ್ಗ = parentp->name() + "__DOT__" + cellp->name();
            ಆದರೆ (cellp->modp()->verilatorLib()) {
                fixupLibStub(ಮಾರ್ಗ, placeholderp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                fixupPlaceholder(ಮಾರ್ಗ, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಶ_2 : m_ifaceRefInitPlaceholders) {
            ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = std::get<0>(ಅಂಶ_2);
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ placeholderp = std::get<1>(ಅಂಶ_2);
            ಸ್ಥಿರಾಂಕ std::string ಮಾರ್ಗ = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(ಮಾರ್ಗ, placeholderp);
        }
    }

    ಶೂನ್ಯ remove_Redundant_ಪೂರ್ವಪ್ರತ್ಯಯ_ತಳ್ಳು_ತೆಗೆ() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp : pair.ಎರಡನೇ) {
                Ast_ಗಂಟು* prevp = ಶೂನ್ಯಸೂಚಕ;
                Ast_ಗಂಟು* currp = funcp->stmtsp();
                ಆಗ (currp) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = currp->nextp();
                    ಆದರೆ (VN_IS(prevp, Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ) && VN_IS(currp, Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), currp);
                    }
                    ಆದರೆ (!nextp) ನಿಲ್ಲಿಸು;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    ಶೂನ್ಯ checkCalls(ಸ್ಥಿರಾಂಕ AstCFunc* funcp) {
        ಆದರೆ (!v3Global.opt.debugCheck()) ಹಿಂತಿರುಗಿ;
        checkCallsRecurse(funcp);
        ಆದರೆ (!m_declUncalledps.ಖಾಲಿಗಳು()) {  // LCOV_EXCL_START
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    ಶೂನ್ಯ checkCallsRecurse(ಸ್ಥಿರಾಂಕ AstCFunc* funcp) {
        funcp->foreach([ಇದು](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCCall* ಸ್ಥಿರಾಂಕ ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    ತಾರ್ಕಿಕ isBasicIO() { ಹಿಂತಿರುಗಿ m_traVscp->varp()->isVLIO(); }
    ಶೂನ್ಯ newDeclSubFunc() {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_dtypeFuncp->fileline();
        AstCFunc* ಸ್ಥಿರಾಂಕ subFuncp = newCFunc(flp, m_dtype_ಹೆಸರುಗಳು.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_ವಾದಗಳು);
        AstCCall* ಸ್ಥಿರಾಂಕ subCallp = ಹೊಸ AstCCall{flp, subFuncp};
        subCallp->dtype_ಗಣ_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->ಕೂಡಿಸು_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_ಗಾತ್ರ = 0;
    }
    ಶೂನ್ಯ newDeclFunc(Ast_ಗಂಟು_DType* nodep) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_ವಾದಗಳು{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = skipTypep->fileline();

        ಸ್ಥಿರಾಂಕ Dtype_Func_ಕೀಲಿ dtype_ಕೀಲಿ{skipTypep, m_traVscp->varp()->varType()};
        ಸ್ವಯಂಚಾಲಿತ pair = m_dtypeFuncs.emplace(dtype_ಕೀಲಿ, ಶೂನ್ಯಸೂಚಕ);
        AstCFunc** funcpp = &pair.ಮೊದಲ->ಎರಡನೇ;
        ಆದರೆ (pair.ಎರಡನೇ) {
            *funcpp = newCFunc(flp, m_dtype_ಹೆಸರುಗಳು.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_ವಾದಗಳು);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, *funcpp};
        callp->dtype_ಗಣ_Void();
        callp->argTypes(call_ವಾದಗಳು);
        m_dtypeDeclp = ಕೂಡಿಸು_Trace_Decl(VNumRange{}, skipTypep->ಅಗಲ(), callp);
        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(callp->makeStmt());

        ಆದರೆ (pair.ಎರಡನೇ) {
            VL_RESTORER(m_ಸ್ಥಾನಾಂತರ);
            m_ಸ್ಥಾನಾಂತರ = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_ಗಾತ್ರ);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            ಆದರೆ (AstStructDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstPackArrayDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(ಸುಳ್ಳು, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_ಸ್ಥಾನಾಂತರ);
        }

        m_dtypeDeclp->ಸಂಕೇತ_Inc((*funcpp)->user1());
        m_ಸ್ಥಾನಾಂತರ += m_dtypeDeclp->ಸಂಕೇತ_Inc();
    }
    ಶೂನ್ಯ declUnpackedArray(AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ nodep, ತಾರ್ಕಿಕ newFunc) {
        string ಪೂರ್ವಪ್ರತ್ಯಯ_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{flp, ಪೂರ್ವಪ್ರತ್ಯಯ_Name, VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::ARRAY_UNPACKED,
                                            nodep->ಎಡ(), nodep->ಬಲ(), !newFunc});

        ಆದರೆ (VN_IS(nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            ಆದರೆ (m_traVscp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump()->isString()) {
                ಕೂಡಿಸು_Ignore("Unsupported: strings");
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_traName = "";
                ಕೂಡಿಸು_Trace_Decl(nodep->declRange(), 0);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump();
            // Always iterate left index to right index
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ i : nodep->declRange().seq_ಎಡ_ಗಮ್ಯ_ಬಲ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::ಗಮ್ಯ_string(i) + ']';
                m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                m_traValuep = ಹೊಸ AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
            }
        }

        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{flp});
    }
    ಶೂನ್ಯ declPackedArray(AstPackArrayDType* ಸ್ಥಿರಾಂಕ nodep, ತಾರ್ಕಿಕ newFunc) {
        string ಪೂರ್ವಪ್ರತ್ಯಯ_Name(newFunc ? "name" : m_traName);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump();

        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{flp, ಪೂರ್ವಪ್ರತ್ಯಯ_Name, VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::ARRAY_PACKED,
                                            nodep->ಎಡ(), nodep->ಬಲ(), !newFunc});

        // Always iterate left index to right index
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ i : nodep->declRange().seq_ಎಡ_ಗಮ್ಯ_ಬಲ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::ಗಮ್ಯ_string(i) + ']';
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = (i - nodep->lo()) * subtypep->ಅಗಲ();
            m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            m_traValuep = ಹೊಸ AstSel{flp, m_traValuep, lsb, subtypep->ಅಗಲ()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep = ಶೂನ್ಯಸೂಚಕ);
        }

        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{flp});
    }
    ಶೂನ್ಯ declStruct(AstStructDType* ಸ್ಥಿರಾಂಕ nodep, ತಾರ್ಕಿಕ newFunc) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        string ಪೂರ್ವಪ್ರತ್ಯಯ_Name(newFunc ? "name" : m_traName);
        ಪೂರ್ಣಾಂಕ nMembers = 0;
        ಗಾಗಿ (Ast_ಗಂಟು* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        ಆದರೆ (!nodep->packed()) {
            ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{flp, ಪೂರ್ವಪ್ರತ್ಯಯ_Name,  //
                                                VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = itemp->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                m_traValuep = ಹೊಸ AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
            }
            ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{flp});
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{flp, ಪೂರ್ವಪ್ರತ್ಯಯ_Name,  //
                                                VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = itemp->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                m_traValuep = ಹೊಸ AstSel{flp, m_traValuep, itemp->lsb(), subtypep->ಅಗಲ()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
            }
            ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{flp});
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.ಖಾಲಿಗಳು(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.ಖಾಲಿಗಳು(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.ಖಾಲಿಗಳು(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        ಆದರೆ (nodep->modp()->verilatorLib()) ಹಿಂತಿರುಗಿ;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_ಹಿಂಭಾಗ(nodep, cellp);
        }

        ಆದರೆ (!m_entries.ಖಾಲಿಗಳು()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_ವಿಂಗಡಿಸು(
                m_entries.begin(), m_entries.ಕೊನೆ(),
                [](ಸ್ಥಿರಾಂಕ Trace_ನಮೂದು& a, ಸ್ಥಿರಾಂಕ Trace_ನಮೂದು& b) { ಹಿಂತಿರುಗಿ a.operatorCompare(b); });

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            ಮಾರ್ಗ_Adjustor ಮಾರ್ಗ_Adjustor_2{flp, [&](Ast_ಗಂಟು_Stmt* stmtp) { ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(stmtp); }};
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ವಿಭಜಿಸು_ಬೇರು_Primary_Ios = nodep->is_ತುದಿ() && !v3Global.opt.libCreate().ಖಾಲಿಗಳು();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emit_ನಮೂದು = [&](ಸ್ಥಿರಾಂಕ Trace_ನಮೂದು& ನಮೂದು) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ನಮೂದು.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << ನಮೂದು.ಮಾರ್ಗ() << "' name='" << ನಮೂದು.name() << "' "
                                  << (ನಮೂದು.cellp() ? static_cast<Ast_ಗಂಟು*>(ನಮೂದು.cellp())
                                                    : static_cast<Ast_ಗಂಟು*>(ನಮೂದು.vscp())));
                ಮಾರ್ಗ_Adjustor_2.adjust(ನಮೂದು.ಮಾರ್ಗ(), ನಮೂದು.cellp(), ನಮೂದು.vscp());

                m_traName = ನಮೂದು.name();

                ಆದರೆ (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ಸ್ಥಿರಾಂಕ string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    ಆದರೆ (!ignoreReason.ಖಾಲಿಗಳು()) {
                        ಕೂಡಿಸು_Ignore(ignoreReason);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = ಹೊಸ AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        ಆದರೆ (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = ಶೂನ್ಯಸೂಚಕ;
                        }
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ಸ್ಥಿರಾಂಕ cellp = ನಮೂದು.cellp();
                    Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = ಹೊಸ AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_ಹಿಂಭಾಗ(nodep, cellp, stmtp);
                }
            };
            ಆದರೆ (ವಿಭಜಿಸು_ಬೇರು_Primary_Ios) {
                m_ತುದಿ_Scope_ಬೇರು_Phase = ನಿಜ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Trace_ನಮೂದು& ನಮೂದು : m_entries) {
                    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ನಮೂದು.vscp();
                    ಆದರೆ (!(vscp && vscp->varp()->isPrimaryIO())) ಮುಂದುವರಿ;
                    emit_ನಮೂದು(ನಮೂದು);
                }
                m_ತುದಿ_Scope_ಬೇರು_Phase = ಸುಳ್ಳು;
                ಮಾರ್ಗ_Adjustor_2.unwind();
                m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ = m_subFuncps.ಗಾತ್ರ_2();
                ಆದರೆ (m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_ಗಾತ್ರ = 0;
                    m_force_New_ಎಲೆ = ನಿಜ;
                }
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Trace_ನಮೂದು& ನಮೂದು : m_entries) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ನಮೂದು.vscp();
                ಆದರೆ (ವಿಭಜಿಸು_ಬೇರು_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) ಮುಂದುವರಿ;
                emit_ನಮೂದು(ನಮೂದು);
            }
            ಮಾರ್ಗ_Adjustor_2.unwind();
            m_traVscp = ಶೂನ್ಯಸೂಚಕ;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ಸ್ಥಿರಾಂಕ std::string ಮಾರ್ಗ = nodep->prettyName();
        m_ಮಾರ್ಗ_ಗಮ್ಯ_Scopep.emplace(ಮಾರ್ಗ, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = nodep->aboveCellp();
        ಆದರೆ (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆಯ_Dot = ಮಾರ್ಗ.ಕಂಡುಹಿಡಿ_ಕೊನೆಯ_ದ('.');
            UASSERT_OBJ(ಕೊನೆಯ_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << ಮಾರ್ಗ);
            ಸ್ಥಿರಾಂಕ std::string ಪೋಷಕ_ಮಾರ್ಗ = ಮಾರ್ಗ.substr(0, ಕೊನೆಯ_Dot + 1);

            ಗಾಗಿ (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ಸ್ಥಿರಾಂಕ std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                ಆದರೆ (!VString::startsWith(refName, ಪೋಷಕ_ಮಾರ್ಗ)) ಮುಂದುವರಿ;

                // Save the mapping from the path of the reference to the scope
                m_ಮಾರ್ಗ_ಗಮ್ಯ_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                ಆದರೆ (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), intfRefp);
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        ಆದರೆ (nodep->varp()->is_ತಾತ್ಕಾಲಿಕ() && !nodep->varp()->isTrace()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->varp()->isClassMember()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->varp()->isFuncLocal()) ಹಿಂತಿರುಗಿ;

        // When creating a --lib-create library ...
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            // Ignore parameters in packages. These will be traced at the top level.
            ಆದರೆ (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) ಹಿಂತಿರುಗಿ;
        }

        // Add to traced signal list
        m_entries.emplace_ಹಿಂಭಾಗ(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    ಶೂನ್ಯ ಭೇಟಿ(AstConstDType* nodep) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಸ್ಥಾನಾಂತರ);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ನಿಜ;
        iterate(nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಸ್ಥಾನಾಂತರ);
        iterate(nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* /*nodep*/) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;
        // Insert a placeholder to be fixed up later
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_traVscp->fileline();
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp
            = ಹೊಸ AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_ಹಿಂಭಾಗ(m_traVscp, stmtp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (v3Global.opt.traceMaxArray()
            && static_cast<ಪೂರ್ಣಾಂಕ>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            ಕೂಡಿಸು_Ignore("Wide memory > --trace-max-array ents");
            ಹಿಂತಿರುಗಿ;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        ಆದರೆ (VN_IS(nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump()->isString()) {
            ಕೂಡಿಸು_Ignore("Unsupported: strings");
            ಹಿಂತಿರುಗಿ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ಆದರೆ (isBasicIO()) m_skipDtypeFunc = ನಿಜ;

        ಆದರೆ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_ಸ್ಥಾನಾಂತರ);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackArrayDType* nodep) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            ಕೂಡಿಸು_Trace_Decl(VNumRange{}, nodep->ಅಗಲ());
            ಹಿಂತಿರುಗಿ;
        }

        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = nodep->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            ಆದರೆ (basicp->is_ಬಿಟ್_Logic() && !basicp->isRanged()) {
                ಕೂಡಿಸು_Trace_Decl(VNumRange{}, nodep->ಅಗಲ());
                ಹಿಂತಿರುಗಿ;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ಆದರೆ (isBasicIO()) m_skipDtypeFunc = ನಿಜ;

        ಆದರೆ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_ಸ್ಥಾನಾಂತರ);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructDType* nodep) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            ಕೂಡಿಸು_Trace_Decl(VNumRange{}, nodep->ಅಗಲ());
            ಹಿಂತಿರುಗಿ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ಆದರೆ (isBasicIO()) m_skipDtypeFunc = ನಿಜ;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        ಆದರೆ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_ಸ್ಥಾನಾಂತರ);
            newDeclFunc(nodep);
        }
        declStruct(nodep, ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnionDType* nodep) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ನಿಜ;

        ಆದರೆ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            ಕೂಡಿಸು_Trace_Decl(VNumRange{}, nodep->ಅಗಲ());
            ಹಿಂತಿರುಗಿ;
        }

        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        ಪೂರ್ಣಾಂಕ nMembers = 0;
        ಗಾಗಿ (Ast_ಗಂಟು* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        ಆದರೆ (!nodep->packed()) {
            ಕೂಡಿಸು_Ignore("Unsupported: Unpacked union");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ{flp, m_traName,  //
                                                VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::UNION_PACKED, nMembers});
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = itemp->subDTypep()->skip_Ref_ಗಮ್ಯ_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            ಕೂಡಿಸು_ಗಮ್ಯ_Sub_Func(ಹೊಸ Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ{flp});
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBasicDType* nodep) override {
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->isString()) {
            ಕೂಡಿಸು_Ignore("Unsupported: strings");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Trace_Decl(VNumRange{}, 0);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType*) override {
        // Note more specific dtypes above
        ಆದರೆ (!m_traVscp) ಹಿಂತಿರುಗಿ;
        ಕೂಡಿಸು_Ignore("Unsupported: data type");
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TraceDeclVisitor(AstNetlist* nodep)
        : m_ತುದಿ_Scopep{nodep->ತುದಿ_Scopep()} {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.ಖಾಲಿಗಳು(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_ಪೂರ್ವಪ್ರತ್ಯಯ_ತಳ್ಳು_ತೆಗೆ();

        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstCFunc*>& ತುದಿ_Scope_Funcps = m_scopeInitFuncps.at(m_ತುದಿ_Scopep->scopep());
        AstCFunc* ಬೇರು_Funcp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            ಬೇರು_Funcp = newCFunc(flp, "trace_init_root");
            ಬೇರು_Funcp->ನಮೂದು_ಬಿಂದು(ನಿಜ);
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ; ++i) {
                AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, ತುದಿ_Scope_Funcps.at(i)};
                callp->dtype_ಗಣ_Void();
                callp->argTypes("tracep");
                ಬೇರು_Funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
            }
            ಆದರೆ (!m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ) ಬೇರು_Funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        ಗಾಗಿ (ಗಾತ್ರ_t i = m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ; i < ತುದಿ_Scope_Funcps.ಗಾತ್ರ_2(); ++i) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ತುದಿ_Scope_Funcps.at(i);
            AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp};
            callp->dtype_ಗಣ_Void();
            callp->argTypes("tracep");
            ಕೂಡಿಸು_ಗಮ್ಯ_ತುದಿ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        ಆದರೆ (m_ತುದಿ_Funcps.ಖಾಲಿಗಳು()) ಕೂಡಿಸು_ಗಮ್ಯ_ತುದಿ_Func(ಹೊಸ AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        ಆದರೆ (m_ತುದಿ_Funcps.ಗಾತ್ರ_2() > 1) {
            AstCFunc* ಸ್ಥಿರಾಂಕ ತುದಿ_Funcp = newCFunc(flp, "");
            ಗಾಗಿ (AstCFunc* funcp : m_ತುದಿ_Funcps) {
                AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp};
                callp->dtype_ಗಣ_Void();
                callp->argTypes("tracep");
                ತುದಿ_Funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
            }
            m_ತುದಿ_Funcps.clear();
            m_ತುದಿ_Funcps.ತಳ್ಳು_ಹಿಂಭಾಗ(ತುದಿ_Funcp);
        }

        // Set name of top level function
        AstCFunc* ಸ್ಥಿರಾಂಕ ತುದಿ_Funcp = m_ತುದಿ_Funcps.front();
        ತುದಿ_Funcp->name("trace_init_top");
        ತುದಿ_Funcp->ನಮೂದು_ಬಿಂದು(ನಿಜ);

        ಆದರೆ (ಬೇರು_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ಬೇರು_Funcp);
        checkCalls(ತುದಿ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::ಕೂಡಿಸು_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

ಶೂನ್ಯ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("tracedecl", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
