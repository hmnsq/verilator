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
ساكن constexpr خپلکاری symClassName = &EmitCUtil::symClassName;
ساكن constexpr خپلکاری سر_Class_Name = &EmitCUtil::سر_Class_Name;

صنف EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ثابت VNUser1_دننه_Use m_inuser1;

    // TYPES
    بنية Scope_معلومات final {
        ثابت Ast_غوټه* m_nodep;  // Scope symbol-table entry corresponds to
        ثابت std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ثابت std::string m_prettyName;  // Pretty (unprotected) name for display
        ثابت std::string m_defName;  // Name registered for this scope
        ثابت صحيح m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_معلومات(ثابت Ast_غوټه* nodep, ثابت std::string& symName, ثابت std::string& prettyName,
                  ثابت std::string& defName, صحيح timeunit, ثابت std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    بنية Iface_Ref_معلومات final {
        ثابت AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_پلار_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_معلومات(ثابت AstScope* scopep, ثابت std::string& پلار_Sym,
                     ثابت std::string& suffix, ثابت std::string& name,
                     ثابت std::string& modportName)
            : m_scopep{scopep}
            , m_پلار_Sym{پلار_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    بنية Scope_Func_معلومات final {
        ثابت AstScopeName* ثابت m_scopep;  // Scope the DPI export function is registered under
        ثابت AstCFunc* ثابت m_cfuncp;  // DPI export function
        ثابت Ast_غوټه_Module* ثابت m_modp;  // Module containing the DPI export function
        Scope_Func_معلومات(ثابت AstScopeName* scopep, ثابت AstCFunc* funcp, ثابت Ast_غوټه_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    بنية Scope_Var_معلومات final {
        ثابت std::string m_scopeName;  // Full name of the scope containing the variable
        ثابت std::string m_var_بنسټ_Pretty;  // Pretty base name of the variable, without scope
        ثابت AstVar* ثابت m_varp;  // Public variable
        ثابت Ast_غوټه_Module* ثابت m_modp;  // Module containing the variable
        ثابت AstScope* ثابت m_scopep;  // Scope containing the variable
        Scope_Var_معلومات(ثابت std::string& scopeName, ثابت std::string& var_بنسټ_Pretty,
                     ثابت AstVar* varp, ثابت Ast_غوټه_Module* modp, ثابت AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_بنسټ_Pretty{var_بنسټ_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    کارول Scope_نومونه = std::map<ثابت std::string, Scope_معلومات>;
    کارول ScopeModPair = std::pair<ثابت AstScope*, Ast_غوټه_Module*>;
    کارول ModVarPair = std::pair<ثابت Ast_غوټه_Module*, ثابت AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ساكن constexpr صحيح VPI_جدول_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = تش_اشاره;  // Current function
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    ثابت AstScope* m_scopep = تش_اشاره;  // Current scope
    std::ویکټور<ScopeModPair> m_scopes;  // Every scope by module
    std::ویکټور<AstCFunc*> m_dpis;  // DPI functions
    std::ویکټور<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ثابت std::string, Scope_Func_معلومات> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ثابت std::string, Scope_Var_معلومات> m_scopeVars;  // Each {scope,public-var}
    Scope_نومونه m_scope_نومونه;  // Each unique AstScopeName. Dpi scopes added later
    Scope_نومونه m_dpi_Scope_نومونه;  // Each unique AstScopeName for DPI export
    Scope_نومونه m_vpiScopeCandidates;  // All scopes for VPI
    std::ویکټور<Iface_Ref_معلومات> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ثابت std::string, std::ویکټور<std::string>> m_vpiScopeHierarchy;
    صحيح m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    صحيح m_modCoverBins = 0;  // Per-module coverage bin number
    ثابت منطقي m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_بایټونه;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::ویکټور<std::pair<std::string, std::ویکټور<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_جدول_Name;
    std::ویکټور<std::string> m_scope_جدول_لیکې;
    std::string m_iface_Ref_جدول_Name;
    std::ویکټور<std::string> m_iface_Ref_جدول_لیکې;

    // METHODS
    فراغ emitSymHdr();
    فراغ emitSymImpPreamble();
    فراغ emitVarTables();
    فراغ emitScopeHier(متن_Stmts& stmts, منطقي destroy);
    فراغ emitSymImp(ثابت AstNetlist* netlistp);
    فراغ emitDpiHdr();
    فراغ emitDpiStruct(ثابت Ast_غوټه_DType* dtypep, std::سېټ<std::string>& namesr);
    فراغ emitDpiImp();

    متن_Stmts getSymCtorStmts();
    متن_Stmts getSymDtorStmts();

    ساكن فراغ nameCheck(Ast_غوټه* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        إذا (nodep->name().تشه()) أرجع;
        ثابت AstCFunc* ثابت cfuncp = VN_CAST(nodep, CFunc);
        إذا (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ثابت std::string rsvd = V3_Language_ټکي::isKeyword(nodep->name());
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
        std::string بهر = scpname;
        std::string::اندازه_type pos;
        طالما ((pos = بهر.پیدا_کړه("__PVT__")) != std::string::npos) بهر.replace(pos, 7, "");
        إذا (VString::startsWith(بهر, "TOP__DOT__")) بهر.replace(0, 10, "");
        إذا (VString::startsWith(بهر, "TOP.")) بهر.replace(0, 4, "");
        طالما ((pos = بهر.پیدا_کړه('.')) != std::string::npos) بهر.replace(pos, 1, "__");
        طالما ((pos = بهر.پیدا_کړه("__DOT__")) != std::string::npos) بهر.replace(pos, 7, "__");
        أرجع بهر;
    }

    ساكن string scopeDecodeIdentifier(ثابت std::string& scpname) {
        std::string::اندازه_type pos = std::string::npos;

        // Remove hierarchy
        اندازه_t i = 0;
        // always makes progress
        طالما (i < scpname.اوږدتیا()) {
            إذا (scpname[i] == '\\') {
                طالما (i < scpname.اوږدتیا() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } وإلا {
                طالما (i < scpname.اوږدتیا() && scpname[i] != '.') ++i;
                إذا (i < scpname.اوږدتیا()) pos = i++;
            }
        }

        أرجع pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    بنية VarDims final {
        صحيح pdim = 0;  // Packed array dims plus a ranged basic leaf
        صحيح udim = 0;
        std::ویکټور<std::pair<صحيح, صحيح>> unpackedLR;  // (left,right), outer-first
        std::ویکټور<std::pair<صحيح, صحيح>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::ویکټور<std::pair<صحيح, صحيح>> flatten() ثابت {
            std::ویکټور<std::pair<صحيح, صحيح>> flat(unpackedLR);
            flat.ورننباسه(flat.پای_ټکی(), packedLR.cbegin(), packedLR.cend());
            أرجع flat;
        }
    };
    ساكن VarDims getVarDims(ثابت Ast_غوټه_DType* ثابت ریښه_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        لكل (ثابت Ast_غوټه_DType* dtypep = ریښه_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            إذا (ثابت Ast_غوټه_Array_DType* ثابت adtypep = VN_CAST(dtypep, غوټه_Array_DType)) {
                إذا (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_شا(adtypep->کیڼ(), adtypep->ښی());
                    ++d.pdim;
                } وإلا {
                    d.unpackedLR.emplace_شا(adtypep->کیڼ(), adtypep->ښی());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } وإلا {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isRanged()) {
                        d.packedLR.emplace_شا(basicp->کیڼ(), basicp->ښی());
                        ++d.pdim;
                    }
                }
                اكسر;  // Non-array leaf
            }
        }
        أرجع d;
    }

    ساكن VarDims dimsFor(ثابت Scope_Var_معلومات& svd) { أرجع getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ساكن std::string boundsString(ثابت VarDims& d) {
        std::string bounds;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::منزل_string(lr.لومړی);
            bounds += ",";
            bounds += std::منزل_string(lr.دوهم);
        }
        أرجع bounds;
    }

    ساكن صحيح getUnpackedElements(ثابت Ast_غوټه_DType* ریښه_Dtypep) {
        صحيح elements = 1;
        لكل (ثابت Ast_غوټه_DType* dtypep = ریښه_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType);
            إذا (!adtypep) اكسر;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        أرجع elements;
    }

    ساكن منطقي needs_Emitted_Ent_اندازه(ثابت std::string& vlEnumType) {
        أرجع vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ساكن std::pair<منطقي, std::string> isForceControlSignal(ثابت AstVar* ثابت signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ثابت std::اندازه_t suffixPos = signalVarp->name().پیدا_کړه(forceControlSuffix);
            ثابت منطقي suffix_وموندل = suffixPos != std::string::npos;
            ثابت منطقي suffix_Is_At_پای_ټکی
                = suffixPos + forceControlSuffix.اوږدتیا() == signalVarp->name().اوږدتیا();
            إذا (!(suffix_وموندل && suffix_Is_At_پای_ټکی)) استمر;
            ثابت std::string بنسټ_Signal_Name = signalVarp->name().substr(0, suffixPos);
            أرجع std::pair<منطقي, std::string>{ریښتیا, بنسټ_Signal_Name};
        }
        أرجع std::pair<منطقي, std::string>{خطأ, ""};
    }

    ساكن فراغ appendVarProperties(std::string& stmt, ثابت std::string& vlEnumType,
                                    ثابت std::string& vlEnumDir, ثابت صحيح udim, ثابت صحيح pdim,
                                    ثابت std::string& bounds, ثابت std::string& ent_اندازه = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::منزل_string(udim);
        إذا (!ent_اندازه.تشه()) {
            stmt += ", ";
            stmt += ent_اندازه;
        } وإلا {
            stmt += ", ";
            stmt += std::منزل_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ساكن std::string memberVlEnumDir(ثابت AstVar* ثابت varp,
                                       ثابت Ast_غوټه_DType* ثابت dtypep) {
        std::string بهر = '(' + varp->vlEnumDir(/*forMember=*/ریښتیا);
        ثابت Ast_غوټه_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (skipDTypep->isSigned()) بهر += "|VLVF_SIGNED";
        إذا (ثابت AstBasicDType* ثابت basicp = skipDTypep->basicp()) {
            إذا (basicp->keyword() == VBasicDTypeKwd::بټ) بهر += "|VLVF_BITVAR";
        }
        بهر += skipDTypep->vlEnumPackedAgg();
        أرجع بهر + ')';
    }

    // Name of the instance member for the given scope
    ساكن std::string instName(ثابت AstScope* scopep) {
        أرجع "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ساكن std::string ورننباسه_Var_Statement(ثابت Scope_Var_معلومات& svd, ثابت AstScope* ثابت scopep,
                                          ثابت AstVar* ثابت varp, ثابت صحيح udim, ثابت صحيح pdim,
                                          ثابت std::string& bounds) {
        ثابت std::string var_بیان = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ثابت std::string vlEnumType = varp->vlEnumType();
        ثابت منطقي needs_Ent_اندازه = needs_Emitted_Ent_اندازه(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_اندازه ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_بهر_Formatter::quoteNameControls(protect(svd.m_var_بنسټ_Pretty)) + '"';

        إذا (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_بیان;
            stmt += "), false, ";
        } وإلا إذا (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_بیان;
            stmt += ".c_str())), true, ";
        } وإلا {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_بیان;
            stmt += "))), true, ";
        }

        ثابت std::string ent_اندازه = needs_Ent_اندازه
                                        ? "sizeof(" + var_بیان + ") / "
                                              + std::منزل_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_اندازه);
        أرجع stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    تعداد صنف جدول_ثبت_ډول : uint8_t {
        جدول_لیکه,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    جدول_ثبت_ډول try_Build_جدول_ثبت(ثابت Scope_Var_معلومات& svd, ثابت AstVar* ثابت varp,
                                      ثابت AstScope* ثابت scopep,
                                      ثابت std::string& modClassName, ثابت VarDims& dims,
                                      std::string& لیکه_بهر) ثابت {
        ثابت صحيح pdim = dims.pdim;
        ثابت صحيح udim = dims.udim;
        إذا (varp->isForceable() && force_Control_سیګنالونه_Are_Valid(scopep, varp)) {
            أرجع جدول_ثبت_ډول::FORCEABLE_RESIDUAL;
        }
        إذا (udim + pdim > VPI_جدول_MAX_DIMS) أرجع جدول_ثبت_ډول::PLAIN_RESIDUAL;

        ثابت std::string vlEnumType = varp->vlEnumType();
        إذا (needs_Emitted_Ent_اندازه(vlEnumType))
            أرجع جدول_ثبت_ډول::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        إذا (varp->isParam()) أرجع جدول_ثبت_ډول::PLAIN_RESIDUAL;

        ثابت std::string name = V3_بهر_Formatter::quoteNameControls(protect(svd.m_var_بنسټ_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ثابت std::string member = varName(varp);
        ثابت std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::ویکټور<صحيح> dv;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : dims.flatten()) {
            dv.ټېل_شا(lr.لومړی);
            dv.ټېل_شا(lr.دوهم);
        }

        std::string لیکه = "{\"" + name + "\", ";
        لیکه += "offsetof(" + modClassName + ", " + member + "), ";
        لیکه += vlEnumType + ", ";
        لیکه += "(" + dir + "), ";
        لیکه += std::منزل_string(udim) + ", " + std::منزل_string(pdim) + ", {";
        لكل (صحيح i = 0; i < VPI_جدول_MAX_DIMS * 2; ++i) {
            إذا (i) لیکه += ", ";
            لیکه += std::منزل_string(i < static_cast<صحيح>(dv.اندازه()) ? dv[i] : 0);
        }
        لیکه += "}}";
        لیکه_بهر = لیکه;
        أرجع جدول_ثبت_ډول::جدول_لیکه;
    }

    ساكن std::string ورننباسه_DType_Var_Statement(ثابت Scope_Var_معلومات& svd,
                                               ثابت AstScope* ثابت scopep,
                                               ثابت std::string& prettyName,
                                               ثابت std::string& cName,
                                               ثابت Ast_غوټه_DType* ثابت dtypep, ثابت صحيح udim,
                                               ثابت صحيح pdim, ثابت std::string& bounds) {
        ثابت std::string vlEnumType = dtypep->vlEnumType();
        ثابت منطقي needs_Ent_اندازه = needs_Emitted_Ent_اندازه(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_اندازه ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_بهر_Formatter::quoteNameControls(prettyName) + '"';
        ثابت std::string var_بیان = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        إذا (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_بیان;
            stmt += "))), true, ";
        } وإلا {
            stmt += ", &(";
            stmt += var_بیان;
            stmt += "), false, ";
        }
        ثابت std::string ent_اندازه
            = needs_Ent_اندازه
                  ? "sizeof(" + var_بیان + ") / " + std::منزل_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_اندازه);
        أرجع stmt;
    }

    ساكن فراغ جمع_کړه_UOr_Struct_Member_Vars(std::ویکټور<std::string>& stmts, ثابت Scope_Var_معلومات& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_مختاړی, ثابت std::string& c_مختاړی,
                                       ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep) {
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_غوټه_DType* ثابت توکی_DTypep = itemp->dtypep();
            إذا (توکی_DTypep->vlEnumType().تشه()) استمر;
            ثابت std::string prettyName
                = pretty_مختاړی + "." + Ast_غوټه::vpiName(itemp->shortName());
            ثابت std::string cName = c_مختاړی + "." + itemp->nameProtect();
            ثابت VarDims dims = getVarDims(توکی_DTypep);
            stmts.emplace_شا(ورننباسه_DType_Var_Statement(svd, scopep, prettyName, cName, توکی_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            جمع_کړه_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, توکی_DTypep);
            إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت subp
                = VN_CAST(توکی_DTypep->skipRefp(), غوټه_UOr_Struct_DType)) {
                إذا (!subp->packed())
                    جمع_کړه_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } وإلا إذا (VN_IS(توکی_DTypep->skipRefp(), UnpackArrayDType)) {
                جمع_کړه_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    توکی_DTypep);
            }
        }
    }

    ساكن فراغ جمع_کړه_Unpacked_Array_UOr_Struct_Member_Vars(std::ویکټور<std::string>& stmts,
                                                    ثابت Scope_Var_معلومات& svd,
                                                    ثابت AstScope* ثابت scopep,
                                                    ثابت std::string& pretty_مختاړی,
                                                    ثابت std::string& c_مختاړی,
                                                    ثابت Ast_غوټه_DType* ثابت dtypep) {
        ثابت Ast_غوټه_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            جمع_کړه_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_مختاړی, c_مختاړی + "[0]",
                                                adtypep->subDTypep());
        } وإلا إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(skipDTypep, غوټه_UOr_Struct_DType)) {
            إذا (!sdtypep->packed())
                جمع_کړه_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_مختاړی, c_مختاړی, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ساكن فراغ جمع_کړه_Packed_Member_Vars(std::ویکټور<std::string>& stmts, ثابت Scope_Var_معلومات& svd,
                                    ثابت AstScope* ثابت scopep, ثابت std::string& pretty_مختاړی,
                                    ثابت std::string& cName, ثابت std::string& vlEnumType,
                                    ثابت std::string& ent_اندازه,
                                    ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep,
                                    ثابت صحيح پلار_بټ_دننه_واټن, ثابت منطقي needs_شاخص) {
        ثابت std::string var_بیان = "vlSymsp->" + instName(scopep) + "." + cName;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_غوټه_DType* ثابت توکی_DTypep = itemp->dtypep();
            ثابت std::string prettyName
                = pretty_مختاړی + "." + Ast_غوټه::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ثابت صحيح بټ_دننه_واټن = پلار_بټ_دننه_واټن + itemp->lsb();
            ثابت VarDims dims = getVarDims(توکی_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_بهر_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            إذا (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_بیان + "))), true, ";
            } وإلا {
                stmt += ", &(" + var_بیان + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, توکی_DTypep);
            إذا (needs_شاخص) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::منزل_string(بټ_دننه_واټن) + ", ";
            stmt += ent_اندازه + ", ";
            stmt += std::منزل_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_شا(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ثابت Ast_غوټه_DType* subDTypep = توکی_DTypep->skipRefp();
            منطقي isArray = خطأ;
            طالما (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = ریښتیا;
            }
            إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت subp = VN_CAST(subDTypep, غوټه_UOr_Struct_DType)) {
                جمع_کړه_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_اندازه,
                                    subp, بټ_دننه_واټن, needs_شاخص || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ساكن فراغ جمع_کړه_Packed_Agg_Member_Vars(std::ویکټور<std::string>& stmts, ثابت Scope_Var_معلومات& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_مختاړی, ثابت std::string& cName,
                                       ثابت Ast_غوټه_DType* ثابت dtypep) {
        std::string elemCName = cName;
        ثابت Ast_غوټه_DType* elemDTypep = dtypep->skipRefp();
        منطقي isArray = خطأ;
        طالما (ثابت Ast_غوټه_Array_DType* ثابت adtypep = VN_CAST(elemDTypep, غوټه_Array_DType)) {
            إذا (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = ریښتیا;
        }
        ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep = VN_CAST(elemDTypep, غوټه_UOr_Struct_DType);
        إذا (!sdtypep || !sdtypep->packed()) أرجع;
        ثابت std::string var_بیان = "vlSymsp->" + instName(scopep) + "." + cName;
        ثابت std::string ent_اندازه
            = "sizeof(" + var_بیان + ") / " + std::منزل_string(getUnpackedElements(dtypep));
        جمع_کړه_Packed_Member_Vars(stmts, svd, scopep, pretty_مختاړی, elemCName, dtypep->vlEnumType(),
                            ent_اندازه, sdtypep, 0, isArray);
    }

    std::string ورننباسه_Forceable_Var_Statement(ثابت Scope_Var_معلومات& svd, ثابت AstScope* ثابت scopep,
                                            ثابت AstVar* ثابت varp, ثابت صحيح udim,
                                            ثابت صحيح pdim, ثابت std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_بهر_Formatter::quoteNameControls(protect(svd.m_var_بنسټ_Pretty)) + '"';

        ثابت std::string var_بیان = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        أكد(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_بیان;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_بیان + "__VforceRd";
        stmt += "), \"" + V3_بهر_Formatter::quoteNameControls(protect(svd.m_var_بنسټ_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ثابت std::string enable_Signal_کلی
                = get_کلی_Name(scopep, varp->name() + forceControlSuffix);
            ثابت std::map<ثابت std::string, Scope_Var_معلومات>::const_iterator itpair
                = m_scopeVars.پیدا_کړه(enable_Signal_کلی);

            إذا (itpair == m_scopeVars.پای_ټکی()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_کلی << "'.");
            }

            ثابت Scope_Var_معلومات& svd = itpair->دوهم;
            ثابت AstScope* ثابت scopep = svd.m_scopep;
            ثابت AstVar* ثابت varp = svd.m_varp;
            ثابت VarDims dims = dimsFor(svd);
            stmt
                += ورننباسه_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            إذا (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::منزل_string(udim);
        stmt += ", ";
        stmt += std::منزل_string(pdim);
        stmt += bounds;
        stmt += ")";
        أرجع stmt;
    }

    ساكن std::string get_کلی_Name(ثابت AstScope* ثابت scopep, ثابت std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //ترتیب
        // signal can be found.
        std::string ټولیز = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        إذا (VString::startsWith(ټولیز, "__DOT__TOP")) ټولیز.replace(0, 10, "");
        ثابت std::string::اندازه_type dpos = ټولیز.rfind("__DOT__");
        إذا (dpos != std::string::npos) { scpName = ټولیز.substr(0, dpos); }
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        أرجع scpSym + " " + signal_name;
    }

    منطقي بنسټ_Signal_Is_Valid(ثابت AstScope* ثابت scopep, ثابت AstVar* ثابت controlSignalVarp,
                           ثابت std::string& بنسټ_Signal_Name) ثابت {
        ثابت std::string بنسټ_Signal_کلی = get_کلی_Name(scopep, بنسټ_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_معلومات>::const_iterator بنسټ_Signal_It
            = m_scopeVars.پیدا_کړه(بنسټ_Signal_کلی);
        إذا (بنسټ_Signal_It == m_scopeVars.پای_ټکی()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت بنسټ_Signal_Varp = بنسټ_Signal_It->دوهم.m_varp;
            إذا (m_scopeVars.شمېر(بنسټ_Signal_Name) > 1) {
                بنسټ_Signal_Varp->v3fatalSrc(
                    "Found Signal " << بنسټ_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << بنسټ_Signal_Name << " occurring in the same scope.");
                أرجع خطأ;
            }
            إذا (!بنسټ_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << بنسټ_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                أرجع خطأ;
            }
        }
        أرجع ریښتیا;
    }

    منطقي بنسټ_Signal_Is_عامه(ثابت AstScope* ثابت scopep,
                            ثابت std::string& بنسټ_Signal_Name) ثابت {
        ثابت std::string بنسټ_Signal_کلی = get_کلی_Name(scopep, بنسټ_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_معلومات>::const_iterator بنسټ_Signal_It
            = m_scopeVars.پیدا_کړه(بنسټ_Signal_کلی);
        إذا (بنسټ_Signal_It == m_scopeVars.پای_ټکی()) {
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت بنسټ_Signal_Varp = بنسټ_Signal_It->دوهم.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            إذا (!بنسټ_Signal_Varp->is_Sig_عامه()) أرجع خطأ;
        }
        أرجع ریښتیا;
    }

    منطقي force_Control_سیګنالونه_Are_Valid(ثابت AstScope* ثابت scopep,
                                     ثابت AstVar* ثابت بنسټ_Signal_Varp) ثابت {
        constexpr std::array<ثابت حرف*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        أرجع std::all_د(
            forceControlSuffixes.begin(), forceControlSuffixes.پای_ټکی(),
            [scopep, بنسټ_Signal_Varp, هذا](ثابت std::string& forceControlSuffix) {
                ثابت std::string controlSignalName = بنسټ_Signal_Varp->name() + forceControlSuffix;
                ثابت std::string control_Signal_کلی = get_کلی_Name(scopep, controlSignalName);
                ثابت std::اندازه_t control_Signal_شمېر = m_scopeVars.شمېر(control_Signal_کلی);
                إذا (control_Signal_شمېر == 0) {
                    بنسټ_Signal_Varp->v3fatalSrc("Signal "
                                               << بنسټ_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_کلی << "'.");
                    أرجع خطأ;
                }
                إذا (control_Signal_شمېر > 1) {
                    بنسټ_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << بنسټ_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    أرجع خطأ;
                }
                أرجع ریښتیا;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    فراغ varHierarchyScopes(std::string scp) {

        std::string::اندازه_type prd_pos = scp.rfind('.');
        std::string::اندازه_type dot_pos = scp.rfind("__DOT__");

        طالما (!scp.تشه()) {
            ثابت خپلکاری scpit = m_vpiScopeCandidates.پیدا_کړه(scopeSymString(scp));
            إذا ((scpit != m_vpiScopeCandidates.پای_ټکی())
                && (m_scope_نومونه.پیدا_کړه(scp) == m_scope_نومونه.پای_ټکی())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ثابت خپلکاری pair = m_scope_نومونه.emplace(scpit->دوهم.m_symName, scpit->دوهم);
                إذا (!pair.دوهم) pair.لومړی->دوهم.m_type = scpit->دوهم.m_type;
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

    فراغ جمع_کړه_Scope_Var_ثبت(ثابت AstScope* ثابت scopep, ثابت Ast_غوټه_Module* ثابت modp,
                          ثابت AstVar* ثابت varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string ټولیز = scopep->name() + "__DOT__" + varp->name();
        إذا (VString::startsWith(ټولیز, "__DOT__TOP")) ټولیز.replace(0, 10, "");
        ثابت std::string::اندازه_type dpos = ټولیز.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ثابت std::string scpName = ټولیز.substr(0, dpos);
        ثابت std::string var_بنسټ = ټولیز.substr(dpos + std::مزي_اوږدوالی("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ثابت std::string var_بنسټ_Pretty = Ast_غوټه::vpiName(VName::dehash(var_بنسټ));
        ثابت std::string scpPretty = Ast_غوټه::prettyName(VName::dehash(scpName));
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        إذا (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_نومونه.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_بنسټ_Pretty, varp, modp, scopep));
    }

    فراغ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        لكل (ثابت ScopeModPair& smPair : m_scopes) {
            ثابت AstScope* ثابت scopep = smPair.لومړی;
            ثابت Ast_غوټه_Module* ثابت smodp = smPair.دوهم;
            لكل (ثابت ModVarPair& mvPair : m_modVars) {
                ثابت Ast_غوټه_Module* ثابت modp = mvPair.لومړی;
                ثابت AstVar* ثابت varp = mvPair.دوهم;
                إذا (modp != smodp) استمر;
                جمع_کړه_Scope_Var_ثبت(scopep, modp, varp);
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
        ثابت std::string لار = Ast_غوټه::vpiName(nodep->name());
        ثابت std::string instName = Ast_غوټه::vpiName(cellp->origName());
        UASSERT_OBJ(لار.اوږدتیا() > instName.اوږدتیا() && VString::پایونه_With(لار, instName), nodep,
                    "Interface scope name " << لار << " does not end with instance name "
                                            << instName);
        ثابت std::string پلار_لار = لار.substr(0, لار.اوږدتیا() - instName.اوږدتیا());

        لكل (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ثابت std::string refName = Ast_غوټه::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            إذا (!VString::startsWith(refName, پلار_لار)) استمر;
            m_ifaceRefs.emplace_شا(nodep, scopeSymString(intfRefp->پلار_Name()), refName,
                                     Ast_غوټه::vpiName(intfRefp->بنسټ_Name()),
                                     intfRefp->modportName());
        }
    }

    فراغ build_Iface_Ref_جدول() {
        إذا (m_ifaceRefs.تشه()) أرجع;
        ثابت std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_ترتیبول(
            m_ifaceRefs.begin(), m_ifaceRefs.پای_ټکی(),
            [](ثابت Iface_Ref_معلومات& a, ثابت Iface_Ref_معلومات& b) { أرجع a.m_suffix < b.m_suffix; });
        لكل (ثابت Iface_Ref_معلومات& ird : m_ifaceRefs) {
            ثابت std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            إذا (m_scope_نومونه.پیدا_کړه(scopeSym) == m_scope_نومونه.پای_ټکی()) استمر;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string پلار_دننه_واټن = "VL_IFACEREF_NO_PARENT";
            ثابت خپلکاری پلار = m_scope_نومونه.پیدا_کړه(ird.m_پلار_Sym);
            إذا (پلار != m_scope_نومونه.پای_ټکی()) {
                پلار_دننه_واټن = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + پلار->دوهم.m_symName) + ")";
            }
            std::string لیکه
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            لیکه += پلار_دننه_واټن + ", \"";
            لیکه += V3_بهر_Formatter::quoteNameControls(VIdProtect::protect_ټکي_If(ird.m_name, ریښتیا));
            لیکه += "\", \"";
            لیکه += V3_بهر_Formatter::quoteNameControls(
                VIdProtect::protect_ټکي_If(ird.m_suffix, ریښتیا));
            لیکه += "\", \"";
            لیکه += V3_بهر_Formatter::quoteNameControls(
                VIdProtect::protect_ټکي_If(ird.m_modportName, ریښتیا));
            لیکه += "\"}";
            m_iface_Ref_جدول_لیکې.emplace_شا(std::move(لیکه));
        }
        إذا (!m_iface_Ref_جدول_لیکې.تشه()) m_iface_Ref_جدول_Name = symClass + "__VpiIfaceRefTable";
    }

    فراغ buildVpiHierarchy() {
        لكل (ثابت خپلکاری& itpair : m_scope_نومونه) {
            ثابت std::string symName = itpair.دوهم.m_symName;
            std::string above = symName;
            إذا (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            طالما (!above.تشه()) {
                ثابت std::string::اندازه_type pos = above.rfind("__");
                إذا (pos == std::string::npos) اكسر;
                above.resize(pos);
                إذا (m_vpiScopeHierarchy.پیدا_کړه(above) != m_vpiScopeHierarchy.پای_ټکی()) {
                    m_vpiScopeHierarchy[above].ټېل_شا(symName);
                    اكسر;
                }
            }
            m_vpiScopeHierarchy[symName] = std::ویکټور<string>();
        }
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        إذا (v3Global.opt.vpi()) buildVpiHierarchy();

        إذا (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            لكل (ثابت خپلکاری& scp : m_dpi_Scope_نومونه) m_scope_نومونه.emplace(scp.لومړی, scp.دوهم);
        }

        // Sort by names, so line/process order matters less
        std::stable_ترتیبول(m_scopes.begin(), m_scopes.پای_ټکی(),
                         [](ثابت ScopeModPair& a, ثابت ScopeModPair& b) {
                             أرجع a.لومړی->name() < b.لومړی->name();
                         });
        std::stable_ترتیبول(m_dpis.begin(), m_dpis.پای_ټکی(),  //
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
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
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
    فراغ لیدنه(AstCellInlineScope* nodep) override {
        ثابت std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ثابت std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ثابت صحيح timeunit = m_modp->timeunit().power_د_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_غوټه::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    فراغ لیدنه(AstScope* nodep) override {
        إذا (VN_IS(m_modp, Class)) أرجع;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_شا(nodep, m_modp);

        إذا (v3Global.opt.vpi() && !nodep->is_سر()) {
            ثابت std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ثابت صحيح timeunit = m_modp->timeunit().power_د_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_غوټه::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstScopeName* nodep) override {
        إذا (nodep->selfRelative()) أرجع;  // Its absolute copy is under the scope
        ثابت std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ثابت صحيح timeunit = m_modp ? m_modp->timeunit().power_د_Ten() : 0;
        m_dpi_Scope_نومونه.emplace(  //
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
            m_dpi_Scope_نومونه.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    فراغ لیدنه(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        إذا ((nodep->is_Sig_User_Rd_عامه() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().تشه()) {
            m_modVars.emplace_شا(m_modp, nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        إذا (!nodep->معلومات_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_عدد(m_coverBins);
            m_coverBins += nodep->اندازه();
            nodep->local_Bin_عدد(m_modCoverBins);
            m_modCoverBins += nodep->اندازه();
        }
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        nameCheck(nodep);
        إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ټېل_شا(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    فراغ لیدنه(AstConst*) override {}
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    څرګند EmitCSyms(AstNetlist* nodep, منطقي dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

فراغ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_سرلیک_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_سر_Include();
    puts("#include \"verilated.h\"\n");
    إذا (v3Global.needTraceDumper()) {
        لكل (ثابت string& بنسټ : v3Global.opt.trace_سرچینه_Langs())
            puts("#include \"" + بنسټ + ".h\"\n");
    }
    إذا (v3Global.opt.کاروي_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + سر_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    لكل (Ast_غوټه_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), غوټه_Module);
        إذا (VN_IS(nodep, Class)) استمر;  // Class included earlier
        إذا (nodep->isConstPool()) استمر;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::مختاړی_Name_Protect(nodep) + ".h\"\n");
    }

    إذا (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::سېټ<std::string> types;  // Remove duplicates and sort
        لكل (ثابت خپلکاری& itpair : m_scopeFuncs) {
            ثابت AstCFunc* ثابت funcp = itpair.دوهم.m_cfuncp;
            إذا (!funcp->dpiExportImpl()) استمر;
            ثابت std::string cbtype
                = protect(v3Global.opt.مختاړی() + "__Vcb_" + funcp->cname() + "_t");
            ثابت std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_دلیلونه(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        لكل (ثابت std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_شخصي(خطأ);  // public:

    puts("// INTERNAL STATE\n");
    puts(سر_Class_Name() + "* const __Vm_modelp;\n");

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
        puts("VlPgoProfiler<" + std::منزل_string(ExecMTask::عدد_کارول_شوی_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.لومړی;
        ثابت Ast_غوټه_Module* ثابت modp = itpair.دوهم;
        إذا (VN_IS(modp, Class)) استمر;
        ofp()->چاپ_کړه("%-30s ", EmitCUtil::مختاړی_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    إذا (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.تارونه() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::منزل_string(m_coverBins));
        puts("];\n");
    }

    إذا (!m_scope_نومونه.تشه()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        لكل (ثابت خپلکاری& itpair : m_scope_نومونه) {
            ثابت Scope_معلومات& sd = itpair.دوهم;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    إذا (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + سر_Class_Name()
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

    ofp()->puts_پای_ټکی_Guard();
    closeOutputFile();
}

فراغ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    منطقي needs_New_کرښه = خطأ;
    لكل (ثابت خپلکاری& itpair : m_scopeFuncs) {
        ثابت AstCFunc* ثابت funcp = itpair.دوهم.m_cfuncp;
        إذا (!funcp->dpiExportImpl()) استمر;
        emitCFuncDecl(funcp, itpair.دوهم.m_modp);
        needs_New_کرښه = ریښتیا;
    }
    إذا (needs_New_کرښه) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::سېټ<std::string> lib_نومونه;
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        إذا (itpair.دوهم->verilatorLib()) lib_نومونه.emplace(itpair.دوهم->origName());
    }
    لكل (ثابت std::string& libName : lib_نومونه) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    إذا (!lib_نومونه.تشه()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    إذا (!m_varTables.تشه() || !m_scope_جدول_لیکې.تشه() || !m_iface_Ref_جدول_لیکې.تشه()) {
        لكل (ثابت خپلکاری& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.لومړی + "[];\n");
        }
        إذا (!m_scope_جدول_لیکې.تشه()) {
            puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[];\n");
        }
        إذا (!m_iface_Ref_جدول_لیکې.تشه()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[];\n");
        }
        puts("\n");
    }
}

فراغ EmitCSyms::emitVarTables() {
    إذا (m_varTables.تشه() && m_scope_جدول_لیکې.تشه() && m_iface_Ref_جدول_لیکې.تشه()) أرجع;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    لكل (ثابت خپلکاری& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.لومړی + "[] = {\n");
        لكل (ثابت std::string& لیکه : kv.دوهم) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(لیکه);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_scope_جدول_لیکې.تشه()) {
        puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[] = {\n");
        لكل (ثابت std::string& لیکه : m_scope_جدول_لیکې) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(لیکه);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_iface_Ref_جدول_لیکې.تشه()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[] = {\n");
        لكل (ثابت std::string& لیکه : m_iface_Ref_جدول_لیکې) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(لیکه);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic pop\n");
    puts("#endif\n");
}

فراغ EmitCSyms::emitScopeHier(متن_Stmts& stmts, منطقي destroy) {
    إذا (!v3Global.opt.vpi()) أرجع;

    إذا (destroy) {
        stmts.جمع_کړه("// Tear down scope hierarchy");
    } وإلا {
        stmts.جمع_کړه("// Set up scope hierarchy");
    }

    ثابت std::string لاره = destroy ? "remove" : "add";
    لكل (ثابت خپلکاری& itpair : m_scope_نومونه) {
        إذا (itpair.لومړی == "TOP") استمر;
        ثابت Scope_معلومات& sd = itpair.دوهم;
        ثابت std::string& name = sd.m_prettyName;
        ثابت std::string& scopeType = sd.m_type;
        إذا (name.پیدا_کړه('.') != string::npos) استمر;
        إذا (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") استمر;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جمع_کړه("vlSymsp->__Vhier." + لاره + "(0, " + id + ");");
    }
    لكل (ثابت خپلکاری& itpair : m_vpiScopeHierarchy) {
        ثابت std::string fromName = scopeSymString(itpair.لومړی);
        ثابت std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_نومونه.at(fromName).m_symName);
        لكل (ثابت std::string& name : itpair.دوهم) {
            ثابت std::string منزل_Name = scopeSymString(name);
            ثابت std::string منزل_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_نومونه.at(منزل_Name).m_symName);
            stmts.جمع_کړه("vlSymsp->__Vhier." + لاره + "(" + fromId + ", " + منزل_Id + ");");
        }
    }

    إذا (destroy) {
        stmts.جمع_کړه("// Clear keys from hierarchy map after values have been removed");
        stmts.جمع_کړه("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::متن_Stmts EmitCSyms::getSymCtorStmts() {
    متن_Stmts stmts;

    {
        uint64_t ډېری_اندازه = V3_ډېری_شمېر::شمېر(v3Global.rootp());
        إذا (v3Global.opt.debug_ډېری_Check()) ډېری_اندازه += 1024 * 1024 * 1024;
        V3Stats::جمع_کړه_Stat("Size prediction, Stack (bytes)", ډېری_اندازه);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::جمع_کړه_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_بایټونه);
        V3Stats::جمع_کړه_Stat(V3Stats::STAT_MODEL_اندازه, ډېری_اندازه + m_stat_Var_Scope_بایټونه);

        stmts.جمع_کړه("// Check resources");
        stmts.جمع_کړه("Verilated::stackCheck(" + std::منزل_string(ډېری_اندازه) + ");");
    }

    stmts.جمع_کړه("// Setup sub module instances");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.لومړی;
        ثابت Ast_غوټه_Module* ثابت modp = itpair.دوهم;
        إذا (modp->is_سر()) استمر;
        ثابت std::string name = V3_بهر_Formatter::quoteNameControls(
            VIdProtect::protect_ټکي_If(scopep->prettyName(), scopep->protect()));
        stmts.جمع_کړه("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    إذا (v3Global.opt.profPgo()) {
        stmts.جمع_کړه("// Configure profiling for PGO\n");
        إذا (!v3Global.opt.hier_ماشوم()) {
            stmts.جمع_کړه("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        إذا (v3Global.opt.mtasks()) {
            v3Global.rootp()->سر_Modulep()->foreach([&](ثابت Ast_Exec_ګراف* execGraphp) {
                لكل (ثابت V3_ګراف_راس& vtx : execGraphp->depGraphp()->vertices()) {
                    ثابت ExecMTask& mt = static_cast<ثابت ExecMTask&>(vtx);
                    stmts.جمع_کړه("vlSymsp->_vm_pgoProfiler.addCounter(" + std::منزل_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.جمع_کړه("// Configure time unit / time precision");
    إذا (!v3Global.rootp()->timeunit().isNone()) {
        ثابت std::string unit = std::منزل_string(v3Global.rootp()->timeunit().power_د_Ten());
        stmts.جمع_کړه("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    إذا (!v3Global.rootp()->timeprecision().isNone()) {
        ثابت std::string prec = std::منزل_string(v3Global.rootp()->timeprecision().power_د_Ten());
        stmts.جمع_کړه("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.جمع_کړه("// Setup each module's pointers to their submodules");
    لكل (ثابت خپلکاری& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.لومړی;
        ثابت Ast_غوټه_Module* ثابت modp = i.دوهم;
        ثابت AstScope* ثابت abovep = scopep->aboveScopep();
        إذا (!abovep) استمر;

        ثابت std::string protName = VIdProtect::protect_ټکي_If(scopep->name(), scopep->protect());
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
        stmts.جمع_کړه(stmt);
    }

    إذا (v3Global.opt.coverage()) {
        stmts.جمع_کړه("// Setup each module's pointer back to symbol table (for public functions)");
        لكل (ثابت ScopeModPair& i : m_scopes) {
            ثابت AstScope* ثابت scopep = i.لومړی;
            Ast_غوټه_Module* ثابت modp = i.دوهم;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ثابت منطقي لومړی = !modp->user1();
            modp->user1(ریښتیا);
            stmts.جمع_کړه("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (لومړی ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    لكل (ثابت ScopeModPair& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.لومړی;
        ثابت Ast_غوټه_Module* ثابت modp = i.دوهم;
        إذا (!modp->verilatorLib()) استمر;
        ثابت AstVar* hashp = تش_اشاره;
        ثابت AstVar* handlep = تش_اشاره;
        لكل (ثابت Ast_غوټه* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) استمر;
            إذا (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            إذا (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ثابت uint32_t hash = VN_AS(hashp->valuep(), Const)->منزل_UInt();
        stmts.جمع_کړه(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::منزل_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string لار = scopep->prettyName();
        إذا (!v3Global.opt.libCreate().تشه()) {
            لار.erase(0, v3Global.rootp()->trace_Lib_سر_Name().اندازه() + 1);
        }
        stmts.جمع_کړه("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_بهر_Formatter::quoteNameControls(لار) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    إذا (!m_scope_نومونه.تشه()) {
        stmts.جمع_کړه("// Setup scopes");
        ثابت std::string symClass = symClassName();
        لكل (ثابت خپلکاری& itpair : m_scope_نومونه) {
            ثابت Scope_معلومات& sd = itpair.دوهم;
            std::string لیکه
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            لیکه += V3_بهر_Formatter::quoteNameControls(
                VIdProtect::protect_ټکي_If(sd.m_prettyName, ریښتیا));
            لیکه += "\", \"";
            لیکه += V3_بهر_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            لیکه += "\", \"";
            لیکه += V3_بهر_Formatter::quoteNameControls(sd.m_defName);
            لیکه += "\", ";
            لیکه += std::منزل_string(sd.m_timeunit);
            لیکه += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_جدول_لیکې.emplace_شا(std::move(لیکه));
        }
        m_scope_جدول_Name = symClass + "__VpiScopeTable";
        stmts.جمع_کړه("VerilatedScope::scopesConstructFromTable(" + m_scope_جدول_Name + ", "
                  + std::منزل_string(m_scope_نومونه.اندازه()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_جدول();
    إذا (!m_iface_Ref_جدول_لیکې.تشه()) {
        stmts.جمع_کړه("// Setup interface references");
        stmts.جمع_کړه("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::منزل_string(m_iface_Ref_جدول_لیکې.اندازه()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, خطأ);

    إذا (v3Global.dpi()) {
        لكل (ثابت std::string vfinal : {"0", "1"}) {
            stmts.جمع_کړه("// Setup export functions - final: " + vfinal);
            لكل (ثابت خپلکاری& itpair : m_scopeFuncs) {
                ثابت Scope_Func_معلومات& sfd = itpair.دوهم;
                ثابت AstScopeName* ثابت scopep = sfd.m_scopep;
                ثابت AstCFunc* ثابت funcp = sfd.m_cfuncp;
                ثابت Ast_غوټه_Module* ثابت modp = sfd.m_modp;
                إذا (!funcp->dpiExportImpl()) استمر;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_بهر_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::مختاړی_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.جمع_کړه(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    إذا (!m_scopeVars.تشه()) {
        stmts.جمع_کړه("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> جدول_By_لیکې;
        صحيح جدول_Counter = 0;

        خپلکاری it = m_scopeVars.cbegin();
        طالما (it != m_scopeVars.cend()) {
            ثابت std::string scopeName = it->دوهم.m_scopeName;
            ثابت AstScope* ثابت instScopep = it->دوهم.m_scopep;
            ثابت Ast_غوټه_Module* ثابت modp = it->دوهم.m_modp;
            ثابت std::string modClassName = EmitCUtil::مختاړی_Name_Protect(modp);

            std::ویکټور<std::string> لیکې;
            std::ویکټور<std::string> residual;

            لكل (; it != m_scopeVars.cend() && it->دوهم.m_scopeName == scopeName; ++it) {
                ثابت Scope_Var_معلومات& svd = it->دوهم;
                ثابت AstScope* ثابت scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ثابت AstVar* ثابت varp = svd.m_varp;
                ثابت VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ثابت std::pair<منطقي, std::string> fc = isForceControlSignal(varp);
                إذا (fc.لومړی && بنسټ_Signal_Is_عامه(scopep, fc.دوهم)
                    && بنسټ_Signal_Is_Valid(scopep, varp, fc.دوهم)) {
                    استمر;
                }

                std::string لیکه;
                ثابت جدول_ثبت_ډول ډول
                    = try_Build_جدول_ثبت(svd, varp, scopep, modClassName, dims, لیکه);
                بدل (ډول) {
                حالة جدول_ثبت_ډول::جدول_لیکه:
                    لیکې.emplace_شا(لیکه);
                    جمع_کړه_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_بنسټ_Pretty,
                                           varName(varp), varp->dtypep());
                    اكسر;
                حالة جدول_ثبت_ډول::FORCEABLE_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_شا(ورننباسه_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    اكسر;
                }
                حالة جدول_ثبت_ډول::PLAIN_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_شا(
                        ورننباسه_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    اكسر;
                }
                افتراضي: v3fatalSrc("Bad case");
                }
                إذا (ډول == جدول_ثبت_ډول::جدول_لیکه) استمر;
                إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), غوټه_UOr_Struct_DType)) {
                    إذا (!sdtypep->packed()) {
                        جمع_کړه_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_بنسټ_Pretty,
                                               varName(varp), sdtypep);
                    }
                } وإلا إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    جمع_کړه_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_بنسټ_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                إذا (ډول == جدول_ثبت_ډول::PLAIN_RESIDUAL) {
                    جمع_کړه_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_بنسټ_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            إذا (!لیکې.تشه()) {
                ثابت اندازه_t لیکه_شمېر = لیکې.اندازه();
                std::string کلی;
                لكل (ثابت std::string& r : لیکې) {
                    کلی += r;
                    کلی += '\0';
                }
                ثابت خپلکاری itt = جدول_By_لیکې.پیدا_کړه(کلی);
                std::string جدول_Name;
                إذا (itt != جدول_By_لیکې.پای_ټکی()) {
                    جدول_Name = itt->دوهم;
                } وإلا {
                    جدول_Name = modClassName + "__VpiVarTable" + std::منزل_string(جدول_Counter++);
                    جدول_By_لیکې.emplace(std::move(کلی), جدول_Name);
                    m_varTables.emplace_شا(جدول_Name, std::move(لیکې));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + جدول_Name + ", "
                                   + std::منزل_string(لیکه_شمېر) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.جمع_کړه(call);
            }
            لكل (ثابت std::string& r : residual) stmts.جمع_کړه(r);
        }
    }

    أرجع stmts;
}

EmitCSyms::متن_Stmts EmitCSyms::getSymDtorStmts() {
    متن_Stmts stmts;

    emitScopeHier(stmts, ریښتیا);
    إذا (v3Global.needTraceDumper()) {
        stmts.جمع_کړه("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    إذا (v3Global.opt.profPgo()) {
        stmts.جمع_کړه("vlSymsp->_vm_pgoProfiler.write(\"" + سر_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    إذا (!m_iface_Ref_جدول_لیکې.تشه()) {
        stmts.جمع_کړه("// Tear down interface references");
        stmts.جمع_کړه("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::منزل_string(m_iface_Ref_جدول_لیکې.اندازه()) + ", vlSymsp);");
    }

    stmts.جمع_کړه("// Tear down scopes");
    لكل (ثابت خپلکاری& itpair : m_scope_نومونه) {
        ثابت Scope_معلومات& sd = itpair.دوهم;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جمع_کړه("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.جمع_کړه("// Tear down sub module instances");
    لكل (ثابت ScopeModPair& itpair : vlstd::سرچپه_view(m_scopes)) {
        ثابت AstScope* ثابت scopep = itpair.لومړی;
        ثابت Ast_غوټه_Module* ثابت modp = itpair.دوهم;
        إذا (modp->is_سر()) استمر;
        stmts.جمع_کړه("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    أرجع stmts;
}

فراغ EmitCSyms::emitSymImp(ثابت AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    متن_Stmts ctorStmts = getSymCtorStmts();
    متن_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ثابت std::pair<std::string, std::string> وېشل_Ctor_Dtor_Decls  //
        = emit_وېشل_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            ریښتیا, "Symbol table implementation internals",
                            [هذا]() { emitSymImpPreamble(); });

    open_New_Output_سرچینه_File(symClassName(), ریښتیا, ریښتیا, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ثابت AstScope* ثابت سر_Scopep = netlistp->سر_Scopep()->scopep();
    ثابت std::string سر_Inst = "vlSymsp->" + instName(سر_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    إذا (!وېشل_Ctor_Dtor_Decls.لومړی.تشه()) puts("\n" + وېشل_Ctor_Dtor_Decls.لومړی + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + سر_Inst + ", vlSymsp, namep);\n");
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + سر_Class_Name() + "* modelp)\n");
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
    إذا (v3Global.opt.profPgo() && !v3Global.opt.libCreate().تشه()) {
        puts("    , _vm_pgoProfiler{" + std::منزل_string(v3Global.current_Hier_بلاک_لګښت()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    إذا (!وېشل_Ctor_Dtor_Decls.دوهم.تشه()) puts("\n" + وېشل_Ctor_Dtor_Decls.دوهم + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + سر_Inst + ");\n");
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
        لكل (ثابت منطقي& de : {خطأ, ریښتیا}) {
            ثابت std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ثابت std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ثابت std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            إذا (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            لكل (ثابت ScopeModPair& itpair : m_scopes) {
                ثابت AstScope* ثابت scopep = itpair.لومړی;
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

    open_New_Output_سرلیک_File(سر_Class_Name() + "__Dpi",
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
    std::سېټ<std::string> struct_نومونه;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        لكل (ثابت Ast_غوټه* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_نومونه);
            }
        }
    }
    إذا (!struct_نومونه.تشه()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    صحيح لومړی_Exp = 0;
    صحيح لومړی_Imp = 0;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) استمر;

        ثابت std::string سرچینه_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        إذا (nodep->dpiExportDispatcher()) {
            إذا (!لومړی_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + سرچینه_Loc + "\n");
        } وإلا {
            إذا (!لومړی_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + سرچینه_Loc + "\n");
        }
        إذا (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } وإلا {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_دلیلونه(nodep, خطأ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_پای_ټکی_Guard();

    closeOutputFile();
}

فراغ EmitCSyms::emitDpiStruct(ثابت Ast_غوټه_DType* dtypep, std::سېټ<std::string>& namesr) {
    إذا (!dtypep->لري_Unpacked_Struct()) أرجع;
    ثابت AstStructDType* ثابت sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ثابت std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    إذا (namesr.شمېر(name)) أرجع;
    // After the structures it contains
    لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    إذا (name.تشه()) أرجع;  // Declared where used
    إذا (namesr.تشه()) puts("// DPI TYPES\n");
    namesr.ورننباسه(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

فراغ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_سرچینه_File(سر_Class_Name() + "__Dpi", خطأ, ریښتیا,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + سر_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + سر_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + سر_Class_Name() + ".h\"\n");
    puts("\n");

    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher()) استمر;

        ثابت std::string name = nodep->name();
        ثابت std::string سرچینه_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_دلیلونه(nodep) + ") {\n");
        puts("// DPI export" + سرچینه_Loc + "\n");
        putns(nodep, "return " + سر_Class_Name() + "::" + name + "(");
        std::string comma;
        لكل (ثابت Ast_غوټه* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
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
