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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitC.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3ExecGraph.h"
#అన్తర్భావ "V3LanguageWords.h"
#అన్తర్భావ "V3StackCount.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cstring>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
స్థిర constexpr స్వయంచాలక symClassName = &EmitCUtil::symClassName;
స్థిర constexpr స్వయంచాలక శిఖరం_Class_Name = &EmitCUtil::శిఖరం_Class_Name;

తరగతి EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // TYPES
    నిర్మాణం Scope_దత్తాంశం final {
        స్థిరరాశి Ast_కణుపు* m_nodep;  // Scope symbol-table entry corresponds to
        స్థిరరాశి std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        స్థిరరాశి std::string m_prettyName;  // Pretty (unprotected) name for display
        స్థిరరాశి std::string m_defName;  // Name registered for this scope
        స్థిరరాశి పూర్ణాంకం m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_దత్తాంశం(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి std::string& symName, స్థిరరాశి std::string& prettyName,
                  స్థిరరాశి std::string& defName, పూర్ణాంకం timeunit, స్థిరరాశి std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    నిర్మాణం Iface_Ref_దత్తాంశం final {
        స్థిరరాశి AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_మాతృ_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_దత్తాంశం(స్థిరరాశి AstScope* scopep, స్థిరరాశి std::string& మాతృ_Sym,
                     స్థిరరాశి std::string& suffix, స్థిరరాశి std::string& name,
                     స్థిరరాశి std::string& modportName)
            : m_scopep{scopep}
            , m_మాతృ_Sym{మాతృ_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    నిర్మాణం Scope_Func_దత్తాంశం final {
        స్థిరరాశి AstScopeName* స్థిరరాశి m_scopep;  // Scope the DPI export function is registered under
        స్థిరరాశి AstCFunc* స్థిరరాశి m_cfuncp;  // DPI export function
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Module containing the DPI export function
        Scope_Func_దత్తాంశం(స్థిరరాశి AstScopeName* scopep, స్థిరరాశి AstCFunc* funcp, స్థిరరాశి Ast_కణుపు_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    నిర్మాణం Scope_Var_దత్తాంశం final {
        స్థిరరాశి std::string m_scopeName;  // Full name of the scope containing the variable
        స్థిరరాశి std::string m_var_పునాది_Pretty;  // Pretty base name of the variable, without scope
        స్థిరరాశి AstVar* స్థిరరాశి m_varp;  // Public variable
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Module containing the variable
        స్థిరరాశి AstScope* స్థిరరాశి m_scopep;  // Scope containing the variable
        Scope_Var_దత్తాంశం(స్థిరరాశి std::string& scopeName, స్థిరరాశి std::string& var_పునాది_Pretty,
                     స్థిరరాశి AstVar* varp, స్థిరరాశి Ast_కణుపు_Module* modp, స్థిరరాశి AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_పునాది_Pretty{var_పునాది_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    ఉపయోగించి Scope_పేర్లు = std::map<స్థిరరాశి std::string, Scope_దత్తాంశం>;
    ఉపయోగించి ScopeModPair = std::pair<స్థిరరాశి AstScope*, Ast_కణుపు_Module*>;
    ఉపయోగించి ModVarPair = std::pair<స్థిరరాశి Ast_కణుపు_Module*, స్థిరరాశి AstVar*>;

    // Vars with more dims take the residual per-statement path.
    స్థిర constexpr పూర్ణాంకం VPI_పట్టిక_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current function
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    స్థిరరాశి AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    std::సదిశ<ScopeModPair> m_scopes;  // Every scope by module
    std::సదిశ<AstCFunc*> m_dpis;  // DPI functions
    std::సదిశ<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<స్థిరరాశి std::string, Scope_Func_దత్తాంశం> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<స్థిరరాశి std::string, Scope_Var_దత్తాంశం> m_scopeVars;  // Each {scope,public-var}
    Scope_పేర్లు m_scope_పేర్లు;  // Each unique AstScopeName. Dpi scopes added later
    Scope_పేర్లు m_dpi_Scope_పేర్లు;  // Each unique AstScopeName for DPI export
    Scope_పేర్లు m_vpiScopeCandidates;  // All scopes for VPI
    std::సదిశ<Iface_Ref_దత్తాంశం> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<స్థిరరాశి std::string, std::సదిశ<std::string>> m_vpiScopeHierarchy;
    పూర్ణాంకం m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    పూర్ణాంకం m_modCoverBins = 0;  // Per-module coverage bin number
    స్థిరరాశి తార్కిక m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_బైట్లు;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::సదిశ<std::pair<std::string, std::సదిశ<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_పట్టిక_Name;
    std::సదిశ<std::string> m_scope_పట్టిక_అడ్డువరుసలు;
    std::string m_iface_Ref_పట్టిక_Name;
    std::సదిశ<std::string> m_iface_Ref_పట్టిక_అడ్డువరుసలు;

    // METHODS
    శూన్య emitSymHdr();
    శూన్య emitSymImpPreamble();
    శూన్య emitVarTables();
    శూన్య emitScopeHier(పాఠ్యం_Stmts& stmts, తార్కిక destroy);
    శూన్య emitSymImp(స్థిరరాశి AstNetlist* netlistp);
    శూన్య emitDpiHdr();
    శూన్య emitDpiStruct(స్థిరరాశి Ast_కణుపు_DType* dtypep, std::సమితి<std::string>& namesr);
    శూన్య emitDpiImp();

    పాఠ్యం_Stmts getSymCtorStmts();
    పాఠ్యం_Stmts getSymDtorStmts();

    స్థిర శూన్య nameCheck(Ast_కణుపు* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        ఒకవేళ (nodep->name().ఖాళీలు()) తిరిగి;
        స్థిరరాశి AstCFunc* స్థిరరాశి cfuncp = VN_CAST(nodep, CFunc);
        ఒకవేళ (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            స్థిరరాశి std::string rsvd = V3_Language_పదాలు::isKeyword(nodep->name());
            ఒకవేళ (rsvd != "") {
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

    స్థిర std::string scopeSymString(స్థిరరాశి std::string& scpname) {
        std::string బహిర్గతం = scpname;
        std::string::కొలత_type pos;
        అయితే ((pos = బహిర్గతం.కనుగొను("__PVT__")) != std::string::npos) బహిర్గతం.replace(pos, 7, "");
        ఒకవేళ (VString::startsWith(బహిర్గతం, "TOP__DOT__")) బహిర్గతం.replace(0, 10, "");
        ఒకవేళ (VString::startsWith(బహిర్గతం, "TOP.")) బహిర్గతం.replace(0, 4, "");
        అయితే ((pos = బహిర్గతం.కనుగొను('.')) != std::string::npos) బహిర్గతం.replace(pos, 1, "__");
        అయితే ((pos = బహిర్గతం.కనుగొను("__DOT__")) != std::string::npos) బహిర్గతం.replace(pos, 7, "__");
        తిరిగి బహిర్గతం;
    }

    స్థిర string scopeDecodeIdentifier(స్థిరరాశి std::string& scpname) {
        std::string::కొలత_type pos = std::string::npos;

        // Remove hierarchy
        కొలత_t i = 0;
        // always makes progress
        అయితే (i < scpname.నిడివి()) {
            ఒకవేళ (scpname[i] == '\\') {
                అయితే (i < scpname.నిడివి() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } లేకపోతే {
                అయితే (i < scpname.నిడివి() && scpname[i] != '.') ++i;
                ఒకవేళ (i < scpname.నిడివి()) pos = i++;
            }
        }

        తిరిగి pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    నిర్మాణం VarDims final {
        పూర్ణాంకం pdim = 0;  // Packed array dims plus a ranged basic leaf
        పూర్ణాంకం udim = 0;
        std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> unpackedLR;  // (left,right), outer-first
        std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> flatten() స్థిరరాశి {
            std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> flat(unpackedLR);
            flat.చొప్పించు(flat.చివర(), packedLR.cbegin(), packedLR.cend());
            తిరిగి flat;
        }
    };
    స్థిర VarDims getVarDims(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి మూలం_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        కొరకు (స్థిరరాశి Ast_కణుపు_DType* dtypep = మూలం_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(dtypep, కణుపు_Array_DType)) {
                ఒకవేళ (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_వెనుక(adtypep->ఎడమ(), adtypep->కుడి());
                    ++d.pdim;
                } లేకపోతే {
                    d.unpackedLR.emplace_వెనుక(adtypep->ఎడమ(), adtypep->కుడి());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } లేకపోతే {
                ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp()) {
                    ఒకవేళ (basicp->isRanged()) {
                        d.packedLR.emplace_వెనుక(basicp->ఎడమ(), basicp->కుడి());
                        ++d.pdim;
                    }
                }
                ఆపు;  // Non-array leaf
            }
        }
        తిరిగి d;
    }

    స్థిర VarDims dimsFor(స్థిరరాశి Scope_Var_దత్తాంశం& svd) { తిరిగి getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    స్థిర std::string boundsString(స్థిరరాశి VarDims& d) {
        std::string bounds;
        కొరకు (స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::గమ్యం_string(lr.మొదటి);
            bounds += ",";
            bounds += std::గమ్యం_string(lr.రెండవ);
        }
        తిరిగి bounds;
    }

    స్థిర పూర్ణాంకం getUnpackedElements(స్థిరరాశి Ast_కణుపు_DType* మూలం_Dtypep) {
        పూర్ణాంకం elements = 1;
        కొరకు (స్థిరరాశి Ast_కణుపు_DType* dtypep = మూలం_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(dtypep, UnpackArrayDType);
            ఒకవేళ (!adtypep) ఆపు;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        తిరిగి elements;
    }

    స్థిర తార్కిక needs_Emitted_Ent_కొలత(స్థిరరాశి std::string& vlEnumType) {
        తిరిగి vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    స్థిర std::pair<తార్కిక, std::string> isForceControlSignal(స్థిరరాశి AstVar* స్థిరరాశి signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        కొరకు (స్థిరరాశి std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            స్థిరరాశి std::కొలత_t suffixPos = signalVarp->name().కనుగొను(forceControlSuffix);
            స్థిరరాశి తార్కిక suffix_దొరికింది = suffixPos != std::string::npos;
            స్థిరరాశి తార్కిక suffix_Is_At_చివర
                = suffixPos + forceControlSuffix.నిడివి() == signalVarp->name().నిడివి();
            ఒకవేళ (!(suffix_దొరికింది && suffix_Is_At_చివర)) కొనసాగు;
            స్థిరరాశి std::string పునాది_Signal_Name = signalVarp->name().substr(0, suffixPos);
            తిరిగి std::pair<తార్కిక, std::string>{నిజం, పునాది_Signal_Name};
        }
        తిరిగి std::pair<తార్కిక, std::string>{అబద్ధం, ""};
    }

    స్థిర శూన్య appendVarProperties(std::string& stmt, స్థిరరాశి std::string& vlEnumType,
                                    స్థిరరాశి std::string& vlEnumDir, స్థిరరాశి పూర్ణాంకం udim, స్థిరరాశి పూర్ణాంకం pdim,
                                    స్థిరరాశి std::string& bounds, స్థిరరాశి std::string& ent_కొలత = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::గమ్యం_string(udim);
        ఒకవేళ (!ent_కొలత.ఖాళీలు()) {
            stmt += ", ";
            stmt += ent_కొలత;
        } లేకపోతే {
            stmt += ", ";
            stmt += std::గమ్యం_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    స్థిర std::string memberVlEnumDir(స్థిరరాశి AstVar* స్థిరరాశి varp,
                                       స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        std::string బహిర్గతం = '(' + varp->vlEnumDir(/*forMember=*/నిజం);
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి skipDTypep = dtypep->skipRefp();
        ఒకవేళ (skipDTypep->isSigned()) బహిర్గతం += "|VLVF_SIGNED";
        ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = skipDTypep->basicp()) {
            ఒకవేళ (basicp->keyword() == VBasicDTypeKwd::బిట్టు) బహిర్గతం += "|VLVF_BITVAR";
        }
        బహిర్గతం += skipDTypep->vlEnumPackedAgg();
        తిరిగి బహిర్గతం + ')';
    }

    // Name of the instance member for the given scope
    స్థిర std::string instName(స్థిరరాశి AstScope* scopep) {
        తిరిగి "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    స్థిర std::string చొప్పించు_Var_Statement(స్థిరరాశి Scope_Var_దత్తాంశం& svd, స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                          స్థిరరాశి AstVar* స్థిరరాశి varp, స్థిరరాశి పూర్ణాంకం udim, స్థిరరాశి పూర్ణాంకం pdim,
                                          స్థిరరాశి std::string& bounds) {
        స్థిరరాశి std::string var_సమాసం = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        స్థిరరాశి std::string vlEnumType = varp->vlEnumType();
        స్థిరరాశి తార్కిక needs_Ent_కొలత = needs_Emitted_Ent_కొలత(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_కొలత ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_బహిర్గతం_Formatter::quoteNameControls(protect(svd.m_var_పునాది_Pretty)) + '"';

        ఒకవేళ (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_సమాసం;
            stmt += "), false, ";
        } లేకపోతే ఒకవేళ (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_సమాసం;
            stmt += ".c_str())), true, ";
        } లేకపోతే {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_సమాసం;
            stmt += "))), true, ";
        }

        స్థిరరాశి std::string ent_కొలత = needs_Ent_కొలత
                                        ? "sizeof(" + var_సమాసం + ") / "
                                              + std::గమ్యం_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_కొలత);
        తిరిగి stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    గణన తరగతి పట్టిక_నమోదు_రకం : uint8_t {
        పట్టిక_అడ్డువరుస,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    పట్టిక_నమోదు_రకం try_Build_పట్టిక_నమోదు(స్థిరరాశి Scope_Var_దత్తాంశం& svd, స్థిరరాశి AstVar* స్థిరరాశి varp,
                                      స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                      స్థిరరాశి std::string& modClassName, స్థిరరాశి VarDims& dims,
                                      std::string& అడ్డువరుస_బహిర్గతం) స్థిరరాశి {
        స్థిరరాశి పూర్ణాంకం pdim = dims.pdim;
        స్థిరరాశి పూర్ణాంకం udim = dims.udim;
        ఒకవేళ (varp->isForceable() && force_Control_సంకేతాలు_Are_Valid(scopep, varp)) {
            తిరిగి పట్టిక_నమోదు_రకం::FORCEABLE_RESIDUAL;
        }
        ఒకవేళ (udim + pdim > VPI_పట్టిక_MAX_DIMS) తిరిగి పట్టిక_నమోదు_రకం::PLAIN_RESIDUAL;

        స్థిరరాశి std::string vlEnumType = varp->vlEnumType();
        ఒకవేళ (needs_Emitted_Ent_కొలత(vlEnumType))
            తిరిగి పట్టిక_నమోదు_రకం::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        ఒకవేళ (varp->isParam()) తిరిగి పట్టిక_నమోదు_రకం::PLAIN_RESIDUAL;

        స్థిరరాశి std::string name = V3_బహిర్గతం_Formatter::quoteNameControls(protect(svd.m_var_పునాది_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        స్థిరరాశి std::string member = varName(varp);
        స్థిరరాశి std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::సదిశ<పూర్ణాంకం> dv;
        కొరకు (స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం>& lr : dims.flatten()) {
            dv.నెట్టు_వెనుక(lr.మొదటి);
            dv.నెట్టు_వెనుక(lr.రెండవ);
        }

        std::string అడ్డువరుస = "{\"" + name + "\", ";
        అడ్డువరుస += "offsetof(" + modClassName + ", " + member + "), ";
        అడ్డువరుస += vlEnumType + ", ";
        అడ్డువరుస += "(" + dir + "), ";
        అడ్డువరుస += std::గమ్యం_string(udim) + ", " + std::గమ్యం_string(pdim) + ", {";
        కొరకు (పూర్ణాంకం i = 0; i < VPI_పట్టిక_MAX_DIMS * 2; ++i) {
            ఒకవేళ (i) అడ్డువరుస += ", ";
            అడ్డువరుస += std::గమ్యం_string(i < static_cast<పూర్ణాంకం>(dv.కొలత()) ? dv[i] : 0);
        }
        అడ్డువరుస += "}}";
        అడ్డువరుస_బహిర్గతం = అడ్డువరుస;
        తిరిగి పట్టిక_నమోదు_రకం::పట్టిక_అడ్డువరుస;
    }

    స్థిర std::string చొప్పించు_DType_Var_Statement(స్థిరరాశి Scope_Var_దత్తాంశం& svd,
                                               స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                               స్థిరరాశి std::string& prettyName,
                                               స్థిరరాశి std::string& cName,
                                               స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep, స్థిరరాశి పూర్ణాంకం udim,
                                               స్థిరరాశి పూర్ణాంకం pdim, స్థిరరాశి std::string& bounds) {
        స్థిరరాశి std::string vlEnumType = dtypep->vlEnumType();
        స్థిరరాశి తార్కిక needs_Ent_కొలత = needs_Emitted_Ent_కొలత(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_కొలత ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_బహిర్గతం_Formatter::quoteNameControls(prettyName) + '"';
        స్థిరరాశి std::string var_సమాసం = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        ఒకవేళ (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_సమాసం;
            stmt += "))), true, ";
        } లేకపోతే {
            stmt += ", &(";
            stmt += var_సమాసం;
            stmt += "), false, ";
        }
        స్థిరరాశి std::string ent_కొలత
            = needs_Ent_కొలత
                  ? "sizeof(" + var_సమాసం + ") / " + std::గమ్యం_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_కొలత);
        తిరిగి stmt;
    }

    స్థిర శూన్య కూడు_UOr_Struct_Member_Vars(std::సదిశ<std::string>& stmts, స్థిరరాశి Scope_Var_దత్తాంశం& svd,
                                       స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                       స్థిరరాశి std::string& pretty_ఉపసర్గ, స్థిరరాశి std::string& c_ఉపసర్గ,
                                       స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep) {
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి అంశం_DTypep = itemp->dtypep();
            ఒకవేళ (అంశం_DTypep->vlEnumType().ఖాళీలు()) కొనసాగు;
            స్థిరరాశి std::string prettyName
                = pretty_ఉపసర్గ + "." + Ast_కణుపు::vpiName(itemp->shortName());
            స్థిరరాశి std::string cName = c_ఉపసర్గ + "." + itemp->nameProtect();
            స్థిరరాశి VarDims dims = getVarDims(అంశం_DTypep);
            stmts.emplace_వెనుక(చొప్పించు_DType_Var_Statement(svd, scopep, prettyName, cName, అంశం_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            కూడు_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, అంశం_DTypep);
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి subp
                = VN_CAST(అంశం_DTypep->skipRefp(), కణుపు_UOr_Struct_DType)) {
                ఒకవేళ (!subp->packed())
                    కూడు_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } లేకపోతే ఒకవేళ (VN_IS(అంశం_DTypep->skipRefp(), UnpackArrayDType)) {
                కూడు_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    అంశం_DTypep);
            }
        }
    }

    స్థిర శూన్య కూడు_Unpacked_Array_UOr_Struct_Member_Vars(std::సదిశ<std::string>& stmts,
                                                    స్థిరరాశి Scope_Var_దత్తాంశం& svd,
                                                    స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                                    స్థిరరాశి std::string& pretty_ఉపసర్గ,
                                                    స్థిరరాశి std::string& c_ఉపసర్గ,
                                                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి skipDTypep = dtypep->skipRefp();
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            కూడు_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_ఉపసర్గ, c_ఉపసర్గ + "[0]",
                                                adtypep->subDTypep());
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep
                   = VN_CAST(skipDTypep, కణుపు_UOr_Struct_DType)) {
            ఒకవేళ (!sdtypep->packed())
                కూడు_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_ఉపసర్గ, c_ఉపసర్గ, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    స్థిర శూన్య కూడు_Packed_Member_Vars(std::సదిశ<std::string>& stmts, స్థిరరాశి Scope_Var_దత్తాంశం& svd,
                                    స్థిరరాశి AstScope* స్థిరరాశి scopep, స్థిరరాశి std::string& pretty_ఉపసర్గ,
                                    స్థిరరాశి std::string& cName, స్థిరరాశి std::string& vlEnumType,
                                    స్థిరరాశి std::string& ent_కొలత,
                                    స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep,
                                    స్థిరరాశి పూర్ణాంకం మాతృ_బిట్టు_స్థానభ్రంశం, స్థిరరాశి తార్కిక needs_సూచీ) {
        స్థిరరాశి std::string var_సమాసం = "vlSymsp->" + instName(scopep) + "." + cName;
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి అంశం_DTypep = itemp->dtypep();
            స్థిరరాశి std::string prettyName
                = pretty_ఉపసర్గ + "." + Ast_కణుపు::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            స్థిరరాశి పూర్ణాంకం బిట్టు_స్థానభ్రంశం = మాతృ_బిట్టు_స్థానభ్రంశం + itemp->lsb();
            స్థిరరాశి VarDims dims = getVarDims(అంశం_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_బహిర్గతం_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            ఒకవేళ (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_సమాసం + "))), true, ";
            } లేకపోతే {
                stmt += ", &(" + var_సమాసం + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, అంశం_DTypep);
            ఒకవేళ (needs_సూచీ) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::గమ్యం_string(బిట్టు_స్థానభ్రంశం) + ", ";
            stmt += ent_కొలత + ", ";
            stmt += std::గమ్యం_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_వెనుక(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            స్థిరరాశి Ast_కణుపు_DType* subDTypep = అంశం_DTypep->skipRefp();
            తార్కిక isArray = అబద్ధం;
            అయితే (స్థిరరాశి AstPackArrayDType* స్థిరరాశి adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = నిజం;
            }
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి subp = VN_CAST(subDTypep, కణుపు_UOr_Struct_DType)) {
                కూడు_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_కొలత,
                                    subp, బిట్టు_స్థానభ్రంశం, needs_సూచీ || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    స్థిర శూన్య కూడు_Packed_Agg_Member_Vars(std::సదిశ<std::string>& stmts, స్థిరరాశి Scope_Var_దత్తాంశం& svd,
                                       స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                       స్థిరరాశి std::string& pretty_ఉపసర్గ, స్థిరరాశి std::string& cName,
                                       స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        std::string elemCName = cName;
        స్థిరరాశి Ast_కణుపు_DType* elemDTypep = dtypep->skipRefp();
        తార్కిక isArray = అబద్ధం;
        అయితే (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(elemDTypep, కణుపు_Array_DType)) {
            ఒకవేళ (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = నిజం;
        }
        స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep = VN_CAST(elemDTypep, కణుపు_UOr_Struct_DType);
        ఒకవేళ (!sdtypep || !sdtypep->packed()) తిరిగి;
        స్థిరరాశి std::string var_సమాసం = "vlSymsp->" + instName(scopep) + "." + cName;
        స్థిరరాశి std::string ent_కొలత
            = "sizeof(" + var_సమాసం + ") / " + std::గమ్యం_string(getUnpackedElements(dtypep));
        కూడు_Packed_Member_Vars(stmts, svd, scopep, pretty_ఉపసర్గ, elemCName, dtypep->vlEnumType(),
                            ent_కొలత, sdtypep, 0, isArray);
    }

    std::string చొప్పించు_Forceable_Var_Statement(స్థిరరాశి Scope_Var_దత్తాంశం& svd, స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                            స్థిరరాశి AstVar* స్థిరరాశి varp, స్థిరరాశి పూర్ణాంకం udim,
                                            స్థిరరాశి పూర్ణాంకం pdim, స్థిరరాశి std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_బహిర్గతం_Formatter::quoteNameControls(protect(svd.m_var_పునాది_Pretty)) + '"';

        స్థిరరాశి std::string var_సమాసం = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        ప్రతిజ్ఞా(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_సమాసం;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_సమాసం + "__VforceRd";
        stmt += "), \"" + V3_బహిర్గతం_Formatter::quoteNameControls(protect(svd.m_var_పునాది_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        కొరకు (స్థిరరాశి std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            స్థిరరాశి std::string enable_Signal_కీలకం
                = get_కీలకం_Name(scopep, varp->name() + forceControlSuffix);
            స్థిరరాశి std::map<స్థిరరాశి std::string, Scope_Var_దత్తాంశం>::const_iterator itpair
                = m_scopeVars.కనుగొను(enable_Signal_కీలకం);

            ఒకవేళ (itpair == m_scopeVars.చివర()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_కీలకం << "'.");
            }

            స్థిరరాశి Scope_Var_దత్తాంశం& svd = itpair->రెండవ;
            స్థిరరాశి AstScope* స్థిరరాశి scopep = svd.m_scopep;
            స్థిరరాశి AstVar* స్థిరరాశి varp = svd.m_varp;
            స్థిరరాశి VarDims dims = dimsFor(svd);
            stmt
                += చొప్పించు_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            ఒకవేళ (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::గమ్యం_string(udim);
        stmt += ", ";
        stmt += std::గమ్యం_string(pdim);
        stmt += bounds;
        stmt += ")";
        తిరిగి stmt;
    }

    స్థిర std::string get_కీలకం_Name(స్థిరరాశి AstScope* స్థిరరాశి scopep, స్థిరరాశి std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //క్రమం
        // signal can be found.
        std::string సంపూర్ణ = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        ఒకవేళ (VString::startsWith(సంపూర్ణ, "__DOT__TOP")) సంపూర్ణ.replace(0, 10, "");
        స్థిరరాశి std::string::కొలత_type dpos = సంపూర్ణ.rfind("__DOT__");
        ఒకవేళ (dpos != std::string::npos) { scpName = సంపూర్ణ.substr(0, dpos); }
        స్థిరరాశి std::string scpSym = scopeSymString(VName::dehash(scpName));
        తిరిగి scpSym + " " + signal_name;
    }

    తార్కిక పునాది_Signal_Is_Valid(స్థిరరాశి AstScope* స్థిరరాశి scopep, స్థిరరాశి AstVar* స్థిరరాశి controlSignalVarp,
                           స్థిరరాశి std::string& పునాది_Signal_Name) స్థిరరాశి {
        స్థిరరాశి std::string పునాది_Signal_కీలకం = get_కీలకం_Name(scopep, పునాది_Signal_Name);
        స్థిరరాశి std::map<స్థిరరాశి std::string, Scope_Var_దత్తాంశం>::const_iterator పునాది_Signal_It
            = m_scopeVars.కనుగొను(పునాది_Signal_కీలకం);
        ఒకవేళ (పునాది_Signal_It == m_scopeVars.చివర()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            తిరిగి అబద్ధం;
        } లేకపోతే {
            స్థిరరాశి AstVar* స్థిరరాశి పునాది_Signal_Varp = పునాది_Signal_It->రెండవ.m_varp;
            ఒకవేళ (m_scopeVars.లెక్క(పునాది_Signal_Name) > 1) {
                పునాది_Signal_Varp->v3fatalSrc(
                    "Found Signal " << పునాది_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << పునాది_Signal_Name << " occurring in the same scope.");
                తిరిగి అబద్ధం;
            }
            ఒకవేళ (!పునాది_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << పునాది_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                తిరిగి అబద్ధం;
            }
        }
        తిరిగి నిజం;
    }

    తార్కిక పునాది_Signal_Is_ప్రజా(స్థిరరాశి AstScope* స్థిరరాశి scopep,
                            స్థిరరాశి std::string& పునాది_Signal_Name) స్థిరరాశి {
        స్థిరరాశి std::string పునాది_Signal_కీలకం = get_కీలకం_Name(scopep, పునాది_Signal_Name);
        స్థిరరాశి std::map<స్థిరరాశి std::string, Scope_Var_దత్తాంశం>::const_iterator పునాది_Signal_It
            = m_scopeVars.కనుగొను(పునాది_Signal_కీలకం);
        ఒకవేళ (పునాది_Signal_It == m_scopeVars.చివర()) {
            తిరిగి అబద్ధం;
        } లేకపోతే {
            స్థిరరాశి AstVar* స్థిరరాశి పునాది_Signal_Varp = పునాది_Signal_It->రెండవ.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            ఒకవేళ (!పునాది_Signal_Varp->is_Sig_ప్రజా()) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    తార్కిక force_Control_సంకేతాలు_Are_Valid(స్థిరరాశి AstScope* స్థిరరాశి scopep,
                                     స్థిరరాశి AstVar* స్థిరరాశి పునాది_Signal_Varp) స్థిరరాశి {
        constexpr std::array<స్థిరరాశి అక్షరం*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        తిరిగి std::all_యొక్క(
            forceControlSuffixes.begin(), forceControlSuffixes.చివర(),
            [scopep, పునాది_Signal_Varp, ఇది](స్థిరరాశి std::string& forceControlSuffix) {
                స్థిరరాశి std::string controlSignalName = పునాది_Signal_Varp->name() + forceControlSuffix;
                స్థిరరాశి std::string control_Signal_కీలకం = get_కీలకం_Name(scopep, controlSignalName);
                స్థిరరాశి std::కొలత_t control_Signal_లెక్క = m_scopeVars.లెక్క(control_Signal_కీలకం);
                ఒకవేళ (control_Signal_లెక్క == 0) {
                    పునాది_Signal_Varp->v3fatalSrc("Signal "
                                               << పునాది_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_కీలకం << "'.");
                    తిరిగి అబద్ధం;
                }
                ఒకవేళ (control_Signal_లెక్క > 1) {
                    పునాది_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << పునాది_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    తిరిగి అబద్ధం;
                }
                తిరిగి నిజం;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    శూన్య varHierarchyScopes(std::string scp) {

        std::string::కొలత_type prd_pos = scp.rfind('.');
        std::string::కొలత_type dot_pos = scp.rfind("__DOT__");

        అయితే (!scp.ఖాళీలు()) {
            స్థిరరాశి స్వయంచాలక scpit = m_vpiScopeCandidates.కనుగొను(scopeSymString(scp));
            ఒకవేళ ((scpit != m_vpiScopeCandidates.చివర())
                && (m_scope_పేర్లు.కనుగొను(scp) == m_scope_పేర్లు.చివర())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                స్థిరరాశి స్వయంచాలక pair = m_scope_పేర్లు.emplace(scpit->రెండవ.m_symName, scpit->రెండవ);
                ఒకవేళ (!pair.రెండవ) pair.మొదటి->రెండవ.m_type = scpit->రెండవ.m_type;
            }

            // resize and advance pointers
            ఒకవేళ ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } లేకపోతే {
                ఒకవేళ (prd_pos == string::npos) ఆపు;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    శూన్య కూడు_Scope_Var_నమోదు(స్థిరరాశి AstScope* స్థిరరాశి scopep, స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp,
                          స్థిరరాశి AstVar* స్థిరరాశి varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string సంపూర్ణ = scopep->name() + "__DOT__" + varp->name();
        ఒకవేళ (VString::startsWith(సంపూర్ణ, "__DOT__TOP")) సంపూర్ణ.replace(0, 10, "");
        స్థిరరాశి std::string::కొలత_type dpos = సంపూర్ణ.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        స్థిరరాశి std::string scpName = సంపూర్ణ.substr(0, dpos);
        స్థిరరాశి std::string var_పునాది = సంపూర్ణ.substr(dpos + std::అక్షరశ్రేణి_పొడవు("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        స్థిరరాశి std::string var_పునాది_Pretty = Ast_కణుపు::vpiName(VName::dehash(var_పునాది));
        స్థిరరాశి std::string scpPretty = Ast_కణుపు::prettyName(VName::dehash(scpName));
        స్థిరరాశి std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        ఒకవేళ (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_పేర్లు.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_పునాది_Pretty, varp, modp, scopep));
    }

    శూన్య varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        కొరకు (స్థిరరాశి ScopeModPair& smPair : m_scopes) {
            స్థిరరాశి AstScope* స్థిరరాశి scopep = smPair.మొదటి;
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి smodp = smPair.రెండవ;
            కొరకు (స్థిరరాశి ModVarPair& mvPair : m_modVars) {
                స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = mvPair.మొదటి;
                స్థిరరాశి AstVar* స్థిరరాశి varp = mvPair.రెండవ;
                ఒకవేళ (modp != smodp) కొనసాగు;
                కూడు_Scope_Var_నమోదు(scopep, modp, varp);
            }
        }
    }

    శూన్య collectIfaceRefs(స్థిరరాశి AstScope* nodep) {
        స్థిరరాశి AstCell* స్థిరరాశి cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        ఒకవేళ (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) తిరిగి;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        స్థిరరాశి std::string మార్గం = Ast_కణుపు::vpiName(nodep->name());
        స్థిరరాశి std::string instName = Ast_కణుపు::vpiName(cellp->origName());
        UASSERT_OBJ(మార్గం.నిడివి() > instName.నిడివి() && VString::చివర్లు_With(మార్గం, instName), nodep,
                    "Interface scope name " << మార్గం << " does not end with instance name "
                                            << instName);
        స్థిరరాశి std::string మాతృ_మార్గం = మార్గం.substr(0, మార్గం.నిడివి() - instName.నిడివి());

        కొరకు (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            స్థిరరాశి std::string refName = Ast_కణుపు::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            ఒకవేళ (!VString::startsWith(refName, మాతృ_మార్గం)) కొనసాగు;
            m_ifaceRefs.emplace_వెనుక(nodep, scopeSymString(intfRefp->మాతృ_Name()), refName,
                                     Ast_కణుపు::vpiName(intfRefp->పునాది_Name()),
                                     intfRefp->modportName());
        }
    }

    శూన్య build_Iface_Ref_పట్టిక() {
        ఒకవేళ (m_ifaceRefs.ఖాళీలు()) తిరిగి;
        స్థిరరాశి std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_క్రమీకరించు(
            m_ifaceRefs.begin(), m_ifaceRefs.చివర(),
            [](స్థిరరాశి Iface_Ref_దత్తాంశం& a, స్థిరరాశి Iface_Ref_దత్తాంశం& b) { తిరిగి a.m_suffix < b.m_suffix; });
        కొరకు (స్థిరరాశి Iface_Ref_దత్తాంశం& ird : m_ifaceRefs) {
            స్థిరరాశి std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            ఒకవేళ (m_scope_పేర్లు.కనుగొను(scopeSym) == m_scope_పేర్లు.చివర()) కొనసాగు;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string మాతృ_స్థానభ్రంశం = "VL_IFACEREF_NO_PARENT";
            స్థిరరాశి స్వయంచాలక మాతృ = m_scope_పేర్లు.కనుగొను(ird.m_మాతృ_Sym);
            ఒకవేళ (మాతృ != m_scope_పేర్లు.చివర()) {
                మాతృ_స్థానభ్రంశం = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + మాతృ->రెండవ.m_symName) + ")";
            }
            std::string అడ్డువరుస
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            అడ్డువరుస += మాతృ_స్థానభ్రంశం + ", \"";
            అడ్డువరుస += V3_బహిర్గతం_Formatter::quoteNameControls(VIdProtect::protect_పదాలు_If(ird.m_name, నిజం));
            అడ్డువరుస += "\", \"";
            అడ్డువరుస += V3_బహిర్గతం_Formatter::quoteNameControls(
                VIdProtect::protect_పదాలు_If(ird.m_suffix, నిజం));
            అడ్డువరుస += "\", \"";
            అడ్డువరుస += V3_బహిర్గతం_Formatter::quoteNameControls(
                VIdProtect::protect_పదాలు_If(ird.m_modportName, నిజం));
            అడ్డువరుస += "\"}";
            m_iface_Ref_పట్టిక_అడ్డువరుసలు.emplace_వెనుక(std::move(అడ్డువరుస));
        }
        ఒకవేళ (!m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) m_iface_Ref_పట్టిక_Name = symClass + "__VpiIfaceRefTable";
    }

    శూన్య buildVpiHierarchy() {
        కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scope_పేర్లు) {
            స్థిరరాశి std::string symName = itpair.రెండవ.m_symName;
            std::string above = symName;
            ఒకవేళ (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            అయితే (!above.ఖాళీలు()) {
                స్థిరరాశి std::string::కొలత_type pos = above.rfind("__");
                ఒకవేళ (pos == std::string::npos) ఆపు;
                above.resize(pos);
                ఒకవేళ (m_vpiScopeHierarchy.కనుగొను(above) != m_vpiScopeHierarchy.చివర()) {
                    m_vpiScopeHierarchy[above].నెట్టు_వెనుక(symName);
                    ఆపు;
                }
            }
            m_vpiScopeHierarchy[symName] = std::సదిశ<string>();
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        ఒకవేళ (v3Global.opt.vpi()) buildVpiHierarchy();

        ఒకవేళ (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            కొరకు (స్థిరరాశి స్వయంచాలక& scp : m_dpi_Scope_పేర్లు) m_scope_పేర్లు.emplace(scp.మొదటి, scp.రెండవ);
        }

        // Sort by names, so line/process order matters less
        std::stable_క్రమీకరించు(m_scopes.begin(), m_scopes.చివర(),
                         [](స్థిరరాశి ScopeModPair& a, స్థిరరాశి ScopeModPair& b) {
                             తిరిగి a.మొదటి->name() < b.మొదటి->name();
                         });
        std::stable_క్రమీకరించు(m_dpis.begin(), m_dpis.చివర(),  //
                         [](స్థిరరాశి AstCFunc* ap, స్థిరరాశి AstCFunc* bp) {
                             ఒకవేళ (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 తిరిగి bp->dpiImportPrototype();
                             }
                             తిరిగి ap->name() < bp->name();
                         });

        // Output
        ఒకవేళ (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        ఒకవేళ (v3Global.dpi()) {
            emitDpiHdr();
            ఒకవేళ (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->isConstPool()) తిరిగి;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCellInlineScope* nodep) override {
        స్థిరరాశి std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        స్థిరరాశి std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        స్థిరరాశి పూర్ణాంకం timeunit = m_modp->timeunit().power_యొక్క_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_కణుపు::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        ఒకవేళ (VN_IS(m_modp, Class)) తిరిగి;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_వెనుక(nodep, m_modp);

        ఒకవేళ (v3Global.opt.vpi() && !nodep->is_శిఖరం()) {
            స్థిరరాశి std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            స్థిరరాశి పూర్ణాంకం timeunit = m_modp->timeunit().power_యొక్క_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_కణుపు::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {
        ఒకవేళ (nodep->selfRelative()) తిరిగి;  // Its absolute copy is under the scope
        స్థిరరాశి std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        స్థిరరాశి పూర్ణాంకం timeunit = m_modp ? m_modp->timeunit().power_యొక్క_Ten() : 0;
        m_dpi_Scope_పేర్లు.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        ఒకవేళ (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } లేకపోతే {
            // Note emplace does not construct when duplicate key
            m_dpi_Scope_పేర్లు.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        ఒకవేళ ((nodep->is_Sig_User_Rd_ప్రజా() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().ఖాళీలు()) {
            m_modVars.emplace_వెనుక(m_modp, nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        ఒకవేళ (!nodep->దత్తాంశం_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_అంకె(m_coverBins);
            m_coverBins += nodep->కొలత();
            nodep->local_Bin_అంకె(m_modCoverBins);
            m_modCoverBins += nodep->కొలత();
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        nameCheck(nodep);
        ఒకవేళ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.నెట్టు_వెనుక(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    శూన్య సందర్శించు(AstConst*) override {}
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్పష్టమైన EmitCSyms(AstNetlist* nodep, తార్కిక dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

శూన్య EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_శీర్షిక_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_శిఖరం_Include();
    puts("#include \"verilated.h\"\n");
    ఒకవేళ (v3Global.needTraceDumper()) {
        కొరకు (స్థిరరాశి string& పునాది : v3Global.opt.trace_ఆధారం_Langs())
            puts("#include \"" + పునాది + ".h\"\n");
    }
    ఒకవేళ (v3Global.opt.ఉపయోగిస్తుంది_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + శిఖరం_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    కొరకు (Ast_కణుపు_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), కణుపు_Module);
        ఒకవేళ (VN_IS(nodep, Class)) కొనసాగు;  // Class included earlier
        ఒకవేళ (nodep->isConstPool()) కొనసాగు;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::ఉపసర్గ_Name_Protect(nodep) + ".h\"\n");
    }

    ఒకవేళ (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::సమితి<std::string> types;  // Remove duplicates and sort
        కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scopeFuncs) {
            స్థిరరాశి AstCFunc* స్థిరరాశి funcp = itpair.రెండవ.m_cfuncp;
            ఒకవేళ (!funcp->dpiExportImpl()) కొనసాగు;
            స్థిరరాశి std::string cbtype
                = protect(v3Global.opt.ఉపసర్గ() + "__Vcb_" + funcp->cname() + "_t");
            స్థిరరాశి std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_వాదనలు(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        కొరకు (స్థిరరాశి std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_వ్యక్తిగత(అబద్ధం);  // public:

    puts("// INTERNAL STATE\n");
    puts(శిఖరం_Class_Name() + "* const __Vm_modelp;\n");

    ఒకవేళ (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    ఒకవేళ (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    ఒకవేళ (v3Global.hasEvents()) {
        ఒకవేళ (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } లేకపోతే {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    ఒకవేళ (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    ఒకవేళ (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    ఒకవేళ (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    ఒకవేళ (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    ఒకవేళ (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::గమ్యం_string(ExecMTask::అంకె_వాడిన_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    కొరకు (స్థిరరాశి ScopeModPair& itpair : m_scopes) {
        స్థిరరాశి AstScope* స్థిరరాశి scopep = itpair.మొదటి;
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = itpair.రెండవ;
        ఒకవేళ (VN_IS(modp, Class)) కొనసాగు;
        ofp()->రాయి("%-30s ", EmitCUtil::ఉపసర్గ_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    ఒకవేళ (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.దారాలు() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::గమ్యం_string(m_coverBins));
        puts("];\n");
    }

    ఒకవేళ (!m_scope_పేర్లు.ఖాళీలు()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scope_పేర్లు) {
            స్థిరరాశి Scope_దత్తాంశం& sd = itpair.రెండవ;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    ఒకవేళ (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + శిఖరం_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    ఒకవేళ (v3Global.hasEvents()) {
        ఒకవేళ (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } లేకపోతే {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        ఒకవేళ (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } లేకపోతే {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        ఒకవేళ (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } లేకపోతే {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    ఒకవేళ (v3Global.needTraceDumper()) {
        ఒకవేళ (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    ఒకవేళ (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->puts_చివర_Guard();
    closeOutputFile();
}

శూన్య EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    తార్కిక needs_New_పంక్తి = అబద్ధం;
    కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scopeFuncs) {
        స్థిరరాశి AstCFunc* స్థిరరాశి funcp = itpair.రెండవ.m_cfuncp;
        ఒకవేళ (!funcp->dpiExportImpl()) కొనసాగు;
        emitCFuncDecl(funcp, itpair.రెండవ.m_modp);
        needs_New_పంక్తి = నిజం;
    }
    ఒకవేళ (needs_New_పంక్తి) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::సమితి<std::string> lib_పేర్లు;
    కొరకు (స్థిరరాశి ScopeModPair& itpair : m_scopes) {
        ఒకవేళ (itpair.రెండవ->verilatorLib()) lib_పేర్లు.emplace(itpair.రెండవ->origName());
    }
    కొరకు (స్థిరరాశి std::string& libName : lib_పేర్లు) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    ఒకవేళ (!lib_పేర్లు.ఖాళీలు()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    ఒకవేళ (!m_varTables.ఖాళీలు() || !m_scope_పట్టిక_అడ్డువరుసలు.ఖాళీలు() || !m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
        కొరకు (స్థిరరాశి స్వయంచాలక& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.మొదటి + "[];\n");
        }
        ఒకవేళ (!m_scope_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
            puts("extern const VlScopeTableEntry " + m_scope_పట్టిక_Name + "[];\n");
        }
        ఒకవేళ (!m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_పట్టిక_Name + "[];\n");
        }
        puts("\n");
    }
}

శూన్య EmitCSyms::emitVarTables() {
    ఒకవేళ (m_varTables.ఖాళీలు() && m_scope_పట్టిక_అడ్డువరుసలు.ఖాళీలు() && m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) తిరిగి;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    కొరకు (స్థిరరాశి స్వయంచాలక& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.మొదటి + "[] = {\n");
        కొరకు (స్థిరరాశి std::string& అడ్డువరుస : kv.రెండవ) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(అడ్డువరుస);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ఒకవేళ (!m_scope_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
        puts("extern const VlScopeTableEntry " + m_scope_పట్టిక_Name + "[] = {\n");
        కొరకు (స్థిరరాశి std::string& అడ్డువరుస : m_scope_పట్టిక_అడ్డువరుసలు) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(అడ్డువరుస);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ఒకవేళ (!m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_పట్టిక_Name + "[] = {\n");
        కొరకు (స్థిరరాశి std::string& అడ్డువరుస : m_iface_Ref_పట్టిక_అడ్డువరుసలు) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(అడ్డువరుస);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic pop\n");
    puts("#endif\n");
}

శూన్య EmitCSyms::emitScopeHier(పాఠ్యం_Stmts& stmts, తార్కిక destroy) {
    ఒకవేళ (!v3Global.opt.vpi()) తిరిగి;

    ఒకవేళ (destroy) {
        stmts.కూడు("// Tear down scope hierarchy");
    } లేకపోతే {
        stmts.కూడు("// Set up scope hierarchy");
    }

    స్థిరరాశి std::string పద్ధతి = destroy ? "remove" : "add";
    కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scope_పేర్లు) {
        ఒకవేళ (itpair.మొదటి == "TOP") కొనసాగు;
        స్థిరరాశి Scope_దత్తాంశం& sd = itpair.రెండవ;
        స్థిరరాశి std::string& name = sd.m_prettyName;
        స్థిరరాశి std::string& scopeType = sd.m_type;
        ఒకవేళ (name.కనుగొను('.') != string::npos) కొనసాగు;
        ఒకవేళ (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") కొనసాగు;
        స్థిరరాశి std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.కూడు("vlSymsp->__Vhier." + పద్ధతి + "(0, " + id + ");");
    }
    కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_vpiScopeHierarchy) {
        స్థిరరాశి std::string fromName = scopeSymString(itpair.మొదటి);
        స్థిరరాశి std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_పేర్లు.at(fromName).m_symName);
        కొరకు (స్థిరరాశి std::string& name : itpair.రెండవ) {
            స్థిరరాశి std::string గమ్యం_Name = scopeSymString(name);
            స్థిరరాశి std::string గమ్యం_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_పేర్లు.at(గమ్యం_Name).m_symName);
            stmts.కూడు("vlSymsp->__Vhier." + పద్ధతి + "(" + fromId + ", " + గమ్యం_Id + ");");
        }
    }

    ఒకవేళ (destroy) {
        stmts.కూడు("// Clear keys from hierarchy map after values have been removed");
        stmts.కూడు("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::పాఠ్యం_Stmts EmitCSyms::getSymCtorStmts() {
    పాఠ్యం_Stmts stmts;

    {
        uint64_t దొంతర_కొలత = V3_దొంతర_లెక్క::లెక్క(v3Global.rootp());
        ఒకవేళ (v3Global.opt.debug_దొంతర_Check()) దొంతర_కొలత += 1024 * 1024 * 1024;
        V3Stats::కూడు_Stat("Size prediction, Stack (bytes)", దొంతర_కొలత);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::కూడు_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_బైట్లు);
        V3Stats::కూడు_Stat(V3Stats::STAT_MODEL_కొలత, దొంతర_కొలత + m_stat_Var_Scope_బైట్లు);

        stmts.కూడు("// Check resources");
        stmts.కూడు("Verilated::stackCheck(" + std::గమ్యం_string(దొంతర_కొలత) + ");");
    }

    stmts.కూడు("// Setup sub module instances");
    కొరకు (స్థిరరాశి ScopeModPair& itpair : m_scopes) {
        స్థిరరాశి AstScope* స్థిరరాశి scopep = itpair.మొదటి;
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = itpair.రెండవ;
        ఒకవేళ (modp->is_శిఖరం()) కొనసాగు;
        స్థిరరాశి std::string name = V3_బహిర్గతం_Formatter::quoteNameControls(
            VIdProtect::protect_పదాలు_If(scopep->prettyName(), scopep->protect()));
        stmts.కూడు("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    ఒకవేళ (v3Global.opt.profPgo()) {
        stmts.కూడు("// Configure profiling for PGO\n");
        ఒకవేళ (!v3Global.opt.hier_బిడ్డ()) {
            stmts.కూడు("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        ఒకవేళ (v3Global.opt.mtasks()) {
            v3Global.rootp()->శిఖరం_Modulep()->foreach([&](స్థిరరాశి Ast_Exec_లేఖాచిత్రం* execGraphp) {
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : execGraphp->depGraphp()->vertices()) {
                    స్థిరరాశి ExecMTask& mt = static_cast<స్థిరరాశి ExecMTask&>(vtx);
                    stmts.కూడు("vlSymsp->_vm_pgoProfiler.addCounter(" + std::గమ్యం_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.కూడు("// Configure time unit / time precision");
    ఒకవేళ (!v3Global.rootp()->timeunit().isNone()) {
        స్థిరరాశి std::string unit = std::గమ్యం_string(v3Global.rootp()->timeunit().power_యొక్క_Ten());
        stmts.కూడు("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    ఒకవేళ (!v3Global.rootp()->timeprecision().isNone()) {
        స్థిరరాశి std::string prec = std::గమ్యం_string(v3Global.rootp()->timeprecision().power_యొక్క_Ten());
        stmts.కూడు("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.కూడు("// Setup each module's pointers to their submodules");
    కొరకు (స్థిరరాశి స్వయంచాలక& i : m_scopes) {
        స్థిరరాశి AstScope* స్థిరరాశి scopep = i.మొదటి;
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = i.రెండవ;
        స్థిరరాశి AstScope* స్థిరరాశి abovep = scopep->aboveScopep();
        ఒకవేళ (!abovep) కొనసాగు;

        స్థిరరాశి std::string protName = VIdProtect::protect_పదాలు_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        ఒకవేళ (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } లేకపోతే {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.కూడు(stmt);
    }

    ఒకవేళ (v3Global.opt.coverage()) {
        stmts.కూడు("// Setup each module's pointer back to symbol table (for public functions)");
        కొరకు (స్థిరరాశి ScopeModPair& i : m_scopes) {
            స్థిరరాశి AstScope* స్థిరరాశి scopep = i.మొదటి;
            Ast_కణుపు_Module* స్థిరరాశి modp = i.రెండవ;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            స్థిరరాశి తార్కిక మొదటి = !modp->user1();
            modp->user1(నిజం);
            stmts.కూడు("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (మొదటి ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    కొరకు (స్థిరరాశి ScopeModPair& i : m_scopes) {
        స్థిరరాశి AstScope* స్థిరరాశి scopep = i.మొదటి;
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = i.రెండవ;
        ఒకవేళ (!modp->verilatorLib()) కొనసాగు;
        స్థిరరాశి AstVar* hashp = శూన్యసూచిక;
        స్థిరరాశి AstVar* handlep = శూన్యసూచిక;
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
            ఒకవేళ (!varp) కొనసాగు;
            ఒకవేళ (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            ఒకవేళ (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        స్థిరరాశి uint32_t hash = VN_AS(hashp->valuep(), Const)->గమ్యం_UInt();
        stmts.కూడు(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::గమ్యం_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string మార్గం = scopep->prettyName();
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            మార్గం.erase(0, v3Global.rootp()->trace_Lib_శిఖరం_Name().కొలత() + 1);
        }
        stmts.కూడు("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_బహిర్గతం_Formatter::quoteNameControls(మార్గం) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    ఒకవేళ (!m_scope_పేర్లు.ఖాళీలు()) {
        stmts.కూడు("// Setup scopes");
        స్థిరరాశి std::string symClass = symClassName();
        కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scope_పేర్లు) {
            స్థిరరాశి Scope_దత్తాంశం& sd = itpair.రెండవ;
            std::string అడ్డువరుస
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            అడ్డువరుస += V3_బహిర్గతం_Formatter::quoteNameControls(
                VIdProtect::protect_పదాలు_If(sd.m_prettyName, నిజం));
            అడ్డువరుస += "\", \"";
            అడ్డువరుస += V3_బహిర్గతం_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            అడ్డువరుస += "\", \"";
            అడ్డువరుస += V3_బహిర్గతం_Formatter::quoteNameControls(sd.m_defName);
            అడ్డువరుస += "\", ";
            అడ్డువరుస += std::గమ్యం_string(sd.m_timeunit);
            అడ్డువరుస += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_పట్టిక_అడ్డువరుసలు.emplace_వెనుక(std::move(అడ్డువరుస));
        }
        m_scope_పట్టిక_Name = symClass + "__VpiScopeTable";
        stmts.కూడు("VerilatedScope::scopesConstructFromTable(" + m_scope_పట్టిక_Name + ", "
                  + std::గమ్యం_string(m_scope_పేర్లు.కొలత()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_పట్టిక();
    ఒకవేళ (!m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
        stmts.కూడు("// Setup interface references");
        stmts.కూడు("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_పట్టిక_Name + ", "
                  + std::గమ్యం_string(m_iface_Ref_పట్టిక_అడ్డువరుసలు.కొలత()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, అబద్ధం);

    ఒకవేళ (v3Global.dpi()) {
        కొరకు (స్థిరరాశి std::string vfinal : {"0", "1"}) {
            stmts.కూడు("// Setup export functions - final: " + vfinal);
            కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scopeFuncs) {
                స్థిరరాశి Scope_Func_దత్తాంశం& sfd = itpair.రెండవ;
                స్థిరరాశి AstScopeName* స్థిరరాశి scopep = sfd.m_scopep;
                స్థిరరాశి AstCFunc* స్థిరరాశి funcp = sfd.m_cfuncp;
                స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = sfd.m_modp;
                ఒకవేళ (!funcp->dpiExportImpl()) కొనసాగు;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_బహిర్గతం_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::ఉపసర్గ_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.కూడు(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    ఒకవేళ (!m_scopeVars.ఖాళీలు()) {
        stmts.కూడు("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> పట్టిక_By_అడ్డువరుసలు;
        పూర్ణాంకం పట్టిక_Counter = 0;

        స్వయంచాలక it = m_scopeVars.cbegin();
        అయితే (it != m_scopeVars.cend()) {
            స్థిరరాశి std::string scopeName = it->రెండవ.m_scopeName;
            స్థిరరాశి AstScope* స్థిరరాశి instScopep = it->రెండవ.m_scopep;
            స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = it->రెండవ.m_modp;
            స్థిరరాశి std::string modClassName = EmitCUtil::ఉపసర్గ_Name_Protect(modp);

            std::సదిశ<std::string> అడ్డువరుసలు;
            std::సదిశ<std::string> residual;

            కొరకు (; it != m_scopeVars.cend() && it->రెండవ.m_scopeName == scopeName; ++it) {
                స్థిరరాశి Scope_Var_దత్తాంశం& svd = it->రెండవ;
                స్థిరరాశి AstScope* స్థిరరాశి scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                స్థిరరాశి AstVar* స్థిరరాశి varp = svd.m_varp;
                స్థిరరాశి VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                స్థిరరాశి std::pair<తార్కిక, std::string> fc = isForceControlSignal(varp);
                ఒకవేళ (fc.మొదటి && పునాది_Signal_Is_ప్రజా(scopep, fc.రెండవ)
                    && పునాది_Signal_Is_Valid(scopep, varp, fc.రెండవ)) {
                    కొనసాగు;
                }

                std::string అడ్డువరుస;
                స్థిరరాశి పట్టిక_నమోదు_రకం రకం
                    = try_Build_పట్టిక_నమోదు(svd, varp, scopep, modClassName, dims, అడ్డువరుస);
                ఎంపిక (రకం) {
                స్థితి పట్టిక_నమోదు_రకం::పట్టిక_అడ్డువరుస:
                    అడ్డువరుసలు.emplace_వెనుక(అడ్డువరుస);
                    కూడు_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_పునాది_Pretty,
                                           varName(varp), varp->dtypep());
                    ఆపు;
                స్థితి పట్టిక_నమోదు_రకం::FORCEABLE_RESIDUAL: {
                    స్థిరరాశి std::string bounds = boundsString(dims);
                    residual.emplace_వెనుక(చొప్పించు_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    ఆపు;
                }
                స్థితి పట్టిక_నమోదు_రకం::PLAIN_RESIDUAL: {
                    స్థిరరాశి std::string bounds = boundsString(dims);
                    residual.emplace_వెనుక(
                        చొప్పించు_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    ఆపు;
                }
                అప్రమేయ: v3fatalSrc("Bad case");
                }
                ఒకవేళ (రకం == పట్టిక_నమోదు_రకం::పట్టిక_అడ్డువరుస) కొనసాగు;
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), కణుపు_UOr_Struct_DType)) {
                    ఒకవేళ (!sdtypep->packed()) {
                        కూడు_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_పునాది_Pretty,
                                               varName(varp), sdtypep);
                    }
                } లేకపోతే ఒకవేళ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    కూడు_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_పునాది_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                ఒకవేళ (రకం == పట్టిక_నమోదు_రకం::PLAIN_RESIDUAL) {
                    కూడు_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_పునాది_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            ఒకవేళ (!అడ్డువరుసలు.ఖాళీలు()) {
                స్థిరరాశి కొలత_t అడ్డువరుస_లెక్క = అడ్డువరుసలు.కొలత();
                std::string కీలకం;
                కొరకు (స్థిరరాశి std::string& r : అడ్డువరుసలు) {
                    కీలకం += r;
                    కీలకం += '\0';
                }
                స్థిరరాశి స్వయంచాలక itt = పట్టిక_By_అడ్డువరుసలు.కనుగొను(కీలకం);
                std::string పట్టిక_Name;
                ఒకవేళ (itt != పట్టిక_By_అడ్డువరుసలు.చివర()) {
                    పట్టిక_Name = itt->రెండవ;
                } లేకపోతే {
                    పట్టిక_Name = modClassName + "__VpiVarTable" + std::గమ్యం_string(పట్టిక_Counter++);
                    పట్టిక_By_అడ్డువరుసలు.emplace(std::move(కీలకం), పట్టిక_Name);
                    m_varTables.emplace_వెనుక(పట్టిక_Name, std::move(అడ్డువరుసలు));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + పట్టిక_Name + ", "
                                   + std::గమ్యం_string(అడ్డువరుస_లెక్క) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.కూడు(call);
            }
            కొరకు (స్థిరరాశి std::string& r : residual) stmts.కూడు(r);
        }
    }

    తిరిగి stmts;
}

EmitCSyms::పాఠ్యం_Stmts EmitCSyms::getSymDtorStmts() {
    పాఠ్యం_Stmts stmts;

    emitScopeHier(stmts, నిజం);
    ఒకవేళ (v3Global.needTraceDumper()) {
        stmts.కూడు("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    ఒకవేళ (v3Global.opt.profPgo()) {
        stmts.కూడు("vlSymsp->_vm_pgoProfiler.write(\"" + శిఖరం_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    ఒకవేళ (!m_iface_Ref_పట్టిక_అడ్డువరుసలు.ఖాళీలు()) {
        stmts.కూడు("// Tear down interface references");
        stmts.కూడు("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_పట్టిక_Name + ", "
                  + std::గమ్యం_string(m_iface_Ref_పట్టిక_అడ్డువరుసలు.కొలత()) + ", vlSymsp);");
    }

    stmts.కూడు("// Tear down scopes");
    కొరకు (స్థిరరాశి స్వయంచాలక& itpair : m_scope_పేర్లు) {
        స్థిరరాశి Scope_దత్తాంశం& sd = itpair.రెండవ;
        స్థిరరాశి std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.కూడు("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.కూడు("// Tear down sub module instances");
    కొరకు (స్థిరరాశి ScopeModPair& itpair : vlstd::తిరగేయి_view(m_scopes)) {
        స్థిరరాశి AstScope* స్థిరరాశి scopep = itpair.మొదటి;
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = itpair.రెండవ;
        ఒకవేళ (modp->is_శిఖరం()) కొనసాగు;
        stmts.కూడు("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    తిరిగి stmts;
}

శూన్య EmitCSyms::emitSymImp(స్థిరరాశి AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    పాఠ్యం_Stmts ctorStmts = getSymCtorStmts();
    పాఠ్యం_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    స్థిరరాశి std::pair<std::string, std::string> విభజించు_Ctor_Dtor_Decls  //
        = emit_విభజించు_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            నిజం, "Symbol table implementation internals",
                            [ఇది]() { emitSymImpPreamble(); });

    open_New_Output_ఆధారం_File(symClassName(), నిజం, నిజం, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    స్థిరరాశి AstScope* స్థిరరాశి శిఖరం_Scopep = netlistp->శిఖరం_Scopep()->scopep();
    స్థిరరాశి std::string శిఖరం_Inst = "vlSymsp->" + instName(శిఖరం_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    ఒకవేళ (!విభజించు_Ctor_Dtor_Decls.మొదటి.ఖాళీలు()) puts("\n" + విభజించు_Ctor_Dtor_Decls.మొదటి + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + శిఖరం_Inst + ", vlSymsp, namep);\n");
    కొరకు (స్థిరరాశి పాఠ్యం_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_పాఠ్యం);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + శిఖరం_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    ఒకవేళ (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    ఒకవేళ (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    ఒకవేళ (v3Global.opt.profPgo() && !v3Global.opt.libCreate().ఖాళీలు()) {
        puts("    , _vm_pgoProfiler{" + std::గమ్యం_string(v3Global.current_Hier_ఖండం_ఖర్చు()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    ఒకవేళ (!విభజించు_Ctor_Dtor_Decls.రెండవ.ఖాళీలు()) puts("\n" + విభజించు_Ctor_Dtor_Decls.రెండవ + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    కొరకు (స్థిరరాశి పాఠ్యం_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_పాఠ్యం);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + శిఖరం_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    ఒకవేళ (v3Global.needTraceDumper()) {
        ఒకవేళ (!optSystemC()) {
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

    ఒకవేళ (v3Global.opt.savable()) {
        కొరకు (స్థిరరాశి తార్కిక& de : {అబద్ధం, నిజం}) {
            స్థిరరాశి std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            స్థిరరాశి std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            స్థిరరాశి std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            ఒకవేళ (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            కొరకు (స్థిరరాశి ScopeModPair& itpair : m_scopes) {
                స్థిరరాశి AstScope* స్థిరరాశి scopep = itpair.మొదటి;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

శూన్య EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    open_New_Output_శీర్షిక_File(శిఖరం_Class_Name() + "__Dpi",
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
    std::సమితి<std::string> struct_పేర్లు;
    కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి nodep : m_dpis) {
        కొరకు (స్థిరరాశి Ast_కణుపు* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_పేర్లు);
            }
        }
    }
    ఒకవేళ (!struct_పేర్లు.ఖాళీలు()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    పూర్ణాంకం మొదటి_Exp = 0;
    పూర్ణాంకం మొదటి_Imp = 0;
    కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి nodep : m_dpis) {
        ఒకవేళ (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) కొనసాగు;

        స్థిరరాశి std::string ఆధారం_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        ఒకవేళ (nodep->dpiExportDispatcher()) {
            ఒకవేళ (!మొదటి_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + ఆధారం_Loc + "\n");
        } లేకపోతే {
            ఒకవేళ (!మొదటి_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + ఆధారం_Loc + "\n");
        }
        ఒకవేళ (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } లేకపోతే {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_వాదనలు(nodep, అబద్ధం) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_చివర_Guard();

    closeOutputFile();
}

శూన్య EmitCSyms::emitDpiStruct(స్థిరరాశి Ast_కణుపు_DType* dtypep, std::సమితి<std::string>& namesr) {
    ఒకవేళ (!dtypep->కలిగియుంది_Unpacked_Struct()) తిరిగి;
    స్థిరరాశి AstStructDType* స్థిరరాశి sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    స్థిరరాశి std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    ఒకవేళ (namesr.లెక్క(name)) తిరిగి;
    // After the structures it contains
    కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    ఒకవేళ (name.ఖాళీలు()) తిరిగి;  // Declared where used
    ఒకవేళ (namesr.ఖాళీలు()) puts("// DPI TYPES\n");
    namesr.చొప్పించు(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

శూన్య EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ఆధారం_File(శిఖరం_Class_Name() + "__Dpi", అబద్ధం, నిజం,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + శిఖరం_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + శిఖరం_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + శిఖరం_Class_Name() + ".h\"\n");
    puts("\n");

    కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి nodep : m_dpis) {
        ఒకవేళ (!nodep->dpiExportDispatcher()) కొనసాగు;

        స్థిరరాశి std::string name = nodep->name();
        స్థిరరాశి std::string ఆధారం_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_వాదనలు(nodep) + ") {\n");
        puts("// DPI export" + ఆధారం_Loc + "\n");
        putns(nodep, "return " + శిఖరం_Class_Name() + "::" + name + "(");
        std::string comma;
        కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isIO() && !portp->isFuncReturn()) {
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

శూన్య V3EmitC::emitcSyms(తార్కిక dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
