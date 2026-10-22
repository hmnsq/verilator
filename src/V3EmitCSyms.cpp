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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3ExecGraph.h"
#अन्तर्भाव "V3LanguageWords.h"
#अन्तर्भाव "V3StackCount.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cstring>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
स्थिर constexpr स्वतः symClassName = &EmitCUtil::symClassName;
स्थिर constexpr स्वतः शीर्ष_Class_Name = &EmitCUtil::शीर्ष_Class_Name;

वर्ग EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    अचर VNUser1InUse m_inuser1;

    // TYPES
    संरचना ScopeData final {
        अचर Ast_गाँठो* m_nodep;  // Scope symbol-table entry corresponds to
        अचर std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        अचर std::string m_prettyName;  // Pretty (unprotected) name for display
        अचर std::string m_defName;  // Name registered for this scope
        अचर पूर्णाङ्क m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(अचर Ast_गाँठो* nodep, अचर std::string& symName, अचर std::string& prettyName,
                  अचर std::string& defName, पूर्णाङ्क timeunit, अचर std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    संरचना IfaceRefData final {
        अचर AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(अचर AstScope* scopep, अचर std::string& parentSym,
                     अचर std::string& suffix, अचर std::string& name,
                     अचर std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    संरचना ScopeFuncData final {
        अचर AstScopeName* अचर m_scopep;  // Scope the DPI export function is registered under
        अचर AstCFunc* अचर m_cfuncp;  // DPI export function
        अचर Ast_गाँठो_Module* अचर m_modp;  // Module containing the DPI export function
        ScopeFuncData(अचर AstScopeName* scopep, अचर AstCFunc* funcp, अचर Ast_गाँठो_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    संरचना ScopeVarData final {
        अचर std::string m_scopeName;  // Full name of the scope containing the variable
        अचर std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        अचर AstVar* अचर m_varp;  // Public variable
        अचर Ast_गाँठो_Module* अचर m_modp;  // Module containing the variable
        अचर AstScope* अचर m_scopep;  // Scope containing the variable
        ScopeVarData(अचर std::string& scopeName, अचर std::string& varBasePretty,
                     अचर AstVar* varp, अचर Ast_गाँठो_Module* modp, अचर AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<अचर std::string, ScopeData>;
    using ScopeModPair = std::pair<अचर AstScope*, Ast_गाँठो_Module*>;
    using ModVarPair = std::pair<अचर Ast_गाँठो_Module*, अचर AstVar*>;

    // Vars with more dims take the residual per-statement path.
    स्थिर constexpr पूर्णाङ्क VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    अचर AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<अचर std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<अचर std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<अचर std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    पूर्णाङ्क m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    पूर्णाङ्क m_modCoverBins = 0;  // Per-module coverage bin number
    अचर तार्किक m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    शून्य emitSymHdr();
    शून्य emitSymImpPreamble();
    शून्य emitVarTables();
    शून्य emitScopeHier(TextStmts& stmts, तार्किक destroy);
    शून्य emitSymImp(अचर AstNetlist* netlistp);
    शून्य emitDpiHdr();
    शून्य emitDpiStruct(अचर Ast_गाँठो_DType* dtypep, std::set<std::string>& namesr);
    शून्य emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    स्थिर शून्य nameCheck(Ast_गाँठो* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        यदि (nodep->name().empty()) फर्काऊ;
        अचर AstCFunc* अचर cfuncp = VN_CAST(nodep, CFunc);
        यदि (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            अचर std::string rsvd = V3_Language_शब्दहरू::isKeyword(nodep->name());
            यदि (rsvd != "") {
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

    स्थिर std::string scopeSymString(अचर std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        जबसम्म ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        यदि (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        यदि (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        जबसम्म ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        जबसम्म ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        फर्काऊ out;
    }

    स्थिर string scopeDecodeIdentifier(अचर std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        माप_प्रकार i = 0;
        // always makes progress
        जबसम्म (i < scpname.length()) {
            यदि (scpname[i] == '\\') {
                जबसम्म (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } अन्यथा {
                जबसम्म (i < scpname.length() && scpname[i] != '.') ++i;
                यदि (i < scpname.length()) pos = i++;
            }
        }

        फर्काऊ pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    संरचना VarDims final {
        पूर्णाङ्क pdim = 0;  // Packed array dims plus a ranged basic leaf
        पूर्णाङ्क udim = 0;
        std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> flatten() अचर {
            std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> flat(unpackedLR);
            flat.घुसाउ(flat.end(), packedLR.cbegin(), packedLR.cend());
            फर्काऊ flat;
        }
    };
    स्थिर VarDims getVarDims(अचर Ast_गाँठो_DType* अचर जरा_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        लागि (अचर Ast_गाँठो_DType* dtypep = जरा_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            यदि (अचर Ast_गाँठो_Array_DType* अचर adtypep = VN_CAST(dtypep, गाँठो_Array_DType)) {
                यदि (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->बायाँ(), adtypep->दायाँ());
                    ++d.pdim;
                } अन्यथा {
                    d.unpackedLR.emplace_back(adtypep->बायाँ(), adtypep->दायाँ());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } अन्यथा {
                यदि (अचर AstBasicDType* अचर basicp = dtypep->basicp()) {
                    यदि (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->बायाँ(), basicp->दायाँ());
                        ++d.pdim;
                    }
                }
                रोक;  // Non-array leaf
            }
        }
        फर्काऊ d;
    }

    स्थिर VarDims dimsFor(अचर ScopeVarData& svd) { फर्काऊ getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    स्थिर std::string boundsString(अचर VarDims& d) {
        std::string bounds;
        लागि (अचर std::pair<पूर्णाङ्क, पूर्णाङ्क>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        फर्काऊ bounds;
    }

    स्थिर पूर्णाङ्क getUnpackedElements(अचर Ast_गाँठो_DType* जरा_Dtypep) {
        पूर्णाङ्क elements = 1;
        लागि (अचर Ast_गाँठो_DType* dtypep = जरा_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            अचर AstUnpackArrayDType* अचर adtypep = VN_CAST(dtypep, UnpackArrayDType);
            यदि (!adtypep) रोक;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        फर्काऊ elements;
    }

    स्थिर तार्किक needsEmittedEntSize(अचर std::string& vlEnumType) {
        फर्काऊ vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    स्थिर std::pair<तार्किक, std::string> isForceControlSignal(अचर AstVar* अचर signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        लागि (अचर std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            अचर std::माप_प्रकार suffixPos = signalVarp->name().find(forceControlSuffix);
            अचर तार्किक suffixFound = suffixPos != std::string::npos;
            अचर तार्किक suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            यदि (!(suffixFound && suffixIsAtEnd)) जारी;
            अचर std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            फर्काऊ std::pair<तार्किक, std::string>{सत्य, baseSignalName};
        }
        फर्काऊ std::pair<तार्किक, std::string>{असत्य, ""};
    }

    स्थिर शून्य appendVarProperties(std::string& stmt, अचर std::string& vlEnumType,
                                    अचर std::string& vlEnumDir, अचर पूर्णाङ्क udim, अचर पूर्णाङ्क pdim,
                                    अचर std::string& bounds, अचर std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        यदि (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } अन्यथा {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    स्थिर std::string memberVlEnumDir(अचर AstVar* अचर varp,
                                       अचर Ast_गाँठो_DType* अचर dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/सत्य);
        अचर Ast_गाँठो_DType* अचर skipDTypep = dtypep->skipRefp();
        यदि (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        यदि (अचर AstBasicDType* अचर basicp = skipDTypep->basicp()) {
            यदि (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        फर्काऊ out + ')';
    }

    // Name of the instance member for the given scope
    स्थिर std::string instName(अचर AstScope* scopep) {
        फर्काऊ "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    स्थिर std::string घुसाउ_Var_Statement(अचर ScopeVarData& svd, अचर AstScope* अचर scopep,
                                          अचर AstVar* अचर varp, अचर पूर्णाङ्क udim, अचर पूर्णाङ्क pdim,
                                          अचर std::string& bounds) {
        अचर std::string var_अभिव्यक्ति = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        अचर std::string vlEnumType = varp->vlEnumType();
        अचर तार्किक needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        यदि (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_अभिव्यक्ति;
            stmt += "), false, ";
        } अन्यथा यदि (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_अभिव्यक्ति;
            stmt += ".c_str())), true, ";
        } अन्यथा {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_अभिव्यक्ति;
            stmt += "))), true, ";
        }

        अचर std::string entSize = needsEntSize
                                        ? "sizeof(" + var_अभिव्यक्ति + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        फर्काऊ stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    गणना वर्ग TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(अचर ScopeVarData& svd, अचर AstVar* अचर varp,
                                      अचर AstScope* अचर scopep,
                                      अचर std::string& modClassName, अचर VarDims& dims,
                                      std::string& rowOut) अचर {
        अचर पूर्णाङ्क pdim = dims.pdim;
        अचर पूर्णाङ्क udim = dims.udim;
        यदि (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            फर्काऊ TableEntryKind::FORCEABLE_RESIDUAL;
        }
        यदि (udim + pdim > VPI_TABLE_MAX_DIMS) फर्काऊ TableEntryKind::PLAIN_RESIDUAL;

        अचर std::string vlEnumType = varp->vlEnumType();
        यदि (needsEmittedEntSize(vlEnumType))
            फर्काऊ TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        यदि (varp->isParam()) फर्काऊ TableEntryKind::PLAIN_RESIDUAL;

        अचर std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        अचर std::string member = varName(varp);
        अचर std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<पूर्णाङ्क> dv;
        लागि (अचर std::pair<पूर्णाङ्क, पूर्णाङ्क>& lr : dims.flatten()) {
            dv.थप_back(lr.first);
            dv.थप_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        लागि (पूर्णाङ्क i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            यदि (i) row += ", ";
            row += std::to_string(i < static_cast<पूर्णाङ्क>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        फर्काऊ TableEntryKind::TABLE_ROW;
    }

    स्थिर std::string घुसाउ_DType_Var_Statement(अचर ScopeVarData& svd,
                                               अचर AstScope* अचर scopep,
                                               अचर std::string& prettyName,
                                               अचर std::string& cName,
                                               अचर Ast_गाँठो_DType* अचर dtypep, अचर पूर्णाङ्क udim,
                                               अचर पूर्णाङ्क pdim, अचर std::string& bounds) {
        अचर std::string vlEnumType = dtypep->vlEnumType();
        अचर तार्किक needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        अचर std::string var_अभिव्यक्ति = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        यदि (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_अभिव्यक्ति;
            stmt += "))), true, ";
        } अन्यथा {
            stmt += ", &(";
            stmt += var_अभिव्यक्ति;
            stmt += "), false, ";
        }
        अचर std::string entSize
            = needsEntSize
                  ? "sizeof(" + var_अभिव्यक्ति + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        फर्काऊ stmt;
    }

    स्थिर शून्य addUOrStructMemberVars(std::vector<std::string>& stmts, अचर ScopeVarData& svd,
                                       अचर AstScope* अचर scopep,
                                       अचर std::string& prettyPrefix, अचर std::string& cPrefix,
                                       अचर Ast_गाँठो_UOr_Struct_DType* अचर sdtypep) {
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            अचर Ast_गाँठो_DType* अचर itemDTypep = itemp->dtypep();
            यदि (itemDTypep->vlEnumType().empty()) जारी;
            अचर std::string prettyName
                = prettyPrefix + "." + Ast_गाँठो::vpiName(itemp->shortName());
            अचर std::string cName = cPrefix + "." + itemp->nameProtect();
            अचर VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(घुसाउ_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            यदि (अचर Ast_गाँठो_UOr_Struct_DType* अचर subp
                = VN_CAST(itemDTypep->skipRefp(), गाँठो_UOr_Struct_DType)) {
                यदि (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } अन्यथा यदि (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    स्थिर शून्य addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    अचर ScopeVarData& svd,
                                                    अचर AstScope* अचर scopep,
                                                    अचर std::string& prettyPrefix,
                                                    अचर std::string& cPrefix,
                                                    अचर Ast_गाँठो_DType* अचर dtypep) {
        अचर Ast_गाँठो_DType* अचर skipDTypep = dtypep->skipRefp();
        यदि (अचर AstUnpackArrayDType* अचर adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } अन्यथा यदि (अचर Ast_गाँठो_UOr_Struct_DType* अचर sdtypep
                   = VN_CAST(skipDTypep, गाँठो_UOr_Struct_DType)) {
            यदि (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    स्थिर शून्य addPackedMemberVars(std::vector<std::string>& stmts, अचर ScopeVarData& svd,
                                    अचर AstScope* अचर scopep, अचर std::string& prettyPrefix,
                                    अचर std::string& cName, अचर std::string& vlEnumType,
                                    अचर std::string& entSize,
                                    अचर Ast_गाँठो_UOr_Struct_DType* अचर sdtypep,
                                    अचर पूर्णाङ्क parentBitOffset, अचर तार्किक needsIndex) {
        अचर std::string var_अभिव्यक्ति = "vlSymsp->" + instName(scopep) + "." + cName;
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            अचर Ast_गाँठो_DType* अचर itemDTypep = itemp->dtypep();
            अचर std::string prettyName
                = prettyPrefix + "." + Ast_गाँठो::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            अचर पूर्णाङ्क bitOffset = parentBitOffset + itemp->lsb();
            अचर VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            यदि (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_अभिव्यक्ति + "))), true, ";
            } अन्यथा {
                stmt += ", &(" + var_अभिव्यक्ति + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            यदि (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            अचर Ast_गाँठो_DType* subDTypep = itemDTypep->skipRefp();
            तार्किक isArray = असत्य;
            जबसम्म (अचर AstPackArrayDType* अचर adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = सत्य;
            }
            यदि (अचर Ast_गाँठो_UOr_Struct_DType* अचर subp = VN_CAST(subDTypep, गाँठो_UOr_Struct_DType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    स्थिर शून्य addPackedAggMemberVars(std::vector<std::string>& stmts, अचर ScopeVarData& svd,
                                       अचर AstScope* अचर scopep,
                                       अचर std::string& prettyPrefix, अचर std::string& cName,
                                       अचर Ast_गाँठो_DType* अचर dtypep) {
        std::string elemCName = cName;
        अचर Ast_गाँठो_DType* elemDTypep = dtypep->skipRefp();
        तार्किक isArray = असत्य;
        जबसम्म (अचर Ast_गाँठो_Array_DType* अचर adtypep = VN_CAST(elemDTypep, गाँठो_Array_DType)) {
            यदि (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = सत्य;
        }
        अचर Ast_गाँठो_UOr_Struct_DType* अचर sdtypep = VN_CAST(elemDTypep, गाँठो_UOr_Struct_DType);
        यदि (!sdtypep || !sdtypep->packed()) फर्काऊ;
        अचर std::string var_अभिव्यक्ति = "vlSymsp->" + instName(scopep) + "." + cName;
        अचर std::string entSize
            = "sizeof(" + var_अभिव्यक्ति + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string घुसाउ_Forceable_Var_Statement(अचर ScopeVarData& svd, अचर AstScope* अचर scopep,
                                            अचर AstVar* अचर varp, अचर पूर्णाङ्क udim,
                                            अचर पूर्णाङ्क pdim, अचर std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        अचर std::string var_अभिव्यक्ति = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        प्रतिज्ञा(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_अभिव्यक्ति;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_अभिव्यक्ति + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        लागि (अचर std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            अचर std::string enable_Signal_साँचो
                = get_साँचो_Name(scopep, varp->name() + forceControlSuffix);
            अचर std::map<अचर std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_साँचो);

            यदि (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_साँचो << "'.");
            }

            अचर ScopeVarData& svd = itpair->second;
            अचर AstScope* अचर scopep = svd.m_scopep;
            अचर AstVar* अचर varp = svd.m_varp;
            अचर VarDims dims = dimsFor(svd);
            stmt
                += घुसाउ_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            यदि (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        फर्काऊ stmt;
    }

    स्थिर std::string get_साँचो_Name(अचर AstScope* अचर scopep, अचर std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        यदि (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        अचर std::string::size_type dpos = whole.rfind("__DOT__");
        यदि (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        अचर std::string scpSym = scopeSymString(VName::dehash(scpName));
        फर्काऊ scpSym + " " + signal_name;
    }

    तार्किक baseSignalIsValid(अचर AstScope* अचर scopep, अचर AstVar* अचर controlSignalVarp,
                           अचर std::string& baseSignalName) अचर {
        अचर std::string base_Signal_साँचो = get_साँचो_Name(scopep, baseSignalName);
        अचर std::map<अचर std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_साँचो);
        यदि (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            फर्काऊ असत्य;
        } अन्यथा {
            अचर AstVar* अचर baseSignalVarp = baseSignalIt->second.m_varp;
            यदि (m_scopeVars.गन्ती(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                फर्काऊ असत्य;
            }
            यदि (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                फर्काऊ असत्य;
            }
        }
        फर्काऊ सत्य;
    }

    तार्किक baseSignalIsPublic(अचर AstScope* अचर scopep,
                            अचर std::string& baseSignalName) अचर {
        अचर std::string base_Signal_साँचो = get_साँचो_Name(scopep, baseSignalName);
        अचर std::map<अचर std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(base_Signal_साँचो);
        यदि (baseSignalIt == m_scopeVars.end()) {
            फर्काऊ असत्य;
        } अन्यथा {
            अचर AstVar* अचर baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            यदि (!baseSignalVarp->isSigPublic()) फर्काऊ असत्य;
        }
        फर्काऊ सत्य;
    }

    तार्किक forceControlSignalsAreValid(अचर AstScope* अचर scopep,
                                     अचर AstVar* अचर baseSignalVarp) अचर {
        constexpr std::array<अचर वर्ण*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        फर्काऊ std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, एतत्](अचर std::string& forceControlSuffix) {
                अचर std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                अचर std::string control_Signal_साँचो = get_साँचो_Name(scopep, controlSignalName);
                अचर std::माप_प्रकार control_Signal_गन्ती = m_scopeVars.गन्ती(control_Signal_साँचो);
                यदि (control_Signal_गन्ती == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_साँचो << "'.");
                    फर्काऊ असत्य;
                }
                यदि (control_Signal_गन्ती > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    फर्काऊ असत्य;
                }
                फर्काऊ सत्य;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    शून्य varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        जबसम्म (!scp.empty()) {
            अचर स्वतः scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            यदि ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                अचर स्वतः pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                यदि (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            यदि ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } अन्यथा {
                यदि (prd_pos == string::npos) रोक;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    शून्य addScopeVarEntry(अचर AstScope* अचर scopep, अचर Ast_गाँठो_Module* अचर modp,
                          अचर AstVar* अचर varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        यदि (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        अचर std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        अचर std::string scpName = whole.substr(0, dpos);
        अचर std::string varBase = whole.substr(dpos + std::म_माप("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        अचर std::string varBasePretty = Ast_गाँठो::vpiName(VName::dehash(varBase));
        अचर std::string scpPretty = Ast_गाँठो::prettyName(VName::dehash(scpName));
        अचर std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        यदि (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    शून्य varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        लागि (अचर ScopeModPair& smPair : m_scopes) {
            अचर AstScope* अचर scopep = smPair.first;
            अचर Ast_गाँठो_Module* अचर smodp = smPair.second;
            लागि (अचर ModVarPair& mvPair : m_modVars) {
                अचर Ast_गाँठो_Module* अचर modp = mvPair.first;
                अचर AstVar* अचर varp = mvPair.second;
                यदि (modp != smodp) जारी;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    शून्य collectIfaceRefs(अचर AstScope* nodep) {
        अचर AstCell* अचर cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        यदि (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) फर्काऊ;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        अचर std::string path = Ast_गाँठो::vpiName(nodep->name());
        अचर std::string instName = Ast_गाँठो::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        अचर std::string parentPath = path.substr(0, path.length() - instName.length());

        लागि (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            अचर std::string refName = Ast_गाँठो::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            यदि (!VString::startsWith(refName, parentPath)) जारी;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_गाँठो::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    शून्य buildIfaceRefTable() {
        यदि (m_ifaceRefs.empty()) फर्काऊ;
        अचर std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](अचर IfaceRefData& a, अचर IfaceRefData& b) { फर्काऊ a.m_suffix < b.m_suffix; });
        लागि (अचर IfaceRefData& ird : m_ifaceRefs) {
            अचर std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            यदि (m_scopeNames.find(scopeSym) == m_scopeNames.end()) जारी;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            अचर स्वतः parent = m_scopeNames.find(ird.m_parentSym);
            यदि (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protect_शब्दहरू_If(ird.m_name, सत्य));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दहरू_If(ird.m_suffix, सत्य));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दहरू_If(ird.m_modportName, सत्य));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        यदि (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    शून्य buildVpiHierarchy() {
        लागि (अचर स्वतः& itpair : m_scopeNames) {
            अचर std::string symName = itpair.second.m_symName;
            std::string above = symName;
            यदि (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            जबसम्म (!above.empty()) {
                अचर std::string::size_type pos = above.rfind("__");
                यदि (pos == std::string::npos) रोक;
                above.resize(pos);
                यदि (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].थप_back(symName);
                    रोक;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        यदि (v3Global.opt.vpi()) buildVpiHierarchy();

        यदि (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            लागि (अचर स्वतः& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](अचर ScopeModPair& a, अचर ScopeModPair& b) {
                             फर्काऊ a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](अचर AstCFunc* ap, अचर AstCFunc* bp) {
                             यदि (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 फर्काऊ bp->dpiImportPrototype();
                             }
                             फर्काऊ ap->name() < bp->name();
                         });

        // Output
        यदि (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        यदि (v3Global.dpi()) {
            emitDpiHdr();
            यदि (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        यदि (nodep->isConstPool()) फर्काऊ;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCellInlineScope* nodep) override {
        अचर std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        अचर std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        अचर पूर्णाङ्क timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_गाँठो::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    शून्य visit(AstScope* nodep) override {
        यदि (VN_IS(m_modp, Class)) फर्काऊ;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        यदि (v3Global.opt.vpi() && !nodep->is_शीर्ष()) {
            अचर std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            अचर पूर्णाङ्क timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_गाँठो::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstScopeName* nodep) override {
        यदि (nodep->selfRelative()) फर्काऊ;  // Its absolute copy is under the scope
        अचर std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        अचर पूर्णाङ्क timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        यदि (nodep->dpiExport()) {
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
    शून्य visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        यदि ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    शून्य visit(Ast_गाँठो_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        यदि (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->bin_अङ्क(m_coverBins);
            m_coverBins += nodep->size();
            nodep->local_Bin_अङ्क(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        यदि (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.थप_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    शून्य visit(AstConst*) override {}
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit EmitCSyms(AstNetlist* nodep, तार्किक dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

शून्य EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    माला_दो("//\n");
    माला_दो("// Internal details; most calling programs do not need this header,\n");
    माला_दो("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    माला_दो("\n");
    ofp()->puts_Int_शीर्ष_Include();
    माला_दो("#include \"verilated.h\"\n");
    यदि (v3Global.needTraceDumper()) {
        लागि (अचर string& base : v3Global.opt.traceSourceLangs())
            माला_दो("#include \"" + base + ".h\"\n");
    }
    यदि (v3Global.opt.usesProfiler()) माला_दो("#include \"verilated_profiler.h\"\n");

    माला_दो("\n// INCLUDE MODEL CLASS\n");
    माला_दो("\n#include \"" + शीर्ष_Class_Name() + ".h\"\n");

    माला_दो("\n// INCLUDE MODULE CLASSES\n");
    लागि (Ast_गाँठो_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), गाँठो_Module);
        यदि (VN_IS(nodep, Class)) जारी;  // Class included earlier
        यदि (nodep->isConstPool()) जारी;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    यदि (v3Global.dpi()) {
        माला_दो("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        लागि (अचर स्वतः& itpair : m_scopeFuncs) {
            अचर AstCFunc* अचर funcp = itpair.second.m_cfuncp;
            यदि (!funcp->dpiExportImpl()) जारी;
            अचर std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            अचर std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        लागि (अचर std::string& type : types) माला_दो(type);
    }

    माला_दो("\n// SYMS CLASS (contains all model state)\n");
    माला_दो("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(असत्य);  // public:

    माला_दो("// INTERNAL STATE\n");
    माला_दो(शीर्ष_Class_Name() + "* const __Vm_modelp;\n");

    यदि (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        माला_दो("bool __Vm_dumping = false;  // Dumping is active\n");
        माला_दो("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        माला_दो(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    यदि (v3Global.opt.trace()) {
        माला_दो("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        माला_दो("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    यदि (v3Global.hasEvents()) {
        यदि (v3Global.assignsEvents()) {
            माला_दो("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } अन्यथा {
            माला_दो("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    यदि (v3Global.hasClasses()) माला_दो("VlDeleter __Vm_deleter;\n");
    माला_दो("bool& __Vm_didInit;\n");
    यदि (v3Global.opt.systemC()) माला_दो("sc_core::sc_event __Vm_wakeEvent;\n");

    यदि (v3Global.opt.mtasks()) {
        माला_दो("\n// MULTI-THREADING\n");
        माला_दो("VlThreadPool* __Vm_threadPoolp;\n");
        माला_दो("bool __Vm_even_cycle__ico = false;\n");
        माला_दो("bool __Vm_even_cycle__act = false;\n");
        माला_दो("bool __Vm_even_cycle__nba = false;\n");
    }

    यदि (v3Global.opt.profExec()) {
        माला_दो("\n// EXECUTION PROFILING\n");
        माला_दो("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    यदि (v3Global.opt.profPgo()) {
        माला_दो("\n// PGO PROFILING\n");
        माला_दो("VlPgoProfiler<" + std::to_string(ExecMTask::अङ्क_Used_Ids()) + "> _vm_pgoProfiler;\n");
    }

    माला_दो("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    माला_दो("union {\nstruct {\n");
    लागि (अचर ScopeModPair& itpair : m_scopes) {
        अचर AstScope* अचर scopep = itpair.first;
        अचर Ast_गाँठो_Module* अचर modp = itpair.second;
        यदि (VN_IS(modp, Class)) जारी;
        ofp()->म_लिखो("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    माला_दो("} vlInst;\n};\n");

    यदि (m_coverBins) {
        माला_दो("\n// COVERAGE\n");
        माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        माला_दो(" __Vcoverage[");
        माला_दो(std::to_string(m_coverBins));
        माला_दो("];\n");
    }

    यदि (!m_scopeNames.empty()) {  // Scope names
        माला_दो("\n// SCOPE NAMES\n");
        लागि (अचर स्वतः& itpair : m_scopeNames) {
            अचर ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    यदि (v3Global.opt.vpi()) {
        माला_दो("\n// SCOPE HIERARCHY\n");
        माला_दो("VerilatedHierarchy __Vhier;\n");
    }

    माला_दो("\n// CONSTRUCTORS\n");
    माला_दो(symClassName() + "(VerilatedContext* contextp, const char* namep, " + शीर्ष_Class_Name()
         + "* modelp);\n");
    माला_दो("~" + symClassName() + "();\n");

    माला_दो("\n// METHODS\n");
    माला_दो("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    यदि (v3Global.hasEvents()) {
        यदि (v3Global.assignsEvents()) {
            माला_दो("void fireEvent(VlAssignableEvent& event) {\n");
        } अन्यथा {
            माला_दो("void fireEvent(VlEvent& event) {\n");
        }
        माला_दो("if (VL_LIKELY(!event.isTriggered())) {\n");
        यदि (v3Global.assignsEvents()) {
            माला_दो("__Vm_triggeredEvents.push_back(event);\n");
        } अन्यथा {
            माला_दो("__Vm_triggeredEvents.push_back(&event);\n");
        }
        माला_दो("}\n");
        माला_दो("event.fire();\n");
        माला_दो("}\n");
        माला_दो("void clearTriggeredEvents() {\n");
        यदि (v3Global.assignsEvents()) {
            माला_दो("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } अन्यथा {
            माला_दो("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        माला_दो("__Vm_triggeredEvents.clear();\n");
        माला_दो("}\n");
    }

    यदि (v3Global.needTraceDumper()) {
        यदि (!optSystemC()) माला_दो("void _traceDump();\n");
        माला_दो("void _traceDumpOpen();\n");
        माला_दो("void _traceDumpClose();\n");
    }

    यदि (v3Global.opt.savable()) {
        माला_दो("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        माला_दो("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    माला_दो("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

शून्य EmitCSyms::emitSymImpPreamble() {
    माला_दो("\n");

    // Includes
    माला_दो("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    माला_दो("\n");
    // Declarations for DPI Export implementation functions
    तार्किक needs_New_पङ्क्ति = असत्य;
    लागि (अचर स्वतः& itpair : m_scopeFuncs) {
        अचर AstCFunc* अचर funcp = itpair.second.m_cfuncp;
        यदि (!funcp->dpiExportImpl()) जारी;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_पङ्क्ति = सत्य;
    }
    यदि (needs_New_पङ्क्ति) माला_दो("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    लागि (अचर ScopeModPair& itpair : m_scopes) {
        यदि (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    लागि (अचर std::string& libName : libNames) {
        माला_दो("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        माला_दो("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    यदि (!libNames.empty()) माला_दो("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    यदि (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        लागि (अचर स्वतः& kv : m_varTables) {
            माला_दो("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        यदि (!m_scopeTableRows.empty()) {
            माला_दो("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        यदि (!m_ifaceRefTableRows.empty()) {
            माला_दो("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        माला_दो("\n");
    }
}

शून्य EmitCSyms::emitVarTables() {
    यदि (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) फर्काऊ;
    माला_दो("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    माला_दो("#if defined(__GNUC__)\n");
    माला_दो("# pragma GCC diagnostic push\n");
    माला_दो("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    माला_दो("#endif\n");
    लागि (अचर स्वतः& kv : m_varTables) {
        माला_दो("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        लागि (अचर std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        माला_दो("};\n");
    }
    यदि (!m_scopeTableRows.empty()) {
        माला_दो("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        लागि (अचर std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        माला_दो("};\n");
    }
    यदि (!m_ifaceRefTableRows.empty()) {
        माला_दो("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        लागि (अचर std::string& row : m_ifaceRefTableRows) {
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

शून्य EmitCSyms::emitScopeHier(TextStmts& stmts, तार्किक destroy) {
    यदि (!v3Global.opt.vpi()) फर्काऊ;

    यदि (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } अन्यथा {
        stmts.add("// Set up scope hierarchy");
    }

    अचर std::string method = destroy ? "remove" : "add";
    लागि (अचर स्वतः& itpair : m_scopeNames) {
        यदि (itpair.first == "TOP") जारी;
        अचर ScopeData& sd = itpair.second;
        अचर std::string& name = sd.m_prettyName;
        अचर std::string& scopeType = sd.m_type;
        यदि (name.find('.') != string::npos) जारी;
        यदि (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") जारी;
        अचर std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    लागि (अचर स्वतः& itpair : m_vpiScopeHierarchy) {
        अचर std::string fromName = scopeSymString(itpair.first);
        अचर std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        लागि (अचर std::string& name : itpair.second) {
            अचर std::string toName = scopeSymString(name);
            अचर std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    यदि (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t थाक_Size = V3_थाक_गन्ती::गन्ती(v3Global.rootp());
        यदि (v3Global.opt.debug_थाक_Check()) थाक_Size += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", थाक_Size);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, थाक_Size + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(थाक_Size) + ");");
    }

    stmts.add("// Setup sub module instances");
    लागि (अचर ScopeModPair& itpair : m_scopes) {
        अचर AstScope* अचर scopep = itpair.first;
        अचर Ast_गाँठो_Module* अचर modp = itpair.second;
        यदि (modp->is_शीर्ष()) जारी;
        अचर std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protect_शब्दहरू_If(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    यदि (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        यदि (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        यदि (v3Global.opt.mtasks()) {
            v3Global.rootp()->शीर्ष_Modulep()->foreach([&](अचर AstExecGraph* execGraphp) {
                लागि (अचर V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    अचर ExecMTask& mt = static_cast<अचर ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    यदि (!v3Global.rootp()->timeunit().isNone()) {
        अचर std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    यदि (!v3Global.rootp()->timeprecision().isNone()) {
        अचर std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    लागि (अचर स्वतः& i : m_scopes) {
        अचर AstScope* अचर scopep = i.first;
        अचर Ast_गाँठो_Module* अचर modp = i.second;
        अचर AstScope* अचर abovep = scopep->aboveScopep();
        यदि (!abovep) जारी;

        अचर std::string protName = VIdProtect::protect_शब्दहरू_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        यदि (VN_IS(modp, ClassPackage)) {
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

    यदि (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        लागि (अचर ScopeModPair& i : m_scopes) {
            अचर AstScope* अचर scopep = i.first;
            Ast_गाँठो_Module* अचर modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            अचर तार्किक first = !modp->user1();
            modp->user1(सत्य);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    लागि (अचर ScopeModPair& i : m_scopes) {
        अचर AstScope* अचर scopep = i.first;
        अचर Ast_गाँठो_Module* अचर modp = i.second;
        यदि (!modp->verilatorLib()) जारी;
        अचर AstVar* hashp = nullptr;
        अचर AstVar* handlep = nullptr;
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अचर AstVar* अचर varp = VN_CAST(nodep, Var);
            यदि (!varp) जारी;
            यदि (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            यदि (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        अचर uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        यदि (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->trace_Lib_शीर्ष_Name().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    यदि (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        अचर std::string symClass = symClassName();
        लागि (अचर स्वतः& itpair : m_scopeNames) {
            अचर ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protect_शब्दहरू_If(sd.m_prettyName, सत्य));
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
    यदि (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, असत्य);

    यदि (v3Global.dpi()) {
        लागि (अचर std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            लागि (अचर स्वतः& itpair : m_scopeFuncs) {
                अचर ScopeFuncData& sfd = itpair.second;
                अचर AstScopeName* अचर scopep = sfd.m_scopep;
                अचर AstCFunc* अचर funcp = sfd.m_cfuncp;
                अचर Ast_गाँठो_Module* अचर modp = sfd.m_modp;
                यदि (!funcp->dpiExportImpl()) जारी;

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
    यदि (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        पूर्णाङ्क tableCounter = 0;

        स्वतः it = m_scopeVars.cbegin();
        जबसम्म (it != m_scopeVars.cend()) {
            अचर std::string scopeName = it->second.m_scopeName;
            अचर AstScope* अचर instScopep = it->second.m_scopep;
            अचर Ast_गाँठो_Module* अचर modp = it->second.m_modp;
            अचर std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            लागि (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                अचर ScopeVarData& svd = it->second;
                अचर AstScope* अचर scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                अचर AstVar* अचर varp = svd.m_varp;
                अचर VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                अचर std::pair<तार्किक, std::string> fc = isForceControlSignal(varp);
                यदि (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    जारी;
                }

                std::string row;
                अचर TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                विकल्प (kind) {
                पक्ष TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    रोक;
                पक्ष TableEntryKind::FORCEABLE_RESIDUAL: {
                    अचर std::string bounds = boundsString(dims);
                    residual.emplace_back(घुसाउ_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    रोक;
                }
                पक्ष TableEntryKind::PLAIN_RESIDUAL: {
                    अचर std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        घुसाउ_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    रोक;
                }
                स्वभाव: v3fatalSrc("Bad case");
                }
                यदि (kind == TableEntryKind::TABLE_ROW) जारी;
                यदि (अचर Ast_गाँठो_UOr_Struct_DType* अचर sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), गाँठो_UOr_Struct_DType)) {
                    यदि (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } अन्यथा यदि (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                यदि (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            यदि (!rows.empty()) {
                अचर माप_प्रकार row_गन्ती = rows.size();
                std::string साँचो;
                लागि (अचर std::string& r : rows) {
                    साँचो += r;
                    साँचो += '\0';
                }
                अचर स्वतः itt = tableByRows.find(साँचो);
                std::string tableName;
                यदि (itt != tableByRows.end()) {
                    tableName = itt->second;
                } अन्यथा {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(साँचो), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(row_गन्ती) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            लागि (अचर std::string& r : residual) stmts.add(r);
        }
    }

    फर्काऊ stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, सत्य);
    यदि (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    यदि (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + शीर्ष_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    यदि (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    लागि (अचर स्वतः& itpair : m_scopeNames) {
        अचर ScopeData& sd = itpair.second;
        अचर std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    लागि (अचर ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        अचर AstScope* अचर scopep = itpair.first;
        अचर Ast_गाँठो_Module* अचर modp = itpair.second;
        यदि (modp->is_शीर्ष()) जारी;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    फर्काऊ stmts;
}

शून्य EmitCSyms::emitSymImp(अचर AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    अचर std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            सत्य, "Symbol table implementation internals",
                            [एतत्]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), सत्य, सत्य, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    अचर AstScope* अचर शीर्ष_Scopep = netlistp->शीर्ष_Scopep()->scopep();
    अचर std::string शीर्ष_Inst = "vlSymsp->" + instName(शीर्ष_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    यदि (!splitCtorDtorDecls.first.empty()) माला_दो("\n" + splitCtorDtorDecls.first + "\n");
    माला_दो("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    माला_दो("// Setup top module instance\n");
    माला_दो("VL_INPLACE_CONSTRUCT(" + शीर्ष_Inst + ", vlSymsp, namep);\n");
    लागि (अचर TextStmts::Stmt& stmt : ctorStmts) {
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
    यदि (v3Global.opt.mtasks()) {
        माला_दो("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    यदि (v3Global.opt.profExec()) {
        माला_दो("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    यदि (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        माला_दो("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    माला_दो("{\n");
    माला_दो(symClassName() + "__ctor(this, namep);\n");
    माला_दो("}\n");

    // Destructor contents, in a loose function, see constructor
    यदि (!splitCtorDtorDecls.second.empty()) माला_दो("\n" + splitCtorDtorDecls.second + "\n");
    माला_दो("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    लागि (अचर TextStmts::Stmt& stmt : dtorStmts) {
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
    यदि (v3Global.needTraceDumper()) {
        यदि (!optSystemC()) {
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

    यदि (v3Global.opt.savable()) {
        लागि (अचर तार्किक& de : {असत्य, सत्य}) {
            अचर std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            अचर std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            अचर std::string op = de ? ">>" : "<<";
            माला_दो("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            माला_दो("// Internal state\n");
            यदि (v3Global.opt.trace()) माला_दो("os" + op + "__Vm_activity;\n");
            माला_दो("os " + op + " __Vm_didInit;\n");
            माला_दो("// Module instance state\n");
            लागि (अचर ScopeModPair& itpair : m_scopes) {
                अचर AstScope* अचर scopep = itpair.first;
                माला_दो(instName(scopep) + "." + funcname + "(os);\n");
            }
            माला_दो("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

शून्य EmitCSyms::emitDpiHdr() {
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
    लागि (अचर AstCFunc* अचर nodep : m_dpis) {
        लागि (अचर Ast_गाँठो* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            यदि (अचर AstVar* अचर portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    यदि (!structNames.empty()) माला_दो("\n");

    माला_दो("#ifdef __cplusplus\n");
    माला_दो("extern \"C\" {\n");
    माला_दो("#endif\n");
    माला_दो("\n");

    पूर्णाङ्क firstExp = 0;
    पूर्णाङ्क firstImp = 0;
    लागि (अचर AstCFunc* अचर nodep : m_dpis) {
        यदि (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) जारी;

        अचर std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        यदि (nodep->dpiExportDispatcher()) {
            यदि (!firstExp++) माला_दो("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } अन्यथा {
            यदि (!firstImp++) माला_दो("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        यदि (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } अन्यथा {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, असत्य) + ");\n");
        }
    }

    माला_दो("\n");
    माला_दो("#ifdef __cplusplus\n");
    माला_दो("}\n");
    माला_दो("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

शून्य EmitCSyms::emitDpiStruct(अचर Ast_गाँठो_DType* dtypep, std::set<std::string>& namesr) {
    यदि (!dtypep->containsUnpackedStruct()) फर्काऊ;
    अचर AstStructDType* अचर sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    अचर std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    यदि (namesr.गन्ती(name)) फर्काऊ;
    // After the structures it contains
    लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    यदि (name.empty()) फर्काऊ;  // Declared where used
    यदि (namesr.empty()) माला_दो("// DPI TYPES\n");
    namesr.घुसाउ(name);
    // Guarded, so the user, or another model, may declare it instead
    माला_दो("#ifndef VL_DPITYPE_" + name + "_\n");
    माला_दो("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    माला_दो("#endif\n");
}

//######################################################################

शून्य EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(शीर्ष_Class_Name() + "__Dpi", असत्य, सत्य,
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

    लागि (अचर AstCFunc* अचर nodep : m_dpis) {
        यदि (!nodep->dpiExportDispatcher()) जारी;

        अचर std::string name = nodep->name();
        अचर std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        माला_दो("#ifndef VL_DPIDECL_" + name + "_\n");
        माला_दो("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        माला_दो("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + शीर्ष_Class_Name() + "::" + name + "(");
        std::string comma;
        लागि (अचर Ast_गाँठो* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (अचर AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO() && !portp->isFuncReturn()) {
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

शून्य V3EmitC::emitcSyms(तार्किक dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
