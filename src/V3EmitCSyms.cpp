// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitC.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3LanguageWords.h"
#ಅನ್ತರ್ಭಾವ "V3StackCount.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cstring>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
ಸ್ಥಿರ constexpr ಸ್ವಯಂಚಾಲಿತ symClassName = &EmitCUtil::symClassName;
ಸ್ಥಿರ constexpr ಸ್ವಯಂಚಾಲಿತ ತುದಿ_Class_Name = &EmitCUtil::ತುದಿ_Class_Name;

ವರ್ಗ EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // TYPES
    ರಚನೆ Scope_ದತ್ತಾಂಶ final {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_nodep;  // Scope symbol-table entry corresponds to
        ಸ್ಥಿರಾಂಕ std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ಸ್ಥಿರಾಂಕ std::string m_prettyName;  // Pretty (unprotected) name for display
        ಸ್ಥಿರಾಂಕ std::string m_defName;  // Name registered for this scope
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ std::string& symName, ಸ್ಥಿರಾಂಕ std::string& prettyName,
                  ಸ್ಥಿರಾಂಕ std::string& defName, ಪೂರ್ಣಾಂಕ timeunit, ಸ್ಥಿರಾಂಕ std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    ರಚನೆ Iface_Ref_ದತ್ತಾಂಶ final {
        ಸ್ಥಿರಾಂಕ AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_ಪೋಷಕ_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ AstScope* scopep, ಸ್ಥಿರಾಂಕ std::string& ಪೋಷಕ_Sym,
                     ಸ್ಥಿರಾಂಕ std::string& suffix, ಸ್ಥಿರಾಂಕ std::string& name,
                     ಸ್ಥಿರಾಂಕ std::string& modportName)
            : m_scopep{scopep}
            , m_ಪೋಷಕ_Sym{ಪೋಷಕ_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    ರಚನೆ Scope_Func_ದತ್ತಾಂಶ final {
        ಸ್ಥಿರಾಂಕ AstScopeName* ಸ್ಥಿರಾಂಕ m_scopep;  // Scope the DPI export function is registered under
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ m_cfuncp;  // DPI export function
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module containing the DPI export function
        Scope_Func_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ AstScopeName* scopep, ಸ್ಥಿರಾಂಕ AstCFunc* funcp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    ರಚನೆ Scope_Var_ದತ್ತಾಂಶ final {
        ಸ್ಥಿರಾಂಕ std::string m_scopeName;  // Full name of the scope containing the variable
        ಸ್ಥಿರಾಂಕ std::string m_var_ತಳ_Pretty;  // Pretty base name of the variable, without scope
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ m_varp;  // Public variable
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module containing the variable
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // Scope containing the variable
        Scope_Var_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ std::string& scopeName, ಸ್ಥಿರಾಂಕ std::string& var_ತಳ_Pretty,
                     ಸ್ಥಿರಾಂಕ AstVar* varp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_ತಳ_Pretty{var_ತಳ_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    ಬಳಸಿ Scope_ಹೆಸರುಗಳು = std::map<ಸ್ಥಿರಾಂಕ std::string, Scope_ದತ್ತಾಂಶ>;
    ಬಳಸಿ ScopeModPair = std::pair<ಸ್ಥಿರಾಂಕ AstScope*, Ast_ಗಂಟು_Module*>;
    ಬಳಸಿ ModVarPair = std::pair<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*, ಸ್ಥಿರಾಂಕ AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ VPI_ಕೋಷ್ಟಕ_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current function
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ಸ್ಥಿರಾಂಕ AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    std::ಸದಿಶ<ScopeModPair> m_scopes;  // Every scope by module
    std::ಸದಿಶ<AstCFunc*> m_dpis;  // DPI functions
    std::ಸದಿಶ<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ಸ್ಥಿರಾಂಕ std::string, Scope_Func_ದತ್ತಾಂಶ> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ಸ್ಥಿರಾಂಕ std::string, Scope_Var_ದತ್ತಾಂಶ> m_scopeVars;  // Each {scope,public-var}
    Scope_ಹೆಸರುಗಳು m_scope_ಹೆಸರುಗಳು;  // Each unique AstScopeName. Dpi scopes added later
    Scope_ಹೆಸರುಗಳು m_dpi_Scope_ಹೆಸರುಗಳು;  // Each unique AstScopeName for DPI export
    Scope_ಹೆಸರುಗಳು m_vpiScopeCandidates;  // All scopes for VPI
    std::ಸದಿಶ<Iface_Ref_ದತ್ತಾಂಶ> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ಸ್ಥಿರಾಂಕ std::string, std::ಸದಿಶ<std::string>> m_vpiScopeHierarchy;
    ಪೂರ್ಣಾಂಕ m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    ಪೂರ್ಣಾಂಕ m_modCoverBins = 0;  // Per-module coverage bin number
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_ಬೈಟುಗಳು;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::ಸದಿಶ<std::pair<std::string, std::ಸದಿಶ<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_ಕೋಷ್ಟಕ_Name;
    std::ಸದಿಶ<std::string> m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು;
    std::string m_iface_Ref_ಕೋಷ್ಟಕ_Name;
    std::ಸದಿಶ<std::string> m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು;

    // METHODS
    ಶೂನ್ಯ emitSymHdr();
    ಶೂನ್ಯ emitSymImpPreamble();
    ಶೂನ್ಯ emitVarTables();
    ಶೂನ್ಯ emitScopeHier(ಪಠ್ಯ_Stmts& stmts, ತಾರ್ಕಿಕ destroy);
    ಶೂನ್ಯ emitSymImp(ಸ್ಥಿರಾಂಕ AstNetlist* netlistp);
    ಶೂನ್ಯ emitDpiHdr();
    ಶೂನ್ಯ emitDpiStruct(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep, std::ಗಣ<std::string>& namesr);
    ಶೂನ್ಯ emitDpiImp();

    ಪಠ್ಯ_Stmts getSymCtorStmts();
    ಪಠ್ಯ_Stmts getSymDtorStmts();

    ಸ್ಥಿರ ಶೂನ್ಯ nameCheck(Ast_ಗಂಟು* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        ಆದರೆ (nodep->name().ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = VN_CAST(nodep, CFunc);
        ಆದರೆ (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ಸ್ಥಿರಾಂಕ std::string rsvd = V3_Language_ಪದಗಳು::isKeyword(nodep->name());
            ಆದರೆ (rsvd != "") {
                // Generally V3Name should find all of these and throw SYMRSVDWORD.
                // We'll still check here because the compiler errors
                // resulting if we miss this warning are SO nasty
                nodep->v3error("Symbol matching " + rsvd
                                   + " reserved word reached emitter,"
                                     " should have hit SYMRSVDWORD: "
                               << nodep->prettyNameQ());
            }
        }
    }

    ಸ್ಥಿರ std::string scopeSymString(ಸ್ಥಿರಾಂಕ std::string& scpname) {
        std::string ಹೊರಹೋಗು = scpname;
        std::string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = ಹೊರಹೋಗು.ಕಂಡುಹಿಡಿ("__PVT__")) != std::string::npos) ಹೊರಹೋಗು.replace(pos, 7, "");
        ಆದರೆ (VString::startsWith(ಹೊರಹೋಗು, "TOP__DOT__")) ಹೊರಹೋಗು.replace(0, 10, "");
        ಆದರೆ (VString::startsWith(ಹೊರಹೋಗು, "TOP.")) ಹೊರಹೋಗು.replace(0, 4, "");
        ಆಗ ((pos = ಹೊರಹೋಗು.ಕಂಡುಹಿಡಿ('.')) != std::string::npos) ಹೊರಹೋಗು.replace(pos, 1, "__");
        ಆಗ ((pos = ಹೊರಹೋಗು.ಕಂಡುಹಿಡಿ("__DOT__")) != std::string::npos) ಹೊರಹೋಗು.replace(pos, 7, "__");
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
    }

    ಸ್ಥಿರ string scopeDecodeIdentifier(ಸ್ಥಿರಾಂಕ std::string& scpname) {
        std::string::ಗಾತ್ರ_type pos = std::string::npos;

        // Remove hierarchy
        ಗಾತ್ರ_t i = 0;
        // always makes progress
        ಆಗ (i < scpname.ಉದ್ದಳತೆ()) {
            ಆದರೆ (scpname[i] == '\\') {
                ಆಗ (i < scpname.ಉದ್ದಳತೆ() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆಗ (i < scpname.ಉದ್ದಳತೆ() && scpname[i] != '.') ++i;
                ಆದರೆ (i < scpname.ಉದ್ದಳತೆ()) pos = i++;
            }
        }

        ಹಿಂತಿರುಗಿ pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    ರಚನೆ VarDims final {
        ಪೂರ್ಣಾಂಕ pdim = 0;  // Packed array dims plus a ranged basic leaf
        ಪೂರ್ಣಾಂಕ udim = 0;
        std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> unpackedLR;  // (left,right), outer-first
        std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> flatten() ಸ್ಥಿರಾಂಕ {
            std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> flat(unpackedLR);
            flat.ಸೇರಿಸು(flat.ಕೊನೆ(), packedLR.cbegin(), packedLR.cend());
            ಹಿಂತಿರುಗಿ flat;
        }
    };
    ಸ್ಥಿರ VarDims getVarDims(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಬೇರು_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = ಬೇರು_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                ಆದರೆ (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_ಹಿಂಭಾಗ(adtypep->ಎಡ(), adtypep->ಬಲ());
                    ++d.pdim;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    d.unpackedLR.emplace_ಹಿಂಭಾಗ(adtypep->ಎಡ(), adtypep->ಬಲ());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp()) {
                    ಆದರೆ (basicp->isRanged()) {
                        d.packedLR.emplace_ಹಿಂಭಾಗ(basicp->ಎಡ(), basicp->ಬಲ());
                        ++d.pdim;
                    }
                }
                ನಿಲ್ಲಿಸು;  // Non-array leaf
            }
        }
        ಹಿಂತಿರುಗಿ d;
    }

    ಸ್ಥಿರ VarDims dimsFor(ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd) { ಹಿಂತಿರುಗಿ getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ಸ್ಥಿರ std::string boundsString(ಸ್ಥಿರಾಂಕ VarDims& d) {
        std::string bounds;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::ಗಮ್ಯ_string(lr.ಮೊದಲ);
            bounds += ",";
            bounds += std::ಗಮ್ಯ_string(lr.ಎರಡನೇ);
        }
        ಹಿಂತಿರುಗಿ bounds;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ getUnpackedElements(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಬೇರು_Dtypep) {
        ಪೂರ್ಣಾಂಕ elements = 1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = ಬೇರು_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, UnpackArrayDType);
            ಆದರೆ (!adtypep) ನಿಲ್ಲಿಸು;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        ಹಿಂತಿರುಗಿ elements;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ needs_Emitted_Ent_ಗಾತ್ರ(ಸ್ಥಿರಾಂಕ std::string& vlEnumType) {
        ಹಿಂತಿರುಗಿ vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ಸ್ಥಿರ std::pair<ತಾರ್ಕಿಕ, std::string> isForceControlSignal(ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ಸ್ಥಿರಾಂಕ std::ಗಾತ್ರ_t suffixPos = signalVarp->name().ಕಂಡುಹಿಡಿ(forceControlSuffix);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ suffix_ಸಿಕ್ಕಿತು = suffixPos != std::string::npos;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ suffix_Is_At_ಕೊನೆ
                = suffixPos + forceControlSuffix.ಉದ್ದಳತೆ() == signalVarp->name().ಉದ್ದಳತೆ();
            ಆದರೆ (!(suffix_ಸಿಕ್ಕಿತು && suffix_Is_At_ಕೊನೆ)) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ std::string ತಳ_Signal_Name = signalVarp->name().substr(0, suffixPos);
            ಹಿಂತಿರುಗಿ std::pair<ತಾರ್ಕಿಕ, std::string>{ನಿಜ, ತಳ_Signal_Name};
        }
        ಹಿಂತಿರುಗಿ std::pair<ತಾರ್ಕಿಕ, std::string>{ಸುಳ್ಳು, ""};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ appendVarProperties(std::string& stmt, ಸ್ಥಿರಾಂಕ std::string& vlEnumType,
                                    ಸ್ಥಿರಾಂಕ std::string& vlEnumDir, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ udim, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ pdim,
                                    ಸ್ಥಿರಾಂಕ std::string& bounds, ಸ್ಥಿರಾಂಕ std::string& ent_ಗಾತ್ರ = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::ಗಮ್ಯ_string(udim);
        ಆದರೆ (!ent_ಗಾತ್ರ.ಖಾಲಿಗಳು()) {
            stmt += ", ";
            stmt += ent_ಗಾತ್ರ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += ", ";
            stmt += std::ಗಮ್ಯ_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ಸ್ಥಿರ std::string memberVlEnumDir(ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp,
                                       ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        std::string ಹೊರಹೋಗು = '(' + varp->vlEnumDir(/*forMember=*/ನಿಜ);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ skipDTypep = dtypep->skipRefp();
        ಆದರೆ (skipDTypep->isSigned()) ಹೊರಹೋಗು += "|VLVF_SIGNED";
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = skipDTypep->basicp()) {
            ಆದರೆ (basicp->keyword() == VBasicDTypeKwd::ಬಿಟ್) ಹೊರಹೋಗು += "|VLVF_BITVAR";
        }
        ಹೊರಹೋಗು += skipDTypep->vlEnumPackedAgg();
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು + ')';
    }

    // Name of the instance member for the given scope
    ಸ್ಥಿರ std::string instName(ಸ್ಥಿರಾಂಕ AstScope* scopep) {
        ಹಿಂತಿರುಗಿ "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ಸ್ಥಿರ std::string ಸೇರಿಸು_Var_Statement(ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd, ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                          ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ udim, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ pdim,
                                          ಸ್ಥಿರಾಂಕ std::string& bounds) {
        ಸ್ಥಿರಾಂಕ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ಸ್ಥಿರಾಂಕ std::string vlEnumType = varp->vlEnumType();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needs_Ent_ಗಾತ್ರ = needs_Emitted_Ent_ಗಾತ್ರ(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_ಗಾತ್ರ ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(protect(svd.m_var_ತಳ_Pretty)) + '"';

        ಆದರೆ (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "), false, ";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += ".c_str())), true, ";
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "))), true, ";
        }

        ಸ್ಥಿರಾಂಕ std::string ent_ಗಾತ್ರ = needs_Ent_ಗಾತ್ರ
                                        ? "sizeof(" + var_ಅಭಿವ್ಯಕ್ತಿ + ") / "
                                              + std::ಗಮ್ಯ_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_ಗಾತ್ರ);
        ಹಿಂತಿರುಗಿ stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    ಎಣಿಕೆ ವರ್ಗ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ : uint8_t {
        ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲು,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ try_Build_ಕೋಷ್ಟಕ_ನಮೂದು(ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd, ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp,
                                      ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                      ಸ್ಥಿರಾಂಕ std::string& modClassName, ಸ್ಥಿರಾಂಕ VarDims& dims,
                                      std::string& ಅಡ್ಡಸಾಲು_ಹೊರಹೋಗು) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ pdim = dims.pdim;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ udim = dims.udim;
        ಆದರೆ (varp->isForceable() && force_Control_ಸಂಕೇತಗಳು_Are_Valid(scopep, varp)) {
            ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::FORCEABLE_RESIDUAL;
        }
        ಆದರೆ (udim + pdim > VPI_ಕೋಷ್ಟಕ_MAX_DIMS) ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::PLAIN_RESIDUAL;

        ಸ್ಥಿರಾಂಕ std::string vlEnumType = varp->vlEnumType();
        ಆದರೆ (needs_Emitted_Ent_ಗಾತ್ರ(vlEnumType))
            ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        ಆದರೆ (varp->isParam()) ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::PLAIN_RESIDUAL;

        ಸ್ಥಿರಾಂಕ std::string name = V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(protect(svd.m_var_ತಳ_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ಸ್ಥಿರಾಂಕ std::string member = varName(varp);
        ಸ್ಥಿರಾಂಕ std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> dv;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>& lr : dims.flatten()) {
            dv.ತಳ್ಳು_ಹಿಂಭಾಗ(lr.ಮೊದಲ);
            dv.ತಳ್ಳು_ಹಿಂಭಾಗ(lr.ಎರಡನೇ);
        }

        std::string ಅಡ್ಡಸಾಲು = "{\"" + name + "\", ";
        ಅಡ್ಡಸಾಲು += "offsetof(" + modClassName + ", " + member + "), ";
        ಅಡ್ಡಸಾಲು += vlEnumType + ", ";
        ಅಡ್ಡಸಾಲು += "(" + dir + "), ";
        ಅಡ್ಡಸಾಲು += std::ಗಮ್ಯ_string(udim) + ", " + std::ಗಮ್ಯ_string(pdim) + ", {";
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VPI_ಕೋಷ್ಟಕ_MAX_DIMS * 2; ++i) {
            ಆದರೆ (i) ಅಡ್ಡಸಾಲು += ", ";
            ಅಡ್ಡಸಾಲು += std::ಗಮ್ಯ_string(i < static_cast<ಪೂರ್ಣಾಂಕ>(dv.ಗಾತ್ರ_2()) ? dv[i] : 0);
        }
        ಅಡ್ಡಸಾಲು += "}}";
        ಅಡ್ಡಸಾಲು_ಹೊರಹೋಗು = ಅಡ್ಡಸಾಲು;
        ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲು;
    }

    ಸ್ಥಿರ std::string ಸೇರಿಸು_DType_Var_Statement(ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd,
                                               ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                               ಸ್ಥಿರಾಂಕ std::string& prettyName,
                                               ಸ್ಥಿರಾಂಕ std::string& cName,
                                               ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ udim,
                                               ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ pdim, ಸ್ಥಿರಾಂಕ std::string& bounds) {
        ಸ್ಥಿರಾಂಕ std::string vlEnumType = dtypep->vlEnumType();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needs_Ent_ಗಾತ್ರ = needs_Emitted_Ent_ಗಾತ್ರ(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_ಗಾತ್ರ ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(prettyName) + '"';
        ಸ್ಥಿರಾಂಕ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        ಆದರೆ (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "))), true, ";
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += ", &(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "), false, ";
        }
        ಸ್ಥಿರಾಂಕ std::string ent_ಗಾತ್ರ
            = needs_Ent_ಗಾತ್ರ
                  ? "sizeof(" + var_ಅಭಿವ್ಯಕ್ತಿ + ") / " + std::ಗಮ್ಯ_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_ಗಾತ್ರ);
        ಹಿಂತಿರುಗಿ stmt;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_UOr_Struct_Member_Vars(std::ಸದಿಶ<std::string>& stmts, ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd,
                                       ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                       ಸ್ಥಿರಾಂಕ std::string& pretty_ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ std::string& c_ಪೂರ್ವಪ್ರತ್ಯಯ,
                                       ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಂಶ_DTypep = itemp->dtypep();
            ಆದರೆ (ಅಂಶ_DTypep->vlEnumType().ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ std::string prettyName
                = pretty_ಪೂರ್ವಪ್ರತ್ಯಯ + "." + Ast_ಗಂಟು::vpiName(itemp->shortName());
            ಸ್ಥಿರಾಂಕ std::string cName = c_ಪೂರ್ವಪ್ರತ್ಯಯ + "." + itemp->nameProtect();
            ಸ್ಥಿರಾಂಕ VarDims dims = getVarDims(ಅಂಶ_DTypep);
            stmts.emplace_ಹಿಂಭಾಗ(ಸೇರಿಸು_DType_Var_Statement(svd, scopep, prettyName, cName, ಅಂಶ_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            ಕೂಡಿಸು_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, ಅಂಶ_DTypep);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ subp
                = VN_CAST(ಅಂಶ_DTypep->skipRefp(), ಗಂಟು_UOr_Struct_DType)) {
                ಆದರೆ (!subp->packed())
                    ಕೂಡಿಸು_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಅಂಶ_DTypep->skipRefp(), UnpackArrayDType)) {
                ಕೂಡಿಸು_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    ಅಂಶ_DTypep);
            }
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Unpacked_Array_UOr_Struct_Member_Vars(std::ಸದಿಶ<std::string>& stmts,
                                                    ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd,
                                                    ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                                    ಸ್ಥಿರಾಂಕ std::string& pretty_ಪೂರ್ವಪ್ರತ್ಯಯ,
                                                    ಸ್ಥಿರಾಂಕ std::string& c_ಪೂರ್ವಪ್ರತ್ಯಯ,
                                                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ skipDTypep = dtypep->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            ಕೂಡಿಸು_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_ಪೂರ್ವಪ್ರತ್ಯಯ, c_ಪೂರ್ವಪ್ರತ್ಯಯ + "[0]",
                                                adtypep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep
                   = VN_CAST(skipDTypep, ಗಂಟು_UOr_Struct_DType)) {
            ಆದರೆ (!sdtypep->packed())
                ಕೂಡಿಸು_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_ಪೂರ್ವಪ್ರತ್ಯಯ, c_ಪೂರ್ವಪ್ರತ್ಯಯ, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Packed_Member_Vars(std::ಸದಿಶ<std::string>& stmts, ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd,
                                    ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep, ಸ್ಥಿರಾಂಕ std::string& pretty_ಪೂರ್ವಪ್ರತ್ಯಯ,
                                    ಸ್ಥಿರಾಂಕ std::string& cName, ಸ್ಥಿರಾಂಕ std::string& vlEnumType,
                                    ಸ್ಥಿರಾಂಕ std::string& ent_ಗಾತ್ರ,
                                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep,
                                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಪೋಷಕ_ಬಿಟ್_ಸ್ಥಾನಾಂತರ, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needs_ಸೂಚ್ಯಂಕ) {
        ಸ್ಥಿರಾಂಕ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + cName;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಂಶ_DTypep = itemp->dtypep();
            ಸ್ಥಿರಾಂಕ std::string prettyName
                = pretty_ಪೂರ್ವಪ್ರತ್ಯಯ + "." + Ast_ಗಂಟು::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಬಿಟ್_ಸ್ಥಾನಾಂತರ = ಪೋಷಕ_ಬಿಟ್_ಸ್ಥಾನಾಂತರ + itemp->lsb();
            ಸ್ಥಿರಾಂಕ VarDims dims = getVarDims(ಅಂಶ_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            ಆದರೆ (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_ಅಭಿವ್ಯಕ್ತಿ + "))), true, ";
            } ಇಲ್ಲದಿದ್ದರೆ {
                stmt += ", &(" + var_ಅಭಿವ್ಯಕ್ತಿ + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, ಅಂಶ_DTypep);
            ಆದರೆ (needs_ಸೂಚ್ಯಂಕ) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::ಗಮ್ಯ_string(ಬಿಟ್_ಸ್ಥಾನಾಂತರ) + ", ";
            stmt += ent_ಗಾತ್ರ + ", ";
            stmt += std::ಗಮ್ಯ_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_ಹಿಂಭಾಗ(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* subDTypep = ಅಂಶ_DTypep->skipRefp();
            ತಾರ್ಕಿಕ isArray = ಸುಳ್ಳು;
            ಆಗ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = ನಿಜ;
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ subp = VN_CAST(subDTypep, ಗಂಟು_UOr_Struct_DType)) {
                ಕೂಡಿಸು_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_ಗಾತ್ರ,
                                    subp, ಬಿಟ್_ಸ್ಥಾನಾಂತರ, needs_ಸೂಚ್ಯಂಕ || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Packed_Agg_Member_Vars(std::ಸದಿಶ<std::string>& stmts, ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd,
                                       ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                       ಸ್ಥಿರಾಂಕ std::string& pretty_ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ std::string& cName,
                                       ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        std::string elemCName = cName;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* elemDTypep = dtypep->skipRefp();
        ತಾರ್ಕಿಕ isArray = ಸುಳ್ಳು;
        ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(elemDTypep, ಗಂಟು_Array_DType)) {
            ಆದರೆ (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = ನಿಜ;
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep = VN_CAST(elemDTypep, ಗಂಟು_UOr_Struct_DType);
        ಆದರೆ (!sdtypep || !sdtypep->packed()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + cName;
        ಸ್ಥಿರಾಂಕ std::string ent_ಗಾತ್ರ
            = "sizeof(" + var_ಅಭಿವ್ಯಕ್ತಿ + ") / " + std::ಗಮ್ಯ_string(getUnpackedElements(dtypep));
        ಕೂಡಿಸು_Packed_Member_Vars(stmts, svd, scopep, pretty_ಪೂರ್ವಪ್ರತ್ಯಯ, elemCName, dtypep->vlEnumType(),
                            ent_ಗಾತ್ರ, sdtypep, 0, isArray);
    }

    std::string ಸೇರಿಸು_Forceable_Var_Statement(ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd, ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ udim,
                                            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ pdim, ಸ್ಥಿರಾಂಕ std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(protect(svd.m_var_ತಳ_Pretty)) + '"';

        ಸ್ಥಿರಾಂಕ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        ಪ್ರತಿಜ್ಞಾ(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_ಅಭಿವ್ಯಕ್ತಿ + "__VforceRd";
        stmt += "), \"" + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(protect(svd.m_var_ತಳ_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ಸ್ಥಿರಾಂಕ std::string enable_Signal_ಕೀಲಿ
                = get_ಕೀಲಿ_Name(scopep, varp->name() + forceControlSuffix);
            ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ std::string, Scope_Var_ದತ್ತಾಂಶ>::const_iterator itpair
                = m_scopeVars.ಕಂಡುಹಿಡಿ(enable_Signal_ಕೀಲಿ);

            ಆದರೆ (itpair == m_scopeVars.ಕೊನೆ()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_ಕೀಲಿ << "'.");
            }

            ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd = itpair->ಎರಡನೇ;
            ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = svd.m_scopep;
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = svd.m_varp;
            ಸ್ಥಿರಾಂಕ VarDims dims = dimsFor(svd);
            stmt
                += ಸೇರಿಸು_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            ಆದರೆ (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::ಗಮ್ಯ_string(udim);
        stmt += ", ";
        stmt += std::ಗಮ್ಯ_string(pdim);
        stmt += bounds;
        stmt += ")";
        ಹಿಂತಿರುಗಿ stmt;
    }

    ಸ್ಥಿರ std::string get_ಕೀಲಿ_Name(ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep, ಸ್ಥಿರಾಂಕ std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //ಕ್ರಮ
        // signal can be found.
        std::string ಸಂಪೂರ್ಣ = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        ಆದರೆ (VString::startsWith(ಸಂಪೂರ್ಣ, "__DOT__TOP")) ಸಂಪೂರ್ಣ.replace(0, 10, "");
        ಸ್ಥಿರಾಂಕ std::string::ಗಾತ್ರ_type dpos = ಸಂಪೂರ್ಣ.rfind("__DOT__");
        ಆದರೆ (dpos != std::string::npos) { scpName = ಸಂಪೂರ್ಣ.substr(0, dpos); }
        ಸ್ಥಿರಾಂಕ std::string scpSym = scopeSymString(VName::dehash(scpName));
        ಹಿಂತಿರುಗಿ scpSym + " " + signal_name;
    }

    ತಾರ್ಕಿಕ ತಳ_Signal_Is_Valid(ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep, ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ controlSignalVarp,
                           ಸ್ಥಿರಾಂಕ std::string& ತಳ_Signal_Name) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ std::string ತಳ_Signal_ಕೀಲಿ = get_ಕೀಲಿ_Name(scopep, ತಳ_Signal_Name);
        ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ std::string, Scope_Var_ದತ್ತಾಂಶ>::const_iterator ತಳ_Signal_It
            = m_scopeVars.ಕಂಡುಹಿಡಿ(ತಳ_Signal_ಕೀಲಿ);
        ಆದರೆ (ತಳ_Signal_It == m_scopeVars.ಕೊನೆ()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ ತಳ_Signal_Varp = ತಳ_Signal_It->ಎರಡನೇ.m_varp;
            ಆದರೆ (m_scopeVars.ಎಣಿಕೆ_2(ತಳ_Signal_Name) > 1) {
                ತಳ_Signal_Varp->v3fatalSrc(
                    "Found Signal " << ತಳ_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << ತಳ_Signal_Name << " occurring in the same scope.");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ (!ತಳ_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << ತಳ_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ ತಳ_Signal_Is_ಬಹಿರಂಗ(ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                            ಸ್ಥಿರಾಂಕ std::string& ತಳ_Signal_Name) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ std::string ತಳ_Signal_ಕೀಲಿ = get_ಕೀಲಿ_Name(scopep, ತಳ_Signal_Name);
        ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ std::string, Scope_Var_ದತ್ತಾಂಶ>::const_iterator ತಳ_Signal_It
            = m_scopeVars.ಕಂಡುಹಿಡಿ(ತಳ_Signal_ಕೀಲಿ);
        ಆದರೆ (ತಳ_Signal_It == m_scopeVars.ಕೊನೆ()) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ ತಳ_Signal_Varp = ತಳ_Signal_It->ಎರಡನೇ.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            ಆದರೆ (!ತಳ_Signal_Varp->is_Sig_ಬಹಿರಂಗ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ force_Control_ಸಂಕೇತಗಳು_Are_Valid(ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep,
                                     ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ ತಳ_Signal_Varp) ಸ್ಥಿರಾಂಕ {
        constexpr std::array<ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        ಹಿಂತಿರುಗಿ std::all_ದ(
            forceControlSuffixes.begin(), forceControlSuffixes.ಕೊನೆ(),
            [scopep, ತಳ_Signal_Varp, ಇದು](ಸ್ಥಿರಾಂಕ std::string& forceControlSuffix) {
                ಸ್ಥಿರಾಂಕ std::string controlSignalName = ತಳ_Signal_Varp->name() + forceControlSuffix;
                ಸ್ಥಿರಾಂಕ std::string control_Signal_ಕೀಲಿ = get_ಕೀಲಿ_Name(scopep, controlSignalName);
                ಸ್ಥಿರಾಂಕ std::ಗಾತ್ರ_t control_Signal_ಎಣಿಕೆ = m_scopeVars.ಎಣಿಕೆ_2(control_Signal_ಕೀಲಿ);
                ಆದರೆ (control_Signal_ಎಣಿಕೆ == 0) {
                    ತಳ_Signal_Varp->v3fatalSrc("Signal "
                                               << ತಳ_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_ಕೀಲಿ << "'.");
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                ಆದರೆ (control_Signal_ಎಣಿಕೆ > 1) {
                    ತಳ_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << ತಳ_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    ಶೂನ್ಯ varHierarchyScopes(std::string scp) {

        std::string::ಗಾತ್ರ_type prd_pos = scp.rfind('.');
        std::string::ಗಾತ್ರ_type dot_pos = scp.rfind("__DOT__");

        ಆಗ (!scp.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ scpit = m_vpiScopeCandidates.ಕಂಡುಹಿಡಿ(scopeSymString(scp));
            ಆದರೆ ((scpit != m_vpiScopeCandidates.ಕೊನೆ())
                && (m_scope_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(scp) == m_scope_ಹೆಸರುಗಳು.ಕೊನೆ())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_scope_ಹೆಸರುಗಳು.emplace(scpit->ಎರಡನೇ.m_symName, scpit->ಎರಡನೇ);
                ಆದರೆ (!pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ.m_type = scpit->ಎರಡನೇ.m_type;
            }

            // resize and advance pointers
            ಆದರೆ ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (prd_pos == string::npos) ನಿಲ್ಲಿಸು;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Scope_Var_ನಮೂದು(ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp,
                          ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string ಸಂಪೂರ್ಣ = scopep->name() + "__DOT__" + varp->name();
        ಆದರೆ (VString::startsWith(ಸಂಪೂರ್ಣ, "__DOT__TOP")) ಸಂಪೂರ್ಣ.replace(0, 10, "");
        ಸ್ಥಿರಾಂಕ std::string::ಗಾತ್ರ_type dpos = ಸಂಪೂರ್ಣ.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ಸ್ಥಿರಾಂಕ std::string scpName = ಸಂಪೂರ್ಣ.substr(0, dpos);
        ಸ್ಥಿರಾಂಕ std::string var_ತಳ = ಸಂಪೂರ್ಣ.substr(dpos + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ಸ್ಥಿರಾಂಕ std::string var_ತಳ_Pretty = Ast_ಗಂಟು::vpiName(VName::dehash(var_ತಳ));
        ಸ್ಥಿರಾಂಕ std::string scpPretty = Ast_ಗಂಟು::prettyName(VName::dehash(scpName));
        ಸ್ಥಿರಾಂಕ std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        ಆದರೆ (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_ಹೆಸರುಗಳು.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_ತಳ_Pretty, varp, modp, scopep));
    }

    ಶೂನ್ಯ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& smPair : m_scopes) {
            ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = smPair.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ smodp = smPair.ಎರಡನೇ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ModVarPair& mvPair : m_modVars) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = mvPair.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = mvPair.ಎರಡನೇ;
                ಆದರೆ (modp != smodp) ಮುಂದುವರಿ;
                ಕೂಡಿಸು_Scope_Var_ನಮೂದು(scopep, modp, varp);
            }
        }
    }

    ಶೂನ್ಯ collectIfaceRefs(ಸ್ಥಿರಾಂಕ AstScope* nodep) {
        ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        ಆದರೆ (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) ಹಿಂತಿರುಗಿ;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ಸ್ಥಿರಾಂಕ std::string ಮಾರ್ಗ = Ast_ಗಂಟು::vpiName(nodep->name());
        ಸ್ಥಿರಾಂಕ std::string instName = Ast_ಗಂಟು::vpiName(cellp->origName());
        UASSERT_OBJ(ಮಾರ್ಗ.ಉದ್ದಳತೆ() > instName.ಉದ್ದಳತೆ() && VString::ತುದಿಗಳು_With(ಮಾರ್ಗ, instName), nodep,
                    "Interface scope name " << ಮಾರ್ಗ << " does not end with instance name "
                                            << instName);
        ಸ್ಥಿರಾಂಕ std::string ಪೋಷಕ_ಮಾರ್ಗ = ಮಾರ್ಗ.substr(0, ಮಾರ್ಗ.ಉದ್ದಳತೆ() - instName.ಉದ್ದಳತೆ());

        ಗಾಗಿ (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ಸ್ಥಿರಾಂಕ std::string refName = Ast_ಗಂಟು::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            ಆದರೆ (!VString::startsWith(refName, ಪೋಷಕ_ಮಾರ್ಗ)) ಮುಂದುವರಿ;
            m_ifaceRefs.emplace_ಹಿಂಭಾಗ(nodep, scopeSymString(intfRefp->ಪೋಷಕ_Name()), refName,
                                     Ast_ಗಂಟು::vpiName(intfRefp->ತಳ_Name()),
                                     intfRefp->modportName());
        }
    }

    ಶೂನ್ಯ build_Iface_Ref_ಕೋಷ್ಟಕ() {
        ಆದರೆ (m_ifaceRefs.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_ವಿಂಗಡಿಸು(
            m_ifaceRefs.begin(), m_ifaceRefs.ಕೊನೆ(),
            [](ಸ್ಥಿರಾಂಕ Iface_Ref_ದತ್ತಾಂಶ& a, ಸ್ಥಿರಾಂಕ Iface_Ref_ದತ್ತಾಂಶ& b) { ಹಿಂತಿರುಗಿ a.m_suffix < b.m_suffix; });
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Iface_Ref_ದತ್ತಾಂಶ& ird : m_ifaceRefs) {
            ಸ್ಥಿರಾಂಕ std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            ಆದರೆ (m_scope_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(scopeSym) == m_scope_ಹೆಸರುಗಳು.ಕೊನೆ()) ಮುಂದುವರಿ;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string ಪೋಷಕ_ಸ್ಥಾನಾಂತರ = "VL_IFACEREF_NO_PARENT";
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಪೋಷಕ = m_scope_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(ird.m_ಪೋಷಕ_Sym);
            ಆದರೆ (ಪೋಷಕ != m_scope_ಹೆಸರುಗಳು.ಕೊನೆ()) {
                ಪೋಷಕ_ಸ್ಥಾನಾಂತರ = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + ಪೋಷಕ->ಎರಡನೇ.m_symName) + ")";
            }
            std::string ಅಡ್ಡಸಾಲು
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            ಅಡ್ಡಸಾಲು += ಪೋಷಕ_ಸ್ಥಾನಾಂತರ + ", \"";
            ಅಡ್ಡಸಾಲು += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(VIdProtect::protect_ಪದಗಳು_If(ird.m_name, ನಿಜ));
            ಅಡ್ಡಸಾಲು += "\", \"";
            ಅಡ್ಡಸಾಲು += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(ird.m_suffix, ನಿಜ));
            ಅಡ್ಡಸಾಲು += "\", \"";
            ಅಡ್ಡಸಾಲು += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(ird.m_modportName, ನಿಜ));
            ಅಡ್ಡಸಾಲು += "\"}";
            m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(std::move(ಅಡ್ಡಸಾಲು));
        }
        ಆದರೆ (!m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) m_iface_Ref_ಕೋಷ್ಟಕ_Name = symClass + "__VpiIfaceRefTable";
    }

    ಶೂನ್ಯ buildVpiHierarchy() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scope_ಹೆಸರುಗಳು) {
            ಸ್ಥಿರಾಂಕ std::string symName = itpair.ಎರಡನೇ.m_symName;
            std::string above = symName;
            ಆದರೆ (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            ಆಗ (!above.ಖಾಲಿಗಳು()) {
                ಸ್ಥಿರಾಂಕ std::string::ಗಾತ್ರ_type pos = above.rfind("__");
                ಆದರೆ (pos == std::string::npos) ನಿಲ್ಲಿಸು;
                above.resize(pos);
                ಆದರೆ (m_vpiScopeHierarchy.ಕಂಡುಹಿಡಿ(above) != m_vpiScopeHierarchy.ಕೊನೆ()) {
                    m_vpiScopeHierarchy[above].ತಳ್ಳು_ಹಿಂಭಾಗ(symName);
                    ನಿಲ್ಲಿಸು;
                }
            }
            m_vpiScopeHierarchy[symName] = std::ಸದಿಶ<string>();
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        ಆದರೆ (v3Global.opt.vpi()) buildVpiHierarchy();

        ಆದರೆ (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& scp : m_dpi_Scope_ಹೆಸರುಗಳು) m_scope_ಹೆಸರುಗಳು.emplace(scp.ಮೊದಲ, scp.ಎರಡನೇ);
        }

        // Sort by names, so line/process order matters less
        std::stable_ವಿಂಗಡಿಸು(m_scopes.begin(), m_scopes.ಕೊನೆ(),
                         [](ಸ್ಥಿರಾಂಕ ScopeModPair& a, ಸ್ಥಿರಾಂಕ ScopeModPair& b) {
                             ಹಿಂತಿರುಗಿ a.ಮೊದಲ->name() < b.ಮೊದಲ->name();
                         });
        std::stable_ವಿಂಗಡಿಸು(m_dpis.begin(), m_dpis.ಕೊನೆ(),  //
                         [](ಸ್ಥಿರಾಂಕ AstCFunc* ap, ಸ್ಥಿರಾಂಕ AstCFunc* bp) {
                             ಆದರೆ (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 ಹಿಂತಿರುಗಿ bp->dpiImportPrototype();
                             }
                             ಹಿಂತಿರುಗಿ ap->name() < bp->name();
                         });

        // Output
        ಆದರೆ (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        ಆದರೆ (v3Global.dpi()) {
            emitDpiHdr();
            ಆದರೆ (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->isConstPool()) ಹಿಂತಿರುಗಿ;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellInlineScope* nodep) override {
        ಸ್ಥಿರಾಂಕ std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ಸ್ಥಿರಾಂಕ std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ timeunit = m_modp->timeunit().power_ದ_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ಗಂಟು::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        ಆದರೆ (VN_IS(m_modp, Class)) ಹಿಂತಿರುಗಿ;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_ಹಿಂಭಾಗ(nodep, m_modp);

        ಆದರೆ (v3Global.opt.vpi() && !nodep->is_ತುದಿ()) {
            ಸ್ಥಿರಾಂಕ std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ timeunit = m_modp->timeunit().power_ದ_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_ಗಂಟು::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {
        ಆದರೆ (nodep->selfRelative()) ಹಿಂತಿರುಗಿ;  // Its absolute copy is under the scope
        ಸ್ಥಿರಾಂಕ std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ timeunit = m_modp ? m_modp->timeunit().power_ದ_Ten() : 0;
        m_dpi_Scope_ಹೆಸರುಗಳು.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        ಆದರೆ (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Note emplace does not construct when duplicate key
            m_dpi_Scope_ಹೆಸರುಗಳು.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        ಆದರೆ ((nodep->is_Sig_User_Rd_ಬಹಿರಂಗ() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().ಖಾಲಿಗಳು()) {
            m_modVars.emplace_ಹಿಂಭಾಗ(m_modp, nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        ಆದರೆ (!nodep->ದತ್ತಾಂಶ_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_ಅಂಕೆ(m_coverBins);
            m_coverBins += nodep->ಗಾತ್ರ_2();
            nodep->local_Bin_ಅಂಕೆ(m_modCoverBins);
            m_modCoverBins += nodep->ಗಾತ್ರ_2();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        nameCheck(nodep);
        ಆದರೆ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ಶೂನ್ಯ ಭೇಟಿ(AstConst*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ EmitCSyms(AstNetlist* nodep, ತಾರ್ಕಿಕ dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

ಶೂನ್ಯ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ಶೀರ್ಷಿಕೆ_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_ತುದಿ_Include();
    puts("#include \"verilated.h\"\n");
    ಆದರೆ (v3Global.needTraceDumper()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ತಳ : v3Global.opt.trace_ಮೂಲ_Langs())
            puts("#include \"" + ತಳ + ".h\"\n");
    }
    ಆದರೆ (v3Global.opt.ಬಳಸುತ್ತದೆ_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + ತುದಿ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    ಗಾಗಿ (Ast_ಗಂಟು_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ಗಂಟು_Module);
        ಆದರೆ (VN_IS(nodep, Class)) ಮುಂದುವರಿ;  // Class included earlier
        ಆದರೆ (nodep->isConstPool()) ಮುಂದುವರಿ;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep) + ".h\"\n");
    }

    ಆದರೆ (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::ಗಣ<std::string> types;  // Remove duplicates and sort
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scopeFuncs) {
            ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = itpair.ಎರಡನೇ.m_cfuncp;
            ಆದರೆ (!funcp->dpiExportImpl()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ std::string cbtype
                = protect(v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "__Vcb_" + funcp->cname() + "_t");
            ಸ್ಥಿರಾಂಕ std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_ವಾದಗಳು(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_ಖಾಸಗಿ(ಸುಳ್ಳು);  // public:

    puts("// INTERNAL STATE\n");
    puts(ತುದಿ_Class_Name() + "* const __Vm_modelp;\n");

    ಆದರೆ (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    ಆದರೆ (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    ಆದರೆ (v3Global.hasEvents()) {
        ಆದರೆ (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    ಆದರೆ (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    ಆದರೆ (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    ಆದರೆ (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    ಆದರೆ (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    ಆದರೆ (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::ಗಮ್ಯ_string(ExecMTask::ಅಂಕೆ_ಬಳಸಿದ_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& itpair : m_scopes) {
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = itpair.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = itpair.ಎರಡನೇ;
        ಆದರೆ (VN_IS(modp, Class)) ಮುಂದುವರಿ;
        ofp()->ಬರೆ("%-30s ", EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    ಆದರೆ (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.ಎಳೆಗಳು() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::ಗಮ್ಯ_string(m_coverBins));
        puts("];\n");
    }

    ಆದರೆ (!m_scope_ಹೆಸರುಗಳು.ಖಾಲಿಗಳು()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scope_ಹೆಸರುಗಳು) {
            ಸ್ಥಿರಾಂಕ Scope_ದತ್ತಾಂಶ& sd = itpair.ಎರಡನೇ;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    ಆದರೆ (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + ತುದಿ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    ಆದರೆ (v3Global.hasEvents()) {
        ಆದರೆ (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        ಆದರೆ (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        ಆದರೆ (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    ಆದರೆ (v3Global.needTraceDumper()) {
        ಆದರೆ (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    ಆದರೆ (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->puts_ಕೊನೆ_Guard();
    closeOutputFile();
}

ಶೂನ್ಯ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    ತಾರ್ಕಿಕ needs_New_ಸಾಲು = ಸುಳ್ಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scopeFuncs) {
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = itpair.ಎರಡನೇ.m_cfuncp;
        ಆದರೆ (!funcp->dpiExportImpl()) ಮುಂದುವರಿ;
        emitCFuncDecl(funcp, itpair.ಎರಡನೇ.m_modp);
        needs_New_ಸಾಲು = ನಿಜ;
    }
    ಆದರೆ (needs_New_ಸಾಲು) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::ಗಣ<std::string> lib_ಹೆಸರುಗಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& itpair : m_scopes) {
        ಆದರೆ (itpair.ಎರಡನೇ->verilatorLib()) lib_ಹೆಸರುಗಳು.emplace(itpair.ಎರಡನೇ->origName());
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& libName : lib_ಹೆಸರುಗಳು) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    ಆದರೆ (!lib_ಹೆಸರುಗಳು.ಖಾಲಿಗಳು()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    ಆದರೆ (!m_varTables.ಖಾಲಿಗಳು() || !m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು() || !m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.ಮೊದಲ + "[];\n");
        }
        ಆದರೆ (!m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
            puts("extern const VlScopeTableEntry " + m_scope_ಕೋಷ್ಟಕ_Name + "[];\n");
        }
        ಆದರೆ (!m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_ಕೋಷ್ಟಕ_Name + "[];\n");
        }
        puts("\n");
    }
}

ಶೂನ್ಯ EmitCSyms::emitVarTables() {
    ಆದರೆ (m_varTables.ಖಾಲಿಗಳು() && m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು() && m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.ಮೊದಲ + "[] = {\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& ಅಡ್ಡಸಾಲು : kv.ಎರಡನೇ) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ಅಡ್ಡಸಾಲು);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ಆದರೆ (!m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
        puts("extern const VlScopeTableEntry " + m_scope_ಕೋಷ್ಟಕ_Name + "[] = {\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& ಅಡ್ಡಸಾಲು : m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ಅಡ್ಡಸಾಲು);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ಆದರೆ (!m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_ಕೋಷ್ಟಕ_Name + "[] = {\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& ಅಡ್ಡಸಾಲು : m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ಅಡ್ಡಸಾಲು);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic pop\n");
    puts("#endif\n");
}

ಶೂನ್ಯ EmitCSyms::emitScopeHier(ಪಠ್ಯ_Stmts& stmts, ತಾರ್ಕಿಕ destroy) {
    ಆದರೆ (!v3Global.opt.vpi()) ಹಿಂತಿರುಗಿ;

    ಆದರೆ (destroy) {
        stmts.ಕೂಡಿಸು("// Tear down scope hierarchy");
    } ಇಲ್ಲದಿದ್ದರೆ {
        stmts.ಕೂಡಿಸು("// Set up scope hierarchy");
    }

    ಸ್ಥಿರಾಂಕ std::string ವಿಧಾನ = destroy ? "remove" : "add";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scope_ಹೆಸರುಗಳು) {
        ಆದರೆ (itpair.ಮೊದಲ == "TOP") ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ Scope_ದತ್ತಾಂಶ& sd = itpair.ಎರಡನೇ;
        ಸ್ಥಿರಾಂಕ std::string& name = sd.m_prettyName;
        ಸ್ಥಿರಾಂಕ std::string& scopeType = sd.m_type;
        ಆದರೆ (name.ಕಂಡುಹಿಡಿ('.') != string::npos) ಮುಂದುವರಿ;
        ಆದರೆ (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.ಕೂಡಿಸು("vlSymsp->__Vhier." + ವಿಧಾನ + "(0, " + id + ");");
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_vpiScopeHierarchy) {
        ಸ್ಥಿರಾಂಕ std::string fromName = scopeSymString(itpair.ಮೊದಲ);
        ಸ್ಥಿರಾಂಕ std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_ಹೆಸರುಗಳು.at(fromName).m_symName);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& name : itpair.ಎರಡನೇ) {
            ಸ್ಥಿರಾಂಕ std::string ಗಮ್ಯ_Name = scopeSymString(name);
            ಸ್ಥಿರಾಂಕ std::string ಗಮ್ಯ_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_ಹೆಸರುಗಳು.at(ಗಮ್ಯ_Name).m_symName);
            stmts.ಕೂಡಿಸು("vlSymsp->__Vhier." + ವಿಧಾನ + "(" + fromId + ", " + ಗಮ್ಯ_Id + ");");
        }
    }

    ಆದರೆ (destroy) {
        stmts.ಕೂಡಿಸು("// Clear keys from hierarchy map after values have been removed");
        stmts.ಕೂಡಿಸು("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::ಪಠ್ಯ_Stmts EmitCSyms::getSymCtorStmts() {
    ಪಠ್ಯ_Stmts stmts;

    {
        uint64_t ರಾಶಿ_ಗಾತ್ರ = V3_ರಾಶಿ_ಎಣಿಕೆ::ಎಣಿಕೆ_2(v3Global.rootp());
        ಆದರೆ (v3Global.opt.debug_ರಾಶಿ_Check()) ರಾಶಿ_ಗಾತ್ರ += 1024 * 1024 * 1024;
        V3Stats::ಕೂಡಿಸು_Stat("Size prediction, Stack (bytes)", ರಾಶಿ_ಗಾತ್ರ);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::ಕೂಡಿಸು_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_ಬೈಟುಗಳು);
        V3Stats::ಕೂಡಿಸು_Stat(V3Stats::STAT_MODEL_ಗಾತ್ರ, ರಾಶಿ_ಗಾತ್ರ + m_stat_Var_Scope_ಬೈಟುಗಳು);

        stmts.ಕೂಡಿಸು("// Check resources");
        stmts.ಕೂಡಿಸು("Verilated::stackCheck(" + std::ಗಮ್ಯ_string(ರಾಶಿ_ಗಾತ್ರ) + ");");
    }

    stmts.ಕೂಡಿಸು("// Setup sub module instances");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& itpair : m_scopes) {
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = itpair.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = itpair.ಎರಡನೇ;
        ಆದರೆ (modp->is_ತುದಿ()) ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ std::string name = V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(
            VIdProtect::protect_ಪದಗಳು_If(scopep->prettyName(), scopep->protect()));
        stmts.ಕೂಡಿಸು("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    ಆದರೆ (v3Global.opt.profPgo()) {
        stmts.ಕೂಡಿಸು("// Configure profiling for PGO\n");
        ಆದರೆ (!v3Global.opt.hier_ಮಗು()) {
            stmts.ಕೂಡಿಸು("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        ಆದರೆ (v3Global.opt.mtasks()) {
            v3Global.rootp()->ತುದಿ_Modulep()->foreach([&](ಸ್ಥಿರಾಂಕ Ast_Exec_ನಕ್ಷೆ* execGraphp) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : execGraphp->depGraphp()->vertices()) {
                    ಸ್ಥಿರಾಂಕ ExecMTask& mt = static_cast<ಸ್ಥಿರಾಂಕ ExecMTask&>(vtx);
                    stmts.ಕೂಡಿಸು("vlSymsp->_vm_pgoProfiler.addCounter(" + std::ಗಮ್ಯ_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.ಕೂಡಿಸು("// Configure time unit / time precision");
    ಆದರೆ (!v3Global.rootp()->timeunit().isNone()) {
        ಸ್ಥಿರಾಂಕ std::string unit = std::ಗಮ್ಯ_string(v3Global.rootp()->timeunit().power_ದ_Ten());
        stmts.ಕೂಡಿಸು("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    ಆದರೆ (!v3Global.rootp()->timeprecision().isNone()) {
        ಸ್ಥಿರಾಂಕ std::string prec = std::ಗಮ್ಯ_string(v3Global.rootp()->timeprecision().power_ದ_Ten());
        stmts.ಕೂಡಿಸು("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.ಕೂಡಿಸು("// Setup each module's pointers to their submodules");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : m_scopes) {
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = i.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = i.ಎರಡನೇ;
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ abovep = scopep->aboveScopep();
        ಆದರೆ (!abovep) ಮುಂದುವರಿ;

        ಸ್ಥಿರಾಂಕ std::string protName = VIdProtect::protect_ಪದಗಳು_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        ಆದರೆ (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.ಕೂಡಿಸು(stmt);
    }

    ಆದರೆ (v3Global.opt.coverage()) {
        stmts.ಕೂಡಿಸು("// Setup each module's pointer back to symbol table (for public functions)");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& i : m_scopes) {
            ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = i.ಮೊದಲ;
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = i.ಎರಡನೇ;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೊದಲ = !modp->user1();
            modp->user1(ನಿಜ);
            stmts.ಕೂಡಿಸು("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (ಮೊದಲ ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& i : m_scopes) {
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = i.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = i.ಎರಡನೇ;
        ಆದರೆ (!modp->verilatorLib()) ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ AstVar* hashp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstVar* handlep = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
            ಆದರೆ (!varp) ಮುಂದುವರಿ;
            ಆದರೆ (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            ಆದರೆ (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ಸ್ಥಿರಾಂಕ uint32_t hash = VN_AS(hashp->valuep(), Const)->ಗಮ್ಯ_UInt();
        stmts.ಕೂಡಿಸು(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::ಗಮ್ಯ_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string ಮಾರ್ಗ = scopep->prettyName();
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            ಮಾರ್ಗ.erase(0, v3Global.rootp()->trace_Lib_ತುದಿ_Name().ಗಾತ್ರ_2() + 1);
        }
        stmts.ಕೂಡಿಸು("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(ಮಾರ್ಗ) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    ಆದರೆ (!m_scope_ಹೆಸರುಗಳು.ಖಾಲಿಗಳು()) {
        stmts.ಕೂಡಿಸು("// Setup scopes");
        ಸ್ಥಿರಾಂಕ std::string symClass = symClassName();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scope_ಹೆಸರುಗಳು) {
            ಸ್ಥಿರಾಂಕ Scope_ದತ್ತಾಂಶ& sd = itpair.ಎರಡನೇ;
            std::string ಅಡ್ಡಸಾಲು
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            ಅಡ್ಡಸಾಲು += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(sd.m_prettyName, ನಿಜ));
            ಅಡ್ಡಸಾಲು += "\", \"";
            ಅಡ್ಡಸಾಲು += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            ಅಡ್ಡಸಾಲು += "\", \"";
            ಅಡ್ಡಸಾಲು += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(sd.m_defName);
            ಅಡ್ಡಸಾಲು += "\", ";
            ಅಡ್ಡಸಾಲು += std::ಗಮ್ಯ_string(sd.m_timeunit);
            ಅಡ್ಡಸಾಲು += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(std::move(ಅಡ್ಡಸಾಲು));
        }
        m_scope_ಕೋಷ್ಟಕ_Name = symClass + "__VpiScopeTable";
        stmts.ಕೂಡಿಸು("VerilatedScope::scopesConstructFromTable(" + m_scope_ಕೋಷ್ಟಕ_Name + ", "
                  + std::ಗಮ್ಯ_string(m_scope_ಹೆಸರುಗಳು.ಗಾತ್ರ_2()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_ಕೋಷ್ಟಕ();
    ಆದರೆ (!m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
        stmts.ಕೂಡಿಸು("// Setup interface references");
        stmts.ಕೂಡಿಸು("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_ಕೋಷ್ಟಕ_Name + ", "
                  + std::ಗಮ್ಯ_string(m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಗಾತ್ರ_2()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, ಸುಳ್ಳು);

    ಆದರೆ (v3Global.dpi()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string vfinal : {"0", "1"}) {
            stmts.ಕೂಡಿಸು("// Setup export functions - final: " + vfinal);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scopeFuncs) {
                ಸ್ಥಿರಾಂಕ Scope_Func_ದತ್ತಾಂಶ& sfd = itpair.ಎರಡನೇ;
                ಸ್ಥಿರಾಂಕ AstScopeName* ಸ್ಥಿರಾಂಕ scopep = sfd.m_scopep;
                ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ funcp = sfd.m_cfuncp;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = sfd.m_modp;
                ಆದರೆ (!funcp->dpiExportImpl()) ಮುಂದುವರಿ;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.ಕೂಡಿಸು(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    ಆದರೆ (!m_scopeVars.ಖಾಲಿಗಳು()) {
        stmts.ಕೂಡಿಸು("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> ಕೋಷ್ಟಕ_By_ಅಡ್ಡಸಾಲುಗಳು;
        ಪೂರ್ಣಾಂಕ ಕೋಷ್ಟಕ_Counter = 0;

        ಸ್ವಯಂಚಾಲಿತ it = m_scopeVars.cbegin();
        ಆಗ (it != m_scopeVars.cend()) {
            ಸ್ಥಿರಾಂಕ std::string scopeName = it->ಎರಡನೇ.m_scopeName;
            ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ instScopep = it->ಎರಡನೇ.m_scopep;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = it->ಎರಡನೇ.m_modp;
            ಸ್ಥಿರಾಂಕ std::string modClassName = EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);

            std::ಸದಿಶ<std::string> ಅಡ್ಡಸಾಲುಗಳು;
            std::ಸದಿಶ<std::string> residual;

            ಗಾಗಿ (; it != m_scopeVars.cend() && it->ಎರಡನೇ.m_scopeName == scopeName; ++it) {
                ಸ್ಥಿರಾಂಕ Scope_Var_ದತ್ತಾಂಶ& svd = it->ಎರಡನೇ;
                ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = svd.m_varp;
                ಸ್ಥಿರಾಂಕ VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ಸ್ಥಿರಾಂಕ std::pair<ತಾರ್ಕಿಕ, std::string> fc = isForceControlSignal(varp);
                ಆದರೆ (fc.ಮೊದಲ && ತಳ_Signal_Is_ಬಹಿರಂಗ(scopep, fc.ಎರಡನೇ)
                    && ತಳ_Signal_Is_Valid(scopep, varp, fc.ಎರಡನೇ)) {
                    ಮುಂದುವರಿ;
                }

                std::string ಅಡ್ಡಸಾಲು;
                ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ ಬಗೆ
                    = try_Build_ಕೋಷ್ಟಕ_ನಮೂದು(svd, varp, scopep, modClassName, dims, ಅಡ್ಡಸಾಲು);
                ಆಯ್ಕೆ (ಬಗೆ) {
                ಸ್ಥಿತಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲು:
                    ಅಡ್ಡಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(ಅಡ್ಡಸಾಲು);
                    ಕೂಡಿಸು_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_ತಳ_Pretty,
                                           varName(varp), varp->dtypep());
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::FORCEABLE_RESIDUAL: {
                    ಸ್ಥಿರಾಂಕ std::string bounds = boundsString(dims);
                    residual.emplace_ಹಿಂಭಾಗ(ಸೇರಿಸು_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::PLAIN_RESIDUAL: {
                    ಸ್ಥಿರಾಂಕ std::string bounds = boundsString(dims);
                    residual.emplace_ಹಿಂಭಾಗ(
                        ಸೇರಿಸು_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    ನಿಲ್ಲಿಸು;
                }
                ಪೂರ್ವನಿಯೋಜಿತ: v3fatalSrc("Bad case");
                }
                ಆದರೆ (ಬಗೆ == ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲು) ಮುಂದುವರಿ;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ಗಂಟು_UOr_Struct_DType)) {
                    ಆದರೆ (!sdtypep->packed()) {
                        ಕೂಡಿಸು_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_ತಳ_Pretty,
                                               varName(varp), sdtypep);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ಕೂಡಿಸು_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_ತಳ_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                ಆದರೆ (ಬಗೆ == ಕೋಷ್ಟಕ_ನಮೂದು_ಬಗೆ::PLAIN_RESIDUAL) {
                    ಕೂಡಿಸು_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_ತಳ_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            ಆದರೆ (!ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಡ್ಡಸಾಲು_ಎಣಿಕೆ = ಅಡ್ಡಸಾಲುಗಳು.ಗಾತ್ರ_2();
                std::string ಕೀಲಿ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& r : ಅಡ್ಡಸಾಲುಗಳು) {
                    ಕೀಲಿ += r;
                    ಕೀಲಿ += '\0';
                }
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itt = ಕೋಷ್ಟಕ_By_ಅಡ್ಡಸಾಲುಗಳು.ಕಂಡುಹಿಡಿ(ಕೀಲಿ);
                std::string ಕೋಷ್ಟಕ_Name;
                ಆದರೆ (itt != ಕೋಷ್ಟಕ_By_ಅಡ್ಡಸಾಲುಗಳು.ಕೊನೆ()) {
                    ಕೋಷ್ಟಕ_Name = itt->ಎರಡನೇ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಕೋಷ್ಟಕ_Name = modClassName + "__VpiVarTable" + std::ಗಮ್ಯ_string(ಕೋಷ್ಟಕ_Counter++);
                    ಕೋಷ್ಟಕ_By_ಅಡ್ಡಸಾಲುಗಳು.emplace(std::move(ಕೀಲಿ), ಕೋಷ್ಟಕ_Name);
                    m_varTables.emplace_ಹಿಂಭಾಗ(ಕೋಷ್ಟಕ_Name, std::move(ಅಡ್ಡಸಾಲುಗಳು));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + ಕೋಷ್ಟಕ_Name + ", "
                                   + std::ಗಮ್ಯ_string(ಅಡ್ಡಸಾಲು_ಎಣಿಕೆ) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.ಕೂಡಿಸು(call);
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& r : residual) stmts.ಕೂಡಿಸು(r);
        }
    }

    ಹಿಂತಿರುಗಿ stmts;
}

EmitCSyms::ಪಠ್ಯ_Stmts EmitCSyms::getSymDtorStmts() {
    ಪಠ್ಯ_Stmts stmts;

    emitScopeHier(stmts, ನಿಜ);
    ಆದರೆ (v3Global.needTraceDumper()) {
        stmts.ಕೂಡಿಸು("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    ಆದರೆ (v3Global.opt.profPgo()) {
        stmts.ಕೂಡಿಸು("vlSymsp->_vm_pgoProfiler.write(\"" + ತುದಿ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    ಆದರೆ (!m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಖಾಲಿಗಳು()) {
        stmts.ಕೂಡಿಸು("// Tear down interface references");
        stmts.ಕೂಡಿಸು("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_ಕೋಷ್ಟಕ_Name + ", "
                  + std::ಗಮ್ಯ_string(m_iface_Ref_ಕೋಷ್ಟಕ_ಅಡ್ಡಸಾಲುಗಳು.ಗಾತ್ರ_2()) + ", vlSymsp);");
    }

    stmts.ಕೂಡಿಸು("// Tear down scopes");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itpair : m_scope_ಹೆಸರುಗಳು) {
        ಸ್ಥಿರಾಂಕ Scope_ದತ್ತಾಂಶ& sd = itpair.ಎರಡನೇ;
        ಸ್ಥಿರಾಂಕ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.ಕೂಡಿಸು("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.ಕೂಡಿಸು("// Tear down sub module instances");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& itpair : vlstd::ಹಿಮ್ಮುಖ_view(m_scopes)) {
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = itpair.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = itpair.ಎರಡನೇ;
        ಆದರೆ (modp->is_ತುದಿ()) ಮುಂದುವರಿ;
        stmts.ಕೂಡಿಸು("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    ಹಿಂತಿರುಗಿ stmts;
}

ಶೂನ್ಯ EmitCSyms::emitSymImp(ಸ್ಥಿರಾಂಕ AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    ಪಠ್ಯ_Stmts ctorStmts = getSymCtorStmts();
    ಪಠ್ಯ_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ಸ್ಥಿರಾಂಕ std::pair<std::string, std::string> ವಿಭಜಿಸು_Ctor_Dtor_Decls  //
        = emit_ವಿಭಜಿಸು_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            ನಿಜ, "Symbol table implementation internals",
                            [ಇದು]() { emitSymImpPreamble(); });

    open_New_Output_ಮೂಲ_File(symClassName(), ನಿಜ, ನಿಜ, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep()->scopep();
    ಸ್ಥಿರಾಂಕ std::string ತುದಿ_Inst = "vlSymsp->" + instName(ತುದಿ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    ಆದರೆ (!ವಿಭಜಿಸು_Ctor_Dtor_Decls.ಮೊದಲ.ಖಾಲಿಗಳು()) puts("\n" + ವಿಭಜಿಸು_Ctor_Dtor_Decls.ಮೊದಲ + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + ತುದಿ_Inst + ", vlSymsp, namep);\n");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪಠ್ಯ_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_ಪಠ್ಯ);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + ತುದಿ_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    ಆದರೆ (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    ಆದರೆ (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    ಆದರೆ (v3Global.opt.profPgo() && !v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
        puts("    , _vm_pgoProfiler{" + std::ಗಮ್ಯ_string(v3Global.current_Hier_ಖಂಡ_ವೆಚ್ಚ()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    ಆದರೆ (!ವಿಭಜಿಸು_Ctor_Dtor_Decls.ಎರಡನೇ.ಖಾಲಿಗಳು()) puts("\n" + ವಿಭಜಿಸು_Ctor_Dtor_Decls.ಎರಡನೇ + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪಠ್ಯ_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_ಪಠ್ಯ);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + ತುದಿ_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    ಆದರೆ (v3Global.needTraceDumper()) {
        ಆದರೆ (!optSystemC()) {
            puts("\nvoid " + symClassName() + "::_traceDump() {\n");
            puts("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
            // Caller checked for __Vm_dumperp non-nullptr
            puts("__Vm_dumperp->dump(VL_TIME_Q());\n");
            puts("}\n");
        }

        puts("\nvoid " + symClassName() + "::_traceDumpOpen() {\n");
        puts("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
        puts("if (VL_UNLIKELY(!__Vm_dumperp)) {\n");
        puts("__Vm_dumperp = new " + v3Global.opt.traceClassLang() + "();\n");
        puts("__Vm_modelp->trace(__Vm_dumperp, 0, 0);\n");
        puts("const std::string dumpfile = _vm_contextp__->dumpfileCheck();\n");
        puts("__Vm_dumperp->open(dumpfile.c_str());\n");
        puts("__Vm_dumping = true;\n");
        puts("}\n");
        puts("}\n");

        puts("\nvoid " + symClassName() + "::_traceDumpClose() {\n");
        puts("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
        puts("__Vm_dumping = false;\n");
        puts("VL_DO_CLEAR(delete __Vm_dumperp, __Vm_dumperp = nullptr);\n");
        puts("}\n");
    }

    ಆದರೆ (v3Global.opt.savable()) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ& de : {ಸುಳ್ಳು, ನಿಜ}) {
            ಸ್ಥಿರಾಂಕ std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ಸ್ಥಿರಾಂಕ std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ಸ್ಥಿರಾಂಕ std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            ಆದರೆ (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ScopeModPair& itpair : m_scopes) {
                ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = itpair.ಮೊದಲ;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

ಶೂನ್ಯ EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    open_New_Output_ಶೀರ್ಷಿಕೆ_File(ತುದಿ_Class_Name() + "__Dpi",
                            "Prototypes for DPI import and export functions.");
    puts("//\n");
    puts("// Verilator includes this file in all generated .cpp files that use DPI functions.\n");
    puts("// Manually include this file where DPI .c import functions are declared to ensure\n");
    puts("// the C functions match the expectations of the DPI imports.\n");

    ofp()->putsGuard();

    puts("\n");
    puts("#include \"svdpi.h\"\n");
    puts("\n");

    // Unpacked structures passed as C structures (IEEE 1800-2023 H.7.8)
    std::ಗಣ<std::string> struct_ಹೆಸರುಗಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ nodep : m_dpis) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_ಹೆಸರುಗಳು);
            }
        }
    }
    ಆದರೆ (!struct_ಹೆಸರುಗಳು.ಖಾಲಿಗಳು()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    ಪೂರ್ಣಾಂಕ ಮೊದಲ_Exp = 0;
    ಪೂರ್ಣಾಂಕ ಮೊದಲ_Imp = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ nodep : m_dpis) {
        ಆದರೆ (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) ಮುಂದುವರಿ;

        ಸ್ಥಿರಾಂಕ std::string ಮೂಲ_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        ಆದರೆ (nodep->dpiExportDispatcher()) {
            ಆದರೆ (!ಮೊದಲ_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + ಮೂಲ_Loc + "\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!ಮೊದಲ_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + ಮೂಲ_Loc + "\n");
        }
        ಆದರೆ (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_ವಾದಗಳು(nodep, ಸುಳ್ಳು) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_ಕೊನೆ_Guard();

    closeOutputFile();
}

ಶೂನ್ಯ EmitCSyms::emitDpiStruct(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep, std::ಗಣ<std::string>& namesr) {
    ಆದರೆ (!dtypep->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()) ಹಿಂತಿರುಗಿ;
    ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ಸ್ಥಿರಾಂಕ std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    ಆದರೆ (namesr.ಎಣಿಕೆ_2(name)) ಹಿಂತಿರುಗಿ;
    // After the structures it contains
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    ಆದರೆ (name.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;  // Declared where used
    ಆದರೆ (namesr.ಖಾಲಿಗಳು()) puts("// DPI TYPES\n");
    namesr.ಸೇರಿಸು(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

ಶೂನ್ಯ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ಮೂಲ_File(ತುದಿ_Class_Name() + "__Dpi", ಸುಳ್ಳು, ನಿಜ,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + ತುದಿ_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + ತುದಿ_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + ತುದಿ_Class_Name() + ".h\"\n");
    puts("\n");

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ nodep : m_dpis) {
        ಆದರೆ (!nodep->dpiExportDispatcher()) ಮುಂದುವರಿ;

        ಸ್ಥಿರಾಂಕ std::string name = nodep->name();
        ಸ್ಥಿರಾಂಕ std::string ಮೂಲ_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_ವಾದಗಳು(nodep) + ") {\n");
        puts("// DPI export" + ಮೂಲ_Loc + "\n");
        putns(nodep, "return " + ತುದಿ_Class_Name() + "::" + name + "(");
        std::string comma;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isIO() && !portp->isFuncReturn()) {
                    puts(comma);
                    comma = ", ";
                    putns(portp, portp->name());
                }
            }
        }
        puts(");\n");
        puts("}\n");
        puts("#endif\n");
        puts("\n");
    }
    closeOutputFile();
}

//######################################################################
// EmitC class functions

ಶೂನ್ಯ V3EmitC::emitcSyms(ತಾರ್ಕಿಕ dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
