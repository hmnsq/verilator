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

वर्ग VirtIfaceVisitor final : सार्वजनिक VNVisitor {
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
    स्थिर तार्किक writesToVirtIface(ध्रुव Ast_गाँठ* ध्रुव nodep) {
        प्रत्यागम nodep->exists([](ध्रुव AstMemberSel* ध्रुव memberSelp) {
            यदि (!memberSelp->access().isWriteOrRW()) प्रत्यागम असत्य;
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            प्रत्यागम dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    शून्य visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        यदि (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            यदि (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCMethodCall* nodep) override {
        यदि (ध्रुव AstIfaceRefDType* ध्रुव dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            यदि (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } अन्यथा {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ध्रुव varp = nodep->varp();
        यदि (varp->isTemp()) प्रत्यागम;
        m_vscpsByVar[varp].धकेलो_back(nodep);
        यदि (ध्रुव AstIface* ध्रुव ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    शून्य visit(Ast_गाँठ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        यदि (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(असत्य); });
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    शून्य addCandidateVscp(ध्रुव AstIface* ध्रुव ifacep, AstVarScope* ध्रुव vscp) {
        ध्रुव IfaceMember member{ifacep, vscp->varp()->name()};
        यदि (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].धकेलो_back(vscp);
    }

    शून्य addReachableIfaceCallable(AstIface* ध्रुव ifacep, AstCFunc* ध्रुव funcp) {
        ध्रुव IfaceCallable callable{ifacep, funcp};
        यदि (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.धकेलो_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    शून्य buildTriggers() {
        प्रति (ध्रुव स्वतः& written : m_vifWrittenMembers) {
            ध्रुव स्वतः it = m_candidateVscps.find(written);
            यदि (it == m_candidateVscps.end()) अनुवर्त;
            प्रति (AstVarScope* ध्रुव vscp : it->second) {
                ध्रुव AstIface* ध्रुव ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        प्रति (माप_प्रकार i = 0; i < m_reachableIfaceCallables.size(); ++i) {
            ध्रुव IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([एतत्, callable](Ast_गाँठ_व्यंजक* ध्रुव nodep) {
                यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ध्रुव varp = refp->varp();
                    यदि (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->isTemp()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        प्रत्यागम;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ध्रुव स्वतः it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    प्रति (AstVarScope* ध्रुव vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } अन्यथा यदि (Ast_गाँठ_CCall* ध्रुव callp = VN_CAST(nodep, गाँठ_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = स्वभाव;

    // METHODS
    VirtIfaceTriggers take_triggers() { प्रत्यागम std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    यदि (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_वृक्ष("sched_vif", 0, dump_वृक्ष_Either_Level() >= 6);
    }
    प्रत्यागम triggers;
}

}  //namespace V3Sched
