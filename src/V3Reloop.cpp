// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Recreate loops to help pack caches
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
// V3Reloop's Transformations:
//
// Each CFunc:
//    Look for a series of assignments that would look better in a loop:
//
//      ASSIGN(ARRAYREF(var, #), ARRAYREF(var, #+C))
//      ASSIGN(ARRAYREF(var, #+1), ARRAYREF(var, #+1+C))
//      ->
//      Create __Vilp local variable
//      FOR(__Vilp = low; __Vilp <= high; ++__Vlip)
//         ASSIGN(ARRAYREF(var, __Vilp), ARRAYREF(var, __Vilp + C))
//
//   Likewise vector assign to the same constant converted to a loop.
//
//*************************************************************************

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Reloop.h"

#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം ReloopVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstCFunc::user1p      -> Var number temp var, 0=not set yet
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    VDouble0 m_statReloops;  // Statistic tracking
    VDouble0 m_stat_Re_ഇനങ്ങൾ;  // Statistic tracking
    AstCFunc* m_cfuncp = nullptr;  // Current block

    std::vector<Ast_കണ്ണി_Assign*> m_mgAssignps;  // List of assignments merging
    AstCFunc* m_mgCfuncp = nullptr;  // Parent C function
    സ്ഥിരം Ast_കണ്ണി* m_mgNextp = nullptr;  // Next node
    സ്ഥിരം Ast_കണ്ണി_Sel* m_mgSelLp = nullptr;  // Parent select, nullptr = idle
    സ്ഥിരം Ast_കണ്ണി_Sel* m_mgSelRp = nullptr;  // Parent select, nullptr = constant
    സ്ഥിരം Ast_കണ്ണി_Var_Ref* m_mgVarrefLp = nullptr;  // Parent varref
    സ്ഥിരം Ast_കണ്ണി_Var_Ref* m_mgVarrefRp = nullptr;  // Parent varref, nullptr = constant
    int64_t m_mgOffset = 0;  // Index offset
    സ്ഥിരം AstConst* m_mgConstRp = nullptr;  // Parent RHS constant, nullptr = sel
    uint32_t m_mgIndexLo = 0;  // Merge range
    uint32_t m_mgIndexHi = 0;  // Merge range

    // METHODS

    സ്ഥിതം AstVar* createVarTemp(File_വരി* fl, AstCFunc* cfuncp) {
        UASSERT_OBJ(cfuncp, fl, "Assignment not under a function");
        സ്ഥിരം string newvarname{"__Vilp" + std::to_string(cfuncp->user1Inc() + 1)};
        AstVar* സ്ഥിരം varp
            = പുതിയ AstVar{fl, VVarType::STMTTEMP, newvarname, VFlagLogicPacked{}, 32};
        cfuncp->addVarsp(varp);
        തിരികെ varp;
    }
    ശൂന്യം mergeEnd() {
        എങ്കിൽ (!m_mgAssignps.empty()) {
            സ്ഥിരം uint32_t ഇനങ്ങൾ = m_mgIndexHi - m_mgIndexLo + 1;
            UINFO(9, "End merge iter=" << ഇനങ്ങൾ << " " << m_mgIndexHi << ":" << m_mgIndexLo << " "
                                       << m_mgOffset << " " << m_mgAssignps[0]);
            എങ്കിൽ (ഇനങ്ങൾ >= static_cast<uint32_t>(v3Global.opt.reloopLimit())) {
                UINFO(6, "Reloop merging items=" << ഇനങ്ങൾ << " " << m_mgIndexHi << ":"
                                                 << m_mgIndexLo << " " << m_mgOffset << " "
                                                 << m_mgAssignps[0]);
                ++m_statReloops;
                m_stat_Re_ഇനങ്ങൾ += ഇനങ്ങൾ;

                // Transform first assign into for loop body
                Ast_കണ്ണി_Assign* സ്ഥിരം bodyp = m_mgAssignps.front();
                UASSERT_OBJ(bodyp->lhsp() == m_mgSelLp, bodyp, "Corrupt queue/state");
                File_വരി* സ്ഥിരം fl = bodyp->fileline();
                AstVar* സ്ഥിരം itp = createVarTemp(fl, m_mgCfuncp);

                എങ്കിൽ (m_mgOffset > 0) {
                    UASSERT_OBJ(m_mgIndexLo >= m_mgOffset, bodyp,
                                "Reloop iteration starts at negative index");
                    m_mgIndexLo -= m_mgOffset;
                    m_mgIndexHi -= m_mgOffset;
                }

                Ast_കണ്ണി* സ്ഥിരം initp = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, itp, VAccess::WRITE},
                                                     പുതിയ AstConst{fl, m_mgIndexLo}};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstLte{fl, പുതിയ AstVarRef{fl, itp, VAccess::READ},
                                                      പുതിയ AstConst{fl, m_mgIndexHi}};
                Ast_കണ്ണി* സ്ഥിരം incp = പുതിയ AstAssign{
                    fl, പുതിയ AstVarRef{fl, itp, VAccess::WRITE},
                    പുതിയ AstAdd{fl, പുതിയ AstConst{fl, 1}, പുതിയ AstVarRef{fl, itp, VAccess::READ}}};
                AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{fl};
                loopp->addStmtsp(പുതിയ AstLoopTest{fl, loopp, condp});
                initp->addNext(loopp);
                bodyp->replaceWith(initp);
                loopp->addStmtsp(bodyp);
                loopp->addStmtsp(incp);

                // Replace constant index with new loop index
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം offsetp
                    = m_mgOffset == 0 ? nullptr : പുതിയ AstConst(fl, std::abs(m_mgOffset));
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lbitp = m_mgSelLp->bitp();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lvrefp = പുതിയ AstVarRef{fl, itp, VAccess::READ};
                lbitp->replaceWith(m_mgOffset > 0 ? പുതിയ AstAdd{fl, lvrefp, offsetp} : lvrefp);
                VL_DO_DANGLING(lbitp->delete_വൃക്ഷം(), lbitp);
                എങ്കിൽ (m_mgSelRp) {  // else constant and no replace
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rbitp = m_mgSelRp->bitp();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rvrefp = പുതിയ AstVarRef{fl, itp, VAccess::READ};
                    rbitp->replaceWith(m_mgOffset < 0 ? പുതിയ AstAdd{fl, rvrefp, offsetp} : rvrefp);
                    VL_DO_DANGLING(rbitp->delete_വൃക്ഷം(), lbitp);
                }
                UINFOTREE(9, initp, "", "new");
                UINFOTREE(9, loopp, "", "new");

                // Remove remaining assigns
                വേണ്ടി (Ast_കണ്ണി_Assign* assp : m_mgAssignps) {
                    എങ്കിൽ (assp != bodyp) {
                        VL_DO_DANGLING(assp->unlinkFrBack()->delete_വൃക്ഷം(), assp);
                    }
                }
            }
            // Setup for next merge
            m_mgAssignps.clear();
            m_mgSelLp = nullptr;
            m_mgSelRp = nullptr;
            m_mgVarrefLp = nullptr;
            m_mgVarrefRp = nullptr;
            m_mgOffset = 0;
            m_mgConstRp = nullptr;
        }
    }

    // VISITORS
    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
        mergeEnd();  // Finish last pending merge, if any
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (!m_cfuncp) തിരികെ;

        // Left select WordSel or ArraySel
        Ast_കണ്ണി_Sel* സ്ഥിരം lselp = VN_CAST(nodep->lhsp(), കണ്ണി_Sel);
        എങ്കിൽ (!lselp) {  // Not ever merged
            mergeEnd();
            തിരികെ;
        }
        // Of a constant index
        സ്ഥിരം AstConst* സ്ഥിരം lbitp = VN_CAST(lselp->bitp(), Const);
        എങ്കിൽ (!lbitp) {
            mergeEnd();
            തിരികെ;
        }
        എങ്കിൽ (lbitp->width() > 32) {  // Assoc arrays can do this
            mergeEnd();
            തിരികെ;
        }
        സ്ഥിരം uint32_t lindex = lbitp->toUInt();
        // Of variable
        സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം lvarrefp = VN_CAST(lselp->fromp(), കണ്ണി_Var_Ref);
        എങ്കിൽ (!lvarrefp) {
            mergeEnd();
            തിരികെ;
        }

        // RHS is a constant or a select
        സ്ഥിരം AstConst* സ്ഥിരം rconstp = VN_CAST(nodep->rhsp(), Const);
        സ്ഥിരം Ast_കണ്ണി_Sel* സ്ഥിരം rselp = VN_CAST(nodep->rhsp(), കണ്ണി_Sel);
        സ്ഥിരം Ast_കണ്ണി_Var_Ref* rvarrefp = nullptr;
        uint32_t rindex = lindex;
        എങ്കിൽ (rconstp) {  // Ok
        } അല്ലെങ്കിൽ എങ്കിൽ (rselp) {
            സ്ഥിരം AstConst* സ്ഥിരം rbitp = VN_CAST(rselp->bitp(), Const);
            rvarrefp = VN_CAST(rselp->fromp(), കണ്ണി_Var_Ref);
            എങ്കിൽ (!rbitp || !rvarrefp || lvarrefp->varp() == rvarrefp->varp()) {
                mergeEnd();
                തിരികെ;
            }
            rindex = rbitp->toUInt();
        } അല്ലെങ്കിൽ {
            mergeEnd();
            തിരികെ;
        }

        എങ്കിൽ (m_mgSelLp) {  // Old merge
            എങ്കിൽ (m_mgCfuncp == m_cfuncp  // In same function
                && m_mgNextp == nodep  // Consecutive node
                && m_mgVarrefLp->isSame(lvarrefp)  // Same array on left hand side
                && (m_mgConstRp  // On the right hand side either ...
                        ? (rconstp && m_mgConstRp->isSame(rconstp))  // ... same constant
                        : (rselp && m_mgVarrefRp->isSame(rvarrefp)))  // ... or same array
                && (lindex == m_mgIndexLo - 1 || lindex == m_mgIndexHi + 1)  // Left index +/- 1
                && (m_mgConstRp || lindex == rindex + m_mgOffset)  // Same right index offset
            ) {
                // Sequentially next to last assign; continue merge
                എങ്കിൽ (lindex == m_mgIndexLo - 1) {
                    m_mgIndexLo = lindex;
                } അല്ലെങ്കിൽ എങ്കിൽ (lindex == m_mgIndexHi + 1) {
                    m_mgIndexHi = lindex;
                }
                UINFO(9, "Continue merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                             << " " << nodep);
                m_mgAssignps.തള്ളുക_back(nodep);
                m_mgNextp = nodep->nextp();
                തിരികെ;
            } അല്ലെങ്കിൽ {
                UINFO(9, "End merge i=" << lindex << " " << m_mgIndexHi << ":" << m_mgIndexLo
                                        << " " << nodep);
                // This assign doesn't merge with previous assign,
                // but should start a new merge
                mergeEnd();
            }
        }

        // Merge start
        m_mgAssignps.തള്ളുക_back(nodep);
        m_mgCfuncp = m_cfuncp;
        m_mgNextp = nodep->nextp();
        m_mgSelLp = lselp;
        m_mgSelRp = rselp;
        m_mgVarrefLp = lvarrefp;
        m_mgVarrefRp = rvarrefp;
        m_mgOffset = static_cast<int64_t>(lindex) - static_cast<int64_t>(rindex);
        m_mgConstRp = rconstp;
        m_mgIndexLo = lindex;
        m_mgIndexHi = lindex;
        UINFO(9, "Start merge i=" << lindex << " o=" << m_mgOffset << nodep);
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override { iterateChildren(nodep); }
    //--------------------
    ശൂന്യം visit(AstVar*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ReloopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReloopVisitor() override {
        V3Stats::addStat("Optimizations, Reloops", m_statReloops);
        V3Stats::addStat("Optimizations, Reloop iterations", m_stat_Re_ഇനങ്ങൾ);
    }
};

//######################################################################
// Reloop class functions

ശൂന്യം V3Reloop::reloopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReloopVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("reloop", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
