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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3AstNodeExpr.h"
#समावेश "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

श्रेणी VirtIfaceVisitor final : खुला VNVisitor {
निजी:
    // STATE
    using IfaceMember = std::pair<स्थिर AstIface*, std::string>;
    using IfaceCallable = std::pair<AstIface*, AstCFunc*>;

    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<IfaceMember> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<IfaceMember, std::vector<AstVarScope*>> m_candidateVscps;
    std::set<std::pair<IfaceMember, AstVarScope*>> m_seenCandidateVscps;
    // VarScope index and callable worklist for VIF method-body writes.
    std::map<स्थिर AstVar*, std::vector<AstVarScope*>> m_vscpsByVar;
    std::vector<IfaceCallable> m_reachableIfaceCallables;
    std::set<IfaceCallable> m_seenReachableIfaceCallables;
    VirtIfaceTriggers m_triggers;

    // METHODS
    // Returns true if statement writes across a virtual interface boundary
    अटल bool writesToVirtIface(स्थिर Ast_गाँठ* स्थिर nodep) {
        वापस nodep->exists([](स्थिर AstMemberSel* स्थिर memberSelp) {
            अगर (!memberSelp->access().isWriteOrRW()) वापस false;
            AstIfaceRefDType* स्थिर dtypep
                = VN_CAST(memberSelp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            वापस dtypep && dtypep->isVirtual();
        });
    }

    // VISITORS
    व्योम visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        अगर (nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* स्थिर dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            अगर (dtypep && dtypep->isVirtual()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstCMethodCall* nodep) override {
        अगर (स्थिर AstIfaceRefDType* स्थिर dtypep
            = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
            अगर (VL_UNCOVERABLE(!dtypep->isVirtual())) {
                // Concrete interface method calls are lowered before this pass.
            } अन्यथा {
                addReachableIfaceCallable(dtypep->ifaceViaCellp(), nodep->funcp());
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        AstVar* स्थिर varp = nodep->varp();
        अगर (varp->is_अस्थायी()) वापस;
        m_vscpsByVar[varp].धकेलो_back(nodep);
        अगर (स्थिर AstIface* स्थिर ifacep = varp->sensIfacep()) addCandidateVscp(ifacep, nodep);
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        अगर (VN_IS(nodep, AlwaysPost) && writesToVirtIface(nodep)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(false); });
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    व्योम addCandidateVscp(स्थिर AstIface* स्थिर ifacep, AstVarScope* स्थिर vscp) {
        स्थिर IfaceMember member{ifacep, vscp->varp()->name()};
        अगर (m_seenCandidateVscps.emplace(member, vscp).second)
            m_candidateVscps[member].धकेलो_back(vscp);
    }

    व्योम addReachableIfaceCallable(AstIface* स्थिर ifacep, AstCFunc* स्थिर funcp) {
        स्थिर IfaceCallable callable{ifacep, funcp};
        अगर (m_seenReachableIfaceCallables.emplace(callable).second) {
            m_reachableIfaceCallables.धकेलो_back(callable);
        }
    }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    व्योम buildTriggers() {
        क्रम (स्थिर स्वतः& written : m_vifWrittenMembers) {
            स्थिर स्वतः it = m_candidateVscps.find(written);
            अगर (it == m_candidateVscps.end()) जारी;
            क्रम (AstVarScope* स्थिर vscp : it->second) {
                स्थिर AstIface* स्थिर ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

खुला:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        क्रम (माप_प्रकार i = 0; i < m_reachableIfaceCallables.आकार(); ++i) {
            स्थिर IfaceCallable callable = m_reachableIfaceCallables[i];
            callable.second->foreach([यह, callable](Ast_गाँठ_व्यंजक* स्थिर nodep) {
                अगर (AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) {
                    // Only persistent interface storage is observable through a VIF read.
                    UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
                    AstVar* स्थिर varp = refp->varp();
                    अगर (!refp->access().isWriteOrRW() || varp->isFuncLocal() || varp->is_अस्थायी()
                        || varp->isEvent() || !VN_IS(refp->varScopep()->scopep()->modp(), Iface)) {
                        वापस;
                    }
                    varp->sensIfacep(callable.first);
                    m_vifWrittenMembers.emplace(callable.first, varp->name());
                    स्थिर स्वतः it = m_vscpsByVar.find(varp);
                    UASSERT_OBJ(it != m_vscpsByVar.end(), varp,
                                "No VarScope for interface member");
                    क्रम (AstVarScope* स्थिर vscp : it->second)
                        addCandidateVscp(callable.first, vscp);
                } अन्यथा अगर (Ast_गाँठ_CCall* स्थिर callp = VN_CAST(nodep, गाँठ_CCall)) {
                    addReachableIfaceCallable(callable.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = शेष;

    // METHODS
    VirtIfaceTriggers take_triggers() { वापस std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    अगर (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dump_Check_Global_वृक्ष("sched_vif", 0, dump_वृक्ष_Either_Level() >= 6);
    }
    वापस triggers;
}

}  //namespace V3Sched
