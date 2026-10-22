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
ساكن constexpr خودکار symClassName = &EmitCUtil::symClassName;
ساكن constexpr خودکار بالاترین_Class_Name = &EmitCUtil::بالاترین_Class_Name;

صنف EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ثابت VNUser1_درون_Use m_inuser1;

    // TYPES
    بنية Scope_داده final {
        ثابت Ast_گره* m_nodep;  // Scope symbol-table entry corresponds to
        ثابت std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ثابت std::string m_prettyName;  // Pretty (unprotected) name for display
        ثابت std::string m_defName;  // Name registered for this scope
        ثابت صحيح m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_داده(ثابت Ast_گره* nodep, ثابت std::string& symName, ثابت std::string& prettyName,
                  ثابت std::string& defName, صحيح timeunit, ثابت std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    بنية Iface_Ref_داده final {
        ثابت AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_والد_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_داده(ثابت AstScope* scopep, ثابت std::string& والد_Sym,
                     ثابت std::string& suffix, ثابت std::string& name,
                     ثابت std::string& modportName)
            : m_scopep{scopep}
            , m_والد_Sym{والد_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    بنية Scope_Func_داده final {
        ثابت AstScopeName* ثابت m_scopep;  // Scope the DPI export function is registered under
        ثابت AstCFunc* ثابت m_cfuncp;  // DPI export function
        ثابت Ast_گره_Module* ثابت m_modp;  // Module containing the DPI export function
        Scope_Func_داده(ثابت AstScopeName* scopep, ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    بنية Scope_Var_داده final {
        ثابت std::string m_scopeName;  // Full name of the scope containing the variable
        ثابت std::string m_var_پایه_Pretty;  // Pretty base name of the variable, without scope
        ثابت AstVar* ثابت m_varp;  // Public variable
        ثابت Ast_گره_Module* ثابت m_modp;  // Module containing the variable
        ثابت AstScope* ثابت m_scopep;  // Scope containing the variable
        Scope_Var_داده(ثابت std::string& scopeName, ثابت std::string& var_پایه_Pretty,
                     ثابت AstVar* varp, ثابت Ast_گره_Module* modp, ثابت AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_پایه_Pretty{var_پایه_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    استفاده Scope_نامها = std::map<ثابت std::string, Scope_داده>;
    استفاده ScopeModPair = std::pair<ثابت AstScope*, Ast_گره_Module*>;
    استفاده ModVarPair = std::pair<ثابت Ast_گره_Module*, ثابت AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ساكن constexpr صحيح VPI_جدول_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = اشاره_تهی;  // Current function
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    ثابت AstScope* m_scopep = اشاره_تهی;  // Current scope
    std::وکتور<ScopeModPair> m_scopes;  // Every scope by module
    std::وکتور<AstCFunc*> m_dpis;  // DPI functions
    std::وکتور<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ثابت std::string, Scope_Func_داده> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ثابت std::string, Scope_Var_داده> m_scopeVars;  // Each {scope,public-var}
    Scope_نامها m_scope_نامها;  // Each unique AstScopeName. Dpi scopes added later
    Scope_نامها m_dpi_Scope_نامها;  // Each unique AstScopeName for DPI export
    Scope_نامها m_vpiScopeCandidates;  // All scopes for VPI
    std::وکتور<Iface_Ref_داده> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ثابت std::string, std::وکتور<std::string>> m_vpiScopeHierarchy;
    صحيح m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    صحيح m_modCoverBins = 0;  // Per-module coverage bin number
    ثابت منطقي m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_بایتها;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::وکتور<std::pair<std::string, std::وکتور<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_جدول_Name;
    std::وکتور<std::string> m_scope_جدول_ردیفها;
    std::string m_iface_Ref_جدول_Name;
    std::وکتور<std::string> m_iface_Ref_جدول_ردیفها;

    // METHODS
    فراغ emitSymHdr();
    فراغ emitSymImpPreamble();
    فراغ emitVarTables();
    فراغ emitScopeHier(متن_Stmts& stmts, منطقي destroy);
    فراغ emitSymImp(ثابت AstNetlist* netlistp);
    فراغ emitDpiHdr();
    فراغ emitDpiStruct(ثابت Ast_گره_DType* dtypep, std::مجموعه<std::string>& namesr);
    فراغ emitDpiImp();

    متن_Stmts getSymCtorStmts();
    متن_Stmts getSymDtorStmts();

    ساكن فراغ nameCheck(Ast_گره* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        إذا (nodep->name().خالی()) أرجع;
        ثابت AstCFunc* ثابت cfuncp = VN_CAST(nodep, CFunc);
        إذا (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ثابت std::string rsvd = V3_Language_واژگان::isKeyword(nodep->name());
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
        std::string بیرون = scpname;
        std::string::اندازه_type pos;
        طالما ((pos = بیرون.پیدا_کن("__PVT__")) != std::string::npos) بیرون.replace(pos, 7, "");
        إذا (VString::startsWith(بیرون, "TOP__DOT__")) بیرون.replace(0, 10, "");
        إذا (VString::startsWith(بیرون, "TOP.")) بیرون.replace(0, 4, "");
        طالما ((pos = بیرون.پیدا_کن('.')) != std::string::npos) بیرون.replace(pos, 1, "__");
        طالما ((pos = بیرون.پیدا_کن("__DOT__")) != std::string::npos) بیرون.replace(pos, 7, "__");
        أرجع بیرون;
    }

    ساكن string scopeDecodeIdentifier(ثابت std::string& scpname) {
        std::string::اندازه_type pos = std::string::npos;

        // Remove hierarchy
        اندازه_t i = 0;
        // always makes progress
        طالما (i < scpname.درازی()) {
            إذا (scpname[i] == '\\') {
                طالما (i < scpname.درازی() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } وإلا {
                طالما (i < scpname.درازی() && scpname[i] != '.') ++i;
                إذا (i < scpname.درازی()) pos = i++;
            }
        }

        أرجع pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    بنية VarDims final {
        صحيح pdim = 0;  // Packed array dims plus a ranged basic leaf
        صحيح udim = 0;
        std::وکتور<std::pair<صحيح, صحيح>> unpackedLR;  // (left,right), outer-first
        std::وکتور<std::pair<صحيح, صحيح>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::وکتور<std::pair<صحيح, صحيح>> flatten() ثابت {
            std::وکتور<std::pair<صحيح, صحيح>> flat(unpackedLR);
            flat.درج(flat.انتها(), packedLR.cbegin(), packedLR.cend());
            أرجع flat;
        }
    };
    ساكن VarDims getVarDims(ثابت Ast_گره_DType* ثابت ریشه_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        لكل (ثابت Ast_گره_DType* dtypep = ریشه_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            إذا (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(dtypep, گره_Array_DType)) {
                إذا (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_پشت(adtypep->چپ(), adtypep->راست());
                    ++d.pdim;
                } وإلا {
                    d.unpackedLR.emplace_پشت(adtypep->چپ(), adtypep->راست());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } وإلا {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isRanged()) {
                        d.packedLR.emplace_پشت(basicp->چپ(), basicp->راست());
                        ++d.pdim;
                    }
                }
                اكسر;  // Non-array leaf
            }
        }
        أرجع d;
    }

    ساكن VarDims dimsFor(ثابت Scope_Var_داده& svd) { أرجع getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ساكن std::string boundsString(ثابت VarDims& d) {
        std::string bounds;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::مقصد_string(lr.اولی);
            bounds += ",";
            bounds += std::مقصد_string(lr.دومی);
        }
        أرجع bounds;
    }

    ساكن صحيح getUnpackedElements(ثابت Ast_گره_DType* ریشه_Dtypep) {
        صحيح elements = 1;
        لكل (ثابت Ast_گره_DType* dtypep = ریشه_Dtypep; dtypep;) {
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
            ثابت std::اندازه_t suffixPos = signalVarp->name().پیدا_کن(forceControlSuffix);
            ثابت منطقي suffix_یافت = suffixPos != std::string::npos;
            ثابت منطقي suffix_Is_At_انتها
                = suffixPos + forceControlSuffix.درازی() == signalVarp->name().درازی();
            إذا (!(suffix_یافت && suffix_Is_At_انتها)) استمر;
            ثابت std::string پایه_Signal_Name = signalVarp->name().substr(0, suffixPos);
            أرجع std::pair<منطقي, std::string>{درست, پایه_Signal_Name};
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
        stmt += std::مقصد_string(udim);
        إذا (!ent_اندازه.خالی()) {
            stmt += ", ";
            stmt += ent_اندازه;
        } وإلا {
            stmt += ", ";
            stmt += std::مقصد_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ساكن std::string memberVlEnumDir(ثابت AstVar* ثابت varp,
                                       ثابت Ast_گره_DType* ثابت dtypep) {
        std::string بیرون = '(' + varp->vlEnumDir(/*forMember=*/درست);
        ثابت Ast_گره_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (skipDTypep->isSigned()) بیرون += "|VLVF_SIGNED";
        إذا (ثابت AstBasicDType* ثابت basicp = skipDTypep->basicp()) {
            إذا (basicp->keyword() == VBasicDTypeKwd::بیت) بیرون += "|VLVF_BITVAR";
        }
        بیرون += skipDTypep->vlEnumPackedAgg();
        أرجع بیرون + ')';
    }

    // Name of the instance member for the given scope
    ساكن std::string instName(ثابت AstScope* scopep) {
        أرجع "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ساكن std::string درج_Var_Statement(ثابت Scope_Var_داده& svd, ثابت AstScope* ثابت scopep,
                                          ثابت AstVar* ثابت varp, ثابت صحيح udim, ثابت صحيح pdim,
                                          ثابت std::string& bounds) {
        ثابت std::string var_عبارت = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ثابت std::string vlEnumType = varp->vlEnumType();
        ثابت منطقي needs_Ent_اندازه = needs_Emitted_Ent_اندازه(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_اندازه ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_بیرون_Formatter::quoteNameControls(protect(svd.m_var_پایه_Pretty)) + '"';

        إذا (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_عبارت;
            stmt += "), false, ";
        } وإلا إذا (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_عبارت;
            stmt += ".c_str())), true, ";
        } وإلا {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_عبارت;
            stmt += "))), true, ";
        }

        ثابت std::string ent_اندازه = needs_Ent_اندازه
                                        ? "sizeof(" + var_عبارت + ") / "
                                              + std::مقصد_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_اندازه);
        أرجع stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    تعداد صنف جدول_مدخل_نوع : uint8_t {
        جدول_ردیف,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    جدول_مدخل_نوع try_Build_جدول_مدخل(ثابت Scope_Var_داده& svd, ثابت AstVar* ثابت varp,
                                      ثابت AstScope* ثابت scopep,
                                      ثابت std::string& modClassName, ثابت VarDims& dims,
                                      std::string& ردیف_بیرون) ثابت {
        ثابت صحيح pdim = dims.pdim;
        ثابت صحيح udim = dims.udim;
        إذا (varp->isForceable() && force_Control_سیگنالها_Are_Valid(scopep, varp)) {
            أرجع جدول_مدخل_نوع::FORCEABLE_RESIDUAL;
        }
        إذا (udim + pdim > VPI_جدول_MAX_DIMS) أرجع جدول_مدخل_نوع::PLAIN_RESIDUAL;

        ثابت std::string vlEnumType = varp->vlEnumType();
        إذا (needs_Emitted_Ent_اندازه(vlEnumType))
            أرجع جدول_مدخل_نوع::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        إذا (varp->isParam()) أرجع جدول_مدخل_نوع::PLAIN_RESIDUAL;

        ثابت std::string name = V3_بیرون_Formatter::quoteNameControls(protect(svd.m_var_پایه_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ثابت std::string member = varName(varp);
        ثابت std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::وکتور<صحيح> dv;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : dims.flatten()) {
            dv.افزودن_پشت(lr.اولی);
            dv.افزودن_پشت(lr.دومی);
        }

        std::string ردیف = "{\"" + name + "\", ";
        ردیف += "offsetof(" + modClassName + ", " + member + "), ";
        ردیف += vlEnumType + ", ";
        ردیف += "(" + dir + "), ";
        ردیف += std::مقصد_string(udim) + ", " + std::مقصد_string(pdim) + ", {";
        لكل (صحيح i = 0; i < VPI_جدول_MAX_DIMS * 2; ++i) {
            إذا (i) ردیف += ", ";
            ردیف += std::مقصد_string(i < static_cast<صحيح>(dv.اندازه()) ? dv[i] : 0);
        }
        ردیف += "}}";
        ردیف_بیرون = ردیف;
        أرجع جدول_مدخل_نوع::جدول_ردیف;
    }

    ساكن std::string درج_DType_Var_Statement(ثابت Scope_Var_داده& svd,
                                               ثابت AstScope* ثابت scopep,
                                               ثابت std::string& prettyName,
                                               ثابت std::string& cName,
                                               ثابت Ast_گره_DType* ثابت dtypep, ثابت صحيح udim,
                                               ثابت صحيح pdim, ثابت std::string& bounds) {
        ثابت std::string vlEnumType = dtypep->vlEnumType();
        ثابت منطقي needs_Ent_اندازه = needs_Emitted_Ent_اندازه(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_اندازه ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_بیرون_Formatter::quoteNameControls(prettyName) + '"';
        ثابت std::string var_عبارت = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        إذا (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_عبارت;
            stmt += "))), true, ";
        } وإلا {
            stmt += ", &(";
            stmt += var_عبارت;
            stmt += "), false, ";
        }
        ثابت std::string ent_اندازه
            = needs_Ent_اندازه
                  ? "sizeof(" + var_عبارت + ") / " + std::مقصد_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_اندازه);
        أرجع stmt;
    }

    ساكن فراغ جمع_کن_UOr_Struct_Member_Vars(std::وکتور<std::string>& stmts, ثابت Scope_Var_داده& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_پیشوند, ثابت std::string& c_پیشوند,
                                       ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep) {
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_گره_DType* ثابت قلم_DTypep = itemp->dtypep();
            إذا (قلم_DTypep->vlEnumType().خالی()) استمر;
            ثابت std::string prettyName
                = pretty_پیشوند + "." + Ast_گره::vpiName(itemp->shortName());
            ثابت std::string cName = c_پیشوند + "." + itemp->nameProtect();
            ثابت VarDims dims = getVarDims(قلم_DTypep);
            stmts.emplace_پشت(درج_DType_Var_Statement(svd, scopep, prettyName, cName, قلم_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            جمع_کن_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, قلم_DTypep);
            إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت subp
                = VN_CAST(قلم_DTypep->skipRefp(), گره_UOr_Struct_DType)) {
                إذا (!subp->packed())
                    جمع_کن_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } وإلا إذا (VN_IS(قلم_DTypep->skipRefp(), UnpackArrayDType)) {
                جمع_کن_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    قلم_DTypep);
            }
        }
    }

    ساكن فراغ جمع_کن_Unpacked_Array_UOr_Struct_Member_Vars(std::وکتور<std::string>& stmts,
                                                    ثابت Scope_Var_داده& svd,
                                                    ثابت AstScope* ثابت scopep,
                                                    ثابت std::string& pretty_پیشوند,
                                                    ثابت std::string& c_پیشوند,
                                                    ثابت Ast_گره_DType* ثابت dtypep) {
        ثابت Ast_گره_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            جمع_کن_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_پیشوند, c_پیشوند + "[0]",
                                                adtypep->subDTypep());
        } وإلا إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(skipDTypep, گره_UOr_Struct_DType)) {
            إذا (!sdtypep->packed())
                جمع_کن_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_پیشوند, c_پیشوند, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ساكن فراغ جمع_کن_Packed_Member_Vars(std::وکتور<std::string>& stmts, ثابت Scope_Var_داده& svd,
                                    ثابت AstScope* ثابت scopep, ثابت std::string& pretty_پیشوند,
                                    ثابت std::string& cName, ثابت std::string& vlEnumType,
                                    ثابت std::string& ent_اندازه,
                                    ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep,
                                    ثابت صحيح والد_بیت_جابه_جایی, ثابت منطقي needs_شاخص) {
        ثابت std::string var_عبارت = "vlSymsp->" + instName(scopep) + "." + cName;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_گره_DType* ثابت قلم_DTypep = itemp->dtypep();
            ثابت std::string prettyName
                = pretty_پیشوند + "." + Ast_گره::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ثابت صحيح بیت_جابه_جایی = والد_بیت_جابه_جایی + itemp->lsb();
            ثابت VarDims dims = getVarDims(قلم_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_بیرون_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            إذا (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_عبارت + "))), true, ";
            } وإلا {
                stmt += ", &(" + var_عبارت + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, قلم_DTypep);
            إذا (needs_شاخص) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::مقصد_string(بیت_جابه_جایی) + ", ";
            stmt += ent_اندازه + ", ";
            stmt += std::مقصد_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_پشت(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ثابت Ast_گره_DType* subDTypep = قلم_DTypep->skipRefp();
            منطقي isArray = خطأ;
            طالما (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = درست;
            }
            إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت subp = VN_CAST(subDTypep, گره_UOr_Struct_DType)) {
                جمع_کن_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_اندازه,
                                    subp, بیت_جابه_جایی, needs_شاخص || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ساكن فراغ جمع_کن_Packed_Agg_Member_Vars(std::وکتور<std::string>& stmts, ثابت Scope_Var_داده& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_پیشوند, ثابت std::string& cName,
                                       ثابت Ast_گره_DType* ثابت dtypep) {
        std::string elemCName = cName;
        ثابت Ast_گره_DType* elemDTypep = dtypep->skipRefp();
        منطقي isArray = خطأ;
        طالما (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(elemDTypep, گره_Array_DType)) {
            إذا (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = درست;
        }
        ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep = VN_CAST(elemDTypep, گره_UOr_Struct_DType);
        إذا (!sdtypep || !sdtypep->packed()) أرجع;
        ثابت std::string var_عبارت = "vlSymsp->" + instName(scopep) + "." + cName;
        ثابت std::string ent_اندازه
            = "sizeof(" + var_عبارت + ") / " + std::مقصد_string(getUnpackedElements(dtypep));
        جمع_کن_Packed_Member_Vars(stmts, svd, scopep, pretty_پیشوند, elemCName, dtypep->vlEnumType(),
                            ent_اندازه, sdtypep, 0, isArray);
    }

    std::string درج_Forceable_Var_Statement(ثابت Scope_Var_داده& svd, ثابت AstScope* ثابت scopep,
                                            ثابت AstVar* ثابت varp, ثابت صحيح udim,
                                            ثابت صحيح pdim, ثابت std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_بیرون_Formatter::quoteNameControls(protect(svd.m_var_پایه_Pretty)) + '"';

        ثابت std::string var_عبارت = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        أكد(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_عبارت;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_عبارت + "__VforceRd";
        stmt += "), \"" + V3_بیرون_Formatter::quoteNameControls(protect(svd.m_var_پایه_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ثابت std::string enable_Signal_کلید
                = get_کلید_Name(scopep, varp->name() + forceControlSuffix);
            ثابت std::map<ثابت std::string, Scope_Var_داده>::const_iterator itpair
                = m_scopeVars.پیدا_کن(enable_Signal_کلید);

            إذا (itpair == m_scopeVars.انتها()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_کلید << "'.");
            }

            ثابت Scope_Var_داده& svd = itpair->دومی;
            ثابت AstScope* ثابت scopep = svd.m_scopep;
            ثابت AstVar* ثابت varp = svd.m_varp;
            ثابت VarDims dims = dimsFor(svd);
            stmt
                += درج_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            إذا (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::مقصد_string(udim);
        stmt += ", ";
        stmt += std::مقصد_string(pdim);
        stmt += bounds;
        stmt += ")";
        أرجع stmt;
    }

    ساكن std::string get_کلید_Name(ثابت AstScope* ثابت scopep, ثابت std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //ترتیب
        // signal can be found.
        std::string کامل = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        إذا (VString::startsWith(کامل, "__DOT__TOP")) کامل.replace(0, 10, "");
        ثابت std::string::اندازه_type dpos = کامل.rfind("__DOT__");
        إذا (dpos != std::string::npos) { scpName = کامل.substr(0, dpos); }
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        أرجع scpSym + " " + signal_name;
    }

    منطقي پایه_Signal_Is_Valid(ثابت AstScope* ثابت scopep, ثابت AstVar* ثابت controlSignalVarp,
                           ثابت std::string& پایه_Signal_Name) ثابت {
        ثابت std::string پایه_Signal_کلید = get_کلید_Name(scopep, پایه_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_داده>::const_iterator پایه_Signal_It
            = m_scopeVars.پیدا_کن(پایه_Signal_کلید);
        إذا (پایه_Signal_It == m_scopeVars.انتها()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت پایه_Signal_Varp = پایه_Signal_It->دومی.m_varp;
            إذا (m_scopeVars.شمار(پایه_Signal_Name) > 1) {
                پایه_Signal_Varp->v3fatalSrc(
                    "Found Signal " << پایه_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << پایه_Signal_Name << " occurring in the same scope.");
                أرجع خطأ;
            }
            إذا (!پایه_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << پایه_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                أرجع خطأ;
            }
        }
        أرجع درست;
    }

    منطقي پایه_Signal_Is_عمومی(ثابت AstScope* ثابت scopep,
                            ثابت std::string& پایه_Signal_Name) ثابت {
        ثابت std::string پایه_Signal_کلید = get_کلید_Name(scopep, پایه_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_داده>::const_iterator پایه_Signal_It
            = m_scopeVars.پیدا_کن(پایه_Signal_کلید);
        إذا (پایه_Signal_It == m_scopeVars.انتها()) {
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت پایه_Signal_Varp = پایه_Signal_It->دومی.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            إذا (!پایه_Signal_Varp->is_Sig_عمومی()) أرجع خطأ;
        }
        أرجع درست;
    }

    منطقي force_Control_سیگنالها_Are_Valid(ثابت AstScope* ثابت scopep,
                                     ثابت AstVar* ثابت پایه_Signal_Varp) ثابت {
        constexpr std::array<ثابت حرف*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        أرجع std::all_از(
            forceControlSuffixes.begin(), forceControlSuffixes.انتها(),
            [scopep, پایه_Signal_Varp, هذا](ثابت std::string& forceControlSuffix) {
                ثابت std::string controlSignalName = پایه_Signal_Varp->name() + forceControlSuffix;
                ثابت std::string control_Signal_کلید = get_کلید_Name(scopep, controlSignalName);
                ثابت std::اندازه_t control_Signal_شمار = m_scopeVars.شمار(control_Signal_کلید);
                إذا (control_Signal_شمار == 0) {
                    پایه_Signal_Varp->v3fatalSrc("Signal "
                                               << پایه_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_کلید << "'.");
                    أرجع خطأ;
                }
                إذا (control_Signal_شمار > 1) {
                    پایه_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << پایه_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    أرجع خطأ;
                }
                أرجع درست;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    فراغ varHierarchyScopes(std::string scp) {

        std::string::اندازه_type prd_pos = scp.rfind('.');
        std::string::اندازه_type dot_pos = scp.rfind("__DOT__");

        طالما (!scp.خالی()) {
            ثابت خودکار scpit = m_vpiScopeCandidates.پیدا_کن(scopeSymString(scp));
            إذا ((scpit != m_vpiScopeCandidates.انتها())
                && (m_scope_نامها.پیدا_کن(scp) == m_scope_نامها.انتها())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ثابت خودکار pair = m_scope_نامها.emplace(scpit->دومی.m_symName, scpit->دومی);
                إذا (!pair.دومی) pair.اولی->دومی.m_type = scpit->دومی.m_type;
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

    فراغ جمع_کن_Scope_Var_مدخل(ثابت AstScope* ثابت scopep, ثابت Ast_گره_Module* ثابت modp,
                          ثابت AstVar* ثابت varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string کامل = scopep->name() + "__DOT__" + varp->name();
        إذا (VString::startsWith(کامل, "__DOT__TOP")) کامل.replace(0, 10, "");
        ثابت std::string::اندازه_type dpos = کامل.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ثابت std::string scpName = کامل.substr(0, dpos);
        ثابت std::string var_پایه = کامل.substr(dpos + std::طول_رشته("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ثابت std::string var_پایه_Pretty = Ast_گره::vpiName(VName::dehash(var_پایه));
        ثابت std::string scpPretty = Ast_گره::prettyName(VName::dehash(scpName));
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        إذا (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_نامها.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_پایه_Pretty, varp, modp, scopep));
    }

    فراغ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        لكل (ثابت ScopeModPair& smPair : m_scopes) {
            ثابت AstScope* ثابت scopep = smPair.اولی;
            ثابت Ast_گره_Module* ثابت smodp = smPair.دومی;
            لكل (ثابت ModVarPair& mvPair : m_modVars) {
                ثابت Ast_گره_Module* ثابت modp = mvPair.اولی;
                ثابت AstVar* ثابت varp = mvPair.دومی;
                إذا (modp != smodp) استمر;
                جمع_کن_Scope_Var_مدخل(scopep, modp, varp);
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
        ثابت std::string مسیر = Ast_گره::vpiName(nodep->name());
        ثابت std::string instName = Ast_گره::vpiName(cellp->origName());
        UASSERT_OBJ(مسیر.درازی() > instName.درازی() && VString::دوسر_With(مسیر, instName), nodep,
                    "Interface scope name " << مسیر << " does not end with instance name "
                                            << instName);
        ثابت std::string والد_مسیر = مسیر.substr(0, مسیر.درازی() - instName.درازی());

        لكل (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ثابت std::string refName = Ast_گره::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            إذا (!VString::startsWith(refName, والد_مسیر)) استمر;
            m_ifaceRefs.emplace_پشت(nodep, scopeSymString(intfRefp->والد_Name()), refName,
                                     Ast_گره::vpiName(intfRefp->پایه_Name()),
                                     intfRefp->modportName());
        }
    }

    فراغ build_Iface_Ref_جدول() {
        إذا (m_ifaceRefs.خالی()) أرجع;
        ثابت std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_مرتب_سازی(
            m_ifaceRefs.begin(), m_ifaceRefs.انتها(),
            [](ثابت Iface_Ref_داده& a, ثابت Iface_Ref_داده& b) { أرجع a.m_suffix < b.m_suffix; });
        لكل (ثابت Iface_Ref_داده& ird : m_ifaceRefs) {
            ثابت std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            إذا (m_scope_نامها.پیدا_کن(scopeSym) == m_scope_نامها.انتها()) استمر;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string والد_جابه_جایی = "VL_IFACEREF_NO_PARENT";
            ثابت خودکار والد = m_scope_نامها.پیدا_کن(ird.m_والد_Sym);
            إذا (والد != m_scope_نامها.انتها()) {
                والد_جابه_جایی = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + والد->دومی.m_symName) + ")";
            }
            std::string ردیف
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            ردیف += والد_جابه_جایی + ", \"";
            ردیف += V3_بیرون_Formatter::quoteNameControls(VIdProtect::protect_واژگان_If(ird.m_name, درست));
            ردیف += "\", \"";
            ردیف += V3_بیرون_Formatter::quoteNameControls(
                VIdProtect::protect_واژگان_If(ird.m_suffix, درست));
            ردیف += "\", \"";
            ردیف += V3_بیرون_Formatter::quoteNameControls(
                VIdProtect::protect_واژگان_If(ird.m_modportName, درست));
            ردیف += "\"}";
            m_iface_Ref_جدول_ردیفها.emplace_پشت(std::move(ردیف));
        }
        إذا (!m_iface_Ref_جدول_ردیفها.خالی()) m_iface_Ref_جدول_Name = symClass + "__VpiIfaceRefTable";
    }

    فراغ buildVpiHierarchy() {
        لكل (ثابت خودکار& itpair : m_scope_نامها) {
            ثابت std::string symName = itpair.دومی.m_symName;
            std::string above = symName;
            إذا (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            طالما (!above.خالی()) {
                ثابت std::string::اندازه_type pos = above.rfind("__");
                إذا (pos == std::string::npos) اكسر;
                above.resize(pos);
                إذا (m_vpiScopeHierarchy.پیدا_کن(above) != m_vpiScopeHierarchy.انتها()) {
                    m_vpiScopeHierarchy[above].افزودن_پشت(symName);
                    اكسر;
                }
            }
            m_vpiScopeHierarchy[symName] = std::وکتور<string>();
        }
    }

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        إذا (v3Global.opt.vpi()) buildVpiHierarchy();

        إذا (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            لكل (ثابت خودکار& scp : m_dpi_Scope_نامها) m_scope_نامها.emplace(scp.اولی, scp.دومی);
        }

        // Sort by names, so line/process order matters less
        std::stable_مرتب_سازی(m_scopes.begin(), m_scopes.انتها(),
                         [](ثابت ScopeModPair& a, ثابت ScopeModPair& b) {
                             أرجع a.اولی->name() < b.اولی->name();
                         });
        std::stable_مرتب_سازی(m_dpis.begin(), m_dpis.انتها(),  //
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
    فراغ بازدید(Ast_گره_Module* nodep) override {
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
    فراغ بازدید(AstCellInlineScope* nodep) override {
        ثابت std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ثابت std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ثابت صحيح timeunit = m_modp->timeunit().power_از_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_گره::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    فراغ بازدید(AstScope* nodep) override {
        إذا (VN_IS(m_modp, Class)) أرجع;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_پشت(nodep, m_modp);

        إذا (v3Global.opt.vpi() && !nodep->is_بالاترین()) {
            ثابت std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ثابت صحيح timeunit = m_modp->timeunit().power_از_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_گره::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstScopeName* nodep) override {
        إذا (nodep->selfRelative()) أرجع;  // Its absolute copy is under the scope
        ثابت std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ثابت صحيح timeunit = m_modp ? m_modp->timeunit().power_از_Ten() : 0;
        m_dpi_Scope_نامها.emplace(  //
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
            m_dpi_Scope_نامها.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    فراغ بازدید(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        إذا ((nodep->is_Sig_User_Rd_عمومی() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().خالی()) {
            m_modVars.emplace_پشت(m_modp, nodep);
        }
    }
    فراغ بازدید(Ast_گره_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        إذا (!nodep->داده_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_رقم(m_coverBins);
            m_coverBins += nodep->اندازه();
            nodep->local_Bin_رقم(m_modCoverBins);
            m_modCoverBins += nodep->اندازه();
        }
    }
    فراغ بازدید(AstCFunc* nodep) override {
        nameCheck(nodep);
        إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.افزودن_پشت(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    فراغ بازدید(AstConst*) override {}
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    صریح EmitCSyms(AstNetlist* nodep, منطقي dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

فراغ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_سرآیند_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_بالاترین_Include();
    puts("#include \"verilated.h\"\n");
    إذا (v3Global.needTraceDumper()) {
        لكل (ثابت string& پایه : v3Global.opt.trace_منبع_Langs())
            puts("#include \"" + پایه + ".h\"\n");
    }
    إذا (v3Global.opt.استفاده_میکند_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + بالاترین_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    لكل (Ast_گره_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), گره_Module);
        إذا (VN_IS(nodep, Class)) استمر;  // Class included earlier
        إذا (nodep->isConstPool()) استمر;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::پیشوند_Name_Protect(nodep) + ".h\"\n");
    }

    إذا (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::مجموعه<std::string> types;  // Remove duplicates and sort
        لكل (ثابت خودکار& itpair : m_scopeFuncs) {
            ثابت AstCFunc* ثابت funcp = itpair.دومی.m_cfuncp;
            إذا (!funcp->dpiExportImpl()) استمر;
            ثابت std::string cbtype
                = protect(v3Global.opt.پیشوند() + "__Vcb_" + funcp->cname() + "_t");
            ثابت std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_آرگومانها(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        لكل (ثابت std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_خصوصی(خطأ);  // public:

    puts("// INTERNAL STATE\n");
    puts(بالاترین_Class_Name() + "* const __Vm_modelp;\n");

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
        puts("VlPgoProfiler<" + std::مقصد_string(ExecMTask::رقم_استفاده_شده_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.اولی;
        ثابت Ast_گره_Module* ثابت modp = itpair.دومی;
        إذا (VN_IS(modp, Class)) استمر;
        ofp()->چاپ_کن("%-30s ", EmitCUtil::پیشوند_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    إذا (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.نخها() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::مقصد_string(m_coverBins));
        puts("];\n");
    }

    إذا (!m_scope_نامها.خالی()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        لكل (ثابت خودکار& itpair : m_scope_نامها) {
            ثابت Scope_داده& sd = itpair.دومی;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    إذا (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + بالاترین_Class_Name()
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

    ofp()->puts_انتها_Guard();
    closeOutputFile();
}

فراغ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    منطقي needs_New_سطر = خطأ;
    لكل (ثابت خودکار& itpair : m_scopeFuncs) {
        ثابت AstCFunc* ثابت funcp = itpair.دومی.m_cfuncp;
        إذا (!funcp->dpiExportImpl()) استمر;
        emitCFuncDecl(funcp, itpair.دومی.m_modp);
        needs_New_سطر = درست;
    }
    إذا (needs_New_سطر) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::مجموعه<std::string> lib_نامها;
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        إذا (itpair.دومی->verilatorLib()) lib_نامها.emplace(itpair.دومی->origName());
    }
    لكل (ثابت std::string& libName : lib_نامها) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    إذا (!lib_نامها.خالی()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    إذا (!m_varTables.خالی() || !m_scope_جدول_ردیفها.خالی() || !m_iface_Ref_جدول_ردیفها.خالی()) {
        لكل (ثابت خودکار& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.اولی + "[];\n");
        }
        إذا (!m_scope_جدول_ردیفها.خالی()) {
            puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[];\n");
        }
        إذا (!m_iface_Ref_جدول_ردیفها.خالی()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[];\n");
        }
        puts("\n");
    }
}

فراغ EmitCSyms::emitVarTables() {
    إذا (m_varTables.خالی() && m_scope_جدول_ردیفها.خالی() && m_iface_Ref_جدول_ردیفها.خالی()) أرجع;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    لكل (ثابت خودکار& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.اولی + "[] = {\n");
        لكل (ثابت std::string& ردیف : kv.دومی) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ردیف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_scope_جدول_ردیفها.خالی()) {
        puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[] = {\n");
        لكل (ثابت std::string& ردیف : m_scope_جدول_ردیفها) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ردیف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_iface_Ref_جدول_ردیفها.خالی()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[] = {\n");
        لكل (ثابت std::string& ردیف : m_iface_Ref_جدول_ردیفها) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ردیف);
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
        stmts.جمع_کن("// Tear down scope hierarchy");
    } وإلا {
        stmts.جمع_کن("// Set up scope hierarchy");
    }

    ثابت std::string روش = destroy ? "remove" : "add";
    لكل (ثابت خودکار& itpair : m_scope_نامها) {
        إذا (itpair.اولی == "TOP") استمر;
        ثابت Scope_داده& sd = itpair.دومی;
        ثابت std::string& name = sd.m_prettyName;
        ثابت std::string& scopeType = sd.m_type;
        إذا (name.پیدا_کن('.') != string::npos) استمر;
        إذا (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") استمر;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جمع_کن("vlSymsp->__Vhier." + روش + "(0, " + id + ");");
    }
    لكل (ثابت خودکار& itpair : m_vpiScopeHierarchy) {
        ثابت std::string fromName = scopeSymString(itpair.اولی);
        ثابت std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_نامها.at(fromName).m_symName);
        لكل (ثابت std::string& name : itpair.دومی) {
            ثابت std::string مقصد_Name = scopeSymString(name);
            ثابت std::string مقصد_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_نامها.at(مقصد_Name).m_symName);
            stmts.جمع_کن("vlSymsp->__Vhier." + روش + "(" + fromId + ", " + مقصد_Id + ");");
        }
    }

    إذا (destroy) {
        stmts.جمع_کن("// Clear keys from hierarchy map after values have been removed");
        stmts.جمع_کن("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::متن_Stmts EmitCSyms::getSymCtorStmts() {
    متن_Stmts stmts;

    {
        uint64_t پشته_اندازه = V3_پشته_شمار::شمار(v3Global.rootp());
        إذا (v3Global.opt.debug_پشته_Check()) پشته_اندازه += 1024 * 1024 * 1024;
        V3Stats::جمع_کن_Stat("Size prediction, Stack (bytes)", پشته_اندازه);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::جمع_کن_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_بایتها);
        V3Stats::جمع_کن_Stat(V3Stats::STAT_MODEL_اندازه, پشته_اندازه + m_stat_Var_Scope_بایتها);

        stmts.جمع_کن("// Check resources");
        stmts.جمع_کن("Verilated::stackCheck(" + std::مقصد_string(پشته_اندازه) + ");");
    }

    stmts.جمع_کن("// Setup sub module instances");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.اولی;
        ثابت Ast_گره_Module* ثابت modp = itpair.دومی;
        إذا (modp->is_بالاترین()) استمر;
        ثابت std::string name = V3_بیرون_Formatter::quoteNameControls(
            VIdProtect::protect_واژگان_If(scopep->prettyName(), scopep->protect()));
        stmts.جمع_کن("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    إذا (v3Global.opt.profPgo()) {
        stmts.جمع_کن("// Configure profiling for PGO\n");
        إذا (!v3Global.opt.hier_فرزند()) {
            stmts.جمع_کن("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        إذا (v3Global.opt.mtasks()) {
            v3Global.rootp()->بالاترین_Modulep()->foreach([&](ثابت Ast_Exec_گراف* execGraphp) {
                لكل (ثابت V3_گراف_رأس& vtx : execGraphp->depGraphp()->vertices()) {
                    ثابت ExecMTask& mt = static_cast<ثابت ExecMTask&>(vtx);
                    stmts.جمع_کن("vlSymsp->_vm_pgoProfiler.addCounter(" + std::مقصد_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.جمع_کن("// Configure time unit / time precision");
    إذا (!v3Global.rootp()->timeunit().isNone()) {
        ثابت std::string unit = std::مقصد_string(v3Global.rootp()->timeunit().power_از_Ten());
        stmts.جمع_کن("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    إذا (!v3Global.rootp()->timeprecision().isNone()) {
        ثابت std::string prec = std::مقصد_string(v3Global.rootp()->timeprecision().power_از_Ten());
        stmts.جمع_کن("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.جمع_کن("// Setup each module's pointers to their submodules");
    لكل (ثابت خودکار& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.اولی;
        ثابت Ast_گره_Module* ثابت modp = i.دومی;
        ثابت AstScope* ثابت abovep = scopep->aboveScopep();
        إذا (!abovep) استمر;

        ثابت std::string protName = VIdProtect::protect_واژگان_If(scopep->name(), scopep->protect());
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
        stmts.جمع_کن(stmt);
    }

    إذا (v3Global.opt.coverage()) {
        stmts.جمع_کن("// Setup each module's pointer back to symbol table (for public functions)");
        لكل (ثابت ScopeModPair& i : m_scopes) {
            ثابت AstScope* ثابت scopep = i.اولی;
            Ast_گره_Module* ثابت modp = i.دومی;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ثابت منطقي اولی = !modp->user1();
            modp->user1(درست);
            stmts.جمع_کن("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (اولی ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    لكل (ثابت ScopeModPair& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.اولی;
        ثابت Ast_گره_Module* ثابت modp = i.دومی;
        إذا (!modp->verilatorLib()) استمر;
        ثابت AstVar* hashp = اشاره_تهی;
        ثابت AstVar* handlep = اشاره_تهی;
        لكل (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) استمر;
            إذا (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            إذا (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ثابت uint32_t hash = VN_AS(hashp->valuep(), Const)->مقصد_UInt();
        stmts.جمع_کن(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::مقصد_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string مسیر = scopep->prettyName();
        إذا (!v3Global.opt.libCreate().خالی()) {
            مسیر.erase(0, v3Global.rootp()->trace_Lib_بالاترین_Name().اندازه() + 1);
        }
        stmts.جمع_کن("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_بیرون_Formatter::quoteNameControls(مسیر) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    إذا (!m_scope_نامها.خالی()) {
        stmts.جمع_کن("// Setup scopes");
        ثابت std::string symClass = symClassName();
        لكل (ثابت خودکار& itpair : m_scope_نامها) {
            ثابت Scope_داده& sd = itpair.دومی;
            std::string ردیف
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            ردیف += V3_بیرون_Formatter::quoteNameControls(
                VIdProtect::protect_واژگان_If(sd.m_prettyName, درست));
            ردیف += "\", \"";
            ردیف += V3_بیرون_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            ردیف += "\", \"";
            ردیف += V3_بیرون_Formatter::quoteNameControls(sd.m_defName);
            ردیف += "\", ";
            ردیف += std::مقصد_string(sd.m_timeunit);
            ردیف += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_جدول_ردیفها.emplace_پشت(std::move(ردیف));
        }
        m_scope_جدول_Name = symClass + "__VpiScopeTable";
        stmts.جمع_کن("VerilatedScope::scopesConstructFromTable(" + m_scope_جدول_Name + ", "
                  + std::مقصد_string(m_scope_نامها.اندازه()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_جدول();
    إذا (!m_iface_Ref_جدول_ردیفها.خالی()) {
        stmts.جمع_کن("// Setup interface references");
        stmts.جمع_کن("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::مقصد_string(m_iface_Ref_جدول_ردیفها.اندازه()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, خطأ);

    إذا (v3Global.dpi()) {
        لكل (ثابت std::string vfinal : {"0", "1"}) {
            stmts.جمع_کن("// Setup export functions - final: " + vfinal);
            لكل (ثابت خودکار& itpair : m_scopeFuncs) {
                ثابت Scope_Func_داده& sfd = itpair.دومی;
                ثابت AstScopeName* ثابت scopep = sfd.m_scopep;
                ثابت AstCFunc* ثابت funcp = sfd.m_cfuncp;
                ثابت Ast_گره_Module* ثابت modp = sfd.m_modp;
                إذا (!funcp->dpiExportImpl()) استمر;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_بیرون_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::پیشوند_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.جمع_کن(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    إذا (!m_scopeVars.خالی()) {
        stmts.جمع_کن("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> جدول_By_ردیفها;
        صحيح جدول_Counter = 0;

        خودکار it = m_scopeVars.cbegin();
        طالما (it != m_scopeVars.cend()) {
            ثابت std::string scopeName = it->دومی.m_scopeName;
            ثابت AstScope* ثابت instScopep = it->دومی.m_scopep;
            ثابت Ast_گره_Module* ثابت modp = it->دومی.m_modp;
            ثابت std::string modClassName = EmitCUtil::پیشوند_Name_Protect(modp);

            std::وکتور<std::string> ردیفها;
            std::وکتور<std::string> residual;

            لكل (; it != m_scopeVars.cend() && it->دومی.m_scopeName == scopeName; ++it) {
                ثابت Scope_Var_داده& svd = it->دومی;
                ثابت AstScope* ثابت scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ثابت AstVar* ثابت varp = svd.m_varp;
                ثابت VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ثابت std::pair<منطقي, std::string> fc = isForceControlSignal(varp);
                إذا (fc.اولی && پایه_Signal_Is_عمومی(scopep, fc.دومی)
                    && پایه_Signal_Is_Valid(scopep, varp, fc.دومی)) {
                    استمر;
                }

                std::string ردیف;
                ثابت جدول_مدخل_نوع نوع
                    = try_Build_جدول_مدخل(svd, varp, scopep, modClassName, dims, ردیف);
                بدل (نوع) {
                حالة جدول_مدخل_نوع::جدول_ردیف:
                    ردیفها.emplace_پشت(ردیف);
                    جمع_کن_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_پایه_Pretty,
                                           varName(varp), varp->dtypep());
                    اكسر;
                حالة جدول_مدخل_نوع::FORCEABLE_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_پشت(درج_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    اكسر;
                }
                حالة جدول_مدخل_نوع::PLAIN_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_پشت(
                        درج_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    اكسر;
                }
                افتراضي: v3fatalSrc("Bad case");
                }
                إذا (نوع == جدول_مدخل_نوع::جدول_ردیف) استمر;
                إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), گره_UOr_Struct_DType)) {
                    إذا (!sdtypep->packed()) {
                        جمع_کن_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_پایه_Pretty,
                                               varName(varp), sdtypep);
                    }
                } وإلا إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    جمع_کن_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_پایه_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                إذا (نوع == جدول_مدخل_نوع::PLAIN_RESIDUAL) {
                    جمع_کن_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_پایه_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            إذا (!ردیفها.خالی()) {
                ثابت اندازه_t ردیف_شمار = ردیفها.اندازه();
                std::string کلید;
                لكل (ثابت std::string& r : ردیفها) {
                    کلید += r;
                    کلید += '\0';
                }
                ثابت خودکار itt = جدول_By_ردیفها.پیدا_کن(کلید);
                std::string جدول_Name;
                إذا (itt != جدول_By_ردیفها.انتها()) {
                    جدول_Name = itt->دومی;
                } وإلا {
                    جدول_Name = modClassName + "__VpiVarTable" + std::مقصد_string(جدول_Counter++);
                    جدول_By_ردیفها.emplace(std::move(کلید), جدول_Name);
                    m_varTables.emplace_پشت(جدول_Name, std::move(ردیفها));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + جدول_Name + ", "
                                   + std::مقصد_string(ردیف_شمار) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.جمع_کن(call);
            }
            لكل (ثابت std::string& r : residual) stmts.جمع_کن(r);
        }
    }

    أرجع stmts;
}

EmitCSyms::متن_Stmts EmitCSyms::getSymDtorStmts() {
    متن_Stmts stmts;

    emitScopeHier(stmts, درست);
    إذا (v3Global.needTraceDumper()) {
        stmts.جمع_کن("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    إذا (v3Global.opt.profPgo()) {
        stmts.جمع_کن("vlSymsp->_vm_pgoProfiler.write(\"" + بالاترین_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    إذا (!m_iface_Ref_جدول_ردیفها.خالی()) {
        stmts.جمع_کن("// Tear down interface references");
        stmts.جمع_کن("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::مقصد_string(m_iface_Ref_جدول_ردیفها.اندازه()) + ", vlSymsp);");
    }

    stmts.جمع_کن("// Tear down scopes");
    لكل (ثابت خودکار& itpair : m_scope_نامها) {
        ثابت Scope_داده& sd = itpair.دومی;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جمع_کن("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.جمع_کن("// Tear down sub module instances");
    لكل (ثابت ScopeModPair& itpair : vlstd::معکوس_view(m_scopes)) {
        ثابت AstScope* ثابت scopep = itpair.اولی;
        ثابت Ast_گره_Module* ثابت modp = itpair.دومی;
        إذا (modp->is_بالاترین()) استمر;
        stmts.جمع_کن("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    أرجع stmts;
}

فراغ EmitCSyms::emitSymImp(ثابت AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    متن_Stmts ctorStmts = getSymCtorStmts();
    متن_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ثابت std::pair<std::string, std::string> تقسیم_Ctor_Dtor_Decls  //
        = emit_تقسیم_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            درست, "Symbol table implementation internals",
                            [هذا]() { emitSymImpPreamble(); });

    open_New_Output_منبع_File(symClassName(), درست, درست, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ثابت AstScope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep()->scopep();
    ثابت std::string بالاترین_Inst = "vlSymsp->" + instName(بالاترین_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    إذا (!تقسیم_Ctor_Dtor_Decls.اولی.خالی()) puts("\n" + تقسیم_Ctor_Dtor_Decls.اولی + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + بالاترین_Inst + ", vlSymsp, namep);\n");
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + بالاترین_Class_Name() + "* modelp)\n");
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
    إذا (v3Global.opt.profPgo() && !v3Global.opt.libCreate().خالی()) {
        puts("    , _vm_pgoProfiler{" + std::مقصد_string(v3Global.current_Hier_بلوک_هزینه()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    إذا (!تقسیم_Ctor_Dtor_Decls.دومی.خالی()) puts("\n" + تقسیم_Ctor_Dtor_Decls.دومی + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + بالاترین_Inst + ");\n");
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
        لكل (ثابت منطقي& de : {خطأ, درست}) {
            ثابت std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ثابت std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ثابت std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            إذا (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            لكل (ثابت ScopeModPair& itpair : m_scopes) {
                ثابت AstScope* ثابت scopep = itpair.اولی;
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

    open_New_Output_سرآیند_File(بالاترین_Class_Name() + "__Dpi",
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
    std::مجموعه<std::string> struct_نامها;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        لكل (ثابت Ast_گره* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_نامها);
            }
        }
    }
    إذا (!struct_نامها.خالی()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    صحيح اولی_Exp = 0;
    صحيح اولی_Imp = 0;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) استمر;

        ثابت std::string منبع_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        إذا (nodep->dpiExportDispatcher()) {
            إذا (!اولی_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + منبع_Loc + "\n");
        } وإلا {
            إذا (!اولی_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + منبع_Loc + "\n");
        }
        إذا (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } وإلا {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_آرگومانها(nodep, خطأ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_انتها_Guard();

    closeOutputFile();
}

فراغ EmitCSyms::emitDpiStruct(ثابت Ast_گره_DType* dtypep, std::مجموعه<std::string>& namesr) {
    إذا (!dtypep->شامل_است_Unpacked_Struct()) أرجع;
    ثابت AstStructDType* ثابت sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ثابت std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    إذا (namesr.شمار(name)) أرجع;
    // After the structures it contains
    لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    إذا (name.خالی()) أرجع;  // Declared where used
    إذا (namesr.خالی()) puts("// DPI TYPES\n");
    namesr.درج(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

فراغ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_منبع_File(بالاترین_Class_Name() + "__Dpi", خطأ, درست,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + بالاترین_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + بالاترین_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + بالاترین_Class_Name() + ".h\"\n");
    puts("\n");

    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher()) استمر;

        ثابت std::string name = nodep->name();
        ثابت std::string منبع_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_آرگومانها(nodep) + ") {\n");
        puts("// DPI export" + منبع_Loc + "\n");
        putns(nodep, "return " + بالاترین_Class_Name() + "::" + name + "(");
        std::string comma;
        لكل (ثابت Ast_گره* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
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
