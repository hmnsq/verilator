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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3AstNodeExpr.h"
#অন্তর্ভাব "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

শ্রেণী VirtIfaceVisitor final : সর্বজনীন VNVisitor {
ব্যক্তিগত:
    // STATE
    using IfaceMember = std::pair<ধ্রুবক AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ধ্রুবক AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    স্থির যৌক্তিক writesToVirtIface(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep) {
        ফেরত nodep->exists([](ধ্রুবক AstMemberSel* ধ্রুবক memberSelp) {
            যদি (!memberSelp->access().isWriteOrRW()) ফেরত মিথ্যা;
            AstIfaceRefDType* ধ্রুবক dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ফেরত dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    শূন্য visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        যদি (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ধ্রুবক dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            যদি (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstCMethodCall* nodep) override {
        যদি (ধ্রুবক AstIfaceRefDType* ধ্রুবক dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            যদি (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } অন্যথা {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ধ্রুবক varp = nodep->varp();
        যদি (varp->is_অস্থায়ী()) ফেরত;
        m_vscpsByVar[varp].ঠেলো_back(nodep);
        যদি (ধ্রুবক AstIface* ধ্রুবক ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        যদি (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(মিথ্যা); });
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    শূন্য addCandidateVscp(ধ্রুবক AstIface* ধ্রুবক ifacep, AstVarScope* ধ্রুবক vscp) {
        ধ্রুবক IfaceMember member{ifacep, vscp->varp()->name()};
        যদি (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].ঠেলো_back(vscp);
    }

    শূন্য addReachableIfaceCallable(AstIface* ধ্রুবক ifacep, AstCFunc* ধ্রুবক funcp) {
        ধ্রুবক IfaceCallable callable{ifacep, funcp};
        যদি (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.ঠেলো_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    শূন্য buildTriggers() {
        জন্য (ধ্রুবক auto& written : m_vifWrittenMembers) {
            ধ্রুবক auto it = m_candidateVscps.find(written);
            যদি (it == m_candidateVscps.end()) চালু;
            জন্য (AstVarScope* ধ্রুবক vscp : it->second) {
                ধ্রুবক AstIface* ধ্রুবক ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        জন্য (আকার_t i = 0; i < m_reachableIfaceCallables.আকার_2(); ++i) {
            ধ্রুবক IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([এই, callable](Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nodep) {
                যদি (AstVarRef* ধ্রুবক refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ধ্রুবক varp = refp->varp();
                    যদি (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_অস্থায়ী()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        ফেরত;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ধ্রুবক auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    জন্য (AstVarScope* ধ্রুবক vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } অন্যথা যদি (Ast_গ্রন্থি_CCall* ধ্রুবক callp = VN_CAST(nodep, গ্রন্থি_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = পূর্বনির্ধারিত;

    // METHODS
    VirtIfaceTriggers take_triggers() { ফেরত std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    যদি (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_বৃক্ষ("sched_vif", 0, dump_বৃক্ষ_Either_Level() >= 6);
    }
    ফেরত triggers;
}

}  //namespace V3Sched
