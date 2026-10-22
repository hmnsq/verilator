// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create triggers necessary for scheduling across
//                         virtual interfaces
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
// V3SchedVirtIface's Transformations:
//
// Identify interface members written through virtual interface variables (VIF writes).
// For each such member, collect all VarScopes across all instantiations of that
// interface type. Each VarScope gets its own value-change trigger in the scheduler.
//
// Also disables lifetime optimization for variables in AlwaysPost blocks that
// write through virtual interfaces.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3AstNodeExpr.h"
#ضمن "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

صنف VirtIfaceVisitor final : عام VNVisitor {
خاص:
    // STATE
    کارول IfaceMember = std::pair<ثابت AstIface*, std::string>;
    کارول IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::سېټ<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::ویکټور<AstVarScope*>> m_candidateVscps;
    std::سېټ<std::pair<IfaceMember, AstVarScope*>> m_کتل_شوی_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ثابت AstVar*, std::ویکټور<AstVarScope*>> m_vscpsByVar;
    std::ویکټور<IfaceCallable> m_reachableIfaceCallables;
    std::سېټ<IfaceCallable> m_کتل_شوی_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ساكن منطقي writes_منزل_Virt_Iface(ثابت Ast_غوټه* ثابت nodep) {
        أرجع nodep->exists([](ثابت AstMemberSel* ثابت memberSelp) {
            إذا (!memberSelp->access().isWriteOrRW()) أرجع خطأ;
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            أرجع dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    فراغ لیدنه(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        إذا (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            إذا (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCMethodCall* nodep) override {
        إذا (ثابت AstIfaceRefDType* ثابت dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            إذا (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } وإلا {
                جمع_کړه_Reachable_Iface_Callable(dtypep->iface_له_لارې_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ثابت varp = nodep->varp();
        إذا (varp->is_لنډمهاله()) أرجع;
        m_vscpsByVar[varp].ټېل_شا(nodep);
        إذا (ثابت AstIface* ثابت ifacep = varp->sensIfacep()) جمع_کړه_Candidate_Vscp(ifacep, nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        إذا (VN_IS(nodep, AlwaysPost) && writes_منزل_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(خطأ); });
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

    فراغ جمع_کړه_Candidate_Vscp(ثابت AstIface* ثابت ifacep, AstVarScope* ثابت vscp) {
        ثابت IfaceMember member{ifacep, vscp->varp()->name()};
        إذا (m_کتل_شوی_Candidate_Vscps.emplace(member, vscp).دوهم)
            m_candidateVscps[member].ټېل_شا(vscp);
    }

    فراغ جمع_کړه_Reachable_Iface_Callable(AstIface* ثابت ifacep, AstCFunc* ثابت funcp) {
        ثابت IfaceCallable callable{ifacep, funcp};
        إذا (m_کتل_شوی_Reachable_Iface_Callables.emplace(callable).دوهم) {
            m_reachableIfaceCallables.ټېل_شا(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    فراغ buildTriggers() {
        لكل (ثابت خپلکاری& written : m_vifWrittenMembers) {
            ثابت خپلکاری it = m_candidateVscps.پیدا_کړه(written);
            إذا (it == m_candidateVscps.پای_ټکی()) استمر;
            لكل (AstVarScope* ثابت vscp : it->دوهم) {
                ثابت AstIface* ثابت ifacep = written.لومړی;
                m_triggers.جمع_کړه_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

عام:
    // CONSTRUCTORS
    څرګند VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        لكل (اندازه_t i = 0; i < m_reachableIfaceCallables.اندازه(); ++i) {
            ثابت IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.دوهم->foreach([هذا, callable](Ast_غوټه_بیان* ثابت nodep) {
                إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ثابت varp = refp->varp();
                    إذا (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_لنډمهاله()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        أرجع;
                    }
                    varp->sensIfacep(callable.لومړی);
                    m_vifWrittenMembers.emplace(callable.لومړی, varp->name());
                    ثابت خپلکاری it = m_vscpsByVar.پیدا_کړه(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.پای_ټکی(), varp,
                                "No VarScope for interface member");
                    لكل (AstVarScope* ثابت vscp : it->دوهم)
                        جمع_کړه_Candidate_Vscp(callable.لومړی, vscp);
                } وإلا إذا (Ast_غوټه_CCall* ثابت callp = VN_CAST(nodep, غوټه_CCall)) {
                    جمع_کړه_Reachable_Iface_Callable(callable.لومړی, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = افتراضي;

    // METHODS
    VirtIfaceTriggers take_triggers() { أرجع std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    إذا (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_ونه("sched_vif", 0, dump_ونه_Either_Level() >= 6);
    }
    أرجع triggers;
}

}  //namespace V3Sched
