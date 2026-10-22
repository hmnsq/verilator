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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3EmitC.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3ExecGraph.h"
#समावेश "V3LanguageWords.h"
#समावेश "V3StackCount.h"
#समावेश "V3Stats.h"

#समावेश <algorithm>
#समावेश <cstring>
#समावेश <map>
#समावेश <set>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
अटल constexpr स्वतः symClassName = &EmitCUtil::symClassName;
अटल constexpr स्वतः शीर्ष_Class_Name = &EmitCUtil::शीर्ष_Class_Name;

श्रेणी EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    स्थिर VNUser1InUse m_inuser1;

    // TYPES
    काष्ठा Scope_तथ्य final {
        स्थिर Ast_गाँठ* m_nodep;  // Scope symbol-table entry corresponds to
        स्थिर std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        स्थिर std::string m_prettyName;  // Pretty (unprotected) name for display
        स्थिर std::string m_defName;  // Name registered for this scope
        स्थिर पूर्णांक m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_तथ्य(स्थिर Ast_गाँठ* nodep, स्थिर std::string& symName, स्थिर std::string& prettyName,
                  स्थिर std::string& defName, पूर्णांक timeunit, स्थिर std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    काष्ठा Iface_Ref_तथ्य final {
        स्थिर AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_तथ्य(स्थिर AstScope* scopep, स्थिर std::string& parentSym,
                     स्थिर std::string& suffix, स्थिर std::string& name,
                     स्थिर std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    काष्ठा Scope_Func_तथ्य final {
        स्थिर AstScopeName* स्थिर m_scopep;  // Scope the DPI export function is registered under
        स्थिर AstCFunc* स्थिर m_cfuncp;  // DPI export function
        स्थिर Ast_गाँठ_Module* स्थिर m_modp;  // Module containing the DPI export function
        Scope_Func_तथ्य(स्थिर AstScopeName* scopep, स्थिर AstCFunc* funcp, स्थिर Ast_गाँठ_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    काष्ठा Scope_Var_तथ्य final {
        स्थिर std::string m_scopeName;  // Full name of the scope containing the variable
        स्थिर std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        स्थिर AstVar* स्थिर m_varp;  // Public variable
        स्थिर Ast_गाँठ_Module* स्थिर m_modp;  // Module containing the variable
        स्थिर AstScope* स्थिर m_scopep;  // Scope containing the variable
        Scope_Var_तथ्य(स्थिर std::string& scopeName, स्थिर std::string& varBasePretty,
                     स्थिर AstVar* varp, स्थिर Ast_गाँठ_Module* modp, स्थिर AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<स्थिर std::string, Scope_तथ्य>;
    using ScopeModPair = std::pair<स्थिर AstScope*, Ast_गाँठ_Module*>;
    using ModVarPair = std::pair<स्थिर Ast_गाँठ_Module*, स्थिर AstVar*>;

    // Vars with more dims take the residual per-statement path.
    अटल constexpr पूर्णांक VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    स्थिर AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<स्थिर std::string, Scope_Func_तथ्य> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<स्थिर std::string, Scope_Var_तथ्य> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<Iface_Ref_तथ्य> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<स्थिर std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    पूर्णांक m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    पूर्णांक m_modCoverBins = 0;  // Per-module coverage bin number
    स्थिर bool m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    व्योम emitSymHdr();
    व्योम emitSymImpPreamble();
    व्योम emitVarTables();
    व्योम emitScopeHier(TextStmts& stmts, bool destroy);
    व्योम emitSymImp(स्थिर AstNetlist* netlistp);
    व्योम emitDpiHdr();
    व्योम emitDpiStruct(स्थिर Ast_गाँठ_DType* dtypep, std::set<std::string>& namesr);
    व्योम emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    अटल व्योम nameCheck(Ast_गाँठ* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        अगर (nodep->name().empty()) वापस;
        स्थिर AstCFunc* स्थिर cfuncp = VN_CAST(nodep, CFunc);
        अगर (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            स्थिर std::string rsvd = V3_Language_शब्दगण::isKeyword(nodep->name());
            अगर (rsvd != "") {
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

    अटल std::string scopeSymString(स्थिर std::string& scpname) {
        std::string out = scpname;
        std::string::आकार_type pos;
        जबतक ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        अगर (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        अगर (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        जबतक ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        जबतक ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        वापस out;
    }

    अटल string scopeDecodeIdentifier(स्थिर std::string& scpname) {
        std::string::आकार_type pos = std::string::npos;

        // Remove hierarchy
        माप_प्रकार i = 0;
        // always makes progress
        जबतक (i < scpname.लंबाई()) {
            अगर (scpname[i] == '\\') {
                जबतक (i < scpname.लंबाई() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } अन्यथा {
                जबतक (i < scpname.लंबाई() && scpname[i] != '.') ++i;
                अगर (i < scpname.लंबाई()) pos = i++;
            }
        }

        वापस pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    काष्ठा VarDims final {
        पूर्णांक pdim = 0;  // Packed array dims plus a ranged basic leaf
        पूर्णांक udim = 0;
        std::vector<std::pair<पूर्णांक, पूर्णांक>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<पूर्णांक, पूर्णांक>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<पूर्णांक, पूर्णांक>> flatten() स्थिर {
            std::vector<std::pair<पूर्णांक, पूर्णांक>> flat(unpackedLR);
            flat.जोड़ो(flat.end(), packedLR.cbegin(), packedLR.cend());
            वापस flat;
        }
    };
    अटल VarDims getVarDims(स्थिर Ast_गाँठ_DType* स्थिर मूल_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        क्रम (स्थिर Ast_गाँठ_DType* dtypep = मूल_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            अगर (स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(dtypep, गाँठ_Array_DType)) {
                अगर (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->बाँए(), adtypep->दाँए());
                    ++d.pdim;
                } अन्यथा {
                    d.unpackedLR.emplace_back(adtypep->बाँए(), adtypep->दाँए());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } अन्यथा {
                अगर (स्थिर AstBasicDType* स्थिर basicp = dtypep->basicp()) {
                    अगर (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->बाँए(), basicp->दाँए());
                        ++d.pdim;
                    }
                }
                अवरोध;  // Non-array leaf
            }
        }
        वापस d;
    }

    अटल VarDims dimsFor(स्थिर Scope_Var_तथ्य& svd) { वापस getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    अटल std::string boundsString(स्थिर VarDims& d) {
        std::string bounds;
        क्रम (स्थिर std::pair<पूर्णांक, पूर्णांक>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        वापस bounds;
    }

    अटल पूर्णांक getUnpackedElements(स्थिर Ast_गाँठ_DType* मूल_Dtypep) {
        पूर्णांक elements = 1;
        क्रम (स्थिर Ast_गाँठ_DType* dtypep = मूल_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(dtypep, UnpackArrayDType);
            अगर (!adtypep) अवरोध;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        वापस elements;
    }

    अटल bool needs_Emitted_Ent_आकार(स्थिर std::string& vlEnumType) {
        वापस vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    अटल std::pair<bool, std::string> isForceControlSignal(स्थिर AstVar* स्थिर signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        क्रम (स्थिर std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            स्थिर std::माप_प्रकार suffixPos = signalVarp->name().find(forceControlSuffix);
            स्थिर bool suffixFound = suffixPos != std::string::npos;
            स्थिर bool suffixIsAtEnd
                = suffixPos + forceControlSuffix.लंबाई() == signalVarp->name().लंबाई();
            अगर (!(suffixFound && suffixIsAtEnd)) जारी;
            स्थिर std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            वापस std::pair<bool, std::string>{true, baseSignalName};
        }
        वापस std::pair<bool, std::string>{false, ""};
    }

    अटल व्योम appendVarProperties(std::string& stmt, स्थिर std::string& vlEnumType,
                                    स्थिर std::string& vlEnumDir, स्थिर पूर्णांक udim, स्थिर पूर्णांक pdim,
                                    स्थिर std::string& bounds, स्थिर std::string& ent_आकार = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        अगर (!ent_आकार.empty()) {
            stmt += ", ";
            stmt += ent_आकार;
        } अन्यथा {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    अटल std::string memberVlEnumDir(स्थिर AstVar* स्थिर varp,
                                       स्थिर Ast_गाँठ_DType* स्थिर dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/true);
        स्थिर Ast_गाँठ_DType* स्थिर skipDTypep = dtypep->skipRefp();
        अगर (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        अगर (स्थिर AstBasicDType* स्थिर basicp = skipDTypep->basicp()) {
            अगर (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        वापस out + ')';
    }

    // Name of the instance member for the given scope
    अटल std::string instName(स्थिर AstScope* scopep) {
        वापस "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    अटल std::string जोड़ो_Var_Statement(स्थिर Scope_Var_तथ्य& svd, स्थिर AstScope* स्थिर scopep,
                                          स्थिर AstVar* स्थिर varp, स्थिर पूर्णांक udim, स्थिर पूर्णांक pdim,
                                          स्थिर std::string& bounds) {
        स्थिर std::string var_व्यंजक = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        स्थिर std::string vlEnumType = varp->vlEnumType();
        स्थिर bool needs_Ent_आकार = needs_Emitted_Ent_आकार(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_आकार ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        अगर (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_व्यंजक;
            stmt += "), false, ";
        } अन्यथा अगर (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_व्यंजक;
            stmt += ".c_str())), true, ";
        } अन्यथा {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_व्यंजक;
            stmt += "))), true, ";
        }

        स्थिर std::string ent_आकार = needs_Ent_आकार
                                        ? "sizeof(" + var_व्यंजक + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_आकार);
        वापस stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    क्रमागत श्रेणी TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(स्थिर Scope_Var_तथ्य& svd, स्थिर AstVar* स्थिर varp,
                                      स्थिर AstScope* स्थिर scopep,
                                      स्थिर std::string& modClassName, स्थिर VarDims& dims,
                                      std::string& rowOut) स्थिर {
        स्थिर पूर्णांक pdim = dims.pdim;
        स्थिर पूर्णांक udim = dims.udim;
        अगर (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            वापस TableEntryKind::FORCEABLE_RESIDUAL;
        }
        अगर (udim + pdim > VPI_TABLE_MAX_DIMS) वापस TableEntryKind::PLAIN_RESIDUAL;

        स्थिर std::string vlEnumType = varp->vlEnumType();
        अगर (needs_Emitted_Ent_आकार(vlEnumType))
            वापस TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        अगर (varp->isParam()) वापस TableEntryKind::PLAIN_RESIDUAL;

        स्थिर std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        स्थिर std::string member = varName(varp);
        स्थिर std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<पूर्णांक> dv;
        क्रम (स्थिर std::pair<पूर्णांक, पूर्णांक>& lr : dims.flatten()) {
            dv.धकेलो_back(lr.first);
            dv.धकेलो_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        क्रम (पूर्णांक i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            अगर (i) row += ", ";
            row += std::to_string(i < static_cast<पूर्णांक>(dv.आकार()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        वापस TableEntryKind::TABLE_ROW;
    }

    अटल std::string जोड़ो_DType_Var_Statement(स्थिर Scope_Var_तथ्य& svd,
                                               स्थिर AstScope* स्थिर scopep,
                                               स्थिर std::string& prettyName,
                                               स्थिर std::string& cName,
                                               स्थिर Ast_गाँठ_DType* स्थिर dtypep, स्थिर पूर्णांक udim,
                                               स्थिर पूर्णांक pdim, स्थिर std::string& bounds) {
        स्थिर std::string vlEnumType = dtypep->vlEnumType();
        स्थिर bool needs_Ent_आकार = needs_Emitted_Ent_आकार(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_आकार ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        स्थिर std::string var_व्यंजक = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        अगर (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_व्यंजक;
            stmt += "))), true, ";
        } अन्यथा {
            stmt += ", &(";
            stmt += var_व्यंजक;
            stmt += "), false, ";
        }
        स्थिर std::string ent_आकार
            = needs_Ent_आकार
                  ? "sizeof(" + var_व्यंजक + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_आकार);
        वापस stmt;
    }

    अटल व्योम addUOrStructMemberVars(std::vector<std::string>& stmts, स्थिर Scope_Var_तथ्य& svd,
                                       स्थिर AstScope* स्थिर scopep,
                                       स्थिर std::string& prettyPrefix, स्थिर std::string& cPrefix,
                                       स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep) {
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            स्थिर Ast_गाँठ_DType* स्थिर itemDTypep = itemp->dtypep();
            अगर (itemDTypep->vlEnumType().empty()) जारी;
            स्थिर std::string prettyName
                = prettyPrefix + "." + Ast_गाँठ::vpiName(itemp->shortName());
            स्थिर std::string cName = cPrefix + "." + itemp->nameProtect();
            स्थिर VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(जोड़ो_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर subp
                = VN_CAST(itemDTypep->skipRefp(), गाँठ_UOr_Struct_DType)) {
                अगर (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } अन्यथा अगर (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    अटल व्योम addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    स्थिर Scope_Var_तथ्य& svd,
                                                    स्थिर AstScope* स्थिर scopep,
                                                    स्थिर std::string& prettyPrefix,
                                                    स्थिर std::string& cPrefix,
                                                    स्थिर Ast_गाँठ_DType* स्थिर dtypep) {
        स्थिर Ast_गाँठ_DType* स्थिर skipDTypep = dtypep->skipRefp();
        अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } अन्यथा अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep
                   = VN_CAST(skipDTypep, गाँठ_UOr_Struct_DType)) {
            अगर (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    अटल व्योम addPackedMemberVars(std::vector<std::string>& stmts, स्थिर Scope_Var_तथ्य& svd,
                                    स्थिर AstScope* स्थिर scopep, स्थिर std::string& prettyPrefix,
                                    स्थिर std::string& cName, स्थिर std::string& vlEnumType,
                                    स्थिर std::string& ent_आकार,
                                    स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep,
                                    स्थिर पूर्णांक parentBitOffset, स्थिर bool needs_सूचकांक) {
        स्थिर std::string var_व्यंजक = "vlSymsp->" + instName(scopep) + "." + cName;
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            स्थिर Ast_गाँठ_DType* स्थिर itemDTypep = itemp->dtypep();
            स्थिर std::string prettyName
                = prettyPrefix + "." + Ast_गाँठ::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            स्थिर पूर्णांक bitOffset = parentBitOffset + itemp->lsb();
            स्थिर VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            अगर (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_व्यंजक + "))), true, ";
            } अन्यथा {
                stmt += ", &(" + var_व्यंजक + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            अगर (needs_सूचकांक) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += ent_आकार + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            स्थिर Ast_गाँठ_DType* subDTypep = itemDTypep->skipRefp();
            bool isArray = false;
            जबतक (स्थिर AstPackArrayDType* स्थिर adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = true;
            }
            अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर subp = VN_CAST(subDTypep, गाँठ_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_आकार,
                                    subp, bitOffset, needs_सूचकांक || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    अटल व्योम addPackedAggMemberVars(std::vector<std::string>& stmts, स्थिर Scope_Var_तथ्य& svd,
                                       स्थिर AstScope* स्थिर scopep,
                                       स्थिर std::string& prettyPrefix, स्थिर std::string& cName,
                                       स्थिर Ast_गाँठ_DType* स्थिर dtypep) {
        std::string elemCName = cName;
        स्थिर Ast_गाँठ_DType* elemDTypep = dtypep->skipRefp();
        bool isArray = false;
        जबतक (स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(elemDTypep, गाँठ_Array_DType)) {
            अगर (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = true;
        }
        स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep = VN_CAST(elemDTypep, गाँठ_UOr_Struct_DType);
        अगर (!sdtypep || !sdtypep->packed()) वापस;
        स्थिर std::string var_व्यंजक = "vlSymsp->" + instName(scopep) + "." + cName;
        स्थिर std::string ent_आकार
            = "sizeof(" + var_व्यंजक + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            ent_आकार, sdtypep, 0, isArray);
    }

    std::string जोड़ो_Forceable_Var_Statement(स्थिर Scope_Var_तथ्य& svd, स्थिर AstScope* स्थिर scopep,
                                            स्थिर AstVar* स्थिर varp, स्थिर पूर्णांक udim,
                                            स्थिर पूर्णांक pdim, स्थिर std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        स्थिर std::string var_व्यंजक = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        निश्चित(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_व्यंजक;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_व्यंजक + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        क्रम (स्थिर std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            स्थिर std::string enable_Signal_कुंजी
                = get_कुंजी_Name(scopep, varp->name() + forceControlSuffix);
            स्थिर std::map<स्थिर std::string, Scope_Var_तथ्य>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_कुंजी);

            अगर (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_कुंजी << "'.");
            }

            स्थिर Scope_Var_तथ्य& svd = itpair->second;
            स्थिर AstScope* स्थिर scopep = svd.m_scopep;
            स्थिर AstVar* स्थिर varp = svd.m_varp;
            स्थिर VarDims dims = dimsFor(svd);
            stmt
                += जोड़ो_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            अगर (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        वापस stmt;
    }

    अटल std::string get_कुंजी_Name(स्थिर AstScope* स्थिर scopep, स्थिर std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        अगर (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        स्थिर std::string::आकार_type dpos = whole.rfind("__DOT__");
        अगर (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        स्थिर std::string scpSym = scopeSymString(VName::dehash(scpName));
        वापस scpSym + " " + signal_name;
    }

    bool baseSignalIsValid(स्थिर AstScope* स्थिर scopep, स्थिर AstVar* स्थिर controlSignalVarp,
                           स्थिर std::string& baseSignalName) स्थिर {
        स्थिर std::string base_Signal_कुंजी = get_कुंजी_Name(scopep, baseSignalName);
        स्थिर std::map<स्थिर std::string, Scope_Var_तथ्य>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_कुंजी);
        अगर (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            वापस false;
        } अन्यथा {
            स्थिर AstVar* स्थिर baseSignalVarp = baseSignalIt->second.m_varp;
            अगर (m_scopeVars.गिनती(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                वापस false;
            }
            अगर (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                वापस false;
            }
        }
        वापस true;
    }

    bool baseSignalIsPublic(स्थिर AstScope* स्थिर scopep,
                            स्थिर std::string& baseSignalName) स्थिर {
        स्थिर std::string base_Signal_कुंजी = get_कुंजी_Name(scopep, baseSignalName);
        स्थिर std::map<स्थिर std::string, Scope_Var_तथ्य>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_कुंजी);
        अगर (baseSignalIt == m_scopeVars.end()) {
            वापस false;
        } अन्यथा {
            स्थिर AstVar* स्थिर baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            अगर (!baseSignalVarp->isSigPublic()) वापस false;
        }
        वापस true;
    }

    bool forceControlSignalsAreValid(स्थिर AstScope* स्थिर scopep,
                                     स्थिर AstVar* स्थिर baseSignalVarp) स्थिर {
        constexpr std::array<स्थिर अक्षर*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        वापस std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, यह](स्थिर std::string& forceControlSuffix) {
                स्थिर std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                स्थिर std::string control_Signal_कुंजी = get_कुंजी_Name(scopep, controlSignalName);
                स्थिर std::माप_प्रकार control_Signal_गिनती = m_scopeVars.गिनती(control_Signal_कुंजी);
                अगर (control_Signal_गिनती == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_कुंजी << "'.");
                    वापस false;
                }
                अगर (control_Signal_गिनती > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    वापस false;
                }
                वापस true;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    व्योम varHierarchyScopes(std::string scp) {

        std::string::आकार_type prd_pos = scp.rfind('.');
        std::string::आकार_type dot_pos = scp.rfind("__DOT__");

        जबतक (!scp.empty()) {
            स्थिर स्वतः scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            अगर ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                स्थिर स्वतः pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                अगर (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            अगर ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } अन्यथा {
                अगर (prd_pos == string::npos) अवरोध;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    व्योम addScopeVarEntry(स्थिर AstScope* स्थिर scopep, स्थिर Ast_गाँठ_Module* स्थिर modp,
                          स्थिर AstVar* स्थिर varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        अगर (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        स्थिर std::string::आकार_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        स्थिर std::string scpName = whole.substr(0, dpos);
        स्थिर std::string varBase = whole.substr(dpos + std::म_माप("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        स्थिर std::string varBasePretty = Ast_गाँठ::vpiName(VName::dehash(varBase));
        स्थिर std::string scpPretty = Ast_गाँठ::prettyName(VName::dehash(scpName));
        स्थिर std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        अगर (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    व्योम varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        क्रम (स्थिर ScopeModPair& smPair : m_scopes) {
            स्थिर AstScope* स्थिर scopep = smPair.first;
            स्थिर Ast_गाँठ_Module* स्थिर smodp = smPair.second;
            क्रम (स्थिर ModVarPair& mvPair : m_modVars) {
                स्थिर Ast_गाँठ_Module* स्थिर modp = mvPair.first;
                स्थिर AstVar* स्थिर varp = mvPair.second;
                अगर (modp != smodp) जारी;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    व्योम collectIfaceRefs(स्थिर AstScope* nodep) {
        स्थिर AstCell* स्थिर cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        अगर (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) वापस;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        स्थिर std::string path = Ast_गाँठ::vpiName(nodep->name());
        स्थिर std::string instName = Ast_गाँठ::vpiName(cellp->origName());
        UASSERT_OBJ(path.लंबाई() > instName.लंबाई() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        स्थिर std::string parentPath = path.substr(0, path.लंबाई() - instName.लंबाई());

        क्रम (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            स्थिर std::string refName = Ast_गाँठ::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            अगर (!VString::startsWith(refName, parentPath)) जारी;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_गाँठ::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    व्योम buildIfaceRefTable() {
        अगर (m_ifaceRefs.empty()) वापस;
        स्थिर std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](स्थिर Iface_Ref_तथ्य& a, स्थिर Iface_Ref_तथ्य& b) { वापस a.m_suffix < b.m_suffix; });
        क्रम (स्थिर Iface_Ref_तथ्य& ird : m_ifaceRefs) {
            स्थिर std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            अगर (m_scopeNames.find(scopeSym) == m_scopeNames.end()) जारी;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            स्थिर स्वतः parent = m_scopeNames.find(ird.m_parentSym);
            अगर (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_शब्दगण_If(ird.m_name, true));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दगण_If(ird.m_suffix, true));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दगण_If(ird.m_modportName, true));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        अगर (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    व्योम buildVpiHierarchy() {
        क्रम (स्थिर स्वतः& itpair : m_scopeNames) {
            स्थिर std::string symName = itpair.second.m_symName;
            std::string above = symName;
            अगर (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            जबतक (!above.empty()) {
                स्थिर std::string::आकार_type pos = above.rfind("__");
                अगर (pos == std::string::npos) अवरोध;
                above.resize(pos);
                अगर (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].धकेलो_back(symName);
                    अवरोध;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        अगर (v3Global.opt.vpi()) buildVpiHierarchy();

        अगर (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            क्रम (स्थिर स्वतः& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](स्थिर ScopeModPair& a, स्थिर ScopeModPair& b) {
                             वापस a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](स्थिर AstCFunc* ap, स्थिर AstCFunc* bp) {
                             अगर (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 वापस bp->dpiImportPrototype();
                             }
                             वापस ap->name() < bp->name();
                         });

        // Output
        अगर (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        अगर (v3Global.dpi()) {
            emitDpiHdr();
            अगर (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->isConstPool()) वापस;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCellInlineScope* nodep) override {
        स्थिर std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        स्थिर std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        स्थिर पूर्णांक timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_गाँठ::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    व्योम visit(AstScope* nodep) override {
        अगर (VN_IS(m_modp, Class)) वापस;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        अगर (v3Global.opt.vpi() && !nodep->is_शीर्ष()) {
            स्थिर std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            स्थिर पूर्णांक timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_गाँठ::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstScopeName* nodep) override {
        अगर (nodep->selfRelative()) वापस;  // Its absolute copy is under the scope
        स्थिर std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        स्थिर पूर्णांक timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        अगर (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } अन्यथा {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    व्योम visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        अगर ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    व्योम visit(Ast_गाँठ_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        अगर (!nodep->तथ्य_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_अंक(m_coverBins);
            m_coverBins += nodep->आकार();
            nodep->local_Bin_अंक(m_modCoverBins);
            m_modCoverBins += nodep->आकार();
        }
    }
    व्योम visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        अगर (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.धकेलो_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    व्योम visit(AstConst*) override {}
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    explicit EmitCSyms(AstNetlist* nodep, bool dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

व्योम EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    माला_दो("//\n");
    माला_दो("// Internal details; most calling programs do not need this header,\n");
    माला_दो("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    माला_दो("\n");
    ofp()->puts_Int_शीर्ष_Include();
    माला_दो("#include \"verilated.h\"\n");
    अगर (v3Global.needTraceDumper()) {
        क्रम (स्थिर string& base : v3Global.opt.traceSourceLangs())
            माला_दो("#include \"" + base + ".h\"\n");
    }
    अगर (v3Global.opt.usesProfiler()) माला_दो("#include \"verilated_profiler.h\"\n");

    माला_दो("\n// INCLUDE MODEL CLASS\n");
    माला_दो("\n#include \"" + शीर्ष_Class_Name() + ".h\"\n");

    माला_दो("\n// INCLUDE MODULE CLASSES\n");
    क्रम (Ast_गाँठ_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), गाँठ_Module);
        अगर (VN_IS(nodep, Class)) जारी;  // Class included earlier
        अगर (nodep->isConstPool()) जारी;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    अगर (v3Global.dpi()) {
        माला_दो("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        क्रम (स्थिर स्वतः& itpair : m_scopeFuncs) {
            स्थिर AstCFunc* स्थिर funcp = itpair.second.m_cfuncp;
            अगर (!funcp->dpiExportImpl()) जारी;
            स्थिर std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            स्थिर std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        क्रम (स्थिर std::string& type : types) माला_दो(type);
    }

    माला_दो("\n// SYMS CLASS (contains all model state)\n");
    माला_दो("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(false);  // public:

    माला_दो("// INTERNAL STATE\n");
    माला_दो(शीर्ष_Class_Name() + "* const __Vm_modelp;\n");

    अगर (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        माला_दो("bool __Vm_dumping = false;  // Dumping is active\n");
        माला_दो("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        माला_दो(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    अगर (v3Global.opt.trace()) {
        माला_दो("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        माला_दो("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    अगर (v3Global.hasEvents()) {
        अगर (v3Global.assignsEvents()) {
            माला_दो("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } अन्यथा {
            माला_दो("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    अगर (v3Global.hasClasses()) माला_दो("VlDeleter __Vm_deleter;\n");
    माला_दो("bool& __Vm_didInit;\n");
    अगर (v3Global.opt.systemC()) माला_दो("sc_core::sc_event __Vm_wakeEvent;\n");

    अगर (v3Global.opt.mtasks()) {
        माला_दो("\n// MULTI-THREADING\n");
        माला_दो("VlThreadPool* __Vm_threadPoolp;\n");
        माला_दो("bool __Vm_even_cycle__ico = false;\n");
        माला_दो("bool __Vm_even_cycle__act = false;\n");
        माला_दो("bool __Vm_even_cycle__nba = false;\n");
    }

    अगर (v3Global.opt.profExec()) {
        माला_दो("\n// EXECUTION PROFILING\n");
        माला_दो("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    अगर (v3Global.opt.profPgo()) {
        माला_दो("\n// PGO PROFILING\n");
        माला_दो("VlPgoProfiler<" + std::to_string(ExecMTask::अंक_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    माला_दो("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    माला_दो("union {\nstruct {\n");
    क्रम (स्थिर ScopeModPair& itpair : m_scopes) {
        स्थिर AstScope* स्थिर scopep = itpair.first;
        स्थिर Ast_गाँठ_Module* स्थिर modp = itpair.second;
        अगर (VN_IS(modp, Class)) जारी;
        ofp()->म_लिखो("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    माला_दो("} vlInst;\n};\n");

    अगर (m_coverBins) {
        माला_दो("\n// COVERAGE\n");
        माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        माला_दो(" __Vcoverage[");
        माला_दो(std::to_string(m_coverBins));
        माला_दो("];\n");
    }

    अगर (!m_scopeNames.empty()) {  // Scope names
        माला_दो("\n// SCOPE NAMES\n");
        क्रम (स्थिर स्वतः& itpair : m_scopeNames) {
            स्थिर Scope_तथ्य& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    अगर (v3Global.opt.vpi()) {
        माला_दो("\n// SCOPE HIERARCHY\n");
        माला_दो("VerilatedHierarchy __Vhier;\n");
    }

    माला_दो("\n// CONSTRUCTORS\n");
    माला_दो(symClassName() + "(VerilatedContext* contextp, const char* namep, " + शीर्ष_Class_Name()
         + "* modelp);\n");
    माला_दो("~" + symClassName() + "();\n");

    माला_दो("\n// METHODS\n");
    माला_दो("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    अगर (v3Global.hasEvents()) {
        अगर (v3Global.assignsEvents()) {
            माला_दो("void fireEvent(VlAssignableEvent& event) {\n");
        } अन्यथा {
            माला_दो("void fireEvent(VlEvent& event) {\n");
        }
        माला_दो("if (VL_LIKELY(!event.isTriggered())) {\n");
        अगर (v3Global.assignsEvents()) {
            माला_दो("__Vm_triggeredEvents.push_back(event);\n");
        } अन्यथा {
            माला_दो("__Vm_triggeredEvents.push_back(&event);\n");
        }
        माला_दो("}\n");
        माला_दो("event.fire();\n");
        माला_दो("}\n");
        माला_दो("void clearTriggeredEvents() {\n");
        अगर (v3Global.assignsEvents()) {
            माला_दो("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } अन्यथा {
            माला_दो("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        माला_दो("__Vm_triggeredEvents.clear();\n");
        माला_दो("}\n");
    }

    अगर (v3Global.needTraceDumper()) {
        अगर (!optSystemC()) माला_दो("void _traceDump();\n");
        माला_दो("void _traceDumpOpen();\n");
        माला_दो("void _traceDumpClose();\n");
    }

    अगर (v3Global.opt.savable()) {
        माला_दो("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        माला_दो("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    माला_दो("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

व्योम EmitCSyms::emitSymImpPreamble() {
    माला_दो("\n");

    // Includes
    माला_दो("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    माला_दो("\n");
    // Declarations for DPI Export implementation functions
    bool needs_New_पंक्ति = false;
    क्रम (स्थिर स्वतः& itpair : m_scopeFuncs) {
        स्थिर AstCFunc* स्थिर funcp = itpair.second.m_cfuncp;
        अगर (!funcp->dpiExportImpl()) जारी;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_पंक्ति = true;
    }
    अगर (needs_New_पंक्ति) माला_दो("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    क्रम (स्थिर ScopeModPair& itpair : m_scopes) {
        अगर (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    क्रम (स्थिर std::string& libName : libNames) {
        माला_दो("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        माला_दो("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    अगर (!libNames.empty()) माला_दो("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    अगर (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        क्रम (स्थिर स्वतः& kv : m_varTables) {
            माला_दो("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        अगर (!m_scopeTableRows.empty()) {
            माला_दो("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        अगर (!m_ifaceRefTableRows.empty()) {
            माला_दो("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        माला_दो("\n");
    }
}

व्योम EmitCSyms::emitVarTables() {
    अगर (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) वापस;
    माला_दो("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    माला_दो("#if defined(__GNUC__)\n");
    माला_दो("# pragma GCC diagnostic push\n");
    माला_दो("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    माला_दो("#endif\n");
    क्रम (स्थिर स्वतः& kv : m_varTables) {
        माला_दो("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        क्रम (स्थिर std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        माला_दो("};\n");
    }
    अगर (!m_scopeTableRows.empty()) {
        माला_दो("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        क्रम (स्थिर std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        माला_दो("};\n");
    }
    अगर (!m_ifaceRefTableRows.empty()) {
        माला_दो("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        क्रम (स्थिर std::string& row : m_ifaceRefTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        माला_दो("};\n");
    }
    माला_दो("#if defined(__GNUC__)\n");
    माला_दो("# pragma GCC diagnostic pop\n");
    माला_दो("#endif\n");
}

व्योम EmitCSyms::emitScopeHier(TextStmts& stmts, bool destroy) {
    अगर (!v3Global.opt.vpi()) वापस;

    अगर (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } अन्यथा {
        stmts.add("// Set up scope hierarchy");
    }

    स्थिर std::string method = destroy ? "remove" : "add";
    क्रम (स्थिर स्वतः& itpair : m_scopeNames) {
        अगर (itpair.first == "TOP") जारी;
        स्थिर Scope_तथ्य& sd = itpair.second;
        स्थिर std::string& name = sd.m_prettyName;
        स्थिर std::string& scopeType = sd.m_type;
        अगर (name.find('.') != string::npos) जारी;
        अगर (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") जारी;
        स्थिर std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    क्रम (स्थिर स्वतः& itpair : m_vpiScopeHierarchy) {
        स्थिर std::string fromName = scopeSymString(itpair.first);
        स्थिर std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        क्रम (स्थिर std::string& name : itpair.second) {
            स्थिर std::string toName = scopeSymString(name);
            स्थिर std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    अगर (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t ढेर_आकार = V3_ढेर_गिनती::गिनती(v3Global.rootp());
        अगर (v3Global.opt.debug_ढेर_Check()) ढेर_आकार += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", ढेर_आकार);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_आकार, ढेर_आकार + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(ढेर_आकार) + ");");
    }

    stmts.add("// Setup sub module instances");
    क्रम (स्थिर ScopeModPair& itpair : m_scopes) {
        स्थिर AstScope* स्थिर scopep = itpair.first;
        स्थिर Ast_गाँठ_Module* स्थिर modp = itpair.second;
        अगर (modp->is_शीर्ष()) जारी;
        स्थिर std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_शब्दगण_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    अगर (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        अगर (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        अगर (v3Global.opt.mtasks()) {
            v3Global.rootp()->शीर्ष_Modulep()->foreach([&](स्थिर AstExecGraph* execGraphp) {
                क्रम (स्थिर V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    स्थिर ExecMTask& mt = static_cast<स्थिर ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    अगर (!v3Global.rootp()->timeunit().isNone()) {
        स्थिर std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    अगर (!v3Global.rootp()->timeprecision().isNone()) {
        स्थिर std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    क्रम (स्थिर स्वतः& i : m_scopes) {
        स्थिर AstScope* स्थिर scopep = i.first;
        स्थिर Ast_गाँठ_Module* स्थिर modp = i.second;
        स्थिर AstScope* स्थिर abovep = scopep->aboveScopep();
        अगर (!abovep) जारी;

        स्थिर std::string protName = VIdProtect::protect_शब्दगण_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        अगर (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } अन्यथा {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    अगर (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        क्रम (स्थिर ScopeModPair& i : m_scopes) {
            स्थिर AstScope* स्थिर scopep = i.first;
            Ast_गाँठ_Module* स्थिर modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            स्थिर bool first = !modp->user1();
            modp->user1(true);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    क्रम (स्थिर ScopeModPair& i : m_scopes) {
        स्थिर AstScope* स्थिर scopep = i.first;
        स्थिर Ast_गाँठ_Module* स्थिर modp = i.second;
        अगर (!modp->verilatorLib()) जारी;
        स्थिर AstVar* hashp = nullptr;
        स्थिर AstVar* handlep = nullptr;
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var);
            अगर (!varp) जारी;
            अगर (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            अगर (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        स्थिर uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        अगर (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_शीर्ष_Name().आकार() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    अगर (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        स्थिर std::string symClass = symClassName();
        क्रम (स्थिर स्वतः& itpair : m_scopeNames) {
            स्थिर Scope_तथ्य& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दगण_If(sd.m_prettyName, true));
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
                  + std::to_string(m_scopeNames.आकार()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    buildIfaceRefTable();
    अगर (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.आकार()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, false);

    अगर (v3Global.dpi()) {
        क्रम (स्थिर std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            क्रम (स्थिर स्वतः& itpair : m_scopeFuncs) {
                स्थिर Scope_Func_तथ्य& sfd = itpair.second;
                स्थिर AstScopeName* स्थिर scopep = sfd.m_scopep;
                स्थिर AstCFunc* स्थिर funcp = sfd.m_cfuncp;
                स्थिर Ast_गाँठ_Module* स्थिर modp = sfd.m_modp;
                अगर (!funcp->dpiExportImpl()) जारी;

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
    अगर (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        पूर्णांक table_गणक = 0;

        स्वतः it = m_scopeVars.cbegin();
        जबतक (it != m_scopeVars.cend()) {
            स्थिर std::string scopeName = it->second.m_scopeName;
            स्थिर AstScope* स्थिर instScopep = it->second.m_scopep;
            स्थिर Ast_गाँठ_Module* स्थिर modp = it->second.m_modp;
            स्थिर std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            क्रम (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                स्थिर Scope_Var_तथ्य& svd = it->second;
                स्थिर AstScope* स्थिर scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                स्थिर AstVar* स्थिर varp = svd.m_varp;
                स्थिर VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                स्थिर std::pair<bool, std::string> fc = isForceControlSignal(varp);
                अगर (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    जारी;
                }

                std::string row;
                स्थिर TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                चयन (kind) {
                हाल TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    अवरोध;
                हाल TableEntryKind::FORCEABLE_RESIDUAL: {
                    स्थिर std::string bounds = boundsString(dims);
                    residual.emplace_back(जोड़ो_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    अवरोध;
                }
                हाल TableEntryKind::PLAIN_RESIDUAL: {
                    स्थिर std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        जोड़ो_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    अवरोध;
                }
                शेष: v3fatalSrc("Bad case");
                }
                अगर (kind == TableEntryKind::TABLE_ROW) जारी;
                अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), गाँठ_UOr_Struct_DType)) {
                    अगर (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } अन्यथा अगर (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                अगर (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            अगर (!rows.empty()) {
                स्थिर माप_प्रकार row_गिनती = rows.आकार();
                std::string कुंजी;
                क्रम (स्थिर std::string& r : rows) {
                    कुंजी += r;
                    कुंजी += '\0';
                }
                स्थिर स्वतः itt = tableByRows.find(कुंजी);
                std::string tableName;
                अगर (itt != tableByRows.end()) {
                    tableName = itt->second;
                } अन्यथा {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(table_गणक++);
                    tableByRows.emplace(std::move(कुंजी), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_गिनती) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            क्रम (स्थिर std::string& r : residual) stmts.add(r);
        }
    }

    वापस stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, true);
    अगर (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    अगर (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + शीर्ष_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    अगर (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.आकार()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    क्रम (स्थिर स्वतः& itpair : m_scopeNames) {
        स्थिर Scope_तथ्य& sd = itpair.second;
        स्थिर std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    क्रम (स्थिर ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        स्थिर AstScope* स्थिर scopep = itpair.first;
        स्थिर Ast_गाँठ_Module* स्थिर modp = itpair.second;
        अगर (modp->is_शीर्ष()) जारी;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    वापस stmts;
}

व्योम EmitCSyms::emitSymImp(स्थिर AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    स्थिर std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            true, "Symbol table implementation internals",
                            [यह]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), true, true, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    स्थिर AstScope* स्थिर शीर्ष_Scopep = netlistp->शीर्ष_Scopep()->scopep();
    स्थिर std::string शीर्ष_Inst = "vlSymsp->" + instName(शीर्ष_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    अगर (!splitCtorDtorDecls.first.empty()) माला_दो("\n" + splitCtorDtorDecls.first + "\n");
    माला_दो("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    माला_दो("// Setup top module instance\n");
    माला_दो("VL_INPLACE_CONSTRUCT(" + शीर्ष_Inst + ", vlSymsp, namep);\n");
    क्रम (स्थिर TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    माला_दो("}\n");

    // Constructor
    माला_दो("\n");
    माला_दो(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + शीर्ष_Class_Name() + "* modelp)\n");
    माला_दो("    : VerilatedSyms{contextp}\n");
    माला_दो("    // Setup internal state of the Syms class\n");
    माला_दो("    , __Vm_modelp{modelp}\n");
    माला_दो("    , __Vm_didInit{modelp->m_didInit}\n");
    अगर (v3Global.opt.mtasks()) {
        माला_दो("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    अगर (v3Global.opt.profExec()) {
        माला_दो("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    अगर (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        माला_दो("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    माला_दो("{\n");
    माला_दो(symClassName() + "__ctor(this, namep);\n");
    माला_दो("}\n");

    // Destructor contents, in a loose function, see constructor
    अगर (!splitCtorDtorDecls.second.empty()) माला_दो("\n" + splitCtorDtorDecls.second + "\n");
    माला_दो("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    क्रम (स्थिर TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    माला_दो("// Tear down top module instance\n");
    माला_दो("VL_INPLACE_DESTRUCT(" + शीर्ष_Inst + ");\n");
    माला_दो("}\n");

    // Destructor
    माला_दो("\n");
    माला_दो(symClassName() + "::~" + symClassName() + "() {\n");
    माला_दो(symClassName() + "__dtor(this);\n");
    माला_दो("}\n");

    // Methods
    अगर (v3Global.needTraceDumper()) {
        अगर (!optSystemC()) {
            माला_दो("\nvoid " + symClassName() + "::_traceDump() {\n");
            माला_दो("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
            // Caller checked for __Vm_dumperp non-nullptr
            माला_दो("__Vm_dumperp->dump(VL_TIME_Q());\n");
            माला_दो("}\n");
        }

        माला_दो("\nvoid " + symClassName() + "::_traceDumpOpen() {\n");
        माला_दो("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
        माला_दो("if (VL_UNLIKELY(!__Vm_dumperp)) {\n");
        माला_दो("__Vm_dumperp = new " + v3Global.opt.traceClassLang() + "();\n");
        माला_दो("__Vm_modelp->trace(__Vm_dumperp, 0, 0);\n");
        माला_दो("const std::string dumpfile = _vm_contextp__->dumpfileCheck();\n");
        माला_दो("__Vm_dumperp->open(dumpfile.c_str());\n");
        माला_दो("__Vm_dumping = true;\n");
        माला_दो("}\n");
        माला_दो("}\n");

        माला_दो("\nvoid " + symClassName() + "::_traceDumpClose() {\n");
        माला_दो("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
        माला_दो("__Vm_dumping = false;\n");
        माला_दो("VL_DO_CLEAR(delete __Vm_dumperp, __Vm_dumperp = nullptr);\n");
        माला_दो("}\n");
    }

    अगर (v3Global.opt.savable()) {
        क्रम (स्थिर bool& de : {false, true}) {
            स्थिर std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            स्थिर std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            स्थिर std::string op = de ? ">>" : "<<";
            माला_दो("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            माला_दो("// Internal state\n");
            अगर (v3Global.opt.trace()) माला_दो("os" + op + "__Vm_activity;\n");
            माला_दो("os " + op + " __Vm_didInit;\n");
            माला_दो("// Module instance state\n");
            क्रम (स्थिर ScopeModPair& itpair : m_scopes) {
                स्थिर AstScope* स्थिर scopep = itpair.first;
                माला_दो(instName(scopep) + "." + funcname + "(os);\n");
            }
            माला_दो("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

व्योम EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(शीर्ष_Class_Name() + "__Dpi",
                            "Prototypes for DPI import and export functions.");
    माला_दो("//\n");
    माला_दो("// Verilator includes this file in all generated .cpp files that use DPI functions.\n");
    माला_दो("// Manually include this file where DPI .c import functions are declared to ensure\n");
    माला_दो("// the C functions match the expectations of the DPI imports.\n");

    ofp()->putsGuard();

    माला_दो("\n");
    माला_दो("#include \"svdpi.h\"\n");
    माला_दो("\n");

    // Unpacked structures passed as C structures (IEEE 1800-2023 H.7.8)
    std::set<std::string> structNames;
    क्रम (स्थिर AstCFunc* स्थिर nodep : m_dpis) {
        क्रम (स्थिर Ast_गाँठ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            अगर (स्थिर AstVar* स्थिर portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    अगर (!structNames.empty()) माला_दो("\n");

    माला_दो("#ifdef __cplusplus\n");
    माला_दो("extern \"C\" {\n");
    माला_दो("#endif\n");
    माला_दो("\n");

    पूर्णांक firstExp = 0;
    पूर्णांक firstImp = 0;
    क्रम (स्थिर AstCFunc* स्थिर nodep : m_dpis) {
        अगर (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) जारी;

        स्थिर std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        अगर (nodep->dpiExportDispatcher()) {
            अगर (!firstExp++) माला_दो("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } अन्यथा {
            अगर (!firstImp++) माला_दो("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        अगर (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } अन्यथा {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, false) + ");\n");
        }
    }

    माला_दो("\n");
    माला_दो("#ifdef __cplusplus\n");
    माला_दो("}\n");
    माला_दो("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

व्योम EmitCSyms::emitDpiStruct(स्थिर Ast_गाँठ_DType* dtypep, std::set<std::string>& namesr) {
    अगर (!dtypep->containsUnpackedStruct()) वापस;
    स्थिर AstStructDType* स्थिर sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    स्थिर std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    अगर (namesr.गिनती(name)) वापस;
    // After the structures it contains
    क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    अगर (name.empty()) वापस;  // Declared where used
    अगर (namesr.empty()) माला_दो("// DPI TYPES\n");
    namesr.जोड़ो(name);
    // Guarded, so the user, or another model, may declare it instead
    माला_दो("#ifndef VL_DPITYPE_" + name + "_\n");
    माला_दो("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    माला_दो("#endif\n");
}

//######################################################################

व्योम EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(शीर्ष_Class_Name() + "__Dpi", false, true,
                            "Implementation of DPI export functions");
    माला_दो("//\n");
    माला_दो("// Verilator compiles this file in when DPI functions are used.\n");
    माला_दो("// If you have multiple Verilated designs with the same DPI exported\n");
    माला_दो("// function names, you will get multiple definition link errors from here.\n");
    माला_दो("// This is an unfortunate result of the DPI specification.\n");
    माला_दो("// To solve this, either\n");
    माला_दो("//    1. Call " + शीर्ष_Class_Name() + "::{export_function} instead,\n");
    माला_दो("//       and do not even bother to compile this file\n");
    माला_दो("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    माला_दो("//       and #ifdefs already inserted here will sort everything out.\n");
    माला_दो("\n");

    माला_दो("#include \"" + शीर्ष_Class_Name() + "__Dpi.h\"\n");
    माला_दो("#include \"" + शीर्ष_Class_Name() + ".h\"\n");
    माला_दो("\n");

    क्रम (स्थिर AstCFunc* स्थिर nodep : m_dpis) {
        अगर (!nodep->dpiExportDispatcher()) जारी;

        स्थिर std::string name = nodep->name();
        स्थिर std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        माला_दो("#ifndef VL_DPIDECL_" + name + "_\n");
        माला_दो("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        माला_दो("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + शीर्ष_Class_Name() + "::" + name + "(");
        std::string comma;
        क्रम (स्थिर Ast_गाँठ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (स्थिर AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isIO() && !portp->isFuncReturn()) {
                    माला_दो(comma);
                    comma = ", ";
                    putns(portp, portp->name());
                }
            }
        }
        माला_दो(");\n");
        माला_दो("}\n");
        माला_दो("#endif\n");
        माला_दो("\n");
    }
    closeOutputFile();
}

//######################################################################
// EmitC class functions

व्योम V3EmitC::emitcSyms(bool dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
