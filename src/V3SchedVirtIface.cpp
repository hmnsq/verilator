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
ಖಾಸಗಿ:
    // STATE
    ಬಳಸಿ IfaceMember = std::pair<ಸ್ಥಿರಾಂಕ AstIface*, std::string>;
    ಬಳಸಿ IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::ಗಣ<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::ಸದಿಶ<AstVarScope*>> m_candidateVscps;
    std::ಗಣ<std::pair<IfaceMember, AstVarScope*>> m_ನೋಡಿದ_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ಸ್ಥಿರಾಂಕ AstVar*, std::ಸದಿಶ<AstVarScope*>> m_vscpsByVar;
    std::ಸದಿಶ<IfaceCallable> m_reachableIfaceCallables;
    std::ಗಣ<IfaceCallable> m_ನೋಡಿದ_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    ಸ್ಥಿರ ತಾರ್ಕಿಕ writes_ಗಮ್ಯ_Virt_Iface(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        ಹಿಂತಿರುಗಿ nodep->exists([](ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp) {
            ಆದರೆ (!memberSelp->access().isWriteOrRW()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ಹಿಂತಿರುಗಿ dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ಆದರೆ (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodCall* nodep) override {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            ಆದರೆ (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಕೂಡಿಸು_Reachable_Iface_Callable(dtypep->iface_ಮೂಲಕ_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (varp->is_ತಾತ್ಕಾಲಿಕ()) ಹಿಂತಿರುಗಿ;
        m_vscpsByVar[varp].ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep = varp->sensIfacep()) ಕೂಡಿಸು_Candidate_Vscp(ifacep, nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        ಆದರೆ (VN_IS(nodep, AlwaysPost) && writes_ಗಮ್ಯ_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(ಸುಳ್ಳು); });
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ ಕೂಡಿಸು_Candidate_Vscp(ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep, AstVarScope* ಸ್ಥಿರಾಂಕ vscp) {
        ಸ್ಥಿರಾಂಕ IfaceMember member{ifacep, vscp->varp()->name()};
        ಆದರೆ (m_ನೋಡಿದ_Candidate_Vscps.emplace(member, vscp).ಎರಡನೇ)
            m_candidateVscps[member].ತಳ್ಳು_ಹಿಂಭಾಗ(vscp);
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Reachable_Iface_Callable(AstIface* ಸ್ಥಿರಾಂಕ ifacep, AstCFunc* ಸ್ಥಿರಾಂಕ funcp) {
        ಸ್ಥಿರಾಂಕ IfaceCallable callable{ifacep, funcp};
        ಆದರೆ (m_ನೋಡಿದ_Reachable_Iface_Callables.emplace(callable).ಎರಡನೇ) {
            m_reachableIfaceCallables.ತಳ್ಳು_ಹಿಂಭಾಗ(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    ಶೂನ್ಯ buildTriggers() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& written : m_vifWrittenMembers) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_candidateVscps.ಕಂಡುಹಿಡಿ(written);
            ಆದರೆ (it == m_candidateVscps.ಕೊನೆ()) ಮುಂದುವರಿ;
            ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : it->ಎರಡನೇ) {
                ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep = written.ಮೊದಲ;
                m_triggers.ಕೂಡಿಸು_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < m_reachableIfaceCallables.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.ಎರಡನೇ->foreach([ಇದು, callable](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
                ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
                    ಆದರೆ (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_ತಾತ್ಕಾಲಿಕ()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        ಹಿಂತಿರುಗಿ;
                    }
                    varp->sensIfacep(callable.ಮೊದಲ);
                    m_vifWrittenMembers.emplace(callable.ಮೊದಲ, varp->name());
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_vscpsByVar.ಕಂಡುಹಿಡಿ(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.ಕೊನೆ(), varp,
                                "No VarScope for interface member");
                    ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : it->ಎರಡನೇ)
                        ಕೂಡಿಸು_Candidate_Vscp(callable.ಮೊದಲ, vscp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ callp = VN_CAST(nodep, ಗಂಟು_CCall)) {
                    ಕೂಡಿಸು_Reachable_Iface_Callable(callable.ಮೊದಲ, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    VirtIfaceTriggers take_triggers() { ಹಿಂತಿರುಗಿ std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    ಆದರೆ (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_ವೃಕ್ಷ("sched_vif", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
    }
    ಹಿಂತಿರುಗಿ triggers;
}

}  //namespace V3Sched
