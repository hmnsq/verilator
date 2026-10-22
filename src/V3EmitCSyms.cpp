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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitC.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LanguageWords.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3StackCount.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstring>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
ਸਥਿਰ constexpr ਸਵੈਚਾਲਿਤ symClassName = &EmitCUtil::symClassName;
ਸਥਿਰ constexpr ਸਵੈਚਾਲਿਤ ਸਿਖਰ_Class_Name = &EmitCUtil::ਸਿਖਰ_Class_Name;

ਵਰਗ EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // TYPES
    ਬਣਤਰ Scope_ਅੰਕੜੇ final {
        ਅਚਲ Ast_ਗੰਢ* m_nodep;  // Scope symbol-table entry corresponds to
        ਅਚਲ std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        ਅਚਲ std::string m_prettyName;  // Pretty (unprotected) name for display
        ਅਚਲ std::string m_defName;  // Name registered for this scope
        ਅਚਲ ਪੂਰਨ m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        Scope_ਅੰਕੜੇ(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ std::string& symName, ਅਚਲ std::string& prettyName,
                  ਅਚਲ std::string& defName, ਪੂਰਨ timeunit, ਅਚਲ std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    ਬਣਤਰ Iface_Ref_ਅੰਕੜੇ final {
        ਅਚਲ AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_ਮਾਪਾ_Sym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        Iface_Ref_ਅੰਕੜੇ(ਅਚਲ AstScope* scopep, ਅਚਲ std::string& ਮਾਪਾ_Sym,
                     ਅਚਲ std::string& suffix, ਅਚਲ std::string& name,
                     ਅਚਲ std::string& modportName)
            : m_scopep{scopep}
            , m_ਮਾਪਾ_Sym{ਮਾਪਾ_Sym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    ਬਣਤਰ Scope_Func_ਅੰਕੜੇ final {
        ਅਚਲ AstScopeName* ਅਚਲ m_scopep;  // Scope the DPI export function is registered under
        ਅਚਲ AstCFunc* ਅਚਲ m_cfuncp;  // DPI export function
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module containing the DPI export function
        Scope_Func_ਅੰਕੜੇ(ਅਚਲ AstScopeName* scopep, ਅਚਲ AstCFunc* funcp, ਅਚਲ Ast_ਗੰਢ_Module* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    ਬਣਤਰ Scope_Var_ਅੰਕੜੇ final {
        ਅਚਲ std::string m_scopeName;  // Full name of the scope containing the variable
        ਅਚਲ std::string m_var_ਅਧਾਰ_Pretty;  // Pretty base name of the variable, without scope
        ਅਚਲ AstVar* ਅਚਲ m_varp;  // Public variable
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module containing the variable
        ਅਚਲ AstScope* ਅਚਲ m_scopep;  // Scope containing the variable
        Scope_Var_ਅੰਕੜੇ(ਅਚਲ std::string& scopeName, ਅਚਲ std::string& var_ਅਧਾਰ_Pretty,
                     ਅਚਲ AstVar* varp, ਅਚਲ Ast_ਗੰਢ_Module* modp, ਅਚਲ AstScope* scopep)
            : m_scopeName{scopeName}
            , m_var_ਅਧਾਰ_Pretty{var_ਅਧਾਰ_Pretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    ਵਰਤੋਂ Scope_ਨਾਂ = std::map<ਅਚਲ std::string, Scope_ਅੰਕੜੇ>;
    ਵਰਤੋਂ ScopeModPair = std::pair<ਅਚਲ AstScope*, Ast_ਗੰਢ_Module*>;
    ਵਰਤੋਂ ModVarPair = std::pair<ਅਚਲ Ast_ਗੰਢ_Module*, ਅਚਲ AstVar*>;

    // Vars with more dims take the residual per-statement path.
    ਸਥਿਰ constexpr ਪੂਰਨ VPI_ਸਾਰਣੀ_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current function
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    std::ਸਦਿਸ਼<ScopeModPair> m_scopes;  // Every scope by module
    std::ਸਦਿਸ਼<AstCFunc*> m_dpis;  // DPI functions
    std::ਸਦਿਸ਼<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<ਅਚਲ std::string, Scope_Func_ਅੰਕੜੇ> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<ਅਚਲ std::string, Scope_Var_ਅੰਕੜੇ> m_scopeVars;  // Each {scope,public-var}
    Scope_ਨਾਂ m_scope_ਨਾਂ;  // Each unique AstScopeName. Dpi scopes added later
    Scope_ਨਾਂ m_dpi_Scope_ਨਾਂ;  // Each unique AstScopeName for DPI export
    Scope_ਨਾਂ m_vpiScopeCandidates;  // All scopes for VPI
    std::ਸਦਿਸ਼<Iface_Ref_ਅੰਕੜੇ> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<ਅਚਲ std::string, std::ਸਦਿਸ਼<std::string>> m_vpiScopeHierarchy;
    ਪੂਰਨ m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    ਪੂਰਨ m_modCoverBins = 0;  // Per-module coverage bin number
    ਅਚਲ ਤਾਰਕਿਕ m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_stat_Var_Scope_ਬਾਈਟਾਂ;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::ਸਦਿਸ਼<std::pair<std::string, std::ਸਦਿਸ਼<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scope_ਸਾਰਣੀ_Name;
    std::ਸਦਿਸ਼<std::string> m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ;
    std::string m_iface_Ref_ਸਾਰਣੀ_Name;
    std::ਸਦਿਸ਼<std::string> m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ;

    // METHODS
    ਖਾਲੀ emitSymHdr();
    ਖਾਲੀ emitSymImpPreamble();
    ਖਾਲੀ emitVarTables();
    ਖਾਲੀ emitScopeHier(ਪਾਠ_Stmts& stmts, ਤਾਰਕਿਕ destroy);
    ਖਾਲੀ emitSymImp(ਅਚਲ AstNetlist* netlistp);
    ਖਾਲੀ emitDpiHdr();
    ਖਾਲੀ emitDpiStruct(ਅਚਲ Ast_ਗੰਢ_DType* dtypep, std::ਸੈੱਟ<std::string>& namesr);
    ਖਾਲੀ emitDpiImp();

    ਪਾਠ_Stmts getSymCtorStmts();
    ਪਾਠ_Stmts getSymDtorStmts();

    ਸਥਿਰ ਖਾਲੀ nameCheck(Ast_ਗੰਢ* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        ਜੇ (nodep->name().ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਅਚਲ AstCFunc* ਅਚਲ cfuncp = VN_CAST(nodep, CFunc);
        ਜੇ (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            ਅਚਲ std::string rsvd = V3_Language_ਸ਼ਬਦ::isKeyword(nodep->name());
            ਜੇ (rsvd != "") {
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

    ਸਥਿਰ std::string scopeSymString(ਅਚਲ std::string& scpname) {
        std::string ਬਾਹਰਲਾ = scpname;
        std::string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = ਬਾਹਰਲਾ.ਲੱਭੋ("__PVT__")) != std::string::npos) ਬਾਹਰਲਾ.replace(pos, 7, "");
        ਜੇ (VString::startsWith(ਬਾਹਰਲਾ, "TOP__DOT__")) ਬਾਹਰਲਾ.replace(0, 10, "");
        ਜੇ (VString::startsWith(ਬਾਹਰਲਾ, "TOP.")) ਬਾਹਰਲਾ.replace(0, 4, "");
        ਜਦੋਂ ((pos = ਬਾਹਰਲਾ.ਲੱਭੋ('.')) != std::string::npos) ਬਾਹਰਲਾ.replace(pos, 1, "__");
        ਜਦੋਂ ((pos = ਬਾਹਰਲਾ.ਲੱਭੋ("__DOT__")) != std::string::npos) ਬਾਹਰਲਾ.replace(pos, 7, "__");
        ਵਾਪਸ ਬਾਹਰਲਾ;
    }

    ਸਥਿਰ string scopeDecodeIdentifier(ਅਚਲ std::string& scpname) {
        std::string::ਮਾਪ_type pos = std::string::npos;

        // Remove hierarchy
        ਮਾਪ_t i = 0;
        // always makes progress
        ਜਦੋਂ (i < scpname.ਲੰਮਾਈ()) {
            ਜੇ (scpname[i] == '\\') {
                ਜਦੋਂ (i < scpname.ਲੰਮਾਈ() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } ਨਹੀਂਤਾਂ {
                ਜਦੋਂ (i < scpname.ਲੰਮਾਈ() && scpname[i] != '.') ++i;
                ਜੇ (i < scpname.ਲੰਮਾਈ()) pos = i++;
            }
        }

        ਵਾਪਸ pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    ਬਣਤਰ VarDims final {
        ਪੂਰਨ pdim = 0;  // Packed array dims plus a ranged basic leaf
        ਪੂਰਨ udim = 0;
        std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> unpackedLR;  // (left,right), outer-first
        std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> flatten() ਅਚਲ {
            std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> flat(unpackedLR);
            flat.ਪਾਓ(flat.ਸਿਰਾ(), packedLR.cbegin(), packedLR.cend());
            ਵਾਪਸ flat;
        }
    };
    ਸਥਿਰ VarDims getVarDims(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਜੜ੍ਹ_Dtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_DType* dtypep = ਜੜ੍ਹ_Dtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(dtypep, ਗੰਢ_Array_DType)) {
                ਜੇ (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_ਪਿੱਠ(adtypep->ਖੱਬੇ(), adtypep->ਸੱਜੇ());
                    ++d.pdim;
                } ਨਹੀਂਤਾਂ {
                    d.unpackedLR.emplace_ਪਿੱਠ(adtypep->ਖੱਬੇ(), adtypep->ਸੱਜੇ());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } ਨਹੀਂਤਾਂ {
                ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp()) {
                    ਜੇ (basicp->isRanged()) {
                        d.packedLR.emplace_ਪਿੱਠ(basicp->ਖੱਬੇ(), basicp->ਸੱਜੇ());
                        ++d.pdim;
                    }
                }
                ਰੋਕੋ;  // Non-array leaf
            }
        }
        ਵਾਪਸ d;
    }

    ਸਥਿਰ VarDims dimsFor(ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd) { ਵਾਪਸ getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    ਸਥਿਰ std::string boundsString(ਅਚਲ VarDims& d) {
        std::string bounds;
        ਲਈ (ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::ਮੰਜ਼ਿਲ_string(lr.ਪਹਿਲਾ);
            bounds += ",";
            bounds += std::ਮੰਜ਼ਿਲ_string(lr.ਦੂਜਾ);
        }
        ਵਾਪਸ bounds;
    }

    ਸਥਿਰ ਪੂਰਨ getUnpackedElements(ਅਚਲ Ast_ਗੰਢ_DType* ਜੜ੍ਹ_Dtypep) {
        ਪੂਰਨ elements = 1;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ_DType* dtypep = ਜੜ੍ਹ_Dtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(dtypep, UnpackArrayDType);
            ਜੇ (!adtypep) ਰੋਕੋ;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        ਵਾਪਸ elements;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ needs_Emitted_Ent_ਮਾਪ(ਅਚਲ std::string& vlEnumType) {
        ਵਾਪਸ vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    ਸਥਿਰ std::pair<ਤਾਰਕਿਕ, std::string> isForceControlSignal(ਅਚਲ AstVar* ਅਚਲ signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        ਲਈ (ਅਚਲ std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            ਅਚਲ std::ਮਾਪ_t suffixPos = signalVarp->name().ਲੱਭੋ(forceControlSuffix);
            ਅਚਲ ਤਾਰਕਿਕ suffix_ਮਿਲਿਆ = suffixPos != std::string::npos;
            ਅਚਲ ਤਾਰਕਿਕ suffix_Is_At_ਸਿਰਾ
                = suffixPos + forceControlSuffix.ਲੰਮਾਈ() == signalVarp->name().ਲੰਮਾਈ();
            ਜੇ (!(suffix_ਮਿਲਿਆ && suffix_Is_At_ਸਿਰਾ)) ਜਾਰੀ;
            ਅਚਲ std::string ਅਧਾਰ_Signal_Name = signalVarp->name().substr(0, suffixPos);
            ਵਾਪਸ std::pair<ਤਾਰਕਿਕ, std::string>{ਸੱਚ, ਅਧਾਰ_Signal_Name};
        }
        ਵਾਪਸ std::pair<ਤਾਰਕਿਕ, std::string>{ਝੂਠ, ""};
    }

    ਸਥਿਰ ਖਾਲੀ appendVarProperties(std::string& stmt, ਅਚਲ std::string& vlEnumType,
                                    ਅਚਲ std::string& vlEnumDir, ਅਚਲ ਪੂਰਨ udim, ਅਚਲ ਪੂਰਨ pdim,
                                    ਅਚਲ std::string& bounds, ਅਚਲ std::string& ent_ਮਾਪ = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::ਮੰਜ਼ਿਲ_string(udim);
        ਜੇ (!ent_ਮਾਪ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            stmt += ", ";
            stmt += ent_ਮਾਪ;
        } ਨਹੀਂਤਾਂ {
            stmt += ", ";
            stmt += std::ਮੰਜ਼ਿਲ_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    ਸਥਿਰ std::string memberVlEnumDir(ਅਚਲ AstVar* ਅਚਲ varp,
                                       ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        std::string ਬਾਹਰਲਾ = '(' + varp->vlEnumDir(/*forMember=*/ਸੱਚ);
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ skipDTypep = dtypep->skipRefp();
        ਜੇ (skipDTypep->isSigned()) ਬਾਹਰਲਾ += "|VLVF_SIGNED";
        ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp = skipDTypep->basicp()) {
            ਜੇ (basicp->keyword() == VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ) ਬਾਹਰਲਾ += "|VLVF_BITVAR";
        }
        ਬਾਹਰਲਾ += skipDTypep->vlEnumPackedAgg();
        ਵਾਪਸ ਬਾਹਰਲਾ + ')';
    }

    // Name of the instance member for the given scope
    ਸਥਿਰ std::string instName(ਅਚਲ AstScope* scopep) {
        ਵਾਪਸ "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    ਸਥਿਰ std::string ਪਾਓ_Var_Statement(ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd, ਅਚਲ AstScope* ਅਚਲ scopep,
                                          ਅਚਲ AstVar* ਅਚਲ varp, ਅਚਲ ਪੂਰਨ udim, ਅਚਲ ਪੂਰਨ pdim,
                                          ਅਚਲ std::string& bounds) {
        ਅਚਲ std::string var_ਵਿਅੰਜਕ = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        ਅਚਲ std::string vlEnumType = varp->vlEnumType();
        ਅਚਲ ਤਾਰਕਿਕ needs_Ent_ਮਾਪ = needs_Emitted_Ent_ਮਾਪ(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_ਮਾਪ ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(protect(svd.m_var_ਅਧਾਰ_Pretty)) + '"';

        ਜੇ (!varp->isParam()) {
            stmt += ", &(";
            stmt += var_ਵਿਅੰਜਕ;
            stmt += "), false, ";
        } ਨਹੀਂਤਾਂ ਜੇ (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += var_ਵਿਅੰਜਕ;
            stmt += ".c_str())), true, ";
        } ਨਹੀਂਤਾਂ {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ਵਿਅੰਜਕ;
            stmt += "))), true, ";
        }

        ਅਚਲ std::string ent_ਮਾਪ = needs_Ent_ਮਾਪ
                                        ? "sizeof(" + var_ਵਿਅੰਜਕ + ") / "
                                              + std::ਮੰਜ਼ਿਲ_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, ent_ਮਾਪ);
        ਵਾਪਸ stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    ਗਿਣਤੀ ਵਰਗ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ : uint8_t {
        ਸਾਰਣੀ_ਪੰਕਤੀ,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ try_Build_ਸਾਰਣੀ_ਇੰਦਰਾਜ(ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd, ਅਚਲ AstVar* ਅਚਲ varp,
                                      ਅਚਲ AstScope* ਅਚਲ scopep,
                                      ਅਚਲ std::string& modClassName, ਅਚਲ VarDims& dims,
                                      std::string& ਪੰਕਤੀ_ਬਾਹਰਲਾ) ਅਚਲ {
        ਅਚਲ ਪੂਰਨ pdim = dims.pdim;
        ਅਚਲ ਪੂਰਨ udim = dims.udim;
        ਜੇ (varp->isForceable() && force_Control_ਸੰਕੇਤ_Are_Valid(scopep, varp)) {
            ਵਾਪਸ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::FORCEABLE_RESIDUAL;
        }
        ਜੇ (udim + pdim > VPI_ਸਾਰਣੀ_MAX_DIMS) ਵਾਪਸ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::PLAIN_RESIDUAL;

        ਅਚਲ std::string vlEnumType = varp->vlEnumType();
        ਜੇ (needs_Emitted_Ent_ਮਾਪ(vlEnumType))
            ਵਾਪਸ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        ਜੇ (varp->isParam()) ਵਾਪਸ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::PLAIN_RESIDUAL;

        ਅਚਲ std::string name = V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(protect(svd.m_var_ਅਧਾਰ_Pretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        ਅਚਲ std::string member = varName(varp);
        ਅਚਲ std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::ਸਦਿਸ਼<ਪੂਰਨ> dv;
        ਲਈ (ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ>& lr : dims.flatten()) {
            dv.ਧੱਕੋ_ਪਿੱਠ(lr.ਪਹਿਲਾ);
            dv.ਧੱਕੋ_ਪਿੱਠ(lr.ਦੂਜਾ);
        }

        std::string ਪੰਕਤੀ = "{\"" + name + "\", ";
        ਪੰਕਤੀ += "offsetof(" + modClassName + ", " + member + "), ";
        ਪੰਕਤੀ += vlEnumType + ", ";
        ਪੰਕਤੀ += "(" + dir + "), ";
        ਪੰਕਤੀ += std::ਮੰਜ਼ਿਲ_string(udim) + ", " + std::ਮੰਜ਼ਿਲ_string(pdim) + ", {";
        ਲਈ (ਪੂਰਨ i = 0; i < VPI_ਸਾਰਣੀ_MAX_DIMS * 2; ++i) {
            ਜੇ (i) ਪੰਕਤੀ += ", ";
            ਪੰਕਤੀ += std::ਮੰਜ਼ਿਲ_string(i < static_cast<ਪੂਰਨ>(dv.ਮਾਪ()) ? dv[i] : 0);
        }
        ਪੰਕਤੀ += "}}";
        ਪੰਕਤੀ_ਬਾਹਰਲਾ = ਪੰਕਤੀ;
        ਵਾਪਸ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::ਸਾਰਣੀ_ਪੰਕਤੀ;
    }

    ਸਥਿਰ std::string ਪਾਓ_DType_Var_Statement(ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd,
                                               ਅਚਲ AstScope* ਅਚਲ scopep,
                                               ਅਚਲ std::string& prettyName,
                                               ਅਚਲ std::string& cName,
                                               ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep, ਅਚਲ ਪੂਰਨ udim,
                                               ਅਚਲ ਪੂਰਨ pdim, ਅਚਲ std::string& bounds) {
        ਅਚਲ std::string vlEnumType = dtypep->vlEnumType();
        ਅਚਲ ਤਾਰਕਿਕ needs_Ent_ਮਾਪ = needs_Emitted_Ent_ਮਾਪ(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needs_Ent_ਮਾਪ ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(prettyName) + '"';
        ਅਚਲ std::string var_ਵਿਅੰਜਕ = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        ਜੇ (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += var_ਵਿਅੰਜਕ;
            stmt += "))), true, ";
        } ਨਹੀਂਤਾਂ {
            stmt += ", &(";
            stmt += var_ਵਿਅੰਜਕ;
            stmt += "), false, ";
        }
        ਅਚਲ std::string ent_ਮਾਪ
            = needs_Ent_ਮਾਪ
                  ? "sizeof(" + var_ਵਿਅੰਜਕ + ") / " + std::ਮੰਜ਼ਿਲ_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, ent_ਮਾਪ);
        ਵਾਪਸ stmt;
    }

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_UOr_Struct_Member_Vars(std::ਸਦਿਸ਼<std::string>& stmts, ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd,
                                       ਅਚਲ AstScope* ਅਚਲ scopep,
                                       ਅਚਲ std::string& pretty_ਅਗੇਤਰ, ਅਚਲ std::string& c_ਅਗੇਤਰ,
                                       ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep) {
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਵਸਤੂ_DTypep = itemp->dtypep();
            ਜੇ (ਵਸਤੂ_DTypep->vlEnumType().ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਅਚਲ std::string prettyName
                = pretty_ਅਗੇਤਰ + "." + Ast_ਗੰਢ::vpiName(itemp->shortName());
            ਅਚਲ std::string cName = c_ਅਗੇਤਰ + "." + itemp->nameProtect();
            ਅਚਲ VarDims dims = getVarDims(ਵਸਤੂ_DTypep);
            stmts.emplace_ਪਿੱਠ(ਪਾਓ_DType_Var_Statement(svd, scopep, prettyName, cName, ਵਸਤੂ_DTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            ਜੋੜੋ_Packed_Agg_Member_Vars(stmts, svd, scopep, prettyName, cName, ਵਸਤੂ_DTypep);
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ subp
                = VN_CAST(ਵਸਤੂ_DTypep->skipRefp(), ਗੰਢ_UOr_Struct_DType)) {
                ਜੇ (!subp->packed())
                    ਜੋੜੋ_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName, subp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਵਸਤੂ_DTypep->skipRefp(), UnpackArrayDType)) {
                ਜੋੜੋ_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, prettyName, cName,
                                                    ਵਸਤੂ_DTypep);
            }
        }
    }

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Unpacked_Array_UOr_Struct_Member_Vars(std::ਸਦਿਸ਼<std::string>& stmts,
                                                    ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd,
                                                    ਅਚਲ AstScope* ਅਚਲ scopep,
                                                    ਅਚਲ std::string& pretty_ਅਗੇਤਰ,
                                                    ਅਚਲ std::string& c_ਅਗੇਤਰ,
                                                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ skipDTypep = dtypep->skipRefp();
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            ਜੋੜੋ_Unpacked_Array_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_ਅਗੇਤਰ, c_ਅਗੇਤਰ + "[0]",
                                                adtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep
                   = VN_CAST(skipDTypep, ਗੰਢ_UOr_Struct_DType)) {
            ਜੇ (!sdtypep->packed())
                ਜੋੜੋ_UOr_Struct_Member_Vars(stmts, svd, scopep, pretty_ਅਗੇਤਰ, c_ਅਗੇਤਰ, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Packed_Member_Vars(std::ਸਦਿਸ਼<std::string>& stmts, ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd,
                                    ਅਚਲ AstScope* ਅਚਲ scopep, ਅਚਲ std::string& pretty_ਅਗੇਤਰ,
                                    ਅਚਲ std::string& cName, ਅਚਲ std::string& vlEnumType,
                                    ਅਚਲ std::string& ent_ਮਾਪ,
                                    ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep,
                                    ਅਚਲ ਪੂਰਨ ਮਾਪਾ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ, ਅਚਲ ਤਾਰਕਿਕ needs_ਸੂਚਕਾਂਕ) {
        ਅਚਲ std::string var_ਵਿਅੰਜਕ = "vlSymsp->" + instName(scopep) + "." + cName;
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਵਸਤੂ_DTypep = itemp->dtypep();
            ਅਚਲ std::string prettyName
                = pretty_ਅਗੇਤਰ + "." + Ast_ਗੰਢ::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            ਅਚਲ ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ = ਮਾਪਾ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ + itemp->lsb();
            ਅਚਲ VarDims dims = getVarDims(ਵਸਤੂ_DTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            ਜੇ (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + var_ਵਿਅੰਜਕ + "))), true, ";
            } ਨਹੀਂਤਾਂ {
                stmt += ", &(" + var_ਵਿਅੰਜਕ + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, ਵਸਤੂ_DTypep);
            ਜੇ (needs_ਸੂਚਕਾਂਕ) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::ਮੰਜ਼ਿਲ_string(ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ) + ", ";
            stmt += ent_ਮਾਪ + ", ";
            stmt += std::ਮੰਜ਼ਿਲ_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_ਪਿੱਠ(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            ਅਚਲ Ast_ਗੰਢ_DType* subDTypep = ਵਸਤੂ_DTypep->skipRefp();
            ਤਾਰਕਿਕ isArray = ਝੂਠ;
            ਜਦੋਂ (ਅਚਲ AstPackArrayDType* ਅਚਲ adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = ਸੱਚ;
            }
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ subp = VN_CAST(subDTypep, ਗੰਢ_UOr_Struct_DType)) {
                ਜੋੜੋ_Packed_Member_Vars(stmts, svd, scopep, prettyName, cName, vlEnumType, ent_ਮਾਪ,
                                    subp, ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ, needs_ਸੂਚਕਾਂਕ || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Packed_Agg_Member_Vars(std::ਸਦਿਸ਼<std::string>& stmts, ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd,
                                       ਅਚਲ AstScope* ਅਚਲ scopep,
                                       ਅਚਲ std::string& pretty_ਅਗੇਤਰ, ਅਚਲ std::string& cName,
                                       ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        std::string elemCName = cName;
        ਅਚਲ Ast_ਗੰਢ_DType* elemDTypep = dtypep->skipRefp();
        ਤਾਰਕਿਕ isArray = ਝੂਠ;
        ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(elemDTypep, ਗੰਢ_Array_DType)) {
            ਜੇ (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = ਸੱਚ;
        }
        ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep = VN_CAST(elemDTypep, ਗੰਢ_UOr_Struct_DType);
        ਜੇ (!sdtypep || !sdtypep->packed()) ਵਾਪਸ;
        ਅਚਲ std::string var_ਵਿਅੰਜਕ = "vlSymsp->" + instName(scopep) + "." + cName;
        ਅਚਲ std::string ent_ਮਾਪ
            = "sizeof(" + var_ਵਿਅੰਜਕ + ") / " + std::ਮੰਜ਼ਿਲ_string(getUnpackedElements(dtypep));
        ਜੋੜੋ_Packed_Member_Vars(stmts, svd, scopep, pretty_ਅਗੇਤਰ, elemCName, dtypep->vlEnumType(),
                            ent_ਮਾਪ, sdtypep, 0, isArray);
    }

    std::string ਪਾਓ_Forceable_Var_Statement(ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd, ਅਚਲ AstScope* ਅਚਲ scopep,
                                            ਅਚਲ AstVar* ਅਚਲ varp, ਅਚਲ ਪੂਰਨ udim,
                                            ਅਚਲ ਪੂਰਨ pdim, ਅਚਲ std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(protect(svd.m_var_ਅਧਾਰ_Pretty)) + '"';

        ਅਚਲ std::string var_ਵਿਅੰਜਕ = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        ਪ੍ਰਤਿਜ੍ਞਾ(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += var_ਵਿਅੰਜਕ;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += var_ਵਿਅੰਜਕ + "__VforceRd";
        stmt += "), \"" + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(protect(svd.m_var_ਅਧਾਰ_Pretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        ਲਈ (ਅਚਲ std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            ਅਚਲ std::string enable_Signal_ਕੁੰਜੀ
                = get_ਕੁੰਜੀ_Name(scopep, varp->name() + forceControlSuffix);
            ਅਚਲ std::map<ਅਚਲ std::string, Scope_Var_ਅੰਕੜੇ>::const_iterator itpair
                = m_scopeVars.ਲੱਭੋ(enable_Signal_ਕੁੰਜੀ);

            ਜੇ (itpair == m_scopeVars.ਸਿਰਾ()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enable_Signal_ਕੁੰਜੀ << "'.");
            }

            ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd = itpair->ਦੂਜਾ;
            ਅਚਲ AstScope* ਅਚਲ scopep = svd.m_scopep;
            ਅਚਲ AstVar* ਅਚਲ varp = svd.m_varp;
            ਅਚਲ VarDims dims = dimsFor(svd);
            stmt
                += ਪਾਓ_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            ਜੇ (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::ਮੰਜ਼ਿਲ_string(udim);
        stmt += ", ";
        stmt += std::ਮੰਜ਼ਿਲ_string(pdim);
        stmt += bounds;
        stmt += ")";
        ਵਾਪਸ stmt;
    }

    ਸਥਿਰ std::string get_ਕੁੰਜੀ_Name(ਅਚਲ AstScope* ਅਚਲ scopep, ਅਚਲ std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        //ਕ੍ਰਮ
        // signal can be found.
        std::string ਸਾਰਾ = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        ਜੇ (VString::startsWith(ਸਾਰਾ, "__DOT__TOP")) ਸਾਰਾ.replace(0, 10, "");
        ਅਚਲ std::string::ਮਾਪ_type dpos = ਸਾਰਾ.rfind("__DOT__");
        ਜੇ (dpos != std::string::npos) { scpName = ਸਾਰਾ.substr(0, dpos); }
        ਅਚਲ std::string scpSym = scopeSymString(VName::dehash(scpName));
        ਵਾਪਸ scpSym + " " + signal_name;
    }

    ਤਾਰਕਿਕ ਅਧਾਰ_Signal_Is_Valid(ਅਚਲ AstScope* ਅਚਲ scopep, ਅਚਲ AstVar* ਅਚਲ controlSignalVarp,
                           ਅਚਲ std::string& ਅਧਾਰ_Signal_Name) ਅਚਲ {
        ਅਚਲ std::string ਅਧਾਰ_Signal_ਕੁੰਜੀ = get_ਕੁੰਜੀ_Name(scopep, ਅਧਾਰ_Signal_Name);
        ਅਚਲ std::map<ਅਚਲ std::string, Scope_Var_ਅੰਕੜੇ>::const_iterator ਅਧਾਰ_Signal_It
            = m_scopeVars.ਲੱਭੋ(ਅਧਾਰ_Signal_ਕੁੰਜੀ);
        ਜੇ (ਅਧਾਰ_Signal_It == m_scopeVars.ਸਿਰਾ()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            ਵਾਪਸ ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ AstVar* ਅਚਲ ਅਧਾਰ_Signal_Varp = ਅਧਾਰ_Signal_It->ਦੂਜਾ.m_varp;
            ਜੇ (m_scopeVars.ਗਿਣਤੀ_2(ਅਧਾਰ_Signal_Name) > 1) {
                ਅਧਾਰ_Signal_Varp->v3fatalSrc(
                    "Found Signal " << ਅਧਾਰ_Signal_Name
                                    << ", but also found at least one other signal with name"
                                    << ਅਧਾਰ_Signal_Name << " occurring in the same scope.");
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ (!ਅਧਾਰ_Signal_Varp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << ਅਧਾਰ_Signal_Varp->prettyNameQ()
                                    << " is not marked as forceable.");
                ਵਾਪਸ ਝੂਠ;
            }
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ ਅਧਾਰ_Signal_Is_ਜਨਤਕ(ਅਚਲ AstScope* ਅਚਲ scopep,
                            ਅਚਲ std::string& ਅਧਾਰ_Signal_Name) ਅਚਲ {
        ਅਚਲ std::string ਅਧਾਰ_Signal_ਕੁੰਜੀ = get_ਕੁੰਜੀ_Name(scopep, ਅਧਾਰ_Signal_Name);
        ਅਚਲ std::map<ਅਚਲ std::string, Scope_Var_ਅੰਕੜੇ>::const_iterator ਅਧਾਰ_Signal_It
            = m_scopeVars.ਲੱਭੋ(ਅਧਾਰ_Signal_ਕੁੰਜੀ);
        ਜੇ (ਅਧਾਰ_Signal_It == m_scopeVars.ਸਿਰਾ()) {
            ਵਾਪਸ ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ AstVar* ਅਚਲ ਅਧਾਰ_Signal_Varp = ਅਧਾਰ_Signal_It->ਦੂਜਾ.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            ਜੇ (!ਅਧਾਰ_Signal_Varp->is_Sig_ਜਨਤਕ()) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ force_Control_ਸੰਕੇਤ_Are_Valid(ਅਚਲ AstScope* ਅਚਲ scopep,
                                     ਅਚਲ AstVar* ਅਚਲ ਅਧਾਰ_Signal_Varp) ਅਚਲ {
        constexpr std::array<ਅਚਲ ਅੱਖਰ*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        ਵਾਪਸ std::all_ਦਾ(
            forceControlSuffixes.begin(), forceControlSuffixes.ਸਿਰਾ(),
            [scopep, ਅਧਾਰ_Signal_Varp, ਇਹ](ਅਚਲ std::string& forceControlSuffix) {
                ਅਚਲ std::string controlSignalName = ਅਧਾਰ_Signal_Varp->name() + forceControlSuffix;
                ਅਚਲ std::string control_Signal_ਕੁੰਜੀ = get_ਕੁੰਜੀ_Name(scopep, controlSignalName);
                ਅਚਲ std::ਮਾਪ_t control_Signal_ਗਿਣਤੀ = m_scopeVars.ਗਿਣਤੀ_2(control_Signal_ਕੁੰਜੀ);
                ਜੇ (control_Signal_ਗਿਣਤੀ == 0) {
                    ਅਧਾਰ_Signal_Varp->v3fatalSrc("Signal "
                                               << ਅਧਾਰ_Signal_Varp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << control_Signal_ਕੁੰਜੀ << "'.");
                    ਵਾਪਸ ਝੂਠ;
                }
                ਜੇ (control_Signal_ਗਿਣਤੀ > 1) {
                    ਅਧਾਰ_Signal_Varp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << ਅਧਾਰ_Signal_Varp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    ਵਾਪਸ ਝੂਠ;
                }
                ਵਾਪਸ ਸੱਚ;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    ਖਾਲੀ varHierarchyScopes(std::string scp) {

        std::string::ਮਾਪ_type prd_pos = scp.rfind('.');
        std::string::ਮਾਪ_type dot_pos = scp.rfind("__DOT__");

        ਜਦੋਂ (!scp.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ scpit = m_vpiScopeCandidates.ਲੱਭੋ(scopeSymString(scp));
            ਜੇ ((scpit != m_vpiScopeCandidates.ਸਿਰਾ())
                && (m_scope_ਨਾਂ.ਲੱਭੋ(scp) == m_scope_ਨਾਂ.ਸਿਰਾ())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_scope_ਨਾਂ.emplace(scpit->ਦੂਜਾ.m_symName, scpit->ਦੂਜਾ);
                ਜੇ (!pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ.m_type = scpit->ਦੂਜਾ.m_type;
            }

            // resize and advance pointers
            ਜੇ ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } ਨਹੀਂਤਾਂ {
                ਜੇ (prd_pos == string::npos) ਰੋਕੋ;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    ਖਾਲੀ ਜੋੜੋ_Scope_Var_ਇੰਦਰਾਜ(ਅਚਲ AstScope* ਅਚਲ scopep, ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp,
                          ਅਚਲ AstVar* ਅਚਲ varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string ਸਾਰਾ = scopep->name() + "__DOT__" + varp->name();
        ਜੇ (VString::startsWith(ਸਾਰਾ, "__DOT__TOP")) ਸਾਰਾ.replace(0, 10, "");
        ਅਚਲ std::string::ਮਾਪ_type dpos = ਸਾਰਾ.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        ਅਚਲ std::string scpName = ਸਾਰਾ.substr(0, dpos);
        ਅਚਲ std::string var_ਅਧਾਰ = ਸਾਰਾ.substr(dpos + std::ਲੜੀ_ਲੰਬਾਈ("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        ਅਚਲ std::string var_ਅਧਾਰ_Pretty = Ast_ਗੰਢ::vpiName(VName::dehash(var_ਅਧਾਰ));
        ਅਚਲ std::string scpPretty = Ast_ਗੰਢ::prettyName(VName::dehash(scpName));
        ਅਚਲ std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        ਜੇ (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scope_ਨਾਂ.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, var_ਅਧਾਰ_Pretty, varp, modp, scopep));
    }

    ਖਾਲੀ varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        ਲਈ (ਅਚਲ ScopeModPair& smPair : m_scopes) {
            ਅਚਲ AstScope* ਅਚਲ scopep = smPair.ਪਹਿਲਾ;
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ smodp = smPair.ਦੂਜਾ;
            ਲਈ (ਅਚਲ ModVarPair& mvPair : m_modVars) {
                ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = mvPair.ਪਹਿਲਾ;
                ਅਚਲ AstVar* ਅਚਲ varp = mvPair.ਦੂਜਾ;
                ਜੇ (modp != smodp) ਜਾਰੀ;
                ਜੋੜੋ_Scope_Var_ਇੰਦਰਾਜ(scopep, modp, varp);
            }
        }
    }

    ਖਾਲੀ collectIfaceRefs(ਅਚਲ AstScope* nodep) {
        ਅਚਲ AstCell* ਅਚਲ cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        ਜੇ (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) ਵਾਪਸ;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        ਅਚਲ std::string ਰਸਤਾ = Ast_ਗੰਢ::vpiName(nodep->name());
        ਅਚਲ std::string instName = Ast_ਗੰਢ::vpiName(cellp->origName());
        UASSERT_OBJ(ਰਸਤਾ.ਲੰਮਾਈ() > instName.ਲੰਮਾਈ() && VString::ਸਿਰੇ_With(ਰਸਤਾ, instName), nodep,
                    "Interface scope name " << ਰਸਤਾ << " does not end with instance name "
                                            << instName);
        ਅਚਲ std::string ਮਾਪਾ_ਰਸਤਾ = ਰਸਤਾ.substr(0, ਰਸਤਾ.ਲੰਮਾਈ() - instName.ਲੰਮਾਈ());

        ਲਈ (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            ਅਚਲ std::string refName = Ast_ਗੰਢ::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            ਜੇ (!VString::startsWith(refName, ਮਾਪਾ_ਰਸਤਾ)) ਜਾਰੀ;
            m_ifaceRefs.emplace_ਪਿੱਠ(nodep, scopeSymString(intfRefp->ਮਾਪਾ_Name()), refName,
                                     Ast_ਗੰਢ::vpiName(intfRefp->ਅਧਾਰ_Name()),
                                     intfRefp->modportName());
        }
    }

    ਖਾਲੀ build_Iface_Ref_ਸਾਰਣੀ() {
        ਜੇ (m_ifaceRefs.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਅਚਲ std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_ਛਾਂਟੋ(
            m_ifaceRefs.begin(), m_ifaceRefs.ਸਿਰਾ(),
            [](ਅਚਲ Iface_Ref_ਅੰਕੜੇ& a, ਅਚਲ Iface_Ref_ਅੰਕੜੇ& b) { ਵਾਪਸ a.m_suffix < b.m_suffix; });
        ਲਈ (ਅਚਲ Iface_Ref_ਅੰਕੜੇ& ird : m_ifaceRefs) {
            ਅਚਲ std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            ਜੇ (m_scope_ਨਾਂ.ਲੱਭੋ(scopeSym) == m_scope_ਨਾਂ.ਸਿਰਾ()) ਜਾਰੀ;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string ਮਾਪਾ_ਵਿੱਥ = "VL_IFACEREF_NO_PARENT";
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਮਾਪਾ = m_scope_ਨਾਂ.ਲੱਭੋ(ird.m_ਮਾਪਾ_Sym);
            ਜੇ (ਮਾਪਾ != m_scope_ਨਾਂ.ਸਿਰਾ()) {
                ਮਾਪਾ_ਵਿੱਥ = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + ਮਾਪਾ->ਦੂਜਾ.m_symName) + ")";
            }
            std::string ਪੰਕਤੀ
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            ਪੰਕਤੀ += ਮਾਪਾ_ਵਿੱਥ + ", \"";
            ਪੰਕਤੀ += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(VIdProtect::protect_ਸ਼ਬਦ_If(ird.m_name, ਸੱਚ));
            ਪੰਕਤੀ += "\", \"";
            ਪੰਕਤੀ += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(
                VIdProtect::protect_ਸ਼ਬਦ_If(ird.m_suffix, ਸੱਚ));
            ਪੰਕਤੀ += "\", \"";
            ਪੰਕਤੀ += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(
                VIdProtect::protect_ਸ਼ਬਦ_If(ird.m_modportName, ਸੱਚ));
            ਪੰਕਤੀ += "\"}";
            m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.emplace_ਪਿੱਠ(std::move(ਪੰਕਤੀ));
        }
        ਜੇ (!m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) m_iface_Ref_ਸਾਰਣੀ_Name = symClass + "__VpiIfaceRefTable";
    }

    ਖਾਲੀ buildVpiHierarchy() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scope_ਨਾਂ) {
            ਅਚਲ std::string symName = itpair.ਦੂਜਾ.m_symName;
            std::string above = symName;
            ਜੇ (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            ਜਦੋਂ (!above.ਖਾਲੀ_ਥਾਵਾਂ()) {
                ਅਚਲ std::string::ਮਾਪ_type pos = above.rfind("__");
                ਜੇ (pos == std::string::npos) ਰੋਕੋ;
                above.resize(pos);
                ਜੇ (m_vpiScopeHierarchy.ਲੱਭੋ(above) != m_vpiScopeHierarchy.ਸਿਰਾ()) {
                    m_vpiScopeHierarchy[above].ਧੱਕੋ_ਪਿੱਠ(symName);
                    ਰੋਕੋ;
                }
            }
            m_vpiScopeHierarchy[symName] = std::ਸਦਿਸ਼<string>();
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        ਜੇ (v3Global.opt.vpi()) buildVpiHierarchy();

        ਜੇ (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& scp : m_dpi_Scope_ਨਾਂ) m_scope_ਨਾਂ.emplace(scp.ਪਹਿਲਾ, scp.ਦੂਜਾ);
        }

        // Sort by names, so line/process order matters less
        std::stable_ਛਾਂਟੋ(m_scopes.begin(), m_scopes.ਸਿਰਾ(),
                         [](ਅਚਲ ScopeModPair& a, ਅਚਲ ScopeModPair& b) {
                             ਵਾਪਸ a.ਪਹਿਲਾ->name() < b.ਪਹਿਲਾ->name();
                         });
        std::stable_ਛਾਂਟੋ(m_dpis.begin(), m_dpis.ਸਿਰਾ(),  //
                         [](ਅਚਲ AstCFunc* ap, ਅਚਲ AstCFunc* bp) {
                             ਜੇ (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 ਵਾਪਸ bp->dpiImportPrototype();
                             }
                             ਵਾਪਸ ap->name() < bp->name();
                         });

        // Output
        ਜੇ (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        ਜੇ (v3Global.dpi()) {
            emitDpiHdr();
            ਜੇ (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->isConstPool()) ਵਾਪਸ;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellInlineScope* nodep) override {
        ਅਚਲ std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        ਅਚਲ std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        ਅਚਲ ਪੂਰਨ timeunit = m_modp->timeunit().power_ਦਾ_Ten();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), Ast_ਗੰਢ::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        ਜੇ (VN_IS(m_modp, Class)) ਵਾਪਸ;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_ਪਿੱਠ(nodep, m_modp);

        ਜੇ (v3Global.opt.vpi() && !nodep->is_ਸਿਖਰ()) {
            ਅਚਲ std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            ਅਚਲ ਪੂਰਨ timeunit = m_modp->timeunit().power_ਦਾ_Ten();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      Ast_ਗੰਢ::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {
        ਜੇ (nodep->selfRelative()) ਵਾਪਸ;  // Its absolute copy is under the scope
        ਅਚਲ std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        ਅਚਲ ਪੂਰਨ timeunit = m_modp ? m_modp->timeunit().power_ਦਾ_Ten() : 0;
        m_dpi_Scope_ਨਾਂ.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        ਜੇ (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } ਨਹੀਂਤਾਂ {
            // Note emplace does not construct when duplicate key
            m_dpi_Scope_ਨਾਂ.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        ਜੇ ((nodep->is_Sig_User_Rd_ਜਨਤਕ() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_modVars.emplace_ਪਿੱਠ(m_modp, nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Decl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        ਜੇ (!nodep->ਅੰਕੜੇ_Decl_Nullp()) {  // else duplicate we don't need code for
            nodep->bin_ਅੰਕ(m_coverBins);
            m_coverBins += nodep->ਮਾਪ();
            nodep->local_Bin_ਅੰਕ(m_modCoverBins);
            m_modCoverBins += nodep->ਮਾਪ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        nameCheck(nodep);
        ਜੇ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.ਧੱਕੋ_ਪਿੱਠ(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    ਖਾਲੀ ਦੇਖੋ(AstConst*) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ EmitCSyms(AstNetlist* nodep, ਤਾਰਕਿਕ dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

ਖਾਲੀ EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ਸਿਰਲੇਖ_File(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->puts_Int_ਸਿਖਰ_Include();
    puts("#include \"verilated.h\"\n");
    ਜੇ (v3Global.needTraceDumper()) {
        ਲਈ (ਅਚਲ string& ਅਧਾਰ : v3Global.opt.trace_ਸਰੋਤ_Langs())
            puts("#include \"" + ਅਧਾਰ + ".h\"\n");
    }
    ਜੇ (v3Global.opt.ਵਰਤਦਾ_Profiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + ਸਿਖਰ_Class_Name() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    ਲਈ (Ast_ਗੰਢ_Module *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), ਗੰਢ_Module);
        ਜੇ (VN_IS(nodep, Class)) ਜਾਰੀ;  // Class included earlier
        ਜੇ (nodep->isConstPool()) ਜਾਰੀ;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep) + ".h\"\n");
    }

    ਜੇ (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::ਸੈੱਟ<std::string> types;  // Remove duplicates and sort
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scopeFuncs) {
            ਅਚਲ AstCFunc* ਅਚਲ funcp = itpair.ਦੂਜਾ.m_cfuncp;
            ਜੇ (!funcp->dpiExportImpl()) ਜਾਰੀ;
            ਅਚਲ std::string cbtype
                = protect(v3Global.opt.ਅਗੇਤਰ() + "__Vcb_" + funcp->cname() + "_t");
            ਅਚਲ std::string functype = funcp->rtnTypeVoid() + " (*) (" + c_Func_ਦਲੀਲਾਂ(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        ਲਈ (ਅਚਲ std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->puts_ਨਿੱਜੀ(ਝੂਠ);  // public:

    puts("// INTERNAL STATE\n");
    puts(ਸਿਖਰ_Class_Name() + "* const __Vm_modelp;\n");

    ਜੇ (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    ਜੇ (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    ਜੇ (v3Global.hasEvents()) {
        ਜੇ (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } ਨਹੀਂਤਾਂ {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    ਜੇ (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    ਜੇ (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    ਜੇ (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    ਜੇ (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    ਜੇ (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::ਮੰਜ਼ਿਲ_string(ExecMTask::ਅੰਕ_ਵਰਤਿਆ_Ids()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    ਲਈ (ਅਚਲ ScopeModPair& itpair : m_scopes) {
        ਅਚਲ AstScope* ਅਚਲ scopep = itpair.ਪਹਿਲਾ;
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = itpair.ਦੂਜਾ;
        ਜੇ (VN_IS(modp, Class)) ਜਾਰੀ;
        ofp()->ਲਿਖੋ("%-30s ", EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    ਜੇ (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.ਧਾਗੇ() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::ਮੰਜ਼ਿਲ_string(m_coverBins));
        puts("];\n");
    }

    ਜੇ (!m_scope_ਨਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scope_ਨਾਂ) {
            ਅਚਲ Scope_ਅੰਕੜੇ& sd = itpair.ਦੂਜਾ;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    ਜੇ (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + ਸਿਖਰ_Class_Name()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    ਜੇ (v3Global.hasEvents()) {
        ਜੇ (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } ਨਹੀਂਤਾਂ {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        ਜੇ (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } ਨਹੀਂਤਾਂ {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        ਜੇ (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } ਨਹੀਂਤਾਂ {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    ਜੇ (v3Global.needTraceDumper()) {
        ਜੇ (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    ਜੇ (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->puts_ਸਿਰਾ_Guard();
    closeOutputFile();
}

ਖਾਲੀ EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    ਤਾਰਕਿਕ needs_New_ਸਤਰ = ਝੂਠ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scopeFuncs) {
        ਅਚਲ AstCFunc* ਅਚਲ funcp = itpair.ਦੂਜਾ.m_cfuncp;
        ਜੇ (!funcp->dpiExportImpl()) ਜਾਰੀ;
        emitCFuncDecl(funcp, itpair.ਦੂਜਾ.m_modp);
        needs_New_ਸਤਰ = ਸੱਚ;
    }
    ਜੇ (needs_New_ਸਤਰ) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::ਸੈੱਟ<std::string> lib_ਨਾਂ;
    ਲਈ (ਅਚਲ ScopeModPair& itpair : m_scopes) {
        ਜੇ (itpair.ਦੂਜਾ->verilatorLib()) lib_ਨਾਂ.emplace(itpair.ਦੂਜਾ->origName());
    }
    ਲਈ (ਅਚਲ std::string& libName : lib_ਨਾਂ) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    ਜੇ (!lib_ਨਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    ਜੇ (!m_varTables.ਖਾਲੀ_ਥਾਵਾਂ() || !m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ() || !m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.ਪਹਿਲਾ + "[];\n");
        }
        ਜੇ (!m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            puts("extern const VlScopeTableEntry " + m_scope_ਸਾਰਣੀ_Name + "[];\n");
        }
        ਜੇ (!m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_ਸਾਰਣੀ_Name + "[];\n");
        }
        puts("\n");
    }
}

ਖਾਲੀ EmitCSyms::emitVarTables() {
    ਜੇ (m_varTables.ਖਾਲੀ_ਥਾਵਾਂ() && m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ() && m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.ਪਹਿਲਾ + "[] = {\n");
        ਲਈ (ਅਚਲ std::string& ਪੰਕਤੀ : kv.ਦੂਜਾ) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ਪੰਕਤੀ);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ਜੇ (!m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        puts("extern const VlScopeTableEntry " + m_scope_ਸਾਰਣੀ_Name + "[] = {\n");
        ਲਈ (ਅਚਲ std::string& ਪੰਕਤੀ : m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ਪੰਕਤੀ);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    ਜੇ (!m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        puts("extern const VlIfaceRefTableEntry " + m_iface_Ref_ਸਾਰਣੀ_Name + "[] = {\n");
        ਲਈ (ਅਚਲ std::string& ਪੰਕਤੀ : m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(ਪੰਕਤੀ);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic pop\n");
    puts("#endif\n");
}

ਖਾਲੀ EmitCSyms::emitScopeHier(ਪਾਠ_Stmts& stmts, ਤਾਰਕਿਕ destroy) {
    ਜੇ (!v3Global.opt.vpi()) ਵਾਪਸ;

    ਜੇ (destroy) {
        stmts.ਜੋੜੋ("// Tear down scope hierarchy");
    } ਨਹੀਂਤਾਂ {
        stmts.ਜੋੜੋ("// Set up scope hierarchy");
    }

    ਅਚਲ std::string ਵਿਧੀ = destroy ? "remove" : "add";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scope_ਨਾਂ) {
        ਜੇ (itpair.ਪਹਿਲਾ == "TOP") ਜਾਰੀ;
        ਅਚਲ Scope_ਅੰਕੜੇ& sd = itpair.ਦੂਜਾ;
        ਅਚਲ std::string& name = sd.m_prettyName;
        ਅਚਲ std::string& scopeType = sd.m_type;
        ਜੇ (name.ਲੱਭੋ('.') != string::npos) ਜਾਰੀ;
        ਜੇ (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") ਜਾਰੀ;
        ਅਚਲ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.ਜੋੜੋ("vlSymsp->__Vhier." + ਵਿਧੀ + "(0, " + id + ");");
    }
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_vpiScopeHierarchy) {
        ਅਚਲ std::string fromName = scopeSymString(itpair.ਪਹਿਲਾ);
        ਅਚਲ std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scope_ਨਾਂ.at(fromName).m_symName);
        ਲਈ (ਅਚਲ std::string& name : itpair.ਦੂਜਾ) {
            ਅਚਲ std::string ਮੰਜ਼ਿਲ_Name = scopeSymString(name);
            ਅਚਲ std::string ਮੰਜ਼ਿਲ_Id
                = "vlSymsp->" + protect("__Vscopep_" + m_scope_ਨਾਂ.at(ਮੰਜ਼ਿਲ_Name).m_symName);
            stmts.ਜੋੜੋ("vlSymsp->__Vhier." + ਵਿਧੀ + "(" + fromId + ", " + ਮੰਜ਼ਿਲ_Id + ");");
        }
    }

    ਜੇ (destroy) {
        stmts.ਜੋੜੋ("// Clear keys from hierarchy map after values have been removed");
        stmts.ਜੋੜੋ("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::ਪਾਠ_Stmts EmitCSyms::getSymCtorStmts() {
    ਪਾਠ_Stmts stmts;

    {
        uint64_t ਢੇਰ_ਮਾਪ = V3_ਢੇਰ_ਗਿਣਤੀ::ਗਿਣਤੀ_2(v3Global.rootp());
        ਜੇ (v3Global.opt.debug_ਢੇਰ_Check()) ਢੇਰ_ਮਾਪ += 1024 * 1024 * 1024;
        V3Stats::ਜੋੜੋ_Stat("Size prediction, Stack (bytes)", ਢੇਰ_ਮਾਪ);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::ਜੋੜੋ_Stat("Size prediction, Heap, from Var Scopes (bytes)", m_stat_Var_Scope_ਬਾਈਟਾਂ);
        V3Stats::ਜੋੜੋ_Stat(V3Stats::STAT_MODEL_ਮਾਪ, ਢੇਰ_ਮਾਪ + m_stat_Var_Scope_ਬਾਈਟਾਂ);

        stmts.ਜੋੜੋ("// Check resources");
        stmts.ਜੋੜੋ("Verilated::stackCheck(" + std::ਮੰਜ਼ਿਲ_string(ਢੇਰ_ਮਾਪ) + ");");
    }

    stmts.ਜੋੜੋ("// Setup sub module instances");
    ਲਈ (ਅਚਲ ScopeModPair& itpair : m_scopes) {
        ਅਚਲ AstScope* ਅਚਲ scopep = itpair.ਪਹਿਲਾ;
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = itpair.ਦੂਜਾ;
        ਜੇ (modp->is_ਸਿਖਰ()) ਜਾਰੀ;
        ਅਚਲ std::string name = V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(
            VIdProtect::protect_ਸ਼ਬਦ_If(scopep->prettyName(), scopep->protect()));
        stmts.ਜੋੜੋ("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    ਜੇ (v3Global.opt.profPgo()) {
        stmts.ਜੋੜੋ("// Configure profiling for PGO\n");
        ਜੇ (!v3Global.opt.hier_ਬੱਚਾ()) {
            stmts.ਜੋੜੋ("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        ਜੇ (v3Global.opt.mtasks()) {
            v3Global.rootp()->ਸਿਖਰ_Modulep()->foreach([&](ਅਚਲ Ast_Exec_ਗ੍ਰਾਫ* execGraphp) {
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : execGraphp->depGraphp()->vertices()) {
                    ਅਚਲ ExecMTask& mt = static_cast<ਅਚਲ ExecMTask&>(vtx);
                    stmts.ਜੋੜੋ("vlSymsp->_vm_pgoProfiler.addCounter(" + std::ਮੰਜ਼ਿਲ_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.ਜੋੜੋ("// Configure time unit / time precision");
    ਜੇ (!v3Global.rootp()->timeunit().isNone()) {
        ਅਚਲ std::string unit = std::ਮੰਜ਼ਿਲ_string(v3Global.rootp()->timeunit().power_ਦਾ_Ten());
        stmts.ਜੋੜੋ("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    ਜੇ (!v3Global.rootp()->timeprecision().isNone()) {
        ਅਚਲ std::string prec = std::ਮੰਜ਼ਿਲ_string(v3Global.rootp()->timeprecision().power_ਦਾ_Ten());
        stmts.ਜੋੜੋ("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.ਜੋੜੋ("// Setup each module's pointers to their submodules");
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : m_scopes) {
        ਅਚਲ AstScope* ਅਚਲ scopep = i.ਪਹਿਲਾ;
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = i.ਦੂਜਾ;
        ਅਚਲ AstScope* ਅਚਲ abovep = scopep->aboveScopep();
        ਜੇ (!abovep) ਜਾਰੀ;

        ਅਚਲ std::string protName = VIdProtect::protect_ਸ਼ਬਦ_If(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        ਜੇ (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } ਨਹੀਂਤਾਂ {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.ਜੋੜੋ(stmt);
    }

    ਜੇ (v3Global.opt.coverage()) {
        stmts.ਜੋੜੋ("// Setup each module's pointer back to symbol table (for public functions)");
        ਲਈ (ਅਚਲ ScopeModPair& i : m_scopes) {
            ਅਚਲ AstScope* ਅਚਲ scopep = i.ਪਹਿਲਾ;
            Ast_ਗੰਢ_Module* ਅਚਲ modp = i.ਦੂਜਾ;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            ਅਚਲ ਤਾਰਕਿਕ ਪਹਿਲਾ = !modp->user1();
            modp->user1(ਸੱਚ);
            stmts.ਜੋੜੋ("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (ਪਹਿਲਾ ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    ਲਈ (ਅਚਲ ScopeModPair& i : m_scopes) {
        ਅਚਲ AstScope* ਅਚਲ scopep = i.ਪਹਿਲਾ;
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = i.ਦੂਜਾ;
        ਜੇ (!modp->verilatorLib()) ਜਾਰੀ;
        ਅਚਲ AstVar* hashp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstVar* handlep = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
            ਜੇ (!varp) ਜਾਰੀ;
            ਜੇ (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            ਜੇ (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        ਅਚਲ uint32_t hash = VN_AS(hashp->valuep(), Const)->ਮੰਜ਼ਿਲ_UInt();
        stmts.ਜੋੜੋ(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::ਮੰਜ਼ਿਲ_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string ਰਸਤਾ = scopep->prettyName();
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਰਸਤਾ.erase(0, v3Global.rootp()->trace_Lib_ਸਿਖਰ_Name().ਮਾਪ() + 1);
        }
        stmts.ਜੋੜੋ("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(ਰਸਤਾ) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    ਜੇ (!m_scope_ਨਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        stmts.ਜੋੜੋ("// Setup scopes");
        ਅਚਲ std::string symClass = symClassName();
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scope_ਨਾਂ) {
            ਅਚਲ Scope_ਅੰਕੜੇ& sd = itpair.ਦੂਜਾ;
            std::string ਪੰਕਤੀ
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            ਪੰਕਤੀ += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(
                VIdProtect::protect_ਸ਼ਬਦ_If(sd.m_prettyName, ਸੱਚ));
            ਪੰਕਤੀ += "\", \"";
            ਪੰਕਤੀ += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(
                protect(scopeDecodeIdentifier(sd.m_prettyName)));
            ਪੰਕਤੀ += "\", \"";
            ਪੰਕਤੀ += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(sd.m_defName);
            ਪੰਕਤੀ += "\", ";
            ਪੰਕਤੀ += std::ਮੰਜ਼ਿਲ_string(sd.m_timeunit);
            ਪੰਕਤੀ += ", VerilatedScope::" + sd.m_type + "}";
            m_scope_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.emplace_ਪਿੱਠ(std::move(ਪੰਕਤੀ));
        }
        m_scope_ਸਾਰਣੀ_Name = symClass + "__VpiScopeTable";
        stmts.ਜੋੜੋ("VerilatedScope::scopesConstructFromTable(" + m_scope_ਸਾਰਣੀ_Name + ", "
                  + std::ਮੰਜ਼ਿਲ_string(m_scope_ਨਾਂ.ਮਾਪ()) + ", vlSymsp);");
    }

    // After the scopes above, as each row points at an already-built VerilatedScope
    build_Iface_Ref_ਸਾਰਣੀ();
    ਜੇ (!m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        stmts.ਜੋੜੋ("// Setup interface references");
        stmts.ਜੋੜੋ("VerilatedScope::ifaceRefsInsertFromTable(" + m_iface_Ref_ਸਾਰਣੀ_Name + ", "
                  + std::ਮੰਜ਼ਿਲ_string(m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਮਾਪ()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, ਝੂਠ);

    ਜੇ (v3Global.dpi()) {
        ਲਈ (ਅਚਲ std::string vfinal : {"0", "1"}) {
            stmts.ਜੋੜੋ("// Setup export functions - final: " + vfinal);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scopeFuncs) {
                ਅਚਲ Scope_Func_ਅੰਕੜੇ& sfd = itpair.ਦੂਜਾ;
                ਅਚਲ AstScopeName* ਅਚਲ scopep = sfd.m_scopep;
                ਅਚਲ AstCFunc* ਅਚਲ funcp = sfd.m_cfuncp;
                ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = sfd.m_modp;
                ਜੇ (!funcp->dpiExportImpl()) ਜਾਰੀ;

                std::string stmt;
                stmt += "vlSymsp->" + protect("__Vscopep_" + scopep->scopeSymName())
                        + "->exportInsert(";
                stmt += vfinal + ", \"";
                // Not protected - user asked for import/export
                stmt += V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(funcp->cname());
                stmt += "\", (void*)(&";
                stmt += EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);
                stmt += "__";
                stmt += funcp->nameProtect();
                stmt += "));";
                stmts.ਜੋੜੋ(stmt);
            }
        }
    }

    // Relies on m_scopeVars being sorted so each VPI scope's vars are
    // contiguous. Tables are deduplicated by row content, not by scope, so
    // distinct scopes mapping to the same C++ class (e.g. --public-flat-rw)
    // still share a table when their rows are identical.
    ਜੇ (!m_scopeVars.ਖਾਲੀ_ਥਾਵਾਂ()) {
        stmts.ਜੋੜੋ("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> ਸਾਰਣੀ_By_ਪੰਕਤੀਆਂ;
        ਪੂਰਨ ਸਾਰਣੀ_Counter = 0;

        ਸਵੈਚਾਲਿਤ it = m_scopeVars.cbegin();
        ਜਦੋਂ (it != m_scopeVars.cend()) {
            ਅਚਲ std::string scopeName = it->ਦੂਜਾ.m_scopeName;
            ਅਚਲ AstScope* ਅਚਲ instScopep = it->ਦੂਜਾ.m_scopep;
            ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = it->ਦੂਜਾ.m_modp;
            ਅਚਲ std::string modClassName = EmitCUtil::ਅਗੇਤਰ_Name_Protect(modp);

            std::ਸਦਿਸ਼<std::string> ਪੰਕਤੀਆਂ;
            std::ਸਦਿਸ਼<std::string> residual;

            ਲਈ (; it != m_scopeVars.cend() && it->ਦੂਜਾ.m_scopeName == scopeName; ++it) {
                ਅਚਲ Scope_Var_ਅੰਕੜੇ& svd = it->ਦੂਜਾ;
                ਅਚਲ AstScope* ਅਚਲ scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                ਅਚਲ AstVar* ਅਚਲ varp = svd.m_varp;
                ਅਚਲ VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                ਅਚਲ std::pair<ਤਾਰਕਿਕ, std::string> fc = isForceControlSignal(varp);
                ਜੇ (fc.ਪਹਿਲਾ && ਅਧਾਰ_Signal_Is_ਜਨਤਕ(scopep, fc.ਦੂਜਾ)
                    && ਅਧਾਰ_Signal_Is_Valid(scopep, varp, fc.ਦੂਜਾ)) {
                    ਜਾਰੀ;
                }

                std::string ਪੰਕਤੀ;
                ਅਚਲ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ ਕਿਸਮ
                    = try_Build_ਸਾਰਣੀ_ਇੰਦਰਾਜ(svd, varp, scopep, modClassName, dims, ਪੰਕਤੀ);
                ਚੋਣ (ਕਿਸਮ) {
                ਹਾਲਤ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::ਸਾਰਣੀ_ਪੰਕਤੀ:
                    ਪੰਕਤੀਆਂ.emplace_ਪਿੱਠ(ਪੰਕਤੀ);
                    ਜੋੜੋ_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_ਅਧਾਰ_Pretty,
                                           varName(varp), varp->dtypep());
                    ਰੋਕੋ;
                ਹਾਲਤ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::FORCEABLE_RESIDUAL: {
                    ਅਚਲ std::string bounds = boundsString(dims);
                    residual.emplace_ਪਿੱਠ(ਪਾਓ_Forceable_Var_Statement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    ਰੋਕੋ;
                }
                ਹਾਲਤ ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::PLAIN_RESIDUAL: {
                    ਅਚਲ std::string bounds = boundsString(dims);
                    residual.emplace_ਪਿੱਠ(
                        ਪਾਓ_Var_Statement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    ਰੋਕੋ;
                }
                ਮੂਲ: v3fatalSrc("Bad case");
                }
                ਜੇ (ਕਿਸਮ == ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::ਸਾਰਣੀ_ਪੰਕਤੀ) ਜਾਰੀ;
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), ਗੰਢ_UOr_Struct_DType)) {
                    ਜੇ (!sdtypep->packed()) {
                        ਜੋੜੋ_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_ਅਧਾਰ_Pretty,
                                               varName(varp), sdtypep);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ਜੋੜੋ_Unpacked_Array_UOr_Struct_Member_Vars(residual, svd, scopep, svd.m_var_ਅਧਾਰ_Pretty,
                                                        varName(varp), varp->dtypep());
                }
                ਜੇ (ਕਿਸਮ == ਸਾਰਣੀ_ਇੰਦਰਾਜ_ਕਿਸਮ::PLAIN_RESIDUAL) {
                    ਜੋੜੋ_Packed_Agg_Member_Vars(residual, svd, scopep, svd.m_var_ਅਧਾਰ_Pretty,
                                           varName(varp), varp->dtypep());
                }
            }

            ਜੇ (!ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                ਅਚਲ ਮਾਪ_t ਪੰਕਤੀ_ਗਿਣਤੀ = ਪੰਕਤੀਆਂ.ਮਾਪ();
                std::string ਕੁੰਜੀ;
                ਲਈ (ਅਚਲ std::string& r : ਪੰਕਤੀਆਂ) {
                    ਕੁੰਜੀ += r;
                    ਕੁੰਜੀ += '\0';
                }
                ਅਚਲ ਸਵੈਚਾਲਿਤ itt = ਸਾਰਣੀ_By_ਪੰਕਤੀਆਂ.ਲੱਭੋ(ਕੁੰਜੀ);
                std::string ਸਾਰਣੀ_Name;
                ਜੇ (itt != ਸਾਰਣੀ_By_ਪੰਕਤੀਆਂ.ਸਿਰਾ()) {
                    ਸਾਰਣੀ_Name = itt->ਦੂਜਾ;
                } ਨਹੀਂਤਾਂ {
                    ਸਾਰਣੀ_Name = modClassName + "__VpiVarTable" + std::ਮੰਜ਼ਿਲ_string(ਸਾਰਣੀ_Counter++);
                    ਸਾਰਣੀ_By_ਪੰਕਤੀਆਂ.emplace(std::move(ਕੁੰਜੀ), ਸਾਰਣੀ_Name);
                    m_varTables.emplace_ਪਿੱਠ(ਸਾਰਣੀ_Name, std::move(ਪੰਕਤੀਆਂ));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + ਸਾਰਣੀ_Name + ", "
                                   + std::ਮੰਜ਼ਿਲ_string(ਪੰਕਤੀ_ਗਿਣਤੀ) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.ਜੋੜੋ(call);
            }
            ਲਈ (ਅਚਲ std::string& r : residual) stmts.ਜੋੜੋ(r);
        }
    }

    ਵਾਪਸ stmts;
}

EmitCSyms::ਪਾਠ_Stmts EmitCSyms::getSymDtorStmts() {
    ਪਾਠ_Stmts stmts;

    emitScopeHier(stmts, ਸੱਚ);
    ਜੇ (v3Global.needTraceDumper()) {
        stmts.ਜੋੜੋ("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    ਜੇ (v3Global.opt.profPgo()) {
        stmts.ਜੋੜੋ("vlSymsp->_vm_pgoProfiler.write(\"" + ਸਿਖਰ_Class_Name()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    ਜੇ (!m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        stmts.ਜੋੜੋ("// Tear down interface references");
        stmts.ਜੋੜੋ("VerilatedScope::ifaceRefsEraseFromTable(" + m_iface_Ref_ਸਾਰਣੀ_Name + ", "
                  + std::ਮੰਜ਼ਿਲ_string(m_iface_Ref_ਸਾਰਣੀ_ਪੰਕਤੀਆਂ.ਮਾਪ()) + ", vlSymsp);");
    }

    stmts.ਜੋੜੋ("// Tear down scopes");
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itpair : m_scope_ਨਾਂ) {
        ਅਚਲ Scope_ਅੰਕੜੇ& sd = itpair.ਦੂਜਾ;
        ਅਚਲ std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.ਜੋੜੋ("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.ਜੋੜੋ("// Tear down sub module instances");
    ਲਈ (ਅਚਲ ScopeModPair& itpair : vlstd::ਉਲਟ_view(m_scopes)) {
        ਅਚਲ AstScope* ਅਚਲ scopep = itpair.ਪਹਿਲਾ;
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = itpair.ਦੂਜਾ;
        ਜੇ (modp->is_ਸਿਖਰ()) ਜਾਰੀ;
        stmts.ਜੋੜੋ("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    ਵਾਪਸ stmts;
}

ਖਾਲੀ EmitCSyms::emitSymImp(ਅਚਲ AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    ਪਾਠ_Stmts ctorStmts = getSymCtorStmts();
    ਪਾਠ_Stmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    ਅਚਲ std::pair<std::string, std::string> ਵੰਡੋ_Ctor_Dtor_Decls  //
        = emit_ਵੰਡੋ_Ctor_Dtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            ਸੱਚ, "Symbol table implementation internals",
                            [ਇਹ]() { emitSymImpPreamble(); });

    open_New_Output_ਸਰੋਤ_File(symClassName(), ਸੱਚ, ਸੱਚ, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    ਅਚਲ AstScope* ਅਚਲ ਸਿਖਰ_Scopep = netlistp->ਸਿਖਰ_Scopep()->scopep();
    ਅਚਲ std::string ਸਿਖਰ_Inst = "vlSymsp->" + instName(ਸਿਖਰ_Scopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    ਜੇ (!ਵੰਡੋ_Ctor_Dtor_Decls.ਪਹਿਲਾ.ਖਾਲੀ_ਥਾਵਾਂ()) puts("\n" + ਵੰਡੋ_Ctor_Dtor_Decls.ਪਹਿਲਾ + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + ਸਿਖਰ_Inst + ", vlSymsp, namep);\n");
    ਲਈ (ਅਚਲ ਪਾਠ_Stmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_ਪਾਠ);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + ਸਿਖਰ_Class_Name() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    ਜੇ (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    ਜੇ (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    ਜੇ (v3Global.opt.profPgo() && !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
        puts("    , _vm_pgoProfiler{" + std::ਮੰਜ਼ਿਲ_string(v3Global.current_Hier_ਖੰਡ_ਲਾਗਤ()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    ਜੇ (!ਵੰਡੋ_Ctor_Dtor_Decls.ਦੂਜਾ.ਖਾਲੀ_ਥਾਵਾਂ()) puts("\n" + ਵੰਡੋ_Ctor_Dtor_Decls.ਦੂਜਾ + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    ਲਈ (ਅਚਲ ਪਾਠ_Stmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_ਪਾਠ);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + ਸਿਖਰ_Inst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    ਜੇ (v3Global.needTraceDumper()) {
        ਜੇ (!optSystemC()) {
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

    ਜੇ (v3Global.opt.savable()) {
        ਲਈ (ਅਚਲ ਤਾਰਕਿਕ& de : {ਝੂਠ, ਸੱਚ}) {
            ਅਚਲ std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            ਅਚਲ std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            ਅਚਲ std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            ਜੇ (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            ਲਈ (ਅਚਲ ScopeModPair& itpair : m_scopes) {
                ਅਚਲ AstScope* ਅਚਲ scopep = itpair.ਪਹਿਲਾ;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

ਖਾਲੀ EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    open_New_Output_ਸਿਰਲੇਖ_File(ਸਿਖਰ_Class_Name() + "__Dpi",
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
    std::ਸੈੱਟ<std::string> struct_ਨਾਂ;
    ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ nodep : m_dpis) {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), struct_ਨਾਂ);
            }
        }
    }
    ਜੇ (!struct_ਨਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    ਪੂਰਨ ਪਹਿਲਾ_Exp = 0;
    ਪੂਰਨ ਪਹਿਲਾ_Imp = 0;
    ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ nodep : m_dpis) {
        ਜੇ (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) ਜਾਰੀ;

        ਅਚਲ std::string ਸਰੋਤ_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        ਜੇ (nodep->dpiExportDispatcher()) {
            ਜੇ (!ਪਹਿਲਾ_Exp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + ਸਰੋਤ_Loc + "\n");
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!ਪਹਿਲਾ_Imp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + ਸਰੋਤ_Loc + "\n");
        }
        ਜੇ (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } ਨਹੀਂਤਾਂ {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + c_Func_ਦਲੀਲਾਂ(nodep, ਝੂਠ) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->puts_ਸਿਰਾ_Guard();

    closeOutputFile();
}

ਖਾਲੀ EmitCSyms::emitDpiStruct(ਅਚਲ Ast_ਗੰਢ_DType* dtypep, std::ਸੈੱਟ<std::string>& namesr) {
    ਜੇ (!dtypep->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()) ਵਾਪਸ;
    ਅਚਲ AstStructDType* ਅਚਲ sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    ਅਚਲ std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    ਜੇ (namesr.ਗਿਣਤੀ_2(name)) ਵਾਪਸ;
    // After the structures it contains
    ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    ਜੇ (name.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;  // Declared where used
    ਜੇ (namesr.ਖਾਲੀ_ਥਾਵਾਂ()) puts("// DPI TYPES\n");
    namesr.ਪਾਓ(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

ਖਾਲੀ EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    open_New_Output_ਸਰੋਤ_File(ਸਿਖਰ_Class_Name() + "__Dpi", ਝੂਠ, ਸੱਚ,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + ਸਿਖਰ_Class_Name() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + ਸਿਖਰ_Class_Name() + "__Dpi.h\"\n");
    puts("#include \"" + ਸਿਖਰ_Class_Name() + ".h\"\n");
    puts("\n");

    ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ nodep : m_dpis) {
        ਜੇ (!nodep->dpiExportDispatcher()) ਜਾਰੀ;

        ਅਚਲ std::string name = nodep->name();
        ਅਚਲ std::string ਸਰੋਤ_Loc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + c_Func_ਦਲੀਲਾਂ(nodep) + ") {\n");
        puts("// DPI export" + ਸਰੋਤ_Loc + "\n");
        putns(nodep, "return " + ਸਿਖਰ_Class_Name() + "::" + name + "(");
        std::string comma;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isIO() && !portp->isFuncReturn()) {
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

ਖਾਲੀ V3EmitC::emitcSyms(ਤਾਰਕਿਕ dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
