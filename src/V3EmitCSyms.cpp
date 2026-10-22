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
स्थिर constexpr स्वचालित symClassName = &EmitCUtil::symClassName;
स्थिर constexpr स्वचालित शिखरम्_Class_Name = &EmitCUtil::शिखरम्_Class_Name;

वर्ग EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // TYPES
    संरचना Scope_दत्त final {
        ध्रुव Ast_ग्रन्थिः* m_nodep;  // Scope symbol-table entry corresponds to
        ध्रुव std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ध्रुव std::string m_prettyName;  // Pretty (unprotected) name for display
        ध्रुव std::string m_defName;  // Name registered for this scope
        ध्रुव पूर्णाङ्क m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_दत्त(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव std::string& symName, ध्रुव std::string& prettyName,
                  ध्रुव std::string& defName, पूर्णाङ्क timeunit, ध्रुव std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    संरचना Iface_Ref_दत्त final {
        ध्रुव AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_दत्त(ध्रुव AstScope* scopep, ध्रुव std::string& parentSym,
                     ध्रुव std::string& suffix, ध्रुव std::string& name,
                     ध्रुव std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    संरचना Scope_Func_दत्त final {
        ध्रुव AstScopeName* ध्रुव m_scopep;  // Scope the DPI export function is registered under
        ध्रुव AstCFunc* ध्रुव m_cfuncp;  // DPI export function
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module containing the DPI export function
        Scope_Func_दत्त(ध्रुव AstScopeName* scopep, ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    संरचना Scope_Var_दत्त final {
        ध्रुव std::string m_scopeName;  // Full name of the scope containing the variable
        ध्रुव std::string m_var_आधारः_Pretty;  // Pretty base name of the variable, without scope
        ध्रुव AstVar* ध्रुव m_varp;  // Public variable
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module containing the variable
        ध्रुव AstScope* ध्रुव m_scopep;  // Scope containing the variable
        Scope_Var_दत्त(ध्रुव std::string& scopeName, ध्रुव std::string& var_आधारः_Pretty,
                     ध्रुव AstVar* varp, ध्रुव Ast_ग्रन्थिः_Module* modp, ध्रुव AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_आधारः_Pretty{var_आधारः_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    उपयुज्य Scope_नामानि = std::map<ध्रुव std::string, Scope_दत्त>;
    उपयुज्य ScopeModPair = std::pair<ध्रुव AstScope*, Ast_ग्रन्थिः_Module*>;
    उपयुज्य ModVarPair = std::pair<ध्रुव Ast_ग्रन्थिः_Module*, ध्रुव AstVar*>;

    // Vars with more dims take the residual per-statement path.
    स्थिर constexpr पूर्णाङ्क VPI_सारणी_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = शून्यसूचकः;  // Current function
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    ध्रुव AstScope* m_scopep = शून्यसूचकः;  // Current scope
    std::सदिशम्<ScopeModPair> m_scopes;  // Every scope by module
    std::सदिशम्<AstCFunc*> m_dpis;  // DPI functions
    std::सदिशम्<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ध्रुव std::string, Scope_Func_दत्त> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ध्रुव std::string, Scope_Var_दत्त> m_scopeVars;  // Each {scope,public-var}
    Scope_नामानि m_scope_नामानि;  // Each unique AstScopeName. Dpi scopes added later
    Scope_नामानि m_dpi_Scope_नामानि;  // Each unique AstScopeName for DPI export
    Scope_नामानि m_vpiScopeCandidates;  // All scopes for VPI
    std::सदिशम्<Iface_Ref_दत्त> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ध्रुव std::string, std::सदिशम्<std::string>> m_vpiScopeHierarchy;
    पूर्णाङ्क m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    पूर्णाङ्क m_modCoverBins = 0;  // Per-module coverage bin number
    ध्रुव तार्किक m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_अष्टकानि;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::सदिशम्<std::pair<std::string, std::सदिशम्<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_सारणी_Name;
    std::सदिशम्<std::string> m_scope_सारणी_पङ्क्तिकाः;
    std::string m_iface_Ref_सारणी_Name;
    std::सदिशम्<std::string> m_iface_Ref_सारणी_पङ्क्तिकाः;

    // METHODS
    शून्य emitSymHdr();
    शून्य emitSymImpPreamble();
    शून्य emitVarTables();
    शून्य emitScopeHier(TextStmts& stmts, तार्किक destroy);
    शून्य emitSymImp(ध्रुव AstNetlist* netlistp);
    शून्य emitDpiHdr();
    शून्य emitDpiStruct(ध्रुव Ast_ग्रन्थिः_DType* dtypep, std::समुच्चयः<std::string>& namesr);
    शून्य emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    स्थिर शून्य nameCheck(Ast_ग्रन्थिः* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        यदि (nodep->name().रिक्तानि()) प्रत्यागम;
        ध्रुव AstCFunc* ध्रुव cfuncp = VN_CAST(nodep, CFunc);
        यदि (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ध्रुव std::string rsvd = V3_Language_शब्दाः::isKeyword(nodep->name());
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

    स्थिर std::string scopeSymString(ध्रुव std::string& scpname) {
        std::string बहिर्गमनम् = scpname;
        std::string::आकार_type pos;
        यावत् ((pos = बहिर्गमनम्.find("__PVT__")) != std::string::npos) बहिर्गमनम्.replace(pos, 7, "");
        यदि (VString::startsWith(बहिर्गमनम्, "TOP__DOT__")) बहिर्गमनम्.replace(0, 10, "");
        यदि (VString::startsWith(बहिर्गमनम्, "TOP.")) बहिर्गमनम्.replace(0, 4, "");
        यावत् ((pos = बहिर्गमनम्.find('.')) != std::string::npos) बहिर्गमनम्.replace(pos, 1, "__");
        यावत् ((pos = बहिर्गमनम्.find("__DOT__")) != std::string::npos) बहिर्गमनम्.replace(pos, 7, "__");
        प्रत्यागम बहिर्गमनम्;
    }

    स्थिर string scopeDecodeIdentifier(ध्रुव std::string& scpname) {
        std::string::आकार_type pos = std::string::npos;

        // Remove hierarchy
        आकार_t i = 0;
        // always makes progress
        यावत् (i < scpname.दीर्घता_2()) {
            यदि (scpname[i] == '\\') {
                यावत् (i < scpname.दीर्घता_2() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } अन्यथा {
                यावत् (i < scpname.दीर्घता_2() && scpname[i] != '.') ++i;
                यदि (i < scpname.दीर्घता_2()) pos = i++;
            }
        }

        प्रत्यागम pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    संरचना VarDims final {
        पूर्णाङ्क pdim = 0;  // Packed array dims plus a ranged basic leaf
        पूर्णाङ्क udim = 0;
        std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> unpackedLR;  // (left,right), outer-first
        std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> flatten() ध्रुव {
            std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> flat(unpackedLR);
            flat.निवेशय(flat.अन्तः_2(), packedLR.cbegin(), packedLR.cend());
            प्रत्यागम flat;
        }
    };
    स्थिर VarDims getVarDims(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव मूलम्_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        प्रति (ध्रुव Ast_ग्रन्थिः_DType* dtypep = मूलम्_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            यदि (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                यदि (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_पृष्ठभागः(adtypep->वामतः(), adtypep->दक्षिणतः());
                    ++d.pdim;
                } अन्यथा {
                    d.unpackedLR.emplace_पृष्ठभागः(adtypep->वामतः(), adtypep->दक्षिणतः());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } अन्यथा {
                यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
                    यदि (basicp->isRanged()) {
                        d.packedLR.emplace_पृष्ठभागः(basicp->वामतः(), basicp->दक्षिणतः());
                        ++d.pdim;
                    }
                }
                भङ्ग;  // Non-array leaf
            }
        }
        प्रत्यागम d;
    }

    स्थिर VarDims dimsFor(ध्रुव Scope_Var_दत्त& svd) { प्रत्यागम getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    स्थिर std::string boundsString(ध्रुव VarDims& d) {
        std::string bounds;
        प्रति (ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::गन्तव्यम्_string(lr.first);
            bounds += ",";
            bounds += std::गन्तव्यम्_string(lr.second);
        }
        प्रत्यागम bounds;
    }

    स्थिर पूर्णाङ्क getUnpackedElements(ध्रुव Ast_ग्रन्थिः_DType* मूलम्_Dtypep) {
        पूर्णाङ्क elements = 1;
        प्रति (ध्रुव Ast_ग्रन्थिः_DType* dtypep = मूलम्_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(dtypep, UnpackArrayDType);
            यदि (!adtypep) भङ्ग;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        प्रत्यागम elements;
    }

    स्थिर तार्किक needs_Emitted_Ent_आकार(ध्रुव std::string& vlEnumType) {
        प्रत्यागम vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    स्थिर std::pair<तार्किक, std::string> isForceControlSignal(ध्रुव AstVar* ध्रुव signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        प्रति (ध्रुव std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ध्रुव std::आकार_t suffixPos = signalVarp->name().find(forceControlSuffix);
            ध्रुव तार्किक suffix_प्राप्तम् = suffixPos != std::string::npos;
            ध्रुव तार्किक suffix_Is_At_अन्तः
                = suffixPos + forceControlSuffix.दीर्घता_2() == signalVarp->name().दीर्घता_2();
            यदि (!(suffix_प्राप्तम् && suffix_Is_At_अन्तः)) अनुवर्त;
            ध्रुव std::string आधारः_Signal_Name = signalVarp->name().substr(0, suffixPos);
            प्रत्यागम std::pair<तार्किक, std::string>{सत्य, आधारः_Signal_Name};
        }
        प्रत्यागम std::pair<तार्किक, std::string>{असत्य, ""};
    }

    स्थिर शून्य appendVarProperties(std::string& stmt, ध्रुव std::string& vlEnumType,
                                    ध्रुव std::string& vlEnumDir, ध्रुव पूर्णाङ्क udim, ध्रुव पूर्णाङ्क pdim,
                                    ध्रुव std::string& bounds, ध्रुव std::string& ent_आकार = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::गन्तव्यम्_string(udim);
        यदि (!ent_आकार.रिक्तानि()) {
            stmt += ", ";
            stmt += ent_आकार;
        } अन्यथा {
            stmt += ", ";
            stmt += std::गन्तव्यम्_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    स्थिर std::string memberVlEnumDir(ध्रुव AstVar* ध्रुव varp,
                                       ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        std::string बहिर्गमनम् = '(' + varp->vlEnumDir(/*forMember=*/सत्य);
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव skipDTypep = dtypep->skipRefp();
        यदि (skipDTypep->isSigned()) बहिर्गमनम् += "|VLVF_SIGNED";
        यदि (ध्रुव AstBasicDType* ध्रुव basicp = skipDTypep->basicp()) {
            यदि (basicp->keyword() == VBasicDTypeKwd::BIT) बहिर्गमनम् += "|VLVF_BITVAR";
        }
        बहिर्गमनम् += skipDTypep->vlEnumPackedAgg();
        प्रत्यागम बहिर्गमनम् + ')';
    }

    // Name of the instance member for the given scope
    स्थिर std::string instName(ध्रुव AstScope* scopep) {
        प्रत्यागम "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    स्थिर std::string निवेशय_Var_Statement(ध्रुव Scope_Var_दत्त& svd, ध्रुव AstScope* ध्रुव scopep,
                                          ध्रुव AstVar* ध्रुव varp, ध्रुव पूर्णाङ्क udim, ध्रुव पूर्णाङ्क pdim,
                                          ध्रुव std::string& bounds) {
        ध्रुव std::string var_व्यञ्जकम् = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ध्रुव std::string vlEnumType = varp->vlEnumType();
        ध्रुव तार्किक needs_Ent_आकार = needs_Emitted_Ent_आकार(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_आकार ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_बहिर्गमनम्_Formatter::quoteNameControls(protect(svd.m_var_आधारः_Pretty)) + '"';

        यदि (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_व्यञ्जकम्;
            stmt += "), false, ";
        } अन्यथा यदि (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_व्यञ्जकम्;
            stmt += ".c_str())), true, ";
        } अन्यथा {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_व्यञ्जकम्;
            stmt += "))), true, ";
        }

        ध्रुव std::string ent_आकार = needs_Ent_आकार
                                        ? "sizeof(" + var_व्यञ्जकम् + ") / "
                                              + std::गन्तव्यम्_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_आकार);
        प्रत्यागम stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    गणना वर्ग सारणी_प्रविष्टिः_विधा : uint8_t {
        सारणी_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    सारणी_प्रविष्टिः_विधा try_Build_सारणी_प्रविष्टिः(ध्रुव Scope_Var_दत्त& svd, ध्रुव AstVar* ध्रुव varp,
                                      ध्रुव AstScope* ध्रुव scopep,
                                      ध्रुव std::string& modClassName, ध्रुव VarDims& dims,
                                      std::string& row_बहिर्गमनम्) ध्रुव {
        ध्रुव पूर्णाङ्क pdim = dims.pdim;
        ध्रुव पूर्णाङ्क udim = dims.udim;
        यदि (varp->isForceable() && force_Control_सङ्केताः_Are_Valid(scopep, varp)) {
            प्रत्यागम सारणी_प्रविष्टिः_विधा::FORCEABLE_RESIDUAL;
        }
        यदि (udim + pdim > VPI_सारणी_MAX_DIMS) प्रत्यागम सारणी_प्रविष्टिः_विधा::PLAIN_RESIDUAL;

        ध्रुव std::string vlEnumType = varp->vlEnumType();
        यदि (needs_Emitted_Ent_आकार(vlEnumType))
            प्रत्यागम सारणी_प्रविष्टिः_विधा::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        यदि (varp->isParam()) प्रत्यागम सारणी_प्रविष्टिः_विधा::PLAIN_RESIDUAL;

        ध्रुव std::string name = V3_बहिर्गमनम्_Formatter::quoteNameControls(protect(svd.m_var_आधारः_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ध्रुव std::string member = varName(varp);
        ध्रुव std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::सदिशम्<पूर्णाङ्क> dv;
        प्रति (ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क>& lr : dims.flatten()) {
            dv.प्रक्षिप_पृष्ठभागः(lr.first);
            dv.प्रक्षिप_पृष्ठभागः(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::गन्तव्यम्_string(udim) + ", " + std::गन्तव्यम्_string(pdim) + ", {";
        प्रति (पूर्णाङ्क i = 0; i < VPI_सारणी_MAX_DIMS * 2; ++i) {
            यदि (i) row += ", ";
            row += std::गन्तव्यम्_string(i < static_cast<पूर्णाङ्क>(dv.आकार()) ? dv[i] : 0);
        }
        row += "}}";
        row_बहिर्गमनम् = row;
        प्रत्यागम सारणी_प्रविष्टिः_विधा::सारणी_ROW;
    }

    स्थिर std::string निवेशय_DType_Var_Statement(ध्रुव Scope_Var_दत्त& svd,
                                               ध्रुव AstScope* ध्रुव scopep,
                                               ध्रुव std::string& prettyName,
                                               ध्रुव std::string& cName,
                                               ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep, ध्रुव पूर्णाङ्क udim,
                                               ध्रुव पूर्णाङ्क pdim, ध्रुव std::string& bounds) {
        ध्रुव std::string vlEnumType = dtypep->vlEnumType();
        ध्रुव तार्किक needs_Ent_आकार = needs_Emitted_Ent_आकार(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_आकार ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_बहिर्गमनम्_Formatter::quoteNameControls(prettyName) + '"';
        ध्रुव std::string var_व्यञ्जकम् = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        यदि (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_व्यञ्जकम्;
            stmt += "))), true, ";
        } अन्यथा {
            stmt += ", &(";
            stmt += var_व्यञ्जकम्;
            stmt += "), false, ";
        }
        ध्रुव std::string ent_आकार
            = needs_Ent_आकार
                  ? "sizeof(" + var_व्यञ्जकम् + ") / " + std::गन्तव्यम्_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_आकार);
        प्रत्यागम stmt;
    }

    स्थिर शून्य सङ्कलय_UOr_Struct_Member_Vars(std::सदिशम्<std::string>& stmts, ध्रुव Scope_Var_दत्त& svd,
                                       ध्रुव AstScope* ध्रुव scopep,
                                       ध्रुव std::string& pretty_उपसर्गः, ध्रुव std::string& c_उपसर्गः,
                                       ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep) {
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव itemDTypep = itemp->dtypep();
            यदि (itemDTypep->vlEnumType().रिक्तानि()) अनुवर्त;
            ध्रुव std::string prettyName
                = pretty_उपसर्गः + "." + Ast_ग्रन्थिः::vpiName(itemp->shortName());
            ध्रुव std::string cName = c_उपसर्गः + "." + itemp->nameProtect();
            ध्रुव VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_पृष्ठभागः(निवेशय_DType_Var_Statement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            सङ्कलय_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp
                = VN_CAST(itemDTypep->skipRefp(), ग्रन्थिः_UOr_Struct_DType)) {
                यदि (!subp->packed())
                    सङ्कलय_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } अन्यथा यदि (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                सङ्कलय_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    स्थिर शून्य सङ्कलय_Unpacked_Array_UOr_Struct_Member_Vars(std::सदिशम्<std::string>& stmts,
                                                    ध्रुव Scope_Var_दत्त& svd,
                                                    ध्रुव AstScope* ध्रुव scopep,
                                                    ध्रुव std::string& pretty_उपसर्गः,
                                                    ध्रुव std::string& c_उपसर्गः,
                                                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव skipDTypep = dtypep->skipRefp();
        यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            सङ्कलय_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_उपसर्गः, c_उपसर्गः + "[0]",
                                                adtypep->subDTypep());
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep
                   = VN_CAST(skipDTypep, ग्रन्थिः_UOr_Struct_DType)) {
            यदि (!sdtypep->packed())
                सङ्कलय_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_उपसर्गः, c_उपसर्गः, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    स्थिर शून्य सङ्कलय_Packed_Member_Vars(std::सदिशम्<std::string>& stmts, ध्रुव Scope_Var_दत्त& svd,
                                    ध्रुव AstScope* ध्रुव scopep, ध्रुव std::string& pretty_उपसर्गः,
                                    ध्रुव std::string& cName, ध्रुव std::string& vlEnumType,
                                    ध्रुव std::string& ent_आकार,
                                    ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep,
                                    ध्रुव पूर्णाङ्क parent_Bit_विस्थापनम्, ध्रुव तार्किक needs_सूचकांक) {
        ध्रुव std::string var_व्यञ्जकम् = "vlSymsp->" + instName(scopep) + "." + cName;
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव itemDTypep = itemp->dtypep();
            ध्रुव std::string prettyName
                = pretty_उपसर्गः + "." + Ast_ग्रन्थिः::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ध्रुव पूर्णाङ्क bit_विस्थापनम् = parent_Bit_विस्थापनम् + itemp->lsb();
            ध्रुव VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_बहिर्गमनम्_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            यदि (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_व्यञ्जकम् + "))), true, ";
            } अन्यथा {
                stmt += ", &(" + var_व्यञ्जकम् + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            यदि (needs_सूचकांक) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::गन्तव्यम्_string(bit_विस्थापनम्) + ", ";
            stmt += ent_आकार + ", ";
            stmt += std::गन्तव्यम्_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_पृष्ठभागः(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ध्रुव Ast_ग्रन्थिः_DType* subDTypep = itemDTypep->skipRefp();
            तार्किक isArray = असत्य;
            यावत् (ध्रुव AstPackArrayDType* ध्रुव adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = सत्य;
            }
            यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp = VN_CAST(subDTypep, ग्रन्थिः_UOr_Struct_DType)) {
                सङ्कलय_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_आकार,
                                    subp, bit_विस्थापनम्, needs_सूचकांक || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    स्थिर शून्य सङ्कलय_Packed_Agg_Member_Vars(std::सदिशम्<std::string>& stmts, ध्रुव Scope_Var_दत्त& svd,
                                       ध्रुव AstScope* ध्रुव scopep,
                                       ध्रुव std::string& pretty_उपसर्गः, ध्रुव std::string& cName,
                                       ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        std::string elemCName = cName;
        ध्रुव Ast_ग्रन्थिः_DType* elemDTypep = dtypep->skipRefp();
        तार्किक isArray = असत्य;
        यावत् (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(elemDTypep, ग्रन्थिः_Array_DType)) {
            यदि (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = सत्य;
        }
        ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep = VN_CAST(elemDTypep, ग्रन्थिः_UOr_Struct_DType);
        यदि (!sdtypep || !sdtypep->packed()) प्रत्यागम;
        ध्रुव std::string var_व्यञ्जकम् = "vlSymsp->" + instName(scopep) + "." + cName;
        ध्रुव std::string ent_आकार
            = "sizeof(" + var_व्यञ्जकम् + ") / " + std::गन्तव्यम्_string(getUnpackedElements(dtypep));
        सङ्कलय_Packed_Member_Vars(stmts, svd, scopep, pretty_उपसर्गः, elemCName, dtypep->vlEnumType(),
                            ent_आकार, sdtypep, 0, isArray);
    }

    std::string निवेशय_Forceable_Var_Statement(ध्रुव Scope_Var_दत्त& svd, ध्रुव AstScope* ध्रुव scopep,
                                            ध्रुव AstVar* ध्रुव varp, ध्रुव पूर्णाङ्क udim,
                                            ध्रुव पूर्णाङ्क pdim, ध्रुव std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_बहिर्गमनम्_Formatter::quoteNameControls(protect(svd.m_var_आधारः_Pretty)) + '"';

        ध्रुव std::string var_व्यञ्जकम् = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        प्रतिज्ञा(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_व्यञ्जकम्;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_व्यञ्जकम् + "__VforceRd";
        stmt += "), \"" + V3_बहिर्गमनम्_Formatter::quoteNameControls(protect(svd.m_var_आधारः_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        प्रति (ध्रुव std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ध्रुव std::string enable_Signal_कुञ्चिका
                = get_कुञ्चिका_Name(scopep, varp->name() + forceControlSuffix);
            ध्रुव std::map<ध्रुव std::string, Scope_Var_दत्त>::const_iterator itpair
                = m_scopeVars.find(enable_Signal_कुञ्चिका);

            यदि (itpair == m_scopeVars.अन्तः_2()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_कुञ्चिका << "'.");
            }

            ध्रुव Scope_Var_दत्त& svd = itpair->second;
            ध्रुव AstScope* ध्रुव scopep = svd.m_scopep;
            ध्रुव AstVar* ध्रुव varp = svd.m_varp;
            ध्रुव VarDims dims = dimsFor(svd);
            stmt
                += निवेशय_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            यदि (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::गन्तव्यम्_string(udim);
        stmt += ", ";
        stmt += std::गन्तव्यम्_string(pdim);
        stmt += bounds;
        stmt += ")";
        प्रत्यागम stmt;
    }

    स्थिर std::string get_कुञ्चिका_Name(ध्रुव AstScope* ध्रुव scopep, ध्रुव std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //क्रमः
        // signal can be found.
        std::string सम्पूर्णम् = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        यदि (VString::startsWith(सम्पूर्णम्, "__DOT__TOP")) सम्पूर्णम्.replace(0, 10, "");
        ध्रुव std::string::आकार_type dpos = सम्पूर्णम्.rfind("__DOT__");
        यदि (dpos != std::string::npos) { scpName = सम्पूर्णम्.substr(0, dpos); }
        ध्रुव std::string scpSym = scopeSymString(VName::dehash(scpName));
        प्रत्यागम scpSym + " " + signal_name;
    }

    तार्किक आधारः_Signal_Is_Valid(ध्रुव AstScope* ध्रुव scopep, ध्रुव AstVar* ध्रुव controlSignalVarp,
                           ध्रुव std::string& आधारः_Signal_Name) ध्रुव {
        ध्रुव std::string आधारः_Signal_कुञ्चिका = get_कुञ्चिका_Name(scopep, आधारः_Signal_Name);
        ध्रुव std::map<ध्रुव std::string, Scope_Var_दत्त>::const_iterator आधारः_Signal_It
            = m_scopeVars.find(आधारः_Signal_कुञ्चिका);
        यदि (आधारः_Signal_It == m_scopeVars.अन्तः_2()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            प्रत्यागम असत्य;
        } अन्यथा {
            ध्रुव AstVar* ध्रुव आधारः_Signal_Varp = आधारः_Signal_It->second.m_varp;
            यदि (m_scopeVars.गणनम्(आधारः_Signal_Name) > 1) {
                आधारः_Signal_Varp->v3fatalSrc(
                    "Found Signal " << आधारः_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << आधारः_Signal_Name << " occurring in the same scope.");
                प्रत्यागम असत्य;
            }
            यदि (!आधारः_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << आधारः_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                प्रत्यागम असत्य;
            }
        }
        प्रत्यागम सत्य;
    }

    तार्किक आधारः_Signal_Is_लौकिकम्(ध्रुव AstScope* ध्रुव scopep,
                            ध्रुव std::string& आधारः_Signal_Name) ध्रुव {
        ध्रुव std::string आधारः_Signal_कुञ्चिका = get_कुञ्चिका_Name(scopep, आधारः_Signal_Name);
        ध्रुव std::map<ध्रुव std::string, Scope_Var_दत्त>::const_iterator आधारः_Signal_It
            = m_scopeVars.find(आधारः_Signal_कुञ्चिका);
        यदि (आधारः_Signal_It == m_scopeVars.अन्तः_2()) {
            प्रत्यागम असत्य;
        } अन्यथा {
            ध्रुव AstVar* ध्रुव आधारः_Signal_Varp = आधारः_Signal_It->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            यदि (!आधारः_Signal_Varp->is_Sig_लौकिकम्()) प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }

    तार्किक force_Control_सङ्केताः_Are_Valid(ध्रुव AstScope* ध्रुव scopep,
                                     ध्रुव AstVar* ध्रुव आधारः_Signal_Varp) ध्रुव {
        constexpr std::array<ध्रुव वर्ण*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        प्रत्यागम std::all_सम्बन्धि(
            forceControlSuffixes.begin(), forceControlSuffixes.अन्तः_2(),
            [scopep, आधारः_Signal_Varp, एतत्](ध्रुव std::string& forceControlSuffix) {
                ध्रुव std::string controlSignalName = आधारः_Signal_Varp->name() + forceControlSuffix;
                ध्रुव std::string control_Signal_कुञ्चिका = get_कुञ्चिका_Name(scopep, controlSignalName);
                ध्रुव std::आकार_t control_Signal_गणनम् = m_scopeVars.गणनम्(control_Signal_कुञ्चिका);
                यदि (control_Signal_गणनम् == 0) {
                    आधारः_Signal_Varp->v3fatalSrc("Signal "
                                               << आधारः_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_कुञ्चिका << "'.");
                    प्रत्यागम असत्य;
                }
                यदि (control_Signal_गणनम् > 1) {
                    आधारः_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << आधारः_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    प्रत्यागम असत्य;
                }
                प्रत्यागम सत्य;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    शून्य varHierarchyScopes(std::string scp) {

        std::string::आकार_type prd_pos = scp.rfind('.');
        std::string::आकार_type dot_pos = scp.rfind("__DOT__");

        यावत् (!scp.रिक्तानि()) {
            ध्रुव स्वचालित scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            यदि ((scpit != m_vpiScopeCandidates.अन्तः_2())
                && (m_scope_नामानि.find(scp) == m_scope_नामानि.अन्तः_2())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ध्रुव स्वचालित pair = m_scope_नामानि.emplace(scpit->second.m_symName, scpit->second);
                यदि (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            यदि ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } अन्यथा {
                यदि (prd_pos == string::npos) भङ्ग;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    शून्य सङ्कलय_Scope_Var_प्रविष्टिः(ध्रुव AstScope* ध्रुव scopep, ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp,
                          ध्रुव AstVar* ध्रुव varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string सम्पूर्णम् = scopep->name() + "__DOT__" + varp->name();
        यदि (VString::startsWith(सम्पूर्णम्, "__DOT__TOP")) सम्पूर्णम्.replace(0, 10, "");
        ध्रुव std::string::आकार_type dpos = सम्पूर्णम्.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ध्रुव std::string scpName = सम्पूर्णम्.substr(0, dpos);
        ध्रुव std::string var_आधारः = सम्पूर्णम्.substr(dpos + std::वाक्यदैर्घ्यम्("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ध्रुव std::string var_आधारः_Pretty = Ast_ग्रन्थिः::vpiName(VName::dehash(var_आधारः));
        ध्रुव std::string scpPretty = Ast_ग्रन्थिः::prettyName(VName::dehash(scpName));
        ध्रुव std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        यदि (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_नामानि.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_आधारः_Pretty, varp, modp, scopep));
    }

    शून्य varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        प्रति (ध्रुव ScopeModPair& smPair : m_scopes) {
            ध्रुव AstScope* ध्रुव scopep = smPair.first;
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव smodp = smPair.second;
            प्रति (ध्रुव ModVarPair& mvPair : m_modVars) {
                ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = mvPair.first;
                ध्रुव AstVar* ध्रुव varp = mvPair.second;
                यदि (modp != smodp) अनुवर्त;
                सङ्कलय_Scope_Var_प्रविष्टिः(scopep, modp, varp);
            }
        }
    }

    शून्य collectIfaceRefs(ध्रुव AstScope* nodep) {
        ध्रुव AstCell* ध्रुव cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        यदि (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) प्रत्यागम;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ध्रुव std::string मार्गः = Ast_ग्रन्थिः::vpiName(nodep->name());
        ध्रुव std::string instName = Ast_ग्रन्थिः::vpiName(cellp->origName());
        UASSERT_OBJ(मार्गः.दीर्घता_2() > instName.दीर्घता_2() && VString::अन्ताः_With(मार्गः, instName), nodep,
                    "Interface scope name " << मार्गः << " does not end with instance name "
                                            << instName);
        ध्रुव std::string parent_मार्गः = मार्गः.substr(0, मार्गः.दीर्घता_2() - instName.दीर्घता_2());

        प्रति (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ध्रुव std::string refName = Ast_ग्रन्थिः::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            यदि (!VString::startsWith(refName, parent_मार्गः)) अनुवर्त;
            m_ifaceRefs.emplace_पृष्ठभागः(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     Ast_ग्रन्थिः::vpiName(intfRefp->आधारः_Name()),
                                     intfRefp->modportName());
        }
    }

    शून्य build_Iface_Ref_सारणी() {
        यदि (m_ifaceRefs.रिक्तानि()) प्रत्यागम;
        ध्रुव std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_क्रमय(
            m_ifaceRefs.begin(), m_ifaceRefs.अन्तः_2(),
            [](ध्रुव Iface_Ref_दत्त& a, ध्रुव Iface_Ref_दत्त& b) { प्रत्यागम a.m_suffix < b.m_suffix; });
        प्रति (ध्रुव Iface_Ref_दत्त& ird : m_ifaceRefs) {
            ध्रुव std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            यदि (m_scope_नामानि.find(scopeSym) == m_scope_नामानि.अन्तः_2()) अनुवर्त;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parent_विस्थापनम् = "VL_IFACEREF_NO_PARENT";
            ध्रुव स्वचालित parent = m_scope_नामानि.find(ird.m_parentSym);
            यदि (parent != m_scope_नामानि.अन्तः_2()) {
                parent_विस्थापनम् = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parent_विस्थापनम् + ", \"";
            row += V3_बहिर्गमनम्_Formatter::quoteNameControls(VIdProtect::protect_शब्दाः_If(ird.m_name, सत्य));
            row += "\", \"";
            row += V3_बहिर्गमनम्_Formatter::quoteNameControls(
                VIdProtect::protect_शब्दाः_If(ird.m_suffix, सत्य));
            row += "\", \"";
            row += V3_बहिर्गमनम्_Formatter::quoteNameControls(
                VIdProtect::protect_शब्दाः_If(ird.m_modportName, सत्य));
            row += "\"}";
            m_iface_Ref_सारणी_पङ्क्तिकाः.emplace_पृष्ठभागः(std::move(row));
        }
        यदि (!m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) m_iface_Ref_सारणी_Name = symClass + "__VpiIfaceRefTable";
    }

    शून्य buildVpiHierarchy() {
        प्रति (ध्रुव स्वचालित& itpair : m_scope_नामानि) {
            ध्रुव std::string symName = itpair.second.m_symName;
            std::string above = symName;
            यदि (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            यावत् (!above.रिक्तानि()) {
                ध्रुव std::string::आकार_type pos = above.rfind("__");
                यदि (pos == std::string::npos) भङ्ग;
                above.resize(pos);
                यदि (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.अन्तः_2()) {
                    m_vpiScopeHierarchy[above].प्रक्षिप_पृष्ठभागः(symName);
                    भङ्ग;
                }
            }
            m_vpiScopeHierarchy[symName] = std::सदिशम्<string>();
        }
    }

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        यदि (v3Global.opt.vpi()) buildVpiHierarchy();

        यदि (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            प्रति (ध्रुव स्वचालित& scp : m_dpi_Scope_नामानि) m_scope_नामानि.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_क्रमय(m_scopes.begin(), m_scopes.अन्तः_2(),
                         [](ध्रुव ScopeModPair& a, ध्रुव ScopeModPair& b) {
                             प्रत्यागम a.first->name() < b.first->name();
                         });
        std::stable_क्रमय(m_dpis.begin(), m_dpis.अन्तः_2(),  //
                         [](ध्रुव AstCFunc* ap, ध्रुव AstCFunc* bp) {
                             यदि (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 प्रत्यागम bp->dpiImportPrototype();
                             }
                             प्रत्यागम ap->name() < bp->name();
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
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        यदि (nodep->isConstPool()) प्रत्यागम;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCellInlineScope* nodep) override {
        ध्रुव std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ध्रुव std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ध्रुव पूर्णाङ्क timeunit = m_modp->timeunit().power_सम्बन्धि_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ग्रन्थिः::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    शून्य दर्शय(AstScope* nodep) override {
        यदि (VN_IS(m_modp, Class)) प्रत्यागम;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_पृष्ठभागः(nodep, m_modp);

        यदि (v3Global.opt.vpi() && !nodep->is_शिखरम्()) {
            ध्रुव std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ध्रुव पूर्णाङ्क timeunit = m_modp->timeunit().power_सम्बन्धि_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_ग्रन्थिः::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstScopeName* nodep) override {
        यदि (nodep->selfRelative()) प्रत्यागम;  // Its absolute copy is under the scope
        ध्रुव std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ध्रुव पूर्णाङ्क timeunit = m_modp ? m_modp->timeunit().power_सम्बन्धि_Ten() : 0;
        m_dpi_Scope_नामानि.emplace(  //
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
            m_dpi_Scope_नामानि.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    शून्य दर्शय(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        यदि ((nodep->is_Sig_User_Rd_लौकिकम्() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().रिक्तानि()) {
            m_modVars.emplace_पृष्ठभागः(m_modp, nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        यदि (!nodep->दत्त_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_अङ्कः(m_coverBins);
            m_coverBins += nodep->आकार();
            nodep->local_Bin_अङ्कः(m_modCoverBins);
            m_modCoverBins += nodep->आकार();
        }
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        nameCheck(nodep);
        यदि (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.प्रक्षिप_पृष्ठभागः(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    शून्य दर्शय(AstConst*) override {}
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्पष्ट EmitCSyms(AstNetlist* nodep, तार्किक dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

शून्य EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_शीर्षकम्_File(symClassName(), "Symbol table internal header");
    पङ्क्तिं_लिख("//\n");
    पङ्क्तिं_लिख("// Internal details; most calling programs do not need this header,\n");
    पङ्क्तिं_लिख("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    पङ्क्तिं_लिख("\n");
    ofp()->puts_Int_शिखरम्_Include();
    पङ्क्तिं_लिख("#include \"verilated.h\"\n");
    यदि (v3Global.needTraceDumper()) {
        प्रति (ध्रुव string& आधारः : v3Global.opt.traceSourceLangs())
            पङ्क्तिं_लिख("#include \"" + आधारः + ".h\"\n");
    }
    यदि (v3Global.opt.उपयुङ्क्ते_Profiler()) पङ्क्तिं_लिख("#include \"verilated_profiler.h\"\n");

    पङ्क्तिं_लिख("\n// INCLUDE MODEL CLASS\n");
    पङ्क्तिं_लिख("\n#include \"" + शिखरम्_Class_Name() + ".h\"\n");

    पङ्क्तिं_लिख("\n// INCLUDE MODULE CLASSES\n");
    प्रति (Ast_ग्रन्थिः_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ग्रन्थिः_Module);
        यदि (VN_IS(nodep, Class)) अनुवर्त;  // Class included earlier
        यदि (nodep->isConstPool()) अनुवर्त;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::उपसर्गः_Name_Protect(nodep) + ".h\"\n");
    }

    यदि (v3Global.dpi()) {
        पङ्क्तिं_लिख("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::समुच्चयः<std::string> types;  // Remove duplicates and sort
        प्रति (ध्रुव स्वचालित& itpair : m_scopeFuncs) {
            ध्रुव AstCFunc* ध्रुव funcp = itpair.second.m_cfuncp;
            यदि (!funcp->dpiExportImpl()) अनुवर्त;
            ध्रुव std::string cbtype
                = protect(v3Global.opt.उपसर्गः() + "__Vcb_" + funcp->cname() + "_t");
            ध्रुव std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_तर्काः(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        प्रति (ध्रुव std::string& type : types) पङ्क्तिं_लिख(type);
    }

    पङ्क्तिं_लिख("\n// SYMS CLASS (contains all model state)\n");
    पङ्क्तिं_लिख("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_वैयक्तिकम्(असत्य);  // public:

    पङ्क्तिं_लिख("// INTERNAL STATE\n");
    पङ्क्तिं_लिख(शिखरम्_Class_Name() + "* const __Vm_modelp;\n");

    यदि (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        पङ्क्तिं_लिख("bool __Vm_dumping = false;  // Dumping is active\n");
        पङ्क्तिं_लिख("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        पङ्क्तिं_लिख(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    यदि (v3Global.opt.trace()) {
        पङ्क्तिं_लिख("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        पङ्क्तिं_लिख("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    यदि (v3Global.hasEvents()) {
        यदि (v3Global.assignsEvents()) {
            पङ्क्तिं_लिख("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } अन्यथा {
            पङ्क्तिं_लिख("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    यदि (v3Global.hasClasses()) पङ्क्तिं_लिख("VlDeleter __Vm_deleter;\n");
    पङ्क्तिं_लिख("bool& __Vm_didInit;\n");
    यदि (v3Global.opt.systemC()) पङ्क्तिं_लिख("sc_core::sc_event __Vm_wakeEvent;\n");

    यदि (v3Global.opt.mtasks()) {
        पङ्क्तिं_लिख("\n// MULTI-THREADING\n");
        पङ्क्तिं_लिख("VlThreadPool* __Vm_threadPoolp;\n");
        पङ्क्तिं_लिख("bool __Vm_even_cycle__ico = false;\n");
        पङ्क्तिं_लिख("bool __Vm_even_cycle__act = false;\n");
        पङ्क्तिं_लिख("bool __Vm_even_cycle__nba = false;\n");
    }

    यदि (v3Global.opt.profExec()) {
        पङ्क्तिं_लिख("\n// EXECUTION PROFILING\n");
        पङ्क्तिं_लिख("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    यदि (v3Global.opt.profPgo()) {
        पङ्क्तिं_लिख("\n// PGO PROFILING\n");
        पङ्क्तिं_लिख("VlPgoProfiler<" + std::गन्तव्यम्_string(ExecMTask::अङ्कः_प्रयुक्तम्_Ids()) + "> _vm_pgoProfiler;\n");
    }

    पङ्क्तिं_लिख("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    पङ्क्तिं_लिख("union {\nstruct {\n");
    प्रति (ध्रुव ScopeModPair& itpair : m_scopes) {
        ध्रुव AstScope* ध्रुव scopep = itpair.first;
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = itpair.second;
        यदि (VN_IS(modp, Class)) अनुवर्त;
        ofp()->प्रारूप_लिख("%-30s ", EmitCUtil::उपसर्गः_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    पङ्क्तिं_लिख("} vlInst;\n};\n");

    यदि (m_coverBins) {
        पङ्क्तिं_लिख("\n// COVERAGE\n");
        पङ्क्तिं_लिख(v3Global.opt.तन्तवः() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        पङ्क्तिं_लिख(" __Vcoverage[");
        पङ्क्तिं_लिख(std::गन्तव्यम्_string(m_coverBins));
        पङ्क्तिं_लिख("];\n");
    }

    यदि (!m_scope_नामानि.रिक्तानि()) {  // Scope names
        पङ्क्तिं_लिख("\n// SCOPE NAMES\n");
        प्रति (ध्रुव स्वचालित& itpair : m_scope_नामानि) {
            ध्रुव Scope_दत्त& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    यदि (v3Global.opt.vpi()) {
        पङ्क्तिं_लिख("\n// SCOPE HIERARCHY\n");
        पङ्क्तिं_लिख("VerilatedHierarchy __Vhier;\n");
    }

    पङ्क्तिं_लिख("\n// CONSTRUCTORS\n");
    पङ्क्तिं_लिख(symClassName() + "(VerilatedContext* contextp, const char* namep, " + शिखरम्_Class_Name()
         + "* modelp);\n");
    पङ्क्तिं_लिख("~" + symClassName() + "();\n");

    पङ्क्तिं_लिख("\n// METHODS\n");
    पङ्क्तिं_लिख("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    यदि (v3Global.hasEvents()) {
        यदि (v3Global.assignsEvents()) {
            पङ्क्तिं_लिख("void fireEvent(VlAssignableEvent& event) {\n");
        } अन्यथा {
            पङ्क्तिं_लिख("void fireEvent(VlEvent& event) {\n");
        }
        पङ्क्तिं_लिख("if (VL_LIKELY(!event.isTriggered())) {\n");
        यदि (v3Global.assignsEvents()) {
            पङ्क्तिं_लिख("__Vm_triggeredEvents.push_back(event);\n");
        } अन्यथा {
            पङ्क्तिं_लिख("__Vm_triggeredEvents.push_back(&event);\n");
        }
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("event.fire();\n");
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("void clearTriggeredEvents() {\n");
        यदि (v3Global.assignsEvents()) {
            पङ्क्तिं_लिख("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } अन्यथा {
            पङ्क्तिं_लिख("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        पङ्क्तिं_लिख("__Vm_triggeredEvents.clear();\n");
        पङ्क्तिं_लिख("}\n");
    }

    यदि (v3Global.needTraceDumper()) {
        यदि (!optSystemC()) पङ्क्तिं_लिख("void _traceDump();\n");
        पङ्क्तिं_लिख("void _traceDumpOpen();\n");
        पङ्क्तिं_लिख("void _traceDumpClose();\n");
    }

    यदि (v3Global.opt.savable()) {
        पङ्क्तिं_लिख("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        पङ्क्तिं_लिख("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    पङ्क्तिं_लिख("};\n");

    ofp()->puts_अन्तः_Guard();
    closeOutputFile();
}

शून्य EmitCSyms::emitSymImpPreamble() {
    पङ्क्तिं_लिख("\n");

    // Includes
    पङ्क्तिं_लिख("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    पङ्क्तिं_लिख("\n");
    // Declarations for DPI Export implementation functions
    तार्किक needs_New_पङ्क्तिः = असत्य;
    प्रति (ध्रुव स्वचालित& itpair : m_scopeFuncs) {
        ध्रुव AstCFunc* ध्रुव funcp = itpair.second.m_cfuncp;
        यदि (!funcp->dpiExportImpl()) अनुवर्त;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needs_New_पङ्क्तिः = सत्य;
    }
    यदि (needs_New_पङ्क्तिः) पङ्क्तिं_लिख("\n");
    // Declarations of the Verilator library functions called at construction
    std::समुच्चयः<std::string> lib_नामानि;
    प्रति (ध्रुव ScopeModPair& itpair : m_scopes) {
        यदि (itpair.second->verilatorLib()) lib_नामानि.emplace(itpair.second->origName());
    }
    प्रति (ध्रुव std::string& libName : lib_नामानि) {
        पङ्क्तिं_लिख("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        पङ्क्तिं_लिख("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    यदि (!lib_नामानि.रिक्तानि()) पङ्क्तिं_लिख("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    यदि (!m_varTables.रिक्तानि() || !m_scope_सारणी_पङ्क्तिकाः.रिक्तानि() || !m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) {
        प्रति (ध्रुव स्वचालित& kv : m_varTables) {
            पङ्क्तिं_लिख("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        यदि (!m_scope_सारणी_पङ्क्तिकाः.रिक्तानि()) {
            पङ्क्तिं_लिख("extern const VlScopeTableEntry " + m_scope_सारणी_Name + "[];\n");
        }
        यदि (!m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) {
            पङ्क्तिं_लिख("extern const VlIfaceRefTableEntry " + m_iface_Ref_सारणी_Name + "[];\n");
        }
        पङ्क्तिं_लिख("\n");
    }
}

शून्य EmitCSyms::emitVarTables() {
    यदि (m_varTables.रिक्तानि() && m_scope_सारणी_पङ्क्तिकाः.रिक्तानि() && m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) प्रत्यागम;
    पङ्क्तिं_लिख("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    पङ्क्तिं_लिख("#if defined(__GNUC__)\n");
    पङ्क्तिं_लिख("# pragma GCC diagnostic push\n");
    पङ्क्तिं_लिख("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    पङ्क्तिं_लिख("#endif\n");
    प्रति (ध्रुव स्वचालित& kv : m_varTables) {
        पङ्क्तिं_लिख("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        प्रति (ध्रुव std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        पङ्क्तिं_लिख("};\n");
    }
    यदि (!m_scope_सारणी_पङ्क्तिकाः.रिक्तानि()) {
        पङ्क्तिं_लिख("extern const VlScopeTableEntry " + m_scope_सारणी_Name + "[] = {\n");
        प्रति (ध्रुव std::string& row : m_scope_सारणी_पङ्क्तिकाः) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        पङ्क्तिं_लिख("};\n");
    }
    यदि (!m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) {
        पङ्क्तिं_लिख("extern const VlIfaceRefTableEntry " + m_iface_Ref_सारणी_Name + "[] = {\n");
        प्रति (ध्रुव std::string& row : m_iface_Ref_सारणी_पङ्क्तिकाः) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        पङ्क्तिं_लिख("};\n");
    }
    पङ्क्तिं_लिख("#if defined(__GNUC__)\n");
    पङ्क्तिं_लिख("# pragma GCC diagnostic pop\n");
    पङ्क्तिं_लिख("#endif\n");
}

शून्य EmitCSyms::emitScopeHier(TextStmts& stmts, तार्किक destroy) {
    यदि (!v3Global.opt.vpi()) प्रत्यागम;

    यदि (destroy) {
        stmts.सङ्कलय("// Tear down scope hierarchy");
    } अन्यथा {
        stmts.सङ्कलय("// Set up scope hierarchy");
    }

    ध्रुव std::string विधानम् = destroy ? "remove" : "add";
    प्रति (ध्रुव स्वचालित& itpair : m_scope_नामानि) {
        यदि (itpair.first == "TOP") अनुवर्त;
        ध्रुव Scope_दत्त& sd = itpair.second;
        ध्रुव std::string& name = sd.m_prettyName;
        ध्रुव std::string& scopeType = sd.m_type;
        यदि (name.find('.') != string::npos) अनुवर्त;
        यदि (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") अनुवर्त;
        ध्रुव std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.सङ्कलय("vlSymsp->__Vhier." + विधानम् + "(0, " + id + ");");
    }
    प्रति (ध्रुव स्वचालित& itpair : m_vpiScopeHierarchy) {
        ध्रुव std::string fromName = scopeSymString(itpair.first);
        ध्रुव std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_नामानि.at(fromName).m_symName);
        प्रति (ध्रुव std::string& name : itpair.second) {
            ध्रुव std::string गन्तव्यम्_Name = scopeSymString(name);
            ध्रुव std::string गन्तव्यम्_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_नामानि.at(गन्तव्यम्_Name).m_symName);
            stmts.सङ्कलय("vlSymsp->__Vhier." + विधानम् + "(" + fromId + ", " + गन्तव्यम्_Id + ");");
        }
    }

    यदि (destroy) {
        stmts.सङ्कलय("// Clear keys from hierarchy map after values have been removed");
        stmts.सङ्कलय("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t राशिः_आकार = V3_राशिः_गणनम्::गणनम्(v3Global.rootp());
        यदि (v3Global.opt.debug_राशिः_Check()) राशिः_आकार += 1024 * 1024 * 1024;
        V3Stats::सङ्कलय_Stat("Size prediction, Stack (bytes)", राशिः_आकार);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::सङ्कलय_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_अष्टकानि);
        V3Stats::सङ्कलय_Stat(V3Stats::STAT_MODEL_आकार, राशिः_आकार + m_stat_Var_Scope_अष्टकानि);

        stmts.सङ्कलय("// Check resources");
        stmts.सङ्कलय("Verilated::stackCheck(" + std::गन्तव्यम्_string(राशिः_आकार) + ");");
    }

    stmts.सङ्कलय("// Setup sub module instances");
    प्रति (ध्रुव ScopeModPair& itpair : m_scopes) {
        ध्रुव AstScope* ध्रुव scopep = itpair.first;
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = itpair.second;
        यदि (modp->is_शिखरम्()) अनुवर्त;
        ध्रुव std::string name = V3_बहिर्गमनम्_Formatter::quoteNameControls(
            VIdProtect::protect_शब्दाः_If(scopep->prettyName(), scopep->protect()));
        stmts.सङ्कलय("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    यदि (v3Global.opt.profPgo()) {
        stmts.सङ्कलय("// Configure profiling for PGO\n");
        यदि (!v3Global.opt.hierChild()) {
            stmts.सङ्कलय("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        यदि (v3Global.opt.mtasks()) {
            v3Global.rootp()->शिखरम्_Modulep()->foreach([&](ध्रुव AstExecGraph* execGraphp) {
                प्रति (ध्रुव V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    ध्रुव ExecMTask& mt = static_cast<ध्रुव ExecMTask&>(vtx);
                    stmts.सङ्कलय("vlSymsp->_vm_pgoProfiler.addCounter(" + std::गन्तव्यम्_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.सङ्कलय("// Configure time unit / time precision");
    यदि (!v3Global.rootp()->timeunit().isNone()) {
        ध्रुव std::string unit = std::गन्तव्यम्_string(v3Global.rootp()->timeunit().power_सम्बन्धि_Ten());
        stmts.सङ्कलय("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    यदि (!v3Global.rootp()->timeprecision().isNone()) {
        ध्रुव std::string prec = std::गन्तव्यम्_string(v3Global.rootp()->timeprecision().power_सम्बन्धि_Ten());
        stmts.सङ्कलय("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.सङ्कलय("// Setup each module's pointers to their submodules");
    प्रति (ध्रुव स्वचालित& i : m_scopes) {
        ध्रुव AstScope* ध्रुव scopep = i.first;
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = i.second;
        ध्रुव AstScope* ध्रुव abovep = scopep->aboveScopep();
        यदि (!abovep) अनुवर्त;

        ध्रुव std::string protName = VIdProtect::protect_शब्दाः_If(scopep->name(), scopep->protect());
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
        stmts.सङ्कलय(stmt);
    }

    यदि (v3Global.opt.coverage()) {
        stmts.सङ्कलय("// Setup each module's pointer back to symbol table (for public functions)");
        प्रति (ध्रुव ScopeModPair& i : m_scopes) {
            ध्रुव AstScope* ध्रुव scopep = i.first;
            Ast_ग्रन्थिः_Module* ध्रुव modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ध्रुव तार्किक first = !modp->user1();
            modp->user1(सत्य);
            stmts.सङ्कलय("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    प्रति (ध्रुव ScopeModPair& i : m_scopes) {
        ध्रुव AstScope* ध्रुव scopep = i.first;
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = i.second;
        यदि (!modp->verilatorLib()) अनुवर्त;
        ध्रुव AstVar* hashp = शून्यसूचकः;
        ध्रुव AstVar* handlep = शून्यसूचकः;
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (!varp) अनुवर्त;
            यदि (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            यदि (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ध्रुव uint32_t hash = VN_AS(hashp->valuep(), Const)->गन्तव्यम्_UInt();
        stmts.सङ्कलय(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::गन्तव्यम्_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string मार्गः = scopep->prettyName();
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            मार्गः.erase(0, v3Global.rootp()->trace_Lib_शिखरम्_Name().आकार() + 1);
        }
        stmts.सङ्कलय("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_बहिर्गमनम्_Formatter::quoteNameControls(मार्गः) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    यदि (!m_scope_नामानि.रिक्तानि()) {
        stmts.सङ्कलय("// Setup scopes");
        ध्रुव std::string symClass = symClassName();
        प्रति (ध्रुव स्वचालित& itpair : m_scope_नामानि) {
            ध्रुव Scope_दत्त& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3_बहिर्गमनम्_Formatter::quoteNameControls(
                VIdProtect::protect_शब्दाः_If(sd.m_prettyName, सत्य));
            row += "\", \"";
            row += V3_बहिर्गमनम्_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            row += "\", \"";
            row += V3_बहिर्गमनम्_Formatter::quoteNameControls(sd.m_defName);
            row += "\", ";
            row += std::गन्तव्यम्_string(sd.m_timeunit);
            row += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_सारणी_पङ्क्तिकाः.emplace_पृष्ठभागः(std::move(row));
        }
        m_scope_सारणी_Name = symClass + "__VpiScopeTable";
        stmts.सङ्कलय("VerilatedScope::scopesConstructFromTable(" + m_scope_सारणी_Name + ", "
                  + std::गन्तव्यम्_string(m_scope_नामानि.आकार()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_सारणी();
    यदि (!m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) {
        stmts.सङ्कलय("// Setup interface references");
        stmts.सङ्कलय("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_सारणी_Name + ", "
                  + std::गन्तव्यम्_string(m_iface_Ref_सारणी_पङ्क्तिकाः.आकार()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, असत्य);

    यदि (v3Global.dpi()) {
        प्रति (ध्रुव std::string vfinal : {"0", "1"}) {
            stmts.सङ्कलय("// Setup export functions - final: " + vfinal);
            प्रति (ध्रुव स्वचालित& itpair : m_scopeFuncs) {
                ध्रुव Scope_Func_दत्त& sfd = itpair.second;
                ध्रुव AstScopeName* ध्रुव scopep = sfd.m_scopep;
                ध्रुव AstCFunc* ध्रुव funcp = sfd.m_cfuncp;
                ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = sfd.m_modp;
                यदि (!funcp->dpiExportImpl()) अनुवर्त;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_बहिर्गमनम्_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::उपसर्गः_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.सङ्कलय(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    यदि (!m_scopeVars.रिक्तानि()) {
        stmts.सङ्कलय("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> सारणी_By_पङ्क्तिकाः;
        पूर्णाङ्क सारणी_गणक = 0;

        स्वचालित it = m_scopeVars.cbegin();
        यावत् (it != m_scopeVars.cend()) {
            ध्रुव std::string scopeName = it->second.m_scopeName;
            ध्रुव AstScope* ध्रुव instScopep = it->second.m_scopep;
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = it->second.m_modp;
            ध्रुव std::string modClassName = EmitCUtil::उपसर्गः_Name_Protect(modp);

            std::सदिशम्<std::string> पङ्क्तिकाः;
            std::सदिशम्<std::string> residual;

            प्रति (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                ध्रुव Scope_Var_दत्त& svd = it->second;
                ध्रुव AstScope* ध्रुव scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ध्रुव AstVar* ध्रुव varp = svd.m_varp;
                ध्रुव VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ध्रुव std::pair<तार्किक, std::string> fc = isForceControlSignal(varp);
                यदि (fc.first && आधारः_Signal_Is_लौकिकम्(scopep, fc.second)
                    && आधारः_Signal_Is_Valid(scopep, varp, fc.second)) {
                    अनुवर्त;
                }

                std::string row;
                ध्रुव सारणी_प्रविष्टिः_विधा विधा
                    = try_Build_सारणी_प्रविष्टिः(svd, varp, scopep, modClassName, dims, row);
                विकल्प (विधा) {
                पक्ष सारणी_प्रविष्टिः_विधा::सारणी_ROW:
                    पङ्क्तिकाः.emplace_पृष्ठभागः(row);
                    सङ्कलय_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_आधारः_Pretty,
                                           varName(varp), varp->dtypep());
                    भङ्ग;
                पक्ष सारणी_प्रविष्टिः_विधा::FORCEABLE_RESIDUAL: {
                    ध्रुव std::string bounds = boundsString(dims);
                    residual.emplace_पृष्ठभागः(निवेशय_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    भङ्ग;
                }
                पक्ष सारणी_प्रविष्टिः_विधा::PLAIN_RESIDUAL: {
                    ध्रुव std::string bounds = boundsString(dims);
                    residual.emplace_पृष्ठभागः(
                        निवेशय_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    भङ्ग;
                }
                स्वभाव: v3fatalSrc("Bad case");
                }
                यदि (विधा == सारणी_प्रविष्टिः_विधा::सारणी_ROW) अनुवर्त;
                यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ग्रन्थिः_UOr_Struct_DType)) {
                    यदि (!sdtypep->packed()) {
                        सङ्कलय_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_आधारः_Pretty,
                                               varName(varp), sdtypep);
                    }
                } अन्यथा यदि (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    सङ्कलय_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_आधारः_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                यदि (विधा == सारणी_प्रविष्टिः_विधा::PLAIN_RESIDUAL) {
                    सङ्कलय_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_आधारः_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            यदि (!पङ्क्तिकाः.रिक्तानि()) {
                ध्रुव आकार_t row_गणनम् = पङ्क्तिकाः.आकार();
                std::string कुञ्चिका;
                प्रति (ध्रुव std::string& r : पङ्क्तिकाः) {
                    कुञ्चिका += r;
                    कुञ्चिका += '\0';
                }
                ध्रुव स्वचालित itt = सारणी_By_पङ्क्तिकाः.find(कुञ्चिका);
                std::string सारणी_Name;
                यदि (itt != सारणी_By_पङ्क्तिकाः.अन्तः_2()) {
                    सारणी_Name = itt->second;
                } अन्यथा {
                    सारणी_Name = modClassName + "__VpiVarTable" + std::गन्तव्यम्_string(सारणी_गणक++);
                    सारणी_By_पङ्क्तिकाः.emplace(std::move(कुञ्चिका), सारणी_Name);
                    m_varTables.emplace_पृष्ठभागः(सारणी_Name, std::move(पङ्क्तिकाः));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + सारणी_Name + ", "
                                   + std::गन्तव्यम्_string(row_गणनम्) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.सङ्कलय(call);
            }
            प्रति (ध्रुव std::string& r : residual) stmts.सङ्कलय(r);
        }
    }

    प्रत्यागम stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, सत्य);
    यदि (v3Global.needTraceDumper()) {
        stmts.सङ्कलय("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    यदि (v3Global.opt.profPgo()) {
        stmts.सङ्कलय("vlSymsp->_vm_pgoProfiler.write(\"" + शिखरम्_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    यदि (!m_iface_Ref_सारणी_पङ्क्तिकाः.रिक्तानि()) {
        stmts.सङ्कलय("// Tear down interface references");
        stmts.सङ्कलय("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_सारणी_Name + ", "
                  + std::गन्तव्यम्_string(m_iface_Ref_सारणी_पङ्क्तिकाः.आकार()) + ", vlSymsp);");
    }

    stmts.सङ्कलय("// Tear down scopes");
    प्रति (ध्रुव स्वचालित& itpair : m_scope_नामानि) {
        ध्रुव Scope_दत्त& sd = itpair.second;
        ध्रुव std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.सङ्कलय("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.सङ्कलय("// Tear down sub module instances");
    प्रति (ध्रुव ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        ध्रुव AstScope* ध्रुव scopep = itpair.first;
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = itpair.second;
        यदि (modp->is_शिखरम्()) अनुवर्त;
        stmts.सङ्कलय("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    प्रत्यागम stmts;
}

शून्य EmitCSyms::emitSymImp(ध्रुव AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ध्रुव std::pair<std::string, std::string> विभज_Ctor_Dtor_Decls  //
        = emit_विभज_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            सत्य, "Symbol table implementation internals",
                            [एतत्]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), सत्य, सत्य, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ध्रुव AstScope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep()->scopep();
    ध्रुव std::string शिखरम्_Inst = "vlSymsp->" + instName(शिखरम्_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    यदि (!विभज_Ctor_Dtor_Decls.first.रिक्तानि()) पङ्क्तिं_लिख("\n" + विभज_Ctor_Dtor_Decls.first + "\n");
    पङ्क्तिं_लिख("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    पङ्क्तिं_लिख("// Setup top module instance\n");
    पङ्क्तिं_लिख("VL_INPLACE_CONSTRUCT(" + शिखरम्_Inst + ", vlSymsp, namep);\n");
    प्रति (ध्रुव TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    पङ्क्तिं_लिख("}\n");

    // Constructor
    पङ्क्तिं_लिख("\n");
    पङ्क्तिं_लिख(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + शिखरम्_Class_Name() + "* modelp)\n");
    पङ्क्तिं_लिख("    : VerilatedSyms{contextp}\n");
    पङ्क्तिं_लिख("    // Setup internal state of the Syms class\n");
    पङ्क्तिं_लिख("    , __Vm_modelp{modelp}\n");
    पङ्क्तिं_लिख("    , __Vm_didInit{modelp->m_didInit}\n");
    यदि (v3Global.opt.mtasks()) {
        पङ्क्तिं_लिख("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    यदि (v3Global.opt.profExec()) {
        पङ्क्तिं_लिख("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    यदि (v3Global.opt.profPgo() && !v3Global.opt.libCreate().रिक्तानि()) {
        पङ्क्तिं_लिख("    , _vm_pgoProfiler{" + std::गन्तव्यम्_string(v3Global.current_Hier_खण्डम्_Cost()) + "}\n");
    }
    पङ्क्तिं_लिख("{\n");
    पङ्क्तिं_लिख(symClassName() + "__ctor(this, namep);\n");
    पङ्क्तिं_लिख("}\n");

    // Destructor contents, in a loose function, see constructor
    यदि (!विभज_Ctor_Dtor_Decls.second.रिक्तानि()) पङ्क्तिं_लिख("\n" + विभज_Ctor_Dtor_Decls.second + "\n");
    पङ्क्तिं_लिख("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    प्रति (ध्रुव TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    पङ्क्तिं_लिख("// Tear down top module instance\n");
    पङ्क्तिं_लिख("VL_INPLACE_DESTRUCT(" + शिखरम्_Inst + ");\n");
    पङ्क्तिं_लिख("}\n");

    // Destructor
    पङ्क्तिं_लिख("\n");
    पङ्क्तिं_लिख(symClassName() + "::~" + symClassName() + "() {\n");
    पङ्क्तिं_लिख(symClassName() + "__dtor(this);\n");
    पङ्क्तिं_लिख("}\n");

    // Methods
    यदि (v3Global.needTraceDumper()) {
        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("\nvoid " + symClassName() + "::_traceDump() {\n");
            पङ्क्तिं_लिख("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
            // Caller checked for __Vm_dumperp non-nullptr
            पङ्क्तिं_लिख("__Vm_dumperp->dump(VL_TIME_Q());\n");
            पङ्क्तिं_लिख("}\n");
        }

        पङ्क्तिं_लिख("\nvoid " + symClassName() + "::_traceDumpOpen() {\n");
        पङ्क्तिं_लिख("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
        पङ्क्तिं_लिख("if (VL_UNLIKELY(!__Vm_dumperp)) {\n");
        पङ्क्तिं_लिख("__Vm_dumperp = new " + v3Global.opt.traceClassLang() + "();\n");
        पङ्क्तिं_लिख("__Vm_modelp->trace(__Vm_dumperp, 0, 0);\n");
        पङ्क्तिं_लिख("const std::string dumpfile = _vm_contextp__->dumpfileCheck();\n");
        पङ्क्तिं_लिख("__Vm_dumperp->open(dumpfile.c_str());\n");
        पङ्क्तिं_लिख("__Vm_dumping = true;\n");
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("}\n");

        पङ्क्तिं_लिख("\nvoid " + symClassName() + "::_traceDumpClose() {\n");
        पङ्क्तिं_लिख("const VerilatedLockGuard lock{__Vm_dumperMutex};\n");
        पङ्क्तिं_लिख("__Vm_dumping = false;\n");
        पङ्क्तिं_लिख("VL_DO_CLEAR(delete __Vm_dumperp, __Vm_dumperp = nullptr);\n");
        पङ्क्तिं_लिख("}\n");
    }

    यदि (v3Global.opt.savable()) {
        प्रति (ध्रुव तार्किक& de : {असत्य, सत्य}) {
            ध्रुव std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ध्रुव std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ध्रुव std::string op = de ? ">>" : "<<";
            पङ्क्तिं_लिख("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            पङ्क्तिं_लिख("// Internal state\n");
            यदि (v3Global.opt.trace()) पङ्क्तिं_लिख("os" + op + "__Vm_activity;\n");
            पङ्क्तिं_लिख("os " + op + " __Vm_didInit;\n");
            पङ्क्तिं_लिख("// Module instance state\n");
            प्रति (ध्रुव ScopeModPair& itpair : m_scopes) {
                ध्रुव AstScope* ध्रुव scopep = itpair.first;
                पङ्क्तिं_लिख(instName(scopep) + "." + funcname + "(os);\n");
            }
            पङ्क्तिं_लिख("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

शून्य EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    open_New_Output_शीर्षकम्_File(शिखरम्_Class_Name() + "__Dpi",
                            "Prototypes for DPI import and export functions.");
    पङ्क्तिं_लिख("//\n");
    पङ्क्तिं_लिख("// Verilator includes this file in all generated .cpp files that use DPI functions.\n");
    पङ्क्तिं_लिख("// Manually include this file where DPI .c import functions are declared to ensure\n");
    पङ्क्तिं_लिख("// the C functions match the expectations of the DPI imports.\n");

    ofp()->putsGuard();

    पङ्क्तिं_लिख("\n");
    पङ्क्तिं_लिख("#include \"svdpi.h\"\n");
    पङ्क्तिं_लिख("\n");

    // Unpacked structures passed as C structures (IEEE 1800-2023 H.7.8)
    std::समुच्चयः<std::string> struct_नामानि;
    प्रति (ध्रुव AstCFunc* ध्रुव nodep : m_dpis) {
        प्रति (ध्रुव Ast_ग्रन्थिः* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_नामानि);
            }
        }
    }
    यदि (!struct_नामानि.रिक्तानि()) पङ्क्तिं_लिख("\n");

    पङ्क्तिं_लिख("#ifdef __cplusplus\n");
    पङ्क्तिं_लिख("extern \"C\" {\n");
    पङ्क्तिं_लिख("#endif\n");
    पङ्क्तिं_लिख("\n");

    पूर्णाङ्क firstExp = 0;
    पूर्णाङ्क firstImp = 0;
    प्रति (ध्रुव AstCFunc* ध्रुव nodep : m_dpis) {
        यदि (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) अनुवर्त;

        ध्रुव std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        यदि (nodep->dpiExportDispatcher()) {
            यदि (!firstExp++) पङ्क्तिं_लिख("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } अन्यथा {
            यदि (!firstImp++) पङ्क्तिं_लिख("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        यदि (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } अन्यथा {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_तर्काः(nodep, असत्य) + ");\n");
        }
    }

    पङ्क्तिं_लिख("\n");
    पङ्क्तिं_लिख("#ifdef __cplusplus\n");
    पङ्क्तिं_लिख("}\n");
    पङ्क्तिं_लिख("#endif\n");

    ofp()->puts_अन्तः_Guard();

    closeOutputFile();
}

शून्य EmitCSyms::emitDpiStruct(ध्रुव Ast_ग्रन्थिः_DType* dtypep, std::समुच्चयः<std::string>& namesr) {
    यदि (!dtypep->अन्तर्भवति_Unpacked_Struct()) प्रत्यागम;
    ध्रुव AstStructDType* ध्रुव sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ध्रुव std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    यदि (namesr.गणनम्(name)) प्रत्यागम;
    // After the structures it contains
    प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    यदि (name.रिक्तानि()) प्रत्यागम;  // Declared where used
    यदि (namesr.रिक्तानि()) पङ्क्तिं_लिख("// DPI TYPES\n");
    namesr.निवेशय(name);
    // Guarded, so the user, or another model, may declare it instead
    पङ्क्तिं_लिख("#ifndef VL_DPITYPE_" + name + "_\n");
    पङ्क्तिं_लिख("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    पङ्क्तिं_लिख("#endif\n");
}

//######################################################################

शून्य EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(शिखरम्_Class_Name() + "__Dpi", असत्य, सत्य,
                            "Implementation of DPI export functions");
    पङ्क्तिं_लिख("//\n");
    पङ्क्तिं_लिख("// Verilator compiles this file in when DPI functions are used.\n");
    पङ्क्तिं_लिख("// If you have multiple Verilated designs with the same DPI exported\n");
    पङ्क्तिं_लिख("// function names, you will get multiple definition link errors from here.\n");
    पङ्क्तिं_लिख("// This is an unfortunate result of the DPI specification.\n");
    पङ्क्तिं_लिख("// To solve this, either\n");
    पङ्क्तिं_लिख("//    1. Call " + शिखरम्_Class_Name() + "::{export_function} instead,\n");
    पङ्क्तिं_लिख("//       and do not even bother to compile this file\n");
    पङ्क्तिं_लिख("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    पङ्क्तिं_लिख("//       and #ifdefs already inserted here will sort everything out.\n");
    पङ्क्तिं_लिख("\n");

    पङ्क्तिं_लिख("#include \"" + शिखरम्_Class_Name() + "__Dpi.h\"\n");
    पङ्क्तिं_लिख("#include \"" + शिखरम्_Class_Name() + ".h\"\n");
    पङ्क्तिं_लिख("\n");

    प्रति (ध्रुव AstCFunc* ध्रुव nodep : m_dpis) {
        यदि (!nodep->dpiExportDispatcher()) अनुवर्त;

        ध्रुव std::string name = nodep->name();
        ध्रुव std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        पङ्क्तिं_लिख("#ifndef VL_DPIDECL_" + name + "_\n");
        पङ्क्तिं_लिख("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_तर्काः(nodep) + ") {\n");
        पङ्क्तिं_लिख("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + शिखरम्_Class_Name() + "::" + name + "(");
        std::string comma;
        प्रति (ध्रुव Ast_ग्रन्थिः* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO() && !portp->isFuncReturn()) {
                    पङ्क्तिं_लिख(comma);
                    comma = ", ";
                    putns(portp, portp->name());
                }
            }
        }
        पङ्क्तिं_लिख(");\n");
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("#endif\n");
        पङ्क्तिं_लिख("\n");
    }
    closeOutputFile();
}

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcSyms(तार्किक dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
