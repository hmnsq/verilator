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
ساكن constexpr خودڪار symClassName = &EmitCUtil::symClassName;
ساكن constexpr خودڪار چوٽي_Class_Name = &EmitCUtil::چوٽي_Class_Name;

صنف EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ثابت VNUser1_اندريون_Use m_inuser1;

    // TYPES
    بنية Scope_معلومات final {
        ثابت Ast_ڳنڍ* m_nodep;  // Scope symbol-table entry corresponds to
        ثابت std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ثابت std::string m_prettyName;  // Pretty (unprotected) name for display
        ثابت std::string m_defName;  // Name registered for this scope
        ثابت صحيح m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_معلومات(ثابت Ast_ڳنڍ* nodep, ثابت std::string& symName, ثابت std::string& prettyName,
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
        std::string m_والد_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_معلومات(ثابت AstScope* scopep, ثابت std::string& والد_Sym,
                     ثابت std::string& suffix, ثابت std::string& name,
                     ثابت std::string& modportName)
            : m_scopep{scopep}
            , m_والد_Sym{والد_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    بنية Scope_Func_معلومات final {
        ثابت AstScopeName* ثابت m_scopep;  // Scope the DPI export function is registered under
        ثابت AstCFunc* ثابت m_cfuncp;  // DPI export function
        ثابت Ast_ڳنڍ_Module* ثابت m_modp;  // Module containing the DPI export function
        Scope_Func_معلومات(ثابت AstScopeName* scopep, ثابت AstCFunc* funcp, ثابت Ast_ڳنڍ_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    بنية Scope_Var_معلومات final {
        ثابت std::string m_scopeName;  // Full name of the scope containing the variable
        ثابت std::string m_var_بنياد_Pretty;  // Pretty base name of the variable, without scope
        ثابت AstVar* ثابت m_varp;  // Public variable
        ثابت Ast_ڳنڍ_Module* ثابت m_modp;  // Module containing the variable
        ثابت AstScope* ثابت m_scopep;  // Scope containing the variable
        Scope_Var_معلومات(ثابت std::string& scopeName, ثابت std::string& var_بنياد_Pretty,
                     ثابت AstVar* varp, ثابت Ast_ڳنڍ_Module* modp, ثابت AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_بنياد_Pretty{var_بنياد_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    استعمال Scope_نالا = std::map<ثابت std::string, Scope_معلومات>;
    استعمال ScopeModPair = std::pair<ثابت AstScope*, Ast_ڳنڍ_Module*>;
    استعمال ModVarPair = std::pair<ثابت Ast_ڳنڍ_Module*, ثابت AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ساكن constexpr صحيح VPI_جدول_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = خالي_اشارو;  // Current function
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    ثابت AstScope* m_scopep = خالي_اشارو;  // Current scope
    std::ويڪٽر<ScopeModPair> m_scopes;  // Every scope by module
    std::ويڪٽر<AstCFunc*> m_dpis;  // DPI functions
    std::ويڪٽر<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ثابت std::string, Scope_Func_معلومات> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ثابت std::string, Scope_Var_معلومات> m_scopeVars;  // Each {scope,public-var}
    Scope_نالا m_scope_نالا;  // Each unique AstScopeName. Dpi scopes added later
    Scope_نالا m_dpi_Scope_نالا;  // Each unique AstScopeName for DPI export
    Scope_نالا m_vpiScopeCandidates;  // All scopes for VPI
    std::ويڪٽر<Iface_Ref_معلومات> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ثابت std::string, std::ويڪٽر<std::string>> m_vpiScopeHierarchy;
    صحيح m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    صحيح m_modCoverBins = 0;  // Per-module coverage bin number
    ثابت منطقي m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_بائيٽون;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::ويڪٽر<std::pair<std::string, std::ويڪٽر<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_جدول_Name;
    std::ويڪٽر<std::string> m_scope_جدول_صفون;
    std::string m_iface_Ref_جدول_Name;
    std::ويڪٽر<std::string> m_iface_Ref_جدول_صفون;

    // METHODS
    فراغ emitSymHdr();
    فراغ emitSymImpPreamble();
    فراغ emitVarTables();
    فراغ emitScopeHier(متن_Stmts& stmts, منطقي destroy);
    فراغ emitSymImp(ثابت AstNetlist* netlistp);
    فراغ emitDpiHdr();
    فراغ emitDpiStruct(ثابت Ast_ڳنڍ_DType* dtypep, std::سيٽ<std::string>& namesr);
    فراغ emitDpiImp();

    متن_Stmts getSymCtorStmts();
    متن_Stmts getSymDtorStmts();

    ساكن فراغ nameCheck(Ast_ڳنڍ* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        إذا (nodep->name().خالي_جايون()) أرجع;
        ثابت AstCFunc* ثابت cfuncp = VN_CAST(nodep, CFunc);
        إذا (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ثابت std::string rsvd = V3_Language_لفظ::isKeyword(nodep->name());
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
        std::string ٻاهريون = scpname;
        std::string::سائيز_type pos;
        طالما ((pos = ٻاهريون.ڳوليو("__PVT__")) != std::string::npos) ٻاهريون.replace(pos, 7, "");
        إذا (VString::startsWith(ٻاهريون, "TOP__DOT__")) ٻاهريون.replace(0, 10, "");
        إذا (VString::startsWith(ٻاهريون, "TOP.")) ٻاهريون.replace(0, 4, "");
        طالما ((pos = ٻاهريون.ڳوليو('.')) != std::string::npos) ٻاهريون.replace(pos, 1, "__");
        طالما ((pos = ٻاهريون.ڳوليو("__DOT__")) != std::string::npos) ٻاهريون.replace(pos, 7, "__");
        أرجع ٻاهريون;
    }

    ساكن string scopeDecodeIdentifier(ثابت std::string& scpname) {
        std::string::سائيز_type pos = std::string::npos;

        // Remove hierarchy
        سائيز_t i = 0;
        // always makes progress
        طالما (i < scpname.ڊيگهائي()) {
            إذا (scpname[i] == '\\') {
                طالما (i < scpname.ڊيگهائي() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } وإلا {
                طالما (i < scpname.ڊيگهائي() && scpname[i] != '.') ++i;
                إذا (i < scpname.ڊيگهائي()) pos = i++;
            }
        }

        أرجع pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    بنية VarDims final {
        صحيح pdim = 0;  // Packed array dims plus a ranged basic leaf
        صحيح udim = 0;
        std::ويڪٽر<std::pair<صحيح, صحيح>> unpackedLR;  // (left,right), outer-first
        std::ويڪٽر<std::pair<صحيح, صحيح>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::ويڪٽر<std::pair<صحيح, صحيح>> flatten() ثابت {
            std::ويڪٽر<std::pair<صحيح, صحيح>> flat(unpackedLR);
            flat.وجهو(flat.پڇاڙي(), packedLR.cbegin(), packedLR.cend());
            أرجع flat;
        }
    };
    ساكن VarDims getVarDims(ثابت Ast_ڳنڍ_DType* ثابت پاڙ_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        لكل (ثابت Ast_ڳنڍ_DType* dtypep = پاڙ_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            إذا (ثابت Ast_ڳنڍ_Array_DType* ثابت adtypep = VN_CAST(dtypep, ڳنڍ_Array_DType)) {
                إذا (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_پٺ(adtypep->کاٻي(), adtypep->ساڄي());
                    ++d.pdim;
                } وإلا {
                    d.unpackedLR.emplace_پٺ(adtypep->کاٻي(), adtypep->ساڄي());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } وإلا {
                إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
                    إذا (basicp->isRanged()) {
                        d.packedLR.emplace_پٺ(basicp->کاٻي(), basicp->ساڄي());
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
            bounds += std::منزل_string(lr.پهريون);
            bounds += ",";
            bounds += std::منزل_string(lr.ٻيو);
        }
        أرجع bounds;
    }

    ساكن صحيح getUnpackedElements(ثابت Ast_ڳنڍ_DType* پاڙ_Dtypep) {
        صحيح elements = 1;
        لكل (ثابت Ast_ڳنڍ_DType* dtypep = پاڙ_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(dtypep, UnpackArrayDType);
            إذا (!adtypep) اكسر;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        أرجع elements;
    }

    ساكن منطقي needs_Emitted_Ent_سائيز(ثابت std::string& vlEnumType) {
        أرجع vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ساكن std::pair<منطقي, std::string> isForceControlSignal(ثابت AstVar* ثابت signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ثابت std::سائيز_t suffixPos = signalVarp->name().ڳوليو(forceControlSuffix);
            ثابت منطقي suffix_مليو = suffixPos != std::string::npos;
            ثابت منطقي suffix_Is_At_پڇاڙي
                = suffixPos + forceControlSuffix.ڊيگهائي() == signalVarp->name().ڊيگهائي();
            إذا (!(suffix_مليو && suffix_Is_At_پڇاڙي)) استمر;
            ثابت std::string بنياد_Signal_Name = signalVarp->name().substr(0, suffixPos);
            أرجع std::pair<منطقي, std::string>{سچ, بنياد_Signal_Name};
        }
        أرجع std::pair<منطقي, std::string>{خطأ, ""};
    }

    ساكن فراغ appendVarProperties(std::string& stmt, ثابت std::string& vlEnumType,
                                    ثابت std::string& vlEnumDir, ثابت صحيح udim, ثابت صحيح pdim,
                                    ثابت std::string& bounds, ثابت std::string& ent_سائيز = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::منزل_string(udim);
        إذا (!ent_سائيز.خالي_جايون()) {
            stmt += ", ";
            stmt += ent_سائيز;
        } وإلا {
            stmt += ", ";
            stmt += std::منزل_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ساكن std::string memberVlEnumDir(ثابت AstVar* ثابت varp,
                                       ثابت Ast_ڳنڍ_DType* ثابت dtypep) {
        std::string ٻاهريون = '(' + varp->vlEnumDir(/*forMember=*/سچ);
        ثابت Ast_ڳنڍ_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (skipDTypep->isSigned()) ٻاهريون += "|VLVF_SIGNED";
        إذا (ثابت AstBasicDType* ثابت basicp = skipDTypep->basicp()) {
            إذا (basicp->keyword() == VBasicDTypeKwd::ٻٽو_انگ) ٻاهريون += "|VLVF_BITVAR";
        }
        ٻاهريون += skipDTypep->vlEnumPackedAgg();
        أرجع ٻاهريون + ')';
    }

    // Name of the instance member for the given scope
    ساكن std::string instName(ثابت AstScope* scopep) {
        أرجع "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ساكن std::string وجهو_Var_Statement(ثابت Scope_Var_معلومات& svd, ثابت AstScope* ثابت scopep,
                                          ثابت AstVar* ثابت varp, ثابت صحيح udim, ثابت صحيح pdim,
                                          ثابت std::string& bounds) {
        ثابت std::string var_اظهار = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ثابت std::string vlEnumType = varp->vlEnumType();
        ثابت منطقي needs_Ent_سائيز = needs_Emitted_Ent_سائيز(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_سائيز ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_ٻاهريون_Formatter::quoteNameControls(protect(svd.m_var_بنياد_Pretty)) + '"';

        إذا (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_اظهار;
            stmt += "), false, ";
        } وإلا إذا (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_اظهار;
            stmt += ".c_str())), true, ";
        } وإلا {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_اظهار;
            stmt += "))), true, ";
        }

        ثابت std::string ent_سائيز = needs_Ent_سائيز
                                        ? "sizeof(" + var_اظهار + ") / "
                                              + std::منزل_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_سائيز);
        أرجع stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    تعداد صنف جدول_داخلا_قسم : uint8_t {
        جدول_صف,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    جدول_داخلا_قسم try_Build_جدول_داخلا(ثابت Scope_Var_معلومات& svd, ثابت AstVar* ثابت varp,
                                      ثابت AstScope* ثابت scopep,
                                      ثابت std::string& modClassName, ثابت VarDims& dims,
                                      std::string& صف_ٻاهريون) ثابت {
        ثابت صحيح pdim = dims.pdim;
        ثابت صحيح udim = dims.udim;
        إذا (varp->isForceable() && force_Control_سگنل_Are_Valid(scopep, varp)) {
            أرجع جدول_داخلا_قسم::FORCEABLE_RESIDUAL;
        }
        إذا (udim + pdim > VPI_جدول_MAX_DIMS) أرجع جدول_داخلا_قسم::PLAIN_RESIDUAL;

        ثابت std::string vlEnumType = varp->vlEnumType();
        إذا (needs_Emitted_Ent_سائيز(vlEnumType))
            أرجع جدول_داخلا_قسم::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        إذا (varp->isParam()) أرجع جدول_داخلا_قسم::PLAIN_RESIDUAL;

        ثابت std::string name = V3_ٻاهريون_Formatter::quoteNameControls(protect(svd.m_var_بنياد_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ثابت std::string member = varName(varp);
        ثابت std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::ويڪٽر<صحيح> dv;
        لكل (ثابت std::pair<صحيح, صحيح>& lr : dims.flatten()) {
            dv.ڌڪو_پٺ(lr.پهريون);
            dv.ڌڪو_پٺ(lr.ٻيو);
        }

        std::string صف = "{\"" + name + "\", ";
        صف += "offsetof(" + modClassName + ", " + member + "), ";
        صف += vlEnumType + ", ";
        صف += "(" + dir + "), ";
        صف += std::منزل_string(udim) + ", " + std::منزل_string(pdim) + ", {";
        لكل (صحيح i = 0; i < VPI_جدول_MAX_DIMS * 2; ++i) {
            إذا (i) صف += ", ";
            صف += std::منزل_string(i < static_cast<صحيح>(dv.سائيز()) ? dv[i] : 0);
        }
        صف += "}}";
        صف_ٻاهريون = صف;
        أرجع جدول_داخلا_قسم::جدول_صف;
    }

    ساكن std::string وجهو_DType_Var_Statement(ثابت Scope_Var_معلومات& svd,
                                               ثابت AstScope* ثابت scopep,
                                               ثابت std::string& prettyName,
                                               ثابت std::string& cName,
                                               ثابت Ast_ڳنڍ_DType* ثابت dtypep, ثابت صحيح udim,
                                               ثابت صحيح pdim, ثابت std::string& bounds) {
        ثابت std::string vlEnumType = dtypep->vlEnumType();
        ثابت منطقي needs_Ent_سائيز = needs_Emitted_Ent_سائيز(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_سائيز ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_ٻاهريون_Formatter::quoteNameControls(prettyName) + '"';
        ثابت std::string var_اظهار = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        إذا (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_اظهار;
            stmt += "))), true, ";
        } وإلا {
            stmt += ", &(";
            stmt += var_اظهار;
            stmt += "), false, ";
        }
        ثابت std::string ent_سائيز
            = needs_Ent_سائيز
                  ? "sizeof(" + var_اظهار + ") / " + std::منزل_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_سائيز);
        أرجع stmt;
    }

    ساكن فراغ جوڙيو_UOr_Struct_Member_Vars(std::ويڪٽر<std::string>& stmts, ثابت Scope_Var_معلومات& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_اڳياڙي, ثابت std::string& c_اڳياڙي,
                                       ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep) {
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_ڳنڍ_DType* ثابت شي_DTypep = itemp->dtypep();
            إذا (شي_DTypep->vlEnumType().خالي_جايون()) استمر;
            ثابت std::string prettyName
                = pretty_اڳياڙي + "." + Ast_ڳنڍ::vpiName(itemp->shortName());
            ثابت std::string cName = c_اڳياڙي + "." + itemp->nameProtect();
            ثابت VarDims dims = getVarDims(شي_DTypep);
            stmts.emplace_پٺ(وجهو_DType_Var_Statement(svd, scopep, prettyName, cName, شي_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            جوڙيو_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, شي_DTypep);
            إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت subp
                = VN_CAST(شي_DTypep->skipRefp(), ڳنڍ_UOr_Struct_DType)) {
                إذا (!subp->packed())
                    جوڙيو_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } وإلا إذا (VN_IS(شي_DTypep->skipRefp(), UnpackArrayDType)) {
                جوڙيو_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    شي_DTypep);
            }
        }
    }

    ساكن فراغ جوڙيو_Unpacked_Array_UOr_Struct_Member_Vars(std::ويڪٽر<std::string>& stmts,
                                                    ثابت Scope_Var_معلومات& svd,
                                                    ثابت AstScope* ثابت scopep,
                                                    ثابت std::string& pretty_اڳياڙي,
                                                    ثابت std::string& c_اڳياڙي,
                                                    ثابت Ast_ڳنڍ_DType* ثابت dtypep) {
        ثابت Ast_ڳنڍ_DType* ثابت skipDTypep = dtypep->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            جوڙيو_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_اڳياڙي, c_اڳياڙي + "[0]",
                                                adtypep->subDTypep());
        } وإلا إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep
                   = VN_CAST(skipDTypep, ڳنڍ_UOr_Struct_DType)) {
            إذا (!sdtypep->packed())
                جوڙيو_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_اڳياڙي, c_اڳياڙي, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ساكن فراغ جوڙيو_Packed_Member_Vars(std::ويڪٽر<std::string>& stmts, ثابت Scope_Var_معلومات& svd,
                                    ثابت AstScope* ثابت scopep, ثابت std::string& pretty_اڳياڙي,
                                    ثابت std::string& cName, ثابت std::string& vlEnumType,
                                    ثابت std::string& ent_سائيز,
                                    ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep,
                                    ثابت صحيح والد_ٻٽو_انگ_وٿي, ثابت منطقي needs_اشاريو) {
        ثابت std::string var_اظهار = "vlSymsp->" + instName(scopep) + "." + cName;
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ثابت Ast_ڳنڍ_DType* ثابت شي_DTypep = itemp->dtypep();
            ثابت std::string prettyName
                = pretty_اڳياڙي + "." + Ast_ڳنڍ::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ثابت صحيح ٻٽو_انگ_وٿي = والد_ٻٽو_انگ_وٿي + itemp->lsb();
            ثابت VarDims dims = getVarDims(شي_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_ٻاهريون_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            إذا (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_اظهار + "))), true, ";
            } وإلا {
                stmt += ", &(" + var_اظهار + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, شي_DTypep);
            إذا (needs_اشاريو) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::منزل_string(ٻٽو_انگ_وٿي) + ", ";
            stmt += ent_سائيز + ", ";
            stmt += std::منزل_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_پٺ(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ثابت Ast_ڳنڍ_DType* subDTypep = شي_DTypep->skipRefp();
            منطقي isArray = خطأ;
            طالما (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = سچ;
            }
            إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت subp = VN_CAST(subDTypep, ڳنڍ_UOr_Struct_DType)) {
                جوڙيو_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_سائيز,
                                    subp, ٻٽو_انگ_وٿي, needs_اشاريو || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ساكن فراغ جوڙيو_Packed_Agg_Member_Vars(std::ويڪٽر<std::string>& stmts, ثابت Scope_Var_معلومات& svd,
                                       ثابت AstScope* ثابت scopep,
                                       ثابت std::string& pretty_اڳياڙي, ثابت std::string& cName,
                                       ثابت Ast_ڳنڍ_DType* ثابت dtypep) {
        std::string elemCName = cName;
        ثابت Ast_ڳنڍ_DType* elemDTypep = dtypep->skipRefp();
        منطقي isArray = خطأ;
        طالما (ثابت Ast_ڳنڍ_Array_DType* ثابت adtypep = VN_CAST(elemDTypep, ڳنڍ_Array_DType)) {
            إذا (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = سچ;
        }
        ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(elemDTypep, ڳنڍ_UOr_Struct_DType);
        إذا (!sdtypep || !sdtypep->packed()) أرجع;
        ثابت std::string var_اظهار = "vlSymsp->" + instName(scopep) + "." + cName;
        ثابت std::string ent_سائيز
            = "sizeof(" + var_اظهار + ") / " + std::منزل_string(getUnpackedElements(dtypep));
        جوڙيو_Packed_Member_Vars(stmts, svd, scopep, pretty_اڳياڙي, elemCName, dtypep->vlEnumType(),
                            ent_سائيز, sdtypep, 0, isArray);
    }

    std::string وجهو_Forceable_Var_Statement(ثابت Scope_Var_معلومات& svd, ثابت AstScope* ثابت scopep,
                                            ثابت AstVar* ثابت varp, ثابت صحيح udim,
                                            ثابت صحيح pdim, ثابت std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_ٻاهريون_Formatter::quoteNameControls(protect(svd.m_var_بنياد_Pretty)) + '"';

        ثابت std::string var_اظهار = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        أكد(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_اظهار;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_اظهار + "__VforceRd";
        stmt += "), \"" + V3_ٻاهريون_Formatter::quoteNameControls(protect(svd.m_var_بنياد_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        لكل (ثابت std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ثابت std::string enable_Signal_ڪنجي
                = get_ڪنجي_Name(scopep, varp->name() + forceControlSuffix);
            ثابت std::map<ثابت std::string, Scope_Var_معلومات>::const_iterator itpair
                = m_scopeVars.ڳوليو(enable_Signal_ڪنجي);

            إذا (itpair == m_scopeVars.پڇاڙي()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_ڪنجي << "'.");
            }

            ثابت Scope_Var_معلومات& svd = itpair->ٻيو;
            ثابت AstScope* ثابت scopep = svd.m_scopep;
            ثابت AstVar* ثابت varp = svd.m_varp;
            ثابت VarDims dims = dimsFor(svd);
            stmt
                += وجهو_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
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

    ساكن std::string get_ڪنجي_Name(ثابت AstScope* ثابت scopep, ثابت std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //ترتيب
        // signal can be found.
        std::string سڄو = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        إذا (VString::startsWith(سڄو, "__DOT__TOP")) سڄو.replace(0, 10, "");
        ثابت std::string::سائيز_type dpos = سڄو.rfind("__DOT__");
        إذا (dpos != std::string::npos) { scpName = سڄو.substr(0, dpos); }
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        أرجع scpSym + " " + signal_name;
    }

    منطقي بنياد_Signal_Is_Valid(ثابت AstScope* ثابت scopep, ثابت AstVar* ثابت controlSignalVarp,
                           ثابت std::string& بنياد_Signal_Name) ثابت {
        ثابت std::string بنياد_Signal_ڪنجي = get_ڪنجي_Name(scopep, بنياد_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_معلومات>::const_iterator بنياد_Signal_It
            = m_scopeVars.ڳوليو(بنياد_Signal_ڪنجي);
        إذا (بنياد_Signal_It == m_scopeVars.پڇاڙي()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت بنياد_Signal_Varp = بنياد_Signal_It->ٻيو.m_varp;
            إذا (m_scopeVars.ڳڻپ(بنياد_Signal_Name) > 1) {
                بنياد_Signal_Varp->v3fatalSrc(
                    "Found Signal " << بنياد_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << بنياد_Signal_Name << " occurring in the same scope.");
                أرجع خطأ;
            }
            إذا (!بنياد_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << بنياد_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                أرجع خطأ;
            }
        }
        أرجع سچ;
    }

    منطقي بنياد_Signal_Is_عوامي(ثابت AstScope* ثابت scopep,
                            ثابت std::string& بنياد_Signal_Name) ثابت {
        ثابت std::string بنياد_Signal_ڪنجي = get_ڪنجي_Name(scopep, بنياد_Signal_Name);
        ثابت std::map<ثابت std::string, Scope_Var_معلومات>::const_iterator بنياد_Signal_It
            = m_scopeVars.ڳوليو(بنياد_Signal_ڪنجي);
        إذا (بنياد_Signal_It == m_scopeVars.پڇاڙي()) {
            أرجع خطأ;
        } وإلا {
            ثابت AstVar* ثابت بنياد_Signal_Varp = بنياد_Signal_It->ٻيو.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            إذا (!بنياد_Signal_Varp->is_Sig_عوامي()) أرجع خطأ;
        }
        أرجع سچ;
    }

    منطقي force_Control_سگنل_Are_Valid(ثابت AstScope* ثابت scopep,
                                     ثابت AstVar* ثابت بنياد_Signal_Varp) ثابت {
        constexpr std::array<ثابت حرف*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        أرجع std::all_جو(
            forceControlSuffixes.begin(), forceControlSuffixes.پڇاڙي(),
            [scopep, بنياد_Signal_Varp, هذا](ثابت std::string& forceControlSuffix) {
                ثابت std::string controlSignalName = بنياد_Signal_Varp->name() + forceControlSuffix;
                ثابت std::string control_Signal_ڪنجي = get_ڪنجي_Name(scopep, controlSignalName);
                ثابت std::سائيز_t control_Signal_ڳڻپ = m_scopeVars.ڳڻپ(control_Signal_ڪنجي);
                إذا (control_Signal_ڳڻپ == 0) {
                    بنياد_Signal_Varp->v3fatalSrc("Signal "
                                               << بنياد_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_ڪنجي << "'.");
                    أرجع خطأ;
                }
                إذا (control_Signal_ڳڻپ > 1) {
                    بنياد_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << بنياد_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    أرجع خطأ;
                }
                أرجع سچ;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    فراغ varHierarchyScopes(std::string scp) {

        std::string::سائيز_type prd_pos = scp.rfind('.');
        std::string::سائيز_type dot_pos = scp.rfind("__DOT__");

        طالما (!scp.خالي_جايون()) {
            ثابت خودڪار scpit = m_vpiScopeCandidates.ڳوليو(scopeSymString(scp));
            إذا ((scpit != m_vpiScopeCandidates.پڇاڙي())
                && (m_scope_نالا.ڳوليو(scp) == m_scope_نالا.پڇاڙي())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ثابت خودڪار pair = m_scope_نالا.emplace(scpit->ٻيو.m_symName, scpit->ٻيو);
                إذا (!pair.ٻيو) pair.پهريون->ٻيو.m_type = scpit->ٻيو.m_type;
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

    فراغ جوڙيو_Scope_Var_داخلا(ثابت AstScope* ثابت scopep, ثابت Ast_ڳنڍ_Module* ثابت modp,
                          ثابت AstVar* ثابت varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string سڄو = scopep->name() + "__DOT__" + varp->name();
        إذا (VString::startsWith(سڄو, "__DOT__TOP")) سڄو.replace(0, 10, "");
        ثابت std::string::سائيز_type dpos = سڄو.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ثابت std::string scpName = سڄو.substr(0, dpos);
        ثابت std::string var_بنياد = سڄو.substr(dpos + std::لڙي_ڊيگهه("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ثابت std::string var_بنياد_Pretty = Ast_ڳنڍ::vpiName(VName::dehash(var_بنياد));
        ثابت std::string scpPretty = Ast_ڳنڍ::prettyName(VName::dehash(scpName));
        ثابت std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        إذا (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_نالا.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_بنياد_Pretty, varp, modp, scopep));
    }

    فراغ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        لكل (ثابت ScopeModPair& smPair : m_scopes) {
            ثابت AstScope* ثابت scopep = smPair.پهريون;
            ثابت Ast_ڳنڍ_Module* ثابت smodp = smPair.ٻيو;
            لكل (ثابت ModVarPair& mvPair : m_modVars) {
                ثابت Ast_ڳنڍ_Module* ثابت modp = mvPair.پهريون;
                ثابت AstVar* ثابت varp = mvPair.ٻيو;
                إذا (modp != smodp) استمر;
                جوڙيو_Scope_Var_داخلا(scopep, modp, varp);
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
        ثابت std::string رستو = Ast_ڳنڍ::vpiName(nodep->name());
        ثابت std::string instName = Ast_ڳنڍ::vpiName(cellp->origName());
        UASSERT_OBJ(رستو.ڊيگهائي() > instName.ڊيگهائي() && VString::پڇاڙيون_With(رستو, instName), nodep,
                    "Interface scope name " << رستو << " does not end with instance name "
                                            << instName);
        ثابت std::string والد_رستو = رستو.substr(0, رستو.ڊيگهائي() - instName.ڊيگهائي());

        لكل (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ثابت std::string refName = Ast_ڳنڍ::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            إذا (!VString::startsWith(refName, والد_رستو)) استمر;
            m_ifaceRefs.emplace_پٺ(nodep, scopeSymString(intfRefp->والد_Name()), refName,
                                     Ast_ڳنڍ::vpiName(intfRefp->بنياد_Name()),
                                     intfRefp->modportName());
        }
    }

    فراغ build_Iface_Ref_جدول() {
        إذا (m_ifaceRefs.خالي_جايون()) أرجع;
        ثابت std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_ترتيب_ڏيو(
            m_ifaceRefs.begin(), m_ifaceRefs.پڇاڙي(),
            [](ثابت Iface_Ref_معلومات& a, ثابت Iface_Ref_معلومات& b) { أرجع a.m_suffix < b.m_suffix; });
        لكل (ثابت Iface_Ref_معلومات& ird : m_ifaceRefs) {
            ثابت std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            إذا (m_scope_نالا.ڳوليو(scopeSym) == m_scope_نالا.پڇاڙي()) استمر;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string والد_وٿي = "VL_IFACEREF_NO_PARENT";
            ثابت خودڪار والد = m_scope_نالا.ڳوليو(ird.m_والد_Sym);
            إذا (والد != m_scope_نالا.پڇاڙي()) {
                والد_وٿي = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + والد->ٻيو.m_symName) + ")";
            }
            std::string صف
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            صف += والد_وٿي + ", \"";
            صف += V3_ٻاهريون_Formatter::quoteNameControls(VIdProtect::protect_لفظ_If(ird.m_name, سچ));
            صف += "\", \"";
            صف += V3_ٻاهريون_Formatter::quoteNameControls(
                VIdProtect::protect_لفظ_If(ird.m_suffix, سچ));
            صف += "\", \"";
            صف += V3_ٻاهريون_Formatter::quoteNameControls(
                VIdProtect::protect_لفظ_If(ird.m_modportName, سچ));
            صف += "\"}";
            m_iface_Ref_جدول_صفون.emplace_پٺ(std::move(صف));
        }
        إذا (!m_iface_Ref_جدول_صفون.خالي_جايون()) m_iface_Ref_جدول_Name = symClass + "__VpiIfaceRefTable";
    }

    فراغ buildVpiHierarchy() {
        لكل (ثابت خودڪار& itpair : m_scope_نالا) {
            ثابت std::string symName = itpair.ٻيو.m_symName;
            std::string above = symName;
            إذا (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            طالما (!above.خالي_جايون()) {
                ثابت std::string::سائيز_type pos = above.rfind("__");
                إذا (pos == std::string::npos) اكسر;
                above.resize(pos);
                إذا (m_vpiScopeHierarchy.ڳوليو(above) != m_vpiScopeHierarchy.پڇاڙي()) {
                    m_vpiScopeHierarchy[above].ڌڪو_پٺ(symName);
                    اكسر;
                }
            }
            m_vpiScopeHierarchy[symName] = std::ويڪٽر<string>();
        }
    }

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        إذا (v3Global.opt.vpi()) buildVpiHierarchy();

        إذا (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            لكل (ثابت خودڪار& scp : m_dpi_Scope_نالا) m_scope_نالا.emplace(scp.پهريون, scp.ٻيو);
        }

        // Sort by names, so line/process order matters less
        std::stable_ترتيب_ڏيو(m_scopes.begin(), m_scopes.پڇاڙي(),
                         [](ثابت ScopeModPair& a, ثابت ScopeModPair& b) {
                             أرجع a.پهريون->name() < b.پهريون->name();
                         });
        std::stable_ترتيب_ڏيو(m_dpis.begin(), m_dpis.پڇاڙي(),  //
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
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
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
    فراغ گهمو(AstCellInlineScope* nodep) override {
        ثابت std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ثابت std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ثابت صحيح timeunit = m_modp->timeunit().power_جو_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ڳنڍ::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    فراغ گهمو(AstScope* nodep) override {
        إذا (VN_IS(m_modp, Class)) أرجع;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_پٺ(nodep, m_modp);

        إذا (v3Global.opt.vpi() && !nodep->is_چوٽي()) {
            ثابت std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ثابت صحيح timeunit = m_modp->timeunit().power_جو_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_ڳنڍ::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstScopeName* nodep) override {
        إذا (nodep->selfRelative()) أرجع;  // Its absolute copy is under the scope
        ثابت std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ثابت صحيح timeunit = m_modp ? m_modp->timeunit().power_جو_Ten() : 0;
        m_dpi_Scope_نالا.emplace(  //
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
            m_dpi_Scope_نالا.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    فراغ گهمو(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        إذا ((nodep->is_Sig_User_Rd_عوامي() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().خالي_جايون()) {
            m_modVars.emplace_پٺ(m_modp, nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        إذا (!nodep->معلومات_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_انگ(m_coverBins);
            m_coverBins += nodep->سائيز();
            nodep->local_Bin_انگ(m_modCoverBins);
            m_modCoverBins += nodep->سائيز();
        }
    }
    فراغ گهمو(AstCFunc* nodep) override {
        nameCheck(nodep);
        إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ڌڪو_پٺ(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    فراغ گهمو(AstConst*) override {}
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    واضح EmitCSyms(AstNetlist* nodep, منطقي dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

فراغ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_سرو_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_چوٽي_Include();
    puts("#include \"verilated.h\"\n");
    إذا (v3Global.needTraceDumper()) {
        لكل (ثابت string& بنياد : v3Global.opt.trace_ذريعو_Langs())
            puts("#include \"" + بنياد + ".h\"\n");
    }
    إذا (v3Global.opt.استعمال_ڪري_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + چوٽي_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    لكل (Ast_ڳنڍ_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ڳنڍ_Module);
        إذا (VN_IS(nodep, Class)) استمر;  // Class included earlier
        إذا (nodep->isConstPool()) استمر;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::اڳياڙي_Name_Protect(nodep) + ".h\"\n");
    }

    إذا (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::سيٽ<std::string> types;  // Remove duplicates and sort
        لكل (ثابت خودڪار& itpair : m_scopeFuncs) {
            ثابت AstCFunc* ثابت funcp = itpair.ٻيو.m_cfuncp;
            إذا (!funcp->dpiExportImpl()) استمر;
            ثابت std::string cbtype
                = protect(v3Global.opt.اڳياڙي() + "__Vcb_" + funcp->cname() + "_t");
            ثابت std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_دليلون(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        لكل (ثابت std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_ذاتي(خطأ);  // public:

    puts("// INTERNAL STATE\n");
    puts(چوٽي_Class_Name() + "* const __Vm_modelp;\n");

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
        puts("VlPgoProfiler<" + std::منزل_string(ExecMTask::انگ_استعمال_ٿيل_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.پهريون;
        ثابت Ast_ڳنڍ_Module* ثابت modp = itpair.ٻيو;
        إذا (VN_IS(modp, Class)) استمر;
        ofp()->لکو("%-30s ", EmitCUtil::اڳياڙي_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    إذا (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.ڌاڳا() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::منزل_string(m_coverBins));
        puts("];\n");
    }

    إذا (!m_scope_نالا.خالي_جايون()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        لكل (ثابت خودڪار& itpair : m_scope_نالا) {
            ثابت Scope_معلومات& sd = itpair.ٻيو;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    إذا (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + چوٽي_Class_Name()
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

    ofp()->puts_پڇاڙي_Guard();
    closeOutputFile();
}

فراغ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    منطقي needs_New_سٽ = خطأ;
    لكل (ثابت خودڪار& itpair : m_scopeFuncs) {
        ثابت AstCFunc* ثابت funcp = itpair.ٻيو.m_cfuncp;
        إذا (!funcp->dpiExportImpl()) استمر;
        emitCFuncDecl(funcp, itpair.ٻيو.m_modp);
        needs_New_سٽ = سچ;
    }
    إذا (needs_New_سٽ) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::سيٽ<std::string> lib_نالا;
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        إذا (itpair.ٻيو->verilatorLib()) lib_نالا.emplace(itpair.ٻيو->origName());
    }
    لكل (ثابت std::string& libName : lib_نالا) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    إذا (!lib_نالا.خالي_جايون()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    إذا (!m_varTables.خالي_جايون() || !m_scope_جدول_صفون.خالي_جايون() || !m_iface_Ref_جدول_صفون.خالي_جايون()) {
        لكل (ثابت خودڪار& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.پهريون + "[];\n");
        }
        إذا (!m_scope_جدول_صفون.خالي_جايون()) {
            puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[];\n");
        }
        إذا (!m_iface_Ref_جدول_صفون.خالي_جايون()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[];\n");
        }
        puts("\n");
    }
}

فراغ EmitCSyms::emitVarTables() {
    إذا (m_varTables.خالي_جايون() && m_scope_جدول_صفون.خالي_جايون() && m_iface_Ref_جدول_صفون.خالي_جايون()) أرجع;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    لكل (ثابت خودڪار& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.پهريون + "[] = {\n");
        لكل (ثابت std::string& صف : kv.ٻيو) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(صف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_scope_جدول_صفون.خالي_جايون()) {
        puts("extern const VlScopeTableEntry " + m_scope_جدول_Name + "[] = {\n");
        لكل (ثابت std::string& صف : m_scope_جدول_صفون) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(صف);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    إذا (!m_iface_Ref_جدول_صفون.خالي_جايون()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_جدول_Name + "[] = {\n");
        لكل (ثابت std::string& صف : m_iface_Ref_جدول_صفون) {
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

فراغ EmitCSyms::emitScopeHier(متن_Stmts& stmts, منطقي destroy) {
    إذا (!v3Global.opt.vpi()) أرجع;

    إذا (destroy) {
        stmts.جوڙيو("// Tear down scope hierarchy");
    } وإلا {
        stmts.جوڙيو("// Set up scope hierarchy");
    }

    ثابت std::string طريقو = destroy ? "remove" : "add";
    لكل (ثابت خودڪار& itpair : m_scope_نالا) {
        إذا (itpair.پهريون == "TOP") استمر;
        ثابت Scope_معلومات& sd = itpair.ٻيو;
        ثابت std::string& name = sd.m_prettyName;
        ثابت std::string& scopeType = sd.m_type;
        إذا (name.ڳوليو('.') != string::npos) استمر;
        إذا (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") استمر;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جوڙيو("vlSymsp->__Vhier." + طريقو + "(0, " + id + ");");
    }
    لكل (ثابت خودڪار& itpair : m_vpiScopeHierarchy) {
        ثابت std::string fromName = scopeSymString(itpair.پهريون);
        ثابت std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_نالا.at(fromName).m_symName);
        لكل (ثابت std::string& name : itpair.ٻيو) {
            ثابت std::string منزل_Name = scopeSymString(name);
            ثابت std::string منزل_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_نالا.at(منزل_Name).m_symName);
            stmts.جوڙيو("vlSymsp->__Vhier." + طريقو + "(" + fromId + ", " + منزل_Id + ");");
        }
    }

    إذا (destroy) {
        stmts.جوڙيو("// Clear keys from hierarchy map after values have been removed");
        stmts.جوڙيو("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::متن_Stmts EmitCSyms::getSymCtorStmts() {
    متن_Stmts stmts;

    {
        uint64_t ڍير_سائيز = V3_ڍير_ڳڻپ::ڳڻپ(v3Global.rootp());
        إذا (v3Global.opt.debug_ڍير_Check()) ڍير_سائيز += 1024 * 1024 * 1024;
        V3Stats::جوڙيو_Stat("Size prediction, Stack (bytes)", ڍير_سائيز);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::جوڙيو_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_بائيٽون);
        V3Stats::جوڙيو_Stat(V3Stats::STAT_MODEL_سائيز, ڍير_سائيز + m_stat_Var_Scope_بائيٽون);

        stmts.جوڙيو("// Check resources");
        stmts.جوڙيو("Verilated::stackCheck(" + std::منزل_string(ڍير_سائيز) + ");");
    }

    stmts.جوڙيو("// Setup sub module instances");
    لكل (ثابت ScopeModPair& itpair : m_scopes) {
        ثابت AstScope* ثابت scopep = itpair.پهريون;
        ثابت Ast_ڳنڍ_Module* ثابت modp = itpair.ٻيو;
        إذا (modp->is_چوٽي()) استمر;
        ثابت std::string name = V3_ٻاهريون_Formatter::quoteNameControls(
            VIdProtect::protect_لفظ_If(scopep->prettyName(), scopep->protect()));
        stmts.جوڙيو("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    إذا (v3Global.opt.profPgo()) {
        stmts.جوڙيو("// Configure profiling for PGO\n");
        إذا (!v3Global.opt.hier_ٻار()) {
            stmts.جوڙيو("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        إذا (v3Global.opt.mtasks()) {
            v3Global.rootp()->چوٽي_Modulep()->foreach([&](ثابت Ast_Exec_گراف* execGraphp) {
                لكل (ثابت V3_گراف_راس& vtx : execGraphp->depGraphp()->vertices()) {
                    ثابت ExecMTask& mt = static_cast<ثابت ExecMTask&>(vtx);
                    stmts.جوڙيو("vlSymsp->_vm_pgoProfiler.addCounter(" + std::منزل_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.جوڙيو("// Configure time unit / time precision");
    إذا (!v3Global.rootp()->timeunit().isNone()) {
        ثابت std::string unit = std::منزل_string(v3Global.rootp()->timeunit().power_جو_Ten());
        stmts.جوڙيو("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    إذا (!v3Global.rootp()->timeprecision().isNone()) {
        ثابت std::string prec = std::منزل_string(v3Global.rootp()->timeprecision().power_جو_Ten());
        stmts.جوڙيو("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.جوڙيو("// Setup each module's pointers to their submodules");
    لكل (ثابت خودڪار& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.پهريون;
        ثابت Ast_ڳنڍ_Module* ثابت modp = i.ٻيو;
        ثابت AstScope* ثابت abovep = scopep->aboveScopep();
        إذا (!abovep) استمر;

        ثابت std::string protName = VIdProtect::protect_لفظ_If(scopep->name(), scopep->protect());
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
        stmts.جوڙيو(stmt);
    }

    إذا (v3Global.opt.coverage()) {
        stmts.جوڙيو("// Setup each module's pointer back to symbol table (for public functions)");
        لكل (ثابت ScopeModPair& i : m_scopes) {
            ثابت AstScope* ثابت scopep = i.پهريون;
            Ast_ڳنڍ_Module* ثابت modp = i.ٻيو;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ثابت منطقي پهريون = !modp->user1();
            modp->user1(سچ);
            stmts.جوڙيو("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (پهريون ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    لكل (ثابت ScopeModPair& i : m_scopes) {
        ثابت AstScope* ثابت scopep = i.پهريون;
        ثابت Ast_ڳنڍ_Module* ثابت modp = i.ٻيو;
        إذا (!modp->verilatorLib()) استمر;
        ثابت AstVar* hashp = خالي_اشارو;
        ثابت AstVar* handlep = خالي_اشارو;
        لكل (ثابت Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (!varp) استمر;
            إذا (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            إذا (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ثابت uint32_t hash = VN_AS(hashp->valuep(), Const)->منزل_UInt();
        stmts.جوڙيو(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::منزل_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string رستو = scopep->prettyName();
        إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            رستو.erase(0, v3Global.rootp()->trace_Lib_چوٽي_Name().سائيز() + 1);
        }
        stmts.جوڙيو("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_ٻاهريون_Formatter::quoteNameControls(رستو) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    إذا (!m_scope_نالا.خالي_جايون()) {
        stmts.جوڙيو("// Setup scopes");
        ثابت std::string symClass = symClassName();
        لكل (ثابت خودڪار& itpair : m_scope_نالا) {
            ثابت Scope_معلومات& sd = itpair.ٻيو;
            std::string صف
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            صف += V3_ٻاهريون_Formatter::quoteNameControls(
                VIdProtect::protect_لفظ_If(sd.m_prettyName, سچ));
            صف += "\", \"";
            صف += V3_ٻاهريون_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            صف += "\", \"";
            صف += V3_ٻاهريون_Formatter::quoteNameControls(sd.m_defName);
            صف += "\", ";
            صف += std::منزل_string(sd.m_timeunit);
            صف += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_جدول_صفون.emplace_پٺ(std::move(صف));
        }
        m_scope_جدول_Name = symClass + "__VpiScopeTable";
        stmts.جوڙيو("VerilatedScope::scopesConstructFromTable(" + m_scope_جدول_Name + ", "
                  + std::منزل_string(m_scope_نالا.سائيز()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_جدول();
    إذا (!m_iface_Ref_جدول_صفون.خالي_جايون()) {
        stmts.جوڙيو("// Setup interface references");
        stmts.جوڙيو("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::منزل_string(m_iface_Ref_جدول_صفون.سائيز()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, خطأ);

    إذا (v3Global.dpi()) {
        لكل (ثابت std::string vfinal : {"0", "1"}) {
            stmts.جوڙيو("// Setup export functions - final: " + vfinal);
            لكل (ثابت خودڪار& itpair : m_scopeFuncs) {
                ثابت Scope_Func_معلومات& sfd = itpair.ٻيو;
                ثابت AstScopeName* ثابت scopep = sfd.m_scopep;
                ثابت AstCFunc* ثابت funcp = sfd.m_cfuncp;
                ثابت Ast_ڳنڍ_Module* ثابت modp = sfd.m_modp;
                إذا (!funcp->dpiExportImpl()) استمر;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_ٻاهريون_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::اڳياڙي_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.جوڙيو(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    إذا (!m_scopeVars.خالي_جايون()) {
        stmts.جوڙيو("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> جدول_By_صفون;
        صحيح جدول_Counter = 0;

        خودڪار it = m_scopeVars.cbegin();
        طالما (it != m_scopeVars.cend()) {
            ثابت std::string scopeName = it->ٻيو.m_scopeName;
            ثابت AstScope* ثابت instScopep = it->ٻيو.m_scopep;
            ثابت Ast_ڳنڍ_Module* ثابت modp = it->ٻيو.m_modp;
            ثابت std::string modClassName = EmitCUtil::اڳياڙي_Name_Protect(modp);

            std::ويڪٽر<std::string> صفون;
            std::ويڪٽر<std::string> residual;

            لكل (; it != m_scopeVars.cend() && it->ٻيو.m_scopeName == scopeName; ++it) {
                ثابت Scope_Var_معلومات& svd = it->ٻيو;
                ثابت AstScope* ثابت scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ثابت AstVar* ثابت varp = svd.m_varp;
                ثابت VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ثابت std::pair<منطقي, std::string> fc = isForceControlSignal(varp);
                إذا (fc.پهريون && بنياد_Signal_Is_عوامي(scopep, fc.ٻيو)
                    && بنياد_Signal_Is_Valid(scopep, varp, fc.ٻيو)) {
                    استمر;
                }

                std::string صف;
                ثابت جدول_داخلا_قسم قسم
                    = try_Build_جدول_داخلا(svd, varp, scopep, modClassName, dims, صف);
                بدل (قسم) {
                حالة جدول_داخلا_قسم::جدول_صف:
                    صفون.emplace_پٺ(صف);
                    جوڙيو_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_بنياد_Pretty,
                                           varName(varp), varp->dtypep());
                    اكسر;
                حالة جدول_داخلا_قسم::FORCEABLE_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_پٺ(وجهو_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    اكسر;
                }
                حالة جدول_داخلا_قسم::PLAIN_RESIDUAL: {
                    ثابت std::string bounds = boundsString(dims);
                    residual.emplace_پٺ(
                        وجهو_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    اكسر;
                }
                افتراضي: v3fatalSrc("Bad case");
                }
                إذا (قسم == جدول_داخلا_قسم::جدول_صف) استمر;
                إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ڳنڍ_UOr_Struct_DType)) {
                    إذا (!sdtypep->packed()) {
                        جوڙيو_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_بنياد_Pretty,
                                               varName(varp), sdtypep);
                    }
                } وإلا إذا (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    جوڙيو_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_بنياد_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                إذا (قسم == جدول_داخلا_قسم::PLAIN_RESIDUAL) {
                    جوڙيو_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_بنياد_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            إذا (!صفون.خالي_جايون()) {
                ثابت سائيز_t صف_ڳڻپ = صفون.سائيز();
                std::string ڪنجي;
                لكل (ثابت std::string& r : صفون) {
                    ڪنجي += r;
                    ڪنجي += '\0';
                }
                ثابت خودڪار itt = جدول_By_صفون.ڳوليو(ڪنجي);
                std::string جدول_Name;
                إذا (itt != جدول_By_صفون.پڇاڙي()) {
                    جدول_Name = itt->ٻيو;
                } وإلا {
                    جدول_Name = modClassName + "__VpiVarTable" + std::منزل_string(جدول_Counter++);
                    جدول_By_صفون.emplace(std::move(ڪنجي), جدول_Name);
                    m_varTables.emplace_پٺ(جدول_Name, std::move(صفون));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + جدول_Name + ", "
                                   + std::منزل_string(صف_ڳڻپ) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.جوڙيو(call);
            }
            لكل (ثابت std::string& r : residual) stmts.جوڙيو(r);
        }
    }

    أرجع stmts;
}

EmitCSyms::متن_Stmts EmitCSyms::getSymDtorStmts() {
    متن_Stmts stmts;

    emitScopeHier(stmts, سچ);
    إذا (v3Global.needTraceDumper()) {
        stmts.جوڙيو("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    إذا (v3Global.opt.profPgo()) {
        stmts.جوڙيو("vlSymsp->_vm_pgoProfiler.write(\"" + چوٽي_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    إذا (!m_iface_Ref_جدول_صفون.خالي_جايون()) {
        stmts.جوڙيو("// Tear down interface references");
        stmts.جوڙيو("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_جدول_Name + ", "
                  + std::منزل_string(m_iface_Ref_جدول_صفون.سائيز()) + ", vlSymsp);");
    }

    stmts.جوڙيو("// Tear down scopes");
    لكل (ثابت خودڪار& itpair : m_scope_نالا) {
        ثابت Scope_معلومات& sd = itpair.ٻيو;
        ثابت std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.جوڙيو("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.جوڙيو("// Tear down sub module instances");
    لكل (ثابت ScopeModPair& itpair : vlstd::ابتو_view(m_scopes)) {
        ثابت AstScope* ثابت scopep = itpair.پهريون;
        ثابت Ast_ڳنڍ_Module* ثابت modp = itpair.ٻيو;
        إذا (modp->is_چوٽي()) استمر;
        stmts.جوڙيو("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    أرجع stmts;
}

فراغ EmitCSyms::emitSymImp(ثابت AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    متن_Stmts ctorStmts = getSymCtorStmts();
    متن_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ثابت std::pair<std::string, std::string> ورهايو_Ctor_Dtor_Decls  //
        = emit_ورهايو_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            سچ, "Symbol table implementation internals",
                            [هذا]() { emitSymImpPreamble(); });

    open_New_Output_ذريعو_File(symClassName(), سچ, سچ, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ثابت AstScope* ثابت چوٽي_Scopep = netlistp->چوٽي_Scopep()->scopep();
    ثابت std::string چوٽي_Inst = "vlSymsp->" + instName(چوٽي_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    إذا (!ورهايو_Ctor_Dtor_Decls.پهريون.خالي_جايون()) puts("\n" + ورهايو_Ctor_Dtor_Decls.پهريون + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + چوٽي_Inst + ", vlSymsp, namep);\n");
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + چوٽي_Class_Name() + "* modelp)\n");
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
    إذا (v3Global.opt.profPgo() && !v3Global.opt.libCreate().خالي_جايون()) {
        puts("    , _vm_pgoProfiler{" + std::منزل_string(v3Global.current_Hier_بلاڪ_لاڳت()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    إذا (!ورهايو_Ctor_Dtor_Decls.ٻيو.خالي_جايون()) puts("\n" + ورهايو_Ctor_Dtor_Decls.ٻيو + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_متن);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + چوٽي_Inst + ");\n");
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
        لكل (ثابت منطقي& de : {خطأ, سچ}) {
            ثابت std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ثابت std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ثابت std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            إذا (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            لكل (ثابت ScopeModPair& itpair : m_scopes) {
                ثابت AstScope* ثابت scopep = itpair.پهريون;
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

    open_New_Output_سرو_File(چوٽي_Class_Name() + "__Dpi",
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
    std::سيٽ<std::string> struct_نالا;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        لكل (ثابت Ast_ڳنڍ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_نالا);
            }
        }
    }
    إذا (!struct_نالا.خالي_جايون()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    صحيح پهريون_Exp = 0;
    صحيح پهريون_Imp = 0;
    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) استمر;

        ثابت std::string ذريعو_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        إذا (nodep->dpiExportDispatcher()) {
            إذا (!پهريون_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + ذريعو_Loc + "\n");
        } وإلا {
            إذا (!پهريون_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + ذريعو_Loc + "\n");
        }
        إذا (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } وإلا {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_دليلون(nodep, خطأ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_پڇاڙي_Guard();

    closeOutputFile();
}

فراغ EmitCSyms::emitDpiStruct(ثابت Ast_ڳنڍ_DType* dtypep, std::سيٽ<std::string>& namesr) {
    إذا (!dtypep->شامل_آهي_Unpacked_Struct()) أرجع;
    ثابت AstStructDType* ثابت sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ثابت std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    إذا (namesr.ڳڻپ(name)) أرجع;
    // After the structures it contains
    لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    إذا (name.خالي_جايون()) أرجع;  // Declared where used
    إذا (namesr.خالي_جايون()) puts("// DPI TYPES\n");
    namesr.وجهو(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

فراغ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ذريعو_File(چوٽي_Class_Name() + "__Dpi", خطأ, سچ,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + چوٽي_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + چوٽي_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + چوٽي_Class_Name() + ".h\"\n");
    puts("\n");

    لكل (ثابت AstCFunc* ثابت nodep : m_dpis) {
        إذا (!nodep->dpiExportDispatcher()) استمر;

        ثابت std::string name = nodep->name();
        ثابت std::string ذريعو_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_دليلون(nodep) + ") {\n");
        puts("// DPI export" + ذريعو_Loc + "\n");
        putns(nodep, "return " + چوٽي_Class_Name() + "::" + name + "(");
        std::string comma;
        لكل (ثابت Ast_ڳنڍ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
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
