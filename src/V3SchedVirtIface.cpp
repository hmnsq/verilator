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

বর্গ VirtIfaceVisitor final : সার্বজনিক VNVisitor {
গুপ্ত:
    // STATE
    using IfaceMember = std::pair<ধ্রুব AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ধ্রুব AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    স্থির তার্কিক writesToVirtIface(ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep) {
        প্রত্যাগম nodep->exists([](ধ্রুব AstMemberSel* ধ্রুব memberSelp) {
            যদি (!memberSelp->access().isWriteOrRW()) প্রত্যাগম অসত্য;
            AstIfaceRefDType* ধ্রুব dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            প্রত্যাগম dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    শূন্য visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        যদি (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ধ্রুব dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            যদি (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstCMethodCall* nodep) override {
        যদি (ধ্রুব AstIfaceRefDType* ধ্রুব dtypep
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
        AstVar* ধ্রুব varp = nodep->varp();
        যদি (varp->isTemp()) প্রত্যাগম;
        m_vscpsByVar[varp].ঠেলো_back(nodep);
        যদি (ধ্রুব AstIface* ধ্রুব ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        যদি (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(অসত্য); });
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    শূন্য addCandidateVscp(ধ্রুব AstIface* ধ্রুব ifacep, AstVarScope* ধ্রুব vscp) {
        ধ্রুব IfaceMember member{ifacep, vscp->varp()->name()};
        যদি (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].ঠেলো_back(vscp);
    }

    শূন্য addReachableIfaceCallable(AstIface* ধ্রুব ifacep, AstCFunc* ধ্রুব funcp) {
        ধ্রুব IfaceCallable callable{ifacep, funcp};
        যদি (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.ঠেলো_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    শূন্য buildTriggers() {
        প্রতি (ধ্রুব auto& written : m_vifWrittenMembers) {
            ধ্রুব auto it = m_candidateVscps.find(written);
            যদি (it == m_candidateVscps.end()) অনুবর্ত;
            প্রতি (AstVarScope* ধ্রুব vscp : it->second) {
                ধ্রুব AstIface* ধ্রুব ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        প্রতি (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ধ্রুব IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([এতত্, callable](Ast_গ্রন্থি_রাশিমালা* ধ্রুব nodep) {
                যদি (AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ধ্রুব varp = refp->varp();
                    যদি (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        প্রত্যাগম;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ধ্রুব auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    প্রতি (AstVarScope* ধ্রুব vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } অন্যথা যদি (Ast_গ্রন্থি_CCall* ধ্রুব callp = VN_CAST(nodep, গ্রন্থি_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = স্বভাব;

    // METHODS
    VirtIfaceTriggers take_triggers() { প্রত্যাগম std::move(m_triggers); }
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
    প্রত্যাগম triggers;
}

}  //namespace V3Sched
