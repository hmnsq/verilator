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
    ساكن منطقي writesToVirtIface(ثابت Ast_گرێ* ثابت nodep) {
        أرجع nodep->exists([](ثابت AstMemberSel* ثابت memberSelp) {
            إذا (!memberSelp->access().isWriteOrRW()) أرجع خطأ;
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            أرجع dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    فراغ visit(AstMemberSel* nodep) override {
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
    فراغ visit(AstCMethodCall* nodep) override {
        إذا (ثابت AstIfaceRefDType* ثابت dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            إذا (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } وإلا {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ثابت varp = nodep->varp();
        إذا (varp->isTemp()) أرجع;
        m_vscpsByVar[varp].پاڵنان_back(nodep);
        إذا (ثابت AstIface* ثابت ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    فراغ visit(Ast_گرێ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        إذا (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(خطأ); });
        }
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرێ* nodep) override { iterateChildren(nodep); }

    فراغ addCandidateVscp(ثابت AstIface* ثابت ifacep, AstVarScope* ثابت vscp) {
        ثابت IfaceMember member{ifacep, vscp->varp()->name()};
        إذا (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].پاڵنان_back(vscp);
    }

    فراغ addReachableIfaceCallable(AstIface* ثابت ifacep, AstCFunc* ثابت funcp) {
        ثابت IfaceCallable callable{ifacep, funcp};
        إذا (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.پاڵنان_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    فراغ buildTriggers() {
        لكل (ثابت auto& written : m_vifWrittenMembers) {
            ثابت auto it = m_candidateVscps.find(written);
            إذا (it == m_candidateVscps.end()) استمر;
            لكل (AstVarScope* ثابت vscp : it->second) {
                ثابت AstIface* ثابت ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

عام:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        لكل (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ثابت IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([هذا, callable](Ast_گرێ_دەربڕین* ثابت nodep) {
                إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ثابت varp = refp->varp();
                    إذا (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        أرجع;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ثابت auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    لكل (AstVarScope* ثابت vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } وإلا إذا (Ast_گرێ_CCall* ثابت callp = VN_CAST(nodep, گرێ_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
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
        V3Global::dump_Check_Global_درەخت("sched_vif", 0, dump_درەخت_Either_Level() >= 6);
    }
    أرجع triggers;
}

}  //namespace V3Sched
