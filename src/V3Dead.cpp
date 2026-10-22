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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dead.h"

#شامل "V3ConstPool.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <queue>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

رده DeadVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    Ast_گره* ثابت m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    منطقی m_removable = نادرست;  // Subject to dead removal
    منطقی m_isVirtual = نادرست;  // Virtual node, indirection

عمومی:
    DeadVertex(V3Graph* graphp, Ast_گره* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = پیشفرض;
    string dotShape() ثابت override {
        بازگشت isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_گره* nodep() ثابت VL_MT_STABLE { بازگشت m_nodep; }
    string name() ثابت override VL_MT_STABLE {
        بازگشت (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    منطقی removable() ثابت { بازگشت m_removable; }
    تهی removable(منطقی flag) { m_removable = flag; }
    منطقی isVirtual() ثابت { بازگشت m_isVirtual; }
    تهی isVirtual(منطقی flag) { m_isVirtual = flag; }
    uint64_t workPos() ثابت { بازگشت m_workPos; }
    تهی workPos(uint64_t مقدار) { m_workPos = مقدار; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
رده DeadWorkList final {
    // MEMBERS
    ایستا uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
عمومی:
    // METHODS
    منطقی empty() ثابت { بازگشت m_works.empty(); }
    تهی افزودن(DeadVertex* vtxp) {
        اگر (vtxp->workPos()) بازگشت;  // Already on list
        ثابت uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    تهی erase(DeadVertex* vtxp) {
        ثابت uint64_t id = vtxp->workPos();
        اگر (!id) بازگشت;
        UINFO(9, "Worklist.erase " << vtxp);
        ثابت auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* get_برداشت_Front() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        ثابت auto it = m_works.begin();
        DeadVertex* ثابت vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        بازگشت vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

رده DeadGraph final : عمومی V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    تهی newEdge(DeadVertex* fromp, DeadVertex* بالاترین) {
        جدید V3GraphEdge{این, fromp, بالاترین, 1, نادرست};
        اگر (بالاترین->removable()) m_funcs.erase(بالاترین);  // Now has an input edge
    }
    تهی افزودن_Work_Maybe(DeadVertex* vtxp, منطقی allowSize1) {
        اگر (vtxp->removable()) {
            اگر (VN_IS(vtxp->nodep(), گره_FTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.افزودن(vtxp);
        }
    }

عمومی:
    // METHODS
    DeadGraph() = پیشفرض;
    ~DeadGraph() override = پیشفرض;
    DeadVertex* findNewVertex(Ast_گره* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        اگر (!vtxp) {
            vtxp = جدید DeadVertex{این, nodep};
            nodep->user2p(vtxp);
            افزودن_Work_Maybe(vtxp, نادرست);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        بازگشت vtxp;
    }
    DeadVertex* findVirtualVertex(Ast_گره* nodep) {
        ثابت auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        اگر (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } وگرنه {
            vtxp = جدید DeadVertex{این, nodep};
            vtxp->removable(درست);
            vtxp->isVirtual(درست);
            افزودن_Work_Maybe(vtxp, نادرست);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        بازگشت vtxp;
    }
    تهی findNewRemovableVertex(Ast_گره* nodep, منطقی removable) {
        DeadVertex* ثابت vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        اگر (removable && !vtxp->removable()) {
            vtxp->removable(درست);
            افزودن_Work_Maybe(vtxp, نادرست);
        }
    }
    تهی delete_گره_Vertex(Ast_گره* nodep) {
        اگر (DeadVertex* ثابت vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    تهی deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        برای (ثابت V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* ثابت toVtxp = static_cast<DeadVertex*>(oedge.بالاترین());
            افزودن_Work_Maybe(toVtxp, درست);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        اگر (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        اگر (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(این), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    تهی needs(Ast_گره* nodep, Ast_گره* parentp) {
        اگر (parentp == nodep) بازگشت;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* ثابت parentVtxp = findNewVertex(parentp);
        DeadVertex* ثابت گره_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << گره_Vtxp);
        newEdge(parentVtxp, گره_Vtxp);
    }
    تهی needsVirtual(Ast_گره_FTask* nodep, Ast_گره* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        اگر (parentp == nodep) بازگشت;  // No need for tracking needs itself (recursion)
        DeadVertex* ثابت parentVtxp = findNewVertex(parentp);
        DeadVertex* ثابت virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    تهی funcVirtual(Ast_گره_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* ثابت virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* ثابت گره_Vtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << گره_Vtxp);
        newEdge(virtualVtxp, گره_Vtxp);
    }
    منطقی funcsEmpty() ثابت { بازگشت m_funcs.empty(); }
    Ast_گره* funcs_Get_برداشت_Front() {
        DeadVertex* ثابت vtxp = m_funcs.get_برداشت_Front();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        اگر (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            بازگشت nullptr;  // Caller will search again
        }
        Ast_گره* ثابت nodep = vtxp->nodep();
        بازگشت nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

رده DeadVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, Ast_گره_Assign*>;

    // STATE - across all visitors
    ثابت منطقی m_elimUserVars;  // Allow removal of user's vars
    ثابت منطقی m_elimDTypes;  // Allow removal of DTypes
    ثابت منطقی m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<Ast_گره*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_گره_DType*, Ast_گره_Module*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    منطقی m_sideEffect = نادرست;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    منطقی m_inAssign = نادرست;  // Currently in an assign
    Ast_گره_DType* m_curDTypep = nullptr;  // Current NodeDType
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    Ast_گره* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    منطقی m_inFTask = نادرست;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    تهی deleting(Ast_گره* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.delete_گره_Vertex(nodep);
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    تهی checkAll(Ast_گره* nodep) {
        اگر (Ast_گره* ثابت subnodep = nodep->dtypep()) {
            اگر (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        اگر (Ast_گره* ثابت subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    تهی checkVarRef(Ast_گره_Var_Ref* nodep) ثابت {
        اگر (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    تهی checkDType(Ast_گره_DType* nodep) {
        اگر (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.افزودن_back(nodep);
        }
        اگر (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep = VN_CAST(nodep, گره_UOr_Struct_DType)) {
            اگر (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        اگر (Ast_گره* ثابت subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        اگر (Ast_گره* ثابت subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    تهی needsTask(Ast_گره_FTask* taskp, Ast_گره* containerp) {
        اگر (!taskp) بازگشت;  // Unlinked
        اگر (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } وگرنه {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        اگر (nodep->dead()) بازگشت;
        اگر (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            اگر (classp->extendsp()) classp->extendsp()->user1Inc();
            اگر (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.افزودن_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    تهی visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        برای (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    تهی visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    تهی visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        اگر (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        اگر (m_modp->isConstPool()) nodep->user1Inc();
        اگر (!nodep->is_بالاترین() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.افزودن_back(nodep);
        }
    }
    تهی visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.افزودن_back(nodep);
        nodep->modp()->user1Inc();
    }

    تهی visit(Ast_گره_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        اگر (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        اگر (nodep->varp()) nodep->varp()->user1Inc();
        اگر (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        اگر (!m_sideEffect && !nodep->isPure()) m_sideEffect = درست;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } وگرنه {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    تهی visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    تهی visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->varp()) nodep->varp()->user1Inc();
    }
    تهی visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } وگرنه {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    تهی visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } وگرنه {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        اگر (nodep->classp()) nodep->classp()->user1Inc();
    }
    تهی visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        اگر (nodep->modportp()) {
            اگر (m_elimCells) {
                nodep->modportp(nullptr);
            } وگرنه {
                nodep->modportp()->user1Inc();
            }
        }
        اگر (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    تهی visit(Ast_گره_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    تهی visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } وگرنه {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    تهی visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->varp()) nodep->varp()->user1Inc();
        اگر (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    تهی visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    تهی visit(AstModport* nodep) override {
        iterateChildren(nodep);
        اگر (m_elimCells) {
            اگر (!nodep->varsp()) {
                deleting(nodep);
                بازگشت;
            }
        }
        checkAll(nodep);
    }
    تهی visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    تهی visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.افزودن_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        اگر (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    تهی visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->scopep()) nodep->scopep()->user1Inc();
        اگر (mightElimVar(nodep->varp())) m_vscsp.افزودن_back(nodep);
    }
    تهی visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        اگر (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        اگر (mightElimVar(nodep)) {
            m_varsp.افزودن_back(nodep);
        } وگرنه {
            اگر (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ثابت منطقی assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = درست;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
            اگر (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } وگرنه {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        اگر (assignInAssign) m_sideEffect = درست;  // Parent assign shouldn't optimize
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        ثابت منطقی removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = درست;
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        اگر (!removable) {
            اگر (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } وگرنه {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    تهی visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    تهی visit(Ast_گره* nodep) override {
        اگر (!m_sideEffect && !nodep->isPure()) m_sideEffect = درست;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    تهی deadCheckTypedefs() {
        برای (AstTypedef* typedefp : m_typedefsp) {
            اگر (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                ادامه;
            }
            checkAll(typedefp);
        }
    }
    منطقی shouldDeleteTypedef(ثابت AstTypedef* typedefp) ثابت {
        اگر (ثابت auto* ثابت structp = VN_CAST(typedefp->subDTypep(), گره_UOr_Struct_DType)) {
            اگر (structp->user1() && !structp->packed()) بازگشت نادرست;
        }
        بازگشت m_elimCells && !typedefp->attrPublic();
    }

    تهی deadCheckTasks() {
        وقتی (!m_graph.funcsEmpty()) {
            Ast_گره* ثابت nodep = m_graph.funcs_Get_برداشت_Front();
            اگر (!nodep) ادامه;
            UINFO(9, "Dead " << nodep);
            اگر (Ast_گره_FTask* ثابت taskp = VN_CAST(nodep, گره_FTask)) {
                اگر (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } وگرنه اگر (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } وگرنه {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    تهی deadCheckDemote() {
        برای (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* ثابت vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            اگر (!vtxp->outSize1()) ادامه;
            اگر (!vtxp->isVirtual()) ادامه;
            برای (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                Ast_گره* ثابت nodep = edge.بالاترین()->as<DeadVertex>()->nodep();
                Ast_گره_FTask* ثابت funcp = VN_AS(nodep, گره_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                اگر (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(نادرست);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    تهی deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        برای (منطقی retry = درست; retry;) {
            retry = نادرست;
            Ast_گره_Module* nextmodp;
            برای (Ast_گره_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), گره_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ثابت منطقی keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                اگر (modp->dead() || (!modp->is_بالاترین() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    اگر (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ثابت AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    اگر (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = درست;
                }
            }
        }
    }
    منطقی mightElimVar(ثابت AstVar* nodep) ثابت {
        اگر (nodep->isSigPublic()) بازگشت نادرست;  // Can't elim publics!
        اگر (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) بازگشت نادرست;
        اگر (nodep->isTemp() && !nodep->isTrace()) بازگشت درست;
        بازگشت m_elimUserVars;  // Post-Trace can kill most anything
    }

    تهی deadCheckScope() {
        برای (منطقی retry = درست; retry;) {
            retry = نادرست;
            برای (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* ثابت scp = *it;
                اگر (!scp) ادامه;
                اگر (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    اگر (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = درست;
                }
            }
        }
    }

    تهی deadCheckCells() {
        برای (AstCell* cellp : m_cellsp) {
            اگر (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    تهی deadCheckClasses() {
        برای (منطقی retry = درست; retry;) {
            retry = نادرست;
            برای (auto& itr : m_classesp) {
                اگر (AstClass* ثابت nodep = itr) {  // nullptr if deleted earlier
                    اگر (nodep->user1() == 0) {
                        اگر (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        اگر (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = درست;
                    }
                }
            }
        }
    }

    تهی deadCheckVar() {
        // Delete any unused varscopes
        برای (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            اگر (vscp->varp()->user2()) ادامه;
            اگر (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ثابت std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                برای (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    Ast_گره_Assign* ثابت assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                اگر (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        برای (منطقی retry = درست; retry;) {
            retry = نادرست;
            برای (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* ثابت varp = *it;
                اگر (!varp) ادامه;
                اگر (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    اگر (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = درست;
                }
            }
        }
        برای (std::vector<Ast_گره*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            اگر ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت classp
                    = VN_CAST((*it), گره_UOr_Struct_DType)) {
                    منطقی cont = درست;
                    برای (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        اگر (memberp->user1() != 0) {
                            cont = نادرست;
                            بشکن;
                        }
                    }
                    اگر (!cont) ادامه;
                }
                deleting(*it);
            }
        }
    }

عمومی:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, منطقی elimUserVars, منطقی elimDTypes, منطقی elimScopes,
                منطقی elimCells, منطقی elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        اگر (AstVarScope* ثابت vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        برای (auto& itr : m_dtypePkgsp) {
            اگر (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        اگر (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", نادرست);

        اگر (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        اگر (elimScopes) deadCheckScope();
        اگر (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        اگر (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_درخت();  // Removing assignments may affect isPure
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

تهی V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, نادرست, نادرست, نادرست, نادرست, نادرست};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadModules", 0, dump_درخت_Either_Level() >= 6);
}

تهی V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, نادرست, درست, نادرست, نادرست, درست}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadDtypes", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, نادرست, درست, درست, نادرست, نادرست}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadDtypesScoped", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, درست, درست, نادرست, درست, درست}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadAll", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, درست, درست, درست, درست, درست}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadAllScoped", 0, dump_درخت_Either_Level() >= 3);
}
