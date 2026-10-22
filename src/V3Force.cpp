// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert forceable signals, process force/release
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
//  V3Force's Transformations:
//
//  For each forceable var/net "<name>":
//    - Create <name>__VforceVec (VlForceVec) to track active force ranges
//    - Create <name>__VforceRHS<ID> vars to hold RHS shadow values
//    - Add continuous assignments: <name>__VforceRHS<ID> = RHS
//
//  For each `force <name>[range] = <RHS>` with ID:
//    - <name>__VforceVec.addForce(lsb, msb, &__VforceRHS, rhsLsb)
//
//  For each `release <name>[range]`:
//    - If not continuously driven: <name> = VlForceVec::read(<name>, __VforceVec)
//    - <name>__VforceVec.release(lsb, msb)
//
//  For each read of <name>:
//    - Replace with: VlForceVec::read(<name>, __VforceVec)
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Force.h"

#include "V3AstUserAllocator.h"
#include "V3Stats.h"
#include "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

class ForceState final {
public:
    struct ForceRange VL_NOT_FINAL {
        정수 m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        정수 m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        정수 m_padLsb = 0;  // Bit positions for RHS padding
        정수 m_padMsb = 0;  // Bit positions for RHS padding
    };

    struct ForceInfo final : ForceRange {
        // MEMBERS
        정수 m_forceId = 0;  // Unique (per signal) variable of this force assignment
        bool m_hasArraySel = false;  // If this has an array select on LHS
        bool m_isExternal = false;  // Synthetic force for the public force controls
        bool m_skipRdRefresh = false;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        AstNodeExpr* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = default;
        ForceInfo(정수 rangeLsb, 정수 rangeMsb, 정수 padLsb, 정수 padMsb, 정수 forceId,
                  bool hasArraySel, AstVarScope* rhsVarVscp, AstNodeExpr* rhsExprp)
            : m_forceId{forceId}
            , m_hasArraySel{hasArraySel}
            , m_rhsVarVscp{rhsVarVscp}
            , m_rhsExprp{rhsExprp} {
            m_rangeLsb = rangeLsb;
            m_rangeMsb = rangeMsb;
            m_padLsb = padLsb;
            m_padMsb = padMsb;
        }
    };

    struct VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, 정수> m_forcePathToIndex;
        정수 m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    private:
        static void buildForcePathKeyRecurse(AstNodeExpr* nodep, string& out) {
            만약 (VN_IS(nodep, VarRef)) 반환;
            만약 (const AstSel* const selp = VN_CAST(nodep, Sel)) {
                buildForcePathKeyRecurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                반환;
            }
            만약 (AstStructSel* const selp = VN_CAST(nodep, StructSel)) {
                AstNodeExpr* const fromp = selp->fromp();
                buildForcePathKeyRecurse(fromp, out);
                out += "|T" + selp->name();
                반환;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        static string forcePathKey(AstNodeExpr* nodep) {
            string out;
            buildForcePathKeyRecurse(nodep, out);
            반환 out;
        }

    public:
        정수 getOrCreateForcePathIndex(AstNodeExpr* nodep) {
            const auto pair
                = m_forcePathToIndex.emplace(forcePathKey(nodep), m_nextForcePathIndex);
            만약 (pair.second) ++m_nextForcePathIndex;
            반환 pair.first->second;
        }

        정수 findForcePathIndex(AstNodeExpr* nodep) const {
            const auto it = m_forcePathToIndex.find(forcePathKey(nodep));
            반환 it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    struct ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    struct ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        bool m_hasBitSel = false;  // Bit-select was found along the path too
    };

    struct ForceRangeInfo final : ForceRange {
        bool m_hasArraySel = false;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

private:
    using ScopeVarCache = std::unordered_map<const AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    const VNUser1InUse m_user1InUse;
    const VNUser2InUse m_user2InUse;

public:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<const AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<const AstVarScope*>;

private:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, 정수> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    bool m_doingAssign = false;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

public:
    ForceState(bool doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    static AstConst* makeZeroConst(AstNode* nodep, 정수 width) {
        V3Number zero{nodep, width};
        zero.setAllBits0();
        반환 new AstConst{nodep->fileline(), zero};
    }

    static AstConst* makeConst32(FileLine* flp, 정수 value) {
        반환 new AstConst{flp, AstConst::WidthedValue{}, 32, static_cast<uint32_t>(value)};
    }

    static AstConst* makeRangeMaskConst(AstNode* nodep, 정수 width, 정수 lsb, 정수 msb) {
        V3Number mask{nodep, width};
        mask.setAllBits0();
        위해 (정수 bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        반환 new AstConst{nodep->fileline(), mask};
    }

    static AstNodeExpr* zeroPadToBaseWidth(AstNodeExpr* exprp, 정수 baseWidth, 정수 padLsb,
                                           정수 padMsb) {
        만약 (baseWidth <= 0) 반환 exprp;
        const 정수 lowPad = padLsb;
        const 정수 highPad = baseWidth - (padMsb + 1);
        만약 (lowPad > 0) {
            exprp = new AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, lowPad)};
        }
        만약 (highPad > 0) {
            exprp = new AstConcat{exprp->fileline(), makeZeroConst(exprp, highPad), exprp};
        }
        반환 exprp;
    }

    static bool isUnpackedArrayDType(const AstNodeDType* dtypep) {
        반환 VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    static bool isBitwiseDType(const AstNode* nodep) {
        const AstBasicDType* const basicp = nodep->dtypep()->skipRefp()->basicp();
        반환 basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    static AstNodeExpr* castToNodeDType(AstNodeExpr* exprp, AstNode* dtypeFromp) {
        const AstNodeDType* const dtypep = dtypeFromp->dtypep()->skipRefp();
        const AstBasicDType* const basicp = dtypep->basicp();
        만약 (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            반환 exprp;
        }
        반환 new AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    static void markNonReplaceable(AstVarRef* const nodep) { nodep->user1SetOnce(); }
    bool isNotReplaceable(const AstVarRef* const nodep) const {
        반환 nodep->user1() || m_permanentlyProtected.count(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    void markPermanentlyProtected(AstVarRef* const nodep) const {
        markNonReplaceable(nodep);
        m_permanentlyProtected.insert(nodep);
    }

    static std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        위해 (auto& it : info.m_forces) forceps.push_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](const ForceInfo* ap, const ForceInfo* bp) {
            반환 ap->m_forceId < bp->m_forceId;
        });
        반환 forceps;
    }

    static std::vector<const ForceInfo*> forceInfosInIdOrder(const VarForceInfo& info) {
        std::vector<const ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        위해 (const auto& it : info.m_forces) forceps.push_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](const ForceInfo* ap, const ForceInfo* bp) {
            반환 ap->m_forceId < bp->m_forceId;
        });
        반환 forceps;
    }

    static bool isOpaquePathSelector(const AstNode* nodep) {
        반환 VN_IS(nodep, Sel) || VN_IS(nodep, NodeSel) || VN_IS(nodep, StructSel);
    }

    static bool isOutermostOpaquePathSelector(const AstNode* nodep) {
        만약 (!isOpaquePathSelector(nodep)) 반환 false;
        const AstNode* const backp = nodep->backp();
        반환 !backp || !isOpaquePathSelector(backp);
    }

    static AstVarRef* getOneVarRef(AstNodeExpr* forceStmtp) {
        AstNode* const basep = forceStmtp->baseFromp(true);
        만약 (AstSampled* sampledp = VN_CAST(basep, Sampled))
            만약 (AstNodeExpr* exprp = VN_CAST(sampledp->exprp(), NodeExpr))
                반환 getOneVarRef(exprp);
        AstVarRef* const varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        반환 varRefp;
    }

    static AstNodeExpr* buildNestedArraySel(FileLine* flp, AstNodeExpr* fromp,
                                            const std::vector<정수>& indicies) {
        AstNodeExpr* curp = fromp;
        위해 (const 정수 idx : indicies) curp = new AstArraySel{flp, curp, idx};
        반환 curp;
    }

    template <typename Fn>
    static AstNodeStmt* foreachUnpackedLeaf(const std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        AstNodeStmt* headp = nullptr;
        AstNodeStmt* tailp = nullptr;
        만약 (dims.empty()) 반환 nullptr;
        정수 total = 1;
        위해 (const AstUnpackArrayDType* const d : dims) total *= d->elementsConst();
        만약 (total <= 0) 반환 nullptr;
        std::vector<정수> idx(dims.size(), 0);
        위해 (정수 flat = 0; flat < total; ++flat) {
            AstNodeStmt* const stmtp = buildLeaf(idx, flat);
            만약 (!headp) {
                headp = stmtp;
            } 아니면 {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            위해 (정수 d = static_cast<정수>(dims.size()) - 1; d >= 0; --d) {
                만약 (++idx[d] < dims[d]->elementsConst()) break;
                idx[d] = 0;
            }
        }
        반환 headp;
    }

    ForceRangeInfo getForceRangeInfo(AstNodeExpr* lhsp, AstVar* varp,
                                     bool requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        만약 (const AstSel* const outerSelp = VN_CAST(lhsp, Sel)) {
            정수 totalLsb = 0;
            위해 (AstNodeExpr* curp = lhsp; const AstSel* const selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                만약 (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                totalLsb += selp->lsbConst();
            }
            info.m_padLsb = totalLsb;
            info.m_padMsb = totalLsb + outerSelp->widthConst() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        만약 (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<정수> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            위해 (AstArraySel* const selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.push_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } 아니면 만약 (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        만약 (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            const 정수 index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        반환 info;
    }

    AstNodeExpr* addRhsValueReads(const VarForceInfo& varInfo, AstNodeExpr* exprp) const {
        만약 (!doingAssign()) 반환 exprp;

        const std::vector<const ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        만약 (forceps.empty()) 반환 exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* const cexprp = new AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        위해 (const ForceInfo* const finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* const refp
                = new AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        반환 cexprp;
    }

    AstNodeExpr* createForceReadCall(const VarForceInfo& varInfo, FileLine* flp, VCMethod method,
                                     AstNodeExpr* originalExprp, AstNode* dtypeFromp,
                                     AstNodeExpr* indexExprp) const {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        AstNodeExpr* const origValp
            = addRhsValueReads(varInfo, castToNodeDType(originalExprp, dtypeFromp));

        AstCMethodHard* const callp = new AstCMethodHard{
            flp, new AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        만약 (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        반환 callp;
    }

    AstNodeExpr* createForceRdExpression(const VarForceInfo& varInfo, FileLine* flp) const {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* const varp = varInfo.m_varVscp->varp();
        만약 (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            만약 (doingAssign()) { 반환 new AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, true);
            AstVarRef* const baseRefp = new AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            반환 createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* const baseRefp = new AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        AstNodeExpr* const enRefp = new AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        AstNodeExpr* const valRefp = new AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        만약 (isBitwiseDType(varp)) {
            반환 new AstOr{
                flp, new AstAnd{flp, enRefp, valRefp},
                new AstAnd{flp, new AstNot{flp, enRefp->cloneTreePure(false)}, baseRefp}};
        }
        반환 new AstCond{flp, enRefp, valRefp, baseRefp};
    }

    AstNodeStmt* createForceRdUpdateStmt(const VarForceInfo& varInfo) const {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* const varp = varInfo.m_varVscp->varp();
        만약 (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            반환 createForceRdUpdateStmtUnpacked(varInfo);
        }
        FileLine* const flp = varInfo.m_varVscp->fileline();
        반환 new AstAssign{flp, new AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    AstNodeStmt* createForceRdUpdateStmtUnpacked(const VarForceInfo& varInfo) const {
        FileLine* const flp = varInfo.m_varVscp->fileline();
        AstVar* const varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* const arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        const std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        AstNodeStmt* const stmtsp = foreachUnpackedLeaf(
            dims, [&](const std::vector<정수>& idx, 정수 /*flat*/) -> AstNodeStmt* {
                AstVarRef* const baseRefp = new AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                AstNodeExpr* const baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                AstNodeExpr* const enSelp = buildNestedArraySel(
                    flp, new AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                AstNodeExpr* const valSelp = buildNestedArraySel(
                    flp, new AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                AstNodeExpr* const readExprp = new AstOr{
                    flp, new AstAnd{flp, enSelp, valSelp},
                    new AstAnd{flp, new AstNot{flp, enSelp->cloneTreePure(false)}, baseSelp}};
                AstNodeExpr* const rdLhsSelp = buildNestedArraySel(
                    flp, new AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                반환 new AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        위해 (const ForceInfo* const finfop : forceInfosInIdOrder(varInfo)) {
            만약 (!finfop->m_isExternal) continue;
            stmtsp->addNext(new AstAssign{flp,
                                          new AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->cloneTreePure(false)});
        }
        반환 stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        const auto it = m_varToId.find(vscp);
        만약 (it != m_varToId.end()) 반환 m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* const varp = info.m_varp;
        만약 (!varp->isForceable()) 반환 info;

        FileLine* const flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        const bool helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        만약 (!helperVarsBuilt) {
            const bool unpacked = isUnpackedArrayDType(varp->dtypep());
            const VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            AstNodeDType* const enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = new AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(true);
            helperVars.m_enVarp
                = new AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(true);
            helperVars.m_valVarp
                = new AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(true);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        만약 (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } 아니면 {
            info.m_forceRdVscp = new AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = new AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = new AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        반환 info;
    }

    void markClockedWrite(AstVar* varp) { m_clockedWrites.insert(varp); }
    bool hasClockedWrite(AstVar* varp) const { 반환 m_clockedWrites.count(varp); }

    bool doingAssign() const { 반환 m_doingAssign; }

    const VarForceInfo* getVarInfo(AstVarScope* vscp) const {
        const auto it = m_varToId.find(vscp);
        반환 it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, const AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        만약 (cache.empty()) {
            위해 (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        const auto it = cache.find(varp);
        반환 it != cache.end() ? it->second : nullptr;
    }
    void addForceAssignment(AstVar* varp, AstVarScope* vscp, AstNodeExpr* rhsExprp,
                            AstAssignForce* forceStmtp, 정수 rangeLsb, 정수 rangeMsb, 정수 padLsb,
                            정수 padMsb, bool hasArraySel, bool skipRdRefresh = false) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        const 정수 forceId = info.m_forces.size();
        FileLine* const flp = varp->fileline();
        AstScope* const scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        만약 (!info.m_forceVecVscp) {
            AstCDType* const forceVecDtypep = new AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* const forceVecVarp
                = new AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(false);
            forceVecVarp->isInternal(true);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = new AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        만약 (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* const refp) {
                만약 (!refp->access().isReadOnly()) 반환;
                AstVar* const depVarp = refp->varp();
                만약 (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.push_back(depVarp);
                }
            });
            위해 (AstVar* const depVarp : depVarps) m_rhsDepToForces[depVarp].push_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    static void collectArraySelInfo(AstNodeExpr* exprp, ArraySelInfo& info) {
        만약 (const auto* const selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = true;
            collectArraySelInfo(selp->fromp(), info);
        } 아니면 만약 (auto* const selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.push_back(selp);
        } 아니면 만약 (const auto* const memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    static ArraySelInfo getArraySelInfo(AstNodeExpr* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        반환 info;
    }

    static std::vector<정수> arraySelDimSizes(const ArraySelInfo& info) {
        std::vector<정수> dims;
        dims.reserve(info.m_sels.size());
        위해 (AstArraySel* const selp : info.m_sels) {
            AstNodeDType* const dtypep = selp->fromp()->dtypep()->skipRefp();
            const AstUnpackArrayDType* const arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.push_back(arrayp->declRange().elements());
        }
        반환 dims;
    }

    static 정수 flattenIndex(const std::vector<정수>& indices, const std::vector<정수>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        정수 index = 0;
        정수 stride = 1;
        위해 (정수 i = static_cast<정수>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        반환 index;
    }

    static AstNodeExpr* buildFlattenIndexExpr(FileLine* flp, const ArraySelInfo& info) {
        const std::vector<정수> dimSizes = arraySelDimSizes(info);
        bool allConst = true;
        위해 (AstArraySel* const selp : info.m_sels) {
            만약 (!VN_IS(selp->bitp(), Const)) {
                allConst = false;
                break;
            }
        }
        만약 (allConst) {
            std::vector<정수> constIndices;
            constIndices.reserve(info.m_sels.size());
            위해 (AstArraySel* const selp : info.m_sels) {
                constIndices.push_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            반환 makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        AstNodeExpr* resultp = nullptr;
        정수 stride = 1;
        위해 (정수 i = static_cast<정수>(info.m_sels.size()) - 1; i >= 0; --i) {
            AstNodeExpr* termp = info.m_sels[i]->bitp()->cloneTreePure(false);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            만약 (termp->width() < 32) termp = new AstExtend{flp, termp, 32};
            만약 (stride != 1) termp = new AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? new AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        반환 resultp;
    }

    static AstNodeExpr* buildRhsDataExpr(FileLine* flp, const ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        반환 new AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    void finalizeRhsVars() {
        위해 (VarForceInfo& info : m_varInfos) {
            AstVar* const varp = info.m_varp;
            만약 (info.m_forces.empty()) continue;

            AstScope* const scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            FileLine* const flp = varp->fileline();
            const std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            위해 (ForceInfo* const finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* const rhsVarp = new AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(true);
                rhsVarp->sigPublic(true);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = new AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* const rhsAssignp = new AstAssign{
                    flp, new AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                만약 (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* const touchCallp = new AstCMethodHard{
                        flp, new AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    AstNodeStmt* const touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* const alwaysp
                    = new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                AstSenTree* const senTreep
                    = new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* const activep = new AstActive{flp, "force-rhs-update", senTreep};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            만약 (info.m_forceRdVscp) {
                만약 (!m_forceRdUpdateBuilt.insert(info.m_varVscp).second) continue;
                AstActive* const activeInitp = new AstActive{
                    flp, "force-init",
                    new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->senTreeStorep(activeInitp->sentreep());
                AstNodeStmt* initStmtp = nullptr;
                만약 (AstUnpackArrayDType* const arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    const std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    const 정수 innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](const std::vector<정수>& idx, 정수 /*flat*/) -> AstNodeStmt* {
                            AstNodeExpr* const lhsp = buildNestedArraySel(
                                flp, new AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            반환 new AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } 아니면 {
                    initStmtp = new AstAssign{
                        flp, new AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(new AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    만약 (!vscp) 반환;
                    AstSenItem* const nextp = new AstSenItem{
                        flp, VEdgeType::ET_CHANGED, new AstVarRef{flp, vscp, VAccess::READ}};
                    만약 (itemsp) {
                        itemsp->addNext(nextp);
                    } 아니면 {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* const origSenRefp = new AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* const origItemp
                    = new AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                만약 (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                위해 (ForceInfo* const finfop : forceps) {
                    만약 (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* const activep
                    = new AstActive{flp, "force-rd-update", new AstSenTree{flp, itemsp}};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    AstNode* createRhsUpdatesForWrite(FileLine* flp, AstVar* writtenVarp) const {
        만약 (!doingAssign()) 반환 nullptr;

        const auto it = m_rhsDepToForces.find(writtenVarp);
        만약 (it == m_rhsDepToForces.end()) 반환 nullptr;

        AstNode* headp = nullptr;
        AstNode* tailp = nullptr;
        위해 (const ForceInfo* const finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* const updatep
                = new AstAssign{flp, new AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->cloneTreePure(false)};
            만약 (tailp) {
                tailp->addNextHere(updatep);
            } 아니면 {
                headp = updatep;
            }
            tailp = updatep;
        }
        반환 headp;
    }

    const ForceInfo& getForceInfo(AstAssignForce* forceStmtp) const {
        AstVarScope* const vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        const VarForceInfo* const varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        반환 it2->second;
    }

    static bool selOverlapsAnyForce(const VarForceInfo& varInfo, 정수 selLsb, 정수 selMsb) {
        위해 (const auto& pair : varInfo.m_forces) {
            만약 (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) 반환 true;
        }
        반환 false;
    }

    AstNodeExpr* createForceReadExpression(const VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) const {
        FileLine* const flp = originalRefp->fileline();
        AstVarRef* const refp = originalRefp->cloneTreePure(false);
        refp->access(VAccess::READ);
        반환 createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    AstNodeExpr* createForceReadIndexExpression(const VarForceInfo& varInfo,
                                                AstNodeExpr* originalExprp,
                                                AstNodeExpr* indexExprp) const {
        FileLine* const flp = originalExprp->fileline();
        AstNodeExpr* const exprp = originalExprp->cloneTreePure(false);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        반환 createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    static AstNodeExpr* rebuildSelPath(AstNodeExpr* pathp, AstNodeExpr* baseExprp) {
        만약 (const AstSel* const selp = VN_CAST(pathp, Sel)) {
            AstNodeExpr* const fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* const outp
                = new AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            반환 outp;
        }
        반환 baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
static void splitDeassign(AstDeassign* nodep) {
    AstConcat* const concatp = VN_CAST(nodep->lhsp(), Concat);
    만약 (!concatp) 반환;

    FileLine* const flp = nodep->fileline();
    AstDeassign* const newLp = new AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* const newRp = new AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    AstNodeExpr* const conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->deleteTree(), nodep);
    VL_DO_DANGLING(conp->deleteTree(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

class ForceDiscoveryVisitor final : public VNVisitorConst {
    ForceState& m_state;
    bool m_inClockedActive = false;

    void buildForceableUnpackedArray(AstVarScope* const nodep,
                                     AstUnpackArrayDType* const arrDtypep) {
        AstVar* const varp = nodep->varp();
        const std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        const AstNodeDType* const leafDtypep = dims.back()->subDTypep()->skipRefp();
        const AstBasicDType* const innerBasicp = leafDtypep->basicp();
        만약 (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            반환;
        }

        FileLine* const flp = varp->fileline();
        const 정수 innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* const enVscp = info.m_forceEnVscp;
        AstVarScope* const valVscp = info.m_forceValVscp;

        AstSenItem* const itemsp = new AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  new AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* const activep = new AstActive{flp, "force-update", new AstSenTree{flp, itemsp}};
        activep->senTreeStorep(activep->sentreep());

        AstNodeStmt* const alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](const std::vector<정수>& idx, 정수 flat) -> AstNodeStmt* {
                AstVarRef* const origRefp = new AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                AstNodeExpr* const origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                AstNodeExpr* const enSelp = ForceState::buildNestedArraySel(
                    flp, new AstVarRef{flp, enVscp, VAccess::READ}, idx);
                AstNodeExpr* const valSelp = ForceState::buildNestedArraySel(
                    flp, new AstVarRef{flp, valVscp, VAccess::READ}, idx);
                AstNodeExpr* const forceExprp = new AstOr{
                    flp, new AstAnd{flp, enSelp, valSelp},
                    new AstAnd{flp, new AstNot{flp, enSelp->cloneTreePure(false)}, origSelp}};
                AstNodeExpr* const lhsSelp = ForceState::buildNestedArraySel(
                    flp, new AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* const forceAssignp = new AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(true);

                AstNodeExpr* const rhsClonep = forceExprp->cloneTreePure(false);
                rhsClonep->foreach([varp](AstVarRef* const r) {
                    만약 (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/true, /*skipRdRefresh=*/true);
                반환 forceAssignp;
            });
        activep->addStmtsp(new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    void visit(AstAssignForce* nodep) override {
        만약 (nodep->user2()) 반환;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* const forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, true);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        AstNodeExpr* const rhsExprp = nodep->rhsp()->cloneTreePure(false);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    void visit(AstAssign* nodep) override {
        만약 (m_state.doingAssign() && m_inClockedActive) {
            만약 (AstVarRef* const lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    void visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    void visit(AstVarScope* nodep) override {
        만약 (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            만약 (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                반환;
            }

            만약 (AstUnpackArrayDType* const arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                반환;
            }

            const AstBasicDType* const bdtypep = nodep->varp()->basicp();
            만약 (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* const varp = nodep->varp();
            FileLine* const flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* const enVscp = info.m_forceEnVscp;
            AstVarScope* const valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* const itemsp = new AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      new AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* const activep
                = new AstActive{flp, "force-update", new AstSenTree{flp, itemsp}};
            activep->senTreeStorep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* const refp = new AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* const enRefp = new AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* const valRefp = new AstVarRef{flp, valVscp, VAccess::READ};
            const AstBasicDType* const basicp = varp->dtypep()->skipRefp()->basicp();
            AstNodeExpr* const forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<AstNodeExpr*>(new AstOr{
                            flp, new AstAnd{flp, enRefp, valRefp},
                            new AstAnd{flp, new AstNot{flp, enRefp->cloneTreePure(false)}, refp}})
                      : static_cast<AstNodeExpr*>(new AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* const forceAssignp
                = new AstAssignForce{flp, new AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(true);
            activep->addStmtsp(new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            AstNodeExpr* const rhsClonep = forceExprp->cloneTreePure(false);
            rhsClonep->foreach([varp](AstVarRef* const refp) {
                만약 (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            const bool bitwiseVar = ForceState::isBitwiseDType(varp);
            const 정수 padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            정수 rangeLsb = 0;
            정수 rangeMsb = padMsb;
            만약 (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, false);
        }
        iterateChildrenConst(nodep);
    }

    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

public:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

class ForceConvertVisitor final : public VNVisitor {
    ForceState& m_state;

    void visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        AstNodeExpr* const lhsp = nodep->lhsp();
        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* const forcedVarp = lhsVarRefp->varp();

        const ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        const ForceState::VarForceInfo* const varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        FileLine* const flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* const rhsAssignp
            = new AstAssign{flp, new AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->cloneTreePure(false)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        const bool bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        만약 (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            AstNodeExpr* baseExprp = nodep->rhsp()->cloneTreePure(false);
            baseExprp->foreach(
                [](AstVarRef* const refp) { ForceState::markNonReplaceable(refp); });
            만약 (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            만약 (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* const maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                AstNodeExpr* const valReadp
                    = new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                AstNodeExpr* const valWritep
                    = new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                AstNodeExpr* const notMaskp = new AstNot{flp, maskConstp};
                AstNodeExpr* const maskedOldp = new AstAnd{flp, valReadp, notMaskp};
                AstNodeExpr* const newValp = new AstOr{flp, maskedOldp, baseExprp};
                valAssignp = new AstAssign{flp, valWritep, newValp};

                AstNodeExpr* const enReadp
                    = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                AstNodeExpr* const enWritep
                    = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                AstNodeExpr* const newEnp
                    = new AstOr{flp, enReadp, maskConstp->cloneTreePure(false)};
                enAssignp = new AstAssign{flp, enWritep, newEnp};
            } 아니면 {
                AstConst* const oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                AstNodeExpr* const rhsValp = ForceState::castToNodeDType(baseExprp, forcedVarp);
                valAssignp = new AstAssign{
                    flp, new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = new AstAssign{
                    flp, new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        const AstSel* const selLhsp = VN_CAST(lhsp, Sel);
        const bool arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        AstNodeExpr* const rhsDatap = ForceState::buildRhsDataExpr(flp, info);
        AstCExpr* const rhsAddrp = new AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* const addForceCallp = new AstCMethodHard{
            flp, new AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        만약 (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        AstNodeStmt* const stmtp = addForceCallp->makeStmt();

        AstNode* tailp = rhsAssignp;
        만약 (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        만약 (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        만약 (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        AstNodeExpr* const lhsp = nodep->lhsp();
        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* const releasedVarp = lhsVarRefp->varp();

        const ForceState::VarForceInfo* const varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        만약 (!varInfo) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            반환;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        FileLine* const flp = nodep->fileline();

        const ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, false);

        const AstSel* const selLhsp = VN_CAST(lhsp, Sel);
        const bool arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* const releaseCallp = new AstCMethodHard{
            flp, new AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        만약 (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        AstNodeStmt* const releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        만약 (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            AstNodeExpr* const enWritep
                = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            만약 (ForceState::isBitwiseDType(releasedVarp)) {
                const 정수 varWidth = releasedVarp->width();
                만약 (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = new AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } 아니면 {
                    // forceEn = forceEn & ~mask(range);
                    AstNodeExpr* const enReadp
                        = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* const maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    AstNodeExpr* const newEnp
                        = new AstAnd{flp, enReadp, new AstNot{flp, maskConst}};
                    clearEnp = new AstAssign{flp, enWritep, newEnp};
                }
            } 아니면 {
                clearEnp = new AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        const AstSel* const selp = VN_CAST(lhsp, Sel);
        AstNodeExpr* const basep = selp ? selp->fromp() : lhsp;

        AstNode* stmtListp = releasep;
        만약 (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        const bool fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        만약 (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            const bool hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            AstNodeExpr* forceReadp = nullptr;
            만약 (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } 아니면 만약 (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::buildFlattenIndexExpr(flp, rangeInfo.m_arrayInfo));
                만약 (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } 아니면 {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* const assignp = new AstAssign{flp, lhsp->cloneTreePure(false), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        만약 (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

class ForceReplaceVisitor final : public VNVisitor {
    const ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    AstNodeStmt* m_stmtp = nullptr;
    bool m_inLogic = false;

    void iterateLogic(AstNode* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = true;
        iterateChildren(nodep);
    }

    void visit(AstNodeStmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        만약 (AstVarRef* const lhsp = VN_CAST(nodep->lhsp()->baseFromp(true), VarRef)) {
            만약 (AstNode* const updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    void visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    void visit(AstDeassign*) override {}
    void visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    void visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    void visit(AstNodeProcedure* nodep) override { iterateLogic(nodep); }
    void visit(AstAlways* nodep) override {
        만약 (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            반환;
        }
        iterateLogic(nodep);
    }
    void visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    void visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* const refp = VN_CAST(nodep->fromp(), VarRef);
        만약 (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<AstNode*>(nodep));
            반환;
        }

        AstVar* const varp = refp->varp();
        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(refp->varScopep());
        만약 (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<AstNode*>(nodep));
            반환;
        }

        만약 (const AstConst* const lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            const 정수 selLsb = lsbConstp->toSInt();
            const 정수 selMsb = selLsb + nodep->width() - 1;
            만약 (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<AstNode*>(nodep));
                반환;
            }
        }

        FileLine* const flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* const refClonep = refp->cloneTreePure(false);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* const callp = new AstCMethodHard{
            flp, new AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->cloneTreePure(false));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstArraySel* nodep) override {
        만약 (const AstArraySel* const backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            만약 (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                반환;
            }
        }

        AstNode* const basep = nodep->baseFromp(true);
        AstVarRef* const baseRefp = VN_CAST(basep, VarRef);
        만약 (!baseRefp) {
            iterateChildren(nodep);
            반환;
        }
        AstVar* const varp = baseRefp->varp();
        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        만약 (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            반환;
        }

        만약 (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            반환;
        }
        만약 (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            반환;
        }
        const ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        위해 (AstArraySel* const selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        AstNodeExpr* const indexExprp
            = ForceState::buildFlattenIndexExpr(nodep->fileline(), arrayInfo);
        AstNodeExpr* const readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstVarRef* nodep) override {
        만약 (m_state.isNotReplaceable(nodep)) 반환;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        만약 (const AstArraySel* const backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            만약 (backSelp->fromp() == nodep) 반환;
        }

        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(nodep->varScopep());
        만약 (!varInfo) 반환;

        만약 (varInfo->m_forceRdVscp) {
            만약 (nodep->access().isRW()) {
                만약 (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                반환;
            }
            만약 (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                AstNodeExpr* const readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                반환;
            }
            만약 (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            반환;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        만약 (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            반환;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        만약 (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } 아니면 만약 (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            AstNodeExpr* const readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstNode* nodep) override {
        만약 (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            AstNodeExpr* const exprp = VN_AS(nodep, NodeExpr);
            AstNode* const basep = exprp->baseFromp(true);
            AstVarRef* const baseRefp = VN_CAST(basep, VarRef);
            만약 (baseRefp) {
                AstVar* const varp = baseRefp->varp();
                만약 (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    const ForceState::VarForceInfo* const varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    만약 (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        const 정수 forcePathIndex = varInfo->findForcePathIndex(exprp);
                        만약 (forcePathIndex >= 0) {
                            만약 (!baseRefp->access().isReadOnly()) 반환;
                            AstNodeExpr* const readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(pushDeletep(nodep), nodep);
                            반환;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

public:
    explicit ForceReplaceVisitor(AstNetlist* nodep, const ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::addStat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

static void forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    만약 (!v3Global.hasForceableSignals()) 반환;
    ForceState state{false, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
}

static void assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    만약 (!v3Global.hasAssignDeassign()) 반환;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.push_back(deassignp); });
    위해 (AstDeassign* const deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](AstNodeStmt* nodep) {
        만약 (AstAssignCont* const assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.push_back(assignContp);
        } 아니면 만약 (AstDeassign* const deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.push_back(deassignp);
        }
    });

    위해 (AstAssignCont* const assignp : assignContps) {
        assignp->replaceWith(new AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->deleteTree(), assignp);
    }
    위해 (AstDeassign* const deassignp : deassignps) {
        deassignp->replaceWith(
            new AstRelease{deassignp->fileline(), deassignp->lhsp()->cloneTreePure(true)});
        VL_DO_DANGLING(deassignp->deleteTree(), deassignp);
    }
    ForceState state{true, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dumpCheckGlobalTree("assign-deassign", 0, dumpTreeEitherLevel() >= 3);
}

void V3Force::forceAndAssignAll(AstNetlist* nodep) {
    const VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
