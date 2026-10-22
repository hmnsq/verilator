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

فضاینام V3Sched {

فضاینام {

رده VirtIfaceVisitor final : عمومی VNVisitor {
خصوصی:
    // STATE
    using IfaceMember = std::pair<ثابت AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ثابت AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ایستا منطقی writesToVirtIface(ثابت Ast_گره* ثابت nodep) {
        بازگشت nodep->exists([](ثابت AstMemberSel* ثابت memberSelp) {
            اگر (!memberSelp->access().isWriteOrRW()) بازگشت نادرست;
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            بازگشت dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    تهی visit(AstMemberSel* nodep) override {
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
    تهی visit(AstCMethodCall* nodep) override {
        اگر (ثابت AstIfaceRefDType* ثابت dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            اگر (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } وگرنه {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ثابت varp = nodep->varp();
        اگر (varp->isTemp()) بازگشت;
        m_vscpsByVar[varp].افزودن_back(nodep);
        اگر (ثابت AstIface* ثابت ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        اگر (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(نادرست); });
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    تهی addCandidateVscp(ثابت AstIface* ثابت ifacep, AstVarScope* ثابت vscp) {
        ثابت IfaceMember member{ifacep, vscp->varp()->name()};
        اگر (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].افزودن_back(vscp);
    }

    تهی addReachableIfaceCallable(AstIface* ثابت ifacep, AstCFunc* ثابت funcp) {
        ثابت IfaceCallable callable{ifacep, funcp};
        اگر (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.افزودن_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    تهی buildTriggers() {
        برای (ثابت auto& written : m_vifWrittenMembers) {
            ثابت auto it = m_candidateVscps.find(written);
            اگر (it == m_candidateVscps.end()) ادامه;
            برای (AstVarScope* ثابت vscp : it->second) {
                ثابت AstIface* ثابت ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

عمومی:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        برای (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ثابت IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([این, callable](Ast_گره_عبارت* ثابت nodep) {
                اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ثابت varp = refp->varp();
                    اگر (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        بازگشت;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ثابت auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    برای (AstVarScope* ثابت vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } وگرنه اگر (Ast_گره_CCall* ثابت callp = VN_CAST(nodep, گره_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = پیشفرض;

    // METHODS
    VirtIfaceTriggers take_triggers() { بازگشت std::move(m_triggers); }
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
    بازگشت triggers;
}

}  //namespace V3Sched
