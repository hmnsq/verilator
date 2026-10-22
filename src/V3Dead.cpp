// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dead code elimination
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
// DEAD TRANSFORMATIONS:
//      Remove any unreferenced modules
//      Remove any unreferenced variables
//
// TODO: A graph would make the process of circular and interlinked
// dependencies easier to resolve.
// NOTE: If redo this, consider using maybePointedTo()/broken() ish scheme
// instead of needing as many visitors.
//
// The following nodes have package pointers and are cleaned up here:
// AstRefDType, AstEnumItemRef, AstNodeVarRef, AstNodeFTask
// These have packagep but will not exist at this stage
// AstPackageImport, AstDot, AstClassOrPackageRef
//
// Note on packagep: After the V3Scope/V3LinkDotScoped stage, package links
// are no longer used, but their presence prevents us from removing empty
// packages. As the links as no longer used after V3Scope, we remove them
// here after scoping to allow more dead node removal.
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dead.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <queue>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

ವರ್ಗ Dead_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Dead_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    ತಾರ್ಕಿಕ m_removable = ಸುಳ್ಳು;  // Subject to dead removal
    ತಾರ್ಕಿಕ m_isVirtual = ಸುಳ್ಳು;  // Virtual node, indirection

ಸಾರ್ವಜನಿಕ:
    Dead_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Dead_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string dotShape() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_nodep; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvt_ಗಮ್ಯ_Hex(nodep()) + ' ' + nodep()->name();
    }
    ತಾರ್ಕಿಕ removable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_removable; }
    ಶೂನ್ಯ removable(ತಾರ್ಕಿಕ flag) { m_removable = flag; }
    ತಾರ್ಕಿಕ isVirtual() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isVirtual; }
    ಶೂನ್ಯ isVirtual(ತಾರ್ಕಿಕ flag) { m_isVirtual = flag; }
    uint64_t workPos() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_workPos; }
    ಶೂನ್ಯ workPos(uint64_t ಮೌಲ್ಯ) { m_workPos = ಮೌಲ್ಯ; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
ವರ್ಗ DeadWorkList final {
    // MEMBERS
    ಸ್ಥಿರ uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, Dead_ಶೃಂಗ*> m_ಕೆಲಸಮಾಡುತ್ತದೆ;  // Work list by sequence
ಸಾರ್ವಜನಿಕ:
    // METHODS
    ತಾರ್ಕಿಕ ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೆಲಸಮಾಡುತ್ತದೆ.ಖಾಲಿಗಳು(); }
    ಶೂನ್ಯ ತಳ್ಳು(Dead_ಶೃಂಗ* vtxp) {
        ಆದರೆ (vtxp->workPos()) ಹಿಂತಿರುಗಿ;  // Already on list
        ಸ್ಥಿರಾಂಕ uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_ಕೆಲಸಮಾಡುತ್ತದೆ.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    ಶೂನ್ಯ erase(Dead_ಶೃಂಗ* vtxp) {
        ಸ್ಥಿರಾಂಕ uint64_t id = vtxp->workPos();
        ಆದರೆ (!id) ಹಿಂತಿರುಗಿ;
        UINFO(9, "Worklist.erase " << vtxp);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಕೆಲಸಮಾಡುತ್ತದೆ.ಕಂಡುಹಿಡಿ(id);
        UASSERT_OBJ(it != m_ಕೆಲಸಮಾಡುತ್ತದೆ.ಕೊನೆ(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_ಕೆಲಸಮಾಡುತ್ತದೆ.erase(it);
        vtxp->workPos(0);
    }
    Dead_ಶೃಂಗ* get_ತೆಗೆ_Front() {
        UDEBUGONLY(UASSERT(!ಖಾಲಿಗಳು(), "Front invalid to call on empty list"););
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಕೆಲಸಮಾಡುತ್ತದೆ.begin();
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = it->ಎರಡನೇ;
        m_ಕೆಲಸಮಾಡುತ್ತದೆ.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        ಹಿಂತಿರುಗಿ vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

ವರ್ಗ Dead_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, Dead_ಶೃಂಗ*> m_virtualVtxsp;

    ಶೂನ್ಯ new_ಅಂಚು(Dead_ಶೃಂಗ* fromp, Dead_ಶೃಂಗ* ತುದಿ) {
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, fromp, ತುದಿ, 1, ಸುಳ್ಳು};
        ಆದರೆ (ತುದಿ->removable()) m_funcs.erase(ತುದಿ);  // Now has an input edge
    }
    ಶೂನ್ಯ ತಳ್ಳು_Work_Maybe(Dead_ಶೃಂಗ* vtxp, ತಾರ್ಕಿಕ allowSize1) {
        ಆದರೆ (vtxp->removable()) {
            ಆದರೆ (VN_IS(vtxp->nodep(), ಗಂಟು_FTask)
                && (vtxp->ಒಳ_ಖಾಲಿಗಳು() || (allowSize1 && vtxp->ಒಳ_Size1())))
                m_funcs.ತಳ್ಳು(vtxp);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    Dead_ನಕ್ಷೆ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~Dead_ನಕ್ಷೆ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    Dead_ಶೃಂಗ* ಕಂಡುಹಿಡಿ_New_ಶೃಂಗ(Ast_ಗಂಟು* nodep) {
        Dead_ಶೃಂಗ* vtxp = nodep->user2u().ಗಮ್ಯ<Dead_ಶೃಂಗ*>();
        ಆದರೆ (!vtxp) {
            vtxp = ಹೊಸ Dead_ಶೃಂಗ{ಇದು, nodep};
            nodep->user2p(vtxp);
            ತಳ್ಳು_Work_Maybe(vtxp, ಸುಳ್ಳು);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ಹಿಂತಿರುಗಿ vtxp;
    }
    Dead_ಶೃಂಗ* ಕಂಡುಹಿಡಿ_Virtual_ಶೃಂಗ(Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_virtualVtxsp.ಕಂಡುಹಿಡಿ(nodep->name());
        Dead_ಶೃಂಗ* vtxp;
        ಆದರೆ (it != m_virtualVtxsp.ಕೊನೆ()) {
            vtxp = it->ಎರಡನೇ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            vtxp = ಹೊಸ Dead_ಶೃಂಗ{ಇದು, nodep};
            vtxp->removable(ನಿಜ);
            vtxp->isVirtual(ನಿಜ);
            ತಳ್ಳು_Work_Maybe(vtxp, ಸುಳ್ಳು);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ಹಿಂತಿರುಗಿ vtxp;
    }
    ಶೂನ್ಯ ಕಂಡುಹಿಡಿ_New_Removable_ಶೃಂಗ(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ removable) {
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಕಂಡುಹಿಡಿ_New_ಶೃಂಗ(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ಆದರೆ (removable && !vtxp->removable()) {
            vtxp->removable(ನಿಜ);
            ತಳ್ಳು_Work_Maybe(vtxp, ಸುಳ್ಳು);
        }
    }
    ಶೂನ್ಯ delete_ಗಂಟು_ಶೃಂಗ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = nodep->user2u().ಗಮ್ಯ<Dead_ಶೃಂಗ*>()) delete_ಶೃಂಗ(vtxp);
    }
    ಶೂನ್ಯ delete_ಶೃಂಗ(Dead_ಶೃಂಗ* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& oedge : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Vtxp = static_cast<Dead_ಶೃಂಗ*>(oedge.ತುದಿ());
            ತಳ್ಳು_Work_Maybe(ಗಮ್ಯ_Vtxp, ನಿಜ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ಆದರೆ (!vtxp->isVirtual()) vtxp->nodep()->user2p(ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(ಇದು), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    ಶೂನ್ಯ needs(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* parentp) {
        ಆದರೆ (parentp == nodep) ಹಿಂತಿರುಗಿ;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Vtxp = ಕಂಡುಹಿಡಿ_New_ಶೃಂಗ(parentp);
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಂಟು_Vtxp = ಕಂಡುಹಿಡಿ_New_ಶೃಂಗ(nodep);
        UINFO(9, "Edge need " << ಪೋಷಕ_Vtxp << " -> " << ಗಂಟು_Vtxp);
        new_ಅಂಚು(ಪೋಷಕ_Vtxp, ಗಂಟು_Vtxp);
    }
    ಶೂನ್ಯ needsVirtual(Ast_ಗಂಟು_FTask* nodep, Ast_ಗಂಟು* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ಆದರೆ (parentp == nodep) ಹಿಂತಿರುಗಿ;  // No need for tracking needs itself (recursion)
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Vtxp = ಕಂಡುಹಿಡಿ_New_ಶೃಂಗ(parentp);
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ virtualVtxp = ಕಂಡುಹಿಡಿ_Virtual_ಶೃಂಗ(nodep);
        UINFO(9, "Edge needVirtual " << ಪೋಷಕ_Vtxp << " -> " << virtualVtxp);
        new_ಅಂಚು(ಪೋಷಕ_Vtxp, virtualVtxp);
    }
    ಶೂನ್ಯ funcVirtual(Ast_ಗಂಟು_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ virtualVtxp = ಕಂಡುಹಿಡಿ_Virtual_ಶೃಂಗ(nodep);
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಂಟು_Vtxp = ಕಂಡುಹಿಡಿ_New_ಶೃಂಗ(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << ಗಂಟು_Vtxp);
        new_ಅಂಚು(virtualVtxp, ಗಂಟು_Vtxp);
    }
    ತಾರ್ಕಿಕ funcs_ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcs.ಖಾಲಿಗಳು(); }
    Ast_ಗಂಟು* funcs_Get_ತೆಗೆ_Front() {
        Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = m_funcs.get_ತೆಗೆ_Front();
        UASSERT_OBJ(vtxp->ಒಳ_ಖಾಲಿಗಳು(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ಆದರೆ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            delete_ಶೃಂಗ(vtxp);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Caller will search again
        }
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = vtxp->nodep();
        ಹಿಂತಿರುಗಿ nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

ವರ್ಗ DeadVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;  // For usage information see DeadGraph

    // TYPES
    ಬಳಸಿ ನಿಯೋಜಿಸು_Map = std::multimap<AstVarScope*, Ast_ಗಂಟು_ನಿಯೋಜಿಸು*>;

    // STATE - across all visitors
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_elimUserVars;  // Allow removal of user's vars
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_elimDTypes;  // Allow removal of DTypes
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_elimCells;  // Allow removal of Cells

    Dead_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::ಸದಿಶ<AstVar*> m_varsp;
    std::ಸದಿಶ<Ast_ಗಂಟು*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_ಗಂಟು_DType*, Ast_ಗಂಟು_Module*> m_dtypePkgsp;  // Data type's containing package
    std::ಸದಿಶ<AstVarScope*> m_vscsp;
    std::ಸದಿಶ<AstScope*> m_scopesp;
    std::ಸದಿಶ<AstCell*> m_cellsp;
    std::ಸದಿಶ<AstClass*> m_classesp;
    std::ಸದಿಶ<AstTypedef*> m_typedefsp;
    ನಿಯೋಜಿಸು_Map m_ನಿಯೋಜಿಸು_Map;  // List of all simple assignments for each variable
    ತಾರ್ಕಿಕ m_sideEffect = ಸುಳ್ಳು;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    ತಾರ್ಕಿಕ m_ಒಳ_ನಿಯೋಜಿಸು = ಸುಳ್ಳು;  // Currently in an assign
    Ast_ಗಂಟು_DType* m_curDTypep = ಶೂನ್ಯಸೂಚಕ;  // Current NodeDType
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_Foreach_ಶೀರ್ಷಿಕೆ* m_foreachHeaderp = ಶೂನ್ಯಸೂಚಕ;  // Current foreach header
    Ast_ಗಂಟು* m_containingFTaskRefp = ಶೂನ್ಯಸೂಚಕ;  // Parent of ftaskref (e.g. task/module)
    ತಾರ್ಕಿಕ m_ಒಳ_FTask = ಸುಳ್ಳು;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    ಶೂನ್ಯ deleting(Ast_ಗಂಟು* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_ನಕ್ಷೆ.delete_ಗಂಟು_ಶೃಂಗ(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    ಶೂನ್ಯ checkAll(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subnodep = nodep->dtypep()) {
            ಆದರೆ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subnodep = nodep->get_ಮಗು_DTypep()) subnodep->user1Inc();
    }
    ಶೂನ್ಯ checkVarRef(Ast_ಗಂಟು_Var_Ref* nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ checkDType(Ast_ಗಂಟು_DType* nodep) {
        ಆದರೆ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        }
        ಆದರೆ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಆದರೆ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    ಶೂನ್ಯ needsTask(Ast_ಗಂಟು_FTask* taskp, Ast_ಗಂಟು* containerp) {
        ಆದರೆ (!taskp) ಹಿಂತಿರುಗಿ;  // Unlinked
        ಆದರೆ (taskp->isVirtual()) {
            m_ನಕ್ಷೆ.needsVirtual(taskp, containerp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ನಕ್ಷೆ.needs(taskp, containerp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ಆದರೆ (nodep->dead()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->mod_ಬಹಿರಂಗ()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) {
            ಆದರೆ (classp->extendsp()) classp->extendsp()->user1Inc();
            ಆದರೆ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ತಳ್ಳು_ಹಿಂಭಾಗ(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        ಗಾಗಿ (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ಆದರೆ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ಆದರೆ (m_modp->isConstPool()) nodep->user1Inc();
        ಆದರೆ (!nodep->is_ತುದಿ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        nodep->modp()->user1Inc();
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ಆದರೆ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ಆದರೆ (nodep->varp()) nodep->varp()->user1Inc();
        ಆದರೆ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!m_sideEffect && !nodep->is_ಶುದ್ಧ()) m_sideEffect = ನಿಜ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ಆದರೆ (nodep->classOrPackagep()) {
            ಆದರೆ (m_elimCells) {
                nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->varp()) nodep->varp()->user1Inc();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ಆದರೆ (nodep->classOrPackagep()) {
            ಆದರೆ (m_elimCells) {
                nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->classOrPackagep()) {
            ಆದರೆ (m_elimCells) {
                nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ಆದರೆ (nodep->classp()) nodep->classp()->user1Inc();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->modportp()) {
            ಆದರೆ (m_elimCells) {
                nodep->modportp(ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->modportp()->user1Inc();
            }
        }
        ಆದರೆ (nodep->iface_ಮೂಲಕ_Cellp()) nodep->iface_ಮೂಲಕ_Cellp()->user1Inc();
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->classOrPackagep()) {
            ಆದರೆ (m_elimCells) {
                nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->varp()) nodep->varp()->user1Inc();
        ಆದರೆ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModport* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_elimCells) {
            ಆದರೆ (!nodep->varsp()) {
                deleting(nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        checkAll(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Foreach_ಶೀರ್ಷಿಕೆ* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ಆದರೆ (nodep->attr_ಬಹಿರಂಗ() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->scopep()) nodep->scopep()->user1Inc();
        ಆದರೆ (mightElimVar(nodep->varp())) m_vscsp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ಆದರೆ (m_ಒಳ_FTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ಆದರೆ (mightElimVar(nodep)) {
            m_varsp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ನಿಯೋಜಿಸು_ಒಳ_ನಿಯೋಜಿಸು = m_ಒಳ_ನಿಯೋಜಿಸು;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_ಒಳ_ನಿಯೋಜಿಸು);
            VL_RESTORER(m_sideEffect);
            m_ಒಳ_ನಿಯೋಜಿಸು = ನಿಜ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ಆದರೆ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_ನಿಯೋಜಿಸು_Map.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ಇಲ್ಲದಿದ್ದರೆ {  // Track like any other statement
                iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ಆದರೆ (ನಿಯೋಜಿಸು_ಒಳ_ನಿಯೋಜಿಸು) m_sideEffect = ನಿಜ;  // Parent assign shouldn't optimize
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ removable = !(nodep->task_ಬಹಿರಂಗ() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->class_ವಿಧಾನ()));
        m_ನಕ್ಷೆ.ಕಂಡುಹಿಡಿ_New_Removable_ಶೃಂಗ(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_ಒಳ_FTask);
        m_containingFTaskRefp = nodep;
        m_ಒಳ_FTask = ನಿಜ;
        iterateChildren(nodep);
        checkAll(nodep);
        ಆದರೆ (nodep->isVirtual()) m_ನಕ್ಷೆ.funcVirtual(nodep);
        ಆದರೆ (!removable) {
            ಆದರೆ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ಆದರೆ (nodep->classOrPackagep()) {
            ಆದರೆ (m_elimCells) {
                nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Clocking_ಅಂಶ* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!m_sideEffect && !nodep->is_ಶುದ್ಧ()) m_sideEffect = ನಿಜ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    ಶೂನ್ಯ deadCheckTypedefs() {
        ಗಾಗಿ (AstTypedef* typedefp : m_typedefsp) {
            ಆದರೆ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                ಮುಂದುವರಿ;
            }
            checkAll(typedefp);
        }
    }
    ತಾರ್ಕಿಕ shouldDeleteTypedef(ಸ್ಥಿರಾಂಕ AstTypedef* typedefp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ structp = VN_CAST(typedefp->subDTypep(), ಗಂಟು_UOr_Struct_DType)) {
            ಆದರೆ (structp->user1() && !structp->packed()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ m_elimCells && !typedefp->attr_ಬಹಿರಂಗ();
    }

    ಶೂನ್ಯ deadCheckTasks() {
        ಆಗ (!m_ನಕ್ಷೆ.funcs_ಖಾಲಿಗಳು()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = m_ನಕ್ಷೆ.funcs_Get_ತೆಗೆ_Front();
            ಆದರೆ (!nodep) ಮುಂದುವರಿ;
            UINFO(9, "Dead " << nodep);
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(nodep, ಗಂಟು_FTask)) {
                ಆದರೆ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (taskp->class_ವಿಧಾನ()) {
                    ++m_statFTasksMDeadified;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    ಶೂನ್ಯ deadCheckDemote() {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& gvtx : m_ನಕ್ಷೆ.vertices()) {
            Dead_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = gvtx.cast<Dead_ಶೃಂಗ>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ಆದರೆ (!vtxp->ಹೊರಹೋಗು_Size1()) ಮುಂದುವರಿ;
            ಆದರೆ (!vtxp->isVirtual()) ಮುಂದುವರಿ;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {  // Always a single one
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = ಅಂಚು.ತುದಿ()->as<Dead_ಶೃಂಗ>()->nodep();
                Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ funcp = VN_AS(nodep, ಗಂಟು_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ಆದರೆ (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(ಸುಳ್ಳು);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    ಶೂನ್ಯ deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        ಗಾಗಿ (ತಾರ್ಕಿಕ retry = ನಿಜ; retry;) {
            retry = ಸುಳ್ಳು;
            Ast_ಗಂಟು_Module* nextmodp;
            ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), ಗಂಟು_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ಆದರೆ (modp->dead() || (!modp->is_ತುದಿ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ಆದರೆ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ಸ್ಥಿರಾಂಕ AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ಆದರೆ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(ಶೂನ್ಯಸೂಚಕ);
                    deleting(modp);
                    retry = ನಿಜ;
                }
            }
        }
    }
    ತಾರ್ಕಿಕ mightElimVar(ಸ್ಥಿರಾಂಕ AstVar* nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (nodep->is_Sig_ಬಹಿರಂಗ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Can't elim publics!
        ಆದರೆ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (nodep->is_ತಾತ್ಕಾಲಿಕ() && !nodep->isTrace()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ m_elimUserVars;  // Post-Trace can kill most anything
    }

    ಶೂನ್ಯ deadCheckScope() {
        ಗಾಗಿ (ತಾರ್ಕಿಕ retry = ನಿಜ; retry;) {
            retry = ಸುಳ್ಳು;
            ಗಾಗಿ (std::ಸದಿಶ<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.ಕೊನೆ();
                 ++it) {
                AstScope* ಸ್ಥಿರಾಂಕ scp = *it;
                ಆದರೆ (!scp) ಮುಂದುವರಿ;
                ಆದರೆ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ಆದರೆ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = ಶೂನ್ಯಸೂಚಕ;
                    retry = ನಿಜ;
                }
            }
        }
    }

    ಶೂನ್ಯ deadCheckCells() {
        ಗಾಗಿ (AstCell* cellp : m_cellsp) {
            ಆದರೆ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    ಶೂನ್ಯ deadCheckClasses() {
        ಗಾಗಿ (ತಾರ್ಕಿಕ retry = ನಿಜ; retry;) {
            retry = ಸುಳ್ಳು;
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : m_classesp) {
                ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ nodep = itr) {  // nullptr if deleted earlier
                    ಆದರೆ (nodep->user1() == 0) {
                        ಆದರೆ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ಆದರೆ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = ಶೂನ್ಯಸೂಚಕ;
                        retry = ನಿಜ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ deadCheckVar() {
        // Delete any unused varscopes
        ಗಾಗಿ (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ಆದರೆ (vscp->varp()->user2()) ಮುಂದುವರಿ;
            ಆದರೆ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ಸ್ಥಿರಾಂಕ std::pair<ನಿಯೋಜಿಸು_Map::iterator, ನಿಯೋಜಿಸು_Map::iterator> eqrange
                    = m_ನಿಯೋಜಿಸು_Map.equal_range(vscp);
                ಗಾಗಿ (ನಿಯೋಜಿಸು_Map::iterator itr = eqrange.ಮೊದಲ; itr != eqrange.ಎರಡನೇ; ++itr) {
                    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = itr->ಎರಡನೇ;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ಆದರೆ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        ಗಾಗಿ (ತಾರ್ಕಿಕ retry = ನಿಜ; retry;) {
            retry = ಸುಳ್ಳು;
            ಗಾಗಿ (std::ಸದಿಶ<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.ಕೊನೆ(); ++it) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = *it;
                ಆದರೆ (!varp) ಮುಂದುವರಿ;
                ಆದರೆ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ಆದರೆ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = ಶೂನ್ಯಸೂಚಕ;
                    retry = ನಿಜ;
                }
            }
        }
        ಗಾಗಿ (std::ಸದಿಶ<Ast_ಗಂಟು*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.ಕೊನೆ();
             ++it) {
            ಆದರೆ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ classp
                    = VN_CAST((*it), ಗಂಟು_UOr_Struct_DType)) {
                    ತಾರ್ಕಿಕ cont = ನಿಜ;
                    ಗಾಗಿ (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ಆದರೆ (memberp->user1() != 0) {
                            cont = ಸುಳ್ಳು;
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                    ಆದರೆ (!cont) ಮುಂದುವರಿ;
                }
                deleting(*it);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, ತಾರ್ಕಿಕ elimUserVars, ತಾರ್ಕಿಕ elimDTypes, ತಾರ್ಕಿಕ elimScopes,
                ತಾರ್ಕಿಕ elimCells, ತಾರ್ಕಿಕ elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : m_dtypePkgsp) {
            ಆದರೆ (itr.ಮೊದಲ->user1()) itr.ಎರಡನೇ->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_ನಕ್ಷೆ.remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9 || debug() >= 9)
            m_ನಕ್ಷೆ.dumpDotFilePrefixed("dead_graph", ಸುಳ್ಳು);

        ಆದರೆ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ಆದರೆ (elimScopes) deadCheckScope();
        ಆದರೆ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ಆದರೆ (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_ವೃಕ್ಷ();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

ಶೂನ್ಯ V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, ಸುಳ್ಳು, ಸುಳ್ಳು, ಸುಳ್ಳು, ಸುಳ್ಳು, ಸುಳ್ಳು};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadModules", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}

ಶೂನ್ಯ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ಸುಳ್ಳು, ನಿಜ, ಸುಳ್ಳು, ಸುಳ್ಳು, ನಿಜ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadDtypes", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ಸುಳ್ಳು, ನಿಜ, ನಿಜ, ಸುಳ್ಳು, ಸುಳ್ಳು}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadDtypesScoped", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ನಿಜ, ನಿಜ, ಸುಳ್ಳು, ನಿಜ, ನಿಜ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadAll", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ನಿಜ, ನಿಜ, ನಿಜ, ನಿಜ, ನಿಜ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadAllScoped", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
