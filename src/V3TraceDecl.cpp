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

ವರ್ಗ PathAdjustor final {
    File_ಸಾಲು* ಧ್ರುವ m_flp;  // FileLine used for created nodes
    std::function<ಶೂನ್ಯ(Ast_ಗಂಟು_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_ರಾಶಿ{""};  // Stack of current paths

    ಸ್ಥಿರ constexpr ವರ್ಣ SEPARATOR = ' ';

ಸಾರ್ವಜನಿಕ:
    explicit PathAdjustor(File_ಸಾಲು* flp, std::function<ಶೂನ್ಯ(Ast_ಗಂಟು_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    ಶೂನ್ಯ adjust(ಧ್ರುವ string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        ಯಾವತ್ (!VString::startsWith(newPath, m_ರಾಶಿ.back())) {
            m_emit(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{m_flp});
            m_ರಾಶಿ.ತೆಗೆ_back();
        }

        ಯದಿ (newPath == m_ರಾಶಿ.back()) ಪ್ರತ್ಯಾಗಮ;

        ಧ್ರುವ VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        ಧ್ರುವ std::string extraPrefix = newPath.substr(m_ರಾಶಿ.back().size());
        size_t begin = 0;
        ಧ್ರುವ size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        ಯಾವತ್ (ಸತ್ಯ) {
            ಧ್ರುವ size_t end = extraPrefix.find(SEPARATOR, begin);
            ಯದಿ (end == string::npos) ಭಙ್ಗ;
            ಧ್ರುವ string& extra = extraPrefix.substr(begin, end - begin);
            ಯದಿ (end == last) {
                m_emit(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{m_flp, extra, lastScopeType});
            } ಅನ್ಯಥಾ {
                m_emit(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_ರಾಶಿ.ತಳ್ಳು_back(m_ರಾಶಿ.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    ಶೂನ್ಯ unwind() {
        ಯಾವತ್ (m_ರಾಶಿ.size() > 1) {
            m_emit(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{m_flp});
            m_ರಾಶಿ.ತೆಗೆ_back();
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
    Ast_ತುದಿ_Scope* ಧ್ರುವ m_ತುದಿ_Scopep;  // The singleton AstTopScope
    ಧ್ರುವ AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_ತುದಿ_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<ಧ್ರುವ AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ಸಂರಚನಾ Dtype_Func_ಕೀಲಿ final {
        ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep;
        VVarType varType;
        ತಾರ್ಕಿಕ operator==(ಧ್ರುವ Dtype_Func_ಕೀಲಿ& other) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ dtypep == other.dtypep && varType == other.varType;
        }
    };
    ಸಂರಚನಾ Dtype_Func_ಕೀಲಿ_Hash final {
        size_t operator()(ಧ್ರುವ Dtype_Func_ಕೀಲಿ& ಕೀಲಿ) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ std::hash<ಧ್ರುವ Ast_ಗಂಟು_DType*>{}(ಕೀಲಿ.dtypep)
                   ^ (std::hash<uint8_t>{}(ಕೀಲಿ.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ಕೀಲಿ, AstCFunc*, Dtype_Func_ಕೀಲಿ_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    ಧ್ರುವ string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    ತಾರ್ಕಿಕ m_skipDtypeFunc = ಅಸತ್ಯ;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    ಪೂರ್ಣಾಙ್ಕ m_ತುದಿ_Func_Size = 0;  // Size of the top function currently being built
    ಪೂರ್ಣಾಙ್ಕ m_subFuncSize = 0;  // Size of the sub function currently being built
    ತಾರ್ಕಿಕ m_forceNewLeaf = ಅಸತ್ಯ;  // Force next addToSubFunc to start a fresh leaf
    size_t m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ = 0;  // Top-scope init functions used only for wrapper IOs
    ತಾರ್ಕಿಕ m_ತುದಿ_Scope_ಬೇರು_Phase = ಅಸತ್ಯ;  // Emitting top-scope wrapper IO declarations
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
    // Trace init functions to for each scope
    std::unordered_map<ಧ್ರುವ AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ಧ್ರುವ AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_ಗಂಟು_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_ಗಂಟು_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    ವರ್ಗ TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        ಶೂನ್ಯ init(ಧ್ರುವ std::string& name, Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ in_ತುದಿ_Scope) {
            // Compute path in hierarchy and item name
            ಧ್ರುವ std::string& vcdName = Ast_ಗಂಟು::vcdName(name);
            AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
            ಯದಿ (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } ಅನ್ಯಥಾ ಯದಿ (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } ಅನ್ಯಥಾ {
                // Other Variable
                ಧ್ರುವ size_t pos = vcdName.rfind(' ');
                ಧ್ರುವ size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            ಯದಿ (in_ತುದಿ_Scope && !v3Global.opt.libCreate().empty()) {
                ಧ್ರುವ size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                ಧ್ರುವ std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                ಯದಿ (prefix == "$rootio") ಪ್ರತ್ಯಾಗಮ;
                m_path = m_path.substr(start + 1);
                ಯದಿ (v3Global.rootp()->trace_Lib_ತುದಿ_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    ಸಾರ್ವಜನಿಕ:
        explicit TraceEntry(ಧ್ರುವ AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_ತುದಿ());
        }
        explicit TraceEntry(ಧ್ರುವ AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_ತುದಿ());
        }
        ಪೂರ್ಣಾಙ್ಕ operatorCompare(ಧ್ರುವ TraceEntry& b) ಧ್ರುವ {
            ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ cmp = path().compare(b.path())) ಪ್ರತ್ಯಾಗಮ cmp < 0;
            ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ cmp = fileline().operatorCompare(b.fileline())) ಪ್ರತ್ಯಾಗಮ cmp < 0;
            ಪ್ರತ್ಯಾಗಮ name() < b.name();
        }
        AstVarScope* vscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp; }
        AstCell* cellp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cellp; }
        ಧ್ರುವ std::string& path() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_path; }
        ಶೂನ್ಯ path(ಧ್ರುವ std::string& path) { m_path = path; }
        ಧ್ರುವ std::string& name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
        File_ಸಾಲು& fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ಧ್ರುವ AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (!varp->isTrace()) ಪ್ರತ್ಯಾಗಮ "Verilator trace_off";
        ಯದಿ (!nodep->isTrace()) ಪ್ರತ್ಯಾಗಮ "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        ಯದಿ (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            ಪ್ರತ್ಯಾಗಮ "Automatic variable";

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = recurseDTypeWidth(nodep->varp()->dtypep());
        ಯದಿ (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            ಪ್ರತ್ಯಾಗಮ "Width " + cvtToStr(width) + " > --trace-max-width";

        ಧ್ರುವ string prettyName = nodep->prettyName();
        ಯದಿ (!V3Control::getScopeTraceOn(prettyName)) ಪ್ರತ್ಯಾಗಮ "Vlt scope trace_off";
        ಯದಿ (!v3Global.opt.traceUnderscore()) {
            ಯದಿ (!prettyName.empty() && prettyName[0] == '_') ಪ್ರತ್ಯಾಗಮ "Leading underscore";
            ಯದಿ (prettyName.find("._") != string::npos) ಪ್ರತ್ಯಾಗಮ "Inlined leading underscore";
        }
        ಪ್ರತ್ಯಾಗಮ ""s;
    }

    ಪೂರ್ಣಾಙ್ಕ recurseDTypeWidth(ಧ್ರುವ Ast_ಗಂಟು_DType* nodep) {
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* adtypep = VN_CAST(nodep, ಗಂಟು_Array_DType))
            ಪ್ರತ್ಯಾಗಮ recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        ಪ್ರತ್ಯಾಗಮ nodep->width();
    }

    AstCFunc* newCFunc(File_ಸಾಲು* flp, ಧ್ರುವ string& name) {
        AstScope* ಧ್ರುವ ತುದಿ_Scopep = m_ತುದಿ_Scopep->scopep();
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{flp, name, ತುದಿ_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(ಸತ್ಯ);
        funcp->isStatic(ಅಸತ್ಯ);
        funcp->isLoose(ಸತ್ಯ);
        funcp->slow(ಸತ್ಯ);
        ತುದಿ_Scopep->addBlocksp(funcp);
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    ಶೂನ್ಯ add_To_ತುದಿ_Func(Ast_ಗಂಟು_Stmt* stmtp) {
        ಯದಿ (m_ತುದಿ_Func_Size > m_funcSizeLimit || m_ತುದಿ_Funcps.empty()) {
            m_ತುದಿ_Func_Size = 0;
            //
            ಧ್ರುವ string n = cvtToStr(m_ತುದಿ_Funcps.size());
            ಧ್ರುವ string name{"trace_init_top__" + n};
            AstCFunc* ಧ್ರುವ funcp = newCFunc(m_ತುದಿ_Scopep->fileline(), name);
            m_ತುದಿ_Funcps.ತಳ್ಳು_back(funcp);
        }
        m_ತುದಿ_Funcps.back()->addStmtsp(stmtp);
        m_ತುದಿ_Func_Size += stmtp->ಗಂಟು_ಎಣಿಕೆ();
    }

    ಶೂನ್ಯ addToSubFunc(Ast_ಗಂಟು_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        ಯದಿ (m_dtypeSubFuncp) {
            ಯದಿ (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->ಗಂಟು_ಎಣಿಕೆ();
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Defer trace splitting until V3Trace
        ಯದಿ (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = ಅಸತ್ಯ;
            File_ಸಾಲು* ಧ್ರುವ flp = m_currScopep->fileline();
            ಧ್ರುವ string n = cvtToStr(m_subFuncps.size());
            ಧ್ರುವ string name
                = m_currScopep == m_ತುದಿ_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_ತುದಿ_Scope_ಬೇರು_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ಧ್ರುವ funcp = newCFunc(flp, name);
            funcp->addStmtsp(ನೂತನ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ತಳ್ಳು_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->ಗಂಟು_ಎಣಿಕೆ();
    }

    AstTraceDecl* addTraceDecl(ಧ್ರುವ VNumRange& arrayRange,
                               ಪೂರ್ಣಾಙ್ಕ widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ಧ್ರುವ dtypeCallp = nullptr) {
        VNumRange bitRange;
        ಯದಿ (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_ಸಾಲು* ಧ್ರುವ flp = m_traVscp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = m_traValuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        ಧ್ರುವ ತಾರ್ಕಿಕ validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ಧ್ರುವ newp
            = ನೂತನ AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        ಯದಿ (validOffset) {
            newp->code(m_offset);
            ಯದಿ (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* ಧ್ರುವ refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(ನೂತನ AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
            });
        } ಅನ್ಯಥಾ {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        ಪ್ರತ್ಯಾಗಮ newp;
    }

    ಶೂನ್ಯ addIgnore(ಧ್ರುವ string& why) {
        ++m_statIgnSigs;
        ಧ್ರುವ std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        ಯದಿ (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    ಶೂನ್ಯ fixupPlaceholder(ಧ್ರುವ std::string& path, Ast_ಗಂಟು_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ಧ್ರುವ auto it = m_pathToScopep.find(Ast_ಗಂಟು::prettyName(path));
        ಯದಿ (it != m_pathToScopep.end()) {
            ಧ್ರುವ AstScope* ಧ್ರುವ scopep = it->second;
            File_ಸಾಲು* ಧ್ರುವ flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ಗಂಟು* stmtp = nullptr;
            ಪ್ರತಿ (AstCFunc* ಧ್ರುವ subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_ಗಂಟು::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            ಯದಿ (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_ವೃಕ್ಷ(), placeholderp);
    }

    ಶೂನ್ಯ fixupLibStub(ಧ್ರುವ std::string& path, Ast_ಗಂಟು_Stmt* placeholderp) {
        File_ಸಾಲು* ಧ್ರುವ flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ಗಂಟು::prettyName(path);
        ಯದಿ (!v3Global.opt.libCreate().empty()) {
            ಧ್ರುವ std::string ತುದಿ_Prefix = v3Global.rootp()->trace_Lib_ತುದಿ_Name() + ".";
            ಯದಿ (VString::startsWith(name, ತುದಿ_Prefix)) name = name.substr(ತುದಿ_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* ಧ್ರುವ initp = ನೂತನ AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(ನೂತನ AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_ವೃಕ್ಷ(), placeholderp);
    }

    ಶೂನ್ಯ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        ಪ್ರತಿ (ಧ್ರುವ auto& item : m_cellInitPlaceholders) {
            ಧ್ರುವ AstScope* ಧ್ರುವ parentp = std::get<0>(item);
            ಧ್ರುವ AstCell* ಧ್ರುವ cellp = std::get<1>(item);
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ placeholderp = std::get<2>(item);
            ಧ್ರುವ std::string path = parentp->name() + "__DOT__" + cellp->name();
            ಯದಿ (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } ಅನ್ಯಥಾ {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        ಪ್ರತಿ (ಧ್ರುವ auto& item : m_ifaceRefInitPlaceholders) {
            ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp = std::get<0>(item);
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ placeholderp = std::get<1>(item);
            ಧ್ರುವ std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    ಶೂನ್ಯ remove_Redundant_Prefix_ತಳ್ಳು_ತೆಗೆ() {
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            ಪ್ರತಿ (AstCFunc* ಧ್ರುವ funcp : pair.second) {
                Ast_ಗಂಟು* prevp = nullptr;
                Ast_ಗಂಟು* currp = funcp->stmtsp();
                ಯಾವತ್ (currp) {
                    Ast_ಗಂಟು* ಧ್ರುವ nextp = currp->nextp();
                    ಯದಿ (VN_IS(prevp, Trace_ತಳ್ಳು_Prefix) && VN_IS(currp, Trace_ತೆಗೆ_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_ವೃಕ್ಷ(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_ವೃಕ್ಷ(), currp);
                    }
                    ಯದಿ (!nextp) ಭಙ್ಗ;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    ಶೂನ್ಯ checkCalls(ಧ್ರುವ AstCFunc* funcp) {
        ಯದಿ (!v3Global.opt.debugCheck()) ಪ್ರತ್ಯಾಗಮ;
        checkCallsRecurse(funcp);
        ಯದಿ (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            ಪ್ರತಿ (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    ಶೂನ್ಯ checkCallsRecurse(ಧ್ರುವ AstCFunc* funcp) {
        funcp->foreach([ಏತತ್](ಧ್ರುವ Ast_ಗಂಟು* nodep) {
            ಯದಿ (ಧ್ರುವ AstTraceDecl* ಧ್ರುವ declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCCall* ಧ್ರುವ ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    ತಾರ್ಕಿಕ isBasicIO() { ಪ್ರತ್ಯಾಗಮ m_traVscp->varp()->isVLIO(); }
    ಶೂನ್ಯ newDeclSubFunc() {
        File_ಸಾಲು* ಧ್ರುವ flp = m_dtypeFuncp->fileline();
        AstCFunc* ಧ್ರುವ subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* ಧ್ರುವ subCallp = ನೂತನ AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    ಶೂನ್ಯ newDeclFunc(Ast_ಗಂಟು_DType* nodep) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಧ್ರುವ flp = skipTypep->fileline();

        ಧ್ರುವ Dtype_Func_ಕೀಲಿ dtype_ಕೀಲಿ{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_ಕೀಲಿ, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        ಯದಿ (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        ಯದಿ (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            ಯದಿ (AstStructDType* ಧ್ರುವ dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (AstPackArrayDType* ಧ್ರುವ dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, ಸತ್ಯ);
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(ಅಸತ್ಯ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    ಶೂನ್ಯ declUnpackedArray(AstUnpackArrayDType* ಧ್ರುವ nodep, ತಾರ್ಕಿಕ newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        addToSubFunc(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->ಎಡ(), nodep->ಬಲ(), !newFunc});

        ಯದಿ (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            ಯದಿ (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } ಅನ್ಯಥಾ {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ i : nodep->declRange().seq_ಎಡ_To_ಬಲ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                m_traValuep = ನೂತನ AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
            }
        }

        addToSubFunc(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{flp});
    }
    ಶೂನ್ಯ declPackedArray(AstPackArrayDType* ಧ್ರುವ nodep, ತಾರ್ಕಿಕ newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        addToSubFunc(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->ಎಡ(), nodep->ಬಲ(), !newFunc});

        // Always iterate left index to right index
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ i : nodep->declRange().seq_ಎಡ_To_ಬಲ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            m_traValuep = ನೂತನ AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep = nullptr);
        }

        addToSubFunc(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{flp});
    }
    ಶೂನ್ಯ declStruct(AstStructDType* ಧ್ರುವ nodep, ತಾರ್ಕಿಕ newFunc) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        ಪೂರ್ಣಾಙ್ಕ nMembers = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        ಯದಿ (!nodep->packed()) {
            addToSubFunc(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                m_traValuep = ನೂತನ AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
            }
            addToSubFunc(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{flp});
        } ಅನ್ಯಥಾ {
            addToSubFunc(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                m_traValuep = ನೂತನ AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
            }
            addToSubFunc(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{flp});
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        ಯದಿ (nodep->modp()->verilatorLib()) ಪ್ರತ್ಯಾಗಮ;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstCell* ಧ್ರುವ cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        ಯದಿ (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](ಧ್ರುವ TraceEntry& a, ಧ್ರುವ TraceEntry& b) { ಪ್ರತ್ಯಾಗಮ a.operatorCompare(b); });

            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_ಗಂಟು_Stmt* stmtp) { addToSubFunc(stmtp); }};
            ಧ್ರುವ ತಾರ್ಕಿಕ split_ಬೇರು_Primary_Ios = nodep->is_ತುದಿ() && !v3Global.opt.libCreate().empty();
            ಧ್ರುವ auto emitEntry = [&](ಧ್ರುವ TraceEntry& entry) {
                AstVarScope* ಧ್ರುವ vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_ಗಂಟು*>(entry.cellp())
                                                    : static_cast<Ast_ಗಂಟು*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                ಯದಿ (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ಧ್ರುವ string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    ಯದಿ (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } ಅನ್ಯಥಾ {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = ನೂತನ AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        ಯದಿ (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_ವೃಕ್ಷ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } ಅನ್ಯಥಾ {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ಧ್ರುವ cellp = entry.cellp();
                    Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = ನೂತನ AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            ಯದಿ (split_ಬೇರು_Primary_Ios) {
                m_ತುದಿ_Scope_ಬೇರು_Phase = ಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ TraceEntry& entry : m_entries) {
                    AstVarScope* ಧ್ರುವ vscp = entry.vscp();
                    ಯದಿ (!(vscp && vscp->varp()->isPrimaryIO())) ಅನುವರ್ತ;
                    emitEntry(entry);
                }
                m_ತುದಿ_Scope_ಬೇರು_Phase = ಅಸತ್ಯ;
                pathAdjustor.unwind();
                m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ = m_subFuncps.size();
                ಯದಿ (m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = ಸತ್ಯ;
                }
            }
            ಪ್ರತಿ (ಧ್ರುವ TraceEntry& entry : m_entries) {
                AstVarScope* ಧ್ರುವ vscp = entry.vscp();
                ಯದಿ (split_ಬೇರು_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) ಅನುವರ್ತ;
                emitEntry(entry);
            }
            pathAdjustor.unwind();
            m_traVscp = nullptr;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ಧ್ರುವ std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ಧ್ರುವ AstCell* ಧ್ರುವ cellp = nodep->aboveCellp();
        ಯದಿ (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ಧ್ರುವ size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            ಧ್ರುವ std::string parentPath = path.substr(0, lastDot + 1);

            ಪ್ರತಿ (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ಧ್ರುವ std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                ಯದಿ (!VString::startsWith(refName, parentPath)) ಅನುವರ್ತ;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                ಯದಿ (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_ವೃಕ್ಷ(), intfRefp);
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        ಯದಿ (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->varp()->isClassMember()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->varp()->isFuncLocal()) ಪ್ರತ್ಯಾಗಮ;

        // When creating a --lib-create library ...
        ಯದಿ (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            ಯದಿ (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) ಪ್ರತ್ಯಾಗಮ;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    ಶೂನ್ಯ visit(AstConstDType* nodep) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ಸತ್ಯ;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    ಶೂನ್ಯ visit(AstIfaceRefDType* /*nodep*/) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;
        // Insert a placeholder to be fixed up later
        File_ಸಾಲು* ಧ್ರುವ flp = m_traVscp->fileline();
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp
            = ನೂತನ AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    ಶೂನ್ಯ visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (v3Global.opt.traceMaxArray()
            && static_cast<ಪೂರ್ಣಾಙ್ಕ>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        ಯದಿ (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            ಪ್ರತ್ಯಾಗಮ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ಯದಿ (isBasicIO()) m_skipDtypeFunc = ಸತ್ಯ;

        ಯದಿ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstPackArrayDType* nodep) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ಪ್ರತ್ಯಾಗಮ;
        }

        Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        ಯದಿ (AstBasicDType* ಧ್ರುವ basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            ಯದಿ (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ಯದಿ (isBasicIO()) m_skipDtypeFunc = ಸತ್ಯ;

        ಯದಿ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstStructDType* nodep) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ಪ್ರತ್ಯಾಗಮ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ಯದಿ (isBasicIO()) m_skipDtypeFunc = ಸತ್ಯ;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        ಯದಿ (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstUnionDType* nodep) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ಸತ್ಯ;

        ಯದಿ (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ಪ್ರತ್ಯಾಗಮ;
        }

        VL_RESTORER_COPY(m_traName);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        ಪೂರ್ಣಾಙ್ಕ nMembers = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        ಯದಿ (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } ಅನ್ಯಥಾ {
            addToSubFunc(ನೂತನ Ast_Trace_ತಳ್ಳು_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(ನೂತನ Ast_Trace_ತೆಗೆ_Prefix{flp});
        }
    }
    ಶೂನ್ಯ visit(AstBasicDType* nodep) override {
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } ಅನ್ಯಥಾ {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    ಶೂನ್ಯ visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType*) override {
        // Note more specific dtypes above
        ಯದಿ (!m_traVscp) ಪ್ರತ್ಯಾಗಮ;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_ತುದಿ_Scopep{nodep->ತುದಿ_Scopep()} {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterateAndNextConstNull(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_Prefix_ತಳ್ಳು_ತೆಗೆ();

        ಧ್ರುವ std::vector<AstCFunc*>& ತುದಿ_Scope_Funcps = m_scopeInitFuncps.at(m_ತುದಿ_Scopep->scopep());
        AstCFunc* ಬೇರು_Funcp = nullptr;
        ಯದಿ (!v3Global.opt.libCreate().empty()) {
            ಬೇರು_Funcp = newCFunc(flp, "trace_init_root");
            ಬೇರು_Funcp->entryPoint(ಸತ್ಯ);
            ಪ್ರತಿ (size_t i = 0; i < m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ; ++i) {
                AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, ತುದಿ_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                ಬೇರು_Funcp->addStmtsp(callp->makeStmt());
            }
            ಯದಿ (!m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ) ಬೇರು_Funcp->addStmtsp(ನೂತನ AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        ಪ್ರತಿ (size_t i = m_ತುದಿ_Scope_ಬೇರು_Func_ಎಣಿಕೆ; i < ತುದಿ_Scope_Funcps.size(); ++i) {
            AstCFunc* ಧ್ರುವ funcp = ತುದಿ_Scope_Funcps.at(i);
            AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_ತುದಿ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        ಯದಿ (m_ತುದಿ_Funcps.empty()) add_To_ತುದಿ_Func(ನೂತನ AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        ಯದಿ (m_ತುದಿ_Funcps.size() > 1) {
            AstCFunc* ಧ್ರುವ ತುದಿ_Funcp = newCFunc(flp, "");
            ಪ್ರತಿ (AstCFunc* funcp : m_ತುದಿ_Funcps) {
                AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                ತುದಿ_Funcp->addStmtsp(callp->makeStmt());
            }
            m_ತುದಿ_Funcps.clear();
            m_ತುದಿ_Funcps.ತಳ್ಳು_back(ತುದಿ_Funcp);
        }

        // Set name of top level function
        AstCFunc* ಧ್ರುವ ತುದಿ_Funcp = m_ತುದಿ_Funcps.front();
        ತುದಿ_Funcp->name("trace_init_top");
        ತುದಿ_Funcp->entryPoint(ಸತ್ಯ);

        ಯದಿ (ಬೇರು_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ಬೇರು_Funcp);
        checkCalls(ತುದಿ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

ಶೂನ್ಯ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("tracedecl", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
