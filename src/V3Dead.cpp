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

ವರ್ಗ DeadVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_ಗಂಟು* ಧ್ರುವ m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    ತಾರ್ಕಿಕ m_removable = ಅಸತ್ಯ;  // Subject to dead removal
    ತಾರ್ಕಿಕ m_isVirtual = ಅಸತ್ಯ;  // Virtual node, indirection

ಸಾರ್ವಜನಿಕ:
    DeadVertex(V3Graph* graphp, Ast_ಗಂಟು* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = ಸ್ವಭಾವ;
    string dotShape() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_ಗಂಟು* nodep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    string name() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    ತಾರ್ಕಿಕ removable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_removable; }
    ಶೂನ್ಯ removable(ತಾರ್ಕಿಕ flag) { m_removable = flag; }
    ತಾರ್ಕಿಕ isVirtual() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isVirtual; }
    ಶೂನ್ಯ isVirtual(ತಾರ್ಕಿಕ flag) { m_isVirtual = flag; }
    uint64_t workPos() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_workPos; }
    ಶೂನ್ಯ workPos(uint64_t ಮೌಲ್ಯ) { m_workPos = ಮೌಲ್ಯ; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
ವರ್ಗ DeadWorkList final {
    // MEMBERS
    ಸ್ಥಿರ uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
ಸಾರ್ವಜನಿಕ:
    // METHODS
    ತಾರ್ಕಿಕ empty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_works.empty(); }
    ಶೂನ್ಯ ತಳ್ಳು(DeadVertex* vtxp) {
        ಯದಿ (vtxp->workPos()) ಪ್ರತ್ಯಾಗಮ;  // Already on list
        ಧ್ರುವ uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    ಶೂನ್ಯ erase(DeadVertex* vtxp) {
        ಧ್ರುವ uint64_t id = vtxp->workPos();
        ಯದಿ (!id) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "Worklist.erase " << vtxp);
        ಧ್ರುವ auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_ತೆಗೆ_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        ಧ್ರುವ auto it = m_works.begin();
        DeadVertex* ಧ್ರುವ vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

ವರ್ಗ DeadGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    ಶೂನ್ಯ newEdge(DeadVertex* fromp, DeadVertex* ತುದಿ) {
        ನೂತನ V3GraphEdge{ಏತತ್, fromp, ತುದಿ, 1, ಅಸತ್ಯ};
        ಯದಿ (ತುದಿ->removable()) m_funcs.erase(ತುದಿ);  // Now has an input edge
    }
    ಶೂನ್ಯ ತಳ್ಳು_Work_Maybe(DeadVertex* vtxp, ತಾರ್ಕಿಕ allowSize1) {
        ಯದಿ (vtxp->removable()) {
            ಯದಿ (VN_IS(vtxp->nodep(), ಗಂಟು_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.ತಳ್ಳು(vtxp);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    DeadGraph() = ಸ್ವಭಾವ;
    ~DeadGraph() override = ಸ್ವಭಾವ;
    DeadVertex* findNewVertex(Ast_ಗಂಟು* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        ಯದಿ (!vtxp) {
            vtxp = ನೂತನ DeadVertex{ಏತತ್, nodep};
            nodep->user2p(vtxp);
            ತಳ್ಳು_Work_Maybe(vtxp, ಅಸತ್ಯ);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        ಯದಿ (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } ಅನ್ಯಥಾ {
            vtxp = ನೂತನ DeadVertex{ಏತತ್, nodep};
            vtxp->removable(ಸತ್ಯ);
            vtxp->isVirtual(ಸತ್ಯ);
            ತಳ್ಳು_Work_Maybe(vtxp, ಅಸತ್ಯ);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }
    ಶೂನ್ಯ findNewRemovableVertex(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ removable) {
        DeadVertex* ಧ್ರುವ vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ಯದಿ (removable && !vtxp->removable()) {
            vtxp->removable(ಸತ್ಯ);
            ತಳ್ಳು_Work_Maybe(vtxp, ಅಸತ್ಯ);
        }
    }
    ಶೂನ್ಯ delete_ಗಂಟು_Vertex(Ast_ಗಂಟು* nodep) {
        ಯದಿ (DeadVertex* ಧ್ರುವ vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    ಶೂನ್ಯ deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* ಧ್ರುವ toVtxp = static_cast<DeadVertex*>(oedge.ತುದಿ());
            ತಳ್ಳು_Work_Maybe(toVtxp, ಸತ್ಯ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ಯದಿ (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        ಯದಿ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(ಏತತ್), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    ಶೂನ್ಯ needs(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* parentp) {
        ಯದಿ (parentp == nodep) ಪ್ರತ್ಯಾಗಮ;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* ಧ್ರುವ parentVtxp = findNewVertex(parentp);
        DeadVertex* ಧ್ರುವ ಗಂಟು_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << ಗಂಟು_Vtxp);
        newEdge(parentVtxp, ಗಂಟು_Vtxp);
    }
    ಶೂನ್ಯ needsVirtual(Ast_ಗಂಟು_FTask* nodep, Ast_ಗಂಟು* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ಯದಿ (parentp == nodep) ಪ್ರತ್ಯಾಗಮ;  // No need for tracking needs itself (recursion)
        DeadVertex* ಧ್ರುವ parentVtxp = findNewVertex(parentp);
        DeadVertex* ಧ್ರುವ virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    ಶೂನ್ಯ funcVirtual(Ast_ಗಂಟು_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* ಧ್ರುವ virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* ಧ್ರುವ ಗಂಟು_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << ಗಂಟು_Vtxp);
        newEdge(virtualVtxp, ಗಂಟು_Vtxp);
    }
    ತಾರ್ಕಿಕ funcsEmpty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcs.empty(); }
    Ast_ಗಂಟು* funcs_Get_ತೆಗೆ_Front() {
        DeadVertex* ಧ್ರುವ vtxp = m_funcs.get_ತೆಗೆ_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ಯದಿ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            ಪ್ರತ್ಯಾಗಮ nullptr;  // Caller will search again
        }
        Ast_ಗಂಟು* ಧ್ರುವ nodep = vtxp->nodep();
        ಪ್ರತ್ಯಾಗಮ nodep;
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
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_ಗಂಟು_Assign*>;

    // STATE - across all visitors
    ಧ್ರುವ ತಾರ್ಕಿಕ m_elimUserVars;  // Allow removal of user's vars
    ಧ್ರುವ ತಾರ್ಕಿಕ m_elimDTypes;  // Allow removal of DTypes
    ಧ್ರುವ ತಾರ್ಕಿಕ m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_ಗಂಟು*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_ಗಂಟು_DType*, Ast_ಗಂಟು_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    ತಾರ್ಕಿಕ m_sideEffect = ಅಸತ್ಯ;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    ತಾರ್ಕಿಕ m_inAssign = ಅಸತ್ಯ;  // Currently in an assign
    Ast_ಗಂಟು_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_ಗಂಟು* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    ತಾರ್ಕಿಕ m_inFTask = ಅಸತ್ಯ;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    ಶೂನ್ಯ deleting(Ast_ಗಂಟು* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_ಗಂಟು_Vertex(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    ಶೂನ್ಯ checkAll(Ast_ಗಂಟು* nodep) {
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ subnodep = nodep->dtypep()) {
            ಯದಿ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    ಶೂನ್ಯ checkVarRef(Ast_ಗಂಟು_Var_Ref* nodep) ಧ್ರುವ {
        ಯದಿ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    ಶೂನ್ಯ checkDType(Ast_ಗಂಟು_DType* nodep) {
        ಯದಿ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ತಳ್ಳು_back(nodep);
        }
        ಯದಿ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಯದಿ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    ಶೂನ್ಯ needsTask(Ast_ಗಂಟು_FTask* taskp, Ast_ಗಂಟು* containerp) {
        ಯದಿ (!taskp) ಪ್ರತ್ಯಾಗಮ;  // Unlinked
        ಯದಿ (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } ಅನ್ಯಥಾ {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ಯದಿ (nodep->dead()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) {
            ಯದಿ (classp->extendsp()) classp->extendsp()->user1Inc();
            ಯದಿ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ತಳ್ಳು_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        ಪ್ರತಿ (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    ಶೂನ್ಯ visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ಯದಿ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ಯದಿ (m_modp->isConstPool()) nodep->user1Inc();
        ಯದಿ (!nodep->is_ತುದಿ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ತಳ್ಳು_back(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ತಳ್ಳು_back(nodep);
        nodep->modp()->user1Inc();
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ಯದಿ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ಯದಿ (nodep->varp()) nodep->varp()->user1Inc();
        ಯದಿ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!m_sideEffect && !nodep->isPure()) m_sideEffect = ಸತ್ಯ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ಯದಿ (nodep->classOrPackagep()) {
            ಯದಿ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ಅನ್ಯಥಾ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ಶೂನ್ಯ visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    ಶೂನ್ಯ visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->varp()) nodep->varp()->user1Inc();
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ಯದಿ (nodep->classOrPackagep()) {
            ಯದಿ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ಅನ್ಯಥಾ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->classOrPackagep()) {
            ಯದಿ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ಅನ್ಯಥಾ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ಯದಿ (nodep->classp()) nodep->classp()->user1Inc();
    }
    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->modportp()) {
            ಯದಿ (m_elimCells) {
                nodep->modportp(nullptr);
            } ಅನ್ಯಥಾ {
                nodep->modportp()->user1Inc();
            }
        }
        ಯದಿ (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->classOrPackagep()) {
            ಯದಿ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ಅನ್ಯಥಾ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->varp()) nodep->varp()->user1Inc();
        ಯದಿ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    ಶೂನ್ಯ visit(AstModport* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_elimCells) {
            ಯದಿ (!nodep->varsp()) {
                deleting(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        checkAll(nodep);
    }
    ಶೂನ್ಯ visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ತಳ್ಳು_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ಯದಿ (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->scopep()) nodep->scopep()->user1Inc();
        ಯದಿ (mightElimVar(nodep->varp())) m_vscsp.ತಳ್ಳು_back(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ಯದಿ (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ಯದಿ (mightElimVar(nodep)) {
            m_varsp.ತಳ್ಳು_back(nodep);
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ಧ್ರುವ ತಾರ್ಕಿಕ assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = ಸತ್ಯ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ಧ್ರುವ varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ಯದಿ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ಅನ್ಯಥಾ {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ಯದಿ (assignInAssign) m_sideEffect = ಸತ್ಯ;  // Parent assign shouldn't optimize
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = ಸತ್ಯ;
        iterateChildren(nodep);
        checkAll(nodep);
        ಯದಿ (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        ಯದಿ (!removable) {
            ಯದಿ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ಯದಿ (nodep->classOrPackagep()) {
            ಯದಿ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ಅನ್ಯಥಾ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    ಶೂನ್ಯ visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!m_sideEffect && !nodep->isPure()) m_sideEffect = ಸತ್ಯ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    ಶೂನ್ಯ deadCheckTypedefs() {
        ಪ್ರತಿ (AstTypedef* typedefp : m_typedefsp) {
            ಯದಿ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                ಅನುವರ್ತ;
            }
            checkAll(typedefp);
        }
    }
    ತಾರ್ಕಿಕ shouldDeleteTypedef(ಧ್ರುವ AstTypedef* typedefp) ಧ್ರುವ {
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ structp = VN_CAST(typedefp->subDTypep(), ಗಂಟು_UOr_Struct_DType)) {
            ಯದಿ (structp->user1() && !structp->packed()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ m_elimCells && !typedefp->attrPublic();
    }

    ಶೂನ್ಯ deadCheckTasks() {
        ಯಾವತ್ (!m_graph.funcsEmpty()) {
            Ast_ಗಂಟು* ಧ್ರುವ nodep = m_graph.funcs_Get_ತೆಗೆ_Front();
            ಯದಿ (!nodep) ಅನುವರ್ತ;
            UINFO(9, "Dead " << nodep);
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ taskp = VN_CAST(nodep, ಗಂಟು_FTask)) {
                ಯದಿ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ಅನ್ಯಥಾ ಯದಿ (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } ಅನ್ಯಥಾ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    ಶೂನ್ಯ deadCheckDemote() {
        ಪ್ರತಿ (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* ಧ್ರುವ vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ಯದಿ (!vtxp->outSize1()) ಅನುವರ್ತ;
            ಯದಿ (!vtxp->isVirtual()) ಅನುವರ್ತ;
            ಪ್ರತಿ (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_ಗಂಟು* ಧ್ರುವ nodep = edge.ತುದಿ()->as<DeadVertex>()->nodep();
                Ast_ಗಂಟು_FTask* ಧ್ರುವ funcp = VN_AS(nodep, ಗಂಟು_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ಯದಿ (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(ಅಸತ್ಯ);
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
        ಪ್ರತಿ (ತಾರ್ಕಿಕ retry = ಸತ್ಯ; retry;) {
            retry = ಅಸತ್ಯ;
            Ast_ಗಂಟು_Module* nextmodp;
            ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), ಗಂಟು_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ಧ್ರುವ ತಾರ್ಕಿಕ keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ಯದಿ (modp->dead() || (!modp->is_ತುದಿ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ಯದಿ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ಧ್ರುವ AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ಯದಿ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = ಸತ್ಯ;
                }
            }
        }
    }
    ತಾರ್ಕಿಕ mightElimVar(ಧ್ರುವ AstVar* nodep) ಧ್ರುವ {
        ಯದಿ (nodep->isSigPublic()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Can't elim publics!
        ಯದಿ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->isTemp() && !nodep->isTrace()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ m_elimUserVars;  // Post-Trace can kill most anything
    }

    ಶೂನ್ಯ deadCheckScope() {
        ಪ್ರತಿ (ತಾರ್ಕಿಕ retry = ಸತ್ಯ; retry;) {
            retry = ಅಸತ್ಯ;
            ಪ್ರತಿ (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* ಧ್ರುವ scp = *it;
                ಯದಿ (!scp) ಅನುವರ್ತ;
                ಯದಿ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ಯದಿ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = ಸತ್ಯ;
                }
            }
        }
    }

    ಶೂನ್ಯ deadCheckCells() {
        ಪ್ರತಿ (AstCell* cellp : m_cellsp) {
            ಯದಿ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    ಶೂನ್ಯ deadCheckClasses() {
        ಪ್ರತಿ (ತಾರ್ಕಿಕ retry = ಸತ್ಯ; retry;) {
            retry = ಅಸತ್ಯ;
            ಪ್ರತಿ (auto& itr : m_classesp) {
                ಯದಿ (AstClass* ಧ್ರುವ nodep = itr) {  // nullptr if deleted earlier
                    ಯದಿ (nodep->user1() == 0) {
                        ಯದಿ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ಯದಿ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = ಸತ್ಯ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ deadCheckVar() {
        // Delete any unused varscopes
        ಪ್ರತಿ (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ಯದಿ (vscp->varp()->user2()) ಅನುವರ್ತ;
            ಯದಿ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ಧ್ರುವ std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                ಪ್ರತಿ (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ಯದಿ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        ಪ್ರತಿ (ತಾರ್ಕಿಕ retry = ಸತ್ಯ; retry;) {
            retry = ಅಸತ್ಯ;
            ಪ್ರತಿ (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* ಧ್ರುವ varp = *it;
                ಯದಿ (!varp) ಅನುವರ್ತ;
                ಯದಿ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ಯದಿ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = ಸತ್ಯ;
                }
            }
        }
        ಪ್ರತಿ (std::vector<Ast_ಗಂಟು*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            ಯದಿ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ classp
                    = VN_CAST((*it), ಗಂಟು_UOr_Struct_DType)) {
                    ತಾರ್ಕಿಕ cont = ಸತ್ಯ;
                    ಪ್ರತಿ (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ಯದಿ (memberp->user1() != 0) {
                            cont = ಅಸತ್ಯ;
                            ಭಙ್ಗ;
                        }
                    }
                    ಯದಿ (!cont) ಅನುವರ್ತ;
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

        ಯದಿ (AstVarScope* ಧ್ರುವ vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        ಪ್ರತಿ (auto& itr : m_dtypePkgsp) {
            ಯದಿ (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ಯದಿ (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", ಅಸತ್ಯ);

        ಯದಿ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ಯದಿ (elimScopes) deadCheckScope();
        ಯದಿ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ಯದಿ (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_ವೃಕ್ಷ();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::addStatSum("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::addStatSum("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::addStatSum("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::addStatSum("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

ಶೂನ್ಯ V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadModules", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}

ಶೂನ್ಯ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ಅಸತ್ಯ, ಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadDtypes", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ಅಸತ್ಯ, ಸತ್ಯ, ಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadDtypesScoped", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ಸತ್ಯ, ಸತ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ, ಸತ್ಯ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadAll", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ಸತ್ಯ, ಸತ್ಯ, ಸತ್ಯ, ಸತ್ಯ, ಸತ್ಯ}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deadAllScoped", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
