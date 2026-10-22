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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3EmitC.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3ExecGraph.h"
#અન્તર્ભાવ "V3LanguageWords.h"
#અન્તર્ભાવ "V3StackCount.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cstring>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
સ્થિર constexpr auto symClassName = &EmitCUtil::symClassName;
સ્થિર constexpr auto ટોચ_Class_Name = &EmitCUtil::ટોચ_Class_Name;

વર્ગ EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    અચલ VNUser1InUse m_inuser1;

    // TYPES
    સંરચના ScopeData final {
        અચલ Ast_ગાંઠ* m_nodep;  // Scope symbol-table entry corresponds to
        અચલ std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        અચલ std::string m_prettyName;  // Pretty (unprotected) name for display
        અચલ std::string m_defName;  // Name registered for this scope
        અચલ પૂર્ણાંક m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(અચલ Ast_ગાંઠ* nodep, અચલ std::string& symName, અચલ std::string& prettyName,
                  અચલ std::string& defName, પૂર્ણાંક timeunit, અચલ std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    સંરચના IfaceRefData final {
        અચલ AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(અચલ AstScope* scopep, અચલ std::string& parentSym,
                     અચલ std::string& suffix, અચલ std::string& name,
                     અચલ std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    સંરચના ScopeFuncData final {
        અચલ AstScopeName* અચલ m_scopep;  // Scope the DPI export function is registered under
        અચલ AstCFunc* અચલ m_cfuncp;  // DPI export function
        અચલ Ast_ગાંઠ_Module* અચલ m_modp;  // Module containing the DPI export function
        ScopeFuncData(અચલ AstScopeName* scopep, અચલ AstCFunc* funcp, અચલ Ast_ગાંઠ_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    સંરચના ScopeVarData final {
        અચલ std::string m_scopeName;  // Full name of the scope containing the variable
        અચલ std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        અચલ AstVar* અચલ m_varp;  // Public variable
        અચલ Ast_ગાંઠ_Module* અચલ m_modp;  // Module containing the variable
        અચલ AstScope* અચલ m_scopep;  // Scope containing the variable
        ScopeVarData(અચલ std::string& scopeName, અચલ std::string& varBasePretty,
                     અચલ AstVar* varp, અચલ Ast_ગાંઠ_Module* modp, અચલ AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<અચલ std::string, ScopeData>;
    using ScopeModPair = std::pair<અચલ AstScope*, Ast_ગાંઠ_Module*>;
    using ModVarPair = std::pair<અચલ Ast_ગાંઠ_Module*, અચલ AstVar*>;

    // Vars with more dims take the residual per-statement path.
    સ્થિર constexpr પૂર્ણાંક VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    અચલ AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<અચલ std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<અચલ std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<અચલ std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    પૂર્ણાંક m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    પૂર્ણાંક m_modCoverBins = 0;  // Per-module coverage bin number
    અચલ તાર્કિક m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    શૂન્ય emitSymHdr();
    શૂન્ય emitSymImpPreamble();
    શૂન્ય emitVarTables();
    શૂન્ય emitScopeHier(TextStmts& stmts, તાર્કિક destroy);
    શૂન્ય emitSymImp(અચલ AstNetlist* netlistp);
    શૂન્ય emitDpiHdr();
    શૂન્ય emitDpiStruct(અચલ Ast_ગાંઠ_DType* dtypep, std::set<std::string>& namesr);
    શૂન્ય emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    સ્થિર શૂન્ય nameCheck(Ast_ગાંઠ* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        જો (nodep->name().empty()) પાછા;
        અચલ AstCFunc* અચલ cfuncp = VN_CAST(nodep, CFunc);
        જો (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            અચલ std::string rsvd = V3_Language_શબ્દો::isKeyword(nodep->name());
            જો (rsvd != "") {
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

    સ્થિર std::string scopeSymString(અચલ std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        જ્યારે ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        જો (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        જો (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        જ્યારે ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        જ્યારે ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        પાછા out;
    }

    સ્થિર string scopeDecodeIdentifier(અચલ std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        જ્યારે (i < scpname.length()) {
            જો (scpname[i] == '\\') {
                જ્યારે (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } નહીંતર {
                જ્યારે (i < scpname.length() && scpname[i] != '.') ++i;
                જો (i < scpname.length()) pos = i++;
            }
        }

        પાછા pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    સંરચના VarDims final {
        પૂર્ણાંક pdim = 0;  // Packed array dims plus a ranged basic leaf
        પૂર્ણાંક udim = 0;
        std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> flatten() અચલ {
            std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> flat(unpackedLR);
            flat.ઉમેરો(flat.end(), packedLR.cbegin(), packedLR.cend());
            પાછા flat;
        }
    };
    સ્થિર VarDims getVarDims(અચલ Ast_ગાંઠ_DType* અચલ મૂળ_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        માટે (અચલ Ast_ગાંઠ_DType* dtypep = મૂળ_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            જો (અચલ Ast_ગાંઠ_Array_DType* અચલ adtypep = VN_CAST(dtypep, ગાંઠ_Array_DType)) {
                જો (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->ડાબું(), adtypep->જમણું());
                    ++d.pdim;
                } નહીંતર {
                    d.unpackedLR.emplace_back(adtypep->ડાબું(), adtypep->જમણું());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } નહીંતર {
                જો (અચલ AstBasicDType* અચલ basicp = dtypep->basicp()) {
                    જો (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->ડાબું(), basicp->જમણું());
                        ++d.pdim;
                    }
                }
                તોડો;  // Non-array leaf
            }
        }
        પાછા d;
    }

    સ્થિર VarDims dimsFor(અચલ ScopeVarData& svd) { પાછા getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    સ્થિર std::string boundsString(અચલ VarDims& d) {
        std::string bounds;
        માટે (અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        પાછા bounds;
    }

    સ્થિર પૂર્ણાંક getUnpackedElements(અચલ Ast_ગાંઠ_DType* મૂળ_Dtypep) {
        પૂર્ણાંક elements = 1;
        માટે (અચલ Ast_ગાંઠ_DType* dtypep = મૂળ_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(dtypep, UnpackArrayDType);
            જો (!adtypep) તોડો;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        પાછા elements;
    }

    સ્થિર તાર્કિક needsEmittedEntSize(અચલ std::string& vlEnumType) {
        પાછા vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    સ્થિર std::pair<તાર્કિક, std::string> isForceControlSignal(અચલ AstVar* અચલ signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        માટે (અચલ std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            અચલ std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            અચલ તાર્કિક suffixFound = suffixPos != std::string::npos;
            અચલ તાર્કિક suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            જો (!(suffixFound && suffixIsAtEnd)) ચાલુ;
            અચલ std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            પાછા std::pair<તાર્કિક, std::string>{સાચું, baseSignalName};
        }
        પાછા std::pair<તાર્કિક, std::string>{ખોટું, ""};
    }

    સ્થિર શૂન્ય appendVarProperties(std::string& stmt, અચલ std::string& vlEnumType,
                                    અચલ std::string& vlEnumDir, અચલ પૂર્ણાંક udim, અચલ પૂર્ણાંક pdim,
                                    અચલ std::string& bounds, અચલ std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        જો (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } નહીંતર {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    સ્થિર std::string memberVlEnumDir(અચલ AstVar* અચલ varp,
                                       અચલ Ast_ગાંઠ_DType* અચલ dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/સાચું);
        અચલ Ast_ગાંઠ_DType* અચલ skipDTypep = dtypep->skipRefp();
        જો (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        જો (અચલ AstBasicDType* અચલ basicp = skipDTypep->basicp()) {
            જો (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        પાછા out + ')';
    }

    // Name of the instance member for the given scope
    સ્થિર std::string instName(અચલ AstScope* scopep) {
        પાછા "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    સ્થિર std::string ઉમેરો_Var_Statement(અચલ ScopeVarData& svd, અચલ AstScope* અચલ scopep,
                                          અચલ AstVar* અચલ varp, અચલ પૂર્ણાંક udim, અચલ પૂર્ણાંક pdim,
                                          અચલ std::string& bounds) {
        અચલ std::string var_પદાવલિ = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        અચલ std::string vlEnumType = varp->vlEnumType();
        અચલ તાર્કિક needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        જો (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_પદાવલિ;
            stmt += "), false, ";
        } નહીંતર જો (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_પદાવલિ;
            stmt += ".c_str())), true, ";
        } નહીંતર {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_પદાવલિ;
            stmt += "))), true, ";
        }

        અચલ std::string entSize = needsEntSize
                                        ? "sizeof(" + var_પદાવલિ + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        પાછા stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    ગણના વર્ગ TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(અચલ ScopeVarData& svd, અચલ AstVar* અચલ varp,
                                      અચલ AstScope* અચલ scopep,
                                      અચલ std::string& modClassName, અચલ VarDims& dims,
                                      std::string& rowOut) અચલ {
        અચલ પૂર્ણાંક pdim = dims.pdim;
        અચલ પૂર્ણાંક udim = dims.udim;
        જો (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            પાછા TableEntryKind::FORCEABLE_RESIDUAL;
        }
        જો (udim + pdim > VPI_TABLE_MAX_DIMS) પાછા TableEntryKind::PLAIN_RESIDUAL;

        અચલ std::string vlEnumType = varp->vlEnumType();
        જો (needsEmittedEntSize(vlEnumType))
            પાછા TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        જો (varp->isParam()) પાછા TableEntryKind::PLAIN_RESIDUAL;

        અચલ std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        અચલ std::string member = varName(varp);
        અચલ std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<પૂર્ણાંક> dv;
        માટે (અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક>& lr : dims.flatten()) {
            dv.ધકેલો_back(lr.first);
            dv.ધકેલો_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        માટે (પૂર્ણાંક i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            જો (i) row += ", ";
            row += std::to_string(i < static_cast<પૂર્ણાંક>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        પાછા TableEntryKind::TABLE_ROW;
    }

    સ્થિર std::string ઉમેરો_DType_Var_Statement(અચલ ScopeVarData& svd,
                                               અચલ AstScope* અચલ scopep,
                                               અચલ std::string& prettyName,
                                               અચલ std::string& cName,
                                               અચલ Ast_ગાંઠ_DType* અચલ dtypep, અચલ પૂર્ણાંક udim,
                                               અચલ પૂર્ણાંક pdim, અચલ std::string& bounds) {
        અચલ std::string vlEnumType = dtypep->vlEnumType();
        અચલ તાર્કિક needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        અચલ std::string var_પદાવલિ = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        જો (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_પદાવલિ;
            stmt += "))), true, ";
        } નહીંતર {
            stmt += ", &(";
            stmt += var_પદાવલિ;
            stmt += "), false, ";
        }
        અચલ std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_પદાવલિ + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        પાછા stmt;
    }

    સ્થિર શૂન્ય addUOrStructMemberVars(std::vector<std::string>& stmts, અચલ ScopeVarData& svd,
                                       અચલ AstScope* અચલ scopep,
                                       અચલ std::string& prettyPrefix, અચલ std::string& cPrefix,
                                       અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep) {
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            અચલ Ast_ગાંઠ_DType* અચલ itemDTypep = itemp->dtypep();
            જો (itemDTypep->vlEnumType().empty()) ચાલુ;
            અચલ std::string prettyName
                = prettyPrefix + "." + Ast_ગાંઠ::vpiName(itemp->shortName());
            અચલ std::string cName = cPrefix + "." + itemp->nameProtect();
            અચલ VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(ઉમેરો_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ subp
                = VN_CAST(itemDTypep->skipRefp(), ગાંઠ_UOr_Struct_DType)) {
                જો (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } નહીંતર જો (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    સ્થિર શૂન્ય addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    અચલ ScopeVarData& svd,
                                                    અચલ AstScope* અચલ scopep,
                                                    અચલ std::string& prettyPrefix,
                                                    અચલ std::string& cPrefix,
                                                    અચલ Ast_ગાંઠ_DType* અચલ dtypep) {
        અચલ Ast_ગાંઠ_DType* અચલ skipDTypep = dtypep->skipRefp();
        જો (અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } નહીંતર જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep
                   = VN_CAST(skipDTypep, ગાંઠ_UOr_Struct_DType)) {
            જો (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    સ્થિર શૂન્ય addPackedMemberVars(std::vector<std::string>& stmts, અચલ ScopeVarData& svd,
                                    અચલ AstScope* અચલ scopep, અચલ std::string& prettyPrefix,
                                    અચલ std::string& cName, અચલ std::string& vlEnumType,
                                    અચલ std::string& entSize,
                                    અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep,
                                    અચલ પૂર્ણાંક parentBitOffset, અચલ તાર્કિક needsIndex) {
        અચલ std::string var_પદાવલિ = "vlSymsp->" + instName(scopep) + "." + cName;
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            અચલ Ast_ગાંઠ_DType* અચલ itemDTypep = itemp->dtypep();
            અચલ std::string prettyName
                = prettyPrefix + "." + Ast_ગાંઠ::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            અચલ પૂર્ણાંક bitOffset = parentBitOffset + itemp->lsb();
            અચલ VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            જો (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_પદાવલિ + "))), true, ";
            } નહીંતર {
                stmt += ", &(" + var_પદાવલિ + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            જો (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            અચલ Ast_ગાંઠ_DType* subDTypep = itemDTypep->skipRefp();
            તાર્કિક isArray = ખોટું;
            જ્યારે (અચલ AstPackArrayDType* અચલ adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = સાચું;
            }
            જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ subp = VN_CAST(subDTypep, ગાંઠ_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    સ્થિર શૂન્ય addPackedAggMemberVars(std::vector<std::string>& stmts, અચલ ScopeVarData& svd,
                                       અચલ AstScope* અચલ scopep,
                                       અચલ std::string& prettyPrefix, અચલ std::string& cName,
                                       અચલ Ast_ગાંઠ_DType* અચલ dtypep) {
        std::string elemCName = cName;
        અચલ Ast_ગાંઠ_DType* elemDTypep = dtypep->skipRefp();
        તાર્કિક isArray = ખોટું;
        જ્યારે (અચલ Ast_ગાંઠ_Array_DType* અચલ adtypep = VN_CAST(elemDTypep, ગાંઠ_Array_DType)) {
            જો (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = સાચું;
        }
        અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep = VN_CAST(elemDTypep, ગાંઠ_UOr_Struct_DType);
        જો (!sdtypep || !sdtypep->packed()) પાછા;
        અચલ std::string var_પદાવલિ = "vlSymsp->" + instName(scopep) + "." + cName;
        અચલ std::string entSize
            = "sizeof(" + var_પદાવલિ + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string ઉમેરો_Forceable_Var_Statement(અચલ ScopeVarData& svd, અચલ AstScope* અચલ scopep,
                                            અચલ AstVar* અચલ varp, અચલ પૂર્ણાંક udim,
                                            અચલ પૂર્ણાંક pdim, અચલ std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        અચલ std::string var_પદાવલિ = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        પ્રતિજ્ઞા(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_પદાવલિ;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_પદાવલિ + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        માટે (અચલ std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            અચલ std::string enable_Signal_ચાવી
                = get_ચાવી_Name(scopep, varp->name() + forceControlSuffix);
            અચલ std::map<અચલ std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_ચાવી);

            જો (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_ચાવી << "'.");
            }

            અચલ ScopeVarData& svd = itpair->second;
            અચલ AstScope* અચલ scopep = svd.m_scopep;
            અચલ AstVar* અચલ varp = svd.m_varp;
            અચલ VarDims dims = dimsFor(svd);
            stmt
                += ઉમેરો_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            જો (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        પાછા stmt;
    }

    સ્થિર std::string get_ચાવી_Name(અચલ AstScope* અચલ scopep, અચલ std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        જો (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        અચલ std::string::size_type dpos = whole.rfind("__DOT__");
        જો (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        અચલ std::string scpSym = scopeSymString(VName::dehash(scpName));
        પાછા scpSym + " " + signal_name;
    }

    તાર્કિક baseSignalIsValid(અચલ AstScope* અચલ scopep, અચલ AstVar* અચલ controlSignalVarp,
                           અચલ std::string& baseSignalName) અચલ {
        અચલ std::string base_Signal_ચાવી = get_ચાવી_Name(scopep, baseSignalName);
        અચલ std::map<અચલ std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_ચાવી);
        જો (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            પાછા ખોટું;
        } નહીંતર {
            અચલ AstVar* અચલ baseSignalVarp = baseSignalIt->second.m_varp;
            જો (m_scopeVars.ગણતરી(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                પાછા ખોટું;
            }
            જો (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                પાછા ખોટું;
            }
        }
        પાછા સાચું;
    }

    તાર્કિક baseSignalIsPublic(અચલ AstScope* અચલ scopep,
                            અચલ std::string& baseSignalName) અચલ {
        અચલ std::string base_Signal_ચાવી = get_ચાવી_Name(scopep, baseSignalName);
        અચલ std::map<અચલ std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_ચાવી);
        જો (baseSignalIt == m_scopeVars.end()) {
            પાછા ખોટું;
        } નહીંતર {
            અચલ AstVar* અચલ baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            જો (!baseSignalVarp->isSigPublic()) પાછા ખોટું;
        }
        પાછા સાચું;
    }

    તાર્કિક forceControlSignalsAreValid(અચલ AstScope* અચલ scopep,
                                     અચલ AstVar* અચલ baseSignalVarp) અચલ {
        constexpr std::array<અચલ અક્ષર*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        પાછા std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, આ](અચલ std::string& forceControlSuffix) {
                અચલ std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                અચલ std::string control_Signal_ચાવી = get_ચાવી_Name(scopep, controlSignalName);
                અચલ std::size_t control_Signal_ગણતરી = m_scopeVars.ગણતરી(control_Signal_ચાવી);
                જો (control_Signal_ગણતરી == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_ચાવી << "'.");
                    પાછા ખોટું;
                }
                જો (control_Signal_ગણતરી > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    પાછા ખોટું;
                }
                પાછા સાચું;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    શૂન્ય varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        જ્યારે (!scp.empty()) {
            અચલ auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            જો ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                અચલ auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                જો (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            જો ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } નહીંતર {
                જો (prd_pos == string::npos) તોડો;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    શૂન્ય addScopeVarEntry(અચલ AstScope* અચલ scopep, અચલ Ast_ગાંઠ_Module* અચલ modp,
                          અચલ AstVar* અચલ varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        જો (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        અચલ std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        અચલ std::string scpName = whole.substr(0, dpos);
        અચલ std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        અચલ std::string varBasePretty = Ast_ગાંઠ::vpiName(VName::dehash(varBase));
        અચલ std::string scpPretty = Ast_ગાંઠ::prettyName(VName::dehash(scpName));
        અચલ std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        જો (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    શૂન્ય varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        માટે (અચલ ScopeModPair& smPair : m_scopes) {
            અચલ AstScope* અચલ scopep = smPair.first;
            અચલ Ast_ગાંઠ_Module* અચલ smodp = smPair.second;
            માટે (અચલ ModVarPair& mvPair : m_modVars) {
                અચલ Ast_ગાંઠ_Module* અચલ modp = mvPair.first;
                અચલ AstVar* અચલ varp = mvPair.second;
                જો (modp != smodp) ચાલુ;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    શૂન્ય collectIfaceRefs(અચલ AstScope* nodep) {
        અચલ AstCell* અચલ cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        જો (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) પાછા;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        અચલ std::string path = Ast_ગાંઠ::vpiName(nodep->name());
        અચલ std::string instName = Ast_ગાંઠ::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        અચલ std::string parentPath = path.substr(0, path.length() - instName.length());

        માટે (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            અચલ std::string refName = Ast_ગાંઠ::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            જો (!VString::startsWith(refName, parentPath)) ચાલુ;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_ગાંઠ::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    શૂન્ય buildIfaceRefTable() {
        જો (m_ifaceRefs.empty()) પાછા;
        અચલ std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](અચલ IfaceRefData& a, અચલ IfaceRefData& b) { પાછા a.m_suffix < b.m_suffix; });
        માટે (અચલ IfaceRefData& ird : m_ifaceRefs) {
            અચલ std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            જો (m_scopeNames.find(scopeSym) == m_scopeNames.end()) ચાલુ;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            અચલ auto parent = m_scopeNames.find(ird.m_parentSym);
            જો (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_શબ્દો_If(ird.m_name, સાચું));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_શબ્દો_If(ird.m_suffix, સાચું));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_શબ્દો_If(ird.m_modportName, સાચું));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        જો (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    શૂન્ય buildVpiHierarchy() {
        માટે (અચલ auto& itpair : m_scopeNames) {
            અચલ std::string symName = itpair.second.m_symName;
            std::string above = symName;
            જો (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            જ્યારે (!above.empty()) {
                અચલ std::string::size_type pos = above.rfind("__");
                જો (pos == std::string::npos) તોડો;
                above.resize(pos);
                જો (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].ધકેલો_back(symName);
                    તોડો;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        જો (v3Global.opt.vpi()) buildVpiHierarchy();

        જો (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            માટે (અચલ auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](અચલ ScopeModPair& a, અચલ ScopeModPair& b) {
                             પાછા a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](અચલ AstCFunc* ap, અચલ AstCFunc* bp) {
                             જો (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 પાછા bp->dpiImportPrototype();
                             }
                             પાછા ap->name() < bp->name();
                         });

        // Output
        જો (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        જો (v3Global.dpi()) {
            emitDpiHdr();
            જો (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->isConstPool()) પાછા;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCellInlineScope* nodep) override {
        અચલ std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        અચલ std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        અચલ પૂર્ણાંક timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ગાંઠ::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    શૂન્ય visit(AstScope* nodep) override {
        જો (VN_IS(m_modp, Class)) પાછા;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        જો (v3Global.opt.vpi() && !nodep->is_ટોચ()) {
            અચલ std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            અચલ પૂર્ણાંક timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_ગાંઠ::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstScopeName* nodep) override {
        જો (nodep->selfRelative()) પાછા;  // Its absolute copy is under the scope
        અચલ std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        અચલ પૂર્ણાંક timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        જો (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } નહીંતર {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    શૂન્ય visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        જો ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        જો (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_અંક(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_અંક(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        જો (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ધકેલો_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    શૂન્ય visit(AstConst*) override {}
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    explicit EmitCSyms(AstNetlist* nodep, તાર્કિક dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

શૂન્ય EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_ટોચ_Include();
    puts("#include \"verilated.h\"\n");
    જો (v3Global.needTraceDumper()) {
        માટે (અચલ string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    જો (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + ટોચ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    માટે (Ast_ગાંઠ_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ગાંઠ_Module);
        જો (VN_IS(nodep, Class)) ચાલુ;  // Class included earlier
        જો (nodep->isConstPool()) ચાલુ;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    જો (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        માટે (અચલ auto& itpair : m_scopeFuncs) {
            અચલ AstCFunc* અચલ funcp = itpair.second.m_cfuncp;
            જો (!funcp->dpiExportImpl()) ચાલુ;
            અચલ std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            અચલ std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        માટે (અચલ std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(ખોટું);  // public:

    puts("// INTERNAL STATE\n");
    puts(ટોચ_Class_Name() + "* const __Vm_modelp;\n");

    જો (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    જો (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    જો (v3Global.hasEvents()) {
        જો (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } નહીંતર {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    જો (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    જો (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    જો (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    જો (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    જો (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::અંક_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    માટે (અચલ ScopeModPair& itpair : m_scopes) {
        અચલ AstScope* અચલ scopep = itpair.first;
        અચલ Ast_ગાંઠ_Module* અચલ modp = itpair.second;
        જો (VN_IS(modp, Class)) ચાલુ;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    જો (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    જો (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        માટે (અચલ auto& itpair : m_scopeNames) {
            અચલ ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    જો (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + ટોચ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    જો (v3Global.hasEvents()) {
        જો (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } નહીંતર {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        જો (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } નહીંતર {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        જો (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } નહીંતર {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    જો (v3Global.needTraceDumper()) {
        જો (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    જો (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

શૂન્ય EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    તાર્કિક needs_New_લીટી = ખોટું;
    માટે (અચલ auto& itpair : m_scopeFuncs) {
        અચલ AstCFunc* અચલ funcp = itpair.second.m_cfuncp;
        જો (!funcp->dpiExportImpl()) ચાલુ;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_લીટી = સાચું;
    }
    જો (needs_New_લીટી) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    માટે (અચલ ScopeModPair& itpair : m_scopes) {
        જો (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    માટે (અચલ std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    જો (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    જો (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        માટે (અચલ auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        જો (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        જો (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

શૂન્ય EmitCSyms::emitVarTables() {
    જો (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) પાછા;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    માટે (અચલ auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        માટે (અચલ std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    જો (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        માટે (અચલ std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    જો (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        માટે (અચલ std::string& row : m_ifaceRefTableRows) {
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

શૂન્ય EmitCSyms::emitScopeHier(TextStmts& stmts, તાર્કિક destroy) {
    જો (!v3Global.opt.vpi()) પાછા;

    જો (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } નહીંતર {
        stmts.add("// Set up scope hierarchy");
    }

    અચલ std::string method = destroy ? "remove" : "add";
    માટે (અચલ auto& itpair : m_scopeNames) {
        જો (itpair.first == "TOP") ચાલુ;
        અચલ ScopeData& sd = itpair.second;
        અચલ std::string& name = sd.m_prettyName;
        અચલ std::string& scopeType = sd.m_type;
        જો (name.find('.') != string::npos) ચાલુ;
        જો (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") ચાલુ;
        અચલ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    માટે (અચલ auto& itpair : m_vpiScopeHierarchy) {
        અચલ std::string fromName = scopeSymString(itpair.first);
        અચલ std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        માટે (અચલ std::string& name : itpair.second) {
            અચલ std::string toName = scopeSymString(name);
            અચલ std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    જો (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t ઢગલો_Size = V3_ઢગલો_ગણતરી::ગણતરી(v3Global.rootp());
        જો (v3Global.opt.debug_ઢગલો_Check()) ઢગલો_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", ઢગલો_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, ઢગલો_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(ઢગલો_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    માટે (અચલ ScopeModPair& itpair : m_scopes) {
        અચલ AstScope* અચલ scopep = itpair.first;
        અચલ Ast_ગાંઠ_Module* અચલ modp = itpair.second;
        જો (modp->is_ટોચ()) ચાલુ;
        અચલ std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_શબ્દો_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    જો (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        જો (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        જો (v3Global.opt.mtasks()) {
            v3Global.rootp()->ટોચ_Modulep()->foreach([&](અચલ AstExecGraph* execGraphp) {
                માટે (અચલ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    અચલ ExecMTask& mt = static_cast<અચલ ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    જો (!v3Global.rootp()->timeunit().isNone()) {
        અચલ std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    જો (!v3Global.rootp()->timeprecision().isNone()) {
        અચલ std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    માટે (અચલ auto& i : m_scopes) {
        અચલ AstScope* અચલ scopep = i.first;
        અચલ Ast_ગાંઠ_Module* અચલ modp = i.second;
        અચલ AstScope* અચલ abovep = scopep->aboveScopep();
        જો (!abovep) ચાલુ;

        અચલ std::string protName = VIdProtect::protect_શબ્દો_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        જો (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } નહીંતર {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    જો (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        માટે (અચલ ScopeModPair& i : m_scopes) {
            અચલ AstScope* અચલ scopep = i.first;
            Ast_ગાંઠ_Module* અચલ modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            અચલ તાર્કિક first = !modp->user1();
            modp->user1(સાચું);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    માટે (અચલ ScopeModPair& i : m_scopes) {
        અચલ AstScope* અચલ scopep = i.first;
        અચલ Ast_ગાંઠ_Module* અચલ modp = i.second;
        જો (!modp->verilatorLib()) ચાલુ;
        અચલ AstVar* hashp = nullptr;
        અચલ AstVar* handlep = nullptr;
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstVar* અચલ varp = VN_CAST(nodep, Var);
            જો (!varp) ચાલુ;
            જો (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            જો (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        અચલ uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        જો (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_ટોચ_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    જો (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        અચલ std::string symClass = symClassName();
        માટે (અચલ auto& itpair : m_scopeNames) {
            અચલ ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_શબ્દો_If(sd.m_prettyName, સાચું));
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
    જો (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, ખોટું);

    જો (v3Global.dpi()) {
        માટે (અચલ std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            માટે (અચલ auto& itpair : m_scopeFuncs) {
                અચલ ScopeFuncData& sfd = itpair.second;
                અચલ AstScopeName* અચલ scopep = sfd.m_scopep;
                અચલ AstCFunc* અચલ funcp = sfd.m_cfuncp;
                અચલ Ast_ગાંઠ_Module* અચલ modp = sfd.m_modp;
                જો (!funcp->dpiExportImpl()) ચાલુ;

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
    જો (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        પૂર્ણાંક tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        જ્યારે (it != m_scopeVars.cend()) {
            અચલ std::string scopeName = it->second.m_scopeName;
            અચલ AstScope* અચલ instScopep = it->second.m_scopep;
            અચલ Ast_ગાંઠ_Module* અચલ modp = it->second.m_modp;
            અચલ std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            માટે (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                અચલ ScopeVarData& svd = it->second;
                અચલ AstScope* અચલ scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                અચલ AstVar* અચલ varp = svd.m_varp;
                અચલ VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                અચલ std::pair<તાર્કિક, std::string> fc = isForceControlSignal(varp);
                જો (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    ચાલુ;
                }

                std::string row;
                અચલ TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                પસંદગી (kind) {
                સ્થિતિ TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    તોડો;
                સ્થિતિ TableEntryKind::FORCEABLE_RESIDUAL: {
                    અચલ std::string bounds = boundsString(dims);
                    residual.emplace_back(ઉમેરો_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    તોડો;
                }
                સ્થિતિ TableEntryKind::PLAIN_RESIDUAL: {
                    અચલ std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        ઉમેરો_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    તોડો;
                }
                મૂળભૂત: v3fatalSrc("Bad case");
                }
                જો (kind == TableEntryKind::TABLE_ROW) ચાલુ;
                જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ગાંઠ_UOr_Struct_DType)) {
                    જો (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } નહીંતર જો (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                જો (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            જો (!rows.empty()) {
                અચલ size_t row_ગણતરી = rows.size();
                std::string ચાવી;
                માટે (અચલ std::string& r : rows) {
                    ચાવી += r;
                    ચાવી += '\0';
                }
                અચલ auto itt = tableByRows.find(ચાવી);
                std::string tableName;
                જો (itt != tableByRows.end()) {
                    tableName = itt->second;
                } નહીંતર {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(ચાવી), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_ગણતરી) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            માટે (અચલ std::string& r : residual) stmts.add(r);
        }
    }

    પાછા stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, સાચું);
    જો (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    જો (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + ટોચ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    જો (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    માટે (અચલ auto& itpair : m_scopeNames) {
        અચલ ScopeData& sd = itpair.second;
        અચલ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    માટે (અચલ ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        અચલ AstScope* અચલ scopep = itpair.first;
        અચલ Ast_ગાંઠ_Module* અચલ modp = itpair.second;
        જો (modp->is_ટોચ()) ચાલુ;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    પાછા stmts;
}

શૂન્ય EmitCSyms::emitSymImp(અચલ AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    અચલ std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            સાચું, "Symbol table implementation internals",
                            [આ]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), સાચું, સાચું, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    અચલ AstScope* અચલ ટોચ_Scopep = netlistp->ટોચ_Scopep()->scopep();
    અચલ std::string ટોચ_Inst = "vlSymsp->" + instName(ટોચ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    જો (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + ટોચ_Inst + ", vlSymsp, namep);\n");
    માટે (અચલ TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + ટોચ_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    જો (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    જો (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    જો (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    જો (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    માટે (અચલ TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + ટોચ_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    જો (v3Global.needTraceDumper()) {
        જો (!optSystemC()) {
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

    જો (v3Global.opt.savable()) {
        માટે (અચલ તાર્કિક& de : {ખોટું, સાચું}) {
            અચલ std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            અચલ std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            અચલ std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            જો (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            માટે (અચલ ScopeModPair& itpair : m_scopes) {
                અચલ AstScope* અચલ scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

શૂન્ય EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(ટોચ_Class_Name() + "__Dpi",
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
    માટે (અચલ AstCFunc* અચલ nodep : m_dpis) {
        માટે (અચલ Ast_ગાંઠ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            જો (અચલ AstVar* અચલ portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    જો (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    પૂર્ણાંક firstExp = 0;
    પૂર્ણાંક firstImp = 0;
    માટે (અચલ AstCFunc* અચલ nodep : m_dpis) {
        જો (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) ચાલુ;

        અચલ std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        જો (nodep->dpiExportDispatcher()) {
            જો (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } નહીંતર {
            જો (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        જો (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } નહીંતર {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, ખોટું) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

શૂન્ય EmitCSyms::emitDpiStruct(અચલ Ast_ગાંઠ_DType* dtypep, std::set<std::string>& namesr) {
    જો (!dtypep->containsUnpackedStruct()) પાછા;
    અચલ AstStructDType* અચલ sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    અચલ std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    જો (namesr.ગણતરી(name)) પાછા;
    // After the structures it contains
    માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    જો (name.empty()) પાછા;  // Declared where used
    જો (namesr.empty()) puts("// DPI TYPES\n");
    namesr.ઉમેરો(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

શૂન્ય EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(ટોચ_Class_Name() + "__Dpi", ખોટું, સાચું,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + ટોચ_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + ટોચ_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + ટોચ_Class_Name() + ".h\"\n");
    puts("\n");

    માટે (અચલ AstCFunc* અચલ nodep : m_dpis) {
        જો (!nodep->dpiExportDispatcher()) ચાલુ;

        અચલ std::string name = nodep->name();
        અચલ std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + ટોચ_Class_Name() + "::" + name + "(");
        std::string comma;
        માટે (અચલ Ast_ગાંઠ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (અચલ AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isIO() && !portp->isFuncReturn()) {
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

શૂન્ય V3EmitC::emitcSyms(તાર્કિક dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
