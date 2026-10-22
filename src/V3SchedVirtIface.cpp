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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AstNodeExpr.h"
#شامل "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا V3Sched {

نامخلا {

جماعت VirtIfaceVisitor final : عوامی VNVisitor {
نجی:
    // STATE
    استعمال IfaceMember = std::pair<ثابت AstIface*, std::string>;
    استعمال IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::سیٹ<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::سمتیہ<AstVarScope*>> m_candidateVscps;
    std::سیٹ<std::pair<IfaceMember, AstVarScope*>> m_نظر_آیا_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ثابت AstVar*, std::سمتیہ<AstVarScope*>> m_vscpsByVar;
    std::سمتیہ<IfaceCallable> m_reachableIfaceCallables;
    std::سیٹ<IfaceCallable> m_نظر_آیا_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ساکن منطقی writes_مقصد_Virt_Iface(ثابت Ast_گرہ* ثابت nodep) {
        واپس nodep->exists([](ثابت AstMemberSel* ثابت memberSelp) {
            اگر (!memberSelp->access().isWriteOrRW()) واپس جھوٹ;
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            واپس dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    خالی دورہ(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        اگر (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            اگر (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstCMethodCall* nodep) override {
        اگر (ثابت AstIfaceRefDType* ثابت dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            اگر (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } ورنہ {
                جوڑو_Reachable_Iface_Callable(dtypep->iface_بذریعہ_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ثابت varp = nodep->varp();
        اگر (varp->is_عارضی()) واپس;
        m_vscpsByVar[varp].ڈالو_عقب(nodep);
        اگر (ثابت AstIface* ثابت ifacep = varp->sensIfacep()) جوڑو_Candidate_Vscp(ifacep, nodep);
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        اگر (VN_IS(nodep, AlwaysPost) && writes_مقصد_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(جھوٹ); });
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    خالی جوڑو_Candidate_Vscp(ثابت AstIface* ثابت ifacep, AstVarScope* ثابت vscp) {
        ثابت IfaceMember member{ifacep, vscp->varp()->name()};
        اگر (m_نظر_آیا_Candidate_Vscps.emplace(member, vscp).دوسرا)
            m_candidateVscps[member].ڈالو_عقب(vscp);
    }

    خالی جوڑو_Reachable_Iface_Callable(AstIface* ثابت ifacep, AstCFunc* ثابت funcp) {
        ثابت IfaceCallable callable{ifacep, funcp};
        اگر (m_نظر_آیا_Reachable_Iface_Callables.emplace(callable).دوسرا) {
            m_reachableIfaceCallables.ڈالو_عقب(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    خالی buildTriggers() {
        برائے (ثابت خودکار& written : m_vifWrittenMembers) {
            ثابت خودکار it = m_candidateVscps.ڈھونڈو(written);
            اگر (it == m_candidateVscps.انتہا()) جاری;
            برائے (AstVarScope* ثابت vscp : it->دوسرا) {
                ثابت AstIface* ثابت ifacep = written.پہلا;
                m_triggers.جوڑو_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

عوامی:
    // CONSTRUCTORS
    صریح VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        برائے (سائز_t i = 0; i < m_reachableIfaceCallables.سائز(); ++i) {
            ثابت IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.دوسرا->foreach([یہ, callable](Ast_گرہ_اظہاریہ* ثابت nodep) {
                اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ثابت varp = refp->varp();
                    اگر (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_عارضی()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        واپس;
                    }
                    varp->sensIfacep(callable.پہلا);
                    m_vifWrittenMembers.emplace(callable.پہلا, varp->name());
                    ثابت خودکار it = m_vscpsByVar.ڈھونڈو(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.انتہا(), varp,
                                "No VarScope for interface member");
                    برائے (AstVarScope* ثابت vscp : it->دوسرا)
                        جوڑو_Candidate_Vscp(callable.پہلا, vscp);
                } ورنہ اگر (Ast_گرہ_CCall* ثابت callp = VN_CAST(nodep, گرہ_CCall)) {
                    جوڑو_Reachable_Iface_Callable(callable.پہلا, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = طےشدہ;

    // METHODS
    VirtIfaceTriggers take_triggers() { واپس std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    اگر (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_درخت("sched_vif", 0, dump_درخت_Either_Level() >= 6);
    }
    واپس triggers;
}

}  //namespace V3Sched
