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
ಸ್ಥಿರ constexpr auto symClassName = &EmitCUtil::symClassName;
ಸ್ಥಿರ constexpr auto ತುದಿ_Class_Name = &EmitCUtil::ತುದಿ_Class_Name;

ವರ್ಗ EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // TYPES
    ಸಂರಚನಾ ScopeData final {
        ಧ್ರುವ Ast_ಗಂಟು* m_nodep;  // Scope symbol-table entry corresponds to
        ಧ್ರುವ std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ಧ್ರುವ std::string m_prettyName;  // Pretty (unprotected) name for display
        ಧ್ರುವ std::string m_defName;  // Name registered for this scope
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ std::string& symName, ಧ್ರುವ std::string& prettyName,
                  ಧ್ರುವ std::string& defName, ಪೂರ್ಣಾಙ್ಕ timeunit, ಧ್ರುವ std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    ಸಂರಚನಾ IfaceRefData final {
        ಧ್ರುವ AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(ಧ್ರುವ AstScope* scopep, ಧ್ರುವ std::string& parentSym,
                     ಧ್ರುವ std::string& suffix, ಧ್ರುವ std::string& name,
                     ಧ್ರುವ std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    ಸಂರಚನಾ ScopeFuncData final {
        ಧ್ರುವ AstScopeName* ಧ್ರುವ m_scopep;  // Scope the DPI export function is registered under
        ಧ್ರುವ AstCFunc* ಧ್ರುವ m_cfuncp;  // DPI export function
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module containing the DPI export function
        ScopeFuncData(ಧ್ರುವ AstScopeName* scopep, ಧ್ರುವ AstCFunc* funcp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    ಸಂರಚನಾ ScopeVarData final {
        ಧ್ರುವ std::string m_scopeName;  // Full name of the scope containing the variable
        ಧ್ರುವ std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        ಧ್ರುವ AstVar* ಧ್ರುವ m_varp;  // Public variable
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module containing the variable
        ಧ್ರುವ AstScope* ಧ್ರುವ m_scopep;  // Scope containing the variable
        ScopeVarData(ಧ್ರುವ std::string& scopeName, ಧ್ರುವ std::string& varBasePretty,
                     ಧ್ರುವ AstVar* varp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ಧ್ರುವ AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<ಧ್ರುವ std::string, ScopeData>;
    using ScopeModPair = std::pair<ಧ್ರುವ AstScope*, Ast_ಗಂಟು_Module*>;
    using ModVarPair = std::pair<ಧ್ರುವ Ast_ಗಂಟು_Module*, ಧ್ರುವ AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    ಧ್ರುವ AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ಧ್ರುವ std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ಧ್ರುವ std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ಧ್ರುವ std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    ಪೂರ್ಣಾಙ್ಕ m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    ಪೂರ್ಣಾಙ್ಕ m_modCoverBins = 0;  // Per-module coverage bin number
    ಧ್ರುವ ತಾರ್ಕಿಕ m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    ಶೂನ್ಯ emitSymHdr();
    ಶೂನ್ಯ emitSymImpPreamble();
    ಶೂನ್ಯ emitVarTables();
    ಶೂನ್ಯ emitScopeHier(TextStmts& stmts, ತಾರ್ಕಿಕ destroy);
    ಶೂನ್ಯ emitSymImp(ಧ್ರುವ AstNetlist* netlistp);
    ಶೂನ್ಯ emitDpiHdr();
    ಶೂನ್ಯ emitDpiStruct(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep, std::set<std::string>& namesr);
    ಶೂನ್ಯ emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    ಸ್ಥಿರ ಶೂನ್ಯ nameCheck(Ast_ಗಂಟು* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        ಯದಿ (nodep->name().empty()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ AstCFunc* ಧ್ರುವ cfuncp = VN_CAST(nodep, CFunc);
        ಯದಿ (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ಧ್ರುವ std::string rsvd = V3_Language_ಪದಗಳು::isKeyword(nodep->name());
            ಯದಿ (rsvd != "") {
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

    ಸ್ಥಿರ std::string scopeSymString(ಧ್ರುವ std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        ಯಾವತ್ ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        ಯದಿ (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        ಯದಿ (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        ಯಾವತ್ ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        ಯಾವತ್ ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        ಪ್ರತ್ಯಾಗಮ out;
    }

    ಸ್ಥಿರ string scopeDecodeIdentifier(ಧ್ರುವ std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        ಯಾವತ್ (i < scpname.length()) {
            ಯದಿ (scpname[i] == '\\') {
                ಯಾವತ್ (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } ಅನ್ಯಥಾ {
                ಯಾವತ್ (i < scpname.length() && scpname[i] != '.') ++i;
                ಯದಿ (i < scpname.length()) pos = i++;
            }
        }

        ಪ್ರತ್ಯಾಗಮ pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    ಸಂರಚನಾ VarDims final {
        ಪೂರ್ಣಾಙ್ಕ pdim = 0;  // Packed array dims plus a ranged basic leaf
        ಪೂರ್ಣಾಙ್ಕ udim = 0;
        std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> flatten() ಧ್ರುವ {
            std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> flat(unpackedLR);
            flat.ಸೇರಿಸು(flat.end(), packedLR.cbegin(), packedLR.cend());
            ಪ್ರತ್ಯಾಗಮ flat;
        }
    };
    ಸ್ಥಿರ VarDims getVarDims(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ ಬೇರು_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = ಬೇರು_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                ಯದಿ (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->ಎಡ(), adtypep->ಬಲ());
                    ++d.pdim;
                } ಅನ್ಯಥಾ {
                    d.unpackedLR.emplace_back(adtypep->ಎಡ(), adtypep->ಬಲ());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } ಅನ್ಯಥಾ {
                ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp()) {
                    ಯದಿ (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->ಎಡ(), basicp->ಬಲ());
                        ++d.pdim;
                    }
                }
                ಭಙ್ಗ;  // Non-array leaf
            }
        }
        ಪ್ರತ್ಯಾಗಮ d;
    }

    ಸ್ಥಿರ VarDims dimsFor(ಧ್ರುವ ScopeVarData& svd) { ಪ್ರತ್ಯಾಗಮ getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ಸ್ಥಿರ std::string boundsString(ಧ್ರುವ VarDims& d) {
        std::string bounds;
        ಪ್ರತಿ (ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        ಪ್ರತ್ಯಾಗಮ bounds;
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ getUnpackedElements(ಧ್ರುವ Ast_ಗಂಟು_DType* ಬೇರು_Dtypep) {
        ಪೂರ್ಣಾಙ್ಕ elements = 1;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = ಬೇರು_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, UnpackArrayDType);
            ಯದಿ (!adtypep) ಭಙ್ಗ;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        ಪ್ರತ್ಯಾಗಮ elements;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ needsEmittedEntSize(ಧ್ರುವ std::string& vlEnumType) {
        ಪ್ರತ್ಯಾಗಮ vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ಸ್ಥಿರ std::pair<ತಾರ್ಕಿಕ, std::string> isForceControlSignal(ಧ್ರುವ AstVar* ಧ್ರುವ signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        ಪ್ರತಿ (ಧ್ರುವ std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ಧ್ರುವ std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            ಧ್ರುವ ತಾರ್ಕಿಕ suffixFound = suffixPos != std::string::npos;
            ಧ್ರುವ ತಾರ್ಕಿಕ suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            ಯದಿ (!(suffixFound && suffixIsAtEnd)) ಅನುವರ್ತ;
            ಧ್ರುವ std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            ಪ್ರತ್ಯಾಗಮ std::pair<ತಾರ್ಕಿಕ, std::string>{ಸತ್ಯ, baseSignalName};
        }
        ಪ್ರತ್ಯಾಗಮ std::pair<ತಾರ್ಕಿಕ, std::string>{ಅಸತ್ಯ, ""};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ appendVarProperties(std::string& stmt, ಧ್ರುವ std::string& vlEnumType,
                                    ಧ್ರುವ std::string& vlEnumDir, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ udim, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ pdim,
                                    ಧ್ರುವ std::string& bounds, ಧ್ರುವ std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        ಯದಿ (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } ಅನ್ಯಥಾ {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ಸ್ಥಿರ std::string memberVlEnumDir(ಧ್ರುವ AstVar* ಧ್ರುವ varp,
                                       ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/ಸತ್ಯ);
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ skipDTypep = dtypep->skipRefp();
        ಯದಿ (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = skipDTypep->basicp()) {
            ಯದಿ (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        ಪ್ರತ್ಯಾಗಮ out + ')';
    }

    // Name of the instance member for the given scope
    ಸ್ಥಿರ std::string instName(ಧ್ರುವ AstScope* scopep) {
        ಪ್ರತ್ಯಾಗಮ "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ಸ್ಥಿರ std::string ಸೇರಿಸು_Var_Statement(ಧ್ರುವ ScopeVarData& svd, ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                          ಧ್ರುವ AstVar* ಧ್ರುವ varp, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ udim, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ pdim,
                                          ಧ್ರುವ std::string& bounds) {
        ಧ್ರುವ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ಧ್ರುವ std::string vlEnumType = varp->vlEnumType();
        ಧ್ರುವ ತಾರ್ಕಿಕ needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        ಯದಿ (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "), false, ";
        } ಅನ್ಯಥಾ ಯದಿ (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += ".c_str())), true, ";
        } ಅನ್ಯಥಾ {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "))), true, ";
        }

        ಧ್ರುವ std::string entSize = needsEntSize
                                        ? "sizeof(" + var_ಅಭಿವ್ಯಕ್ತಿ + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        ಪ್ರತ್ಯಾಗಮ stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    ಗಣನಾ ವರ್ಗ TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(ಧ್ರುವ ScopeVarData& svd, ಧ್ರುವ AstVar* ಧ್ರುವ varp,
                                      ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                      ಧ್ರುವ std::string& modClassName, ಧ್ರುವ VarDims& dims,
                                      std::string& rowOut) ಧ್ರುವ {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ pdim = dims.pdim;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ udim = dims.udim;
        ಯದಿ (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            ಪ್ರತ್ಯಾಗಮ TableEntryKind::FORCEABLE_RESIDUAL;
        }
        ಯದಿ (udim + pdim > VPI_TABLE_MAX_DIMS) ಪ್ರತ್ಯಾಗಮ TableEntryKind::PLAIN_RESIDUAL;

        ಧ್ರುವ std::string vlEnumType = varp->vlEnumType();
        ಯದಿ (needsEmittedEntSize(vlEnumType))
            ಪ್ರತ್ಯಾಗಮ TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        ಯದಿ (varp->isParam()) ಪ್ರತ್ಯಾಗಮ TableEntryKind::PLAIN_RESIDUAL;

        ಧ್ರುವ std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ಧ್ರುವ std::string member = varName(varp);
        ಧ್ರುವ std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<ಪೂರ್ಣಾಙ್ಕ> dv;
        ಪ್ರತಿ (ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>& lr : dims.flatten()) {
            dv.ತಳ್ಳು_back(lr.first);
            dv.ತಳ್ಳು_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            ಯದಿ (i) row += ", ";
            row += std::to_string(i < static_cast<ಪೂರ್ಣಾಙ್ಕ>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        ಪ್ರತ್ಯಾಗಮ TableEntryKind::TABLE_ROW;
    }

    ಸ್ಥಿರ std::string ಸೇರಿಸು_DType_Var_Statement(ಧ್ರುವ ScopeVarData& svd,
                                               ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                               ಧ್ರುವ std::string& prettyName,
                                               ಧ್ರುವ std::string& cName,
                                               ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ udim,
                                               ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ pdim, ಧ್ರುವ std::string& bounds) {
        ಧ್ರುವ std::string vlEnumType = dtypep->vlEnumType();
        ಧ್ರುವ ತಾರ್ಕಿಕ needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        ಧ್ರುವ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        ಯದಿ (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "))), true, ";
        } ಅನ್ಯಥಾ {
            stmt += ", &(";
            stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
            stmt += "), false, ";
        }
        ಧ್ರುವ std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_ಅಭಿವ್ಯಕ್ತಿ + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        ಪ್ರತ್ಯಾಗಮ stmt;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ addUOrStructMemberVars(std::vector<std::string>& stmts, ಧ್ರುವ ScopeVarData& svd,
                                       ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                       ಧ್ರುವ std::string& prettyPrefix, ಧ್ರುವ std::string& cPrefix,
                                       ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep) {
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ itemDTypep = itemp->dtypep();
            ಯದಿ (itemDTypep->vlEnumType().empty()) ಅನುವರ್ತ;
            ಧ್ರುವ std::string prettyName
                = prettyPrefix + "." + Ast_ಗಂಟು::vpiName(itemp->shortName());
            ಧ್ರುವ std::string cName = cPrefix + "." + itemp->nameProtect();
            ಧ್ರುವ VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(ಸೇರಿಸು_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ subp
                = VN_CAST(itemDTypep->skipRefp(), ಗಂಟು_UOr_Struct_DType)) {
                ಯದಿ (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    ಧ್ರುವ ScopeVarData& svd,
                                                    ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                                    ಧ್ರುವ std::string& prettyPrefix,
                                                    ಧ್ರುವ std::string& cPrefix,
                                                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ skipDTypep = dtypep->skipRefp();
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep
                   = VN_CAST(skipDTypep, ಗಂಟು_UOr_Struct_DType)) {
            ಯದಿ (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ಸ್ಥಿರ ಶೂನ್ಯ addPackedMemberVars(std::vector<std::string>& stmts, ಧ್ರುವ ScopeVarData& svd,
                                    ಧ್ರುವ AstScope* ಧ್ರುವ scopep, ಧ್ರುವ std::string& prettyPrefix,
                                    ಧ್ರುವ std::string& cName, ಧ್ರುವ std::string& vlEnumType,
                                    ಧ್ರುವ std::string& entSize,
                                    ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep,
                                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ parentBitOffset, ಧ್ರುವ ತಾರ್ಕಿಕ needsIndex) {
        ಧ್ರುವ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + cName;
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ itemDTypep = itemp->dtypep();
            ಧ್ರುವ std::string prettyName
                = prettyPrefix + "." + Ast_ಗಂಟು::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bitOffset = parentBitOffset + itemp->lsb();
            ಧ್ರುವ VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            ಯದಿ (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_ಅಭಿವ್ಯಕ್ತಿ + "))), true, ";
            } ಅನ್ಯಥಾ {
                stmt += ", &(" + var_ಅಭಿವ್ಯಕ್ತಿ + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            ಯದಿ (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ಧ್ರುವ Ast_ಗಂಟು_DType* subDTypep = itemDTypep->skipRefp();
            ತಾರ್ಕಿಕ isArray = ಅಸತ್ಯ;
            ಯಾವತ್ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = ಸತ್ಯ;
            }
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ subp = VN_CAST(subDTypep, ಗಂಟು_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ಸ್ಥಿರ ಶೂನ್ಯ addPackedAggMemberVars(std::vector<std::string>& stmts, ಧ್ರುವ ScopeVarData& svd,
                                       ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                       ಧ್ರುವ std::string& prettyPrefix, ಧ್ರುವ std::string& cName,
                                       ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        std::string elemCName = cName;
        ಧ್ರುವ Ast_ಗಂಟು_DType* elemDTypep = dtypep->skipRefp();
        ತಾರ್ಕಿಕ isArray = ಅಸತ್ಯ;
        ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(elemDTypep, ಗಂಟು_Array_DType)) {
            ಯದಿ (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = ಸತ್ಯ;
        }
        ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep = VN_CAST(elemDTypep, ಗಂಟು_UOr_Struct_DType);
        ಯದಿ (!sdtypep || !sdtypep->packed()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + cName;
        ಧ್ರುವ std::string entSize
            = "sizeof(" + var_ಅಭಿವ್ಯಕ್ತಿ + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string ಸೇರಿಸು_Forceable_Var_Statement(ಧ್ರುವ ScopeVarData& svd, ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                            ಧ್ರುವ AstVar* ಧ್ರುವ varp, ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ udim,
                                            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ pdim, ಧ್ರುವ std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        ಧ್ರುವ std::string var_ಅಭಿವ್ಯಕ್ತಿ = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        ಪ್ರತಿಜ್ಞಾ(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_ಅಭಿವ್ಯಕ್ತಿ;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_ಅಭಿವ್ಯಕ್ತಿ + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        ಪ್ರತಿ (ಧ್ರುವ std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ಧ್ರುವ std::string enable_Signal_ಕೀಲಿ
                = get_ಕೀಲಿ_Name(scopep, varp->name() + forceControlSuffix);
            ಧ್ರುವ std::map<ಧ್ರುವ std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_ಕೀಲಿ);

            ಯದಿ (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_ಕೀಲಿ << "'.");
            }

            ಧ್ರುವ ScopeVarData& svd = itpair->second;
            ಧ್ರುವ AstScope* ಧ್ರುವ scopep = svd.m_scopep;
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = svd.m_varp;
            ಧ್ರುವ VarDims dims = dimsFor(svd);
            stmt
                += ಸೇರಿಸು_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            ಯದಿ (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        ಪ್ರತ್ಯಾಗಮ stmt;
    }

    ಸ್ಥಿರ std::string get_ಕೀಲಿ_Name(ಧ್ರುವ AstScope* ಧ್ರುವ scopep, ಧ್ರುವ std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        ಯದಿ (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ಧ್ರುವ std::string::size_type dpos = whole.rfind("__DOT__");
        ಯದಿ (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        ಧ್ರುವ std::string scpSym = scopeSymString(VName::dehash(scpName));
        ಪ್ರತ್ಯಾಗಮ scpSym + " " + signal_name;
    }

    ತಾರ್ಕಿಕ baseSignalIsValid(ಧ್ರುವ AstScope* ಧ್ರುವ scopep, ಧ್ರುವ AstVar* ಧ್ರುವ controlSignalVarp,
                           ಧ್ರುವ std::string& baseSignalName) ಧ್ರುವ {
        ಧ್ರುವ std::string base_Signal_ಕೀಲಿ = get_ಕೀಲಿ_Name(scopep, baseSignalName);
        ಧ್ರುವ std::map<ಧ್ರುವ std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_ಕೀಲಿ);
        ಯದಿ (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ AstVar* ಧ್ರುವ baseSignalVarp = baseSignalIt->second.m_varp;
            ಯದಿ (m_scopeVars.ಎಣಿಕೆ(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ baseSignalIsPublic(ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                            ಧ್ರುವ std::string& baseSignalName) ಧ್ರುವ {
        ಧ್ರುವ std::string base_Signal_ಕೀಲಿ = get_ಕೀಲಿ_Name(scopep, baseSignalName);
        ಧ್ರುವ std::map<ಧ್ರುವ std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_ಕೀಲಿ);
        ಯದಿ (baseSignalIt == m_scopeVars.end()) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ AstVar* ಧ್ರುವ baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            ಯದಿ (!baseSignalVarp->isSigPublic()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ forceControlSignalsAreValid(ಧ್ರುವ AstScope* ಧ್ರುವ scopep,
                                     ಧ್ರುವ AstVar* ಧ್ರುವ baseSignalVarp) ಧ್ರುವ {
        constexpr std::array<ಧ್ರುವ ವರ್ಣ*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        ಪ್ರತ್ಯಾಗಮ std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, ಏತತ್](ಧ್ರುವ std::string& forceControlSuffix) {
                ಧ್ರುವ std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                ಧ್ರುವ std::string control_Signal_ಕೀಲಿ = get_ಕೀಲಿ_Name(scopep, controlSignalName);
                ಧ್ರುವ std::size_t control_Signal_ಎಣಿಕೆ = m_scopeVars.ಎಣಿಕೆ(control_Signal_ಕೀಲಿ);
                ಯದಿ (control_Signal_ಎಣಿಕೆ == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_ಕೀಲಿ << "'.");
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                ಯದಿ (control_Signal_ಎಣಿಕೆ > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    ಶೂನ್ಯ varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        ಯಾವತ್ (!scp.empty()) {
            ಧ್ರುವ auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            ಯದಿ ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ಧ್ರುವ auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                ಯದಿ (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            ಯದಿ ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } ಅನ್ಯಥಾ {
                ಯದಿ (prd_pos == string::npos) ಭಙ್ಗ;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    ಶೂನ್ಯ addScopeVarEntry(ಧ್ರುವ AstScope* ಧ್ರುವ scopep, ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp,
                          ಧ್ರುವ AstVar* ಧ್ರುವ varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        ಯದಿ (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ಧ್ರುವ std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ಧ್ರುವ std::string scpName = whole.substr(0, dpos);
        ಧ್ರುವ std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ಧ್ರುವ std::string varBasePretty = Ast_ಗಂಟು::vpiName(VName::dehash(varBase));
        ಧ್ರುವ std::string scpPretty = Ast_ಗಂಟು::prettyName(VName::dehash(scpName));
        ಧ್ರುವ std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        ಯದಿ (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    ಶೂನ್ಯ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& smPair : m_scopes) {
            ಧ್ರುವ AstScope* ಧ್ರುವ scopep = smPair.first;
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ smodp = smPair.second;
            ಪ್ರತಿ (ಧ್ರುವ ModVarPair& mvPair : m_modVars) {
                ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = mvPair.first;
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = mvPair.second;
                ಯದಿ (modp != smodp) ಅನುವರ್ತ;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    ಶೂನ್ಯ collectIfaceRefs(ಧ್ರುವ AstScope* nodep) {
        ಧ್ರುವ AstCell* ಧ್ರುವ cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        ಯದಿ (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) ಪ್ರತ್ಯಾಗಮ;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ಧ್ರುವ std::string path = Ast_ಗಂಟು::vpiName(nodep->name());
        ಧ್ರುವ std::string instName = Ast_ಗಂಟು::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        ಧ್ರುವ std::string parentPath = path.substr(0, path.length() - instName.length());

        ಪ್ರತಿ (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ಧ್ರುವ std::string refName = Ast_ಗಂಟು::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            ಯದಿ (!VString::startsWith(refName, parentPath)) ಅನುವರ್ತ;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_ಗಂಟು::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    ಶೂನ್ಯ buildIfaceRefTable() {
        ಯದಿ (m_ifaceRefs.empty()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](ಧ್ರುವ IfaceRefData& a, ಧ್ರುವ IfaceRefData& b) { ಪ್ರತ್ಯಾಗಮ a.m_suffix < b.m_suffix; });
        ಪ್ರತಿ (ಧ್ರುವ IfaceRefData& ird : m_ifaceRefs) {
            ಧ್ರುವ std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            ಯದಿ (m_scopeNames.find(scopeSym) == m_scopeNames.end()) ಅನುವರ್ತ;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            ಧ್ರುವ auto parent = m_scopeNames.find(ird.m_parentSym);
            ಯದಿ (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_ಪದಗಳು_If(ird.m_name, ಸತ್ಯ));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(ird.m_suffix, ಸತ್ಯ));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(ird.m_modportName, ಸತ್ಯ));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        ಯದಿ (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    ಶೂನ್ಯ buildVpiHierarchy() {
        ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeNames) {
            ಧ್ರುವ std::string symName = itpair.second.m_symName;
            std::string above = symName;
            ಯದಿ (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            ಯಾವತ್ (!above.empty()) {
                ಧ್ರುವ std::string::size_type pos = above.rfind("__");
                ಯದಿ (pos == std::string::npos) ಭಙ್ಗ;
                above.resize(pos);
                ಯದಿ (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].ತಳ್ಳು_back(symName);
                    ಭಙ್ಗ;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        ಯದಿ (v3Global.opt.vpi()) buildVpiHierarchy();

        ಯದಿ (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            ಪ್ರತಿ (ಧ್ರುವ auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](ಧ್ರುವ ScopeModPair& a, ಧ್ರುವ ScopeModPair& b) {
                             ಪ್ರತ್ಯಾಗಮ a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](ಧ್ರುವ AstCFunc* ap, ಧ್ರುವ AstCFunc* bp) {
                             ಯದಿ (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 ಪ್ರತ್ಯಾಗಮ bp->dpiImportPrototype();
                             }
                             ಪ್ರತ್ಯಾಗಮ ap->name() < bp->name();
                         });

        // Output
        ಯದಿ (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        ಯದಿ (v3Global.dpi()) {
            emitDpiHdr();
            ಯದಿ (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->isConstPool()) ಪ್ರತ್ಯಾಗಮ;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCellInlineScope* nodep) override {
        ಧ್ರುವ std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ಧ್ರುವ std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ಗಂಟು::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        ಯದಿ (VN_IS(m_modp, Class)) ಪ್ರತ್ಯಾಗಮ;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        ಯದಿ (v3Global.opt.vpi() && !nodep->is_ತುದಿ()) {
            ಧ್ರುವ std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ timeunit = m_modp->timeunit().powerOfTen();
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
    ಶೂನ್ಯ visit(AstScopeName* nodep) override {
        ಯದಿ (nodep->selfRelative()) ಪ್ರತ್ಯಾಗಮ;  // Its absolute copy is under the scope
        ಧ್ರುವ std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        ಯದಿ (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } ಅನ್ಯಥಾ {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        ಯದಿ ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        ಯದಿ (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_ಅಂಕೆ(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_ಅಂಕೆ(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        ಯದಿ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ತಳ್ಳು_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ಶೂನ್ಯ visit(AstConst*) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit EmitCSyms(AstNetlist* nodep, ತಾರ್ಕಿಕ dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

ಶೂನ್ಯ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_ತುದಿ_Include();
    puts("#include \"verilated.h\"\n");
    ಯದಿ (v3Global.needTraceDumper()) {
        ಪ್ರತಿ (ಧ್ರುವ string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    ಯದಿ (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + ತುದಿ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    ಪ್ರತಿ (Ast_ಗಂಟು_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ಗಂಟು_Module);
        ಯದಿ (VN_IS(nodep, Class)) ಅನುವರ್ತ;  // Class included earlier
        ಯದಿ (nodep->isConstPool()) ಅನುವರ್ತ;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    ಯದಿ (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeFuncs) {
            ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = itpair.second.m_cfuncp;
            ಯದಿ (!funcp->dpiExportImpl()) ಅನುವರ್ತ;
            ಧ್ರುವ std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            ಧ್ರುವ std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        ಪ್ರತಿ (ಧ್ರುವ std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(ಅಸತ್ಯ);  // public:

    puts("// INTERNAL STATE\n");
    puts(ತುದಿ_Class_Name() + "* const __Vm_modelp;\n");

    ಯದಿ (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    ಯದಿ (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    ಯದಿ (v3Global.hasEvents()) {
        ಯದಿ (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } ಅನ್ಯಥಾ {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    ಯದಿ (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    ಯದಿ (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    ಯದಿ (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    ಯದಿ (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    ಯದಿ (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::ಅಂಕೆ_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& itpair : m_scopes) {
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = itpair.first;
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = itpair.second;
        ಯದಿ (VN_IS(modp, Class)) ಅನುವರ್ತ;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    ಯದಿ (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    ಯದಿ (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeNames) {
            ಧ್ರುವ ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    ಯದಿ (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + ತುದಿ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    ಯದಿ (v3Global.hasEvents()) {
        ಯದಿ (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } ಅನ್ಯಥಾ {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        ಯದಿ (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } ಅನ್ಯಥಾ {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        ಯದಿ (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } ಅನ್ಯಥಾ {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    ಯದಿ (v3Global.needTraceDumper()) {
        ಯದಿ (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    ಯದಿ (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

ಶೂನ್ಯ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    ತಾರ್ಕಿಕ needs_New_ಸಾಲು = ಅಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeFuncs) {
        ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = itpair.second.m_cfuncp;
        ಯದಿ (!funcp->dpiExportImpl()) ಅನುವರ್ತ;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_ಸಾಲು = ಸತ್ಯ;
    }
    ಯದಿ (needs_New_ಸಾಲು) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& itpair : m_scopes) {
        ಯದಿ (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    ಪ್ರತಿ (ಧ್ರುವ std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    ಯದಿ (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    ಯದಿ (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        ಪ್ರತಿ (ಧ್ರುವ auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        ಯದಿ (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        ಯದಿ (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

ಶೂನ್ಯ EmitCSyms::emitVarTables() {
    ಯದಿ (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) ಪ್ರತ್ಯಾಗಮ;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    ಪ್ರತಿ (ಧ್ರುವ auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        ಪ್ರತಿ (ಧ್ರುವ std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ಯದಿ (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        ಪ್ರತಿ (ಧ್ರುವ std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ಯದಿ (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        ಪ್ರತಿ (ಧ್ರುವ std::string& row : m_ifaceRefTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic pop\n");
    puts("#endif\n");
}

ಶೂನ್ಯ EmitCSyms::emitScopeHier(TextStmts& stmts, ತಾರ್ಕಿಕ destroy) {
    ಯದಿ (!v3Global.opt.vpi()) ಪ್ರತ್ಯಾಗಮ;

    ಯದಿ (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } ಅನ್ಯಥಾ {
        stmts.add("// Set up scope hierarchy");
    }

    ಧ್ರುವ std::string method = destroy ? "remove" : "add";
    ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeNames) {
        ಯದಿ (itpair.first == "TOP") ಅನುವರ್ತ;
        ಧ್ರುವ ScopeData& sd = itpair.second;
        ಧ್ರುವ std::string& name = sd.m_prettyName;
        ಧ್ರುವ std::string& scopeType = sd.m_type;
        ಯದಿ (name.find('.') != string::npos) ಅನುವರ್ತ;
        ಯದಿ (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") ಅನುವರ್ತ;
        ಧ್ರುವ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_vpiScopeHierarchy) {
        ಧ್ರುವ std::string fromName = scopeSymString(itpair.first);
        ಧ್ರುವ std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        ಪ್ರತಿ (ಧ್ರುವ std::string& name : itpair.second) {
            ಧ್ರುವ std::string toName = scopeSymString(name);
            ಧ್ರುವ std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    ಯದಿ (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t ರಾಶಿ_Size = V3_ರಾಶಿ_ಎಣಿಕೆ::ಎಣಿಕೆ(v3Global.rootp());
        ಯದಿ (v3Global.opt.debug_ರಾಶಿ_Check()) ರಾಶಿ_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", ರಾಶಿ_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, ರಾಶಿ_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(ರಾಶಿ_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& itpair : m_scopes) {
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = itpair.first;
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = itpair.second;
        ಯದಿ (modp->is_ತುದಿ()) ಅನುವರ್ತ;
        ಧ್ರುವ std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_ಪದಗಳು_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    ಯದಿ (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        ಯದಿ (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        ಯದಿ (v3Global.opt.mtasks()) {
            v3Global.rootp()->ತುದಿ_Modulep()->foreach([&](ಧ್ರುವ AstExecGraph* execGraphp) {
                ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    ಧ್ರುವ ExecMTask& mt = static_cast<ಧ್ರುವ ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    ಯದಿ (!v3Global.rootp()->timeunit().isNone()) {
        ಧ್ರುವ std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    ಯದಿ (!v3Global.rootp()->timeprecision().isNone()) {
        ಧ್ರುವ std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    ಪ್ರತಿ (ಧ್ರುವ auto& i : m_scopes) {
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = i.first;
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = i.second;
        ಧ್ರುವ AstScope* ಧ್ರುವ abovep = scopep->aboveScopep();
        ಯದಿ (!abovep) ಅನುವರ್ತ;

        ಧ್ರುವ std::string protName = VIdProtect::protect_ಪದಗಳು_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        ಯದಿ (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } ಅನ್ಯಥಾ {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    ಯದಿ (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& i : m_scopes) {
            ಧ್ರುವ AstScope* ಧ್ರುವ scopep = i.first;
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ಧ್ರುವ ತಾರ್ಕಿಕ first = !modp->user1();
            modp->user1(ಸತ್ಯ);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& i : m_scopes) {
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = i.first;
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = i.second;
        ಯದಿ (!modp->verilatorLib()) ಅನುವರ್ತ;
        ಧ್ರುವ AstVar* hashp = nullptr;
        ಧ್ರುವ AstVar* handlep = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
            ಯದಿ (!varp) ಅನುವರ್ತ;
            ಯದಿ (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            ಯದಿ (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ಧ್ರುವ uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        ಯದಿ (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_ತುದಿ_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    ಯದಿ (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        ಧ್ರುವ std::string symClass = symClassName();
        ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeNames) {
            ಧ್ರುವ ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ಪದಗಳು_If(sd.m_prettyName, ಸತ್ಯ));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(sd.m_defName);
            row += "\", ";
            row += std::to_string(sd.m_timeunit);
            row += ", VerilatedScope::" + sd.m_type + "}";
            m_scopeTableRows.emplace_back(std::move(row));
        }
        m_scopeTableName = symClass + "__VpiScopeTable";
        stmts.add("VerilatedScope::scopesConstructFromTable(" + m_scopeTableName + ", "
                  + std::to_string(m_scopeNames.size()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    buildIfaceRefTable();
    ಯದಿ (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, ಅಸತ್ಯ);

    ಯದಿ (v3Global.dpi()) {
        ಪ್ರತಿ (ಧ್ರುವ std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeFuncs) {
                ಧ್ರುವ ScopeFuncData& sfd = itpair.second;
                ಧ್ರುವ AstScopeName* ಧ್ರುವ scopep = sfd.m_scopep;
                ಧ್ರುವ AstCFunc* ಧ್ರುವ funcp = sfd.m_cfuncp;
                ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = sfd.m_modp;
                ಯದಿ (!funcp->dpiExportImpl()) ಅನುವರ್ತ;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3OutFormatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::prefixNameProtect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.add(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    ಯದಿ (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        ಪೂರ್ಣಾಙ್ಕ tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        ಯಾವತ್ (it != m_scopeVars.cend()) {
            ಧ್ರುವ std::string scopeName = it->second.m_scopeName;
            ಧ್ರುವ AstScope* ಧ್ರುವ instScopep = it->second.m_scopep;
            ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = it->second.m_modp;
            ಧ್ರುವ std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            ಪ್ರತಿ (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                ಧ್ರುವ ScopeVarData& svd = it->second;
                ಧ್ರುವ AstScope* ಧ್ರುವ scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = svd.m_varp;
                ಧ್ರುವ VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ಧ್ರುವ std::pair<ತಾರ್ಕಿಕ, std::string> fc = isForceControlSignal(varp);
                ಯದಿ (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    ಅನುವರ್ತ;
                }

                std::string row;
                ಧ್ರುವ TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                ವಿಕಲ್ಪ (kind) {
                ಪಕ್ಷ TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    ಭಙ್ಗ;
                ಪಕ್ಷ TableEntryKind::FORCEABLE_RESIDUAL: {
                    ಧ್ರುವ std::string bounds = boundsString(dims);
                    residual.emplace_back(ಸೇರಿಸು_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ TableEntryKind::PLAIN_RESIDUAL: {
                    ಧ್ರುವ std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        ಸೇರಿಸು_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    ಭಙ್ಗ;
                }
                ಸ್ವಭಾವ: v3fatalSrc("Bad case");
                }
                ಯದಿ (kind == TableEntryKind::TABLE_ROW) ಅನುವರ್ತ;
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ಗಂಟು_UOr_Struct_DType)) {
                    ಯದಿ (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                ಯದಿ (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            ಯದಿ (!rows.empty()) {
                ಧ್ರುವ size_t row_ಎಣಿಕೆ = rows.size();
                std::string ಕೀಲಿ;
                ಪ್ರತಿ (ಧ್ರುವ std::string& r : rows) {
                    ಕೀಲಿ += r;
                    ಕೀಲಿ += '\0';
                }
                ಧ್ರುವ auto itt = tableByRows.find(ಕೀಲಿ);
                std::string tableName;
                ಯದಿ (itt != tableByRows.end()) {
                    tableName = itt->second;
                } ಅನ್ಯಥಾ {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(ಕೀಲಿ), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_ಎಣಿಕೆ) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            ಪ್ರತಿ (ಧ್ರುವ std::string& r : residual) stmts.add(r);
        }
    }

    ಪ್ರತ್ಯಾಗಮ stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, ಸತ್ಯ);
    ಯದಿ (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    ಯದಿ (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + ತುದಿ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    ಯದಿ (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    ಪ್ರತಿ (ಧ್ರುವ auto& itpair : m_scopeNames) {
        ಧ್ರುವ ScopeData& sd = itpair.second;
        ಧ್ರುವ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = itpair.first;
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = itpair.second;
        ಯದಿ (modp->is_ತುದಿ()) ಅನುವರ್ತ;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    ಪ್ರತ್ಯಾಗಮ stmts;
}

ಶೂನ್ಯ EmitCSyms::emitSymImp(ಧ್ರುವ AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ಧ್ರುವ std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            ಸತ್ಯ, "Symbol table implementation internals",
                            [ಏತತ್]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), ಸತ್ಯ, ಸತ್ಯ, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ಧ್ರುವ AstScope* ಧ್ರುವ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep()->scopep();
    ಧ್ರುವ std::string ತುದಿ_Inst = "vlSymsp->" + instName(ತುದಿ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    ಯದಿ (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + ತುದಿ_Inst + ", vlSymsp, namep);\n");
    ಪ್ರತಿ (ಧ್ರುವ TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
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
    ಯದಿ (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    ಯದಿ (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    ಯದಿ (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    ಯದಿ (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    ಪ್ರತಿ (ಧ್ರುವ TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
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
    ಯದಿ (v3Global.needTraceDumper()) {
        ಯದಿ (!optSystemC()) {
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

    ಯದಿ (v3Global.opt.savable()) {
        ಪ್ರತಿ (ಧ್ರುವ ತಾರ್ಕಿಕ& de : {ಅಸತ್ಯ, ಸತ್ಯ}) {
            ಧ್ರುವ std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ಧ್ರುವ std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ಧ್ರುವ std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            ಯದಿ (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            ಪ್ರತಿ (ಧ್ರುವ ScopeModPair& itpair : m_scopes) {
                ಧ್ರುವ AstScope* ಧ್ರುವ scopep = itpair.first;
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

    openNewOutputHeaderFile(ತುದಿ_Class_Name() + "__Dpi",
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
    std::set<std::string> structNames;
    ಪ್ರತಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ nodep : m_dpis) {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    ಯದಿ (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    ಪೂರ್ಣಾಙ್ಕ firstExp = 0;
    ಪೂರ್ಣಾಙ್ಕ firstImp = 0;
    ಪ್ರತಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ nodep : m_dpis) {
        ಯದಿ (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) ಅನುವರ್ತ;

        ಧ್ರುವ std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        ಯದಿ (nodep->dpiExportDispatcher()) {
            ಯದಿ (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } ಅನ್ಯಥಾ {
            ಯದಿ (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        ಯದಿ (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } ಅನ್ಯಥಾ {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, ಅಸತ್ಯ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

ಶೂನ್ಯ EmitCSyms::emitDpiStruct(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep, std::set<std::string>& namesr) {
    ಯದಿ (!dtypep->containsUnpackedStruct()) ಪ್ರತ್ಯಾಗಮ;
    ಧ್ರುವ AstStructDType* ಧ್ರುವ sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ಧ್ರುವ std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    ಯದಿ (namesr.ಎಣಿಕೆ(name)) ಪ್ರತ್ಯಾಗಮ;
    // After the structures it contains
    ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    ಯದಿ (name.empty()) ಪ್ರತ್ಯಾಗಮ;  // Declared where used
    ಯದಿ (namesr.empty()) puts("// DPI TYPES\n");
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
    openNewOutputSourceFile(ತುದಿ_Class_Name() + "__Dpi", ಅಸತ್ಯ, ಸತ್ಯ,
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

    ಪ್ರತಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ nodep : m_dpis) {
        ಯದಿ (!nodep->dpiExportDispatcher()) ಅನುವರ್ತ;

        ಧ್ರುವ std::string name = nodep->name();
        ಧ್ರುವ std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + ತುದಿ_Class_Name() + "::" + name + "(");
        std::string comma;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isIO() && !portp->isFuncReturn()) {
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
