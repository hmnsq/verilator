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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3EmitC.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3ExecGraph.h"
#অন্তর্ভাব "V3LanguageWords.h"
#অন্তর্ভাব "V3StackCount.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cstring>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
স্থির constexpr auto symClassName = &EmitCUtil::symClassName;
স্থির constexpr auto শীর্ষ_Class_Name = &EmitCUtil::শীর্ষ_Class_Name;

শ্রেণী EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ধ্রুবক VNUser1InUse m_inuser1;

    // TYPES
    কাঠামো Scope_তথ্য final {
        ধ্রুবক Ast_গ্রন্থি* m_nodep;  // Scope symbol-table entry corresponds to
        ধ্রুবক std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ধ্রুবক std::string m_prettyName;  // Pretty (unprotected) name for display
        ধ্রুবক std::string m_defName;  // Name registered for this scope
        ধ্রুবক পূর্ণসংখ্যা m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_তথ্য(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক std::string& symName, ধ্রুবক std::string& prettyName,
                  ধ্রুবক std::string& defName, পূর্ণসংখ্যা timeunit, ধ্রুবক std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    কাঠামো Iface_Ref_তথ্য final {
        ধ্রুবক AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_তথ্য(ধ্রুবক AstScope* scopep, ধ্রুবক std::string& parentSym,
                     ধ্রুবক std::string& suffix, ধ্রুবক std::string& name,
                     ধ্রুবক std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    কাঠামো Scope_Func_তথ্য final {
        ধ্রুবক AstScopeName* ধ্রুবক m_scopep;  // Scope the DPI export function is registered under
        ধ্রুবক AstCFunc* ধ্রুবক m_cfuncp;  // DPI export function
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Module containing the DPI export function
        Scope_Func_তথ্য(ধ্রুবক AstScopeName* scopep, ধ্রুবক AstCFunc* funcp, ধ্রুবক Ast_গ্রন্থি_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    কাঠামো Scope_Var_তথ্য final {
        ধ্রুবক std::string m_scopeName;  // Full name of the scope containing the variable
        ধ্রুবক std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        ধ্রুবক AstVar* ধ্রুবক m_varp;  // Public variable
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Module containing the variable
        ধ্রুবক AstScope* ধ্রুবক m_scopep;  // Scope containing the variable
        Scope_Var_তথ্য(ধ্রুবক std::string& scopeName, ধ্রুবক std::string& varBasePretty,
                     ধ্রুবক AstVar* varp, ধ্রুবক Ast_গ্রন্থি_Module* modp, ধ্রুবক AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<ধ্রুবক std::string, Scope_তথ্য>;
    using ScopeModPair = std::pair<ধ্রুবক AstScope*, Ast_গ্রন্থি_Module*>;
    using ModVarPair = std::pair<ধ্রুবক Ast_গ্রন্থি_Module*, ধ্রুবক AstVar*>;

    // Vars with more dims take the residual per-statement path.
    স্থির constexpr পূর্ণসংখ্যা VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    ধ্রুবক AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ধ্রুবক std::string, Scope_Func_তথ্য> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ধ্রুবক std::string, Scope_Var_তথ্য> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<Iface_Ref_তথ্য> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ধ্রুবক std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    পূর্ণসংখ্যা m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    পূর্ণসংখ্যা m_modCoverBins = 0;  // Per-module coverage bin number
    ধ্রুবক যৌক্তিক m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    শূন্য emitSymHdr();
    শূন্য emitSymImpPreamble();
    শূন্য emitVarTables();
    শূন্য emitScopeHier(TextStmts& stmts, যৌক্তিক destroy);
    শূন্য emitSymImp(ধ্রুবক AstNetlist* netlistp);
    শূন্য emitDpiHdr();
    শূন্য emitDpiStruct(ধ্রুবক Ast_গ্রন্থি_DType* dtypep, std::set<std::string>& namesr);
    শূন্য emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    স্থির শূন্য nameCheck(Ast_গ্রন্থি* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        যদি (nodep->name().empty()) ফেরত;
        ধ্রুবক AstCFunc* ধ্রুবক cfuncp = VN_CAST(nodep, CFunc);
        যদি (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ধ্রুবক std::string rsvd = V3_Language_শব্দগুলি::isKeyword(nodep->name());
            যদি (rsvd != "") {
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

    স্থির std::string scopeSymString(ধ্রুবক std::string& scpname) {
        std::string out = scpname;
        std::string::আকার_type pos;
        যখন ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        যদি (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        যদি (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        যখন ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        যখন ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        ফেরত out;
    }

    স্থির string scopeDecodeIdentifier(ধ্রুবক std::string& scpname) {
        std::string::আকার_type pos = std::string::npos;

        // Remove hierarchy
        আকার_t i = 0;
        // always makes progress
        যখন (i < scpname.দৈর্ঘ্য()) {
            যদি (scpname[i] == '\\') {
                যখন (i < scpname.দৈর্ঘ্য() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } অন্যথা {
                যখন (i < scpname.দৈর্ঘ্য() && scpname[i] != '.') ++i;
                যদি (i < scpname.দৈর্ঘ্য()) pos = i++;
            }
        }

        ফেরত pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    কাঠামো VarDims final {
        পূর্ণসংখ্যা pdim = 0;  // Packed array dims plus a ranged basic leaf
        পূর্ণসংখ্যা udim = 0;
        std::vector<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>> flatten() ধ্রুবক {
            std::vector<std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>> flat(unpackedLR);
            flat.ঢোকাও(flat.end(), packedLR.cbegin(), packedLR.cend());
            ফেরত flat;
        }
    };
    স্থির VarDims getVarDims(ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক মূল_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        জন্য (ধ্রুবক Ast_গ্রন্থি_DType* dtypep = মূল_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            যদি (ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক adtypep = VN_CAST(dtypep, গ্রন্থি_Array_DType)) {
                যদি (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->বাম(), adtypep->ডান());
                    ++d.pdim;
                } অন্যথা {
                    d.unpackedLR.emplace_back(adtypep->বাম(), adtypep->ডান());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } অন্যথা {
                যদি (ধ্রুবক AstBasicDType* ধ্রুবক basicp = dtypep->basicp()) {
                    যদি (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->বাম(), basicp->ডান());
                        ++d.pdim;
                    }
                }
                বিরতি;  // Non-array leaf
            }
        }
        ফেরত d;
    }

    স্থির VarDims dimsFor(ধ্রুবক Scope_Var_তথ্য& svd) { ফেরত getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    স্থির std::string boundsString(ধ্রুবক VarDims& d) {
        std::string bounds;
        জন্য (ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        ফেরত bounds;
    }

    স্থির পূর্ণসংখ্যা getUnpackedElements(ধ্রুবক Ast_গ্রন্থি_DType* মূল_Dtypep) {
        পূর্ণসংখ্যা elements = 1;
        জন্য (ধ্রুবক Ast_গ্রন্থি_DType* dtypep = মূল_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(dtypep, UnpackArrayDType);
            যদি (!adtypep) বিরতি;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        ফেরত elements;
    }

    স্থির যৌক্তিক needs_Emitted_Ent_আকার(ধ্রুবক std::string& vlEnumType) {
        ফেরত vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    স্থির std::pair<যৌক্তিক, std::string> isForceControlSignal(ধ্রুবক AstVar* ধ্রুবক signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        জন্য (ধ্রুবক std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ধ্রুবক std::আকার_t suffixPos = signalVarp->name().find(forceControlSuffix);
            ধ্রুবক যৌক্তিক suffixFound = suffixPos != std::string::npos;
            ধ্রুবক যৌক্তিক suffixIsAtEnd
                = suffixPos + forceControlSuffix.দৈর্ঘ্য() == signalVarp->name().দৈর্ঘ্য();
            যদি (!(suffixFound && suffixIsAtEnd)) চালু;
            ধ্রুবক std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            ফেরত std::pair<যৌক্তিক, std::string>{সত্য, baseSignalName};
        }
        ফেরত std::pair<যৌক্তিক, std::string>{মিথ্যা, ""};
    }

    স্থির শূন্য appendVarProperties(std::string& stmt, ধ্রুবক std::string& vlEnumType,
                                    ধ্রুবক std::string& vlEnumDir, ধ্রুবক পূর্ণসংখ্যা udim, ধ্রুবক পূর্ণসংখ্যা pdim,
                                    ধ্রুবক std::string& bounds, ধ্রুবক std::string& ent_আকার = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        যদি (!ent_আকার.empty()) {
            stmt += ", ";
            stmt += ent_আকার;
        } অন্যথা {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    স্থির std::string memberVlEnumDir(ধ্রুবক AstVar* ধ্রুবক varp,
                                       ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/সত্য);
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক skipDTypep = dtypep->skipRefp();
        যদি (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        যদি (ধ্রুবক AstBasicDType* ধ্রুবক basicp = skipDTypep->basicp()) {
            যদি (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        ফেরত out + ')';
    }

    // Name of the instance member for the given scope
    স্থির std::string instName(ধ্রুবক AstScope* scopep) {
        ফেরত "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    স্থির std::string ঢোকাও_Var_Statement(ধ্রুবক Scope_Var_তথ্য& svd, ধ্রুবক AstScope* ধ্রুবক scopep,
                                          ধ্রুবক AstVar* ধ্রুবক varp, ধ্রুবক পূর্ণসংখ্যা udim, ধ্রুবক পূর্ণসংখ্যা pdim,
                                          ধ্রুবক std::string& bounds) {
        ধ্রুবক std::string var_রাশিমালা = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ধ্রুবক std::string vlEnumType = varp->vlEnumType();
        ধ্রুবক যৌক্তিক needs_Ent_আকার = needs_Emitted_Ent_আকার(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_আকার ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        যদি (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_রাশিমালা;
            stmt += "), false, ";
        } অন্যথা যদি (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_রাশিমালা;
            stmt += ".c_str())), true, ";
        } অন্যথা {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_রাশিমালা;
            stmt += "))), true, ";
        }

        ধ্রুবক std::string ent_আকার = needs_Ent_আকার
                                        ? "sizeof(" + var_রাশিমালা + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_আকার);
        ফেরত stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    গণনা শ্রেণী TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(ধ্রুবক Scope_Var_তথ্য& svd, ধ্রুবক AstVar* ধ্রুবক varp,
                                      ধ্রুবক AstScope* ধ্রুবক scopep,
                                      ধ্রুবক std::string& modClassName, ধ্রুবক VarDims& dims,
                                      std::string& rowOut) ধ্রুবক {
        ধ্রুবক পূর্ণসংখ্যা pdim = dims.pdim;
        ধ্রুবক পূর্ণসংখ্যা udim = dims.udim;
        যদি (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            ফেরত TableEntryKind::FORCEABLE_RESIDUAL;
        }
        যদি (udim + pdim > VPI_TABLE_MAX_DIMS) ফেরত TableEntryKind::PLAIN_RESIDUAL;

        ধ্রুবক std::string vlEnumType = varp->vlEnumType();
        যদি (needs_Emitted_Ent_আকার(vlEnumType))
            ফেরত TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        যদি (varp->isParam()) ফেরত TableEntryKind::PLAIN_RESIDUAL;

        ধ্রুবক std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ধ্রুবক std::string member = varName(varp);
        ধ্রুবক std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<পূর্ণসংখ্যা> dv;
        জন্য (ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা>& lr : dims.flatten()) {
            dv.ঠেলো_back(lr.first);
            dv.ঠেলো_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        জন্য (পূর্ণসংখ্যা i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            যদি (i) row += ", ";
            row += std::to_string(i < static_cast<পূর্ণসংখ্যা>(dv.আকার_2()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        ফেরত TableEntryKind::TABLE_ROW;
    }

    স্থির std::string ঢোকাও_DType_Var_Statement(ধ্রুবক Scope_Var_তথ্য& svd,
                                               ধ্রুবক AstScope* ধ্রুবক scopep,
                                               ধ্রুবক std::string& prettyName,
                                               ধ্রুবক std::string& cName,
                                               ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep, ধ্রুবক পূর্ণসংখ্যা udim,
                                               ধ্রুবক পূর্ণসংখ্যা pdim, ধ্রুবক std::string& bounds) {
        ধ্রুবক std::string vlEnumType = dtypep->vlEnumType();
        ধ্রুবক যৌক্তিক needs_Ent_আকার = needs_Emitted_Ent_আকার(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_আকার ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        ধ্রুবক std::string var_রাশিমালা = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        যদি (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_রাশিমালা;
            stmt += "))), true, ";
        } অন্যথা {
            stmt += ", &(";
            stmt += var_রাশিমালা;
            stmt += "), false, ";
        }
        ধ্রুবক std::string ent_আকার
            = needs_Ent_আকার
                  ? "sizeof(" + var_রাশিমালা + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_আকার);
        ফেরত stmt;
    }

    স্থির শূন্য addUOrStructMemberVars(std::vector<std::string>& stmts, ধ্রুবক Scope_Var_তথ্য& svd,
                                       ধ্রুবক AstScope* ধ্রুবক scopep,
                                       ধ্রুবক std::string& prettyPrefix, ধ্রুবক std::string& cPrefix,
                                       ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep) {
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক itemDTypep = itemp->dtypep();
            যদি (itemDTypep->vlEnumType().empty()) চালু;
            ধ্রুবক std::string prettyName
                = prettyPrefix + "." + Ast_গ্রন্থি::vpiName(itemp->shortName());
            ধ্রুবক std::string cName = cPrefix + "." + itemp->nameProtect();
            ধ্রুবক VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(ঢোকাও_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক subp
                = VN_CAST(itemDTypep->skipRefp(), গ্রন্থি_UOr_Struct_DType)) {
                যদি (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } অন্যথা যদি (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    স্থির শূন্য addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    ধ্রুবক Scope_Var_তথ্য& svd,
                                                    ধ্রুবক AstScope* ধ্রুবক scopep,
                                                    ধ্রুবক std::string& prettyPrefix,
                                                    ধ্রুবক std::string& cPrefix,
                                                    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep) {
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক skipDTypep = dtypep->skipRefp();
        যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep
                   = VN_CAST(skipDTypep, গ্রন্থি_UOr_Struct_DType)) {
            যদি (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    স্থির শূন্য addPackedMemberVars(std::vector<std::string>& stmts, ধ্রুবক Scope_Var_তথ্য& svd,
                                    ধ্রুবক AstScope* ধ্রুবক scopep, ধ্রুবক std::string& prettyPrefix,
                                    ধ্রুবক std::string& cName, ধ্রুবক std::string& vlEnumType,
                                    ধ্রুবক std::string& ent_আকার,
                                    ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep,
                                    ধ্রুবক পূর্ণসংখ্যা parentBitOffset, ধ্রুবক যৌক্তিক needs_সূচক) {
        ধ্রুবক std::string var_রাশিমালা = "vlSymsp->" + instName(scopep) + "." + cName;
        জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক itemDTypep = itemp->dtypep();
            ধ্রুবক std::string prettyName
                = prettyPrefix + "." + Ast_গ্রন্থি::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ধ্রুবক পূর্ণসংখ্যা bitOffset = parentBitOffset + itemp->lsb();
            ধ্রুবক VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            যদি (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_রাশিমালা + "))), true, ";
            } অন্যথা {
                stmt += ", &(" + var_রাশিমালা + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            যদি (needs_সূচক) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += ent_আকার + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ধ্রুবক Ast_গ্রন্থি_DType* subDTypep = itemDTypep->skipRefp();
            যৌক্তিক isArray = মিথ্যা;
            যখন (ধ্রুবক AstPackArrayDType* ধ্রুবক adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = সত্য;
            }
            যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক subp = VN_CAST(subDTypep, গ্রন্থি_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_আকার,
                                    subp, bitOffset, needs_সূচক || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    স্থির শূন্য addPackedAggMemberVars(std::vector<std::string>& stmts, ধ্রুবক Scope_Var_তথ্য& svd,
                                       ধ্রুবক AstScope* ধ্রুবক scopep,
                                       ধ্রুবক std::string& prettyPrefix, ধ্রুবক std::string& cName,
                                       ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep) {
        std::string elemCName = cName;
        ধ্রুবক Ast_গ্রন্থি_DType* elemDTypep = dtypep->skipRefp();
        যৌক্তিক isArray = মিথ্যা;
        যখন (ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক adtypep = VN_CAST(elemDTypep, গ্রন্থি_Array_DType)) {
            যদি (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = সত্য;
        }
        ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep = VN_CAST(elemDTypep, গ্রন্থি_UOr_Struct_DType);
        যদি (!sdtypep || !sdtypep->packed()) ফেরত;
        ধ্রুবক std::string var_রাশিমালা = "vlSymsp->" + instName(scopep) + "." + cName;
        ধ্রুবক std::string ent_আকার
            = "sizeof(" + var_রাশিমালা + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            ent_আকার, sdtypep, 0, isArray);
    }

    std::string ঢোকাও_Forceable_Var_Statement(ধ্রুবক Scope_Var_তথ্য& svd, ধ্রুবক AstScope* ধ্রুবক scopep,
                                            ধ্রুবক AstVar* ধ্রুবক varp, ধ্রুবক পূর্ণসংখ্যা udim,
                                            ধ্রুবক পূর্ণসংখ্যা pdim, ধ্রুবক std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        ধ্রুবক std::string var_রাশিমালা = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        প্রতিজ্ঞা(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_রাশিমালা;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_রাশিমালা + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        জন্য (ধ্রুবক std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ধ্রুবক std::string enable_Signal_চাবি
                = get_চাবি_Name(scopep, varp->name() + forceControlSuffix);
            ধ্রুবক std::map<ধ্রুবক std::string, Scope_Var_তথ্য>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_চাবি);

            যদি (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_চাবি << "'.");
            }

            ধ্রুবক Scope_Var_তথ্য& svd = itpair->second;
            ধ্রুবক AstScope* ধ্রুবক scopep = svd.m_scopep;
            ধ্রুবক AstVar* ধ্রুবক varp = svd.m_varp;
            ধ্রুবক VarDims dims = dimsFor(svd);
            stmt
                += ঢোকাও_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            যদি (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        ফেরত stmt;
    }

    স্থির std::string get_চাবি_Name(ধ্রুবক AstScope* ধ্রুবক scopep, ধ্রুবক std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        যদি (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ধ্রুবক std::string::আকার_type dpos = whole.rfind("__DOT__");
        যদি (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        ধ্রুবক std::string scpSym = scopeSymString(VName::dehash(scpName));
        ফেরত scpSym + " " + signal_name;
    }

    যৌক্তিক baseSignalIsValid(ধ্রুবক AstScope* ধ্রুবক scopep, ধ্রুবক AstVar* ধ্রুবক controlSignalVarp,
                           ধ্রুবক std::string& baseSignalName) ধ্রুবক {
        ধ্রুবক std::string base_Signal_চাবি = get_চাবি_Name(scopep, baseSignalName);
        ধ্রুবক std::map<ধ্রুবক std::string, Scope_Var_তথ্য>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_চাবি);
        যদি (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            ফেরত মিথ্যা;
        } অন্যথা {
            ধ্রুবক AstVar* ধ্রুবক baseSignalVarp = baseSignalIt->second.m_varp;
            যদি (m_scopeVars.গুনতি(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                ফেরত মিথ্যা;
            }
            যদি (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                ফেরত মিথ্যা;
            }
        }
        ফেরত সত্য;
    }

    যৌক্তিক baseSignalIsPublic(ধ্রুবক AstScope* ধ্রুবক scopep,
                            ধ্রুবক std::string& baseSignalName) ধ্রুবক {
        ধ্রুবক std::string base_Signal_চাবি = get_চাবি_Name(scopep, baseSignalName);
        ধ্রুবক std::map<ধ্রুবক std::string, Scope_Var_তথ্য>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_চাবি);
        যদি (baseSignalIt == m_scopeVars.end()) {
            ফেরত মিথ্যা;
        } অন্যথা {
            ধ্রুবক AstVar* ধ্রুবক baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            যদি (!baseSignalVarp->isSigPublic()) ফেরত মিথ্যা;
        }
        ফেরত সত্য;
    }

    যৌক্তিক forceControlSignalsAreValid(ধ্রুবক AstScope* ধ্রুবক scopep,
                                     ধ্রুবক AstVar* ধ্রুবক baseSignalVarp) ধ্রুবক {
        constexpr std::array<ধ্রুবক অক্ষর*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        ফেরত std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, এই](ধ্রুবক std::string& forceControlSuffix) {
                ধ্রুবক std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                ধ্রুবক std::string control_Signal_চাবি = get_চাবি_Name(scopep, controlSignalName);
                ধ্রুবক std::আকার_t control_Signal_গুনতি = m_scopeVars.গুনতি(control_Signal_চাবি);
                যদি (control_Signal_গুনতি == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_চাবি << "'.");
                    ফেরত মিথ্যা;
                }
                যদি (control_Signal_গুনতি > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    ফেরত মিথ্যা;
                }
                ফেরত সত্য;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    শূন্য varHierarchyScopes(std::string scp) {

        std::string::আকার_type prd_pos = scp.rfind('.');
        std::string::আকার_type dot_pos = scp.rfind("__DOT__");

        যখন (!scp.empty()) {
            ধ্রুবক auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            যদি ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ধ্রুবক auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                যদি (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            যদি ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } অন্যথা {
                যদি (prd_pos == string::npos) বিরতি;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    শূন্য addScopeVarEntry(ধ্রুবক AstScope* ধ্রুবক scopep, ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp,
                          ধ্রুবক AstVar* ধ্রুবক varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        যদি (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        ধ্রুবক std::string::আকার_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ধ্রুবক std::string scpName = whole.substr(0, dpos);
        ধ্রুবক std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ধ্রুবক std::string varBasePretty = Ast_গ্রন্থি::vpiName(VName::dehash(varBase));
        ধ্রুবক std::string scpPretty = Ast_গ্রন্থি::prettyName(VName::dehash(scpName));
        ধ্রুবক std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        যদি (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    শূন্য varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        জন্য (ধ্রুবক ScopeModPair& smPair : m_scopes) {
            ধ্রুবক AstScope* ধ্রুবক scopep = smPair.first;
            ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক smodp = smPair.second;
            জন্য (ধ্রুবক ModVarPair& mvPair : m_modVars) {
                ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = mvPair.first;
                ধ্রুবক AstVar* ধ্রুবক varp = mvPair.second;
                যদি (modp != smodp) চালু;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    শূন্য collectIfaceRefs(ধ্রুবক AstScope* nodep) {
        ধ্রুবক AstCell* ধ্রুবক cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        যদি (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) ফেরত;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ধ্রুবক std::string path = Ast_গ্রন্থি::vpiName(nodep->name());
        ধ্রুবক std::string instName = Ast_গ্রন্থি::vpiName(cellp->origName());
        UASSERT_OBJ(path.দৈর্ঘ্য() > instName.দৈর্ঘ্য() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        ধ্রুবক std::string parentPath = path.substr(0, path.দৈর্ঘ্য() - instName.দৈর্ঘ্য());

        জন্য (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ধ্রুবক std::string refName = Ast_গ্রন্থি::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            যদি (!VString::startsWith(refName, parentPath)) চালু;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_গ্রন্থি::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    শূন্য buildIfaceRefTable() {
        যদি (m_ifaceRefs.empty()) ফেরত;
        ধ্রুবক std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](ধ্রুবক Iface_Ref_তথ্য& a, ধ্রুবক Iface_Ref_তথ্য& b) { ফেরত a.m_suffix < b.m_suffix; });
        জন্য (ধ্রুবক Iface_Ref_তথ্য& ird : m_ifaceRefs) {
            ধ্রুবক std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            যদি (m_scopeNames.find(scopeSym) == m_scopeNames.end()) চালু;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            ধ্রুবক auto parent = m_scopeNames.find(ird.m_parentSym);
            যদি (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_শব্দগুলি_If(ird.m_name, সত্য));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_শব্দগুলি_If(ird.m_suffix, সত্য));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_শব্দগুলি_If(ird.m_modportName, সত্য));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        যদি (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    শূন্য buildVpiHierarchy() {
        জন্য (ধ্রুবক auto& itpair : m_scopeNames) {
            ধ্রুবক std::string symName = itpair.second.m_symName;
            std::string above = symName;
            যদি (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            যখন (!above.empty()) {
                ধ্রুবক std::string::আকার_type pos = above.rfind("__");
                যদি (pos == std::string::npos) বিরতি;
                above.resize(pos);
                যদি (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].ঠেলো_back(symName);
                    বিরতি;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        যদি (v3Global.opt.vpi()) buildVpiHierarchy();

        যদি (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            জন্য (ধ্রুবক auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](ধ্রুবক ScopeModPair& a, ধ্রুবক ScopeModPair& b) {
                             ফেরত a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](ধ্রুবক AstCFunc* ap, ধ্রুবক AstCFunc* bp) {
                             যদি (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 ফেরত bp->dpiImportPrototype();
                             }
                             ফেরত ap->name() < bp->name();
                         });

        // Output
        যদি (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        যদি (v3Global.dpi()) {
            emitDpiHdr();
            যদি (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->isConstPool()) ফেরত;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCellInlineScope* nodep) override {
        ধ্রুবক std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ধ্রুবক std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ধ্রুবক পূর্ণসংখ্যা timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_গ্রন্থি::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    শূন্য visit(AstScope* nodep) override {
        যদি (VN_IS(m_modp, Class)) ফেরত;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        যদি (v3Global.opt.vpi() && !nodep->is_শীর্ষ()) {
            ধ্রুবক std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ধ্রুবক পূর্ণসংখ্যা timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_গ্রন্থি::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstScopeName* nodep) override {
        যদি (nodep->selfRelative()) ফেরত;  // Its absolute copy is under the scope
        ধ্রুবক std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ধ্রুবক পূর্ণসংখ্যা timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        যদি (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } অন্যথা {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    শূন্য visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        যদি ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        যদি (!nodep->তথ্য_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_অঙ্ক(m_coverBins);
            m_coverBins += nodep->আকার_2();
            nodep->local_Bin_অঙ্ক(m_modCoverBins);
            m_modCoverBins += nodep->আকার_2();
        }
    }
    শূন্য visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        যদি (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ঠেলো_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    শূন্য visit(AstConst*) override {}
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    explicit EmitCSyms(AstNetlist* nodep, যৌক্তিক dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

শূন্য EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_শীর্ষ_Include();
    puts("#include \"verilated.h\"\n");
    যদি (v3Global.needTraceDumper()) {
        জন্য (ধ্রুবক string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    যদি (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + শীর্ষ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    জন্য (Ast_গ্রন্থি_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), গ্রন্থি_Module);
        যদি (VN_IS(nodep, Class)) চালু;  // Class included earlier
        যদি (nodep->isConstPool()) চালু;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    যদি (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        জন্য (ধ্রুবক auto& itpair : m_scopeFuncs) {
            ধ্রুবক AstCFunc* ধ্রুবক funcp = itpair.second.m_cfuncp;
            যদি (!funcp->dpiExportImpl()) চালু;
            ধ্রুবক std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            ধ্রুবক std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        জন্য (ধ্রুবক std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(মিথ্যা);  // public:

    puts("// INTERNAL STATE\n");
    puts(শীর্ষ_Class_Name() + "* const __Vm_modelp;\n");

    যদি (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    যদি (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    যদি (v3Global.hasEvents()) {
        যদি (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } অন্যথা {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    যদি (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    যদি (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    যদি (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    যদি (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    যদি (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::অঙ্ক_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    জন্য (ধ্রুবক ScopeModPair& itpair : m_scopes) {
        ধ্রুবক AstScope* ধ্রুবক scopep = itpair.first;
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = itpair.second;
        যদি (VN_IS(modp, Class)) চালু;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    যদি (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    যদি (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        জন্য (ধ্রুবক auto& itpair : m_scopeNames) {
            ধ্রুবক Scope_তথ্য& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    যদি (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + শীর্ষ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    যদি (v3Global.hasEvents()) {
        যদি (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } অন্যথা {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        যদি (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } অন্যথা {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        যদি (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } অন্যথা {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    যদি (v3Global.needTraceDumper()) {
        যদি (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    যদি (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

শূন্য EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    যৌক্তিক needs_New_সারি = মিথ্যা;
    জন্য (ধ্রুবক auto& itpair : m_scopeFuncs) {
        ধ্রুবক AstCFunc* ধ্রুবক funcp = itpair.second.m_cfuncp;
        যদি (!funcp->dpiExportImpl()) চালু;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_সারি = সত্য;
    }
    যদি (needs_New_সারি) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    জন্য (ধ্রুবক ScopeModPair& itpair : m_scopes) {
        যদি (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    জন্য (ধ্রুবক std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    যদি (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    যদি (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        জন্য (ধ্রুবক auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        যদি (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        যদি (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

শূন্য EmitCSyms::emitVarTables() {
    যদি (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) ফেরত;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    জন্য (ধ্রুবক auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        জন্য (ধ্রুবক std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    যদি (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        জন্য (ধ্রুবক std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    যদি (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        জন্য (ধ্রুবক std::string& row : m_ifaceRefTableRows) {
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

শূন্য EmitCSyms::emitScopeHier(TextStmts& stmts, যৌক্তিক destroy) {
    যদি (!v3Global.opt.vpi()) ফেরত;

    যদি (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } অন্যথা {
        stmts.add("// Set up scope hierarchy");
    }

    ধ্রুবক std::string method = destroy ? "remove" : "add";
    জন্য (ধ্রুবক auto& itpair : m_scopeNames) {
        যদি (itpair.first == "TOP") চালু;
        ধ্রুবক Scope_তথ্য& sd = itpair.second;
        ধ্রুবক std::string& name = sd.m_prettyName;
        ধ্রুবক std::string& scopeType = sd.m_type;
        যদি (name.find('.') != string::npos) চালু;
        যদি (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") চালু;
        ধ্রুবক std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    জন্য (ধ্রুবক auto& itpair : m_vpiScopeHierarchy) {
        ধ্রুবক std::string fromName = scopeSymString(itpair.first);
        ধ্রুবক std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        জন্য (ধ্রুবক std::string& name : itpair.second) {
            ধ্রুবক std::string toName = scopeSymString(name);
            ধ্রুবক std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    যদি (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t স্তূপ_আকার = V3_স্তূপ_গুনতি::গুনতি(v3Global.rootp());
        যদি (v3Global.opt.debug_স্তূপ_Check()) স্তূপ_আকার += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", স্তূপ_আকার);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_আকার, স্তূপ_আকার + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(স্তূপ_আকার) + ");");
    }

    stmts.add("// Setup sub module instances");
    জন্য (ধ্রুবক ScopeModPair& itpair : m_scopes) {
        ধ্রুবক AstScope* ধ্রুবক scopep = itpair.first;
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = itpair.second;
        যদি (modp->is_শীর্ষ()) চালু;
        ধ্রুবক std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_শব্দগুলি_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    যদি (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        যদি (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        যদি (v3Global.opt.mtasks()) {
            v3Global.rootp()->শীর্ষ_Modulep()->foreach([&](ধ্রুবক AstExecGraph* execGraphp) {
                জন্য (ধ্রুবক V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    ধ্রুবক ExecMTask& mt = static_cast<ধ্রুবক ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    যদি (!v3Global.rootp()->timeunit().isNone()) {
        ধ্রুবক std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    যদি (!v3Global.rootp()->timeprecision().isNone()) {
        ধ্রুবক std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    জন্য (ধ্রুবক auto& i : m_scopes) {
        ধ্রুবক AstScope* ধ্রুবক scopep = i.first;
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = i.second;
        ধ্রুবক AstScope* ধ্রুবক abovep = scopep->aboveScopep();
        যদি (!abovep) চালু;

        ধ্রুবক std::string protName = VIdProtect::protect_শব্দগুলি_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        যদি (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } অন্যথা {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    যদি (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        জন্য (ধ্রুবক ScopeModPair& i : m_scopes) {
            ধ্রুবক AstScope* ধ্রুবক scopep = i.first;
            Ast_গ্রন্থি_Module* ধ্রুবক modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ধ্রুবক যৌক্তিক first = !modp->user1();
            modp->user1(সত্য);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    জন্য (ধ্রুবক ScopeModPair& i : m_scopes) {
        ধ্রুবক AstScope* ধ্রুবক scopep = i.first;
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = i.second;
        যদি (!modp->verilatorLib()) চালু;
        ধ্রুবক AstVar* hashp = nullptr;
        ধ্রুবক AstVar* handlep = nullptr;
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ধ্রুবক AstVar* ধ্রুবক varp = VN_CAST(nodep, Var);
            যদি (!varp) চালু;
            যদি (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            যদি (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ধ্রুবক uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        যদি (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_শীর্ষ_Name().আকার_2() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    যদি (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        ধ্রুবক std::string symClass = symClassName();
        জন্য (ধ্রুবক auto& itpair : m_scopeNames) {
            ধ্রুবক Scope_তথ্য& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_শব্দগুলি_If(sd.m_prettyName, সত্য));
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
                  + std::to_string(m_scopeNames.আকার_2()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    buildIfaceRefTable();
    যদি (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.আকার_2()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, মিথ্যা);

    যদি (v3Global.dpi()) {
        জন্য (ধ্রুবক std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            জন্য (ধ্রুবক auto& itpair : m_scopeFuncs) {
                ধ্রুবক Scope_Func_তথ্য& sfd = itpair.second;
                ধ্রুবক AstScopeName* ধ্রুবক scopep = sfd.m_scopep;
                ধ্রুবক AstCFunc* ধ্রুবক funcp = sfd.m_cfuncp;
                ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = sfd.m_modp;
                যদি (!funcp->dpiExportImpl()) চালু;

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
    যদি (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        পূর্ণসংখ্যা table_গণক = 0;

        auto it = m_scopeVars.cbegin();
        যখন (it != m_scopeVars.cend()) {
            ধ্রুবক std::string scopeName = it->second.m_scopeName;
            ধ্রুবক AstScope* ধ্রুবক instScopep = it->second.m_scopep;
            ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = it->second.m_modp;
            ধ্রুবক std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            জন্য (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                ধ্রুবক Scope_Var_তথ্য& svd = it->second;
                ধ্রুবক AstScope* ধ্রুবক scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ধ্রুবক AstVar* ধ্রুবক varp = svd.m_varp;
                ধ্রুবক VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ধ্রুবক std::pair<যৌক্তিক, std::string> fc = isForceControlSignal(varp);
                যদি (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    চালু;
                }

                std::string row;
                ধ্রুবক TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                নির্বাচন (kind) {
                ক্ষেত্র TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    বিরতি;
                ক্ষেত্র TableEntryKind::FORCEABLE_RESIDUAL: {
                    ধ্রুবক std::string bounds = boundsString(dims);
                    residual.emplace_back(ঢোকাও_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    বিরতি;
                }
                ক্ষেত্র TableEntryKind::PLAIN_RESIDUAL: {
                    ধ্রুবক std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        ঢোকাও_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    বিরতি;
                }
                পূর্বনির্ধারিত: v3fatalSrc("Bad case");
                }
                যদি (kind == TableEntryKind::TABLE_ROW) চালু;
                যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), গ্রন্থি_UOr_Struct_DType)) {
                    যদি (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } অন্যথা যদি (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                যদি (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            যদি (!rows.empty()) {
                ধ্রুবক আকার_t row_গুনতি = rows.আকার_2();
                std::string চাবি;
                জন্য (ধ্রুবক std::string& r : rows) {
                    চাবি += r;
                    চাবি += '\0';
                }
                ধ্রুবক auto itt = tableByRows.find(চাবি);
                std::string tableName;
                যদি (itt != tableByRows.end()) {
                    tableName = itt->second;
                } অন্যথা {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(table_গণক++);
                    tableByRows.emplace(std::move(চাবি), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_গুনতি) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            জন্য (ধ্রুবক std::string& r : residual) stmts.add(r);
        }
    }

    ফেরত stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, সত্য);
    যদি (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    যদি (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + শীর্ষ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    যদি (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.আকার_2()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    জন্য (ধ্রুবক auto& itpair : m_scopeNames) {
        ধ্রুবক Scope_তথ্য& sd = itpair.second;
        ধ্রুবক std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    জন্য (ধ্রুবক ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        ধ্রুবক AstScope* ধ্রুবক scopep = itpair.first;
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = itpair.second;
        যদি (modp->is_শীর্ষ()) চালু;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    ফেরত stmts;
}

শূন্য EmitCSyms::emitSymImp(ধ্রুবক AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ধ্রুবক std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            সত্য, "Symbol table implementation internals",
                            [এই]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), সত্য, সত্য, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ধ্রুবক AstScope* ধ্রুবক শীর্ষ_Scopep = netlistp->শীর্ষ_Scopep()->scopep();
    ধ্রুবক std::string শীর্ষ_Inst = "vlSymsp->" + instName(শীর্ষ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    যদি (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + শীর্ষ_Inst + ", vlSymsp, namep);\n");
    জন্য (ধ্রুবক TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + শীর্ষ_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    যদি (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    যদি (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    যদি (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    যদি (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    জন্য (ধ্রুবক TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + শীর্ষ_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    যদি (v3Global.needTraceDumper()) {
        যদি (!optSystemC()) {
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

    যদি (v3Global.opt.savable()) {
        জন্য (ধ্রুবক যৌক্তিক& de : {মিথ্যা, সত্য}) {
            ধ্রুবক std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ধ্রুবক std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ধ্রুবক std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            যদি (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            জন্য (ধ্রুবক ScopeModPair& itpair : m_scopes) {
                ধ্রুবক AstScope* ধ্রুবক scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

শূন্য EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(শীর্ষ_Class_Name() + "__Dpi",
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
    জন্য (ধ্রুবক AstCFunc* ধ্রুবক nodep : m_dpis) {
        জন্য (ধ্রুবক Ast_গ্রন্থি* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            যদি (ধ্রুবক AstVar* ধ্রুবক portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    যদি (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    পূর্ণসংখ্যা firstExp = 0;
    পূর্ণসংখ্যা firstImp = 0;
    জন্য (ধ্রুবক AstCFunc* ধ্রুবক nodep : m_dpis) {
        যদি (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) চালু;

        ধ্রুবক std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        যদি (nodep->dpiExportDispatcher()) {
            যদি (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } অন্যথা {
            যদি (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        যদি (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } অন্যথা {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, মিথ্যা) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

শূন্য EmitCSyms::emitDpiStruct(ধ্রুবক Ast_গ্রন্থি_DType* dtypep, std::set<std::string>& namesr) {
    যদি (!dtypep->containsUnpackedStruct()) ফেরত;
    ধ্রুবক AstStructDType* ধ্রুবক sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ধ্রুবক std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    যদি (namesr.গুনতি(name)) ফেরত;
    // After the structures it contains
    জন্য (ধ্রুবক AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    যদি (name.empty()) ফেরত;  // Declared where used
    যদি (namesr.empty()) puts("// DPI TYPES\n");
    namesr.ঢোকাও(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

শূন্য EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(শীর্ষ_Class_Name() + "__Dpi", মিথ্যা, সত্য,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + শীর্ষ_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + শীর্ষ_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + শীর্ষ_Class_Name() + ".h\"\n");
    puts("\n");

    জন্য (ধ্রুবক AstCFunc* ধ্রুবক nodep : m_dpis) {
        যদি (!nodep->dpiExportDispatcher()) চালু;

        ধ্রুবক std::string name = nodep->name();
        ধ্রুবক std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + শীর্ষ_Class_Name() + "::" + name + "(");
        std::string comma;
        জন্য (ধ্রুবক Ast_গ্রন্থি* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (ধ্রুবক AstVar* ধ্রুবক portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isIO() && !portp->isFuncReturn()) {
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

শূন্য V3EmitC::emitcSyms(যৌক্তিক dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
