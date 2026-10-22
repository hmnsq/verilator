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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitC.h"
#شامل "V3EmitCBase.h"
#شامل "V3ExecGraph.h"
#شامل "V3LanguageWords.h"
#شامل "V3StackCount.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <cstring>
#شامل <map>
#شامل <set>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
ساکن constexpr خودکار symClassName = &EmitCUtil::symClassName;
ساکن constexpr خودکار چوٹی_Class_Name = &EmitCUtil::چوٹی_Class_Name;

جماعت EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ثابت VNUser1_اندر_Use m_inuser1;

    // TYPES
    ساخت Scope_ڈیٹا final {
        ثابت Ast_گرہ* m_nodep;  // Scope symbol-table entry corresponds to
        ثابت std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ثابت std::string m_prettyName;  // Pretty (unprotected) name for display
        ثابت std::string m_defName;  // Name registered for this scope
        ثابت صحیح m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_ڈیٹا(ثابت Ast_گرہ* nodep, ثابت std::string& symName, ثابت std::string& prettyName,
                  ثابت std::string& defName, صحیح timeunit, ثابت std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    ساخت Iface_Ref_ڈیٹا final {
        ثابت AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_والد_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_ڈیٹا(ثابت AstScope* scopep, ثابت std::string& والد_Sym,
                     ثابت std::string& suffix, ثابت std::string& name,
                     ثابت std::string& modportName)
            : m_scopep{scopep}
            , m_والد_Sym{والد_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    ساخت Scope_Func_ڈیٹا final {
        ثابت AstScopeName* ثابت m_scopep;  // Scope the DPI export function is registered under
        ثابت AstCFunc* ثابت m_cfuncp;  // DPI export function
        ثابت Ast_گرہ_Module* ثابت m_modp;  // Module containing the DPI export function
        Scope_Func_ڈیٹا(ثابت AstScopeName* scopep, ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    ساخت Scope_Var_ڈیٹا final {
        ثابت std::string m_scopeName;  // Full name of the scope containing the variable
        ثابت std::string m_var_بنیاد_Pretty;  // Pretty base name of the variable, without scope
        ثابت AstVar* ثابت m_varp;  // Public variable
        ثابت Ast_گرہ_Module* ثابت m_modp;  // Module containing the variable
        ثابت AstScope* ثابت m_scopep;  // Scope containing the variable
        Scope_Var_ڈیٹا(ثابت std::string& scopeName, ثابت std::string& var_بنیاد_Pretty,
                     ثابت AstVar* varp, ثابت Ast_گرہ_Module* modp, ثابت AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_بنیاد_Pretty{var_بنیاد_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    استعمال Scope_نام = std::map<ثابت std::string, Scope_ڈیٹا>;
    استعمال ScopeModPair = std::pair<ثابت AstScope*, Ast_گرہ_Module*>;
    استعمال ModVarPair = std::pair<ثابت Ast_گرہ_Module*, ثابت AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ساکن constexpr صحیح VPI_جدول_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current function
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    ثابت AstScope* m_scopep = خالی_اشاریہ;  // Current scope
    std::سمتیہ<ScopeModPair> m_scopes;  // Every scope by module
    std::سمتیہ<AstCFunc*> m_dpis;  // DPI functions
    std::سمتیہ<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ثابت std::string, Scope_Func_ڈیٹا> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ثابت std::string, Scope_Var_ڈیٹا> m_scopeVars;  // Each {scope,public-var}
    Scope_نام m_scope_نام;  // Each unique AstScopeName. Dpi scopes added later
    Scope_نام m_dpi_Scope_نام;  // Each unique AstScopeName for DPI export
    Scope_نام m_vpiScopeCandidates;  // All scopes for VPI
    std::سمتیہ<Iface_Ref_ڈیٹا> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ثابت std::string, std::سمتیہ<std::string>> m_vpiScopeHierarchy;
    صحیح m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    صحیح m_modCoverBins = 0;  // Per-module coverage bin number
    ثابت منطقی m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_بائٹیں;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::سمتیہ<std::pair<std::string, std::سمتیہ<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_جدول_Name;
    std::سمتیہ<std::string> m_scope_جدول_صفیں;
    std::string m_iface_Ref_جدول_Name;
    std::سمتیہ<std::string> m_iface_Ref_جدول_صفیں;

    // METHODS
    خالی emitSymHdr();
    خالی emitSymImpPreamble();
    خالی emitVarTables();
    خالی emitScopeHier(متن_Stmts& stmts, منطقی destroy);
    خالی emitSymImp(ثابت AstNetlist* netlistp);
    خالی emitDpiHdr();
    خالی emitDpiStruct(ثابت Ast_گرہ_DType* dtypep, std::سیٹ<std::string>& namesr);
    خالی emitDpiImp();

    متن_Stmts getSymCtorStmts();
    متن_Stmts getSymDtorStmts();

    ساکن خالی nameCheck(Ast_گرہ* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        اگر (nodep->name().خالی_خانے()) واپس;
        ثابت AstCFunc* ثابت cfuncp = VN_CAST(nodep, CFunc);
        اگر (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ثابت std::string rsvd = V3_Language_الفاظ::isKeyword(nodep->name());
            اگر (rsvd != "") {
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

    ساکن std::string scopeSymString(ثابت std::string& scpname) {
        std::string برآمد = scpname;
        std::string::سائز_type pos;
        جب ((pos = برآمد.ڈھونڈو("__PVT__")) != std::string::npos) برآمد.replace(pos, 7, "");
        اگر (VString::startsWith(برآمد, "TOP__DOT__")) برآمد.replace(0, 10, "");
        اگر (VString::startsWith(برآمد, "TOP.")) برآمد.replace(0, 4, "");
        جب ((pos = برآمد.ڈھونڈو('.')) != std::string::npos) برآمد.replace(pos, 1, "__");
        جب ((pos = برآمد.ڈھونڈو("__DOT__")) != std::string::npos) برآمد.replace(pos, 7, "__");
        واپس برآمد;
    }

    ساکن string scopeDecodeIdentifier(ثابت std::string& scpname) {
        std::string::سائز_type pos = std::string::npos;

        // Remove hierarchy
        سائز_t i = 0;
        // always makes progress
        جب (i < scpname.لمبائی_2()) {
            اگر (scpname[i] == '\\') {
                جب (i < scpname.لمبائی_2() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } ورنہ {
                جب (i < scpname.لمبائی_2() && scpname[i] != '.') ++i;
                اگر (i < scpname.لمبائی_2()) pos = i++;
            }
        }

        واپس pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    ساخت VarDims final {
        صحیح pdim = 0;  // Packed array dims plus a ranged basic leaf
        صحیح udim = 0;
        std::سمتیہ<std::pair<صحیح, صحیح>> unpackedLR;  // (left,right), outer-first
        std::سمتیہ<std::pair<صحیح, صحیح>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::سمتیہ<std::pair<صحیح, صحیح>> flatten() ثابت {
            std::سمتیہ<std::pair<صحیح, صحیح>> flat(unpackedLR);
            flat.داخل_کرو(flat.انتہا(), packedLR.cbegin(), packedLR.cend());
            واپس flat;
        }
    };
    ساکن VarDims getVarDims(ثابت Ast_گرہ_DType* ثابت جڑ_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        برائے (ثابت Ast_گرہ_DType* dtypep = جڑ_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            اگر (ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرہ_Array_DType)) {
                اگر (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_عقب(adtypep->بائیں(), adtypep->دائیں());
                    ++d.pdim;
                } ورنہ {
                    d.unpackedLR.emplace_عقب(adtypep->بائیں(), adtypep->دائیں());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } ورنہ {
                اگر (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    اگر (basicp->isRanged()) {
                        d.packedLR.emplace_عقب(basicp->بائیں(), basicp->دائیں());
                        ++d.pdim;
                    }
                }
                توڑو;  // Non-array leaf
            }
        }
        واپس d;
    }

    ساکن VarDims dimsFor(ثابت Scope_Var_ڈیٹا& svd) { واپس getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ساکن std::string boundsString(ثابت VarDims& d) {
        std::string bounds;
        برائے (ثابت std::pair<صحیح, صحیح>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::مقصد_string(lr.پہلا);
            bounds += ",";
            bounds += std::مقصد_string(lr.دوسرا);
        }
        واپس bounds;
    }

    ساکن صحیح getUnpackedElements(ثابت Ast_گرہ_DType* جڑ_Dtypep) {
        صحیح elements = 1;
        برائے (ثابت Ast_گرہ_DType* dtypep = جڑ_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType);
            اگر (!adtypep) توڑو;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        واپس elements;
    }

    ساکن منطقی needs_Emitted_Ent_سائز(ثابت std::string& vlEnumType) {
        واپس vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ساکن std::pair<منطقی, std::string> isForceControlSignal(ثابت AstVar* ثابت signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        برائے (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ثابت std::سائز_t suffixPos = signalVarp->name().ڈھونڈو(forceControlSuffix);
            ثابت منطقی suffix_ملا = suffixPos != std::string::npos;
            ثابت منطقی suffix_Is_At_انتہا
                = suffixPos + forceControlSuffix.لمبائی_2() == signalVarp->name().لمبائی_2();
            اگر (!(suffix_ملا && suffix_Is_At_انتہا)) جاری;
            ثابت std::string بنیاد_Signal_Name = signalVarp->name().substr(0, suffixPos);
            واپس std::pair<منطقی, std::string>{سچ, بنیاد_Signal_Name};
        }
        واپس std::pair<منطقی, std::string>{جھوٹ, ""};
    }

    ساکن خالی appendVarProperties(std::string& stmt, ثابت std::string& vlEnumType,
                                    ثابت std::string& vlEnumDir, ثابت صحیح udim, ثابت صحیح pdim,
                                    ثابت std::string& bounds, ثابت std::string& ent_سائز = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::مقصد_string(udim);
        اگر (!ent_سائز.خالی_خانے()) {
            stmt += ", ";
            stmt += ent_سائز;
        } ورنہ {
            stmt += ", ";
            stmt += std::مقصد_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ساکن std::string memberVlEnumDir(ثابت AstVar* ثابت varp,
                                       ثابت Ast_گرہ_DType* ثابت dtypep) {
        std::string برآمد = '(' + varp->vlEnumDir(/*forMember=*/سچ);
        ثابت Ast_گرہ_DType* ثابت skipDTypep = dtypep->skipRefp();
        اگر (skipDTypep->isSigned()) برآمد += "|VLVF_SIGNED";
        اگر (ثابت AstBasicDType* ثابت basicp = skipDTypep->basicp()) {
            اگر (basicp->keyword() == VBasicDTypeKwd::ثنائی_ہندسہ) برآمد += "|VLVF_BITVAR";
        }
        برآمد += skipDTypep->vlEnumPackedAgg();
        واپس برآمد + ')';
    }

    // Name of the instance member for the given scope
    ساکن std::string instName(ثابت AstScope* scopep) {
        واپس "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ساکن std::string داخل_کرو_Var_Statement(ثابت Scope_Var_ڈیٹا& svd, ثابت AstScope* ثابت scopep,
                                          ثابت AstVar* ثابت varp, ثابت صحیح udim, ثابت صحیح pdim,
                                          ثابت std::string& bounds) {
        ثابت std::string var_اظہاریہ = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ثابت std::string vlEnumType = varp->vlEnumType();
        ثابت منطقی needs_Ent_سائز = needs_Emitted_Ent_سائز(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_سائز ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_برآمد_Formatter::quoteNameControls(protect(svd.m_var_بنیاد_Pretty)) + '"';

        اگر (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_اظہاریہ;
            stmt += "), false, ";
        } ورنہ اگر (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_اظہاریہ;
            stmt += ".c_str())), true, ";
        } ورنہ {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_اظہاریہ;
            stmt += "))), true, ";
        }

        ثابت std::string ent_سائز = needs_Ent_سائز
                                        ? "sizeof(" + var_اظہاریہ + ") / "
                                              + std::مقصد_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_سائز);
        واپس stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    شمار جماعت جدول_اندراج_قسم : uint8_t {
        جدول_صف,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    جدول_اندراج_قسم try_Build_جدول_اندراج(ثابت Scope_Var_ڈیٹا& svd, ثابت AstVar* ثابت varp,
                                      ثابت AstScope* ثابت scopep,
                                      ثابت std::string& modClassName, ثابت VarDims& dims,
                                      std::string& صف_برآمد) ثابت {
        ثابت صحیح pdim = dims.pdim;
        ثابت صحیح udim = dims.udim;
        اگر (varp->isForceable() && force_Control_اشارات_Are_Valid(scopep, varp)) {
            واپس جدول_اندراج_قسم::FORCEABLE_RESIDUAL;
        }
        اگر (udim + pdim > VPI_جدول_MAX_DIMS) واپس جدول_اندراج_قسم::PLAIN_RESIDUAL;

        ثابت std::string vlEnumType = varp->vlEnumType();
        اگر (needs_Emitted_Ent_سائز(vlEnumType))
            واپس جدول_اندراج_قسم::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        اگر (varp->isParam()) واپس جدول_اندراج_قسم::PLAIN_RESIDUAL;

        ثابت std::string name = V3_برآمد_Formatter::quoteNameControls(protect(svd.m_var_بنیاد_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ثابت std::string member = varName(varp);
        ثابت std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::سمتیہ<صحیح> dv;
        برائے (ثابت std::pair<صحیح, صحیح>& lr : dims.flatten()) {
            dv.ڈالو_عقب(lr.پہلا);
            dv.ڈالو_عقب(lr.دوسرا);
        }

        std::string صف = "{\"" + name + "\", ";
        صف += "offsetof(" + modClassName + ", " + member + "), ";
        صف += vlEnumType + ", ";
        صف += "(" + dir + "), ";
        صف += std::مقصد_string(udim) + ", " + std::مقصد_string(pdim) + ", {";
        برائے (صحیح i = 0; i < VPI_جدول_MAX_DIMS * 2; ++i) {
            اگر (i) صف += ", ";
            صف += std::مقصد_string(i < static_cast<صحیح>(dv.سائز()) ? dv[i] : 0);
        }
        صف += "}}";
        صف_برآمد = صف;
        واپس جدول_اندراج_قسم::جدول_صف;
    }

    ساکن std::string داخل_کرو_DType_Var_Statement(ثابت Scope_Var_ڈیٹا& svd,
                                               ثابت AstScope* ثابت scopep,
                                               ثابت std::string& prettyName,
                                               ثابت std::string& cName,
                                               ثابت Ast_گرہ_DType* ثابت dtypep, ثابت صحیح udim,
                                               ثابت صحیح pdim, ثابت std::string& bounds) {
        ثابت std::string vlEnumType = dtypep->vlEnumType();
        ثابت منطقی needs_Ent_سائز = needs_Emitted_Ent_سائز(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_سائز ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_برآمد_Formatter::quoteNameControls(prettyName) + '"';
        ثابت std::string var_اظہاریہ = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        اگر (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_اظہاریہ;
            stmt += "))), true, ";
        } ورنہ {
            stmt += ", &(";
            stmt += var_اظہاریہ;
            stmt += "), false, ";
        }
        ثابت std::string ent_سائز
            = needs_Ent_سائز
                  ? "sizeof(" + var_اظہاریہ + ") / " + std::مقصد_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_سائز);
        واپس stmt;
    }

    ساکن خالی جوڑو_UOr_Struct_Member_Vars(std::سمتیہ<std::string>& stmts, ثابت Scope_Var_ڈیٹا& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_سابقہ, ثابت std::string& c_سابقہ,
                                       ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep) {
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_گرہ_DType* ثابت شے_DTypep = itemp->dtypep();
            اگر (شے_DTypep->vlEnumType().خالی_خانے()) جاری;
            ثابت std::string prettyName
                = pretty_سابقہ + "." + Ast_گرہ::vpiName(itemp->shortName());
            ثابت std::string cName = c_سابقہ + "." + itemp->nameProtect();
            ثابت VarDims dims = getVarDims(شے_DTypep);
            stmts.emplace_عقب(داخل_کرو_DType_Var_Statement(svd, scopep, prettyName, cName, شے_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            جوڑو_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, شے_DTypep);
            اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت subp
                = VN_CAST(شے_DTypep->skipRefp(), گرہ_UOr_Struct_DType)) {
                اگر (!subp->packed())
                    جوڑو_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } ورنہ اگر (VN_IS(شے_DTypep->skipRefp(), UnpackArrayDType)) {
                جوڑو_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    شے_DTypep);
            }
        }
    }

    ساکن خالی جوڑو_Unpacked_Array_UOr_Struct_Member_Vars(std::سمتیہ<std::string>& stmts,
                                                    ثابت Scope_Var_ڈیٹا& svd,
                                                    ثابت AstScope* ثابت scopep,
                                                    ثابت std::string& pretty_سابقہ,
                                                    ثابت std::string& c_سابقہ,
                                                    ثابت Ast_گرہ_DType* ثابت dtypep) {
        ثابت Ast_گرہ_DType* ثابت skipDTypep = dtypep->skipRefp();
        اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            جوڑو_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_سابقہ, c_سابقہ + "[0]",
                                                adtypep->subDTypep());
        } ورنہ اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(skipDTypep, گرہ_UOr_Struct_DType)) {
            اگر (!sdtypep->packed())
                جوڑو_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_سابقہ, c_سابقہ, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ساکن خالی جوڑو_Packed_Member_Vars(std::سمتیہ<std::string>& stmts, ثابت Scope_Var_ڈیٹا& svd,
                                    ثابت AstScope* ثابت scopep, ثابت std::string& pretty_سابقہ,
                                    ثابت std::string& cName, ثابت std::string& vlEnumType,
                                    ثابت std::string& ent_سائز,
                                    ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep,
                                    ثابت صحیح والد_ثنائی_ہندسہ_انحراف, ثابت منطقی needs_اشاریہ) {
        ثابت std::string var_اظہاریہ = "vlSymsp->" + instName(scopep) + "." + cName;
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_گرہ_DType* ثابت شے_DTypep = itemp->dtypep();
            ثابت std::string prettyName
                = pretty_سابقہ + "." + Ast_گرہ::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ثابت صحیح ثنائی_ہندسہ_انحراف = والد_ثنائی_ہندسہ_انحراف + itemp->lsb();
            ثابت VarDims dims = getVarDims(شے_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_برآمد_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            اگر (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_اظہاریہ + "))), true, ";
            } ورنہ {
                stmt += ", &(" + var_اظہاریہ + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, شے_DTypep);
            اگر (needs_اشاریہ) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::مقصد_string(ثنائی_ہندسہ_انحراف) + ", ";
            stmt += ent_سائز + ", ";
            stmt += std::مقصد_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_عقب(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ثابت Ast_گرہ_DType* subDTypep = شے_DTypep->skipRefp();
            منطقی isArray = جھوٹ;
            جب (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = سچ;
            }
            اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت subp = VN_CAST(subDTypep, گرہ_UOr_Struct_DType)) {
                جوڑو_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_سائز,
                                    subp, ثنائی_ہندسہ_انحراف, needs_اشاریہ || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ساکن خالی جوڑو_Packed_Agg_Member_Vars(std::سمتیہ<std::string>& stmts, ثابت Scope_Var_ڈیٹا& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_سابقہ, ثابت std::string& cName,
                                       ثابت Ast_گرہ_DType* ثابت dtypep) {
        std::string elemCName = cName;
        ثابت Ast_گرہ_DType* elemDTypep = dtypep->skipRefp();
        منطقی isArray = جھوٹ;
        جب (ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(elemDTypep, گرہ_Array_DType)) {
            اگر (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = سچ;
        }
        ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(elemDTypep, گرہ_UOr_Struct_DType);
        اگر (!sdtypep || !sdtypep->packed()) واپس;
        ثابت std::string var_اظہاریہ = "vlSymsp->" + instName(scopep) + "." + cName;
        ثابت std::string ent_سائز
            = "sizeof(" + var_اظہاریہ + ") / " + std::مقصد_string(getUnpackedElements(dtypep));
        جوڑو_Packed_Member_Vars(stmts, svd, scopep, pretty_سابقہ, elemCName, dtypep->vlEnumType(),
                            ent_سائز, sdtypep, 0, isArray);
    }

    std::string داخل_کرو_Forceable_Var_Statement(ثابت Scope_Var_ڈیٹا& svd, ثابت AstScope* ثابت scopep,
                                            ثابت AstVar* ثابت varp, ثابت صحیح udim,
                                            ثابت صحیح pdim, ثابت std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_برآمد_Formatter::quoteNameControls(protect(svd.m_var_بنیاد_Pretty)) + '"';

        ثابت std::string var_اظہاریہ = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        أكد(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_اظہاریہ;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_اظہاریہ + "__VforceRd";
        stmt += "), \"" + V3_برآمد_Formatter::quoteNameControls(protect(svd.m_var_بنیاد_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        برائے (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ثابت std::string enable_Signal_کلید
                = get_کلید_Name(scopep, varp->name() + forceControlSuffix);
            ثابت std::map<ثابت std::string, Scope_Var_ڈیٹا>::const_iterator itpair
                = m_scopeVars.ڈھونڈو(enable_Signal_کلید);

            اگر (itpair == m_scopeVars.انتہا()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_کلید << "'.");
            }

            ثابت Scope_Var_ڈیٹا& svd = itpair->دوسرا;
            ثابت AstScope* ثابت scopep = svd.m_scopep;
            ثابت AstVar* ثابت varp = svd.m_varp;
            ثابت VarDims dims = dimsFor(svd);
            stmt
                += داخل_کرو_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            اگر (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::مقصد_string(udim);
        stmt += ", ";
        stmt += std::مقصد_string(pdim);
        stmt += bounds;
        stmt += ")";
        واپس stmt;
    }

    ساکن std::string get_کلید_Name(ثابت AstScope* ثابت scopep, ثابت std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //ترتیب
        // signal can be found.
        std::string پورا = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        اگر (VString::startsWith(پورا, "__DOT__TOP")) پورا.replace(0, 10, "");
        ثابت std::string::سائز_type dpos = پورا.rfind("__DOT__");
        اگر (dpos != std::string::npos) { scpName = پورا.substr(0, dpos); }
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        واپس scpSym + " " + signal_name;
    }

    منطقی بنیاد_Signal_Is_Valid(ثابت AstScope* ثابت scopep, ثابت AstVar* ثابت controlSignalVarp,
                           ثابت std::string& بنیاد_Signal_Name) ثابت {
        ثابت std::string بنیاد_Signal_کلید = get_کلید_Name(scopep, بنیاد_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_ڈیٹا>::const_iterator بنیاد_Signal_It
            = m_scopeVars.ڈھونڈو(بنیاد_Signal_کلید);
        اگر (بنیاد_Signal_It == m_scopeVars.انتہا()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            واپس جھوٹ;
        } ورنہ {
            ثابت AstVar* ثابت بنیاد_Signal_Varp = بنیاد_Signal_It->دوسرا.m_varp;
            اگر (m_scopeVars.گنتی(بنیاد_Signal_Name) > 1) {
                بنیاد_Signal_Varp->v3fatalSrc(
                    "Found Signal " << بنیاد_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << بنیاد_Signal_Name << " occurring in the same scope.");
                واپس جھوٹ;
            }
            اگر (!بنیاد_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << بنیاد_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                واپس جھوٹ;
            }
        }
        واپس سچ;
    }

    منطقی بنیاد_Signal_Is_عوامی(ثابت AstScope* ثابت scopep,
                            ثابت std::string& بنیاد_Signal_Name) ثابت {
        ثابت std::string بنیاد_Signal_کلید = get_کلید_Name(scopep, بنیاد_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_ڈیٹا>::const_iterator بنیاد_Signal_It
            = m_scopeVars.ڈھونڈو(بنیاد_Signal_کلید);
        اگر (بنیاد_Signal_It == m_scopeVars.انتہا()) {
            واپس جھوٹ;
        } ورنہ {
            ثابت AstVar* ثابت بنیاد_Signal_Varp = بنیاد_Signal_It->دوسرا.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            اگر (!بنیاد_Signal_Varp->is_Sig_عوامی()) واپس جھوٹ;
        }
        واپس سچ;
    }

    منطقی force_Control_اشارات_Are_Valid(ثابت AstScope* ثابت scopep,
                                     ثابت AstVar* ثابت بنیاد_Signal_Varp) ثابت {
        constexpr std::array<ثابت حرف*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        واپس std::all_کا(
            forceControlSuffixes.begin(), forceControlSuffixes.انتہا(),
            [scopep, بنیاد_Signal_Varp, یہ](ثابت std::string& forceControlSuffix) {
                ثابت std::string controlSignalName = بنیاد_Signal_Varp->name() + forceControlSuffix;
                ثابت std::string control_Signal_کلید = get_کلید_Name(scopep, controlSignalName);
                ثابت std::سائز_t control_Signal_گنتی = m_scopeVars.گنتی(control_Signal_کلید);
                اگر (control_Signal_گنتی == 0) {
                    بنیاد_Signal_Varp->v3fatalSrc("Signal "
                                               << بنیاد_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_کلید << "'.");
                    واپس جھوٹ;
                }
                اگر (control_Signal_گنتی > 1) {
                    بنیاد_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << بنیاد_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    واپس جھوٹ;
                }
                واپس سچ;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    خالی varHierarchyScopes(std::string scp) {

        std::string::سائز_type prd_pos = scp.rfind('.');
        std::string::سائز_type dot_pos = scp.rfind("__DOT__");

        جب (!scp.خالی_خانے()) {
            ثابت خودکار scpit = m_vpiScopeCandidates.ڈھونڈو(scopeSymString(scp));
            اگر ((scpit != m_vpiScopeCandidates.انتہا())
                && (m_scope_نام.ڈھونڈو(scp) == m_scope_نام.انتہا())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ثابت خودکار pair = m_scope_نام.emplace(scpit->دوسرا.m_symName, scpit->دوسرا);
                اگر (!pair.دوسرا) pair.پہلا->دوسرا.m_type = scpit->دوسرا.m_type;
            }

            // resize and advance pointers
            اگر ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } ورنہ {
                اگر (prd_pos == string::npos) توڑو;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    خالی جوڑو_Scope_Var_اندراج(ثابت AstScope* ثابت scopep, ثابت Ast_گرہ_Module* ثابت modp,
                          ثابت AstVar* ثابت varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string پورا = scopep->name() + "__DOT__" + varp->name();
        اگر (VString::startsWith(پورا, "__DOT__TOP")) پورا.replace(0, 10, "");
        ثابت std::string::سائز_type dpos = پورا.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ثابت std::string scpName = پورا.substr(0, dpos);
        ثابت std::string var_بنیاد = پورا.substr(dpos + std::لڑی_لمبائی("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ثابت std::string var_بنیاد_Pretty = Ast_گرہ::vpiName(VName::dehash(var_بنیاد));
        ثابت std::string scpPretty = Ast_گرہ::prettyName(VName::dehash(scpName));
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        اگر (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_نام.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_بنیاد_Pretty, varp, modp, scopep));
    }

    خالی varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        برائے (ثابت ScopeModPair& smPair : m_scopes) {
            ثابت AstScope* ثابت scopep = smPair.پہلا;
            ثابت Ast_گرہ_Module* ثابت smodp = smPair.دوسرا;
            برائے (ثابت ModVarPair& mvPair : m_modVars) {
                ثابت Ast_گرہ_Module* ثابت modp = mvPair.پہلا;
                ثابت AstVar* ثابت varp = mvPair.دوسرا;
                اگر (modp != smodp) جاری;
                جوڑو_Scope_Var_اندراج(scopep, modp, varp);
            }
        }
    }

    خالی collectIfaceRefs(ثابت AstScope* nodep) {
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        اگر (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) واپس;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ثابت std::string راستہ = Ast_گرہ::vpiName(nodep->name());
        ثابت std::string instName = Ast_گرہ::vpiName(cellp->origName());
        UASSERT_OBJ(راستہ.لمبائی_2() > instName.لمبائی_2() && VString::سرے_With(راستہ, instName), nodep,
                    "Interface scope name " << راستہ << " does not end with instance name "
                                            << instName);
        ثابت std::string والد_راستہ = راستہ.substr(0, راستہ.لمبائی_2() - instName.لمبائی_2());

        برائے (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ثابت std::string refName = Ast_گرہ::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            اگر (!VString::startsWith(refName, والد_راستہ)) جاری;
            m_ifaceRefs.emplace_عقب(nodep, scopeSymString(intfRefp->والد_Name()), refName,
                                     Ast_گرہ::vpiName(intfRefp->بنیاد_Name()),
                                     intfRefp->modportName());
        }
    }

    خالی build_Iface_Ref_جدول() {
        اگر (m_ifaceRefs.خالی_خانے()) واپس;
        ثابت std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_ترتیب_دو(
            m_ifaceRefs.begin(), m_ifaceRefs.انتہا(),
            [](ثابت Iface_Ref_ڈیٹا& a, ثابت Iface_Ref_ڈیٹا& b) { واپس a.m_suffix < b.m_suffix; });
        برائے (ثابت Iface_Ref_ڈیٹا& ird : m_ifaceRefs) {
            ثابت std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            اگر (m_scope_نام.ڈھونڈو(scopeSym) == m_scope_نام.انتہا()) جاری;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string والد_انحراف = "VL_IFACEREF_NO_PARENT";
            ثابت خودکار والد = m_scope_نام.ڈھونڈو(ird.m_والد_Sym);
            اگر (والد != m_scope_نام.انتہا()) {
                والد_انحراف = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + والد->دوسرا.m_symName) + ")";
            }
            std::string صف
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            صف += والد_انحراف + ", \"";
            صف += V3_برآمد_Formatter::quoteNameControls(VIdProtect::protect_الفاظ_If(ird.m_name, سچ));
            صف += "\", \"";
            صف += V3_برآمد_Formatter::quoteNameControls(
                VIdProtect::protect_الفاظ_If(ird.m_suffix, سچ));
            صف += "\", \"";
            صف += V3_برآمد_Formatter::quoteNameControls(
                VIdProtect::protect_الفاظ_If(ird.m_modportName, سچ));
            صف += "\"}";
            m_iface_Ref_جدول_صفیں.emplace_عقب(std::move(صف));
        }
        اگر (!m_iface_Ref_جدول_صفیں.خالی_خانے()) m_iface_Ref_جدول_Name = symClass + "__VpiIfaceRefTable";
    }

    خالی buildVpiHierarchy() {
        برائے (ثابت خودکار& itpair : m_scope_نام) {
            ثابت std::string symName = itpair.دوسرا.m_symName;
            std::string above = symName;
            اگر (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            جب (!above.خالی_خانے()) {
                ثابت std::string::سائز_type pos = above.rfind("__");
                اگر (pos == std::string::npos) توڑو;
                above.resize(pos);
                اگر (m_vpiScopeHierarchy.ڈھونڈو(above) != m_vpiScopeHierarchy.انتہا()) {
                    m_vpiScopeHierarchy[above].ڈالو_عقب(symName);
                    توڑو;
                }
            }
            m_vpiScopeHierarchy[symName] = std::سمتیہ<string>();
        }
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        اگر (v3Global.opt.vpi()) buildVpiHierarchy();

        اگر (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            برائے (ثابت خودکار& scp : m_dpi_Scope_نام) m_scope_نام.emplace(scp.پہلا, scp.دوسرا);
        }

        // Sort by names, so line/process order matters less
        std::stable_ترتیب_دو(m_scopes.begin(), m_scopes.انتہا(),
                         [](ثابت ScopeModPair& a, ثابت ScopeModPair& b) {
                             واپس a.پہلا->name() < b.پہلا->name();
                         });
        std::stable_ترتیب_دو(m_dpis.begin(), m_dpis.انتہا(),  //
                         [](ثابت AstCFunc* ap, ثابت AstCFunc* bp) {
                             اگر (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 واپس bp->dpiImportPrototype();
                             }
                             واپس ap->name() < bp->name();
                         });

        // Output
        اگر (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        اگر (v3Global.dpi()) {
            emitDpiHdr();
            اگر (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->isConstPool()) واپس;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCellInlineScope* nodep) override {
        ثابت std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ثابت std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ثابت صحیح timeunit = m_modp->timeunit().power_کا_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_گرہ::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    خالی دورہ(AstScope* nodep) override {
        اگر (VN_IS(m_modp, Class)) واپس;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_عقب(nodep, m_modp);

        اگر (v3Global.opt.vpi() && !nodep->is_چوٹی()) {
            ثابت std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ثابت صحیح timeunit = m_modp->timeunit().power_کا_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_گرہ::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstScopeName* nodep) override {
        اگر (nodep->selfRelative()) واپس;  // Its absolute copy is under the scope
        ثابت std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ثابت صحیح timeunit = m_modp ? m_modp->timeunit().power_کا_Ten() : 0;
        m_dpi_Scope_نام.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        اگر (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } ورنہ {
            // Note emplace does not construct when duplicate key
            m_dpi_Scope_نام.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    خالی دورہ(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        اگر ((nodep->is_Sig_User_Rd_عوامی() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().خالی_خانے()) {
            m_modVars.emplace_عقب(m_modp, nodep);
        }
    }
    خالی دورہ(Ast_گرہ_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        اگر (!nodep->ڈیٹا_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_عدد(m_coverBins);
            m_coverBins += nodep->سائز();
            nodep->local_Bin_عدد(m_modCoverBins);
            m_modCoverBins += nodep->سائز();
        }
    }
    خالی دورہ(AstCFunc* nodep) override {
        nameCheck(nodep);
        اگر (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ڈالو_عقب(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    خالی دورہ(AstConst*) override {}
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    صریح EmitCSyms(AstNetlist* nodep, منطقی dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

خالی EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_سرخی_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_چوٹی_Include();
    puts("#include \"verilated.h\"\n");
    اگر (v3Global.needTraceDumper()) {
        برائے (ثابت string& بنیاد : v3Global.opt.trace_ماخذ_Langs())
            puts("#include \"" + بنیاد + ".h\"\n");
    }
    اگر (v3Global.opt.استعمال_کرتا_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + چوٹی_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    برائے (Ast_گرہ_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), گرہ_Module);
        اگر (VN_IS(nodep, Class)) جاری;  // Class included earlier
        اگر (nodep->isConstPool()) جاری;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::سابقہ_Name_Protect(nodep) + ".h\"\n");
    }

    اگر (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::سیٹ<std::string> types;  // Remove duplicates and sort
        برائے (ثابت خودکار& itpair : m_scopeFuncs) {
            ثابت AstCFunc* ثابت funcp = itpair.دوسرا.m_cfuncp;
            اگر (!funcp->dpiExportImpl()) جاری;
            ثابت std::string cbtype
                = protect(v3Global.opt.سابقہ() + "__Vcb_" + funcp->cname() + "_t");
            ثابت std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_دلائل(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        برائے (ثابت std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_نجی(جھوٹ);  // public:

    puts("// INTERNAL STATE\n");
    puts(چوٹی_Class_Name() + "* const __Vm_modelp;\n");

    اگر (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    اگر (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    اگر (v3Global.hasEvents()) {
        اگر (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } ورنہ {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    اگر (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    اگر (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    اگر (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    اگر (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    اگر (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::مقصد_string(ExecMTask::عدد_مستعمل_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    برائے (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.پہلا;
        ثابت Ast_گرہ_Module* ثابت modp = itpair.دوسرا;
        اگر (VN_IS(modp, Class)) جاری;
        ofp()->لکھو("%-30s ", EmitCUtil::سابقہ_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    اگر (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.دھاگے() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::مقصد_string(m_coverBins));
        puts("];\n");
    }

    اگر (!m_scope_نام.خالی_خانے()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        برائے (ثابت خودکار& itpair : m_scope_نام) {
            ثابت Scope_ڈیٹا& sd = itpair.دوسرا;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    اگر (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + چوٹی_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    اگر (v3Global.hasEvents()) {
        اگر (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } ورنہ {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        اگر (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } ورنہ {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        اگر (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } ورنہ {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    اگر (v3Global.needTraceDumper()) {
        اگر (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    اگر (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->puts_انتہا_Guard();
    closeOutputFile();
}

خالی EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    منطقی needs_New_سطر = جھوٹ;
    برائے (ثابت خودکار& itpair : m_scopeFuncs) {
        ثابت AstCFunc* ثابت funcp = itpair.دوسرا.m_cfuncp;
        اگر (!funcp->dpiExportImpl()) جاری;
        emitCFuncDecl(funcp, itpair.دوسرا.m_modp);
        needs_New_سطر = سچ;
    }
    اگر (needs_New_سطر) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::سیٹ<std::string> lib_نام;
    برائے (ثابت ScopeModPair& itpair : m_scopes) {
        اگر (itpair.دوسرا->verilatorLib()) lib_نام.emplace(itpair.دوسرا->origName());
    }
    برائے (ثابت std::string& libName : lib_نام) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    اگر (!lib_نام.خالی_خانے()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    اگر (!m_varTables.خالی_خانے() || !m_scope_جدول_صفیں.خالی_خانے() || !m_iface_Ref_جدول_صفیں.خالی_خانے()) {
        برائے (ثابت خودکار& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.پہلا + "[];\n");
        }
        اگر (!m_scope_جدول_صفیں.خالی_خانے()) {
            puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[];\n");
        }
        اگر (!m_iface_Ref_جدول_صفیں.خالی_خانے()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[];\n");
        }
        puts("\n");
    }
}

خالی EmitCSyms::emitVarTables() {
    اگر (m_varTables.خالی_خانے() && m_scope_جدول_صفیں.خالی_خانے() && m_iface_Ref_جدول_صفیں.خالی_خانے()) واپس;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    برائے (ثابت خودکار& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.پہلا + "[] = {\n");
        برائے (ثابت std::string& صف : kv.دوسرا) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(صف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    اگر (!m_scope_جدول_صفیں.خالی_خانے()) {
        puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[] = {\n");
        برائے (ثابت std::string& صف : m_scope_جدول_صفیں) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(صف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    اگر (!m_iface_Ref_جدول_صفیں.خالی_خانے()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[] = {\n");
        برائے (ثابت std::string& صف : m_iface_Ref_جدول_صفیں) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(صف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic pop\n");
    puts("#endif\n");
}

خالی EmitCSyms::emitScopeHier(متن_Stmts& stmts, منطقی destroy) {
    اگر (!v3Global.opt.vpi()) واپس;

    اگر (destroy) {
        stmts.جوڑو("// Tear down scope hierarchy");
    } ورنہ {
        stmts.جوڑو("// Set up scope hierarchy");
    }

    ثابت std::string طریقہ = destroy ? "remove" : "add";
    برائے (ثابت خودکار& itpair : m_scope_نام) {
        اگر (itpair.پہلا == "TOP") جاری;
        ثابت Scope_ڈیٹا& sd = itpair.دوسرا;
        ثابت std::string& name = sd.m_prettyName;
        ثابت std::string& scopeType = sd.m_type;
        اگر (name.ڈھونڈو('.') != string::npos) جاری;
        اگر (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") جاری;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جوڑو("vlSymsp->__Vhier." + طریقہ + "(0, " + id + ");");
    }
    برائے (ثابت خودکار& itpair : m_vpiScopeHierarchy) {
        ثابت std::string fromName = scopeSymString(itpair.پہلا);
        ثابت std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_نام.at(fromName).m_symName);
        برائے (ثابت std::string& name : itpair.دوسرا) {
            ثابت std::string مقصد_Name = scopeSymString(name);
            ثابت std::string مقصد_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_نام.at(مقصد_Name).m_symName);
            stmts.جوڑو("vlSymsp->__Vhier." + طریقہ + "(" + fromId + ", " + مقصد_Id + ");");
        }
    }

    اگر (destroy) {
        stmts.جوڑو("// Clear keys from hierarchy map after values have been removed");
        stmts.جوڑو("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::متن_Stmts EmitCSyms::getSymCtorStmts() {
    متن_Stmts stmts;

    {
        uint64_t ڈھیر_سائز = V3_ڈھیر_گنتی::گنتی(v3Global.rootp());
        اگر (v3Global.opt.debug_ڈھیر_Check()) ڈھیر_سائز += 1024 * 1024 * 1024;
        V3Stats::جوڑو_Stat("Size prediction, Stack (bytes)", ڈھیر_سائز);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::جوڑو_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_بائٹیں);
        V3Stats::جوڑو_Stat(V3Stats::STAT_MODEL_سائز, ڈھیر_سائز + m_stat_Var_Scope_بائٹیں);

        stmts.جوڑو("// Check resources");
        stmts.جوڑو("Verilated::stackCheck(" + std::مقصد_string(ڈھیر_سائز) + ");");
    }

    stmts.جوڑو("// Setup sub module instances");
    برائے (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.پہلا;
        ثابت Ast_گرہ_Module* ثابت modp = itpair.دوسرا;
        اگر (modp->is_چوٹی()) جاری;
        ثابت std::string name = V3_برآمد_Formatter::quoteNameControls(
            VIdProtect::protect_الفاظ_If(scopep->prettyName(), scopep->protect()));
        stmts.جوڑو("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    اگر (v3Global.opt.profPgo()) {
        stmts.جوڑو("// Configure profiling for PGO\n");
        اگر (!v3Global.opt.hier_بچہ()) {
            stmts.جوڑو("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        اگر (v3Global.opt.mtasks()) {
            v3Global.rootp()->چوٹی_Modulep()->foreach([&](ثابت Ast_Exec_گراف* execGraphp) {
                برائے (ثابت V3_گراف_راس& vtx : execGraphp->depGraphp()->vertices()) {
                    ثابت ExecMTask& mt = static_cast<ثابت ExecMTask&>(vtx);
                    stmts.جوڑو("vlSymsp->_vm_pgoProfiler.addCounter(" + std::مقصد_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.جوڑو("// Configure time unit / time precision");
    اگر (!v3Global.rootp()->timeunit().isNone()) {
        ثابت std::string unit = std::مقصد_string(v3Global.rootp()->timeunit().power_کا_Ten());
        stmts.جوڑو("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    اگر (!v3Global.rootp()->timeprecision().isNone()) {
        ثابت std::string prec = std::مقصد_string(v3Global.rootp()->timeprecision().power_کا_Ten());
        stmts.جوڑو("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.جوڑو("// Setup each module's pointers to their submodules");
    برائے (ثابت خودکار& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.پہلا;
        ثابت Ast_گرہ_Module* ثابت modp = i.دوسرا;
        ثابت AstScope* ثابت abovep = scopep->aboveScopep();
        اگر (!abovep) جاری;

        ثابت std::string protName = VIdProtect::protect_الفاظ_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        اگر (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } ورنہ {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.جوڑو(stmt);
    }

    اگر (v3Global.opt.coverage()) {
        stmts.جوڑو("// Setup each module's pointer back to symbol table (for public functions)");
        برائے (ثابت ScopeModPair& i : m_scopes) {
            ثابت AstScope* ثابت scopep = i.پہلا;
            Ast_گرہ_Module* ثابت modp = i.دوسرا;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ثابت منطقی پہلا = !modp->user1();
            modp->user1(سچ);
            stmts.جوڑو("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (پہلا ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    برائے (ثابت ScopeModPair& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.پہلا;
        ثابت Ast_گرہ_Module* ثابت modp = i.دوسرا;
        اگر (!modp->verilatorLib()) جاری;
        ثابت AstVar* hashp = خالی_اشاریہ;
        ثابت AstVar* handlep = خالی_اشاریہ;
        برائے (ثابت Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (!varp) جاری;
            اگر (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            اگر (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ثابت uint32_t hash = VN_AS(hashp->valuep(), Const)->مقصد_UInt();
        stmts.جوڑو(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::مقصد_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string راستہ = scopep->prettyName();
        اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            راستہ.erase(0, v3Global.rootp()->trace_Lib_چوٹی_Name().سائز() + 1);
        }
        stmts.جوڑو("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_برآمد_Formatter::quoteNameControls(راستہ) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    اگر (!m_scope_نام.خالی_خانے()) {
        stmts.جوڑو("// Setup scopes");
        ثابت std::string symClass = symClassName();
        برائے (ثابت خودکار& itpair : m_scope_نام) {
            ثابت Scope_ڈیٹا& sd = itpair.دوسرا;
            std::string صف
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            صف += V3_برآمد_Formatter::quoteNameControls(
                VIdProtect::protect_الفاظ_If(sd.m_prettyName, سچ));
            صف += "\", \"";
            صف += V3_برآمد_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            صف += "\", \"";
            صف += V3_برآمد_Formatter::quoteNameControls(sd.m_defName);
            صف += "\", ";
            صف += std::مقصد_string(sd.m_timeunit);
            صف += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_جدول_صفیں.emplace_عقب(std::move(صف));
        }
        m_scope_جدول_Name = symClass + "__VpiScopeTable";
        stmts.جوڑو("VerilatedScope::scopesConstructFromTable(" + m_scope_جدول_Name + ", "
                  + std::مقصد_string(m_scope_نام.سائز()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_جدول();
    اگر (!m_iface_Ref_جدول_صفیں.خالی_خانے()) {
        stmts.جوڑو("// Setup interface references");
        stmts.جوڑو("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::مقصد_string(m_iface_Ref_جدول_صفیں.سائز()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, جھوٹ);

    اگر (v3Global.dpi()) {
        برائے (ثابت std::string vfinal : {"0", "1"}) {
            stmts.جوڑو("// Setup export functions - final: " + vfinal);
            برائے (ثابت خودکار& itpair : m_scopeFuncs) {
                ثابت Scope_Func_ڈیٹا& sfd = itpair.دوسرا;
                ثابت AstScopeName* ثابت scopep = sfd.m_scopep;
                ثابت AstCFunc* ثابت funcp = sfd.m_cfuncp;
                ثابت Ast_گرہ_Module* ثابت modp = sfd.m_modp;
                اگر (!funcp->dpiExportImpl()) جاری;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_برآمد_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::سابقہ_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.جوڑو(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    اگر (!m_scopeVars.خالی_خانے()) {
        stmts.جوڑو("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> جدول_By_صفیں;
        صحیح جدول_شمار = 0;

        خودکار it = m_scopeVars.cbegin();
        جب (it != m_scopeVars.cend()) {
            ثابت std::string scopeName = it->دوسرا.m_scopeName;
            ثابت AstScope* ثابت instScopep = it->دوسرا.m_scopep;
            ثابت Ast_گرہ_Module* ثابت modp = it->دوسرا.m_modp;
            ثابت std::string modClassName = EmitCUtil::سابقہ_Name_Protect(modp);

            std::سمتیہ<std::string> صفیں;
            std::سمتیہ<std::string> residual;

            برائے (; it != m_scopeVars.cend() && it->دوسرا.m_scopeName == scopeName; ++it) {
                ثابت Scope_Var_ڈیٹا& svd = it->دوسرا;
                ثابت AstScope* ثابت scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ثابت AstVar* ثابت varp = svd.m_varp;
                ثابت VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ثابت std::pair<منطقی, std::string> fc = isForceControlSignal(varp);
                اگر (fc.پہلا && بنیاد_Signal_Is_عوامی(scopep, fc.دوسرا)
                    && بنیاد_Signal_Is_Valid(scopep, varp, fc.دوسرا)) {
                    جاری;
                }

                std::string صف;
                ثابت جدول_اندراج_قسم قسم
                    = try_Build_جدول_اندراج(svd, varp, scopep, modClassName, dims, صف);
                انتخاب (قسم) {
                حالت جدول_اندراج_قسم::جدول_صف:
                    صفیں.emplace_عقب(صف);
                    جوڑو_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_بنیاد_Pretty,
                                           varName(varp), varp->dtypep());
                    توڑو;
                حالت جدول_اندراج_قسم::FORCEABLE_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_عقب(داخل_کرو_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    توڑو;
                }
                حالت جدول_اندراج_قسم::PLAIN_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_عقب(
                        داخل_کرو_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    توڑو;
                }
                طےشدہ: v3fatalSrc("Bad case");
                }
                اگر (قسم == جدول_اندراج_قسم::جدول_صف) جاری;
                اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), گرہ_UOr_Struct_DType)) {
                    اگر (!sdtypep->packed()) {
                        جوڑو_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_بنیاد_Pretty,
                                               varName(varp), sdtypep);
                    }
                } ورنہ اگر (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    جوڑو_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_بنیاد_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                اگر (قسم == جدول_اندراج_قسم::PLAIN_RESIDUAL) {
                    جوڑو_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_بنیاد_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            اگر (!صفیں.خالی_خانے()) {
                ثابت سائز_t صف_گنتی = صفیں.سائز();
                std::string کلید;
                برائے (ثابت std::string& r : صفیں) {
                    کلید += r;
                    کلید += '\0';
                }
                ثابت خودکار itt = جدول_By_صفیں.ڈھونڈو(کلید);
                std::string جدول_Name;
                اگر (itt != جدول_By_صفیں.انتہا()) {
                    جدول_Name = itt->دوسرا;
                } ورنہ {
                    جدول_Name = modClassName + "__VpiVarTable" + std::مقصد_string(جدول_شمار++);
                    جدول_By_صفیں.emplace(std::move(کلید), جدول_Name);
                    m_varTables.emplace_عقب(جدول_Name, std::move(صفیں));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + جدول_Name + ", "
                                   + std::مقصد_string(صف_گنتی) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.جوڑو(call);
            }
            برائے (ثابت std::string& r : residual) stmts.جوڑو(r);
        }
    }

    واپس stmts;
}

EmitCSyms::متن_Stmts EmitCSyms::getSymDtorStmts() {
    متن_Stmts stmts;

    emitScopeHier(stmts, سچ);
    اگر (v3Global.needTraceDumper()) {
        stmts.جوڑو("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    اگر (v3Global.opt.profPgo()) {
        stmts.جوڑو("vlSymsp->_vm_pgoProfiler.write(\"" + چوٹی_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    اگر (!m_iface_Ref_جدول_صفیں.خالی_خانے()) {
        stmts.جوڑو("// Tear down interface references");
        stmts.جوڑو("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::مقصد_string(m_iface_Ref_جدول_صفیں.سائز()) + ", vlSymsp);");
    }

    stmts.جوڑو("// Tear down scopes");
    برائے (ثابت خودکار& itpair : m_scope_نام) {
        ثابت Scope_ڈیٹا& sd = itpair.دوسرا;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جوڑو("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.جوڑو("// Tear down sub module instances");
    برائے (ثابت ScopeModPair& itpair : vlstd::الٹ_view(m_scopes)) {
        ثابت AstScope* ثابت scopep = itpair.پہلا;
        ثابت Ast_گرہ_Module* ثابت modp = itpair.دوسرا;
        اگر (modp->is_چوٹی()) جاری;
        stmts.جوڑو("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    واپس stmts;
}

خالی EmitCSyms::emitSymImp(ثابت AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    متن_Stmts ctorStmts = getSymCtorStmts();
    متن_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ثابت std::pair<std::string, std::string> بانٹو_Ctor_Dtor_Decls  //
        = emit_بانٹو_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            سچ, "Symbol table implementation internals",
                            [یہ]() { emitSymImpPreamble(); });

    open_New_Output_ماخذ_File(symClassName(), سچ, سچ, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ثابت AstScope* ثابت چوٹی_Scopep = netlistp->چوٹی_Scopep()->scopep();
    ثابت std::string چوٹی_Inst = "vlSymsp->" + instName(چوٹی_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    اگر (!بانٹو_Ctor_Dtor_Decls.پہلا.خالی_خانے()) puts("\n" + بانٹو_Ctor_Dtor_Decls.پہلا + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + چوٹی_Inst + ", vlSymsp, namep);\n");
    برائے (ثابت متن_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + چوٹی_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    اگر (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    اگر (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    اگر (v3Global.opt.profPgo() && !v3Global.opt.libCreate().خالی_خانے()) {
        puts("    , _vm_pgoProfiler{" + std::مقصد_string(v3Global.current_Hier_بلاک_لاگت()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    اگر (!بانٹو_Ctor_Dtor_Decls.دوسرا.خالی_خانے()) puts("\n" + بانٹو_Ctor_Dtor_Decls.دوسرا + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    برائے (ثابت متن_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + چوٹی_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    اگر (v3Global.needTraceDumper()) {
        اگر (!optSystemC()) {
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

    اگر (v3Global.opt.savable()) {
        برائے (ثابت منطقی& de : {جھوٹ, سچ}) {
            ثابت std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ثابت std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ثابت std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            اگر (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            برائے (ثابت ScopeModPair& itpair : m_scopes) {
                ثابت AstScope* ثابت scopep = itpair.پہلا;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

خالی EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    open_New_Output_سرخی_File(چوٹی_Class_Name() + "__Dpi",
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
    std::سیٹ<std::string> struct_نام;
    برائے (ثابت AstCFunc* ثابت nodep : m_dpis) {
        برائے (ثابت Ast_گرہ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            اگر (ثابت AstVar* ثابت portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_نام);
            }
        }
    }
    اگر (!struct_نام.خالی_خانے()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    صحیح پہلا_Exp = 0;
    صحیح پہلا_Imp = 0;
    برائے (ثابت AstCFunc* ثابت nodep : m_dpis) {
        اگر (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) جاری;

        ثابت std::string ماخذ_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        اگر (nodep->dpiExportDispatcher()) {
            اگر (!پہلا_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + ماخذ_Loc + "\n");
        } ورنہ {
            اگر (!پہلا_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + ماخذ_Loc + "\n");
        }
        اگر (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } ورنہ {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_دلائل(nodep, جھوٹ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_انتہا_Guard();

    closeOutputFile();
}

خالی EmitCSyms::emitDpiStruct(ثابت Ast_گرہ_DType* dtypep, std::سیٹ<std::string>& namesr) {
    اگر (!dtypep->شامل_ہے_Unpacked_Struct()) واپس;
    ثابت AstStructDType* ثابت sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ثابت std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    اگر (namesr.گنتی(name)) واپس;
    // After the structures it contains
    برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    اگر (name.خالی_خانے()) واپس;  // Declared where used
    اگر (namesr.خالی_خانے()) puts("// DPI TYPES\n");
    namesr.داخل_کرو(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

خالی EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ماخذ_File(چوٹی_Class_Name() + "__Dpi", جھوٹ, سچ,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + چوٹی_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + چوٹی_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + چوٹی_Class_Name() + ".h\"\n");
    puts("\n");

    برائے (ثابت AstCFunc* ثابت nodep : m_dpis) {
        اگر (!nodep->dpiExportDispatcher()) جاری;

        ثابت std::string name = nodep->name();
        ثابت std::string ماخذ_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_دلائل(nodep) + ") {\n");
        puts("// DPI export" + ماخذ_Loc + "\n");
        putns(nodep, "return " + چوٹی_Class_Name() + "::" + name + "(");
        std::string comma;
        برائے (ثابت Ast_گرہ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO() && !portp->isFuncReturn()) {
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

خالی V3EmitC::emitcSyms(منطقی dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
