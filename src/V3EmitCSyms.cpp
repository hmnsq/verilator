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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3EmitC.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3ExecGraph.h"
#അന്തര്ഭാവ "V3LanguageWords.h"
#അന്തര്ഭാവ "V3StackCount.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cstring>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
സ്ഥിതം constexpr auto symClassName = &EmitCUtil::symClassName;
സ്ഥിതം constexpr auto മുകൾ_Class_Name = &EmitCUtil::മുകൾ_Class_Name;

വർഗ്ഗം EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    സ്ഥിരം VNUser1InUse m_inuser1;

    // TYPES
    ഘടന ScopeData final {
        സ്ഥിരം Ast_കണ്ണി* m_nodep;  // Scope symbol-table entry corresponds to
        സ്ഥിരം std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        സ്ഥിരം std::string m_prettyName;  // Pretty (unprotected) name for display
        സ്ഥിരം std::string m_defName;  // Name registered for this scope
        സ്ഥിരം പൂർണ്ണസംഖ്യ m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം std::string& symName, സ്ഥിരം std::string& prettyName,
                  സ്ഥിരം std::string& defName, പൂർണ്ണസംഖ്യ timeunit, സ്ഥിരം std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    ഘടന IfaceRefData final {
        സ്ഥിരം AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(സ്ഥിരം AstScope* scopep, സ്ഥിരം std::string& parentSym,
                     സ്ഥിരം std::string& suffix, സ്ഥിരം std::string& name,
                     സ്ഥിരം std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    ഘടന ScopeFuncData final {
        സ്ഥിരം AstScopeName* സ്ഥിരം m_scopep;  // Scope the DPI export function is registered under
        സ്ഥിരം AstCFunc* സ്ഥിരം m_cfuncp;  // DPI export function
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module containing the DPI export function
        ScopeFuncData(സ്ഥിരം AstScopeName* scopep, സ്ഥിരം AstCFunc* funcp, സ്ഥിരം Ast_കണ്ണി_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    ഘടന ScopeVarData final {
        സ്ഥിരം std::string m_scopeName;  // Full name of the scope containing the variable
        സ്ഥിരം std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        സ്ഥിരം AstVar* സ്ഥിരം m_varp;  // Public variable
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module containing the variable
        സ്ഥിരം AstScope* സ്ഥിരം m_scopep;  // Scope containing the variable
        ScopeVarData(സ്ഥിരം std::string& scopeName, സ്ഥിരം std::string& varBasePretty,
                     സ്ഥിരം AstVar* varp, സ്ഥിരം Ast_കണ്ണി_Module* modp, സ്ഥിരം AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<സ്ഥിരം std::string, ScopeData>;
    using ScopeModPair = std::pair<സ്ഥിരം AstScope*, Ast_കണ്ണി_Module*>;
    using ModVarPair = std::pair<സ്ഥിരം Ast_കണ്ണി_Module*, സ്ഥിരം AstVar*>;

    // Vars with more dims take the residual per-statement path.
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    സ്ഥിരം AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<സ്ഥിരം std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<സ്ഥിരം std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<സ്ഥിരം std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    പൂർണ്ണസംഖ്യ m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    പൂർണ്ണസംഖ്യ m_modCoverBins = 0;  // Per-module coverage bin number
    സ്ഥിരം തർക്കം m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    ശൂന്യം emitSymHdr();
    ശൂന്യം emitSymImpPreamble();
    ശൂന്യം emitVarTables();
    ശൂന്യം emitScopeHier(TextStmts& stmts, തർക്കം destroy);
    ശൂന്യം emitSymImp(സ്ഥിരം AstNetlist* netlistp);
    ശൂന്യം emitDpiHdr();
    ശൂന്യം emitDpiStruct(സ്ഥിരം Ast_കണ്ണി_DType* dtypep, std::set<std::string>& namesr);
    ശൂന്യം emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    സ്ഥിതം ശൂന്യം nameCheck(Ast_കണ്ണി* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        എങ്കിൽ (nodep->name().empty()) തിരികെ;
        സ്ഥിരം AstCFunc* സ്ഥിരം cfuncp = VN_CAST(nodep, CFunc);
        എങ്കിൽ (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            സ്ഥിരം std::string rsvd = V3_Language_വാക്കുകൾ::isKeyword(nodep->name());
            എങ്കിൽ (rsvd != "") {
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

    സ്ഥിതം std::string scopeSymString(സ്ഥിരം std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        ആകുമ്പോൾ ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        എങ്കിൽ (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        എങ്കിൽ (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        ആകുമ്പോൾ ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        ആകുമ്പോൾ ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        തിരികെ out;
    }

    സ്ഥിതം string scopeDecodeIdentifier(സ്ഥിരം std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        ആകുമ്പോൾ (i < scpname.length()) {
            എങ്കിൽ (scpname[i] == '\\') {
                ആകുമ്പോൾ (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } അല്ലെങ്കിൽ {
                ആകുമ്പോൾ (i < scpname.length() && scpname[i] != '.') ++i;
                എങ്കിൽ (i < scpname.length()) pos = i++;
            }
        }

        തിരികെ pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    ഘടന VarDims final {
        പൂർണ്ണസംഖ്യ pdim = 0;  // Packed array dims plus a ranged basic leaf
        പൂർണ്ണസംഖ്യ udim = 0;
        std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> flatten() സ്ഥിരം {
            std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> flat(unpackedLR);
            flat.തിരുകുക(flat.end(), packedLR.cbegin(), packedLR.cend());
            തിരികെ flat;
        }
    };
    സ്ഥിതം VarDims getVarDims(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം വേര്_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_DType* dtypep = വേര്_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(dtypep, കണ്ണി_Array_DType)) {
                എങ്കിൽ (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->ഇടത്(), adtypep->വലത്());
                    ++d.pdim;
                } അല്ലെങ്കിൽ {
                    d.unpackedLR.emplace_back(adtypep->ഇടത്(), adtypep->വലത്());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp()) {
                    എങ്കിൽ (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->ഇടത്(), basicp->വലത്());
                        ++d.pdim;
                    }
                }
                നിർത്തുക;  // Non-array leaf
            }
        }
        തിരികെ d;
    }

    സ്ഥിതം VarDims dimsFor(സ്ഥിരം ScopeVarData& svd) { തിരികെ getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    സ്ഥിതം std::string boundsString(സ്ഥിരം VarDims& d) {
        std::string bounds;
        വേണ്ടി (സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        തിരികെ bounds;
    }

    സ്ഥിതം പൂർണ്ണസംഖ്യ getUnpackedElements(സ്ഥിരം Ast_കണ്ണി_DType* വേര്_Dtypep) {
        പൂർണ്ണസംഖ്യ elements = 1;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_DType* dtypep = വേര്_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(dtypep, UnpackArrayDType);
            എങ്കിൽ (!adtypep) നിർത്തുക;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        തിരികെ elements;
    }

    സ്ഥിതം തർക്കം needsEmittedEntSize(സ്ഥിരം std::string& vlEnumType) {
        തിരികെ vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    സ്ഥിതം std::pair<തർക്കം, std::string> isForceControlSignal(സ്ഥിരം AstVar* സ്ഥിരം signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        വേണ്ടി (സ്ഥിരം std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            സ്ഥിരം std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            സ്ഥിരം തർക്കം suffixFound = suffixPos != std::string::npos;
            സ്ഥിരം തർക്കം suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            എങ്കിൽ (!(suffixFound && suffixIsAtEnd)) തുടരുക;
            സ്ഥിരം std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            തിരികെ std::pair<തർക്കം, std::string>{സത്യം, baseSignalName};
        }
        തിരികെ std::pair<തർക്കം, std::string>{അസത്യം, ""};
    }

    സ്ഥിതം ശൂന്യം appendVarProperties(std::string& stmt, സ്ഥിരം std::string& vlEnumType,
                                    സ്ഥിരം std::string& vlEnumDir, സ്ഥിരം പൂർണ്ണസംഖ്യ udim, സ്ഥിരം പൂർണ്ണസംഖ്യ pdim,
                                    സ്ഥിരം std::string& bounds, സ്ഥിരം std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        എങ്കിൽ (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } അല്ലെങ്കിൽ {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    സ്ഥിതം std::string memberVlEnumDir(സ്ഥിരം AstVar* സ്ഥിരം varp,
                                       സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/സത്യം);
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം skipDTypep = dtypep->skipRefp();
        എങ്കിൽ (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = skipDTypep->basicp()) {
            എങ്കിൽ (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        തിരികെ out + ')';
    }

    // Name of the instance member for the given scope
    സ്ഥിതം std::string instName(സ്ഥിരം AstScope* scopep) {
        തിരികെ "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    സ്ഥിതം std::string തിരുകുക_Var_Statement(സ്ഥിരം ScopeVarData& svd, സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                          സ്ഥിരം AstVar* സ്ഥിരം varp, സ്ഥിരം പൂർണ്ണസംഖ്യ udim, സ്ഥിരം പൂർണ്ണസംഖ്യ pdim,
                                          സ്ഥിരം std::string& bounds) {
        സ്ഥിരം std::string var_വ്യഞ്ജകം = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        സ്ഥിരം std::string vlEnumType = varp->vlEnumType();
        സ്ഥിരം തർക്കം needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        എങ്കിൽ (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_വ്യഞ്ജകം;
            stmt += "), false, ";
        } അല്ലെങ്കിൽ എങ്കിൽ (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_വ്യഞ്ജകം;
            stmt += ".c_str())), true, ";
        } അല്ലെങ്കിൽ {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_വ്യഞ്ജകം;
            stmt += "))), true, ";
        }

        സ്ഥിരം std::string entSize = needsEntSize
                                        ? "sizeof(" + var_വ്യഞ്ജകം + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        തിരികെ stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    എണ്ണൽ വർഗ്ഗം TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(സ്ഥിരം ScopeVarData& svd, സ്ഥിരം AstVar* സ്ഥിരം varp,
                                      സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                      സ്ഥിരം std::string& modClassName, സ്ഥിരം VarDims& dims,
                                      std::string& rowOut) സ്ഥിരം {
        സ്ഥിരം പൂർണ്ണസംഖ്യ pdim = dims.pdim;
        സ്ഥിരം പൂർണ്ണസംഖ്യ udim = dims.udim;
        എങ്കിൽ (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            തിരികെ TableEntryKind::FORCEABLE_RESIDUAL;
        }
        എങ്കിൽ (udim + pdim > VPI_TABLE_MAX_DIMS) തിരികെ TableEntryKind::PLAIN_RESIDUAL;

        സ്ഥിരം std::string vlEnumType = varp->vlEnumType();
        എങ്കിൽ (needsEmittedEntSize(vlEnumType))
            തിരികെ TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        എങ്കിൽ (varp->isParam()) തിരികെ TableEntryKind::PLAIN_RESIDUAL;

        സ്ഥിരം std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        സ്ഥിരം std::string member = varName(varp);
        സ്ഥിരം std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<പൂർണ്ണസംഖ്യ> dv;
        വേണ്ടി (സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>& lr : dims.flatten()) {
            dv.തള്ളുക_back(lr.first);
            dv.തള്ളുക_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            എങ്കിൽ (i) row += ", ";
            row += std::to_string(i < static_cast<പൂർണ്ണസംഖ്യ>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        തിരികെ TableEntryKind::TABLE_ROW;
    }

    സ്ഥിതം std::string തിരുകുക_DType_Var_Statement(സ്ഥിരം ScopeVarData& svd,
                                               സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                               സ്ഥിരം std::string& prettyName,
                                               സ്ഥിരം std::string& cName,
                                               സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep, സ്ഥിരം പൂർണ്ണസംഖ്യ udim,
                                               സ്ഥിരം പൂർണ്ണസംഖ്യ pdim, സ്ഥിരം std::string& bounds) {
        സ്ഥിരം std::string vlEnumType = dtypep->vlEnumType();
        സ്ഥിരം തർക്കം needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        സ്ഥിരം std::string var_വ്യഞ്ജകം = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        എങ്കിൽ (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_വ്യഞ്ജകം;
            stmt += "))), true, ";
        } അല്ലെങ്കിൽ {
            stmt += ", &(";
            stmt += var_വ്യഞ്ജകം;
            stmt += "), false, ";
        }
        സ്ഥിരം std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_വ്യഞ്ജകം + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        തിരികെ stmt;
    }

    സ്ഥിതം ശൂന്യം addUOrStructMemberVars(std::vector<std::string>& stmts, സ്ഥിരം ScopeVarData& svd,
                                       സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                       സ്ഥിരം std::string& prettyPrefix, സ്ഥിരം std::string& cPrefix,
                                       സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep) {
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം itemDTypep = itemp->dtypep();
            എങ്കിൽ (itemDTypep->vlEnumType().empty()) തുടരുക;
            സ്ഥിരം std::string prettyName
                = prettyPrefix + "." + Ast_കണ്ണി::vpiName(itemp->shortName());
            സ്ഥിരം std::string cName = cPrefix + "." + itemp->nameProtect();
            സ്ഥിരം VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(തിരുകുക_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം subp
                = VN_CAST(itemDTypep->skipRefp(), കണ്ണി_UOr_Struct_DType)) {
                എങ്കിൽ (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    സ്ഥിതം ശൂന്യം addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    സ്ഥിരം ScopeVarData& svd,
                                                    സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                                    സ്ഥിരം std::string& prettyPrefix,
                                                    സ്ഥിരം std::string& cPrefix,
                                                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം skipDTypep = dtypep->skipRefp();
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep
                   = VN_CAST(skipDTypep, കണ്ണി_UOr_Struct_DType)) {
            എങ്കിൽ (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    സ്ഥിതം ശൂന്യം addPackedMemberVars(std::vector<std::string>& stmts, സ്ഥിരം ScopeVarData& svd,
                                    സ്ഥിരം AstScope* സ്ഥിരം scopep, സ്ഥിരം std::string& prettyPrefix,
                                    സ്ഥിരം std::string& cName, സ്ഥിരം std::string& vlEnumType,
                                    സ്ഥിരം std::string& entSize,
                                    സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep,
                                    സ്ഥിരം പൂർണ്ണസംഖ്യ parentBitOffset, സ്ഥിരം തർക്കം needsIndex) {
        സ്ഥിരം std::string var_വ്യഞ്ജകം = "vlSymsp->" + instName(scopep) + "." + cName;
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം itemDTypep = itemp->dtypep();
            സ്ഥിരം std::string prettyName
                = prettyPrefix + "." + Ast_കണ്ണി::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            സ്ഥിരം പൂർണ്ണസംഖ്യ bitOffset = parentBitOffset + itemp->lsb();
            സ്ഥിരം VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            എങ്കിൽ (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_വ്യഞ്ജകം + "))), true, ";
            } അല്ലെങ്കിൽ {
                stmt += ", &(" + var_വ്യഞ്ജകം + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            എങ്കിൽ (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            സ്ഥിരം Ast_കണ്ണി_DType* subDTypep = itemDTypep->skipRefp();
            തർക്കം isArray = അസത്യം;
            ആകുമ്പോൾ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = സത്യം;
            }
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം subp = VN_CAST(subDTypep, കണ്ണി_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    സ്ഥിതം ശൂന്യം addPackedAggMemberVars(std::vector<std::string>& stmts, സ്ഥിരം ScopeVarData& svd,
                                       സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                       സ്ഥിരം std::string& prettyPrefix, സ്ഥിരം std::string& cName,
                                       സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        std::string elemCName = cName;
        സ്ഥിരം Ast_കണ്ണി_DType* elemDTypep = dtypep->skipRefp();
        തർക്കം isArray = അസത്യം;
        ആകുമ്പോൾ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(elemDTypep, കണ്ണി_Array_DType)) {
            എങ്കിൽ (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = സത്യം;
        }
        സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep = VN_CAST(elemDTypep, കണ്ണി_UOr_Struct_DType);
        എങ്കിൽ (!sdtypep || !sdtypep->packed()) തിരികെ;
        സ്ഥിരം std::string var_വ്യഞ്ജകം = "vlSymsp->" + instName(scopep) + "." + cName;
        സ്ഥിരം std::string entSize
            = "sizeof(" + var_വ്യഞ്ജകം + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string തിരുകുക_Forceable_Var_Statement(സ്ഥിരം ScopeVarData& svd, സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                            സ്ഥിരം AstVar* സ്ഥിരം varp, സ്ഥിരം പൂർണ്ണസംഖ്യ udim,
                                            സ്ഥിരം പൂർണ്ണസംഖ്യ pdim, സ്ഥിരം std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        സ്ഥിരം std::string var_വ്യഞ്ജകം = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        പ്രതിജ്ഞാ(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_വ്യഞ്ജകം;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_വ്യഞ്ജകം + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        വേണ്ടി (സ്ഥിരം std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            സ്ഥിരം std::string enable_Signal_താക്കോൽ
                = get_താക്കോൽ_Name(scopep, varp->name() + forceControlSuffix);
            സ്ഥിരം std::map<സ്ഥിരം std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_താക്കോൽ);

            എങ്കിൽ (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_താക്കോൽ << "'.");
            }

            സ്ഥിരം ScopeVarData& svd = itpair->second;
            സ്ഥിരം AstScope* സ്ഥിരം scopep = svd.m_scopep;
            സ്ഥിരം AstVar* സ്ഥിരം varp = svd.m_varp;
            സ്ഥിരം VarDims dims = dimsFor(svd);
            stmt
                += തിരുകുക_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            എങ്കിൽ (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        തിരികെ stmt;
    }

    സ്ഥിതം std::string get_താക്കോൽ_Name(സ്ഥിരം AstScope* സ്ഥിരം scopep, സ്ഥിരം std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        എങ്കിൽ (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        സ്ഥിരം std::string::size_type dpos = whole.rfind("__DOT__");
        എങ്കിൽ (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        സ്ഥിരം std::string scpSym = scopeSymString(VName::dehash(scpName));
        തിരികെ scpSym + " " + signal_name;
    }

    തർക്കം baseSignalIsValid(സ്ഥിരം AstScope* സ്ഥിരം scopep, സ്ഥിരം AstVar* സ്ഥിരം controlSignalVarp,
                           സ്ഥിരം std::string& baseSignalName) സ്ഥിരം {
        സ്ഥിരം std::string base_Signal_താക്കോൽ = get_താക്കോൽ_Name(scopep, baseSignalName);
        സ്ഥിരം std::map<സ്ഥിരം std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_താക്കോൽ);
        എങ്കിൽ (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            തിരികെ അസത്യം;
        } അല്ലെങ്കിൽ {
            സ്ഥിരം AstVar* സ്ഥിരം baseSignalVarp = baseSignalIt->second.m_varp;
            എങ്കിൽ (m_scopeVars.എണ്ണം(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                തിരികെ അസത്യം;
            }
            എങ്കിൽ (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                തിരികെ അസത്യം;
            }
        }
        തിരികെ സത്യം;
    }

    തർക്കം baseSignalIsPublic(സ്ഥിരം AstScope* സ്ഥിരം scopep,
                            സ്ഥിരം std::string& baseSignalName) സ്ഥിരം {
        സ്ഥിരം std::string base_Signal_താക്കോൽ = get_താക്കോൽ_Name(scopep, baseSignalName);
        സ്ഥിരം std::map<സ്ഥിരം std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_താക്കോൽ);
        എങ്കിൽ (baseSignalIt == m_scopeVars.end()) {
            തിരികെ അസത്യം;
        } അല്ലെങ്കിൽ {
            സ്ഥിരം AstVar* സ്ഥിരം baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            എങ്കിൽ (!baseSignalVarp->isSigPublic()) തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    തർക്കം forceControlSignalsAreValid(സ്ഥിരം AstScope* സ്ഥിരം scopep,
                                     സ്ഥിരം AstVar* സ്ഥിരം baseSignalVarp) സ്ഥിരം {
        constexpr std::array<സ്ഥിരം അക്ഷരം*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        തിരികെ std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, ഇത്](സ്ഥിരം std::string& forceControlSuffix) {
                സ്ഥിരം std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                സ്ഥിരം std::string control_Signal_താക്കോൽ = get_താക്കോൽ_Name(scopep, controlSignalName);
                സ്ഥിരം std::size_t control_Signal_എണ്ണം = m_scopeVars.എണ്ണം(control_Signal_താക്കോൽ);
                എങ്കിൽ (control_Signal_എണ്ണം == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_താക്കോൽ << "'.");
                    തിരികെ അസത്യം;
                }
                എങ്കിൽ (control_Signal_എണ്ണം > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    തിരികെ അസത്യം;
                }
                തിരികെ സത്യം;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    ശൂന്യം varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        ആകുമ്പോൾ (!scp.empty()) {
            സ്ഥിരം auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            എങ്കിൽ ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                സ്ഥിരം auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                എങ്കിൽ (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            എങ്കിൽ ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (prd_pos == string::npos) നിർത്തുക;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    ശൂന്യം addScopeVarEntry(സ്ഥിരം AstScope* സ്ഥിരം scopep, സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp,
                          സ്ഥിരം AstVar* സ്ഥിരം varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        എങ്കിൽ (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        സ്ഥിരം std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        സ്ഥിരം std::string scpName = whole.substr(0, dpos);
        സ്ഥിരം std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        സ്ഥിരം std::string varBasePretty = Ast_കണ്ണി::vpiName(VName::dehash(varBase));
        സ്ഥിരം std::string scpPretty = Ast_കണ്ണി::prettyName(VName::dehash(scpName));
        സ്ഥിരം std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        എങ്കിൽ (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    ശൂന്യം varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        വേണ്ടി (സ്ഥിരം ScopeModPair& smPair : m_scopes) {
            സ്ഥിരം AstScope* സ്ഥിരം scopep = smPair.first;
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം smodp = smPair.second;
            വേണ്ടി (സ്ഥിരം ModVarPair& mvPair : m_modVars) {
                സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = mvPair.first;
                സ്ഥിരം AstVar* സ്ഥിരം varp = mvPair.second;
                എങ്കിൽ (modp != smodp) തുടരുക;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    ശൂന്യം collectIfaceRefs(സ്ഥിരം AstScope* nodep) {
        സ്ഥിരം AstCell* സ്ഥിരം cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        എങ്കിൽ (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) തിരികെ;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        സ്ഥിരം std::string path = Ast_കണ്ണി::vpiName(nodep->name());
        സ്ഥിരം std::string instName = Ast_കണ്ണി::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        സ്ഥിരം std::string parentPath = path.substr(0, path.length() - instName.length());

        വേണ്ടി (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            സ്ഥിരം std::string refName = Ast_കണ്ണി::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            എങ്കിൽ (!VString::startsWith(refName, parentPath)) തുടരുക;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_കണ്ണി::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    ശൂന്യം buildIfaceRefTable() {
        എങ്കിൽ (m_ifaceRefs.empty()) തിരികെ;
        സ്ഥിരം std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](സ്ഥിരം IfaceRefData& a, സ്ഥിരം IfaceRefData& b) { തിരികെ a.m_suffix < b.m_suffix; });
        വേണ്ടി (സ്ഥിരം IfaceRefData& ird : m_ifaceRefs) {
            സ്ഥിരം std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            എങ്കിൽ (m_scopeNames.find(scopeSym) == m_scopeNames.end()) തുടരുക;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            സ്ഥിരം auto parent = m_scopeNames.find(ird.m_parentSym);
            എങ്കിൽ (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_വാക്കുകൾ_If(ird.m_name, സത്യം));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_വാക്കുകൾ_If(ird.m_suffix, സത്യം));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_വാക്കുകൾ_If(ird.m_modportName, സത്യം));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        എങ്കിൽ (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    ശൂന്യം buildVpiHierarchy() {
        വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeNames) {
            സ്ഥിരം std::string symName = itpair.second.m_symName;
            std::string above = symName;
            എങ്കിൽ (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            ആകുമ്പോൾ (!above.empty()) {
                സ്ഥിരം std::string::size_type pos = above.rfind("__");
                എങ്കിൽ (pos == std::string::npos) നിർത്തുക;
                above.resize(pos);
                എങ്കിൽ (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].തള്ളുക_back(symName);
                    നിർത്തുക;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        എങ്കിൽ (v3Global.opt.vpi()) buildVpiHierarchy();

        എങ്കിൽ (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            വേണ്ടി (സ്ഥിരം auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](സ്ഥിരം ScopeModPair& a, സ്ഥിരം ScopeModPair& b) {
                             തിരികെ a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](സ്ഥിരം AstCFunc* ap, സ്ഥിരം AstCFunc* bp) {
                             എങ്കിൽ (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 തിരികെ bp->dpiImportPrototype();
                             }
                             തിരികെ ap->name() < bp->name();
                         });

        // Output
        എങ്കിൽ (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        എങ്കിൽ (v3Global.dpi()) {
            emitDpiHdr();
            എങ്കിൽ (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->isConstPool()) തിരികെ;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCellInlineScope* nodep) override {
        സ്ഥിരം std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        സ്ഥിരം std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        സ്ഥിരം പൂർണ്ണസംഖ്യ timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_കണ്ണി::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    ശൂന്യം visit(AstScope* nodep) override {
        എങ്കിൽ (VN_IS(m_modp, Class)) തിരികെ;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        എങ്കിൽ (v3Global.opt.vpi() && !nodep->is_മുകൾ()) {
            സ്ഥിരം std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            സ്ഥിരം പൂർണ്ണസംഖ്യ timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_കണ്ണി::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstScopeName* nodep) override {
        എങ്കിൽ (nodep->selfRelative()) തിരികെ;  // Its absolute copy is under the scope
        സ്ഥിരം std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        സ്ഥിരം പൂർണ്ണസംഖ്യ timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        എങ്കിൽ (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } അല്ലെങ്കിൽ {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    ശൂന്യം visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        എങ്കിൽ ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        എങ്കിൽ (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_അക്കം(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_അക്കം(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        എങ്കിൽ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.തള്ളുക_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ശൂന്യം visit(AstConst*) override {}
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    explicit EmitCSyms(AstNetlist* nodep, തർക്കം dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

ശൂന്യം EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_മുകൾ_Include();
    puts("#include \"verilated.h\"\n");
    എങ്കിൽ (v3Global.needTraceDumper()) {
        വേണ്ടി (സ്ഥിരം string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    എങ്കിൽ (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + മുകൾ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    വേണ്ടി (Ast_കണ്ണി_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), കണ്ണി_Module);
        എങ്കിൽ (VN_IS(nodep, Class)) തുടരുക;  // Class included earlier
        എങ്കിൽ (nodep->isConstPool()) തുടരുക;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    എങ്കിൽ (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeFuncs) {
            സ്ഥിരം AstCFunc* സ്ഥിരം funcp = itpair.second.m_cfuncp;
            എങ്കിൽ (!funcp->dpiExportImpl()) തുടരുക;
            സ്ഥിരം std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            സ്ഥിരം std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        വേണ്ടി (സ്ഥിരം std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(അസത്യം);  // public:

    puts("// INTERNAL STATE\n");
    puts(മുകൾ_Class_Name() + "* const __Vm_modelp;\n");

    എങ്കിൽ (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    എങ്കിൽ (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    എങ്കിൽ (v3Global.hasEvents()) {
        എങ്കിൽ (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } അല്ലെങ്കിൽ {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    എങ്കിൽ (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    എങ്കിൽ (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    എങ്കിൽ (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    എങ്കിൽ (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    എങ്കിൽ (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::അക്കം_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    വേണ്ടി (സ്ഥിരം ScopeModPair& itpair : m_scopes) {
        സ്ഥിരം AstScope* സ്ഥിരം scopep = itpair.first;
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = itpair.second;
        എങ്കിൽ (VN_IS(modp, Class)) തുടരുക;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    എങ്കിൽ (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    എങ്കിൽ (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeNames) {
            സ്ഥിരം ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    എങ്കിൽ (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + മുകൾ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    എങ്കിൽ (v3Global.hasEvents()) {
        എങ്കിൽ (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } അല്ലെങ്കിൽ {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        എങ്കിൽ (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } അല്ലെങ്കിൽ {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        എങ്കിൽ (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } അല്ലെങ്കിൽ {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    എങ്കിൽ (v3Global.needTraceDumper()) {
        എങ്കിൽ (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    എങ്കിൽ (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

ശൂന്യം EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    തർക്കം needs_New_വരി = അസത്യം;
    വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeFuncs) {
        സ്ഥിരം AstCFunc* സ്ഥിരം funcp = itpair.second.m_cfuncp;
        എങ്കിൽ (!funcp->dpiExportImpl()) തുടരുക;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_വരി = സത്യം;
    }
    എങ്കിൽ (needs_New_വരി) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    വേണ്ടി (സ്ഥിരം ScopeModPair& itpair : m_scopes) {
        എങ്കിൽ (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    വേണ്ടി (സ്ഥിരം std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    എങ്കിൽ (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    എങ്കിൽ (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        വേണ്ടി (സ്ഥിരം auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        എങ്കിൽ (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        എങ്കിൽ (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

ശൂന്യം EmitCSyms::emitVarTables() {
    എങ്കിൽ (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) തിരികെ;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    വേണ്ടി (സ്ഥിരം auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        വേണ്ടി (സ്ഥിരം std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    എങ്കിൽ (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        വേണ്ടി (സ്ഥിരം std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    എങ്കിൽ (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        വേണ്ടി (സ്ഥിരം std::string& row : m_ifaceRefTableRows) {
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

ശൂന്യം EmitCSyms::emitScopeHier(TextStmts& stmts, തർക്കം destroy) {
    എങ്കിൽ (!v3Global.opt.vpi()) തിരികെ;

    എങ്കിൽ (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } അല്ലെങ്കിൽ {
        stmts.add("// Set up scope hierarchy");
    }

    സ്ഥിരം std::string method = destroy ? "remove" : "add";
    വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeNames) {
        എങ്കിൽ (itpair.first == "TOP") തുടരുക;
        സ്ഥിരം ScopeData& sd = itpair.second;
        സ്ഥിരം std::string& name = sd.m_prettyName;
        സ്ഥിരം std::string& scopeType = sd.m_type;
        എങ്കിൽ (name.find('.') != string::npos) തുടരുക;
        എങ്കിൽ (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") തുടരുക;
        സ്ഥിരം std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    വേണ്ടി (സ്ഥിരം auto& itpair : m_vpiScopeHierarchy) {
        സ്ഥിരം std::string fromName = scopeSymString(itpair.first);
        സ്ഥിരം std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        വേണ്ടി (സ്ഥിരം std::string& name : itpair.second) {
            സ്ഥിരം std::string toName = scopeSymString(name);
            സ്ഥിരം std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    എങ്കിൽ (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t അട്ടി_Size = V3_അട്ടി_എണ്ണം::എണ്ണം(v3Global.rootp());
        എങ്കിൽ (v3Global.opt.debug_അട്ടി_Check()) അട്ടി_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", അട്ടി_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, അട്ടി_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(അട്ടി_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    വേണ്ടി (സ്ഥിരം ScopeModPair& itpair : m_scopes) {
        സ്ഥിരം AstScope* സ്ഥിരം scopep = itpair.first;
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = itpair.second;
        എങ്കിൽ (modp->is_മുകൾ()) തുടരുക;
        സ്ഥിരം std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_വാക്കുകൾ_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    എങ്കിൽ (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        എങ്കിൽ (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        എങ്കിൽ (v3Global.opt.mtasks()) {
            v3Global.rootp()->മുകൾ_Modulep()->foreach([&](സ്ഥിരം AstExecGraph* execGraphp) {
                വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    സ്ഥിരം ExecMTask& mt = static_cast<സ്ഥിരം ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    എങ്കിൽ (!v3Global.rootp()->timeunit().isNone()) {
        സ്ഥിരം std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    എങ്കിൽ (!v3Global.rootp()->timeprecision().isNone()) {
        സ്ഥിരം std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    വേണ്ടി (സ്ഥിരം auto& i : m_scopes) {
        സ്ഥിരം AstScope* സ്ഥിരം scopep = i.first;
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = i.second;
        സ്ഥിരം AstScope* സ്ഥിരം abovep = scopep->aboveScopep();
        എങ്കിൽ (!abovep) തുടരുക;

        സ്ഥിരം std::string protName = VIdProtect::protect_വാക്കുകൾ_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        എങ്കിൽ (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } അല്ലെങ്കിൽ {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    എങ്കിൽ (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        വേണ്ടി (സ്ഥിരം ScopeModPair& i : m_scopes) {
            സ്ഥിരം AstScope* സ്ഥിരം scopep = i.first;
            Ast_കണ്ണി_Module* സ്ഥിരം modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            സ്ഥിരം തർക്കം first = !modp->user1();
            modp->user1(സത്യം);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    വേണ്ടി (സ്ഥിരം ScopeModPair& i : m_scopes) {
        സ്ഥിരം AstScope* സ്ഥിരം scopep = i.first;
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = i.second;
        എങ്കിൽ (!modp->verilatorLib()) തുടരുക;
        സ്ഥിരം AstVar* hashp = nullptr;
        സ്ഥിരം AstVar* handlep = nullptr;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
            എങ്കിൽ (!varp) തുടരുക;
            എങ്കിൽ (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            എങ്കിൽ (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        സ്ഥിരം uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_മുകൾ_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    എങ്കിൽ (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        സ്ഥിരം std::string symClass = symClassName();
        വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeNames) {
            സ്ഥിരം ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_വാക്കുകൾ_If(sd.m_prettyName, സത്യം));
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
    എങ്കിൽ (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, അസത്യം);

    എങ്കിൽ (v3Global.dpi()) {
        വേണ്ടി (സ്ഥിരം std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeFuncs) {
                സ്ഥിരം ScopeFuncData& sfd = itpair.second;
                സ്ഥിരം AstScopeName* സ്ഥിരം scopep = sfd.m_scopep;
                സ്ഥിരം AstCFunc* സ്ഥിരം funcp = sfd.m_cfuncp;
                സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = sfd.m_modp;
                എങ്കിൽ (!funcp->dpiExportImpl()) തുടരുക;

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
    എങ്കിൽ (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        പൂർണ്ണസംഖ്യ tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        ആകുമ്പോൾ (it != m_scopeVars.cend()) {
            സ്ഥിരം std::string scopeName = it->second.m_scopeName;
            സ്ഥിരം AstScope* സ്ഥിരം instScopep = it->second.m_scopep;
            സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = it->second.m_modp;
            സ്ഥിരം std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            വേണ്ടി (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                സ്ഥിരം ScopeVarData& svd = it->second;
                സ്ഥിരം AstScope* സ്ഥിരം scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                സ്ഥിരം AstVar* സ്ഥിരം varp = svd.m_varp;
                സ്ഥിരം VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                സ്ഥിരം std::pair<തർക്കം, std::string> fc = isForceControlSignal(varp);
                എങ്കിൽ (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    തുടരുക;
                }

                std::string row;
                സ്ഥിരം TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                തിരഞ്ഞെടുപ്പ് (kind) {
                അവസ്ഥ TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    നിർത്തുക;
                അവസ്ഥ TableEntryKind::FORCEABLE_RESIDUAL: {
                    സ്ഥിരം std::string bounds = boundsString(dims);
                    residual.emplace_back(തിരുകുക_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    നിർത്തുക;
                }
                അവസ്ഥ TableEntryKind::PLAIN_RESIDUAL: {
                    സ്ഥിരം std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        തിരുകുക_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    നിർത്തുക;
                }
                സ്ഥിരസ്ഥിതി: v3fatalSrc("Bad case");
                }
                എങ്കിൽ (kind == TableEntryKind::TABLE_ROW) തുടരുക;
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), കണ്ണി_UOr_Struct_DType)) {
                    എങ്കിൽ (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                എങ്കിൽ (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            എങ്കിൽ (!rows.empty()) {
                സ്ഥിരം size_t row_എണ്ണം = rows.size();
                std::string താക്കോൽ;
                വേണ്ടി (സ്ഥിരം std::string& r : rows) {
                    താക്കോൽ += r;
                    താക്കോൽ += '\0';
                }
                സ്ഥിരം auto itt = tableByRows.find(താക്കോൽ);
                std::string tableName;
                എങ്കിൽ (itt != tableByRows.end()) {
                    tableName = itt->second;
                } അല്ലെങ്കിൽ {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(താക്കോൽ), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_എണ്ണം) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            വേണ്ടി (സ്ഥിരം std::string& r : residual) stmts.add(r);
        }
    }

    തിരികെ stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, സത്യം);
    എങ്കിൽ (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    എങ്കിൽ (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + മുകൾ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    എങ്കിൽ (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    വേണ്ടി (സ്ഥിരം auto& itpair : m_scopeNames) {
        സ്ഥിരം ScopeData& sd = itpair.second;
        സ്ഥിരം std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    വേണ്ടി (സ്ഥിരം ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        സ്ഥിരം AstScope* സ്ഥിരം scopep = itpair.first;
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = itpair.second;
        എങ്കിൽ (modp->is_മുകൾ()) തുടരുക;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    തിരികെ stmts;
}

ശൂന്യം EmitCSyms::emitSymImp(സ്ഥിരം AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    സ്ഥിരം std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            സത്യം, "Symbol table implementation internals",
                            [ഇത്]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), സത്യം, സത്യം, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    സ്ഥിരം AstScope* സ്ഥിരം മുകൾ_Scopep = netlistp->മുകൾ_Scopep()->scopep();
    സ്ഥിരം std::string മുകൾ_Inst = "vlSymsp->" + instName(മുകൾ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    എങ്കിൽ (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + മുകൾ_Inst + ", vlSymsp, namep);\n");
    വേണ്ടി (സ്ഥിരം TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + മുകൾ_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    എങ്കിൽ (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    എങ്കിൽ (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    എങ്കിൽ (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    എങ്കിൽ (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    വേണ്ടി (സ്ഥിരം TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + മുകൾ_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    എങ്കിൽ (v3Global.needTraceDumper()) {
        എങ്കിൽ (!optSystemC()) {
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

    എങ്കിൽ (v3Global.opt.savable()) {
        വേണ്ടി (സ്ഥിരം തർക്കം& de : {അസത്യം, സത്യം}) {
            സ്ഥിരം std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            സ്ഥിരം std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            സ്ഥിരം std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            എങ്കിൽ (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            വേണ്ടി (സ്ഥിരം ScopeModPair& itpair : m_scopes) {
                സ്ഥിരം AstScope* സ്ഥിരം scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

ശൂന്യം EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(മുകൾ_Class_Name() + "__Dpi",
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
    വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം nodep : m_dpis) {
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    എങ്കിൽ (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    പൂർണ്ണസംഖ്യ firstExp = 0;
    പൂർണ്ണസംഖ്യ firstImp = 0;
    വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം nodep : m_dpis) {
        എങ്കിൽ (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) തുടരുക;

        സ്ഥിരം std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        എങ്കിൽ (nodep->dpiExportDispatcher()) {
            എങ്കിൽ (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        എങ്കിൽ (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } അല്ലെങ്കിൽ {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, അസത്യം) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

ശൂന്യം EmitCSyms::emitDpiStruct(സ്ഥിരം Ast_കണ്ണി_DType* dtypep, std::set<std::string>& namesr) {
    എങ്കിൽ (!dtypep->containsUnpackedStruct()) തിരികെ;
    സ്ഥിരം AstStructDType* സ്ഥിരം sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    സ്ഥിരം std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    എങ്കിൽ (namesr.എണ്ണം(name)) തിരികെ;
    // After the structures it contains
    വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    എങ്കിൽ (name.empty()) തിരികെ;  // Declared where used
    എങ്കിൽ (namesr.empty()) puts("// DPI TYPES\n");
    namesr.തിരുകുക(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

ശൂന്യം EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(മുകൾ_Class_Name() + "__Dpi", അസത്യം, സത്യം,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + മുകൾ_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + മുകൾ_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + മുകൾ_Class_Name() + ".h\"\n");
    puts("\n");

    വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം nodep : m_dpis) {
        എങ്കിൽ (!nodep->dpiExportDispatcher()) തുടരുക;

        സ്ഥിരം std::string name = nodep->name();
        സ്ഥിരം std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + മുകൾ_Class_Name() + "::" + name + "(");
        std::string comma;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isIO() && !portp->isFuncReturn()) {
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

ശൂന്യം V3EmitC::emitcSyms(തർക്കം dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
