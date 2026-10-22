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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3AstNodeExpr.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

ವರ್ಗ VirtIfaceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
ಗುಪ್ತ:
    // STATE
    using IfaceMember = std::pair<ಧ್ರುವ AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ಧ್ರುವ AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ಸ್ಥಿರ ತಾರ್ಕಿಕ writesToVirtIface(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        ಪ್ರತ್ಯಾಗಮ nodep->exists([](ಧ್ರುವ AstMemberSel* ಧ್ರುವ memberSelp) {
            ಯದಿ (!memberSelp->access().isWriteOrRW()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            AstIfaceRefDType* ಧ್ರುವ dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ಪ್ರತ್ಯಾಗಮ dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        ಯದಿ (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ಧ್ರುವ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ಯದಿ (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCMethodCall* nodep) override {
        ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            ಯದಿ (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } ಅನ್ಯಥಾ {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (varp->isTemp()) ಪ್ರತ್ಯಾಗಮ;
        m_vscpsByVar[varp].ತಳ್ಳು_back(nodep);
        ಯದಿ (ಧ್ರುವ AstIface* ಧ್ರುವ ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        ಯದಿ (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(ಅಸತ್ಯ); });
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ addCandidateVscp(ಧ್ರುವ AstIface* ಧ್ರುವ ifacep, AstVarScope* ಧ್ರುವ vscp) {
        ಧ್ರುವ IfaceMember member{ifacep, vscp->varp()->name()};
        ಯದಿ (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].ತಳ್ಳು_back(vscp);
    }

    ಶೂನ್ಯ addReachableIfaceCallable(AstIface* ಧ್ರುವ ifacep, AstCFunc* ಧ್ರುವ funcp) {
        ಧ್ರುವ IfaceCallable callable{ifacep, funcp};
        ಯದಿ (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.ತಳ್ಳು_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    ಶೂನ್ಯ buildTriggers() {
        ಪ್ರತಿ (ಧ್ರುವ auto& written : m_vifWrittenMembers) {
            ಧ್ರುವ auto it = m_candidateVscps.find(written);
            ಯದಿ (it == m_candidateVscps.end()) ಅನುವರ್ತ;
            ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : it->second) {
                ಧ್ರುವ AstIface* ಧ್ರುವ ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        ಪ್ರತಿ (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ಧ್ರುವ IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([ಏತತ್, callable](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
                ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ಧ್ರುವ varp = refp->varp();
                    ಯದಿ (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ಧ್ರುವ auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_CCall* ಧ್ರುವ callp = VN_CAST(nodep, ಗಂಟು_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = ಸ್ವಭಾವ;

    // METHODS
    VirtIfaceTriggers take_triggers() { ಪ್ರತ್ಯಾಗಮ std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    ಯದಿ (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_ವೃಕ್ಷ("sched_vif", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
    }
    ಪ್ರತ್ಯಾಗಮ triggers;
}

}  //namespace V3Sched
