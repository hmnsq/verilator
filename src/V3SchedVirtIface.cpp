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
    उपयुज्य IfaceMember = std::pair<ध्रुव AstIface*, std::string>;
    उपयुज्य IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::समुच्चयः<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::सदिशम्<AstVarScope*>> m_candidateVscps;
    std::समुच्चयः<std::pair<IfaceMember, AstVarScope*>> m_आलोकितम्_Candidate_Vscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<ध्रुव AstVar*, std::सदिशम्<AstVarScope*>> m_vscpsByVar;
    std::सदिशम्<IfaceCallable> m_reachableIfaceCallables;
    std::समुच्चयः<IfaceCallable> m_आलोकितम्_Reachable_Iface_Callables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    स्थिर तार्किक writes_गन्तव्यम्_Virt_Iface(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        प्रत्यागम nodep->exists([](ध्रुव AstMemberSel* ध्रुव memberSelp) {
            यदि (!memberSelp->access().isWriteOrRW()) प्रत्यागम असत्य;
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            प्रत्यागम dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    शून्य दर्शय(AstMemberSel* nodep) override {
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
    शून्य दर्शय(AstCMethodCall* nodep) override {
        यदि (ध्रुव AstIfaceRefDType* ध्रुव dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            यदि (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } अन्यथा {
                सङ्कलय_Reachable_Iface_Callable(dtypep->iface_द्वारा_Cellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* ध्रुव varp = nodep->varp();
        यदि (varp->is_अस्थायी()) प्रत्यागम;
        m_vscpsByVar[varp].प्रक्षिप_पृष्ठभागः(nodep);
        यदि (ध्रुव AstIface* ध्रुव ifacep = varp->sensIfacep()) सङ्कलय_Candidate_Vscp(ifacep, nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        यदि (VN_IS(nodep, AlwaysPost) && writes_गन्तव्यम्_Virt_Iface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(असत्य); });
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    शून्य सङ्कलय_Candidate_Vscp(ध्रुव AstIface* ध्रुव ifacep, AstVarScope* ध्रुव vscp) {
        ध्रुव IfaceMember member{ifacep, vscp->varp()->name()};
        यदि (m_आलोकितम्_Candidate_Vscps.emplace(member, vscp).second)
            m_candidateVscps[member].प्रक्षिप_पृष्ठभागः(vscp);
    }

    शून्य सङ्कलय_Reachable_Iface_Callable(AstIface* ध्रुव ifacep, AstCFunc* ध्रुव funcp) {
        ध्रुव IfaceCallable callable{ifacep, funcp};
        यदि (m_आलोकितम्_Reachable_Iface_Callables.emplace(callable).second) {
            m_reachableIfaceCallables.प्रक्षिप_पृष्ठभागः(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    शून्य buildTriggers() {
        प्रति (ध्रुव स्वचालित& written : m_vifWrittenMembers) {
            ध्रुव स्वचालित it = m_candidateVscps.find(written);
            यदि (it == m_candidateVscps.अन्तः_2()) अनुवर्त;
            प्रति (AstVarScope* ध्रुव vscp : it->second) {
                ध्रुव AstIface* ध्रुव ifacep = written.first;
                m_triggers.सङ्कलय_Trigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        प्रति (आकार_t i = 0; i < m_reachableIfaceCallables.आकार(); ++i) {
            ध्रुव IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([एतत्, callable](Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
                यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* ध्रुव varp = refp->varp();
                    यदि (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_अस्थायी()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        प्रत्यागम;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    ध्रुव स्वचालित it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.अन्तः_2(), varp,
                                "No VarScope for interface member");
                    प्रति (AstVarScope* ध्रुव vscp : it->second)
                        सङ्कलय_Candidate_Vscp(callable.first, vscp);
                } अन्यथा यदि (Ast_ग्रन्थिः_CCall* ध्रुव callp = VN_CAST(nodep, ग्रन्थिः_CCall)) {
                    सङ्कलय_Reachable_Iface_Callable(callable.first, callp->funcp());
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
        V3Global::dump_Check_Global_वृक्षः("sched_vif", 0, dump_वृक्षः_Either_Level() >= 6);
    }
    प्रत्यागम triggers;
}

}  //namespace V3Sched
