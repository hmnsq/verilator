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
    استفاده IfaceMember = std::pair<ثابت AstIface*, std::string>;
    استفاده IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::مجموعه<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::وکتور<AstVarScope*>> m_candidateVscps;
    std::مجموعه<std::pair<IfaceMember, AstVarScope*>> m_دیده_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ثابت AstVar*, std::وکتور<AstVarScope*>> m_vscpsByVar;
    std::وکتور<IfaceCallable> m_reachableIfaceCallables;
    std::مجموعه<IfaceCallable> m_دیده_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ساكن منطقي writes_مقصد_Virt_Iface(ثابت Ast_گره* ثابت nodep) {
        أرجع nodep->exists([](ثابت AstMemberSel* ثابت memberSelp) {
            إذا (!memberSelp->access().isWriteOrRW()) أرجع خطأ;
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            أرجع dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    فراغ بازدید(AstMemberSel* nodep) override {
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
    فراغ بازدید(AstCMethodCall* nodep) override {
        إذا (ثابت AstIfaceRefDType* ثابت dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            إذا (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } وإلا {
                جمع_کن_Reachable_Iface_Callable(dtypep->iface_از_طریق_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ثابت varp = nodep->varp();
        إذا (varp->is_موقت()) أرجع;
        m_vscpsByVar[varp].افزودن_پشت(nodep);
        إذا (ثابت AstIface* ثابت ifacep = varp->sensIfacep()) جمع_کن_Candidate_Vscp(ifacep, nodep);
    }
    فراغ بازدید(Ast_گره_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        إذا (VN_IS(nodep, AlwaysPost) && writes_مقصد_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(خطأ); });
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    فراغ جمع_کن_Candidate_Vscp(ثابت AstIface* ثابت ifacep, AstVarScope* ثابت vscp) {
        ثابت IfaceMember member{ifacep, vscp->varp()->name()};
        إذا (m_دیده_Candidate_Vscps.emplace(member, vscp).دومی)
            m_candidateVscps[member].افزودن_پشت(vscp);
    }

    فراغ جمع_کن_Reachable_Iface_Callable(AstIface* ثابت ifacep, AstCFunc* ثابت funcp) {
        ثابت IfaceCallable callable{ifacep, funcp};
        إذا (m_دیده_Reachable_Iface_Callables.emplace(callable).دومی) {
            m_reachableIfaceCallables.افزودن_پشت(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    فراغ buildTriggers() {
        لكل (ثابت خودکار& written : m_vifWrittenMembers) {
            ثابت خودکار it = m_candidateVscps.پیدا_کن(written);
            إذا (it == m_candidateVscps.انتها()) استمر;
            لكل (AstVarScope* ثابت vscp : it->دومی) {
                ثابت AstIface* ثابت ifacep = written.اولی;
                m_triggers.جمع_کن_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

عام:
    // CONSTRUCTORS
    صریح VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        لكل (اندازه_t i = 0; i < m_reachableIfaceCallables.اندازه(); ++i) {
            ثابت IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.دومی->foreach([هذا, callable](Ast_گره_عبارت* ثابت nodep) {
                إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ثابت varp = refp->varp();
                    إذا (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_موقت()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        أرجع;
                    }
                    varp->sensIfacep(callable.اولی);
                    m_vifWrittenMembers.emplace(callable.اولی, varp->name());
                    ثابت خودکار it = m_vscpsByVar.پیدا_کن(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.انتها(), varp,
                                "No VarScope for interface member");
                    لكل (AstVarScope* ثابت vscp : it->دومی)
                        جمع_کن_Candidate_Vscp(callable.اولی, vscp);
                } وإلا إذا (Ast_گره_CCall* ثابت callp = VN_CAST(nodep, گره_CCall)) {
                    جمع_کن_Reachable_Iface_Callable(callable.اولی, callp->funcp());
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
        V3Global::dump_Check_Global_درخت("sched_vif", 0, dump_درخت_Either_Level() >= 6);
    }
    أرجع triggers;
}

}  //namespace V3Sched
