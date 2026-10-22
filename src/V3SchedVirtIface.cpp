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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AstNodeExpr.h"
#अन्तर्भाव "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

वग्ग VirtIfaceVisitor final : सार्वजनिक VNVisitor {
गुप्त:
    // STATE
    using IfaceMember = std::pair<ध्रुव AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ध्रुव AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    स्थिर तार्किक writesToVirtIface(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        पटिनिवत्त nodep->exists([](ध्रुव AstMemberSel* ध्रुव memberSelp) {
            सचे (!memberSelp->access().isWriteOrRW()) पटिनिवत्त मिच्छा;
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            पटिनिवत्त dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    शून्य visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        सचे (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            सचे (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCMethodCall* nodep) override {
        सचे (ध्रुव AstIfaceRefDType* ध्रुव dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            सचे (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } अञ्ञथा {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ध्रुव varp = nodep->varp();
        सचे (varp->isTemp()) पटिनिवत्त;
        m_vscpsByVar[varp].प्रक्षिप_back(nodep);
        सचे (ध्रुव AstIface* ध्रुव ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        सचे (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(मिच्छा); });
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    शून्य addCandidateVscp(ध्रुव AstIface* ध्रुव ifacep, AstVarScope* ध्रुव vscp) {
        ध्रुव IfaceMember member{ifacep, vscp->varp()->name()};
        सचे (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].प्रक्षिप_back(vscp);
    }

    शून्य addReachableIfaceCallable(AstIface* ध्रुव ifacep, AstCFunc* ध्रुव funcp) {
        ध्रुव IfaceCallable callable{ifacep, funcp};
        सचे (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.प्रक्षिप_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    शून्य buildTriggers() {
        अत्थाय (ध्रुव auto& written : m_vifWrittenMembers) {
            ध्रुव auto it = m_candidateVscps.find(written);
            सचे (it == m_candidateVscps.end()) अनुवर्त;
            अत्थाय (AstVarScope* ध्रुव vscp : it->second) {
                ध्रुव AstIface* ध्रुव ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        अत्थाय (size_t i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ध्रुव IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([एतत्, callable](Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
                सचे (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ध्रुव varp = refp->varp();
                    सचे (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        पटिनिवत्त;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ध्रुव auto it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    अत्थाय (AstVarScope* ध्रुव vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } अञ्ञथा सचे (Ast_ग्रन्थिः_CCall* ध्रुव callp = VN_CAST(nodep, ग्रन्थिः_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = स्वभाव;

    // METHODS
    VirtIfaceTriggers take_triggers() { पटिनिवत्त std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    सचे (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_वृक्षः("sched_vif", 0, dump_वृक्षः_Either_Level() >= 6);
    }
    पटिनिवत्त triggers;
}

}  //namespace V3Sched
