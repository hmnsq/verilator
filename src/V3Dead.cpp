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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dead.h"

#অন্তর্ভাব "V3ConstPool.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <queue>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

শ্রেণী DeadVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_গ্রন্থি* ধ্রুবক m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    যৌক্তিক m_removable = মিথ্যা;  // Subject to dead removal
    যৌক্তিক m_isVirtual = মিথ্যা;  // Virtual node, indirection

সর্বজনীন:
    DeadVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = পূর্বনির্ধারিত;
    string dotShape() ধ্রুবক override {
        ফেরত isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_গ্রন্থি* nodep() ধ্রুবক VL_MT_STABLE { ফেরত m_nodep; }
    string name() ধ্রুবক override VL_MT_STABLE {
        ফেরত (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    যৌক্তিক removable() ধ্রুবক { ফেরত m_removable; }
    শূন্য removable(যৌক্তিক flag) { m_removable = flag; }
    যৌক্তিক isVirtual() ধ্রুবক { ফেরত m_isVirtual; }
    শূন্য isVirtual(যৌক্তিক flag) { m_isVirtual = flag; }
    uint64_t workPos() ধ্রুবক { ফেরত m_workPos; }
    শূন্য workPos(uint64_t মান) { m_workPos = মান; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
শ্রেণী DeadWorkList final {
    // MEMBERS
    স্থির uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
সর্বজনীন:
    // METHODS
    যৌক্তিক empty() ধ্রুবক { ফেরত m_works.empty(); }
    শূন্য ঠেলো(DeadVertex* vtxp) {
        যদি (vtxp->workPos()) ফেরত;  // Already on list
        ধ্রুবক uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    শূন্য erase(DeadVertex* vtxp) {
        ধ্রুবক uint64_t id = vtxp->workPos();
        যদি (!id) ফেরত;
        UINFO(9, "Worklist.erase " << vtxp);
        ধ্রুবক auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_বের_করো_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        ধ্রুবক auto it = m_works.begin();
        DeadVertex* ধ্রুবক vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        ফেরত vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

শ্রেণী DeadGraph final : সর্বজনীন V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    শূন্য newEdge(DeadVertex* fromp, DeadVertex* শীর্ষ) {
        নতুন V3GraphEdge{এই, fromp, শীর্ষ, 1, মিথ্যা};
        যদি (শীর্ষ->removable()) m_funcs.erase(শীর্ষ);  // Now has an input edge
    }
    শূন্য ঠেলো_Work_Maybe(DeadVertex* vtxp, যৌক্তিক allowSize1) {
        যদি (vtxp->removable()) {
            যদি (VN_IS(vtxp->nodep(), গ্রন্থি_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.ঠেলো(vtxp);
        }
    }

সর্বজনীন:
    // METHODS
    DeadGraph() = পূর্বনির্ধারিত;
    ~DeadGraph() override = পূর্বনির্ধারিত;
    DeadVertex* findNewVertex(Ast_গ্রন্থি* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        যদি (!vtxp) {
            vtxp = নতুন DeadVertex{এই, nodep};
            nodep->user2p(vtxp);
            ঠেলো_Work_Maybe(vtxp, মিথ্যা);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ফেরত vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_গ্রন্থি* nodep) {
        ধ্রুবক auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        যদি (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } অন্যথা {
            vtxp = নতুন DeadVertex{এই, nodep};
            vtxp->removable(সত্য);
            vtxp->isVirtual(সত্য);
            ঠেলো_Work_Maybe(vtxp, মিথ্যা);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ফেরত vtxp;
    }
    শূন্য findNewRemovableVertex(Ast_গ্রন্থি* nodep, যৌক্তিক removable) {
        DeadVertex* ধ্রুবক vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        যদি (removable && !vtxp->removable()) {
            vtxp->removable(সত্য);
            ঠেলো_Work_Maybe(vtxp, মিথ্যা);
        }
    }
    শূন্য delete_গ্রন্থি_Vertex(Ast_গ্রন্থি* nodep) {
        যদি (DeadVertex* ধ্রুবক vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    শূন্য deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        জন্য (ধ্রুবক V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* ধ্রুবক toVtxp = static_cast<DeadVertex*>(oedge.শীর্ষ());
            ঠেলো_Work_Maybe(toVtxp, সত্য);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        যদি (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        যদি (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(এই), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    শূন্য needs(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* parentp) {
        যদি (parentp == nodep) ফেরত;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* ধ্রুবক parentVtxp = findNewVertex(parentp);
        DeadVertex* ধ্রুবক গ্রন্থি_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << গ্রন্থি_Vtxp);
        newEdge(parentVtxp, গ্রন্থি_Vtxp);
    }
    শূন্য needsVirtual(Ast_গ্রন্থি_FTask* nodep, Ast_গ্রন্থি* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        যদি (parentp == nodep) ফেরত;  // No need for tracking needs itself (recursion)
        DeadVertex* ধ্রুবক parentVtxp = findNewVertex(parentp);
        DeadVertex* ধ্রুবক virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    শূন্য funcVirtual(Ast_গ্রন্থি_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* ধ্রুবক virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* ধ্রুবক গ্রন্থি_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << গ্রন্থি_Vtxp);
        newEdge(virtualVtxp, গ্রন্থি_Vtxp);
    }
    যৌক্তিক funcsEmpty() ধ্রুবক { ফেরত m_funcs.empty(); }
    Ast_গ্রন্থি* funcs_Get_বের_করো_Front() {
        DeadVertex* ধ্রুবক vtxp = m_funcs.get_বের_করো_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        যদি (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            ফেরত nullptr;  // Caller will search again
        }
        Ast_গ্রন্থি* ধ্রুবক nodep = vtxp->nodep();
        ফেরত nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

শ্রেণী DeadVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ধ্রুবক VNUser1InUse m_inuser1;
    ধ্রুবক VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_গ্রন্থি_Assign*>;

    // STATE - across all visitors
    ধ্রুবক যৌক্তিক m_elimUserVars;  // Allow removal of user's vars
    ধ্রুবক যৌক্তিক m_elimDTypes;  // Allow removal of DTypes
    ধ্রুবক যৌক্তিক m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_গ্রন্থি*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_গ্রন্থি_DType*, Ast_গ্রন্থি_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    যৌক্তিক m_sideEffect = মিথ্যা;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    যৌক্তিক m_inAssign = মিথ্যা;  // Currently in an assign
    Ast_গ্রন্থি_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_গ্রন্থি* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    যৌক্তিক m_inFTask = মিথ্যা;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    শূন্য deleting(Ast_গ্রন্থি* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_গ্রন্থি_Vertex(nodep);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    শূন্য checkAll(Ast_গ্রন্থি* nodep) {
        যদি (Ast_গ্রন্থি* ধ্রুবক subnodep = nodep->dtypep()) {
            যদি (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        যদি (Ast_গ্রন্থি* ধ্রুবক subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    শূন্য checkVarRef(Ast_গ্রন্থি_Var_Ref* nodep) ধ্রুবক {
        যদি (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    শূন্য checkDType(Ast_গ্রন্থি_DType* nodep) {
        যদি (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ঠেলো_back(nodep);
        }
        যদি (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক sdtypep = VN_CAST(nodep, গ্রন্থি_UOr_Struct_DType)) {
            যদি (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        যদি (Ast_গ্রন্থি* ধ্রুবক subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        যদি (Ast_গ্রন্থি* ধ্রুবক subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    শূন্য needsTask(Ast_গ্রন্থি_FTask* taskp, Ast_গ্রন্থি* containerp) {
        যদি (!taskp) ফেরত;  // Unlinked
        যদি (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } অন্যথা {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        যদি (nodep->dead()) ফেরত;
        যদি (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (AstClass* ধ্রুবক classp = VN_CAST(nodep, Class)) {
            যদি (classp->extendsp()) classp->extendsp()->user1Inc();
            যদি (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ঠেলো_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    শূন্য visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        জন্য (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    শূন্য visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    শূন্য visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        যদি (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        যদি (m_modp->isConstPool()) nodep->user1Inc();
        যদি (!nodep->is_শীর্ষ() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ঠেলো_back(nodep);
        }
    }
    শূন্য visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ঠেলো_back(nodep);
        nodep->modp()->user1Inc();
    }

    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        যদি (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        যদি (nodep->varp()) nodep->varp()->user1Inc();
        যদি (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        যদি (!m_sideEffect && !nodep->isPure()) m_sideEffect = সত্য;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        যদি (nodep->classOrPackagep()) {
            যদি (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } অন্যথা {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    শূন্য visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    শূন্য visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->varp()) nodep->varp()->user1Inc();
    }
    শূন্য visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        যদি (nodep->classOrPackagep()) {
            যদি (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } অন্যথা {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        যদি (nodep->classOrPackagep()) {
            যদি (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } অন্যথা {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        যদি (nodep->classp()) nodep->classp()->user1Inc();
    }
    শূন্য visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        যদি (nodep->modportp()) {
            যদি (m_elimCells) {
                nodep->modportp(nullptr);
            } অন্যথা {
                nodep->modportp()->user1Inc();
            }
        }
        যদি (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    শূন্য visit(Ast_গ্রন্থি_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->classOrPackagep()) {
            যদি (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } অন্যথা {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    শূন্য visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->varp()) nodep->varp()->user1Inc();
        যদি (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    শূন্য visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    শূন্য visit(AstModport* nodep) override {
        iterateChildren(nodep);
        যদি (m_elimCells) {
            যদি (!nodep->varsp()) {
                deleting(nodep);
                ফেরত;
            }
        }
        checkAll(nodep);
    }
    শূন্য visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    শূন্য visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ঠেলো_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        যদি (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    শূন্য visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->scopep()) nodep->scopep()->user1Inc();
        যদি (mightElimVar(nodep->varp())) m_vscsp.ঠেলো_back(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        যদি (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        যদি (mightElimVar(nodep)) {
            m_varsp.ঠেলো_back(nodep);
        } অন্যথা {
            যদি (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ধ্রুবক যৌক্তিক assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = সত্য;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_পরবর্তী_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ধ্রুবক varrefp = VN_CAST(nodep->lhsp(), VarRef);
            যদি (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } অন্যথা {  // Track like any other statement
                iterate_And_পরবর্তী_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        যদি (assignInAssign) m_sideEffect = সত্য;  // Parent assign shouldn't optimize
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        ধ্রুবক যৌক্তিক removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = সত্য;
        iterateChildren(nodep);
        checkAll(nodep);
        যদি (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        যদি (!removable) {
            যদি (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        যদি (nodep->classOrPackagep()) {
            যদি (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } অন্যথা {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    শূন্য visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!m_sideEffect && !nodep->isPure()) m_sideEffect = সত্য;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    শূন্য deadCheckTypedefs() {
        জন্য (AstTypedef* typedefp : m_typedefsp) {
            যদি (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                চালু;
            }
            checkAll(typedefp);
        }
    }
    যৌক্তিক shouldDeleteTypedef(ধ্রুবক AstTypedef* typedefp) ধ্রুবক {
        যদি (ধ্রুবক auto* ধ্রুবক structp = VN_CAST(typedefp->subDTypep(), গ্রন্থি_UOr_Struct_DType)) {
            যদি (structp->user1() && !structp->packed()) ফেরত মিথ্যা;
        }
        ফেরত m_elimCells && !typedefp->attrPublic();
    }

    শূন্য deadCheckTasks() {
        যখন (!m_graph.funcsEmpty()) {
            Ast_গ্রন্থি* ধ্রুবক nodep = m_graph.funcs_Get_বের_করো_Front();
            যদি (!nodep) চালু;
            UINFO(9, "Dead " << nodep);
            যদি (Ast_গ্রন্থি_FTask* ধ্রুবক taskp = VN_CAST(nodep, গ্রন্থি_FTask)) {
                যদি (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } অন্যথা যদি (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } অন্যথা {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    শূন্য deadCheckDemote() {
        জন্য (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* ধ্রুবক vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            যদি (!vtxp->outSize1()) চালু;
            যদি (!vtxp->isVirtual()) চালু;
            জন্য (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_গ্রন্থি* ধ্রুবক nodep = edge.শীর্ষ()->as<DeadVertex>()->nodep();
                Ast_গ্রন্থি_FTask* ধ্রুবক funcp = VN_AS(nodep, গ্রন্থি_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                যদি (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(মিথ্যা);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    শূন্য deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        জন্য (যৌক্তিক retry = সত্য; retry;) {
            retry = মিথ্যা;
            Ast_গ্রন্থি_Module* nextmodp;
            জন্য (Ast_গ্রন্থি_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), গ্রন্থি_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ধ্রুবক যৌক্তিক keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                যদি (modp->dead() || (!modp->is_শীর্ষ() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    যদি (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ধ্রুবক AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    যদি (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = সত্য;
                }
            }
        }
    }
    যৌক্তিক mightElimVar(ধ্রুবক AstVar* nodep) ধ্রুবক {
        যদি (nodep->isSigPublic()) ফেরত মিথ্যা;  // Can't elim publics!
        যদি (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ফেরত মিথ্যা;
        যদি (nodep->is_অস্থায়ী() && !nodep->isTrace()) ফেরত সত্য;
        ফেরত m_elimUserVars;  // Post-Trace can kill most anything
    }

    শূন্য deadCheckScope() {
        জন্য (যৌক্তিক retry = সত্য; retry;) {
            retry = মিথ্যা;
            জন্য (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* ধ্রুবক scp = *it;
                যদি (!scp) চালু;
                যদি (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    যদি (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = সত্য;
                }
            }
        }
    }

    শূন্য deadCheckCells() {
        জন্য (AstCell* cellp : m_cellsp) {
            যদি (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    শূন্য deadCheckClasses() {
        জন্য (যৌক্তিক retry = সত্য; retry;) {
            retry = মিথ্যা;
            জন্য (auto& itr : m_classesp) {
                যদি (AstClass* ধ্রুবক nodep = itr) {  // nullptr if deleted earlier
                    যদি (nodep->user1() == 0) {
                        যদি (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        যদি (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = সত্য;
                    }
                }
            }
        }
    }

    শূন্য deadCheckVar() {
        // Delete any unused varscopes
        জন্য (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            যদি (vscp->varp()->user2()) চালু;
            যদি (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ধ্রুবক std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                জন্য (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_গ্রন্থি_Assign* ধ্রুবক assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                যদি (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        জন্য (যৌক্তিক retry = সত্য; retry;) {
            retry = মিথ্যা;
            জন্য (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* ধ্রুবক varp = *it;
                যদি (!varp) চালু;
                যদি (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    যদি (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = সত্য;
                }
            }
        }
        জন্য (std::vector<Ast_গ্রন্থি*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            যদি ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক classp
                    = VN_CAST((*it), গ্রন্থি_UOr_Struct_DType)) {
                    যৌক্তিক cont = সত্য;
                    জন্য (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        যদি (memberp->user1() != 0) {
                            cont = মিথ্যা;
                            বিরতি;
                        }
                    }
                    যদি (!cont) চালু;
                }
                deleting(*it);
            }
        }
    }

সর্বজনীন:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, যৌক্তিক elimUserVars, যৌক্তিক elimDTypes, যৌক্তিক elimScopes,
                যৌক্তিক elimCells, যৌক্তিক elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        যদি (AstVarScope* ধ্রুবক vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        জন্য (auto& itr : m_dtypePkgsp) {
            যদি (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", মিথ্যা);

        যদি (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        যদি (elimScopes) deadCheckScope();
        যদি (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        যদি (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_বৃক্ষ();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::add_Stat_যোগ("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::add_Stat_যোগ("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::add_Stat_যোগ("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::add_Stat_যোগ("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

শূন্য V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, মিথ্যা, মিথ্যা, মিথ্যা, মিথ্যা, মিথ্যা};
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("deadModules", 0, dump_বৃক্ষ_Either_Level() >= 6);
}

শূন্য V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, মিথ্যা, সত্য, মিথ্যা, মিথ্যা, সত্য}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("deadDtypes", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, মিথ্যা, সত্য, সত্য, মিথ্যা, মিথ্যা}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("deadDtypesScoped", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, সত্য, সত্য, মিথ্যা, সত্য, সত্য}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("deadAll", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, সত্য, সত্য, সত্য, সত্য, সত্য}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("deadAllScoped", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
