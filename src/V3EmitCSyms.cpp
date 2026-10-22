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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3ExecGraph.h"
#அந்தர்பாவ "V3LanguageWords.h"
#அந்தர்பாவ "V3StackCount.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cstring>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
நிலையான constexpr auto symClassName = &EmitCUtil::symClassName;
நிலையான constexpr auto உச்சி_Class_Name = &EmitCUtil::உச்சி_Class_Name;

வகுப்பு EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    மாறிலி VNUser1InUse m_inuser1;

    // TYPES
    கட்டமைப்பு ScopeData final {
        மாறிலி Ast_முனை* m_nodep;  // Scope symbol-table entry corresponds to
        மாறிலி std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        மாறிலி std::string m_prettyName;  // Pretty (unprotected) name for display
        மாறிலி std::string m_defName;  // Name registered for this scope
        மாறிலி முழு m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(மாறிலி Ast_முனை* nodep, மாறிலி std::string& symName, மாறிலி std::string& prettyName,
                  மாறிலி std::string& defName, முழு timeunit, மாறிலி std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    கட்டமைப்பு IfaceRefData final {
        மாறிலி AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(மாறிலி AstScope* scopep, மாறிலி std::string& parentSym,
                     மாறிலி std::string& suffix, மாறிலி std::string& name,
                     மாறிலி std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    கட்டமைப்பு ScopeFuncData final {
        மாறிலி AstScopeName* மாறிலி m_scopep;  // Scope the DPI export function is registered under
        மாறிலி AstCFunc* மாறிலி m_cfuncp;  // DPI export function
        மாறிலி Ast_முனை_Module* மாறிலி m_modp;  // Module containing the DPI export function
        ScopeFuncData(மாறிலி AstScopeName* scopep, மாறிலி AstCFunc* funcp, மாறிலி Ast_முனை_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    கட்டமைப்பு ScopeVarData final {
        மாறிலி std::string m_scopeName;  // Full name of the scope containing the variable
        மாறிலி std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        மாறிலி AstVar* மாறிலி m_varp;  // Public variable
        மாறிலி Ast_முனை_Module* மாறிலி m_modp;  // Module containing the variable
        மாறிலி AstScope* மாறிலி m_scopep;  // Scope containing the variable
        ScopeVarData(மாறிலி std::string& scopeName, மாறிலி std::string& varBasePretty,
                     மாறிலி AstVar* varp, மாறிலி Ast_முனை_Module* modp, மாறிலி AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<மாறிலி std::string, ScopeData>;
    using ScopeModPair = std::pair<மாறிலி AstScope*, Ast_முனை_Module*>;
    using ModVarPair = std::pair<மாறிலி Ast_முனை_Module*, மாறிலி AstVar*>;

    // Vars with more dims take the residual per-statement path.
    நிலையான constexpr முழு VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    மாறிலி AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<மாறிலி std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<மாறிலி std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<மாறிலி std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    முழு m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    முழு m_modCoverBins = 0;  // Per-module coverage bin number
    மாறிலி தருக்க m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    வெறுமை emitSymHdr();
    வெறுமை emitSymImpPreamble();
    வெறுமை emitVarTables();
    வெறுமை emitScopeHier(TextStmts& stmts, தருக்க destroy);
    வெறுமை emitSymImp(மாறிலி AstNetlist* netlistp);
    வெறுமை emitDpiHdr();
    வெறுமை emitDpiStruct(மாறிலி Ast_முனை_DType* dtypep, std::set<std::string>& namesr);
    வெறுமை emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    நிலையான வெறுமை nameCheck(Ast_முனை* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        எனில் (nodep->name().empty()) திரும்பு;
        மாறிலி AstCFunc* மாறிலி cfuncp = VN_CAST(nodep, CFunc);
        எனில் (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            மாறிலி std::string rsvd = V3_Language_சொற்கள்::isKeyword(nodep->name());
            எனில் (rsvd != "") {
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

    நிலையான std::string scopeSymString(மாறிலி std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        போது ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        எனில் (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        எனில் (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        போது ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        போது ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        திரும்பு out;
    }

    நிலையான string scopeDecodeIdentifier(மாறிலி std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        போது (i < scpname.length()) {
            எனில் (scpname[i] == '\\') {
                போது (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } இல்லையெனில் {
                போது (i < scpname.length() && scpname[i] != '.') ++i;
                எனில் (i < scpname.length()) pos = i++;
            }
        }

        திரும்பு pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    கட்டமைப்பு VarDims final {
        முழு pdim = 0;  // Packed array dims plus a ranged basic leaf
        முழு udim = 0;
        std::vector<std::pair<முழு, முழு>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<முழு, முழு>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<முழு, முழு>> flatten() மாறிலி {
            std::vector<std::pair<முழு, முழு>> flat(unpackedLR);
            flat.செருகு(flat.end(), packedLR.cbegin(), packedLR.cend());
            திரும்பு flat;
        }
    };
    நிலையான VarDims getVarDims(மாறிலி Ast_முனை_DType* மாறிலி வேர்_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        க்கு (மாறிலி Ast_முனை_DType* dtypep = வேர்_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            எனில் (மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(dtypep, முனை_Array_DType)) {
                எனில் (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->இடது(), adtypep->வலது());
                    ++d.pdim;
                } இல்லையெனில் {
                    d.unpackedLR.emplace_back(adtypep->இடது(), adtypep->வலது());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } இல்லையெனில் {
                எனில் (மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp()) {
                    எனில் (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->இடது(), basicp->வலது());
                        ++d.pdim;
                    }
                }
                நிறுத்து;  // Non-array leaf
            }
        }
        திரும்பு d;
    }

    நிலையான VarDims dimsFor(மாறிலி ScopeVarData& svd) { திரும்பு getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    நிலையான std::string boundsString(மாறிலி VarDims& d) {
        std::string bounds;
        க்கு (மாறிலி std::pair<முழு, முழு>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        திரும்பு bounds;
    }

    நிலையான முழு getUnpackedElements(மாறிலி Ast_முனை_DType* வேர்_Dtypep) {
        முழு elements = 1;
        க்கு (மாறிலி Ast_முனை_DType* dtypep = வேர்_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(dtypep, UnpackArrayDType);
            எனில் (!adtypep) நிறுத்து;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        திரும்பு elements;
    }

    நிலையான தருக்க needsEmittedEntSize(மாறிலி std::string& vlEnumType) {
        திரும்பு vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    நிலையான std::pair<தருக்க, std::string> isForceControlSignal(மாறிலி AstVar* மாறிலி signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        க்கு (மாறிலி std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            மாறிலி std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            மாறிலி தருக்க suffixFound = suffixPos != std::string::npos;
            மாறிலி தருக்க suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            எனில் (!(suffixFound && suffixIsAtEnd)) தொடர்;
            மாறிலி std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            திரும்பு std::pair<தருக்க, std::string>{உண்மை, baseSignalName};
        }
        திரும்பு std::pair<தருக்க, std::string>{பொய், ""};
    }

    நிலையான வெறுமை appendVarProperties(std::string& stmt, மாறிலி std::string& vlEnumType,
                                    மாறிலி std::string& vlEnumDir, மாறிலி முழு udim, மாறிலி முழு pdim,
                                    மாறிலி std::string& bounds, மாறிலி std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        எனில் (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } இல்லையெனில் {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    நிலையான std::string memberVlEnumDir(மாறிலி AstVar* மாறிலி varp,
                                       மாறிலி Ast_முனை_DType* மாறிலி dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/உண்மை);
        மாறிலி Ast_முனை_DType* மாறிலி skipDTypep = dtypep->skipRefp();
        எனில் (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        எனில் (மாறிலி AstBasicDType* மாறிலி basicp = skipDTypep->basicp()) {
            எனில் (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        திரும்பு out + ')';
    }

    // Name of the instance member for the given scope
    நிலையான std::string instName(மாறிலி AstScope* scopep) {
        திரும்பு "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    நிலையான std::string செருகு_Var_Statement(மாறிலி ScopeVarData& svd, மாறிலி AstScope* மாறிலி scopep,
                                          மாறிலி AstVar* மாறிலி varp, மாறிலி முழு udim, மாறிலி முழு pdim,
                                          மாறிலி std::string& bounds) {
        மாறிலி std::string var_கோவை = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        மாறிலி std::string vlEnumType = varp->vlEnumType();
        மாறிலி தருக்க needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        எனில் (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_கோவை;
            stmt += "), false, ";
        } இல்லையெனில் எனில் (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_கோவை;
            stmt += ".c_str())), true, ";
        } இல்லையெனில் {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_கோவை;
            stmt += "))), true, ";
        }

        மாறிலி std::string entSize = needsEntSize
                                        ? "sizeof(" + var_கோவை + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        திரும்பு stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    எண்ணிக்கை வகுப்பு TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(மாறிலி ScopeVarData& svd, மாறிலி AstVar* மாறிலி varp,
                                      மாறிலி AstScope* மாறிலி scopep,
                                      மாறிலி std::string& modClassName, மாறிலி VarDims& dims,
                                      std::string& rowOut) மாறிலி {
        மாறிலி முழு pdim = dims.pdim;
        மாறிலி முழு udim = dims.udim;
        எனில் (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            திரும்பு TableEntryKind::FORCEABLE_RESIDUAL;
        }
        எனில் (udim + pdim > VPI_TABLE_MAX_DIMS) திரும்பு TableEntryKind::PLAIN_RESIDUAL;

        மாறிலி std::string vlEnumType = varp->vlEnumType();
        எனில் (needsEmittedEntSize(vlEnumType))
            திரும்பு TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        எனில் (varp->isParam()) திரும்பு TableEntryKind::PLAIN_RESIDUAL;

        மாறிலி std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        மாறிலி std::string member = varName(varp);
        மாறிலி std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<முழு> dv;
        க்கு (மாறிலி std::pair<முழு, முழு>& lr : dims.flatten()) {
            dv.தள்ளு_back(lr.first);
            dv.தள்ளு_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        க்கு (முழு i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            எனில் (i) row += ", ";
            row += std::to_string(i < static_cast<முழு>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        திரும்பு TableEntryKind::TABLE_ROW;
    }

    நிலையான std::string செருகு_DType_Var_Statement(மாறிலி ScopeVarData& svd,
                                               மாறிலி AstScope* மாறிலி scopep,
                                               மாறிலி std::string& prettyName,
                                               மாறிலி std::string& cName,
                                               மாறிலி Ast_முனை_DType* மாறிலி dtypep, மாறிலி முழு udim,
                                               மாறிலி முழு pdim, மாறிலி std::string& bounds) {
        மாறிலி std::string vlEnumType = dtypep->vlEnumType();
        மாறிலி தருக்க needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        மாறிலி std::string var_கோவை = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        எனில் (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_கோவை;
            stmt += "))), true, ";
        } இல்லையெனில் {
            stmt += ", &(";
            stmt += var_கோவை;
            stmt += "), false, ";
        }
        மாறிலி std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_கோவை + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        திரும்பு stmt;
    }

    நிலையான வெறுமை addUOrStructMemberVars(std::vector<std::string>& stmts, மாறிலி ScopeVarData& svd,
                                       மாறிலி AstScope* மாறிலி scopep,
                                       மாறிலி std::string& prettyPrefix, மாறிலி std::string& cPrefix,
                                       மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep) {
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            மாறிலி Ast_முனை_DType* மாறிலி itemDTypep = itemp->dtypep();
            எனில் (itemDTypep->vlEnumType().empty()) தொடர்;
            மாறிலி std::string prettyName
                = prettyPrefix + "." + Ast_முனை::vpiName(itemp->shortName());
            மாறிலி std::string cName = cPrefix + "." + itemp->nameProtect();
            மாறிலி VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(செருகு_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி subp
                = VN_CAST(itemDTypep->skipRefp(), முனை_UOr_Struct_DType)) {
                எனில் (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } இல்லையெனில் எனில் (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    நிலையான வெறுமை addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    மாறிலி ScopeVarData& svd,
                                                    மாறிலி AstScope* மாறிலி scopep,
                                                    மாறிலி std::string& prettyPrefix,
                                                    மாறிலி std::string& cPrefix,
                                                    மாறிலி Ast_முனை_DType* மாறிலி dtypep) {
        மாறிலி Ast_முனை_DType* மாறிலி skipDTypep = dtypep->skipRefp();
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep
                   = VN_CAST(skipDTypep, முனை_UOr_Struct_DType)) {
            எனில் (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    நிலையான வெறுமை addPackedMemberVars(std::vector<std::string>& stmts, மாறிலி ScopeVarData& svd,
                                    மாறிலி AstScope* மாறிலி scopep, மாறிலி std::string& prettyPrefix,
                                    மாறிலி std::string& cName, மாறிலி std::string& vlEnumType,
                                    மாறிலி std::string& entSize,
                                    மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep,
                                    மாறிலி முழு parentBitOffset, மாறிலி தருக்க needsIndex) {
        மாறிலி std::string var_கோவை = "vlSymsp->" + instName(scopep) + "." + cName;
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            மாறிலி Ast_முனை_DType* மாறிலி itemDTypep = itemp->dtypep();
            மாறிலி std::string prettyName
                = prettyPrefix + "." + Ast_முனை::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            மாறிலி முழு bitOffset = parentBitOffset + itemp->lsb();
            மாறிலி VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            எனில் (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_கோவை + "))), true, ";
            } இல்லையெனில் {
                stmt += ", &(" + var_கோவை + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            எனில் (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            மாறிலி Ast_முனை_DType* subDTypep = itemDTypep->skipRefp();
            தருக்க isArray = பொய்;
            போது (மாறிலி AstPackArrayDType* மாறிலி adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = உண்மை;
            }
            எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி subp = VN_CAST(subDTypep, முனை_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    நிலையான வெறுமை addPackedAggMemberVars(std::vector<std::string>& stmts, மாறிலி ScopeVarData& svd,
                                       மாறிலி AstScope* மாறிலி scopep,
                                       மாறிலி std::string& prettyPrefix, மாறிலி std::string& cName,
                                       மாறிலி Ast_முனை_DType* மாறிலி dtypep) {
        std::string elemCName = cName;
        மாறிலி Ast_முனை_DType* elemDTypep = dtypep->skipRefp();
        தருக்க isArray = பொய்;
        போது (மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(elemDTypep, முனை_Array_DType)) {
            எனில் (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = உண்மை;
        }
        மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep = VN_CAST(elemDTypep, முனை_UOr_Struct_DType);
        எனில் (!sdtypep || !sdtypep->packed()) திரும்பு;
        மாறிலி std::string var_கோவை = "vlSymsp->" + instName(scopep) + "." + cName;
        மாறிலி std::string entSize
            = "sizeof(" + var_கோவை + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string செருகு_Forceable_Var_Statement(மாறிலி ScopeVarData& svd, மாறிலி AstScope* மாறிலி scopep,
                                            மாறிலி AstVar* மாறிலி varp, மாறிலி முழு udim,
                                            மாறிலி முழு pdim, மாறிலி std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        மாறிலி std::string var_கோவை = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        ப்ரதிஜ்ஞா(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_கோவை;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_கோவை + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        க்கு (மாறிலி std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            மாறிலி std::string enable_Signal_சாவி
                = get_சாவி_Name(scopep, varp->name() + forceControlSuffix);
            மாறிலி std::map<மாறிலி std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_சாவி);

            எனில் (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_சாவி << "'.");
            }

            மாறிலி ScopeVarData& svd = itpair->second;
            மாறிலி AstScope* மாறிலி scopep = svd.m_scopep;
            மாறிலி AstVar* மாறிலி varp = svd.m_varp;
            மாறிலி VarDims dims = dimsFor(svd);
            stmt
                += செருகு_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            எனில் (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        திரும்பு stmt;
    }

    நிலையான std::string get_சாவி_Name(மாறிலி AstScope* மாறிலி scopep, மாறிலி std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        எனில் (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        மாறிலி std::string::size_type dpos = whole.rfind("__DOT__");
        எனில் (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        மாறிலி std::string scpSym = scopeSymString(VName::dehash(scpName));
        திரும்பு scpSym + " " + signal_name;
    }

    தருக்க baseSignalIsValid(மாறிலி AstScope* மாறிலி scopep, மாறிலி AstVar* மாறிலி controlSignalVarp,
                           மாறிலி std::string& baseSignalName) மாறிலி {
        மாறிலி std::string base_Signal_சாவி = get_சாவி_Name(scopep, baseSignalName);
        மாறிலி std::map<மாறிலி std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_சாவி);
        எனில் (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            திரும்பு பொய்;
        } இல்லையெனில் {
            மாறிலி AstVar* மாறிலி baseSignalVarp = baseSignalIt->second.m_varp;
            எனில் (m_scopeVars.மொத்தம்(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                திரும்பு பொய்;
            }
            எனில் (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                திரும்பு பொய்;
            }
        }
        திரும்பு உண்மை;
    }

    தருக்க baseSignalIsPublic(மாறிலி AstScope* மாறிலி scopep,
                            மாறிலி std::string& baseSignalName) மாறிலி {
        மாறிலி std::string base_Signal_சாவி = get_சாவி_Name(scopep, baseSignalName);
        மாறிலி std::map<மாறிலி std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_சாவி);
        எனில் (baseSignalIt == m_scopeVars.end()) {
            திரும்பு பொய்;
        } இல்லையெனில் {
            மாறிலி AstVar* மாறிலி baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            எனில் (!baseSignalVarp->isSigPublic()) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }

    தருக்க forceControlSignalsAreValid(மாறிலி AstScope* மாறிலி scopep,
                                     மாறிலி AstVar* மாறிலி baseSignalVarp) மாறிலி {
        constexpr std::array<மாறிலி எழுத்து*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        திரும்பு std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, இது](மாறிலி std::string& forceControlSuffix) {
                மாறிலி std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                மாறிலி std::string control_Signal_சாவி = get_சாவி_Name(scopep, controlSignalName);
                மாறிலி std::size_t control_Signal_மொத்தம் = m_scopeVars.மொத்தம்(control_Signal_சாவி);
                எனில் (control_Signal_மொத்தம் == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_சாவி << "'.");
                    திரும்பு பொய்;
                }
                எனில் (control_Signal_மொத்தம் > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    திரும்பு பொய்;
                }
                திரும்பு உண்மை;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    வெறுமை varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        போது (!scp.empty()) {
            மாறிலி auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            எனில் ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                மாறிலி auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                எனில் (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            எனில் ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } இல்லையெனில் {
                எனில் (prd_pos == string::npos) நிறுத்து;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    வெறுமை addScopeVarEntry(மாறிலி AstScope* மாறிலி scopep, மாறிலி Ast_முனை_Module* மாறிலி modp,
                          மாறிலி AstVar* மாறிலி varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        எனில் (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        மாறிலி std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        மாறிலி std::string scpName = whole.substr(0, dpos);
        மாறிலி std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        மாறிலி std::string varBasePretty = Ast_முனை::vpiName(VName::dehash(varBase));
        மாறிலி std::string scpPretty = Ast_முனை::prettyName(VName::dehash(scpName));
        மாறிலி std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        எனில் (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    வெறுமை varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        க்கு (மாறிலி ScopeModPair& smPair : m_scopes) {
            மாறிலி AstScope* மாறிலி scopep = smPair.first;
            மாறிலி Ast_முனை_Module* மாறிலி smodp = smPair.second;
            க்கு (மாறிலி ModVarPair& mvPair : m_modVars) {
                மாறிலி Ast_முனை_Module* மாறிலி modp = mvPair.first;
                மாறிலி AstVar* மாறிலி varp = mvPair.second;
                எனில் (modp != smodp) தொடர்;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    வெறுமை collectIfaceRefs(மாறிலி AstScope* nodep) {
        மாறிலி AstCell* மாறிலி cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        எனில் (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) திரும்பு;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        மாறிலி std::string path = Ast_முனை::vpiName(nodep->name());
        மாறிலி std::string instName = Ast_முனை::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        மாறிலி std::string parentPath = path.substr(0, path.length() - instName.length());

        க்கு (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            மாறிலி std::string refName = Ast_முனை::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            எனில் (!VString::startsWith(refName, parentPath)) தொடர்;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_முனை::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    வெறுமை buildIfaceRefTable() {
        எனில் (m_ifaceRefs.empty()) திரும்பு;
        மாறிலி std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](மாறிலி IfaceRefData& a, மாறிலி IfaceRefData& b) { திரும்பு a.m_suffix < b.m_suffix; });
        க்கு (மாறிலி IfaceRefData& ird : m_ifaceRefs) {
            மாறிலி std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            எனில் (m_scopeNames.find(scopeSym) == m_scopeNames.end()) தொடர்;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            மாறிலி auto parent = m_scopeNames.find(ird.m_parentSym);
            எனில் (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_சொற்கள்_If(ird.m_name, உண்மை));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_சொற்கள்_If(ird.m_suffix, உண்மை));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_சொற்கள்_If(ird.m_modportName, உண்மை));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        எனில் (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    வெறுமை buildVpiHierarchy() {
        க்கு (மாறிலி auto& itpair : m_scopeNames) {
            மாறிலி std::string symName = itpair.second.m_symName;
            std::string above = symName;
            எனில் (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            போது (!above.empty()) {
                மாறிலி std::string::size_type pos = above.rfind("__");
                எனில் (pos == std::string::npos) நிறுத்து;
                above.resize(pos);
                எனில் (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].தள்ளு_back(symName);
                    நிறுத்து;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        எனில் (v3Global.opt.vpi()) buildVpiHierarchy();

        எனில் (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            க்கு (மாறிலி auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](மாறிலி ScopeModPair& a, மாறிலி ScopeModPair& b) {
                             திரும்பு a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](மாறிலி AstCFunc* ap, மாறிலி AstCFunc* bp) {
                             எனில் (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 திரும்பு bp->dpiImportPrototype();
                             }
                             திரும்பு ap->name() < bp->name();
                         });

        // Output
        எனில் (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        எனில் (v3Global.dpi()) {
            emitDpiHdr();
            எனில் (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->isConstPool()) திரும்பு;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCellInlineScope* nodep) override {
        மாறிலி std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        மாறிலி std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        மாறிலி முழு timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_முனை::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    வெறுமை visit(AstScope* nodep) override {
        எனில் (VN_IS(m_modp, Class)) திரும்பு;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        எனில் (v3Global.opt.vpi() && !nodep->is_உச்சி()) {
            மாறிலி std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            மாறிலி முழு timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_முனை::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstScopeName* nodep) override {
        எனில் (nodep->selfRelative()) திரும்பு;  // Its absolute copy is under the scope
        மாறிலி std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        மாறிலி முழு timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        எனில் (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } இல்லையெனில் {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    வெறுமை visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        எனில் ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    வெறுமை visit(Ast_முனை_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        எனில் (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_இலக்கம்(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_இலக்கம்(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        எனில் (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.தள்ளு_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    வெறுமை visit(AstConst*) override {}
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    explicit EmitCSyms(AstNetlist* nodep, தருக்க dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

வெறுமை EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_உச்சி_Include();
    puts("#include \"verilated.h\"\n");
    எனில் (v3Global.needTraceDumper()) {
        க்கு (மாறிலி string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    எனில் (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + உச்சி_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    க்கு (Ast_முனை_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), முனை_Module);
        எனில் (VN_IS(nodep, Class)) தொடர்;  // Class included earlier
        எனில் (nodep->isConstPool()) தொடர்;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    எனில் (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        க்கு (மாறிலி auto& itpair : m_scopeFuncs) {
            மாறிலி AstCFunc* மாறிலி funcp = itpair.second.m_cfuncp;
            எனில் (!funcp->dpiExportImpl()) தொடர்;
            மாறிலி std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            மாறிலி std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        க்கு (மாறிலி std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(பொய்);  // public:

    puts("// INTERNAL STATE\n");
    puts(உச்சி_Class_Name() + "* const __Vm_modelp;\n");

    எனில் (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    எனில் (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    எனில் (v3Global.hasEvents()) {
        எனில் (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } இல்லையெனில் {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    எனில் (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    எனில் (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    எனில் (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    எனில் (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    எனில் (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::இலக்கம்_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    க்கு (மாறிலி ScopeModPair& itpair : m_scopes) {
        மாறிலி AstScope* மாறிலி scopep = itpair.first;
        மாறிலி Ast_முனை_Module* மாறிலி modp = itpair.second;
        எனில் (VN_IS(modp, Class)) தொடர்;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    எனில் (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    எனில் (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        க்கு (மாறிலி auto& itpair : m_scopeNames) {
            மாறிலி ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    எனில் (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + உச்சி_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    எனில் (v3Global.hasEvents()) {
        எனில் (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } இல்லையெனில் {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        எனில் (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } இல்லையெனில் {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        எனில் (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } இல்லையெனில் {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    எனில் (v3Global.needTraceDumper()) {
        எனில் (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    எனில் (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

வெறுமை EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    தருக்க needs_New_வரி = பொய்;
    க்கு (மாறிலி auto& itpair : m_scopeFuncs) {
        மாறிலி AstCFunc* மாறிலி funcp = itpair.second.m_cfuncp;
        எனில் (!funcp->dpiExportImpl()) தொடர்;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_வரி = உண்மை;
    }
    எனில் (needs_New_வரி) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    க்கு (மாறிலி ScopeModPair& itpair : m_scopes) {
        எனில் (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    க்கு (மாறிலி std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    எனில் (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    எனில் (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        க்கு (மாறிலி auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        எனில் (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        எனில் (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

வெறுமை EmitCSyms::emitVarTables() {
    எனில் (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) திரும்பு;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    க்கு (மாறிலி auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        க்கு (மாறிலி std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    எனில் (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        க்கு (மாறிலி std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    எனில் (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        க்கு (மாறிலி std::string& row : m_ifaceRefTableRows) {
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

வெறுமை EmitCSyms::emitScopeHier(TextStmts& stmts, தருக்க destroy) {
    எனில் (!v3Global.opt.vpi()) திரும்பு;

    எனில் (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } இல்லையெனில் {
        stmts.add("// Set up scope hierarchy");
    }

    மாறிலி std::string method = destroy ? "remove" : "add";
    க்கு (மாறிலி auto& itpair : m_scopeNames) {
        எனில் (itpair.first == "TOP") தொடர்;
        மாறிலி ScopeData& sd = itpair.second;
        மாறிலி std::string& name = sd.m_prettyName;
        மாறிலி std::string& scopeType = sd.m_type;
        எனில் (name.find('.') != string::npos) தொடர்;
        எனில் (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") தொடர்;
        மாறிலி std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    க்கு (மாறிலி auto& itpair : m_vpiScopeHierarchy) {
        மாறிலி std::string fromName = scopeSymString(itpair.first);
        மாறிலி std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        க்கு (மாறிலி std::string& name : itpair.second) {
            மாறிலி std::string toName = scopeSymString(name);
            மாறிலி std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    எனில் (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t அடுக்கு_Size = V3_அடுக்கு_மொத்தம்::மொத்தம்(v3Global.rootp());
        எனில் (v3Global.opt.debug_அடுக்கு_Check()) அடுக்கு_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", அடுக்கு_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, அடுக்கு_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(அடுக்கு_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    க்கு (மாறிலி ScopeModPair& itpair : m_scopes) {
        மாறிலி AstScope* மாறிலி scopep = itpair.first;
        மாறிலி Ast_முனை_Module* மாறிலி modp = itpair.second;
        எனில் (modp->is_உச்சி()) தொடர்;
        மாறிலி std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_சொற்கள்_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    எனில் (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        எனில் (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        எனில் (v3Global.opt.mtasks()) {
            v3Global.rootp()->உச்சி_Modulep()->foreach([&](மாறிலி AstExecGraph* execGraphp) {
                க்கு (மாறிலி V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    மாறிலி ExecMTask& mt = static_cast<மாறிலி ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    எனில் (!v3Global.rootp()->timeunit().isNone()) {
        மாறிலி std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    எனில் (!v3Global.rootp()->timeprecision().isNone()) {
        மாறிலி std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    க்கு (மாறிலி auto& i : m_scopes) {
        மாறிலி AstScope* மாறிலி scopep = i.first;
        மாறிலி Ast_முனை_Module* மாறிலி modp = i.second;
        மாறிலி AstScope* மாறிலி abovep = scopep->aboveScopep();
        எனில் (!abovep) தொடர்;

        மாறிலி std::string protName = VIdProtect::protect_சொற்கள்_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        எனில் (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } இல்லையெனில் {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    எனில் (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        க்கு (மாறிலி ScopeModPair& i : m_scopes) {
            மாறிலி AstScope* மாறிலி scopep = i.first;
            Ast_முனை_Module* மாறிலி modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            மாறிலி தருக்க first = !modp->user1();
            modp->user1(உண்மை);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    க்கு (மாறிலி ScopeModPair& i : m_scopes) {
        மாறிலி AstScope* மாறிலி scopep = i.first;
        மாறிலி Ast_முனை_Module* மாறிலி modp = i.second;
        எனில் (!modp->verilatorLib()) தொடர்;
        மாறிலி AstVar* hashp = nullptr;
        மாறிலி AstVar* handlep = nullptr;
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var);
            எனில் (!varp) தொடர்;
            எனில் (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            எனில் (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        மாறிலி uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        எனில் (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_உச்சி_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    எனில் (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        மாறிலி std::string symClass = symClassName();
        க்கு (மாறிலி auto& itpair : m_scopeNames) {
            மாறிலி ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_சொற்கள்_If(sd.m_prettyName, உண்மை));
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
    எனில் (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, பொய்);

    எனில் (v3Global.dpi()) {
        க்கு (மாறிலி std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            க்கு (மாறிலி auto& itpair : m_scopeFuncs) {
                மாறிலி ScopeFuncData& sfd = itpair.second;
                மாறிலி AstScopeName* மாறிலி scopep = sfd.m_scopep;
                மாறிலி AstCFunc* மாறிலி funcp = sfd.m_cfuncp;
                மாறிலி Ast_முனை_Module* மாறிலி modp = sfd.m_modp;
                எனில் (!funcp->dpiExportImpl()) தொடர்;

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
    எனில் (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        முழு tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        போது (it != m_scopeVars.cend()) {
            மாறிலி std::string scopeName = it->second.m_scopeName;
            மாறிலி AstScope* மாறிலி instScopep = it->second.m_scopep;
            மாறிலி Ast_முனை_Module* மாறிலி modp = it->second.m_modp;
            மாறிலி std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            க்கு (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                மாறிலி ScopeVarData& svd = it->second;
                மாறிலி AstScope* மாறிலி scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                மாறிலி AstVar* மாறிலி varp = svd.m_varp;
                மாறிலி VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                மாறிலி std::pair<தருக்க, std::string> fc = isForceControlSignal(varp);
                எனில் (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    தொடர்;
                }

                std::string row;
                மாறிலி TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                தேர்வு (kind) {
                நிலை TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    நிறுத்து;
                நிலை TableEntryKind::FORCEABLE_RESIDUAL: {
                    மாறிலி std::string bounds = boundsString(dims);
                    residual.emplace_back(செருகு_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    நிறுத்து;
                }
                நிலை TableEntryKind::PLAIN_RESIDUAL: {
                    மாறிலி std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        செருகு_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    நிறுத்து;
                }
                இயல்பு: v3fatalSrc("Bad case");
                }
                எனில் (kind == TableEntryKind::TABLE_ROW) தொடர்;
                எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), முனை_UOr_Struct_DType)) {
                    எனில் (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } இல்லையெனில் எனில் (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                எனில் (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            எனில் (!rows.empty()) {
                மாறிலி size_t row_மொத்தம் = rows.size();
                std::string சாவி;
                க்கு (மாறிலி std::string& r : rows) {
                    சாவி += r;
                    சாவி += '\0';
                }
                மாறிலி auto itt = tableByRows.find(சாவி);
                std::string tableName;
                எனில் (itt != tableByRows.end()) {
                    tableName = itt->second;
                } இல்லையெனில் {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(சாவி), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_மொத்தம்) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            க்கு (மாறிலி std::string& r : residual) stmts.add(r);
        }
    }

    திரும்பு stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, உண்மை);
    எனில் (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    எனில் (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + உச்சி_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    எனில் (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    க்கு (மாறிலி auto& itpair : m_scopeNames) {
        மாறிலி ScopeData& sd = itpair.second;
        மாறிலி std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    க்கு (மாறிலி ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        மாறிலி AstScope* மாறிலி scopep = itpair.first;
        மாறிலி Ast_முனை_Module* மாறிலி modp = itpair.second;
        எனில் (modp->is_உச்சி()) தொடர்;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    திரும்பு stmts;
}

வெறுமை EmitCSyms::emitSymImp(மாறிலி AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    மாறிலி std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            உண்மை, "Symbol table implementation internals",
                            [இது]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), உண்மை, உண்மை, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    மாறிலி AstScope* மாறிலி உச்சி_Scopep = netlistp->உச்சி_Scopep()->scopep();
    மாறிலி std::string உச்சி_Inst = "vlSymsp->" + instName(உச்சி_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    எனில் (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + உச்சி_Inst + ", vlSymsp, namep);\n");
    க்கு (மாறிலி TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + உச்சி_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    எனில் (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    எனில் (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    எனில் (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    எனில் (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    க்கு (மாறிலி TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + உச்சி_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    எனில் (v3Global.needTraceDumper()) {
        எனில் (!optSystemC()) {
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

    எனில் (v3Global.opt.savable()) {
        க்கு (மாறிலி தருக்க& de : {பொய், உண்மை}) {
            மாறிலி std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            மாறிலி std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            மாறிலி std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            எனில் (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            க்கு (மாறிலி ScopeModPair& itpair : m_scopes) {
                மாறிலி AstScope* மாறிலி scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

வெறுமை EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(உச்சி_Class_Name() + "__Dpi",
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
    க்கு (மாறிலி AstCFunc* மாறிலி nodep : m_dpis) {
        க்கு (மாறிலி Ast_முனை* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    எனில் (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    முழு firstExp = 0;
    முழு firstImp = 0;
    க்கு (மாறிலி AstCFunc* மாறிலி nodep : m_dpis) {
        எனில் (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) தொடர்;

        மாறிலி std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        எனில் (nodep->dpiExportDispatcher()) {
            எனில் (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } இல்லையெனில் {
            எனில் (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        எனில் (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } இல்லையெனில் {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, பொய்) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

வெறுமை EmitCSyms::emitDpiStruct(மாறிலி Ast_முனை_DType* dtypep, std::set<std::string>& namesr) {
    எனில் (!dtypep->containsUnpackedStruct()) திரும்பு;
    மாறிலி AstStructDType* மாறிலி sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    மாறிலி std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    எனில் (namesr.மொத்தம்(name)) திரும்பு;
    // After the structures it contains
    க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    எனில் (name.empty()) திரும்பு;  // Declared where used
    எனில் (namesr.empty()) puts("// DPI TYPES\n");
    namesr.செருகு(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

வெறுமை EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(உச்சி_Class_Name() + "__Dpi", பொய், உண்மை,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + உச்சி_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + உச்சி_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + உச்சி_Class_Name() + ".h\"\n");
    puts("\n");

    க்கு (மாறிலி AstCFunc* மாறிலி nodep : m_dpis) {
        எனில் (!nodep->dpiExportDispatcher()) தொடர்;

        மாறிலி std::string name = nodep->name();
        மாறிலி std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + உச்சி_Class_Name() + "::" + name + "(");
        std::string comma;
        க்கு (மாறிலி Ast_முனை* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isIO() && !portp->isFuncReturn()) {
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

வெறுமை V3EmitC::emitcSyms(தருக்க dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
