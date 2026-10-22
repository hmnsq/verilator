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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3EmitC.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3ExecGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3LanguageWords.h"
#ଅନ୍ତର୍ଭାଵ "V3StackCount.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cstring>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
ସ୍ଥିର constexpr auto symClassName = &EmitCUtil::symClassName;
ସ୍ଥିର constexpr auto ଶୀର୍ଷ_Class_Name = &EmitCUtil::ଶୀର୍ଷ_Class_Name;

ଶ୍ରେଣୀ EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ଅଚଳ VNUser1InUse m_inuser1;

    // TYPES
    ସଂରଚନା ScopeData final {
        ଅଚଳ Ast_ଗଣ୍ଠି* m_nodep;  // Scope symbol-table entry corresponds to
        ଅଚଳ std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ଅଚଳ std::string m_prettyName;  // Pretty (unprotected) name for display
        ଅଚଳ std::string m_defName;  // Name registered for this scope
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ std::string& symName, ଅଚଳ std::string& prettyName,
                  ଅଚଳ std::string& defName, ପୂର୍ଣ୍ଣସଂଖ୍ୟା timeunit, ଅଚଳ std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    ସଂରଚନା IfaceRefData final {
        ଅଚଳ AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(ଅଚଳ AstScope* scopep, ଅଚଳ std::string& parentSym,
                     ଅଚଳ std::string& suffix, ଅଚଳ std::string& name,
                     ଅଚଳ std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    ସଂରଚନା ScopeFuncData final {
        ଅଚଳ AstScopeName* ଅଚଳ m_scopep;  // Scope the DPI export function is registered under
        ଅଚଳ AstCFunc* ଅଚଳ m_cfuncp;  // DPI export function
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module containing the DPI export function
        ScopeFuncData(ଅଚଳ AstScopeName* scopep, ଅଚଳ AstCFunc* funcp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    ସଂରଚନା ScopeVarData final {
        ଅଚଳ std::string m_scopeName;  // Full name of the scope containing the variable
        ଅଚଳ std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        ଅଚଳ AstVar* ଅଚଳ m_varp;  // Public variable
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module containing the variable
        ଅଚଳ AstScope* ଅଚଳ m_scopep;  // Scope containing the variable
        ScopeVarData(ଅଚଳ std::string& scopeName, ଅଚଳ std::string& varBasePretty,
                     ଅଚଳ AstVar* varp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<ଅଚଳ std::string, ScopeData>;
    using ScopeModPair = std::pair<ଅଚଳ AstScope*, Ast_ଗଣ୍ଠି_Module*>;
    using ModVarPair = std::pair<ଅଚଳ Ast_ଗଣ୍ଠି_Module*, ଅଚଳ AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ଅଚଳ AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ଅଚଳ std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ଅଚଳ std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ଅଚଳ std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_modCoverBins = 0;  // Per-module coverage bin number
    ଅଚଳ ତାର୍କିକ m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    ଶୂନ୍ୟ emitSymHdr();
    ଶୂନ୍ୟ emitSymImpPreamble();
    ଶୂନ୍ୟ emitVarTables();
    ଶୂନ୍ୟ emitScopeHier(TextStmts& stmts, ତାର୍କିକ destroy);
    ଶୂନ୍ୟ emitSymImp(ଅଚଳ AstNetlist* netlistp);
    ଶୂନ୍ୟ emitDpiHdr();
    ଶୂନ୍ୟ emitDpiStruct(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep, std::set<std::string>& namesr);
    ଶୂନ୍ୟ emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    ସ୍ଥିର ଶୂନ୍ୟ nameCheck(Ast_ଗଣ୍ଠି* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        ଯଦି (nodep->name().empty()) ଫେରନ୍ତୁ;
        ଅଚଳ AstCFunc* ଅଚଳ cfuncp = VN_CAST(nodep, CFunc);
        ଯଦି (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ଅଚଳ std::string rsvd = V3_Language_ଶବ୍ଦଗୁଡ଼ିକ::isKeyword(nodep->name());
            ଯଦି (rsvd != "") {
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

    ସ୍ଥିର std::string scopeSymString(ଅଚଳ std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        ଯେତେବେଳେ ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        ଯଦି (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        ଯଦି (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        ଯେତେବେଳେ ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        ଯେତେବେଳେ ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        ଫେରନ୍ତୁ out;
    }

    ସ୍ଥିର string scopeDecodeIdentifier(ଅଚଳ std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        ଯେତେବେଳେ (i < scpname.length()) {
            ଯଦି (scpname[i] == '\\') {
                ଯେତେବେଳେ (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } ଅନ୍ୟଥା {
                ଯେତେବେଳେ (i < scpname.length() && scpname[i] != '.') ++i;
                ଯଦି (i < scpname.length()) pos = i++;
            }
        }

        ଫେରନ୍ତୁ pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    ସଂରଚନା VarDims final {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା pdim = 0;  // Packed array dims plus a ranged basic leaf
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା udim = 0;
        std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> flatten() ଅଚଳ {
            std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> flat(unpackedLR);
            flat.ଭର୍ତ୍ତି(flat.end(), packedLR.cbegin(), packedLR.cend());
            ଫେରନ୍ତୁ flat;
        }
    };
    ସ୍ଥିର VarDims getVarDims(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ ମୂଳ_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = ମୂଳ_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType)) {
                ଯଦି (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->ବାମ(), adtypep->ଡାହାଣ());
                    ++d.pdim;
                } ଅନ୍ୟଥା {
                    d.unpackedLR.emplace_back(adtypep->ବାମ(), adtypep->ଡାହାଣ());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } ଅନ୍ୟଥା {
                ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp()) {
                    ଯଦି (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->ବାମ(), basicp->ଡାହାଣ());
                        ++d.pdim;
                    }
                }
                ଭାଙ୍ଗ;  // Non-array leaf
            }
        }
        ଫେରନ୍ତୁ d;
    }

    ସ୍ଥିର VarDims dimsFor(ଅଚଳ ScopeVarData& svd) { ଫେରନ୍ତୁ getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ସ୍ଥିର std::string boundsString(ଅଚଳ VarDims& d) {
        std::string bounds;
        ପାଇଁ (ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        ଫେରନ୍ତୁ bounds;
    }

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା getUnpackedElements(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ମୂଳ_Dtypep) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = 1;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = ମୂଳ_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(dtypep, UnpackArrayDType);
            ଯଦି (!adtypep) ଭାଙ୍ଗ;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        ଫେରନ୍ତୁ elements;
    }

    ସ୍ଥିର ତାର୍କିକ needsEmittedEntSize(ଅଚଳ std::string& vlEnumType) {
        ଫେରନ୍ତୁ vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ସ୍ଥିର std::pair<ତାର୍କିକ, std::string> isForceControlSignal(ଅଚଳ AstVar* ଅଚଳ signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        ପାଇଁ (ଅଚଳ std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ଅଚଳ std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            ଅଚଳ ତାର୍କିକ suffixFound = suffixPos != std::string::npos;
            ଅଚଳ ତାର୍କିକ suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            ଯଦି (!(suffixFound && suffixIsAtEnd)) ଜାରି;
            ଅଚଳ std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            ଫେରନ୍ତୁ std::pair<ତାର୍କିକ, std::string>{ସତ୍ୟ, baseSignalName};
        }
        ଫେରନ୍ତୁ std::pair<ତାର୍କିକ, std::string>{ମିଥ୍ୟା, ""};
    }

    ସ୍ଥିର ଶୂନ୍ୟ appendVarProperties(std::string& stmt, ଅଚଳ std::string& vlEnumType,
                                    ଅଚଳ std::string& vlEnumDir, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା udim, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା pdim,
                                    ଅଚଳ std::string& bounds, ଅଚଳ std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        ଯଦି (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } ଅନ୍ୟଥା {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ସ୍ଥିର std::string memberVlEnumDir(ଅଚଳ AstVar* ଅଚଳ varp,
                                       ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/ସତ୍ୟ);
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ skipDTypep = dtypep->skipRefp();
        ଯଦି (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp = skipDTypep->basicp()) {
            ଯଦି (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        ଫେରନ୍ତୁ out + ')';
    }

    // Name of the instance member for the given scope
    ସ୍ଥିର std::string instName(ଅଚଳ AstScope* scopep) {
        ଫେରନ୍ତୁ "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ସ୍ଥିର std::string ଭର୍ତ୍ତି_Var_Statement(ଅଚଳ ScopeVarData& svd, ଅଚଳ AstScope* ଅଚଳ scopep,
                                          ଅଚଳ AstVar* ଅଚଳ varp, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା udim, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା pdim,
                                          ଅଚଳ std::string& bounds) {
        ଅଚଳ std::string var_ପଦସମଷ୍ଟି = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ଅଚଳ std::string vlEnumType = varp->vlEnumType();
        ଅଚଳ ତାର୍କିକ needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        ଯଦି (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_ପଦସମଷ୍ଟି;
            stmt += "), false, ";
        } ଅନ୍ୟଥା ଯଦି (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_ପଦସମଷ୍ଟି;
            stmt += ".c_str())), true, ";
        } ଅନ୍ୟଥା {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ପଦସମଷ୍ଟି;
            stmt += "))), true, ";
        }

        ଅଚଳ std::string entSize = needsEntSize
                                        ? "sizeof(" + var_ପଦସମଷ୍ଟି + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        ଫେରନ୍ତୁ stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    ଗଣନା ଶ୍ରେଣୀ TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(ଅଚଳ ScopeVarData& svd, ଅଚଳ AstVar* ଅଚଳ varp,
                                      ଅଚଳ AstScope* ଅଚଳ scopep,
                                      ଅଚଳ std::string& modClassName, ଅଚଳ VarDims& dims,
                                      std::string& rowOut) ଅଚଳ {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା pdim = dims.pdim;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା udim = dims.udim;
        ଯଦି (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            ଫେରନ୍ତୁ TableEntryKind::FORCEABLE_RESIDUAL;
        }
        ଯଦି (udim + pdim > VPI_TABLE_MAX_DIMS) ଫେରନ୍ତୁ TableEntryKind::PLAIN_RESIDUAL;

        ଅଚଳ std::string vlEnumType = varp->vlEnumType();
        ଯଦି (needsEmittedEntSize(vlEnumType))
            ଫେରନ୍ତୁ TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        ଯଦି (varp->isParam()) ଫେରନ୍ତୁ TableEntryKind::PLAIN_RESIDUAL;

        ଅଚଳ std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ଅଚଳ std::string member = varName(varp);
        ଅଚଳ std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> dv;
        ପାଇଁ (ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>& lr : dims.flatten()) {
            dv.ଠେଲ_back(lr.first);
            dv.ଠେଲ_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            ଯଦି (i) row += ", ";
            row += std::to_string(i < static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        ଫେରନ୍ତୁ TableEntryKind::TABLE_ROW;
    }

    ସ୍ଥିର std::string ଭର୍ତ୍ତି_DType_Var_Statement(ଅଚଳ ScopeVarData& svd,
                                               ଅଚଳ AstScope* ଅଚଳ scopep,
                                               ଅଚଳ std::string& prettyName,
                                               ଅଚଳ std::string& cName,
                                               ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା udim,
                                               ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା pdim, ଅଚଳ std::string& bounds) {
        ଅଚଳ std::string vlEnumType = dtypep->vlEnumType();
        ଅଚଳ ତାର୍କିକ needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        ଅଚଳ std::string var_ପଦସମଷ୍ଟି = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        ଯଦି (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ପଦସମଷ୍ଟି;
            stmt += "))), true, ";
        } ଅନ୍ୟଥା {
            stmt += ", &(";
            stmt += var_ପଦସମଷ୍ଟି;
            stmt += "), false, ";
        }
        ଅଚଳ std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_ପଦସମଷ୍ଟି + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        ଫେରନ୍ତୁ stmt;
    }

    ସ୍ଥିର ଶୂନ୍ୟ addUOrStructMemberVars(std::vector<std::string>& stmts, ଅଚଳ ScopeVarData& svd,
                                       ଅଚଳ AstScope* ଅଚଳ scopep,
                                       ଅଚଳ std::string& prettyPrefix, ଅଚଳ std::string& cPrefix,
                                       ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep) {
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ itemDTypep = itemp->dtypep();
            ଯଦି (itemDTypep->vlEnumType().empty()) ଜାରି;
            ଅଚଳ std::string prettyName
                = prettyPrefix + "." + Ast_ଗଣ୍ଠି::vpiName(itemp->shortName());
            ଅଚଳ std::string cName = cPrefix + "." + itemp->nameProtect();
            ଅଚଳ VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(ଭର୍ତ୍ତି_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ subp
                = VN_CAST(itemDTypep->skipRefp(), ଗଣ୍ଠି_UOr_Struct_DType)) {
                ଯଦି (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    ଅଚଳ ScopeVarData& svd,
                                                    ଅଚଳ AstScope* ଅଚଳ scopep,
                                                    ଅଚଳ std::string& prettyPrefix,
                                                    ଅଚଳ std::string& cPrefix,
                                                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ skipDTypep = dtypep->skipRefp();
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep
                   = VN_CAST(skipDTypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଯଦି (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ସ୍ଥିର ଶୂନ୍ୟ addPackedMemberVars(std::vector<std::string>& stmts, ଅଚଳ ScopeVarData& svd,
                                    ଅଚଳ AstScope* ଅଚଳ scopep, ଅଚଳ std::string& prettyPrefix,
                                    ଅଚଳ std::string& cName, ଅଚଳ std::string& vlEnumType,
                                    ଅଚଳ std::string& entSize,
                                    ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep,
                                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା parentBitOffset, ଅଚଳ ତାର୍କିକ needsIndex) {
        ଅଚଳ std::string var_ପଦସମଷ୍ଟି = "vlSymsp->" + instName(scopep) + "." + cName;
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ itemDTypep = itemp->dtypep();
            ଅଚଳ std::string prettyName
                = prettyPrefix + "." + Ast_ଗଣ୍ଠି::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitOffset = parentBitOffset + itemp->lsb();
            ଅଚଳ VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            ଯଦି (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_ପଦସମଷ୍ଟି + "))), true, ";
            } ଅନ୍ୟଥା {
                stmt += ", &(" + var_ପଦସମଷ୍ଟି + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            ଯଦି (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* subDTypep = itemDTypep->skipRefp();
            ତାର୍କିକ isArray = ମିଥ୍ୟା;
            ଯେତେବେଳେ (ଅଚଳ AstPackArrayDType* ଅଚଳ adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = ସତ୍ୟ;
            }
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ subp = VN_CAST(subDTypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ସ୍ଥିର ଶୂନ୍ୟ addPackedAggMemberVars(std::vector<std::string>& stmts, ଅଚଳ ScopeVarData& svd,
                                       ଅଚଳ AstScope* ଅଚଳ scopep,
                                       ଅଚଳ std::string& prettyPrefix, ଅଚଳ std::string& cName,
                                       ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        std::string elemCName = cName;
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* elemDTypep = dtypep->skipRefp();
        ତାର୍କିକ isArray = ମିଥ୍ୟା;
        ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(elemDTypep, ଗଣ୍ଠି_Array_DType)) {
            ଯଦି (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = ସତ୍ୟ;
        }
        ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep = VN_CAST(elemDTypep, ଗଣ୍ଠି_UOr_Struct_DType);
        ଯଦି (!sdtypep || !sdtypep->packed()) ଫେରନ୍ତୁ;
        ଅଚଳ std::string var_ପଦସମଷ୍ଟି = "vlSymsp->" + instName(scopep) + "." + cName;
        ଅଚଳ std::string entSize
            = "sizeof(" + var_ପଦସମଷ୍ଟି + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string ଭର୍ତ୍ତି_Forceable_Var_Statement(ଅଚଳ ScopeVarData& svd, ଅଚଳ AstScope* ଅଚଳ scopep,
                                            ଅଚଳ AstVar* ଅଚଳ varp, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା udim,
                                            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା pdim, ଅଚଳ std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        ଅଚଳ std::string var_ପଦସମଷ୍ଟି = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        ପ୍ରତିଜ୍ଞା(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_ପଦସମଷ୍ଟି;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_ପଦସମଷ୍ଟି + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        ପାଇଁ (ଅଚଳ std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ଅଚଳ std::string enable_Signal_ଚାବି
                = get_ଚାବି_Name(scopep, varp->name() + forceControlSuffix);
            ଅଚଳ std::map<ଅଚଳ std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_ଚାବି);

            ଯଦି (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_ଚାବି << "'.");
            }

            ଅଚଳ ScopeVarData& svd = itpair->second;
            ଅଚଳ AstScope* ଅଚଳ scopep = svd.m_scopep;
            ଅଚଳ AstVar* ଅଚଳ varp = svd.m_varp;
            ଅଚଳ VarDims dims = dimsFor(svd);
            stmt
                += ଭର୍ତ୍ତି_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            ଯଦି (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        ଫେରନ୍ତୁ stmt;
    }

    ସ୍ଥିର std::string get_ଚାବି_Name(ଅଚଳ AstScope* ଅଚଳ scopep, ଅଚଳ std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        ଯଦି (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ଅଚଳ std::string::size_type dpos = whole.rfind("__DOT__");
        ଯଦି (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        ଅଚଳ std::string scpSym = scopeSymString(VName::dehash(scpName));
        ଫେରନ୍ତୁ scpSym + " " + signal_name;
    }

    ତାର୍କିକ baseSignalIsValid(ଅଚଳ AstScope* ଅଚଳ scopep, ଅଚଳ AstVar* ଅଚଳ controlSignalVarp,
                           ଅଚଳ std::string& baseSignalName) ଅଚଳ {
        ଅଚଳ std::string base_Signal_ଚାବି = get_ଚାବି_Name(scopep, baseSignalName);
        ଅଚଳ std::map<ଅଚଳ std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_ଚାବି);
        ଯଦି (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            ଅଚଳ AstVar* ଅଚଳ baseSignalVarp = baseSignalIt->second.m_varp;
            ଯଦି (m_scopeVars.ଗଣନା_2(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ baseSignalIsPublic(ଅଚଳ AstScope* ଅଚଳ scopep,
                            ଅଚଳ std::string& baseSignalName) ଅଚଳ {
        ଅଚଳ std::string base_Signal_ଚାବି = get_ଚାବି_Name(scopep, baseSignalName);
        ଅଚଳ std::map<ଅଚଳ std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_ଚାବି);
        ଯଦି (baseSignalIt == m_scopeVars.end()) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            ଅଚଳ AstVar* ଅଚଳ baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            ଯଦି (!baseSignalVarp->isSigPublic()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ forceControlSignalsAreValid(ଅଚଳ AstScope* ଅଚଳ scopep,
                                     ଅଚଳ AstVar* ଅଚଳ baseSignalVarp) ଅଚଳ {
        constexpr std::array<ଅଚଳ ଅକ୍ଷର*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        ଫେରନ୍ତୁ std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, ଏହି](ଅଚଳ std::string& forceControlSuffix) {
                ଅଚଳ std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                ଅଚଳ std::string control_Signal_ଚାବି = get_ଚାବି_Name(scopep, controlSignalName);
                ଅଚଳ std::size_t control_Signal_ଗଣନା = m_scopeVars.ଗଣନା_2(control_Signal_ଚାବି);
                ଯଦି (control_Signal_ଗଣନା == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_ଚାବି << "'.");
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                ଯଦି (control_Signal_ଗଣନା > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    ଶୂନ୍ୟ varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        ଯେତେବେଳେ (!scp.empty()) {
            ଅଚଳ auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            ଯଦି ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ଅଚଳ auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                ଯଦି (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            ଯଦି ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } ଅନ୍ୟଥା {
                ଯଦି (prd_pos == string::npos) ଭାଙ୍ଗ;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    ଶୂନ୍ୟ addScopeVarEntry(ଅଚଳ AstScope* ଅଚଳ scopep, ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp,
                          ଅଚଳ AstVar* ଅଚଳ varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        ଯଦି (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ଅଚଳ std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ଅଚଳ std::string scpName = whole.substr(0, dpos);
        ଅଚଳ std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ଅଚଳ std::string varBasePretty = Ast_ଗଣ୍ଠି::vpiName(VName::dehash(varBase));
        ଅଚଳ std::string scpPretty = Ast_ଗଣ୍ଠି::prettyName(VName::dehash(scpName));
        ଅଚଳ std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        ଯଦି (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    ଶୂନ୍ୟ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        ପାଇଁ (ଅଚଳ ScopeModPair& smPair : m_scopes) {
            ଅଚଳ AstScope* ଅଚଳ scopep = smPair.first;
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ smodp = smPair.second;
            ପାଇଁ (ଅଚଳ ModVarPair& mvPair : m_modVars) {
                ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = mvPair.first;
                ଅଚଳ AstVar* ଅଚଳ varp = mvPair.second;
                ଯଦି (modp != smodp) ଜାରି;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    ଶୂନ୍ୟ collectIfaceRefs(ଅଚଳ AstScope* nodep) {
        ଅଚଳ AstCell* ଅଚଳ cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        ଯଦି (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) ଫେରନ୍ତୁ;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ଅଚଳ std::string path = Ast_ଗଣ୍ଠି::vpiName(nodep->name());
        ଅଚଳ std::string instName = Ast_ଗଣ୍ଠି::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        ଅଚଳ std::string parentPath = path.substr(0, path.length() - instName.length());

        ପାଇଁ (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ଅଚଳ std::string refName = Ast_ଗଣ୍ଠି::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            ଯଦି (!VString::startsWith(refName, parentPath)) ଜାରି;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_ଗଣ୍ଠି::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    ଶୂନ୍ୟ buildIfaceRefTable() {
        ଯଦି (m_ifaceRefs.empty()) ଫେରନ୍ତୁ;
        ଅଚଳ std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](ଅଚଳ IfaceRefData& a, ଅଚଳ IfaceRefData& b) { ଫେରନ୍ତୁ a.m_suffix < b.m_suffix; });
        ପାଇଁ (ଅଚଳ IfaceRefData& ird : m_ifaceRefs) {
            ଅଚଳ std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            ଯଦି (m_scopeNames.find(scopeSym) == m_scopeNames.end()) ଜାରି;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            ଅଚଳ auto parent = m_scopeNames.find(ird.m_parentSym);
            ଯଦି (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(ird.m_name, ସତ୍ୟ));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(ird.m_suffix, ସତ୍ୟ));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(ird.m_modportName, ସତ୍ୟ));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        ଯଦି (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    ଶୂନ୍ୟ buildVpiHierarchy() {
        ପାଇଁ (ଅଚଳ auto& itpair : m_scopeNames) {
            ଅଚଳ std::string symName = itpair.second.m_symName;
            std::string above = symName;
            ଯଦି (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            ଯେତେବେଳେ (!above.empty()) {
                ଅଚଳ std::string::size_type pos = above.rfind("__");
                ଯଦି (pos == std::string::npos) ଭାଙ୍ଗ;
                above.resize(pos);
                ଯଦି (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].ଠେଲ_back(symName);
                    ଭାଙ୍ଗ;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        ଯଦି (v3Global.opt.vpi()) buildVpiHierarchy();

        ଯଦି (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            ପାଇଁ (ଅଚଳ auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](ଅଚଳ ScopeModPair& a, ଅଚଳ ScopeModPair& b) {
                             ଫେରନ୍ତୁ a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](ଅଚଳ AstCFunc* ap, ଅଚଳ AstCFunc* bp) {
                             ଯଦି (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 ଫେରନ୍ତୁ bp->dpiImportPrototype();
                             }
                             ଫେରନ୍ତୁ ap->name() < bp->name();
                         });

        // Output
        ଯଦି (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        ଯଦି (v3Global.dpi()) {
            emitDpiHdr();
            ଯଦି (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->isConstPool()) ଫେରନ୍ତୁ;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCellInlineScope* nodep) override {
        ଅଚଳ std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ଅଚଳ std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ଗଣ୍ଠି::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        ଯଦି (VN_IS(m_modp, Class)) ଫେରନ୍ତୁ;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        ଯଦି (v3Global.opt.vpi() && !nodep->is_ଶୀର୍ଷ()) {
            ଅଚଳ std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_ଗଣ୍ଠି::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {
        ଯଦି (nodep->selfRelative()) ଫେରନ୍ତୁ;  // Its absolute copy is under the scope
        ଅଚଳ std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        ଯଦି (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } ଅନ୍ୟଥା {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        ଯଦି ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        ଯଦି (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_ଅଙ୍କ(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_ଅଙ୍କ(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        ଯଦି (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ଠେଲ_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ଶୂନ୍ୟ visit(AstConst*) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    explicit EmitCSyms(AstNetlist* nodep, ତାର୍କିକ dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

ଶୂନ୍ୟ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_ଶୀର୍ଷ_Include();
    puts("#include \"verilated.h\"\n");
    ଯଦି (v3Global.needTraceDumper()) {
        ପାଇଁ (ଅଚଳ string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    ଯଦି (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + ଶୀର୍ଷ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Module);
        ଯଦି (VN_IS(nodep, Class)) ଜାରି;  // Class included earlier
        ଯଦି (nodep->isConstPool()) ଜାରି;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    ଯଦି (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        ପାଇଁ (ଅଚଳ auto& itpair : m_scopeFuncs) {
            ଅଚଳ AstCFunc* ଅଚଳ funcp = itpair.second.m_cfuncp;
            ଯଦି (!funcp->dpiExportImpl()) ଜାରି;
            ଅଚଳ std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            ଅଚଳ std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        ପାଇଁ (ଅଚଳ std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(ମିଥ୍ୟା);  // public:

    puts("// INTERNAL STATE\n");
    puts(ଶୀର୍ଷ_Class_Name() + "* const __Vm_modelp;\n");

    ଯଦି (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    ଯଦି (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    ଯଦି (v3Global.hasEvents()) {
        ଯଦି (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } ଅନ୍ୟଥା {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    ଯଦି (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    ଯଦି (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    ଯଦି (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    ଯଦି (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    ଯଦି (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::ଅଙ୍କ_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    ପାଇଁ (ଅଚଳ ScopeModPair& itpair : m_scopes) {
        ଅଚଳ AstScope* ଅଚଳ scopep = itpair.first;
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = itpair.second;
        ଯଦି (VN_IS(modp, Class)) ଜାରି;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    ଯଦି (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    ଯଦି (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        ପାଇଁ (ଅଚଳ auto& itpair : m_scopeNames) {
            ଅଚଳ ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    ଯଦି (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + ଶୀର୍ଷ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    ଯଦି (v3Global.hasEvents()) {
        ଯଦି (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } ଅନ୍ୟଥା {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        ଯଦି (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } ଅନ୍ୟଥା {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        ଯଦି (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } ଅନ୍ୟଥା {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    ଯଦି (v3Global.needTraceDumper()) {
        ଯଦି (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    ଯଦି (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

ଶୂନ୍ୟ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    ତାର୍କିକ needs_New_ଧାଡ଼ି = ମିଥ୍ୟା;
    ପାଇଁ (ଅଚଳ auto& itpair : m_scopeFuncs) {
        ଅଚଳ AstCFunc* ଅଚଳ funcp = itpair.second.m_cfuncp;
        ଯଦି (!funcp->dpiExportImpl()) ଜାରି;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_ଧାଡ଼ି = ସତ୍ୟ;
    }
    ଯଦି (needs_New_ଧାଡ଼ି) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    ପାଇଁ (ଅଚଳ ScopeModPair& itpair : m_scopes) {
        ଯଦି (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    ପାଇଁ (ଅଚଳ std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    ଯଦି (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    ଯଦି (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        ପାଇଁ (ଅଚଳ auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        ଯଦି (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        ଯଦି (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

ଶୂନ୍ୟ EmitCSyms::emitVarTables() {
    ଯଦି (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) ଫେରନ୍ତୁ;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    ପାଇଁ (ଅଚଳ auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        ପାଇଁ (ଅଚଳ std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ଯଦି (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        ପାଇଁ (ଅଚଳ std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ଯଦି (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        ପାଇଁ (ଅଚଳ std::string& row : m_ifaceRefTableRows) {
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

ଶୂନ୍ୟ EmitCSyms::emitScopeHier(TextStmts& stmts, ତାର୍କିକ destroy) {
    ଯଦି (!v3Global.opt.vpi()) ଫେରନ୍ତୁ;

    ଯଦି (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } ଅନ୍ୟଥା {
        stmts.add("// Set up scope hierarchy");
    }

    ଅଚଳ std::string method = destroy ? "remove" : "add";
    ପାଇଁ (ଅଚଳ auto& itpair : m_scopeNames) {
        ଯଦି (itpair.first == "TOP") ଜାରି;
        ଅଚଳ ScopeData& sd = itpair.second;
        ଅଚଳ std::string& name = sd.m_prettyName;
        ଅଚଳ std::string& scopeType = sd.m_type;
        ଯଦି (name.find('.') != string::npos) ଜାରି;
        ଯଦି (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") ଜାରି;
        ଅଚଳ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    ପାଇଁ (ଅଚଳ auto& itpair : m_vpiScopeHierarchy) {
        ଅଚଳ std::string fromName = scopeSymString(itpair.first);
        ଅଚଳ std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        ପାଇଁ (ଅଚଳ std::string& name : itpair.second) {
            ଅଚଳ std::string toName = scopeSymString(name);
            ଅଚଳ std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    ଯଦି (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t ଗଦା_Size = V3_ଗଦା_ଗଣନା::ଗଣନା_2(v3Global.rootp());
        ଯଦି (v3Global.opt.debug_ଗଦା_Check()) ଗଦା_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", ଗଦା_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, ଗଦା_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(ଗଦା_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    ପାଇଁ (ଅଚଳ ScopeModPair& itpair : m_scopes) {
        ଅଚଳ AstScope* ଅଚଳ scopep = itpair.first;
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = itpair.second;
        ଯଦି (modp->is_ଶୀର୍ଷ()) ଜାରି;
        ଅଚଳ std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    ଯଦି (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        ଯଦି (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        ଯଦି (v3Global.opt.mtasks()) {
            v3Global.rootp()->ଶୀର୍ଷ_Modulep()->foreach([&](ଅଚଳ AstExecGraph* execGraphp) {
                ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    ଅଚଳ ExecMTask& mt = static_cast<ଅଚଳ ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    ଯଦି (!v3Global.rootp()->timeunit().isNone()) {
        ଅଚଳ std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    ଯଦି (!v3Global.rootp()->timeprecision().isNone()) {
        ଅଚଳ std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    ପାଇଁ (ଅଚଳ auto& i : m_scopes) {
        ଅଚଳ AstScope* ଅଚଳ scopep = i.first;
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = i.second;
        ଅଚଳ AstScope* ଅଚଳ abovep = scopep->aboveScopep();
        ଯଦି (!abovep) ଜାରି;

        ଅଚଳ std::string protName = VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        ଯଦି (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } ଅନ୍ୟଥା {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    ଯଦି (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        ପାଇଁ (ଅଚଳ ScopeModPair& i : m_scopes) {
            ଅଚଳ AstScope* ଅଚଳ scopep = i.first;
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ଅଚଳ ତାର୍କିକ first = !modp->user1();
            modp->user1(ସତ୍ୟ);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    ପାଇଁ (ଅଚଳ ScopeModPair& i : m_scopes) {
        ଅଚଳ AstScope* ଅଚଳ scopep = i.first;
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = i.second;
        ଯଦି (!modp->verilatorLib()) ଜାରି;
        ଅଚଳ AstVar* hashp = nullptr;
        ଅଚଳ AstVar* handlep = nullptr;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
            ଯଦି (!varp) ଜାରି;
            ଯଦି (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            ଯଦି (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ଅଚଳ uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        ଯଦି (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_ଶୀର୍ଷ_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    ଯଦି (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        ଅଚଳ std::string symClass = symClassName();
        ପାଇଁ (ଅଚଳ auto& itpair : m_scopeNames) {
            ଅଚଳ ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(sd.m_prettyName, ସତ୍ୟ));
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
    ଯଦି (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, ମିଥ୍ୟା);

    ଯଦି (v3Global.dpi()) {
        ପାଇଁ (ଅଚଳ std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            ପାଇଁ (ଅଚଳ auto& itpair : m_scopeFuncs) {
                ଅଚଳ ScopeFuncData& sfd = itpair.second;
                ଅଚଳ AstScopeName* ଅଚଳ scopep = sfd.m_scopep;
                ଅଚଳ AstCFunc* ଅଚଳ funcp = sfd.m_cfuncp;
                ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = sfd.m_modp;
                ଯଦି (!funcp->dpiExportImpl()) ଜାରି;

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
    ଯଦି (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        ଯେତେବେଳେ (it != m_scopeVars.cend()) {
            ଅଚଳ std::string scopeName = it->second.m_scopeName;
            ଅଚଳ AstScope* ଅଚଳ instScopep = it->second.m_scopep;
            ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = it->second.m_modp;
            ଅଚଳ std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            ପାଇଁ (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                ଅଚଳ ScopeVarData& svd = it->second;
                ଅଚଳ AstScope* ଅଚଳ scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ଅଚଳ AstVar* ଅଚଳ varp = svd.m_varp;
                ଅଚଳ VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ଅଚଳ std::pair<ତାର୍କିକ, std::string> fc = isForceControlSignal(varp);
                ଯଦି (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    ଜାରି;
                }

                std::string row;
                ଅଚଳ TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                ବାଛ (kind) {
                ସ୍ଥିତି TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି TableEntryKind::FORCEABLE_RESIDUAL: {
                    ଅଚଳ std::string bounds = boundsString(dims);
                    residual.emplace_back(ଭର୍ତ୍ତି_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି TableEntryKind::PLAIN_RESIDUAL: {
                    ଅଚଳ std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        ଭର୍ତ୍ତି_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    ଭାଙ୍ଗ;
                }
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3fatalSrc("Bad case");
                }
                ଯଦି (kind == TableEntryKind::TABLE_ROW) ଜାରି;
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ଗଣ୍ଠି_UOr_Struct_DType)) {
                    ଯଦି (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } ଅନ୍ୟଥା ଯଦି (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                ଯଦି (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            ଯଦି (!rows.empty()) {
                ଅଚଳ size_t row_ଗଣନା = rows.size();
                std::string ଚାବି;
                ପାଇଁ (ଅଚଳ std::string& r : rows) {
                    ଚାବି += r;
                    ଚାବି += '\0';
                }
                ଅଚଳ auto itt = tableByRows.find(ଚାବି);
                std::string tableName;
                ଯଦି (itt != tableByRows.end()) {
                    tableName = itt->second;
                } ଅନ୍ୟଥା {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(ଚାବି), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_ଗଣନା) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            ପାଇଁ (ଅଚଳ std::string& r : residual) stmts.add(r);
        }
    }

    ଫେରନ୍ତୁ stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, ସତ୍ୟ);
    ଯଦି (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    ଯଦି (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + ଶୀର୍ଷ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    ଯଦି (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    ପାଇଁ (ଅଚଳ auto& itpair : m_scopeNames) {
        ଅଚଳ ScopeData& sd = itpair.second;
        ଅଚଳ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    ପାଇଁ (ଅଚଳ ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        ଅଚଳ AstScope* ଅଚଳ scopep = itpair.first;
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = itpair.second;
        ଯଦି (modp->is_ଶୀର୍ଷ()) ଜାରି;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    ଫେରନ୍ତୁ stmts;
}

ଶୂନ୍ୟ EmitCSyms::emitSymImp(ଅଚଳ AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ଅଚଳ std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            ସତ୍ୟ, "Symbol table implementation internals",
                            [ଏହି]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), ସତ୍ୟ, ସତ୍ୟ, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ଅଚଳ AstScope* ଅଚଳ ଶୀର୍ଷ_Scopep = netlistp->ଶୀର୍ଷ_Scopep()->scopep();
    ଅଚଳ std::string ଶୀର୍ଷ_Inst = "vlSymsp->" + instName(ଶୀର୍ଷ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    ଯଦି (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + ଶୀର୍ଷ_Inst + ", vlSymsp, namep);\n");
    ପାଇଁ (ଅଚଳ TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + ଶୀର୍ଷ_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    ଯଦି (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    ଯଦି (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    ଯଦି (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    ଯଦି (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    ପାଇଁ (ଅଚଳ TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + ଶୀର୍ଷ_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    ଯଦି (v3Global.needTraceDumper()) {
        ଯଦି (!optSystemC()) {
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

    ଯଦି (v3Global.opt.savable()) {
        ପାଇଁ (ଅଚଳ ତାର୍କିକ& de : {ମିଥ୍ୟା, ସତ୍ୟ}) {
            ଅଚଳ std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ଅଚଳ std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ଅଚଳ std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            ଯଦି (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            ପାଇଁ (ଅଚଳ ScopeModPair& itpair : m_scopes) {
                ଅଚଳ AstScope* ଅଚଳ scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

ଶୂନ୍ୟ EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(ଶୀର୍ଷ_Class_Name() + "__Dpi",
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
    ପାଇଁ (ଅଚଳ AstCFunc* ଅଚଳ nodep : m_dpis) {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    ଯଦି (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstExp = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstImp = 0;
    ପାଇଁ (ଅଚଳ AstCFunc* ଅଚଳ nodep : m_dpis) {
        ଯଦି (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) ଜାରି;

        ଅଚଳ std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        ଯଦି (nodep->dpiExportDispatcher()) {
            ଯଦି (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } ଅନ୍ୟଥା {
            ଯଦି (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        ଯଦି (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } ଅନ୍ୟଥା {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, ମିଥ୍ୟା) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

ଶୂନ୍ୟ EmitCSyms::emitDpiStruct(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep, std::set<std::string>& namesr) {
    ଯଦି (!dtypep->containsUnpackedStruct()) ଫେରନ୍ତୁ;
    ଅଚଳ AstStructDType* ଅଚଳ sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ଅଚଳ std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    ଯଦି (namesr.ଗଣନା_2(name)) ଫେରନ୍ତୁ;
    // After the structures it contains
    ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    ଯଦି (name.empty()) ଫେରନ୍ତୁ;  // Declared where used
    ଯଦି (namesr.empty()) puts("// DPI TYPES\n");
    namesr.ଭର୍ତ୍ତି(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

ଶୂନ୍ୟ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(ଶୀର୍ଷ_Class_Name() + "__Dpi", ମିଥ୍ୟା, ସତ୍ୟ,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + ଶୀର୍ଷ_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + ଶୀର୍ଷ_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + ଶୀର୍ଷ_Class_Name() + ".h\"\n");
    puts("\n");

    ପାଇଁ (ଅଚଳ AstCFunc* ଅଚଳ nodep : m_dpis) {
        ଯଦି (!nodep->dpiExportDispatcher()) ଜାରି;

        ଅଚଳ std::string name = nodep->name();
        ଅଚଳ std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + ଶୀର୍ଷ_Class_Name() + "::" + name + "(");
        std::string comma;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isIO() && !portp->isFuncReturn()) {
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

ଶୂନ୍ୟ V3EmitC::emitcSyms(ତାର୍କିକ dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
