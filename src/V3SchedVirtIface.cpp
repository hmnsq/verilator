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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3AstNodeExpr.h"
#அந்தர்பாவ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

நாமாகாஶ {

வகுப்பு VirtIfaceVisitor final : பொது VNVisitor {
தனிப்பட்ட:
    // STATE
    using IfaceMember = std::pair<மாறிலி AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<மாறிலி AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    நிலையான தருக்க writesToVirtIface(மாறிலி Ast_முனை* மாறிலி nodep) {
        திரும்பு nodep->exists([](மாறிலி AstMemberSel* மாறிலி memberSelp) {
            எனில் (!memberSelp->access().isWriteOrRW()) திரும்பு பொய்;
            AstIfaceRefDType* மாறிலி dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            திரும்பு dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    வெறுமை visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        எனில் (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* மாறிலி dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            எனில் (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCMethodCall* nodep) override {
        எனில் (மாறிலி AstIfaceRefDType* மாறிலி dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            எனில் (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } இல்லையெனில் {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (varp->isTemp()) திரும்பு;
        m_vscpsByVar[varp].தள்ளு_back(nodep);
        எனில் (மாறிலி AstIface* மாறிலி ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        எனில் (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(பொய்); });
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    வெறுமை addCandidateVscp(மாறிலி AstIface* மாறிலி ifacep, AstVarScope* மாறிலி vscp) {
        மாறிலி IfaceMember member{ifacep, vscp->varp()->name()};
        எனில் (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].தள்ளு_back(vscp);
    }

    வெறுமை addReachableIfaceCallable(AstIface* மாறிலி ifacep, AstCFunc* மாறிலி funcp) {
        மாறிலி IfaceCallable callable{ifacep, funcp};
        எனில் (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.தள்ளு_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    வெறுமை buildTriggers() {
        க்கு (மாறிலி auto& written : m_vifWrittenMembers) {
            மாறிலி auto it = m_candidateVscps.find(written);
            எனில் (it == m_candidateVscps.end()) தொடர்;
            க்கு (AstVarScope* மாறிலி vscp : it->second) {
                மாறிலி AstIface* மாறிலி ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

பொது:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        க்கு (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            மாறிலி IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([இது, callable](Ast_முனை_கோவை* மாறிலி nodep) {
                எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* மாறிலி varp = refp->varp();
                    எனில் (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        திரும்பு;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    மாறிலி auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    க்கு (AstVarScope* மாறிலி vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } இல்லையெனில் எனில் (Ast_முனை_CCall* மாறிலி callp = VN_CAST(nodep, முனை_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = இயல்பு;

    // METHODS
    VirtIfaceTriggers take_triggers() { திரும்பு std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    எனில் (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_மரம்("sched_vif", 0, dump_மரம்_Either_Level() >= 6);
    }
    திரும்பு triggers;
}

}  //namespace V3Sched
