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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3EmitC.h"
#include "V3EmitCBase.h"
#include "V3ExecGraph.h"
#include "V3LanguageWords.h"
#include "V3StackCount.h"
#include "V3Stats.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Symbol table emitting

// Some handy short-hands to reduce verbosity
static constexpr auto symClassName = &EmitCUtil::symClassName;
static constexpr auto topClassName = &EmitCUtil::topClassName;

class EmitCSyms final : EmitCBaseVisitorConst {
    // NODE STATE
    // Cleared on Netlist
    //  AstNodeModule::user1()  -> bool.  Set true __Vconfigure called
    const VNUser1InUse m_inuser1;

    // TYPES
    sa_nracanaa ScopeData final {
        const AstNode* m_nodep;  // Scope symbol-table entry corresponds to
        const std::string m_symName;  // Mangled name used to build __Vscopep_ variable
        const std::string m_prettyName;  // Pretty (unprotected) name for display
        const std::string m_defName;  // Name registered for this scope
        const පූර්ණ_සංඛ්‍යා m_timeunit;  // Timeunit of this scope
        std::string m_type;  // TODO: this should be an enum
        ScopeData(const AstNode* nodep, const std::string& symName, const std::string& prettyName,
                  const std::string& defName, පූර්ණ_සංඛ්‍යා timeunit, const std::string& type)
            : m_nodep{nodep}
            , m_symName{symName}
            , m_prettyName{prettyName}
            , m_defName{defName}
            , m_timeunit{timeunit}
            , m_type{type} {}
    };
    sa_nracanaa IfaceRefData final {
        const AstScope* m_scopep;  // Concrete interface scope referred to
        std::string m_parentSym;  // Declaring scope symbol, including inlined levels
        std::string m_suffix;  // Path relative to the model instance
        std::string m_name;  // Name of the reference port
        std::string m_modportName;  // "" = no modport
        IfaceRefData(const AstScope* scopep, const std::string& parentSym,
                     const std::string& suffix, const std::string& name,
                     const std::string& modportName)
            : m_scopep{scopep}
            , m_parentSym{parentSym}
            , m_suffix{suffix}
            , m_name{name}
            , m_modportName{modportName} {}
    };
    sa_nracanaa ScopeFuncData final {
        const AstScopeName* const m_scopep;  // Scope the DPI export function is registered under
        const AstCFunc* const m_cfuncp;  // DPI export function
        const AstNodeModule* const m_modp;  // Module containing the DPI export function
        ScopeFuncData(const AstScopeName* scopep, const AstCFunc* funcp, const AstNodeModule* modp)
            : m_scopep{scopep}
            , m_cfuncp{funcp}
            , m_modp{modp} {}
    };
    sa_nracanaa ScopeVarData final {
        const std::string m_scopeName;  // Full name of the scope containing the variable
        const std::string m_varBasePretty;  // Pretty base name of the variable, without scope
        const AstVar* const m_varp;  // Public variable
        const AstNodeModule* const m_modp;  // Module containing the variable
        const AstScope* const m_scopep;  // Scope containing the variable
        ScopeVarData(const std::string& scopeName, const std::string& varBasePretty,
                     const AstVar* varp, const AstNodeModule* modp, const AstScope* scopep)
            : m_scopeName{scopeName}
            , m_varBasePretty{varBasePretty}
            , m_varp{varp}
            , m_modp{modp}
            , m_scopep{scopep} {}
    };
    using ScopeNames = std::map<const std::string, ScopeData>;
    using ScopeModPair = std::pair<const AstScope*, AstNodeModule*>;
    using ModVarPair = std::pair<const AstNodeModule*, const AstVar*>;

    // Vars with more dims take the residual per-statement path.
    static constexpr පූර්ණ_සංඛ්‍යා VPI_TABLE_MAX_DIMS = 3;

    // STATE
    AstCFunc* m_cfuncp = nullptr;  // Current function
    AstNodeModule* m_modp = nullptr;  // Current module
    const AstScope* m_scopep = nullptr;  // Current scope
    std::vector<ScopeModPair> m_scopes;  // Every scope by module
    std::vector<AstCFunc*> m_dpis;  // DPI functions
    std::vector<ModVarPair> m_modVars;  // Each public {mod,var}
    std::map<const std::string, ScopeFuncData> m_scopeFuncs;  // Each {scope,dpi-export-func}
    std::map<const std::string, ScopeVarData> m_scopeVars;  // Each {scope,public-var}
    ScopeNames m_scopeNames;  // Each unique AstScopeName. Dpi scopes added later
    ScopeNames m_dpiScopeNames;  // Each unique AstScopeName for DPI export
    ScopeNames m_vpiScopeCandidates;  // All scopes for VPI
    std::vector<IfaceRefData> m_ifaceRefs;  // Each interface reference, for VPI
    // The actual hierarchy of scopes
    std::map<const std::string, std::vector<std::string>> m_vpiScopeHierarchy;
    පූර්ණ_සංඛ්‍යා m_coverBins = 0;  // Global coverage bin number for non-object helper functions
    // Counts bins within the current module. Coverage storage is also emitted
    // on each module object so no-inline instances keep independent counters
    // when forcePerInstance is used.
    පූර්ණ_සංඛ්‍යා m_modCoverBins = 0;  // Per-module coverage bin number
    const bool m_dpiHdrOnly;  // Only emit the DPI header
    VDouble0 m_statVarScopeBytes;  // Statistic tracking
    // name -> initializer rows, built in getSymCtorStmts()
    std::vector<std::pair<std::string, std::vector<std::string>>> m_varTables;
    // Single VlScopeTableEntry[] table for all scopes, built in getSymCtorStmts()
    std::string m_scopeTableName;
    std::vector<std::string> m_scopeTableRows;
    std::string m_ifaceRefTableName;
    std::vector<std::string> m_ifaceRefTableRows;

    // METHODS
    void emitSymHdr();
    void emitSymImpPreamble();
    void emitVarTables();
    void emitScopeHier(TextStmts& stmts, bool destroy);
    void emitSymImp(const AstNetlist* netlistp);
    void emitDpiHdr();
    void emitDpiStruct(const AstNodeDType* dtypep, std::set<std::string>& namesr);
    void emitDpiImp();

    TextStmts getSymCtorStmts();
    TextStmts getSymDtorStmts();

    static void nameCheck(AstNode* nodep) {
        // Prevent GCC compile time error; name check all things that reach C++ code
        නම් (nodep->name().empty()) ආපසු;
        const AstCFunc* const cfuncp = VN_CAST(nodep, CFunc);
        නම් (!cfuncp || (!cfuncp->isConstructor() && !cfuncp->isDestructor())) {
            const std::string rsvd = V3LanguageWords::isKeyword(nodep->name());
            නම් (rsvd != "") {
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

    static std::string scopeSymString(const std::string& scpname) {
        std::string out = scpname;
        std::string::size_type pos;
        අතරතුර ((pos = out.find("__PVT__")) != std::string::npos) out.replace(pos, 7, "");
        නම් (VString::startsWith(out, "TOP__DOT__")) out.replace(0, 10, "");
        නම් (VString::startsWith(out, "TOP.")) out.replace(0, 4, "");
        අතරතුර ((pos = out.find('.')) != std::string::npos) out.replace(pos, 1, "__");
        අතරතුර ((pos = out.find("__DOT__")) != std::string::npos) out.replace(pos, 7, "__");
        ආපසු out;
    }

    static string scopeDecodeIdentifier(const std::string& scpname) {
        std::string::size_type pos = std::string::npos;

        // Remove hierarchy
        size_t i = 0;
        // always makes progress
        අතරතුර (i < scpname.length()) {
            නම් (scpname[i] == '\\') {
                අතරතුර (i < scpname.length() && scpname[i] != ' ') ++i;
                ++i;  // Proc ' ', it should always be there. Then grab '.' on next cycle
            } නැතහොත් {
                අතරතුර (i < scpname.length() && scpname[i] != '.') ++i;
                නම් (i < scpname.length()) pos = i++;
            }
        }

        ආපසු pos != std::string::npos ? scpname.substr(pos + 1) : scpname;
    }

    // Encounter order of one dtype-chain walk: unpacked (outer), then packed,
    // then a ranged basic leaf.
    sa_nracanaa VarDims final {
        පූර්ණ_සංඛ්‍යා pdim = 0;  // Packed array dims plus a ranged basic leaf
        පූර්ණ_සංඛ්‍යා udim = 0;
        std::vector<std::pair<පූර්ණ_සංඛ්‍යා, පූර්ණ_සංඛ්‍යා>> unpackedLR;  // (left,right), outer-first
        std::vector<std::pair<පූර්ණ_සංඛ්‍යා, පූර්ණ_සංඛ්‍යා>> packedLR;  // (left,right), inner then leaf

        // Shared order used by both boundsString() and the table-row dv[] fill.
        std::vector<std::pair<පූර්ණ_සංඛ්‍යා, පූර්ණ_සංඛ්‍යා>> flatten() const {
            std::vector<std::pair<පූර්ණ_සංඛ්‍යා, පූර්ණ_සංඛ්‍යා>> flat(unpackedLR);
            flat.insert(flat.end(), packedLR.cbegin(), packedLR.cend());
            ආපසු flat;
        }
    };
    static VarDims getVarDims(const AstNodeDType* const rootDtypep) {
        VarDims d;
        // Range is always first, it's not in "C" order
        සඳහා (const AstNodeDType* dtypep = rootDtypep; dtypep;) {
            // Skip AstRefDType/AstTypedef, or return same node
            dtypep = dtypep->skipRefp();
            නම් (const AstNodeArrayDType* const adtypep = VN_CAST(dtypep, NodeArrayDType)) {
                නම් (VN_IS(dtypep, PackArrayDType)) {
                    d.packedLR.emplace_back(adtypep->left(), adtypep->right());
                    ++d.pdim;
                } නැතහොත් {
                    d.unpackedLR.emplace_back(adtypep->left(), adtypep->right());
                    ++d.udim;
                }
                dtypep = adtypep->subDTypep();
            } නැතහොත් {
                නම් (const AstBasicDType* const basicp = dtypep->basicp()) {
                    නම් (basicp->isRanged()) {
                        d.packedLR.emplace_back(basicp->left(), basicp->right());
                        ++d.pdim;
                    }
                }
                bha_nga;  // Non-array leaf
            }
        }
        ආපසු d;
    }

    static VarDims dimsFor(const ScopeVarData& svd) { ආපසු getVarDims(svd.m_varp->dtypep()); }

    // Only needed on the residual path; the table path uses dims.flatten() directly.
    static std::string boundsString(const VarDims& d) {
        std::string bounds;
        සඳහා (const std::pair<පූර්ණ_සංඛ්‍යා, පූර්ණ_සංඛ්‍යා>& lr : d.flatten()) {
            bounds += " ,";
            bounds += std::to_string(lr.first);
            bounds += ",";
            bounds += std::to_string(lr.second);
        }
        ආපසු bounds;
    }

    static පූර්ණ_සංඛ්‍යා getUnpackedElements(const AstNodeDType* rootDtypep) {
        පූර්ණ_සංඛ්‍යා elements = 1;
        සඳහා (const AstNodeDType* dtypep = rootDtypep; dtypep;) {
            dtypep = dtypep->skipRefp();
            const AstUnpackArrayDType* const adtypep = VN_CAST(dtypep, UnpackArrayDType);
            නම් (!adtypep) bha_nga;
            elements *= adtypep->elementsConst();
            dtypep = adtypep->subDTypep();
        }
        ආපසු elements;
    }

    static bool needsEmittedEntSize(const std::string& vlEnumType) {
        ආපසු vlEnumType == "VLVT_STRUCT" || vlEnumType == "VLVT_UNION";
    }

    static std::pair<bool, std::string> isForceControlSignal(const AstVar* const signalVarp) {
        // __VforceRd should not show up here because it is never public, but just in case it does,
        // it should be skipped because forceableVarInsert creates its VerilatedVar.
        සඳහා (const std::string forceControlSuffix : {"__VforceEn", "__VforceVal", "__VforceRd"}) {
            const std::size_t suffixPos = signalVarp->name().find(forceControlSuffix);
            const bool suffixFound = suffixPos != std::string::npos;
            const bool suffixIsAtEnd
                = suffixPos + forceControlSuffix.length() == signalVarp->name().length();
            නම් (!(suffixFound && suffixIsAtEnd)) continue;
            const std::string baseSignalName = signalVarp->name().substr(0, suffixPos);
            ආපසු std::pair<bool, std::string>{true, baseSignalName};
        }
        ආපසු std::pair<bool, std::string>{false, ""};
    }

    static void appendVarProperties(std::string& stmt, const std::string& vlEnumType,
                                    const std::string& vlEnumDir, const පූර්ණ_සංඛ්‍යා udim, const පූර්ණ_සංඛ්‍යා pdim,
                                    const std::string& bounds, const std::string& entSize = "") {
        stmt += vlEnumType;  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += vlEnumDir;  // VLVD_IN etc
        stmt += ", ";
        stmt += std::to_string(udim);
        නම් (!entSize.empty()) {
            stmt += ", ";
            stmt += entSize;
        } නැතහොත් {
            stmt += ", ";
            stmt += std::to_string(pdim);
        }
        stmt += bounds;
        stmt += ")";
    }
    static std::string memberVlEnumDir(const AstVar* const varp,
                                       const AstNodeDType* const dtypep) {
        std::string out = '(' + varp->vlEnumDir(/*forMember=*/true);
        const AstNodeDType* const skipDTypep = dtypep->skipRefp();
        නම් (skipDTypep->isSigned()) out += "|VLVF_SIGNED";
        නම් (const AstBasicDType* const basicp = skipDTypep->basicp()) {
            නම් (basicp->keyword() == VBasicDTypeKwd::BIT) out += "|VLVF_BITVAR";
        }
        out += skipDTypep->vlEnumPackedAgg();
        ආපසු out + ')';
    }

    // Name of the instance member for the given scope
    static std::string instName(const AstScope* scopep) {
        ආපසු "vlInst." + VIdProtect::protectIf(scopep->nameDotless(), scopep->protect());
    }

    static std::string insertVarStatement(const ScopeVarData& svd, const AstScope* const scopep,
                                          const AstVar* const varp, const පූර්ණ_සංඛ්‍යා udim, const පූර්ණ_සංඛ්‍යා pdim,
                                          const std::string& bounds) {
        const std::string varExpr = "vlSymsp->" + instName(scopep) + "." + varName(varp);
        const std::string vlEnumType = varp->vlEnumType();
        const bool needsEntSize = needsEmittedEntSize(vlEnumType);

        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        නම් (!varp->isParam()) {
            stmt += ", &(";
            stmt += varExpr;
            stmt += "), false, ";
        } නැතහොත් නම් (varp->vlEnumType() == "VLVT_STRING"
                   && !VN_IS(varp->subDTypep(), UnpackArrayDType)) {
            stmt += ", const_cast<void*>(static_cast<const void*>(";
            stmt += varExpr;
            stmt += ".c_str())), true, ";
        } නැතහොත් {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += varExpr;
            stmt += "))), true, ";
        }

        const std::string entSize = needsEntSize
                                        ? "sizeof(" + varExpr + ") / "
                                              + std::to_string(getUnpackedElements(varp->dtypep()))
                                        : "";
        appendVarProperties(stmt, vlEnumType, varp->vlEnumDir(), udim, pdim, bounds, entSize);
        ආපසු stmt;
    }

    // tryBuildTableEntry() classifies each public var once, so the caller does
    // not re-derive (and risk desyncing) the forceable-eligibility test.
    ga_nanaa class TableEntryKind : uint8_t {
        TABLE_ROW,  // Registered via a VlVarTableEntry[] table row
        FORCEABLE_RESIDUAL,  // Residual, via insertForceableVarStatement()
        PLAIN_RESIDUAL,  // Residual, via insertVarStatement() (+ struct/array expansion)
    };

    // Builds one VlVarTableEntry row for 'varp', or reports which residual
    // path it must take instead.
    TableEntryKind tryBuildTableEntry(const ScopeVarData& svd, const AstVar* const varp,
                                      const AstScope* const scopep,
                                      const std::string& modClassName, const VarDims& dims,
                                      std::string& rowOut) const {
        const පූර්ණ_සංඛ්‍යා pdim = dims.pdim;
        const පූර්ණ_සංඛ්‍යා udim = dims.udim;
        නම් (varp->isForceable() && forceControlSignalsAreValid(scopep, varp)) {
            ආපසු TableEntryKind::FORCEABLE_RESIDUAL;
        }
        නම් (udim + pdim > VPI_TABLE_MAX_DIMS) ආපසු TableEntryKind::PLAIN_RESIDUAL;

        const std::string vlEnumType = varp->vlEnumType();
        නම් (needsEmittedEntSize(vlEnumType))
            ආපසු TableEntryKind::PLAIN_RESIDUAL;  // struct/union whole
        // Params are static members (offsetof is invalid on those);
        // string params also need a runtime .c_str().
        නම් (varp->isParam()) ආපසු TableEntryKind::PLAIN_RESIDUAL;

        const std::string name = V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty));
        // nameProtect() (not protect(name())) so the offsetof member matches the
        // emitted struct field: a primary I/O port keeps its unprotected name
        // under --protect-ids, whereas protect() would always hash it.
        const std::string member = varName(varp);
        const std::string dir = varp->vlEnumDir();
        // Flat dim (left,right) ints in varInsert() order: unpacked then packed.
        std::vector<පූර්ණ_සංඛ්‍යා> dv;
        සඳහා (const std::pair<පූර්ණ_සංඛ්‍යා, පූර්ණ_සංඛ්‍යා>& lr : dims.flatten()) {
            dv.push_back(lr.first);
            dv.push_back(lr.second);
        }

        std::string row = "{\"" + name + "\", ";
        row += "offsetof(" + modClassName + ", " + member + "), ";
        row += vlEnumType + ", ";
        row += "(" + dir + "), ";
        row += std::to_string(udim) + ", " + std::to_string(pdim) + ", {";
        සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < VPI_TABLE_MAX_DIMS * 2; ++i) {
            නම් (i) row += ", ";
            row += std::to_string(i < static_cast<පූර්ණ_සංඛ්‍යා>(dv.size()) ? dv[i] : 0);
        }
        row += "}}";
        rowOut = row;
        ආපසු TableEntryKind::TABLE_ROW;
    }

    static std::string insertDTypeVarStatement(const ScopeVarData& svd,
                                               const AstScope* const scopep,
                                               const std::string& prettyName,
                                               const std::string& cName,
                                               const AstNodeDType* const dtypep, const පූර්ණ_සංඛ්‍යා udim,
                                               const පූර්ණ_සංඛ්‍යා pdim, const std::string& bounds) {
        const std::string vlEnumType = dtypep->vlEnumType();
        const bool needsEntSize = needsEmittedEntSize(vlEnumType);
        std::string stmt;
        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
        stmt += needsEntSize ? "->varInsertSized(\"" : "->varInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
        const std::string varExpr = "vlSymsp->" + instName(scopep) + "." + cName;
        // A parameter is emitted 'static const', so its members are const too and
        // need the same cast the whole-parameter insert uses.
        නම් (svd.m_varp->isParam()) {
            stmt += ", const_cast<void*>(static_cast<const void*>(&(";
            stmt += varExpr;
            stmt += "))), true, ";
        } නැතහොත් {
            stmt += ", &(";
            stmt += varExpr;
            stmt += "), false, ";
        }
        const std::string entSize
            = needsEntSize
                  ? "sizeof(" + varExpr + ") / " + std::to_string(getUnpackedElements(dtypep))
                  : "";
        appendVarProperties(stmt, vlEnumType, memberVlEnumDir(svd.m_varp, dtypep), udim, pdim,
                            bounds, entSize);
        ආපසු stmt;
    }

    static void addUOrStructMemberVars(std::vector<std::string>& stmts, const ScopeVarData& svd,
                                       const AstScope* const scopep,
                                       const std::string& prettyPrefix, const std::string& cPrefix,
                                       const AstNodeUOrStructDType* const sdtypep) {
        සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            const AstNodeDType* const itemDTypep = itemp->dtypep();
            නම් (itemDTypep->vlEnumType().empty()) continue;
            const std::string prettyName
                = prettyPrefix + "." + AstNode::vpiName(itemp->shortName());
            const std::string cName = cPrefix + "." + itemp->nameProtect();
            const VarDims dims = getVarDims(itemDTypep);
            stmts.emplace_back(insertDTypeVarStatement(svd, scopep, prettyName, cName, itemDTypep,
                                                       dims.udim, dims.pdim, boundsString(dims))
                               + ";");
            addPackedAggMemberVars(stmts, svd, scopep, prettyName, cName, itemDTypep);
            නම් (const AstNodeUOrStructDType* const subp
                = VN_CAST(itemDTypep->skipRefp(), NodeUOrStructDType)) {
                නම් (!subp->packed())
                    addUOrStructMemberVars(stmts, svd, scopep, prettyName, cName, subp);
            } නැතහොත් නම් (VN_IS(itemDTypep->skipRefp(), UnpackArrayDType)) {
                addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyName, cName,
                                                    itemDTypep);
            }
        }
    }

    static void addUnpackedArrayUOrStructMemberVars(std::vector<std::string>& stmts,
                                                    const ScopeVarData& svd,
                                                    const AstScope* const scopep,
                                                    const std::string& prettyPrefix,
                                                    const std::string& cPrefix,
                                                    const AstNodeDType* const dtypep) {
        const AstNodeDType* const skipDTypep = dtypep->skipRefp();
        නම් (const AstUnpackArrayDType* const adtypep = VN_CAST(skipDTypep, UnpackArrayDType)) {
            addUnpackedArrayUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix + "[0]",
                                                adtypep->subDTypep());
        } නැතහොත් නම් (const AstNodeUOrStructDType* const sdtypep
                   = VN_CAST(skipDTypep, NodeUOrStructDType)) {
            නම් (!sdtypep->packed())
                addUOrStructMemberVars(stmts, svd, scopep, prettyPrefix, cPrefix, sdtypep);
        }
    }

    // Packed struct/union members have no storage of their own; each is registered as a bit
    // slice of the parent's storage 'cName', with the parent's storage type and entSize, and its
    // bit offset within the parent's element 0. VPI adds any packed/unpacked index offset.
    // 'needsIndex' if an ancestor is an array, so the members are only valid after an index.
    static void addPackedMemberVars(std::vector<std::string>& stmts, const ScopeVarData& svd,
                                    const AstScope* const scopep, const std::string& prettyPrefix,
                                    const std::string& cName, const std::string& vlEnumType,
                                    const std::string& entSize,
                                    const AstNodeUOrStructDType* const sdtypep,
                                    const පූර්ණ_සංඛ්‍යා parentBitOffset, const bool needsIndex) {
        const std::string varExpr = "vlSymsp->" + instName(scopep) + "." + cName;
        සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            const AstNodeDType* const itemDTypep = itemp->dtypep();
            const std::string prettyName
                = prettyPrefix + "." + AstNode::vpiName(itemp->shortName());
            // Union members all have lsb 0, so share the parent's offset
            const පූර්ණ_සංඛ්‍යා bitOffset = parentBitOffset + itemp->lsb();
            const VarDims dims = getVarDims(itemDTypep);
            UASSERT_OBJ(dims.udim == 0, itemp, "Unpacked dimension in packed struct/union");
            std::string stmt;
            stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName);
            stmt += "->varInsertMember(\"";
            stmt += V3OutFormatter::quoteNameControls(prettyName) + '"';
            // A parameter is emitted 'static const', see insertDTypeVarStatement
            නම් (svd.m_varp->isParam()) {
                stmt
                    += ", const_cast<void*>(static_cast<const void*>(&(" + varExpr + "))), true, ";
            } නැතහොත් {
                stmt += ", &(" + varExpr + "), false, ";
            }
            stmt += vlEnumType + ", ";
            stmt += memberVlEnumDir(svd.m_varp, itemDTypep);
            නම් (needsIndex) stmt += "|VLVF_PACKED_NEEDS_INDEX";
            stmt += ", ";
            stmt += std::to_string(bitOffset) + ", ";
            stmt += entSize + ", ";
            stmt += std::to_string(dims.pdim);
            stmt += boundsString(dims);
            stmt += ");";
            stmts.emplace_back(stmt);
            // Nested packed struct/union, or packed array of them: register the members of
            // the element at index offset 0
            const AstNodeDType* subDTypep = itemDTypep->skipRefp();
            bool isArray = false;
            අතරතුර (const AstPackArrayDType* const adtypep = VN_CAST(subDTypep, PackArrayDType)) {
                subDTypep = adtypep->subDTypep()->skipRefp();
                isArray = true;
            }
            නම් (const AstNodeUOrStructDType* const subp = VN_CAST(subDTypep, NodeUOrStructDType)) {
                addPackedMemberVars(stmts, svd, scopep, prettyName, cName, vlEnumType, entSize,
                                    subp, bitOffset, needsIndex || isArray);
            }
        }
    }

    // If 'dtypep' (the type of storage 'cName') has packed struct/union elements, register the
    // members of its element 0
    static void addPackedAggMemberVars(std::vector<std::string>& stmts, const ScopeVarData& svd,
                                       const AstScope* const scopep,
                                       const std::string& prettyPrefix, const std::string& cName,
                                       const AstNodeDType* const dtypep) {
        std::string elemCName = cName;
        const AstNodeDType* elemDTypep = dtypep->skipRefp();
        bool isArray = false;
        අතරතුර (const AstNodeArrayDType* const adtypep = VN_CAST(elemDTypep, NodeArrayDType)) {
            නම් (VN_IS(adtypep, UnpackArrayDType)) elemCName += "[0]";
            elemDTypep = adtypep->subDTypep()->skipRefp();
            isArray = true;
        }
        const AstNodeUOrStructDType* const sdtypep = VN_CAST(elemDTypep, NodeUOrStructDType);
        නම් (!sdtypep || !sdtypep->packed()) ආපසු;
        const std::string varExpr = "vlSymsp->" + instName(scopep) + "." + cName;
        const std::string entSize
            = "sizeof(" + varExpr + ") / " + std::to_string(getUnpackedElements(dtypep));
        addPackedMemberVars(stmts, svd, scopep, prettyPrefix, elemCName, dtypep->vlEnumType(),
                            entSize, sdtypep, 0, isArray);
    }

    std::string insertForceableVarStatement(const ScopeVarData& svd, const AstScope* const scopep,
                                            const AstVar* const varp, const පූර්ණ_සංඛ්‍යා udim,
                                            const පූර්ණ_සංඛ්‍යා pdim, const std::string& bounds) {
        std::string stmt;

        stmt += "vlSymsp->" + protect("__Vscopep_" + svd.m_scopeName) + "->forceableVarInsert(\"";
        stmt += V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty)) + '"';

        const std::string varExpr = "vlSymsp->" + instName(scopep) + "." + varName(varp);

        assert(!varp->isParam());  // Forceable params do not make sense
        stmt += ", &(";
        stmt += varExpr;
        stmt += "), false, ";

        stmt += varp->vlEnumType();  // VLVT_UINT32 etc
        stmt += ", ";
        stmt += varp->vlEnumDir();  // VLVD_IN etc
        stmt += ", &(";
        stmt += varExpr + "__VforceRd";
        stmt += "), \"" + V3OutFormatter::quoteNameControls(protect(svd.m_varBasePretty))
                + "__VforceRd" + '"';
        stmt += ", {";

        සඳහා (const std::string forceControlSuffix : {"__VforceEn", "__VforceVal"}) {
            const std::string enableSignalKey
                = getKeyName(scopep, varp->name() + forceControlSuffix);
            const std::map<const std::string, ScopeVarData>::const_iterator itpair
                = m_scopeVars.find(enableSignalKey);

            නම් (itpair == m_scopeVars.end()) {
                varp->v3fatalSrc("Signal "
                                 << varp->prettyNameQ()
                                 << " is marked forceable, but the force control signal '"
                                 << varp->name() << forceControlSuffix
                                 << "' can not be found in m_scopeVars with key '"
                                 << enableSignalKey << "'.");
            }

            const ScopeVarData& svd = itpair->second;
            const AstScope* const scopep = svd.m_scopep;
            const AstVar* const varp = svd.m_varp;
            const VarDims dims = dimsFor(svd);
            stmt
                += insertVarStatement(svd, scopep, varp, dims.udim, dims.pdim, boundsString(dims));
            නම් (forceControlSuffix == "__VforceEn") stmt += ",";
        }

        stmt += "}";
        stmt += ", ";
        stmt += std::to_string(udim);
        stmt += ", ";
        stmt += std::to_string(pdim);
        stmt += bounds;
        stmt += ")";
        ආපසු stmt;
    }

    static std::string getKeyName(const AstScope* const scopep, const std::string& signal_name) {
        // Copies the process from `addScopeVarEntry` which created the keys in the first place, in
        // order
        // signal can be found.
        std::string whole = scopep->name() + "__DOT__" + signal_name;
        std::string scpName;
        නම් (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        const std::string::size_type dpos = whole.rfind("__DOT__");
        නම් (dpos != std::string::npos) { scpName = whole.substr(0, dpos); }
        const std::string scpSym = scopeSymString(VName::dehash(scpName));
        ආපසු scpSym + " " + signal_name;
    }

    bool baseSignalIsValid(const AstScope* const scopep, const AstVar* const controlSignalVarp,
                           const std::string& baseSignalName) const {
        const std::string baseSignalKey = getKeyName(scopep, baseSignalName);
        const std::map<const std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(baseSignalKey);
        නම් (baseSignalIt == m_scopeVars.end()) {
            // This means that the signal is forceable, but not public, so only the force control
            // signals show up in the m_scopeVars, but not the base signal itself.
            // Expect this branch not to be hit, because `baseSignalIsPublic` is checked before
            // this, so this function will not get called in that case.
            ආපසු false;
        } නැතහොත් {
            const AstVar* const baseSignalVarp = baseSignalIt->second.m_varp;
            නම් (m_scopeVars.count(baseSignalName) > 1) {
                baseSignalVarp->v3fatalSrc(
                    "Found Signal " << baseSignalName
                                    << ", but also found at least one other signal with name"
                                    << baseSignalName << " occurring in the same scope.");
                ආපසු false;
            }
            නම් (!baseSignalVarp->isForceable()) {
                controlSignalVarp->v3fatalSrc(
                    "Found signal " << controlSignalVarp->prettyNameQ()
                                    << " which is a force control signal, but the base signal "
                                    << baseSignalVarp->prettyNameQ()
                                    << " is not marked as forceable.");
                ආපසු false;
            }
        }
        ආපසු true;
    }

    bool baseSignalIsPublic(const AstScope* const scopep,
                            const std::string& baseSignalName) const {
        const std::string baseSignalKey = getKeyName(scopep, baseSignalName);
        const std::map<const std::string, ScopeVarData>::const_iterator baseSignalIt
            = m_scopeVars.find(baseSignalKey);
        නම් (baseSignalIt == m_scopeVars.end()) {
            ආපසු false;
        } නැතහොත් {
            const AstVar* const baseSignalVarp = baseSignalIt->second.m_varp;
            // Should not actually occur if the variable is in the m_scopeVars
            නම් (!baseSignalVarp->isSigPublic()) ආපසු false;
        }
        ආපසු true;
    }

    bool forceControlSignalsAreValid(const AstScope* const scopep,
                                     const AstVar* const baseSignalVarp) const {
        constexpr std::array<const var_na*, 2> forceControlSuffixes = {"__VforceEn", "__VforceVal"};
        ආපසු std::all_of(
            forceControlSuffixes.begin(), forceControlSuffixes.end(),
            [scopep, baseSignalVarp, this](const std::string& forceControlSuffix) {
                const std::string controlSignalName = baseSignalVarp->name() + forceControlSuffix;
                const std::string controlSignalKey = getKeyName(scopep, controlSignalName);
                const std::size_t controlSignalCount = m_scopeVars.count(controlSignalKey);
                නම් (controlSignalCount == 0) {
                    baseSignalVarp->v3fatalSrc("Signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " is marked forceable, but the control signal '"
                                               << controlSignalName
                                               << "' can not be found in m_scopeVars with key '"
                                               << controlSignalKey << "'.");
                    ආපසු false;
                }
                නම් (controlSignalCount > 1) {
                    baseSignalVarp->v3fatalSrc("The control signal '"
                                               << controlSignalName << "' for forceable signal "
                                               << baseSignalVarp->prettyNameQ()
                                               << " occurs several times within the same scope.");
                    ආපසු false;
                }
                ආපසු true;
            });
    }

    /// (scp, m_vpiScopeCandidates, m_scopeNames) -> m_scopeNames
    /// Look for parent scopes of scp in m_vpiScopeCandidates (separated by __DOT__ or ".")
    /// Then add/update entry in m_scopeNames if not already there
    void varHierarchyScopes(std::string scp) {

        std::string::size_type prd_pos = scp.rfind('.');
        std::string::size_type dot_pos = scp.rfind("__DOT__");

        අතරතුර (!scp.empty()) {
            const auto scpit = m_vpiScopeCandidates.find(scopeSymString(scp));
            නම් ((scpit != m_vpiScopeCandidates.end())
                && (m_scopeNames.find(scp) == m_scopeNames.end())) {
                // If not in m_scopeNames, add it, otherwise just update m_type
                const auto pair = m_scopeNames.emplace(scpit->second.m_symName, scpit->second);
                නම් (!pair.second) pair.first->second.m_type = scpit->second.m_type;
            }

            // resize and advance pointers
            නම් ((prd_pos < dot_pos || prd_pos == string::npos) && dot_pos != string::npos) {
                scp.resize(dot_pos);
                dot_pos = scp.rfind("__DOT__");
            } නැතහොත් {
                නම් (prd_pos == string::npos) bha_nga;
                scp.resize(prd_pos);
                prd_pos = scp.rfind('.');
            }
        }
    }

    void addScopeVarEntry(const AstScope* const scopep, const AstNodeModule* const modp,
                          const AstVar* const varp) {
        // Need to split the module + var name into the original-ish full scope
        // and variable name under that scope. The module instance name is
        // included later, when we know the scopes this module is under.
        std::string whole = scopep->name() + "__DOT__" + varp->name();
        නම් (VString::startsWith(whole, "__DOT__TOP")) whole.replace(0, 10, "");
        const std::string::size_type dpos = whole.rfind("__DOT__");
        UASSERT_OBJ(dpos != std::string::npos, varp,
                    "Scope/variable name lost its appended __DOT__ separator");
        const std::string scpName = whole.substr(0, dpos);
        const std::string varBase = whole.substr(dpos + std::strlen("__DOT__"));
        // UINFO(9, "For " << scopep->name() << " - " << varp->name() << "  Scp "
        // << scpName << "Var " << varBase);
        const std::string varBasePretty = AstNode::vpiName(VName::dehash(varBase));
        const std::string scpPretty = AstNode::prettyName(VName::dehash(scpName));
        const std::string scpSym = scopeSymString(VName::dehash(scpName));
        // UINFO(9, " scnameins sp " << scpName << " sp " << scpPretty << " ss "
        // << scpSym);
        නම් (v3Global.opt.vpi()) varHierarchyScopes(scpName);

        m_scopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym),  //
            std::forward_as_tuple(varp, scpSym, scpPretty, "<null>", 0, "SCOPE_OTHER"));

        m_scopeVars.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scpSym + " " + varp->name()),  //
            std::forward_as_tuple(scpSym, varBasePretty, varp, modp, scopep));
    }

    void varsExpand() {
        // We didn't have all m_scopes loaded when we encountered variables, so expand them now
        // It would be less code if each module inserted its own variables.
        // Someday.
        සඳහා (const ScopeModPair& smPair : m_scopes) {
            const AstScope* const scopep = smPair.first;
            const AstNodeModule* const smodp = smPair.second;
            සඳහා (const ModVarPair& mvPair : m_modVars) {
                const AstNodeModule* const modp = mvPair.first;
                const AstVar* const varp = mvPair.second;
                නම් (modp != smodp) continue;
                addScopeVarEntry(scopep, modp, varp);
            }
        }
    }

    void collectIfaceRefs(const AstScope* nodep) {
        const AstCell* const cellp = nodep->aboveCellp();
        // Exclude classes inside interfaces; the cell's modp is the interface, not the Class
        නම් (!cellp || !VN_IS(cellp->modp(), Iface) || !VN_IS(nodep->modp(), Iface)) ආපසු;

        // vpiName() to match the scope table these are looked up alongside. Inlining
        // flattens the hierarchy but leaves the inlined levels in the scope name, which
        // therefore carries the full enclosing path.
        const std::string path = AstNode::vpiName(nodep->name());
        const std::string instName = AstNode::vpiName(cellp->origName());
        UASSERT_OBJ(path.length() > instName.length() && VString::endsWith(path, instName), nodep,
                    "Interface scope name " << path << " does not end with instance name "
                                            << instName);
        const std::string parentPath = path.substr(0, path.length() - instName.length());

        සඳහා (AstIntfRef* intfRefp = cellp->intfRefsp(); intfRefp;
             intfRefp = VN_AS(intfRefp->nextp(), IntfRef)) {
            const std::string refName = AstNode::vpiName(intfRefp->name());
            // Assume only references under the same parent scope reference the
            // same interface. Same limitation as the trace path in V3TraceDecl.
            නම් (!VString::startsWith(refName, parentPath)) continue;
            m_ifaceRefs.emplace_back(nodep, scopeSymString(intfRefp->parentName()), refName,
                                     AstNode::vpiName(intfRefp->baseName()),
                                     intfRefp->modportName());
        }
    }

    void buildIfaceRefTable() {
        නම් (m_ifaceRefs.empty()) ආපසු;
        const std::string symClass = symClassName();
        // Sorted by path, as VPI iteration yields references in table order
        std::stable_sort(
            m_ifaceRefs.begin(), m_ifaceRefs.end(),
            [](const IfaceRefData& a, const IfaceRefData& b) { ආපසු a.m_suffix < b.m_suffix; });
        සඳහා (const IfaceRefData& ird : m_ifaceRefs) {
            const std::string scopeSym = scopeSymString(ird.m_scopep->name());
            // Only reference scopes that actually made it into the scope table
            නම් (m_scopeNames.find(scopeSym) == m_scopeNames.end()) continue;
            // The declaring scope may have been omitted from the public scope table.
            // Keep such references reachable by name, without attaching them to a scope.
            std::string parentOffset = "VL_IFACEREF_NO_PARENT";
            const auto parent = m_scopeNames.find(ird.m_parentSym);
            නම් (parent != m_scopeNames.end()) {
                parentOffset = "offsetof(" + symClass + ", "
                               + protect("__Vscopep_" + parent->second.m_symName) + ")";
            }
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + scopeSym) + "), ";
            row += parentOffset + ", \"";
            row += V3OutFormatter::quoteNameControls(VIdProtect::protectWordsIf(ird.m_name, true));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protectWordsIf(ird.m_suffix, true));
            row += "\", \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protectWordsIf(ird.m_modportName, true));
            row += "\"}";
            m_ifaceRefTableRows.emplace_back(std::move(row));
        }
        නම් (!m_ifaceRefTableRows.empty()) m_ifaceRefTableName = symClass + "__VpiIfaceRefTable";
    }

    void buildVpiHierarchy() {
        සඳහා (const auto& itpair : m_scopeNames) {
            const std::string symName = itpair.second.m_symName;
            std::string above = symName;
            නම් (VString::startsWith(above, "TOP.")) above.replace(0, 4, "");

            අතරතුර (!above.empty()) {
                const std::string::size_type pos = above.rfind("__");
                නම් (pos == std::string::npos) bha_nga;
                above.resize(pos);
                නම් (m_vpiScopeHierarchy.find(above) != m_vpiScopeHierarchy.end()) {
                    m_vpiScopeHierarchy[above].push_back(symName);
                    bha_nga;
                }
            }
            m_vpiScopeHierarchy[symName] = std::vector<string>();
        }
    }

    // VISITORS
    void visit(AstNetlist* nodep) override {
        // Collect list of scopes
        iterateChildrenConst(nodep);
        varsExpand();

        නම් (v3Global.opt.vpi()) buildVpiHierarchy();

        නම් (v3Global.dpi()) {
            // add dpi scopes to m_scopeNames if not already there
            සඳහා (const auto& scp : m_dpiScopeNames) m_scopeNames.emplace(scp.first, scp.second);
        }

        // Sort by names, so line/process order matters less
        std::stable_sort(m_scopes.begin(), m_scopes.end(),
                         [](const ScopeModPair& a, const ScopeModPair& b) {
                             ආපසු a.first->name() < b.first->name();
                         });
        std::stable_sort(m_dpis.begin(), m_dpis.end(),  //
                         [](const AstCFunc* ap, const AstCFunc* bp) {
                             නම් (ap->dpiImportPrototype() != bp->dpiImportPrototype()) {
                                 ආපසු bp->dpiImportPrototype();
                             }
                             ආපසු ap->name() < bp->name();
                         });

        // Output
        නම් (!m_dpiHdrOnly) {
            // Must emit implementation first to determine number of splits
            emitSymImp(nodep);
            emitSymHdr();
        }
        නම් (v3Global.dpi()) {
            emitDpiHdr();
            නම් (!m_dpiHdrOnly) emitDpiImp();
        }
    }
    void visit(AstNodeModule* nodep) override {
        නම් (nodep->isConstPool()) ආපසු;  // Special emit rules
        nameCheck(nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modCoverBins);
        m_modp = nodep;
        // Restart bin numbering for the object-local coverage array of this
        // module class.
        m_modCoverBins = 0;
        iterateChildrenConst(nodep);
    }
    void visit(AstCellInlineScope* nodep) override {
        const std::string type = (nodep->origModName() == "__BEGIN__") ? "SCOPE_OTHER"  //
                                                                       : "SCOPE_MODULE";
        const std::string name = m_scopep->shortName() + "__DOT__" + nodep->name();
        const පූර්ණ_සංඛ්‍යා timeunit = m_modp->timeunit().powerOfTen();
        m_vpiScopeCandidates.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(scopeSymString(name)),  //
            std::forward_as_tuple(nodep, scopeSymString(name), AstNode::vpiName(name),
                                  type == "SCOPE_MODULE" ? nodep->origModName() : "<null>",
                                  timeunit, type));
    }
    void visit(AstScope* nodep) override {
        නම් (VN_IS(m_modp, Class)) ආපසු;  // The ClassPackage is what is visible
        nameCheck(nodep);

        m_scopes.emplace_back(nodep, m_modp);

        නම් (v3Global.opt.vpi() && !nodep->isTop()) {
            const std::string type = VN_IS(nodep->modp(), Package) ? "SCOPE_PACKAGE"  //
                                     : VN_IS(nodep->modp(), Iface) ? "SCOPE_INTERFACE"  //
                                                                   : "SCOPE_MODULE";
            const පූර්ණ_සංඛ්‍යා timeunit = m_modp->timeunit().powerOfTen();
            m_vpiScopeCandidates.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(scopeSymString(nodep->name())),  //
                std::forward_as_tuple(nodep, scopeSymString(nodep->name()),
                                      AstNode::vpiName(nodep->shortName()),
                                      nodep->modp()->origName(), timeunit, type));
            collectIfaceRefs(nodep);
        }
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    void visit(AstScopeName* nodep) override {
        නම් (nodep->selfRelative()) ආපසු;  // Its absolute copy is under the scope
        const std::string name = nodep->scopeSymName();
        // UINFO(9, "scnameins sp " << nodep->name() << " sp " << nodep->scopePrettySymName()
        // << " ss" << name);
        const පූර්ණ_සංඛ්‍යා timeunit = m_modp ? m_modp->timeunit().powerOfTen() : 0;
        m_dpiScopeNames.emplace(  //
            std::piecewise_construct,  //
            std::forward_as_tuple(name),  //
            std::forward_as_tuple(nodep, name, nodep->scopePrettySymName(), "<null>", timeunit,
                                  "SCOPE_OTHER"));

        නම් (nodep->dpiExport()) {
            UASSERT_OBJ(m_cfuncp, nodep, "ScopeName not under DPI function");
            m_scopeFuncs.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(name + " " + m_cfuncp->name()),  //
                std::forward_as_tuple(nodep, m_cfuncp, m_modp));
        } නැතහොත් {
            // Note emplace does not construct when duplicate key
            m_dpiScopeNames.emplace(  //
                std::piecewise_construct,  //
                std::forward_as_tuple(nodep->scopeDpiName()),  //
                std::forward_as_tuple(nodep, nodep->scopeDpiName(), nodep->scopePrettyDpiName(),
                                      "<null>", timeunit, "SCOPE_OTHER"));
        }
    }
    void visit(AstVar* nodep) override {
        nameCheck(nodep);
        iterateChildrenConst(nodep);
        // Record if public, ignoring locals and types VPI cannot represent
        නම් ((nodep->isSigUserRdPublic() || nodep->isSigUserRWPublic()) && !m_cfuncp
            && !nodep->vlEnumType().empty()) {
            m_modVars.emplace_back(m_modp, nodep);
        }
    }
    void visit(AstNodeCoverDecl* nodep) override {
        // Assign both global and module-local bin numbers. Most generated
        // coverage uses module-local counters, but static/package/class helper
        // functions may have no vlSelf and still need the global array.
        නම් (!nodep->dataDeclNullp()) {  // else duplicate we don't need code for
            nodep->binNum(m_coverBins);
            m_coverBins += nodep->size();
            nodep->localBinNum(m_modCoverBins);
            m_modCoverBins += nodep->size();
        }
    }
    void visit(AstCFunc* nodep) override {
        nameCheck(nodep);
        නම් (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) m_dpis.push_back(nodep);
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildrenConst(nodep);
    }

    //---------------------------------------
    void visit(AstConst*) override {}
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

public:
    explicit EmitCSyms(AstNetlist* nodep, bool dpiHdrOnly)
        : m_dpiHdrOnly{dpiHdrOnly} {
        iterateConst(nodep);
    }
};

void EmitCSyms::emitSymHdr() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputHeaderFile(symClassName(), "Symbol table internal header");
    puts("//\n");
    puts("// Internal details; most calling programs do not need this header,\n");
    puts("// unless using verilator public meta comments.\n");

    ofp()->putsGuard();

    puts("\n");
    ofp()->putsIntTopInclude();
    puts("#include \"verilated.h\"\n");
    නම් (v3Global.needTraceDumper()) {
        සඳහා (const string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");
    }
    නම් (v3Global.opt.usesProfiler()) puts("#include \"verilated_profiler.h\"\n");

    puts("\n// INCLUDE MODEL CLASS\n");
    puts("\n#include \"" + topClassName() + ".h\"\n");

    puts("\n// INCLUDE MODULE CLASSES\n");
    සඳහා (AstNodeModule *nodep = v3Global.rootp()->modulesp(), *nextp; nodep; nodep = nextp) {
        nextp = VN_AS(nodep->nextp(), NodeModule);
        නම් (VN_IS(nodep, Class)) continue;  // Class included earlier
        නම් (nodep->isConstPool()) continue;  // Special emit rules
        putns(nodep, "#include \"" + EmitCUtil::prefixNameProtect(nodep) + ".h\"\n");
    }

    නම් (v3Global.dpi()) {
        puts("\n// DPI TYPES for DPI Export callbacks (Internal use)\n");
        std::set<std::string> types;  // Remove duplicates and sort
        සඳහා (const auto& itpair : m_scopeFuncs) {
            const AstCFunc* const funcp = itpair.second.m_cfuncp;
            නම් (!funcp->dpiExportImpl()) continue;
            const std::string cbtype
                = protect(v3Global.opt.prefix() + "__Vcb_" + funcp->cname() + "_t");
            const std::string functype = funcp->rtnTypeVoid() + " (*) (" + cFuncArgs(funcp) + ")";
            types.emplace("using " + cbtype + " = " + functype + ";\n");
        }
        සඳහා (const std::string& type : types) puts(type);
    }

    puts("\n// SYMS CLASS (contains all model state)\n");
    puts("class alignas(VL_CACHE_LINE_BYTES) " + symClassName()
         + " final : public VerilatedSyms {\n");
    ofp()->putsPrivate(false);  // public:

    puts("// INTERNAL STATE\n");
    puts(topClassName() + "* const __Vm_modelp;\n");

    නම් (v3Global.needTraceDumper()) {
        // __Vm_dumperp is local, otherwise we wouldn't know what design's eval()
        // should call a global dumpperp
        puts("bool __Vm_dumping = false;  // Dumping is active\n");
        puts("VerilatedMutex __Vm_dumperMutex;  // Protect __Vm_dumperp\n");
        puts(v3Global.opt.traceClassLang()
             + "* __Vm_dumperp VL_GUARDED_BY(__Vm_dumperMutex) = nullptr;"
               "  /// Trace class for $dump*\n");
    }
    නම් (v3Global.opt.trace()) {
        puts("bool __Vm_activity = false;"
             "  ///< Used by trace routines to determine change occurred\n");
        puts("uint32_t __Vm_baseCode = 0;"
             "  ///< Used by trace routines when tracing multiple models\n");
    }
    නම් (v3Global.hasEvents()) {
        නම් (v3Global.assignsEvents()) {
            puts("std::vector<VlAssignableEvent> __Vm_triggeredEvents;\n");
        } නැතහොත් {
            puts("std::vector<VlEvent*> __Vm_triggeredEvents;\n");
        }
    }
    නම් (v3Global.hasClasses()) puts("VlDeleter __Vm_deleter;\n");
    puts("bool& __Vm_didInit;\n");
    නම් (v3Global.opt.systemC()) puts("sc_core::sc_event __Vm_wakeEvent;\n");

    නම් (v3Global.opt.mtasks()) {
        puts("\n// MULTI-THREADING\n");
        puts("VlThreadPool* __Vm_threadPoolp;\n");
        puts("bool __Vm_even_cycle__ico = false;\n");
        puts("bool __Vm_even_cycle__act = false;\n");
        puts("bool __Vm_even_cycle__nba = false;\n");
    }

    නම් (v3Global.opt.profExec()) {
        puts("\n// EXECUTION PROFILING\n");
        puts("VlExecutionProfiler* const __Vm_executionProfilerp;\n");
    }

    නම් (v3Global.opt.profPgo()) {
        puts("\n// PGO PROFILING\n");
        puts("VlPgoProfiler<" + std::to_string(ExecMTask::numUsedIds()) + "> _vm_pgoProfiler;\n");
    }

    puts("\n// MODULE INSTANCE STATE\n");
    // In an anonymous union so the C++ compiler does not construct them implicitly,
    // they are constructed/destroyed explicitly in the ctor/dtor functions.
    puts("union {\nstruct {\n");
    සඳහා (const ScopeModPair& itpair : m_scopes) {
        const AstScope* const scopep = itpair.first;
        const AstNodeModule* const modp = itpair.second;
        නම් (VN_IS(modp, Class)) continue;
        ofp()->printf("%-30s ", EmitCUtil::prefixNameProtect(modp).c_str());
        putns(scopep, VIdProtect::protectIf(scopep->nameDotless(), scopep->protect()) + ";\n");
    }
    puts("} vlInst;\n};\n");

    නම් (m_coverBins) {
        puts("\n// COVERAGE\n");
        puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
        puts(" __Vcoverage[");
        puts(std::to_string(m_coverBins));
        puts("];\n");
    }

    නම් (!m_scopeNames.empty()) {  // Scope names
        puts("\n// SCOPE NAMES\n");
        සඳහා (const auto& itpair : m_scopeNames) {
            const ScopeData& sd = itpair.second;
            putns(sd.m_nodep, "::VerilatedScope* " + protect("__Vscopep_" + sd.m_symName) + ";\n");
        }
    }

    නම් (v3Global.opt.vpi()) {
        puts("\n// SCOPE HIERARCHY\n");
        puts("VerilatedHierarchy __Vhier;\n");
    }

    puts("\n// CONSTRUCTORS\n");
    puts(symClassName() + "(VerilatedContext* contextp, const char* namep, " + topClassName()
         + "* modelp);\n");
    puts("~" + symClassName() + "();\n");

    puts("\n// METHODS\n");
    puts("const char* name() const { return vlInst.TOP.vlNamep; }\n");

    නම් (v3Global.hasEvents()) {
        නම් (v3Global.assignsEvents()) {
            puts("void fireEvent(VlAssignableEvent& event) {\n");
        } නැතහොත් {
            puts("void fireEvent(VlEvent& event) {\n");
        }
        puts("if (VL_LIKELY(!event.isTriggered())) {\n");
        නම් (v3Global.assignsEvents()) {
            puts("__Vm_triggeredEvents.push_back(event);\n");
        } නැතහොත් {
            puts("__Vm_triggeredEvents.push_back(&event);\n");
        }
        puts("}\n");
        puts("event.fire();\n");
        puts("}\n");
        puts("void clearTriggeredEvents() {\n");
        නම් (v3Global.assignsEvents()) {
            puts("for (auto& event : __Vm_triggeredEvents) event.clearTriggered();\n");
        } නැතහොත් {
            puts("for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();\n");
        }
        puts("__Vm_triggeredEvents.clear();\n");
        puts("}\n");
    }

    නම් (v3Global.needTraceDumper()) {
        නම් (!optSystemC()) puts("void _traceDump();\n");
        puts("void _traceDumpOpen();\n");
        puts("void _traceDumpClose();\n");
    }

    නම් (v3Global.opt.savable()) {
        puts("void " + protect("__Vserialize") + "(VerilatedSerialize& os);\n");
        puts("void " + protect("__Vdeserialize") + "(VerilatedDeserialize& os);\n");
    }
    puts("};\n");

    ofp()->putsEndGuard();
    closeOutputFile();
}

void EmitCSyms::emitSymImpPreamble() {
    puts("\n");

    // Includes
    puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
    puts("\n");
    // Declarations for DPI Export implementation functions
    bool needsNewLine = false;
    සඳහා (const auto& itpair : m_scopeFuncs) {
        const AstCFunc* const funcp = itpair.second.m_cfuncp;
        නම් (!funcp->dpiExportImpl()) continue;
        emitCFuncDecl(funcp, itpair.second.m_modp);
        needsNewLine = true;
    }
    නම් (needsNewLine) puts("\n");
    // Declarations of the Verilator library functions called at construction
    std::set<std::string> libNames;
    සඳහා (const ScopeModPair& itpair : m_scopes) {
        නම් (itpair.second->verilatorLib()) libNames.emplace(itpair.second->origName());
    }
    සඳහා (const std::string& libName : libNames) {
        puts("extern \"C\" void " + libName + "_protectlib_check_hash(int protectlib_hash__V);\n");
        puts("extern \"C\" void* " + libName + "_protectlib_create(const char* scopep__V);\n");
    }
    නම් (!libNames.empty()) puts("\n");

    // So split ctor sub-functions in other translation units can reference
    // the VPI variable tables defined below.
    නම් (!m_varTables.empty() || !m_scopeTableRows.empty() || !m_ifaceRefTableRows.empty()) {
        සඳහා (const auto& kv : m_varTables) {
            puts("extern const VlVarTableEntry " + kv.first + "[];\n");
        }
        නම් (!m_scopeTableRows.empty()) {
            puts("extern const VlScopeTableEntry " + m_scopeTableName + "[];\n");
        }
        නම් (!m_ifaceRefTableRows.empty()) {
            puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[];\n");
        }
        puts("\n");
    }
}

void EmitCSyms::emitVarTables() {
    නම් (m_varTables.empty() && m_scopeTableRows.empty() && m_ifaceRefTableRows.empty()) ආපසු;
    puts("\n// VPI VARIABLE/SCOPE TABLES\n");
    // offsetof on the (non-standard-layout) generated module/Syms classes is well
    // defined on all supported compilers but warns; suppress just here.
    puts("#if defined(__GNUC__)\n");
    puts("# pragma GCC diagnostic push\n");
    puts("# pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n");
    puts("#endif\n");
    සඳහා (const auto& kv : m_varTables) {
        puts("extern const VlVarTableEntry " + kv.first + "[] = {\n");
        සඳහා (const std::string& row : kv.second) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    නම් (!m_scopeTableRows.empty()) {
        puts("extern const VlScopeTableEntry " + m_scopeTableName + "[] = {\n");
        සඳහා (const std::string& row : m_scopeTableRows) {
            ofp()->putsNoTracking("    ");
            ofp()->putsNoTracking(row);
            ofp()->putsNoTracking(",\n");
        }
        puts("};\n");
    }
    නම් (!m_ifaceRefTableRows.empty()) {
        puts("extern const VlIfaceRefTableEntry " + m_ifaceRefTableName + "[] = {\n");
        සඳහා (const std::string& row : m_ifaceRefTableRows) {
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

void EmitCSyms::emitScopeHier(TextStmts& stmts, bool destroy) {
    නම් (!v3Global.opt.vpi()) ආපසු;

    නම් (destroy) {
        stmts.add("// Tear down scope hierarchy");
    } නැතහොත් {
        stmts.add("// Set up scope hierarchy");
    }

    const std::string method = destroy ? "remove" : "add";
    සඳහා (const auto& itpair : m_scopeNames) {
        නම් (itpair.first == "TOP") continue;
        const ScopeData& sd = itpair.second;
        const std::string& name = sd.m_prettyName;
        const std::string& scopeType = sd.m_type;
        නම් (name.find('.') != string::npos) continue;
        නම් (scopeType != "SCOPE_MODULE" && scopeType != "SCOPE_PACKAGE") continue;
        const std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("vlSymsp->__Vhier." + method + "(0, " + id + ");");
    }
    සඳහා (const auto& itpair : m_vpiScopeHierarchy) {
        const std::string fromName = scopeSymString(itpair.first);
        const std::string fromId
            = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(fromName).m_symName);
        සඳහා (const std::string& name : itpair.second) {
            const std::string toName = scopeSymString(name);
            const std::string toId
                = "vlSymsp->" + protect("__Vscopep_" + m_scopeNames.at(toName).m_symName);
            stmts.add("vlSymsp->__Vhier." + method + "(" + fromId + ", " + toId + ");");
        }
    }

    නම් (destroy) {
        stmts.add("// Clear keys from hierarchy map after values have been removed");
        stmts.add("vlSymsp->__Vhier.clear();");
    }
}

EmitCSyms::TextStmts EmitCSyms::getSymCtorStmts() {
    TextStmts stmts;

    {
        uint64_t stackSize = V3StackCount::count(v3Global.rootp());
        නම් (v3Global.opt.debugStackCheck()) stackSize += 1024 * 1024 * 1024;
        V3Stats::addStat("Size prediction, Stack (bytes)", stackSize);
        // TODO: 'm_statVarScopeBytes' is always 0, AstVarScope doesn't reach here (V3Descope)
        V3Stats::addStat("Size prediction, Heap, from Var Scopes (bytes)", m_statVarScopeBytes);
        V3Stats::addStat(V3Stats::STAT_MODEL_SIZE, stackSize + m_statVarScopeBytes);

        stmts.add("// Check resources");
        stmts.add("Verilated::stackCheck(" + std::to_string(stackSize) + ");");
    }

    stmts.add("// Setup sub module instances");
    සඳහා (const ScopeModPair& itpair : m_scopes) {
        const AstScope* const scopep = itpair.first;
        const AstNodeModule* const modp = itpair.second;
        නම් (modp->isTop()) continue;
        const std::string name = V3OutFormatter::quoteNameControls(
            VIdProtect::protectWordsIf(scopep->prettyName(), scopep->protect()));
        stmts.add("VL_INPLACE_CONSTRUCT(vlSymsp->" + instName(scopep) + ", vlSymsp, \"" + name
                  + "\");");
    }

    නම් (v3Global.opt.profPgo()) {
        stmts.add("// Configure profiling for PGO\n");
        නම් (!v3Global.opt.hierChild()) {
            stmts.add("vlSymsp->_vm_pgoProfiler.writeHeader("
                      "vlSymsp->_vm_contextp__->profVltFilename());");
        }
        නම් (v3Global.opt.mtasks()) {
            v3Global.rootp()->topModulep()->foreach([&](const AstExecGraph* execGraphp) {
                සඳහා (const V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                    const ExecMTask& mt = static_cast<const ExecMTask&>(vtx);
                    stmts.add("vlSymsp->_vm_pgoProfiler.addCounter(" + std::to_string(mt.id())
                              + ", \"" + mt.hashName() + "\");");
                }
            });
        }
    }

    stmts.add("// Configure time unit / time precision");
    නම් (!v3Global.rootp()->timeunit().isNone()) {
        const std::string unit = std::to_string(v3Global.rootp()->timeunit().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeunit(" + unit + ");");
    }
    නම් (!v3Global.rootp()->timeprecision().isNone()) {
        const std::string prec = std::to_string(v3Global.rootp()->timeprecision().powerOfTen());
        stmts.add("vlSymsp->_vm_contextp__->timeprecision(" + prec + ");");
    }

    stmts.add("// Setup each module's pointers to their submodules");
    සඳහා (const auto& i : m_scopes) {
        const AstScope* const scopep = i.first;
        const AstNodeModule* const modp = i.second;
        const AstScope* const abovep = scopep->aboveScopep();
        නම් (!abovep) continue;

        const std::string protName = VIdProtect::protectWordsIf(scopep->name(), scopep->protect());
        std::string stmt = "vlSymsp->";
        නම් (VN_IS(modp, ClassPackage)) {
            // ClassPackage modules seem to be a bit out of place, so hard code...
            stmt += "vlInst.TOP";
        } නැතහොත් {
            stmt += instName(abovep);
        }
        stmt += ".";
        stmt += protName.substr(protName.rfind('.') + 1);
        stmt += " = &vlSymsp->";
        stmt += instName(scopep) + ";";
        stmts.add(stmt);
    }

    නම් (v3Global.opt.coverage()) {
        stmts.add("// Setup each module's pointer back to symbol table (for public functions)");
        සඳහා (const ScopeModPair& i : m_scopes) {
            const AstScope* const scopep = i.first;
            AstNodeModule* const modp = i.second;
            // first is used by AstCoverDecl's call to __vlCoverInsert
            const bool first = !modp->user1();
            modp->user1(true);
            stmts.add("vlSymsp->" + instName(scopep) + "." + protect("__Vconfigure") + "("
                      + (first ? "true" : "false") + ");");
        }
    }

    // Check the hash and create the library model of each Verilator library wrapper, as its
    // initial process would, but already at construction, so the library model exists before
    // the first evaluation
    සඳහා (const ScopeModPair& i : m_scopes) {
        const AstScope* const scopep = i.first;
        const AstNodeModule* const modp = i.second;
        නම් (!modp->verilatorLib()) continue;
        const AstVar* hashp = nullptr;
        const AstVar* handlep = nullptr;
        සඳහා (const AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            const AstVar* const varp = VN_CAST(nodep, Var);
            නම් (!varp) continue;
            නම් (varp->prettyName() == "protectlib_hash__V") hashp = varp;
            නම් (varp->prettyName() == "handle__V") handlep = varp;
        }
        UASSERT_OBJ(hashp, modp, "Verilator library wrapper without 'protectlib_hash__V'");
        UASSERT_OBJ(handlep, modp, "Verilator library wrapper without 'handle__V'");
        const uint32_t hash = VN_AS(hashp->valuep(), Const)->toUInt();
        stmts.add(modp->origName() + "_protectlib_check_hash(static_cast<int>("
                  + std::to_string(hash) + "U));");
        // The library model is named after the %m of the instance, which in a --lib-create
        // library omits the top module (see V3Scope)
        std::string path = scopep->prettyName();
        නම් (!v3Global.opt.libCreate().empty()) {
            path.erase(0, v3Global.rootp()->traceLibTopName().size() + 1);
        }
        stmts.add("vlSymsp->" + instName(scopep) + "." + varName(handlep) + " = VL_CVT_VP_Q("
                  + modp->origName()
                  + "_protectlib_create((std::string{vlSymsp->name()} + (vlSymsp->name()[0] ? "
                    "\".\" : \"\") + \""
                  + V3OutFormatter::quoteNameControls(path) + "\").c_str()));");
    }

    // Every scope has the same construction shape, so all fold into one table with no
    // residual; offsetof bakes the target __Vscopep_* member address into each row.
    නම් (!m_scopeNames.empty()) {
        stmts.add("// Setup scopes");
        const std::string symClass = symClassName();
        සඳහා (const auto& itpair : m_scopeNames) {
            const ScopeData& sd = itpair.second;
            std::string row
                = "{offsetof(" + symClass + ", " + protect("__Vscopep_" + sd.m_symName) + "), \"";
            row += V3OutFormatter::quoteNameControls(
                VIdProtect::protectWordsIf(sd.m_prettyName, true));
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
    නම් (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Setup interface references");
        stmts.add("VerilatedScope::ifaceRefsInsertFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    emitScopeHier(stmts, false);

    නම් (v3Global.dpi()) {
        සඳහා (const std::string vfinal : {"0", "1"}) {
            stmts.add("// Setup export functions - final: " + vfinal);
            සඳහා (const auto& itpair : m_scopeFuncs) {
                const ScopeFuncData& sfd = itpair.second;
                const AstScopeName* const scopep = sfd.m_scopep;
                const AstCFunc* const funcp = sfd.m_cfuncp;
                const AstNodeModule* const modp = sfd.m_modp;
                නම් (!funcp->dpiExportImpl()) continue;

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
    නම් (!m_scopeVars.empty()) {
        stmts.add("// Setup public variables");
        // Keyed by '\0'-joined row text so identical rows share one table.
        std::unordered_map<std::string, std::string> tableByRows;
        පූර්ණ_සංඛ්‍යා tableCounter = 0;

        auto it = m_scopeVars.cbegin();
        අතරතුර (it != m_scopeVars.cend()) {
            const std::string scopeName = it->second.m_scopeName;
            const AstScope* const instScopep = it->second.m_scopep;
            const AstNodeModule* const modp = it->second.m_modp;
            const std::string modClassName = EmitCUtil::prefixNameProtect(modp);

            std::vector<std::string> rows;
            std::vector<std::string> residual;

            සඳහා (; it != m_scopeVars.cend() && it->second.m_scopeName == scopeName; ++it) {
                const ScopeVarData& svd = it->second;
                const AstScope* const scopep = svd.m_scopep;
                UASSERT(scopep == instScopep && svd.m_modp == modp,
                        "VPI scope '" << scopeName << "' spans multiple C++ instances");
                const AstVar* const varp = svd.m_varp;
                const VarDims dims = dimsFor(svd);

                // Force-control signals fold into the base signal's forceable insert.
                const std::pair<bool, std::string> fc = isForceControlSignal(varp);
                නම් (fc.first && baseSignalIsPublic(scopep, fc.second)
                    && baseSignalIsValid(scopep, varp, fc.second)) {
                    continue;
                }

                std::string row;
                const TableEntryKind kind
                    = tryBuildTableEntry(svd, varp, scopep, modClassName, dims, row);
                switch (kind) {
                pak_sa TableEntryKind::TABLE_ROW:
                    rows.emplace_back(row);
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                    bha_nga;
                pak_sa TableEntryKind::FORCEABLE_RESIDUAL: {
                    const std::string bounds = boundsString(dims);
                    residual.emplace_back(insertForceableVarStatement(svd, scopep, varp, dims.udim,
                                                                      dims.pdim, bounds)
                                          + ";");
                    bha_nga;
                }
                pak_sa TableEntryKind::PLAIN_RESIDUAL: {
                    const std::string bounds = boundsString(dims);
                    residual.emplace_back(
                        insertVarStatement(svd, scopep, varp, dims.udim, dims.pdim, bounds) + ";");
                    bha_nga;
                }
                default: v3fatalSrc("Bad case");
                }
                නම් (kind == TableEntryKind::TABLE_ROW) continue;
                නම් (const AstNodeUOrStructDType* const sdtypep
                    = VN_CAST(varp->dtypeSkipRefp(), NodeUOrStructDType)) {
                    නම් (!sdtypep->packed()) {
                        addUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                               varName(varp), sdtypep);
                    }
                } නැතහොත් නම් (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    addUnpackedArrayUOrStructMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                                        varName(varp), varp->dtypep());
                }
                නම් (kind == TableEntryKind::PLAIN_RESIDUAL) {
                    addPackedAggMemberVars(residual, svd, scopep, svd.m_varBasePretty,
                                           varName(varp), varp->dtypep());
                }
            }

            නම් (!rows.empty()) {
                const size_t rowCount = rows.size();
                std::string key;
                සඳහා (const std::string& r : rows) {
                    key += r;
                    key += '\0';
                }
                const auto itt = tableByRows.find(key);
                std::string tableName;
                නම් (itt != tableByRows.end()) {
                    tableName = itt->second;
                } නැතහොත් {
                    tableName = modClassName + "__VpiVarTable" + std::to_string(tableCounter++);
                    tableByRows.emplace(std::move(key), tableName);
                    m_varTables.emplace_back(tableName, std::move(rows));
                }

                std::string call = "vlSymsp->" + protect("__Vscopep_" + scopeName)
                                   + "->varsInsertFromTable(" + tableName + ", "
                                   + std::to_string(rowCount) + ", &(vlSymsp->"
                                   + instName(instScopep) + "));";
                stmts.add(call);
            }
            සඳහා (const std::string& r : residual) stmts.add(r);
        }
    }

    ආපසු stmts;
}

EmitCSyms::TextStmts EmitCSyms::getSymDtorStmts() {
    TextStmts stmts;

    emitScopeHier(stmts, true);
    නම් (v3Global.needTraceDumper()) {
        stmts.add("if (vlSymsp->__Vm_dumping) vlSymsp->_traceDumpClose();");
    }
    නම් (v3Global.opt.profPgo()) {
        stmts.add("vlSymsp->_vm_pgoProfiler.write(\"" + topClassName()
                  + "\", vlSymsp->_vm_contextp__->profVltFilename());");
    }
    // Before the scopes below, as each row names a scope being torn down
    නම් (!m_ifaceRefTableRows.empty()) {
        stmts.add("// Tear down interface references");
        stmts.add("VerilatedScope::ifaceRefsEraseFromTable(" + m_ifaceRefTableName + ", "
                  + std::to_string(m_ifaceRefTableRows.size()) + ", vlSymsp);");
    }

    stmts.add("// Tear down scopes");
    සඳහා (const auto& itpair : m_scopeNames) {
        const ScopeData& sd = itpair.second;
        const std::string id = "vlSymsp->" + protect("__Vscopep_" + sd.m_symName);
        stmts.add("VL_DO_CLEAR(delete " + id + ", " + id + " = nullptr);");
    }

    stmts.add("// Tear down sub module instances");
    සඳහා (const ScopeModPair& itpair : vlstd::reverse_view(m_scopes)) {
        const AstScope* const scopep = itpair.first;
        const AstNodeModule* const modp = itpair.second;
        නම් (modp->isTop()) continue;
        stmts.add("VL_INPLACE_DESTRUCT(vlSymsp->" + instName(scopep) + ");");
    }
    ආපසු stmts;
}

void EmitCSyms::emitSymImp(const AstNetlist* netlistp) {
    UINFO(6, __FUNCTION__ << ": ");

    // Get the body of the constructor and destructor
    TextStmts ctorStmts = getSymCtorStmts();
    TextStmts dtorStmts = getSymDtorStmts();

    // Split into sub-functions if needed
    const std::pair<std::string, std::string> splitCtorDtorDecls  //
        = emitSplitCtorDtor(ctorStmts, dtorStmts,
                            // Base cost 200 to consider other contents in main file.
                            200, symClassName(), symClassName() + "* vlSymsp", "vlSymsp",  //
                            true, "Symbol table implementation internals",
                            [this]() { emitSymImpPreamble(); });

    openNewOutputSourceFile(symClassName(), true, true, "Symbol table implementation internals");
    emitSymImpPreamble();
    emitVarTables();

    const AstScope* const topScopep = netlistp->topScopep()->scopep();
    const std::string topInst = "vlSymsp->" + instName(topScopep);

    // Constructor contents, in a loose function, so all statements access the members through
    // 'vlSymsp', as they do when split into sub-functions
    නම් (!splitCtorDtorDecls.first.empty()) puts("\n" + splitCtorDtorDecls.first + "\n");
    puts("\nvoid " + symClassName() + "__ctor(" + symClassName()
         + "* vlSymsp, const char* namep) {\n");
    puts("// Setup top module instance\n");
    puts("VL_INPLACE_CONSTRUCT(" + topInst + ", vlSymsp, namep);\n");
    සඳහා (const TextStmts::Stmt& stmt : ctorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("}\n");

    // Constructor
    puts("\n");
    puts(symClassName() + "::" + symClassName()
         + "(VerilatedContext* contextp, const char* namep, " + topClassName() + "* modelp)\n");
    puts("    : VerilatedSyms{contextp}\n");
    puts("    // Setup internal state of the Syms class\n");
    puts("    , __Vm_modelp{modelp}\n");
    puts("    , __Vm_didInit{modelp->m_didInit}\n");
    නම් (v3Global.opt.mtasks()) {
        puts("    , __Vm_threadPoolp{static_cast<VlThreadPool*>(contextp->threadPoolp("
             "modelp->threads()))}\n");
    }
    නම් (v3Global.opt.profExec()) {
        puts("    , __Vm_executionProfilerp{static_cast<VlExecutionProfiler*>(contextp->"
             "enableExecutionProfiler(&VlExecutionProfiler::construct))}\n");
    }
    නම් (v3Global.opt.profPgo() && !v3Global.opt.libCreate().empty()) {
        puts("    , _vm_pgoProfiler{" + std::to_string(v3Global.currentHierBlockCost()) + "}\n");
    }
    puts("{\n");
    puts(symClassName() + "__ctor(this, namep);\n");
    puts("}\n");

    // Destructor contents, in a loose function, see constructor
    නම් (!splitCtorDtorDecls.second.empty()) puts("\n" + splitCtorDtorDecls.second + "\n");
    puts("\nvoid " + symClassName() + "__dtor(" + symClassName() + "* vlSymsp) {\n");
    සඳහා (const TextStmts::Stmt& stmt : dtorStmts) {
        ofp()->putsNoTracking("    ");
        ofp()->putsNoTracking(stmt.m_text);
        ofp()->putsNoTracking("\n");
    }
    puts("// Tear down top module instance\n");
    puts("VL_INPLACE_DESTRUCT(" + topInst + ");\n");
    puts("}\n");

    // Destructor
    puts("\n");
    puts(symClassName() + "::~" + symClassName() + "() {\n");
    puts(symClassName() + "__dtor(this);\n");
    puts("}\n");

    // Methods
    නම් (v3Global.needTraceDumper()) {
        නම් (!optSystemC()) {
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

    නම් (v3Global.opt.savable()) {
        සඳහා (const bool& de : {false, true}) {
            const std::string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
            const std::string funcname = protect(de ? "__Vdeserialize" : "__Vserialize");
            const std::string op = de ? ">>" : "<<";
            puts("\nvoid " + symClassName() + "::" + funcname + "(" + classname + "& os) {\n");
            puts("// Internal state\n");
            නම් (v3Global.opt.trace()) puts("os" + op + "__Vm_activity;\n");
            puts("os " + op + " __Vm_didInit;\n");
            puts("// Module instance state\n");
            සඳහා (const ScopeModPair& itpair : m_scopes) {
                const AstScope* const scopep = itpair.first;
                puts(instName(scopep) + "." + funcname + "(os);\n");
            }
            puts("}\n");
        }
    }

    closeOutputFile();
}

//######################################################################

void EmitCSyms::emitDpiHdr() {
    UINFO(6, __FUNCTION__ << ": ");

    openNewOutputHeaderFile(topClassName() + "__Dpi",
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
    සඳහා (const AstCFunc* const nodep : m_dpis) {
        සඳහා (const AstNode* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            නම් (const AstVar* const portp = VN_CAST(argp, Var)) {
                emitDpiStruct(portp->dtypep(), structNames);
            }
        }
    }
    නම් (!structNames.empty()) puts("\n");

    puts("#ifdef __cplusplus\n");
    puts("extern \"C\" {\n");
    puts("#endif\n");
    puts("\n");

    පූර්ණ_සංඛ්‍යා firstExp = 0;
    පූර්ණ_සංඛ්‍යා firstImp = 0;
    සඳහා (const AstCFunc* const nodep : m_dpis) {
        නම් (!nodep->dpiExportDispatcher() && !nodep->dpiImportPrototype()) continue;

        const std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());
        නම් (nodep->dpiExportDispatcher()) {
            නම් (!firstExp++) puts("\n// DPI EXPORTS\n");
            putsDecoration(nodep, "// DPI export" + sourceLoc + "\n");
        } නැතහොත් {
            නම් (!firstImp++) puts("\n// DPI IMPORTS\n");
            putsDecoration(nodep, "// DPI import" + sourceLoc + "\n");
        }
        නම් (nodep->dpiCDeclOverride()) {
            putns(nodep, "extern " + nodep->dpiCDecl() + ";\n");
        } නැතහොත් {
            putns(nodep, "extern " + nodep->rtnTypeVoid() + " " + nodep->nameProtect() + "("
                             + cFuncArgs(nodep, false) + ");\n");
        }
    }

    puts("\n");
    puts("#ifdef __cplusplus\n");
    puts("}\n");
    puts("#endif\n");

    ofp()->putsEndGuard();

    closeOutputFile();
}

void EmitCSyms::emitDpiStruct(const AstNodeDType* dtypep, std::set<std::string>& namesr) {
    නම් (!dtypep->containsUnpackedStruct()) ආපසු;
    const AstStructDType* const sdtypep = VN_AS(dtypep->elemDTypep(), StructDType);
    const std::string name = sdtypep->dpiName();
    // Once per name, as V3Task checked structures of the same name are the same
    නම් (namesr.count(name)) ආපසු;
    // After the structures it contains
    සඳහා (const AstMemberDType* itemp = sdtypep->membersp(); itemp;
         itemp = VN_AS(itemp->nextp(), MemberDType)) {
        emitDpiStruct(itemp->subDTypep(), namesr);
    }
    නම් (name.empty()) ආපසු;  // Declared where used
    නම් (namesr.empty()) puts("// DPI TYPES\n");
    namesr.insert(name);
    // Guarded, so the user, or another model, may declare it instead
    puts("#ifndef VL_DPITYPE_" + name + "_\n");
    puts("#define VL_DPITYPE_" + name + "_\n");
    putns(sdtypep, sdtypep->dpiTypedef());
    puts("#endif\n");
}

//######################################################################

void EmitCSyms::emitDpiImp() {
    UINFO(6, __FUNCTION__ << ": ");
    openNewOutputSourceFile(topClassName() + "__Dpi", false, true,
                            "Implementation of DPI export functions");
    puts("//\n");
    puts("// Verilator compiles this file in when DPI functions are used.\n");
    puts("// If you have multiple Verilated designs with the same DPI exported\n");
    puts("// function names, you will get multiple definition link errors from here.\n");
    puts("// This is an unfortunate result of the DPI specification.\n");
    puts("// To solve this, either\n");
    puts("//    1. Call " + topClassName() + "::{export_function} instead,\n");
    puts("//       and do not even bother to compile this file\n");
    puts("// or 2. Compile all __Dpi.cpp files in the same compiler run,\n");
    puts("//       and #ifdefs already inserted here will sort everything out.\n");
    puts("\n");

    puts("#include \"" + topClassName() + "__Dpi.h\"\n");
    puts("#include \"" + topClassName() + ".h\"\n");
    puts("\n");

    සඳහා (const AstCFunc* const nodep : m_dpis) {
        නම් (!nodep->dpiExportDispatcher()) continue;

        const std::string name = nodep->name();
        const std::string sourceLoc = VIdProtect::ifNoProtect(" at " + nodep->fileline()->ascii());

        // Prevent multi-definition if used by multiple models
        puts("#ifndef VL_DPIDECL_" + name + "_\n");
        puts("#define VL_DPIDECL_" + name + "_\n");
        putns(nodep, nodep->rtnTypeVoid() + " " + name + "(" + cFuncArgs(nodep) + ") {\n");
        puts("// DPI export" + sourceLoc + "\n");
        putns(nodep, "return " + topClassName() + "::" + name + "(");
        std::string comma;
        සඳහා (const AstNode* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
            නම් (const AstVar* const portp = VN_CAST(stmtp, Var)) {
                නම් (portp->isIO() && !portp->isFuncReturn()) {
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

void V3EmitC::emitcSyms(bool dpiHdrOnly) {
    UINFO(2, __FUNCTION__ << ":");
    EmitCSyms{v3Global.rootp(), dpiHdrOnly};
}
