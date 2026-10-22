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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitC.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3LanguageWords.h"
#ضمن "V3StackCount.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <cstring>
#ضمن <map>
#ضمن <set>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
ساكن constexpr auto symClassName = &EmitCUtil::symClassName;
ساكن constexpr auto لووتکە_Class_Name = &EmitCUtil::لووتکە_Class_Name;

صنف EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ثابت VNUser1InUse m_inuser1;

    // TYPES
    بنية ScopeData final {
        ثابت Ast_گرێ* m_nodep;  // Scope symbol-table entry corresponds to
        ثابت std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ثابت std::string m_prettyName;  // Pretty (unprotected) name for display
        ثابت std::string m_defName;  // Name registered for this scope
        ثابت صحيح m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(ثابت Ast_گرێ* nodep, ثابت std::string& symName, ثابت std::string& prettyName,
                  ثابت std::string& defName, صحيح timeunit, ثابت std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    بنية IfaceRefData final {
        ثابت AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(ثابت AstScope* scopep, ثابت std::string& parentSym,
                     ثابت std::string& suffix, ثابت std::string& name,
                     ثابت std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    بنية ScopeFuncData final {
        ثابت AstScopeName* ثابت m_scopep;  // Scope the DPI export function is registered under
        ثابت AstCFunc* ثابت m_cfuncp;  // DPI export function
        ثابت Ast_گرێ_Module* ثابت m_modp;  // Module containing the DPI export function
        ScopeFuncData(ثابت AstScopeName* scopep, ثابت AstCFunc* funcp, ثابت Ast_گرێ_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    بنية ScopeVarData final {
        ثابت std::string m_scopeName;  // Full name of the scope containing the variable
        ثابت std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        ثابت AstVar* ثابت m_varp;  // Public variable
        ثابت Ast_گرێ_Module* ثابت m_modp;  // Module containing the variable
        ثابت AstScope* ثابت m_scopep;  // Scope containing the variable
        ScopeVarData(ثابت std::string& scopeName, ثابت std::string& varBasePretty,
                     ثابت AstVar* varp, ثابت Ast_گرێ_Module* modp, ثابت AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<ثابت std::string, ScopeData>;
    using ScopeModPair = std::pair<ثابت AstScope*, Ast_گرێ_Module*>;
    using ModVarPair = std::pair<ثابت Ast_گرێ_Module*, ثابت AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ساكن constexpr صحيح VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_گرێ_Module* m_modp = nullptr;  // Current module
    ثابت AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ثابت std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ثابت std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ثابت std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    صحيح m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    صحيح m_modCoverBins = 0;  // Per-module coverage bin number
    ثابت منطقي m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    فراغ emitSymHdr();
    فراغ emitSymImpPreamble();
    فراغ emitVarTables();
    فراغ emitScopeHier(TextStmts& stmts, منطقي destroy);
    فراغ emitSymImp(ثابت AstNetlist* netlistp);
    فراغ emitDpiHdr();
    فراغ emitDpiStruct(ثابت Ast_گرێ_DType* dtypep, std::set<std::string>& namesr);
    فراغ emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    ساكن فراغ nameCheck(Ast_گرێ* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        إذا (nodep->name().empty()) أرجع;
        ثابت AstCFunc* ثابت cfuncp = VN_CAST(nodep, CFunc);
        إذا (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ثابت std::string rsvd = V3_Language_وشەکان::isKeyword(nodep->name());
            إذا (rsvd != "") {
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

    ساكن std::string scopeSymString(ثابت std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        طالما ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        إذا (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        إذا (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        طالما ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        طالما ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        أرجع out;
    }

    ساكن string scopeDecodeIdentifier(ثابت std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        طالما (i < scpname.length()) {
            إذا (scpname[i] == '\\') {
                طالما (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } وإلا {
                طالما (i < scpname.length() && scpname[i] != '.') ++i;
                إذا (i < scpname.length()) pos = i++;
            }
        }

        أرجع pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    بنية VarDims final {
        صحيح pdim = 0;  // Packed array dims plus a ranged basic leaf
        صحيح udim = 0;
        std::vector<std::pair<صحيح, صحيح>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<صحيح, صحيح>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<صحيح, صحيح>> flatten() ثابت {
            std::vector<std::pair<صحيح, صحيح>> flat(unpackedLR);
            flat.زیادکردن(flat.end(), packedLR.cbegin(), packedLR.cend());
            أرجع flat;
        }
    };
    ساكن VarDims getVarDims(ثابت Ast_گرێ_DType* ثابت ڕەگ_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        لكل (ثابت Ast_گرێ_DType* dtypep = ڕەگ_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            إذا (ثابت Ast_گرێ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرێ_Array_DType)) {
                إذا (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->چەپ(), adtypep->ڕاست());
                    ++d.pdim;
                } وإلا {
                    d.unpackedLR.emplace_back(adtypep->چەپ(), adtypep->ڕاست());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } وإلا {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->چەپ(), basicp->ڕاست());
                        ++d.pdim;
                    }
                }
                اكسر;  // Non-array leaf
            }
        }
        أرجع d;
    }

    ساكن VarDims dimsFor(ثابت ScopeVarData& svd) { أرجع getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ساكن std::string boundsString(ثابت VarDims& d) {
        std::string bounds;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        أرجع bounds;
    }

    ساكن صحيح getUnpackedElements(ثابت Ast_گرێ_DType* ڕەگ_Dtypep) {
        صحيح elements = 1;
        لكل (ثابت Ast_گرێ_DType* dtypep = ڕەگ_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType);
            إذا (!adtypep) اكسر;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        أرجع elements;
    }

    ساكن منطقي needsEmittedEntSize(ثابت std::string& vlEnumType) {
        أرجع vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ساكن std::pair<منطقي, std::string> isForceControlSignal(ثابت AstVar* ثابت signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ثابت std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            ثابت منطقي suffixFound = suffixPos != std::string::npos;
            ثابت منطقي suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            إذا (!(suffixFound && suffixIsAtEnd)) استمر;
            ثابت std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            أرجع std::pair<منطقي, std::string>{true, baseSignalName};
        }
        أرجع std::pair<منطقي, std::string>{خطأ, ""};
    }

    ساكن فراغ appendVarProperties(std::string& stmt, ثابت std::string& vlEnumType,
                                    ثابت std::string& vlEnumDir, ثابت صحيح udim, ثابت صحيح pdim,
                                    ثابت std::string& bounds, ثابت std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        إذا (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } وإلا {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ساكن std::string memberVlEnumDir(ثابت AstVar* ثابت varp,
                                       ثابت Ast_گرێ_DType* ثابت dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/true);
        ثابت Ast_گرێ_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        إذا (ثابت AstBasicDType* ثابت basicp = skipDTypep->basicp()) {
            إذا (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        أرجع out + ')';
    }

    // Name of the instance member for the given scope
    ساكن std::string instName(ثابت AstScope* scopep) {
        أرجع "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ساكن std::string زیادکردن_Var_Statement(ثابت ScopeVarData& svd, ثابت AstScope* ثابت scopep,
                                          ثابت AstVar* ثابت varp, ثابت صحيح udim, ثابت صحيح pdim,
                                          ثابت std::string& bounds) {
        ثابت std::string var_دەربڕین = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ثابت std::string vlEnumType = varp->vlEnumType();
        ثابت منطقي needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        إذا (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_دەربڕین;
            stmt += "), false, ";
        } وإلا إذا (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_دەربڕین;
            stmt += ".c_str())), true, ";
        } وإلا {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_دەربڕین;
            stmt += "))), true, ";
        }

        ثابت std::string entSize = needsEntSize
                                        ? "sizeof(" + var_دەربڕین + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        أرجع stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    تعداد صنف TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(ثابت ScopeVarData& svd, ثابت AstVar* ثابت varp,
                                      ثابت AstScope* ثابت scopep,
                                      ثابت std::string& modClassName, ثابت VarDims& dims,
                                      std::string& rowOut) ثابت {
        ثابت صحيح pdim = dims.pdim;
        ثابت صحيح udim = dims.udim;
        إذا (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            أرجع TableEntryKind::FORCEABLE_RESIDUAL;
        }
        إذا (udim + pdim > VPI_TABLE_MAX_DIMS) أرجع TableEntryKind::PLAIN_RESIDUAL;

        ثابت std::string vlEnumType = varp->vlEnumType();
        إذا (needsEmittedEntSize(vlEnumType))
            أرجع TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        إذا (varp->isParam()) أرجع TableEntryKind::PLAIN_RESIDUAL;

        ثابت std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ثابت std::string member = varName(varp);
        ثابت std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<صحيح> dv;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : dims.flatten()) {
            dv.پاڵنان_back(lr.first);
            dv.پاڵنان_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        لكل (صحيح i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            إذا (i) row += ", ";
            row += std::to_string(i < static_cast<صحيح>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        أرجع TableEntryKind::TABLE_ROW;
    }

    ساكن std::string زیادکردن_DType_Var_Statement(ثابت ScopeVarData& svd,
                                               ثابت AstScope* ثابت scopep,
                                               ثابت std::string& prettyName,
                                               ثابت std::string& cName,
                                               ثابت Ast_گرێ_DType* ثابت dtypep, ثابت صحيح udim,
                                               ثابت صحيح pdim, ثابت std::string& bounds) {
        ثابت std::string vlEnumType = dtypep->vlEnumType();
        ثابت منطقي needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        ثابت std::string var_دەربڕین = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        إذا (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_دەربڕین;
            stmt += "))), true, ";
        } وإلا {
            stmt += ", &(";
            stmt += var_دەربڕین;
            stmt += "), false, ";
        }
        ثابت std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_دەربڕین + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        أرجع stmt;
    }

    ساكن فراغ addUOrStructMemberVars(std::vector<std::string>& stmts, ثابت ScopeVarData& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& prettyPrefix, ثابت std::string& cPrefix,
                                       ثابت Ast_گرێ_UOr_Struct_DType* ثابت sdtypep) {
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_گرێ_DType* ثابت itemDTypep = itemp->dtypep();
            إذا (itemDTypep->vlEnumType().empty()) استمر;
            ثابت std::string prettyName
                = prettyPrefix + "." + Ast_گرێ::vpiName(itemp->shortName());
            ثابت std::string cName = cPrefix + "." + itemp->nameProtect();
            ثابت VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(زیادکردن_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            إذا (ثابت Ast_گرێ_UOr_Struct_DType* ثابت subp
                = VN_CAST(itemDTypep->skipRefp(), گرێ_UOr_Struct_DType)) {
                إذا (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } وإلا إذا (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    ساكن فراغ addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    ثابت ScopeVarData& svd,
                                                    ثابت AstScope* ثابت scopep,
                                                    ثابت std::string& prettyPrefix,
                                                    ثابت std::string& cPrefix,
                                                    ثابت Ast_گرێ_DType* ثابت dtypep) {
        ثابت Ast_گرێ_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } وإلا إذا (ثابت Ast_گرێ_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(skipDTypep, گرێ_UOr_Struct_DType)) {
            إذا (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ساكن فراغ addPackedMemberVars(std::vector<std::string>& stmts, ثابت ScopeVarData& svd,
                                    ثابت AstScope* ثابت scopep, ثابت std::string& prettyPrefix,
                                    ثابت std::string& cName, ثابت std::string& vlEnumType,
                                    ثابت std::string& entSize,
                                    ثابت Ast_گرێ_UOr_Struct_DType* ثابت sdtypep,
                                    ثابت صحيح parentBitOffset, ثابت منطقي needsIndex) {
        ثابت std::string var_دەربڕین = "vlSymsp->" + instName(scopep) + "." + cName;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_گرێ_DType* ثابت itemDTypep = itemp->dtypep();
            ثابت std::string prettyName
                = prettyPrefix + "." + Ast_گرێ::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ثابت صحيح bitOffset = parentBitOffset + itemp->lsb();
            ثابت VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            إذا (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_دەربڕین + "))), true, ";
            } وإلا {
                stmt += ", &(" + var_دەربڕین + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            إذا (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ثابت Ast_گرێ_DType* subDTypep = itemDTypep->skipRefp();
            منطقي isArray = خطأ;
            طالما (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = true;
            }
            إذا (ثابت Ast_گرێ_UOr_Struct_DType* ثابت subp = VN_CAST(subDTypep, گرێ_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ساكن فراغ addPackedAggMemberVars(std::vector<std::string>& stmts, ثابت ScopeVarData& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& prettyPrefix, ثابت std::string& cName,
                                       ثابت Ast_گرێ_DType* ثابت dtypep) {
        std::string elemCName = cName;
        ثابت Ast_گرێ_DType* elemDTypep = dtypep->skipRefp();
        منطقي isArray = خطأ;
        طالما (ثابت Ast_گرێ_Array_DType* ثابت adtypep = VN_CAST(elemDTypep, گرێ_Array_DType)) {
            إذا (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = true;
        }
        ثابت Ast_گرێ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(elemDTypep, گرێ_UOr_Struct_DType);
        إذا (!sdtypep || !sdtypep->packed()) أرجع;
        ثابت std::string var_دەربڕین = "vlSymsp->" + instName(scopep) + "." + cName;
        ثابت std::string entSize
            = "sizeof(" + var_دەربڕین + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string زیادکردن_Forceable_Var_Statement(ثابت ScopeVarData& svd, ثابت AstScope* ثابت scopep,
                                            ثابت AstVar* ثابت varp, ثابت صحيح udim,
                                            ثابت صحيح pdim, ثابت std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        ثابت std::string var_دەربڕین = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        أكد(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_دەربڕین;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_دەربڕین + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ثابت std::string enable_Signal_کلیل
                = get_کلیل_Name(scopep, varp->name() + forceControlSuffix);
            ثابت std::map<ثابت std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_کلیل);

            إذا (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_کلیل << "'.");
            }

            ثابت ScopeVarData& svd = itpair->second;
            ثابت AstScope* ثابت scopep = svd.m_scopep;
            ثابت AstVar* ثابت varp = svd.m_varp;
            ثابت VarDims dims = dimsFor(svd);
            stmt
                += زیادکردن_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            إذا (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        أرجع stmt;
    }

    ساكن std::string get_کلیل_Name(ثابت AstScope* ثابت scopep, ثابت std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        إذا (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ثابت std::string::size_type dpos = whole.rfind("__DOT__");
        إذا (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        أرجع scpSym + " " + signal_name;
    }

    منطقي baseSignalIsValid(ثابت AstScope* ثابت scopep, ثابت AstVar* ثابت controlSignalVarp,
                           ثابت std::string& baseSignalName) ثابت {
        ثابت std::string base_Signal_کلیل = get_کلیل_Name(scopep, baseSignalName);
        ثابت std::map<ثابت std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_کلیل);
        إذا (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت baseSignalVarp = baseSignalIt->second.m_varp;
            إذا (m_scopeVars.ژماردن(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                أرجع خطأ;
            }
            إذا (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                أرجع خطأ;
            }
        }
        أرجع true;
    }

    منطقي baseSignalIsPublic(ثابت AstScope* ثابت scopep,
                            ثابت std::string& baseSignalName) ثابت {
        ثابت std::string base_Signal_کلیل = get_کلیل_Name(scopep, baseSignalName);
        ثابت std::map<ثابت std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_کلیل);
        إذا (baseSignalIt == m_scopeVars.end()) {
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            إذا (!baseSignalVarp->isSigPublic()) أرجع خطأ;
        }
        أرجع true;
    }

    منطقي forceControlSignalsAreValid(ثابت AstScope* ثابت scopep,
                                     ثابت AstVar* ثابت baseSignalVarp) ثابت {
        constexpr std::array<ثابت حرف*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        أرجع std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, هذا](ثابت std::string& forceControlSuffix) {
                ثابت std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                ثابت std::string control_Signal_کلیل = get_کلیل_Name(scopep, controlSignalName);
                ثابت std::size_t control_Signal_ژماردن = m_scopeVars.ژماردن(control_Signal_کلیل);
                إذا (control_Signal_ژماردن == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_کلیل << "'.");
                    أرجع خطأ;
                }
                إذا (control_Signal_ژماردن > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    أرجع خطأ;
                }
                أرجع true;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    فراغ varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        طالما (!scp.empty()) {
            ثابت auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            إذا ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ثابت auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                إذا (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            إذا ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } وإلا {
                إذا (prd_pos == string::npos) اكسر;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    فراغ addScopeVarEntry(ثابت AstScope* ثابت scopep, ثابت Ast_گرێ_Module* ثابت modp,
                          ثابت AstVar* ثابت varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        إذا (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ثابت std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ثابت std::string scpName = whole.substr(0, dpos);
        ثابت std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ثابت std::string varBasePretty = Ast_گرێ::vpiName(VName::dehash(varBase));
        ثابت std::string scpPretty = Ast_گرێ::prettyName(VName::dehash(scpName));
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        إذا (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    فراغ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        لكل (ثابت ScopeModPair& smPair : m_scopes) {
            ثابت AstScope* ثابت scopep = smPair.first;
            ثابت Ast_گرێ_Module* ثابت smodp = smPair.second;
            لكل (ثابت ModVarPair& mvPair : m_modVars) {
                ثابت Ast_گرێ_Module* ثابت modp = mvPair.first;
                ثابت AstVar* ثابت varp = mvPair.second;
                إذا (modp != smodp) استمر;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    فراغ collectIfaceRefs(ثابت AstScope* nodep) {
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        إذا (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) أرجع;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ثابت std::string path = Ast_گرێ::vpiName(nodep->name());
        ثابت std::string instName = Ast_گرێ::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        ثابت std::string parentPath = path.substr(0, path.length() - instName.length());

        لكل (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ثابت std::string refName = Ast_گرێ::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            إذا (!VString::startsWith(refName, parentPath)) استمر;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_گرێ::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    فراغ buildIfaceRefTable() {
        إذا (m_ifaceRefs.empty()) أرجع;
        ثابت std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](ثابت IfaceRefData& a, ثابت IfaceRefData& b) { أرجع a.m_suffix < b.m_suffix; });
        لكل (ثابت IfaceRefData& ird : m_ifaceRefs) {
            ثابت std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            إذا (m_scopeNames.find(scopeSym) == m_scopeNames.end()) استمر;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            ثابت auto parent = m_scopeNames.find(ird.m_parentSym);
            إذا (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_وشەکان_If(ird.m_name, true));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_وشەکان_If(ird.m_suffix, true));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_وشەکان_If(ird.m_modportName, true));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        إذا (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    فراغ buildVpiHierarchy() {
        لكل (ثابت auto& itpair : m_scopeNames) {
            ثابت std::string symName = itpair.second.m_symName;
            std::string above = symName;
            إذا (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            طالما (!above.empty()) {
                ثابت std::string::size_type pos = above.rfind("__");
                إذا (pos == std::string::npos) اكسر;
                above.resize(pos);
                إذا (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].پاڵنان_back(symName);
                    اكسر;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        إذا (v3Global.opt.vpi()) buildVpiHierarchy();

        إذا (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            لكل (ثابت auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](ثابت ScopeModPair& a, ثابت ScopeModPair& b) {
                             أرجع a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](ثابت AstCFunc* ap, ثابت AstCFunc* bp) {
                             إذا (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 أرجع bp->dpiImportPrototype();
                             }
                             أرجع ap->name() < bp->name();
                         });

        // Output
        إذا (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        إذا (v3Global.dpi()) {
            emitDpiHdr();
            إذا (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    فراغ visit(Ast_گرێ_Module* nodep) override {
        إذا (nodep->isConstPool()) أرجع;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCellInlineScope* nodep) override {
        ثابت std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ثابت std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ثابت صحيح timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_گرێ::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    فراغ visit(AstScope* nodep) override {
        إذا (VN_IS(m_modp, Class)) أرجع;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        إذا (v3Global.opt.vpi() && !nodep->is_لووتکە()) {
            ثابت std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ثابت صحيح timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_گرێ::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstScopeName* nodep) override {
        إذا (nodep->selfRelative()) أرجع;  // Its absolute copy is under the scope
        ثابت std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ثابت صحيح timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        إذا (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } وإلا {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    فراغ visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        إذا ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    فراغ visit(Ast_گرێ_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        إذا (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_ژمارە(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_ژمارە(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    فراغ visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.پاڵنان_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    فراغ visit(AstConst*) override {}
    فراغ visit(Ast_گرێ* nodep) override { iterateChildrenConst(nodep); }

عام:
    explicit EmitCSyms(AstNetlist* nodep, منطقي dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

فراغ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_لووتکە_Include();
    puts("#include \"verilated.h\"\n");
    إذا (v3Global.needTraceDumper()) {
        لكل (ثابت string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    إذا (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + لووتکە_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    لكل (Ast_گرێ_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), گرێ_Module);
        إذا (VN_IS(nodep, Class)) استمر;  // Class included earlier
        إذا (nodep->isConstPool()) استمر;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    إذا (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        لكل (ثابت auto& itpair : m_scopeFuncs) {
            ثابت AstCFunc* ثابت funcp = itpair.second.m_cfuncp;
            إذا (!funcp->dpiExportImpl()) استمر;
            ثابت std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            ثابت std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        لكل (ثابت std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(خطأ);  // public:

    puts("// INTERNAL STATE\n");
    puts(لووتکە_Class_Name() + "* const __Vm_modelp;\n");

    إذا (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    إذا (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    إذا (v3Global.hasEvents()) {
        إذا (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } وإلا {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    إذا (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    إذا (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    إذا (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    إذا (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    إذا (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::ژمارە_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.first;
        ثابت Ast_گرێ_Module* ثابت modp = itpair.second;
        إذا (VN_IS(modp, Class)) استمر;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    إذا (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    إذا (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        لكل (ثابت auto& itpair : m_scopeNames) {
            ثابت ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    إذا (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + لووتکە_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    إذا (v3Global.hasEvents()) {
        إذا (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } وإلا {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        إذا (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } وإلا {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        إذا (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } وإلا {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    إذا (v3Global.needTraceDumper()) {
        إذا (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    إذا (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

فراغ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    منطقي needs_New_هێڵ = خطأ;
    لكل (ثابت auto& itpair : m_scopeFuncs) {
        ثابت AstCFunc* ثابت funcp = itpair.second.m_cfuncp;
        إذا (!funcp->dpiExportImpl()) استمر;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_هێڵ = true;
    }
    إذا (needs_New_هێڵ) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        إذا (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    لكل (ثابت std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    إذا (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    إذا (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        لكل (ثابت auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        إذا (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        إذا (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

فراغ EmitCSyms::emitVarTables() {
    إذا (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) أرجع;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    لكل (ثابت auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        لكل (ثابت std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        لكل (ثابت std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        لكل (ثابت std::string& row : m_ifaceRefTableRows) {
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

فراغ EmitCSyms::emitScopeHier(TextStmts& stmts, منطقي destroy) {
    إذا (!v3Global.opt.vpi()) أرجع;

    إذا (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } وإلا {
        stmts.add("// Set up scope hierarchy");
    }

    ثابت std::string method = destroy ? "remove" : "add";
    لكل (ثابت auto& itpair : m_scopeNames) {
        إذا (itpair.first == "TOP") استمر;
        ثابت ScopeData& sd = itpair.second;
        ثابت std::string& name = sd.m_prettyName;
        ثابت std::string& scopeType = sd.m_type;
        إذا (name.find('.') != string::npos) استمر;
        إذا (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") استمر;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    لكل (ثابت auto& itpair : m_vpiScopeHierarchy) {
        ثابت std::string fromName = scopeSymString(itpair.first);
        ثابت std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        لكل (ثابت std::string& name : itpair.second) {
            ثابت std::string toName = scopeSymString(name);
            ثابت std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    إذا (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t کەڵەکە_Size = V3_کەڵەکە_ژماردن::ژماردن(v3Global.rootp());
        إذا (v3Global.opt.debug_کەڵەکە_Check()) کەڵەکە_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", کەڵەکە_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, کەڵەکە_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(کەڵەکە_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.first;
        ثابت Ast_گرێ_Module* ثابت modp = itpair.second;
        إذا (modp->is_لووتکە()) استمر;
        ثابت std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_وشەکان_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    إذا (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        إذا (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        إذا (v3Global.opt.mtasks()) {
            v3Global.rootp()->لووتکە_Modulep()->foreach([&](ثابت AstExecGraph* execGraphp) {
                لكل (ثابت V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    ثابت ExecMTask& mt = static_cast<ثابت ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    إذا (!v3Global.rootp()->timeunit().isNone()) {
        ثابت std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    إذا (!v3Global.rootp()->timeprecision().isNone()) {
        ثابت std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    لكل (ثابت auto& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.first;
        ثابت Ast_گرێ_Module* ثابت modp = i.second;
        ثابت AstScope* ثابت abovep = scopep->aboveScopep();
        إذا (!abovep) استمر;

        ثابت std::string protName = VIdProtect::protect_وشەکان_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        إذا (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } وإلا {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    إذا (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        لكل (ثابت ScopeModPair& i : m_scopes) {
            ثابت AstScope* ثابت scopep = i.first;
            Ast_گرێ_Module* ثابت modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ثابت منطقي first = !modp->user1();
            modp->user1(true);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    لكل (ثابت ScopeModPair& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.first;
        ثابت Ast_گرێ_Module* ثابت modp = i.second;
        إذا (!modp->verilatorLib()) استمر;
        ثابت AstVar* hashp = nullptr;
        ثابت AstVar* handlep = nullptr;
        لكل (ثابت Ast_گرێ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) استمر;
            إذا (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            إذا (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ثابت uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        إذا (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_لووتکە_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    إذا (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        ثابت std::string symClass = symClassName();
        لكل (ثابت auto& itpair : m_scopeNames) {
            ثابت ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_وشەکان_If(sd.m_prettyName, true));
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
    إذا (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, خطأ);

    إذا (v3Global.dpi()) {
        لكل (ثابت std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            لكل (ثابت auto& itpair : m_scopeFuncs) {
                ثابت ScopeFuncData& sfd = itpair.second;
                ثابت AstScopeName* ثابت scopep = sfd.m_scopep;
                ثابت AstCFunc* ثابت funcp = sfd.m_cfuncp;
                ثابت Ast_گرێ_Module* ثابت modp = sfd.m_modp;
                إذا (!funcp->dpiExportImpl()) استمر;

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
    إذا (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        صحيح tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        طالما (it != m_scopeVars.cend()) {
            ثابت std::string scopeName = it->second.m_scopeName;
            ثابت AstScope* ثابت instScopep = it->second.m_scopep;
            ثابت Ast_گرێ_Module* ثابت modp = it->second.m_modp;
            ثابت std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            لكل (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                ثابت ScopeVarData& svd = it->second;
                ثابت AstScope* ثابت scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ثابت AstVar* ثابت varp = svd.m_varp;
                ثابت VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ثابت std::pair<منطقي, std::string> fc = isForceControlSignal(varp);
                إذا (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    استمر;
                }

                std::string row;
                ثابت TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                بدل (kind) {
                حالة TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    اكسر;
                حالة TableEntryKind::FORCEABLE_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_back(زیادکردن_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    اكسر;
                }
                حالة TableEntryKind::PLAIN_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        زیادکردن_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    اكسر;
                }
                افتراضي: v3fatalSrc("Bad case");
                }
                إذا (kind == TableEntryKind::TABLE_ROW) استمر;
                إذا (ثابت Ast_گرێ_UOr_Struct_DType* ثابت sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), گرێ_UOr_Struct_DType)) {
                    إذا (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } وإلا إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                إذا (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            إذا (!rows.empty()) {
                ثابت size_t row_ژماردن = rows.size();
                std::string کلیل;
                لكل (ثابت std::string& r : rows) {
                    کلیل += r;
                    کلیل += '\0';
                }
                ثابت auto itt = tableByRows.find(کلیل);
                std::string tableName;
                إذا (itt != tableByRows.end()) {
                    tableName = itt->second;
                } وإلا {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(کلیل), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_ژماردن) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            لكل (ثابت std::string& r : residual) stmts.add(r);
        }
    }

    أرجع stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, true);
    إذا (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    إذا (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + لووتکە_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    إذا (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    لكل (ثابت auto& itpair : m_scopeNames) {
        ثابت ScopeData& sd = itpair.second;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    لكل (ثابت ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        ثابت AstScope* ثابت scopep = itpair.first;
        ثابت Ast_گرێ_Module* ثابت modp = itpair.second;
        إذا (modp->is_لووتکە()) استمر;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    أرجع stmts;
}

فراغ EmitCSyms::emitSymImp(ثابت AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ثابت std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            true, "Symbol table implementation internals",
                            [هذا]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), true, true, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ثابت AstScope* ثابت لووتکە_Scopep = netlistp->لووتکە_Scopep()->scopep();
    ثابت std::string لووتکە_Inst = "vlSymsp->" + instName(لووتکە_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    إذا (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + لووتکە_Inst + ", vlSymsp, namep);\n");
    لكل (ثابت TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + لووتکە_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    إذا (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    إذا (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    إذا (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    إذا (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    لكل (ثابت TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + لووتکە_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    إذا (v3Global.needTraceDumper()) {
        إذا (!optSystemC()) {
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

    إذا (v3Global.opt.savable()) {
        لكل (ثابت منطقي& de : {خطأ, true}) {
            ثابت std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ثابت std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ثابت std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            إذا (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            لكل (ثابت ScopeModPair& itpair : m_scopes) {
                ثابت AstScope* ثابت scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

فراغ EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(لووتکە_Class_Name() + "__Dpi",
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
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        لكل (ثابت Ast_گرێ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    إذا (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    صحيح firstExp = 0;
    صحيح firstImp = 0;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) استمر;

        ثابت std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        إذا (nodep->dpiExportDispatcher()) {
            إذا (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } وإلا {
            إذا (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        إذا (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } وإلا {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, خطأ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

فراغ EmitCSyms::emitDpiStruct(ثابت Ast_گرێ_DType* dtypep, std::set<std::string>& namesr) {
    إذا (!dtypep->containsUnpackedStruct()) أرجع;
    ثابت AstStructDType* ثابت sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ثابت std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    إذا (namesr.ژماردن(name)) أرجع;
    // After the structures it contains
    لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    إذا (name.empty()) أرجع;  // Declared where used
    إذا (namesr.empty()) puts("// DPI TYPES\n");
    namesr.زیادکردن(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

فراغ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(لووتکە_Class_Name() + "__Dpi", خطأ, true,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + لووتکە_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + لووتکە_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + لووتکە_Class_Name() + ".h\"\n");
    puts("\n");

    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher()) استمر;

        ثابت std::string name = nodep->name();
        ثابت std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + لووتکە_Class_Name() + "::" + name + "(");
        std::string comma;
        لكل (ثابت Ast_گرێ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn()) {
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

فراغ V3EmitC::emitcSyms(منطقي dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
