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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dead.h"

#ضمن "V3ConstPool.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <queue>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

صنف DeadVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    AstNode* ثابت m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    منطقي m_removable = ܕܓܠܐ;  // Subject to dead removal
    منطقي m_isVirtual = ܕܓܠܐ;  // Virtual node, indirection

عام:
    DeadVertex(V3Graph* graphp, AstNode* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = افتراضي;
    string dotShape() ثابت override {
        ܦܢܐ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    AstNode* nodep() ثابت VL_MT_STABLE { ܦܢܐ m_nodep; }
    string name() ثابت override VL_MT_STABLE {
        ܦܢܐ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    منطقي removable() ثابت { ܦܢܐ m_removable; }
    فراغ removable(منطقي flag) { m_removable = flag; }
    منطقي isVirtual() ثابت { ܦܢܐ m_isVirtual; }
    فراغ isVirtual(منطقي flag) { m_isVirtual = flag; }
    uint64_t workPos() ثابت { ܦܢܐ m_workPos; }
    فراغ workPos(uint64_t value) { m_workPos = value; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
صنف DeadWorkList final {
    // MEMBERS
    ساكن uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
عام:
    // METHODS
    منطقي empty() ثابت { ܦܢܐ m_works.empty(); }
    فراغ push(DeadVertex* vtxp) {
        ܐܢ (vtxp->workPos()) ܦܢܐ;  // Already on list
        ثابت uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    فراغ erase(DeadVertex* vtxp) {
        ثابت uint64_t id = vtxp->workPos();
        ܐܢ (!id) ܦܢܐ;
        UINFO(9, "Worklist.erase " << vtxp);
        ثابت auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* getPopFront() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        ثابت auto it = m_works.begin();
        DeadVertex* ثابت vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        ܦܢܐ vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

صنف DeadGraph final : عام V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    فراغ newEdge(DeadVertex* fromp, DeadVertex* top) {
        جديد V3GraphEdge{هذا, fromp, top, 1, ܕܓܠܐ};
        ܐܢ (top->removable()) m_funcs.erase(top);  // Now has an input edge
    }
    فراغ pushWorkMaybe(DeadVertex* vtxp, منطقي allowSize1) {
        ܐܢ (vtxp->removable()) {
            ܐܢ (VN_IS(vtxp->nodep(), NodeFTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.push(vtxp);
        }
    }

عام:
    // METHODS
    DeadGraph() = افتراضي;
    ~DeadGraph() override = افتراضي;
    DeadVertex* findNewVertex(AstNode* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        ܐܢ (!vtxp) {
            vtxp = جديد DeadVertex{هذا, nodep};
            nodep->user2p(vtxp);
            pushWorkMaybe(vtxp, ܕܓܠܐ);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ܦܢܐ vtxp;
    }
    DeadVertex* findVirtualVertex(AstNode* nodep) {
        ثابت auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        ܐܢ (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } ܐܚܪܢܐ {
            vtxp = جديد DeadVertex{هذا, nodep};
            vtxp->removable(ܫܪܪܐ);
            vtxp->isVirtual(ܫܪܪܐ);
            pushWorkMaybe(vtxp, ܕܓܠܐ);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ܦܢܐ vtxp;
    }
    فراغ findNewRemovableVertex(AstNode* nodep, منطقي removable) {
        DeadVertex* ثابت vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ܐܢ (removable && !vtxp->removable()) {
            vtxp->removable(ܫܪܪܐ);
            pushWorkMaybe(vtxp, ܕܓܠܐ);
        }
    }
    فراغ deleteNodeVertex(AstNode* nodep) {
        ܐܢ (DeadVertex* ثابت vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    فراغ deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        لكل (ثابت V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* ثابت toVtxp = static_cast<DeadVertex*>(oedge.top());
            pushWorkMaybe(toVtxp, ܫܪܪܐ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ܐܢ (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        ܐܢ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(هذا), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    فراغ needs(AstNode* nodep, AstNode* parentp) {
        ܐܢ (parentp == nodep) ܦܢܐ;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* ثابت parentVtxp = findNewVertex(parentp);
        DeadVertex* ثابت nodeVtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << nodeVtxp);
        newEdge(parentVtxp, nodeVtxp);
    }
    فراغ needsVirtual(AstNodeFTask* nodep, AstNode* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ܐܢ (parentp == nodep) ܦܢܐ;  // No need for tracking needs itself (recursion)
        DeadVertex* ثابت parentVtxp = findNewVertex(parentp);
        DeadVertex* ثابت virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    فراغ funcVirtual(AstNodeFTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* ثابت virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* ثابت nodeVtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << nodeVtxp);
        newEdge(virtualVtxp, nodeVtxp);
    }
    منطقي funcsEmpty() ثابت { ܦܢܐ m_funcs.empty(); }
    AstNode* funcsGetPopFront() {
        DeadVertex* ثابت vtxp = m_funcs.getPopFront();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ܐܢ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            ܦܢܐ nullptr;  // Caller will search again
        }
        AstNode* ثابت nodep = vtxp->nodep();
        ܦܢܐ nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

صنف DeadVisitor final : عام VNVisitor {
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
    using AssignMap = std::multimap<AstVarScope*, AstNodeAssign*>;

    // STATE - across all visitors
    ثابت منطقي m_elimUserVars;  // Allow removal of user's vars
    ثابت منطقي m_elimDTypes;  // Allow removal of DTypes
    ثابت منطقي m_elimCells;  // Allow removal of Cells

    DeadGraph m_graph;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::vector<AstVar*> m_varsp;
    std::vector<AstNode*> m_dtypeElimsp;  // Data types might eliminate
    std::map<AstNodeDType*, AstNodeModule*> m_dtypePkgsp;  // Data type's containing package
    std::vector<AstVarScope*> m_vscsp;
    std::vector<AstScope*> m_scopesp;
    std::vector<AstCell*> m_cellsp;
    std::vector<AstClass*> m_classesp;
    std::vector<AstTypedef*> m_typedefsp;
    AssignMap m_assignMap;  // List of all simple assignments for each variable
    منطقي m_sideEffect = ܕܓܠܐ;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_inAssign = ܕܓܠܐ;  // Currently in an assign
    AstNodeDType* m_curDTypep = nullptr;  // Current NodeDType
    AstNodeModule* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    AstNode* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    منطقي m_inFTask = ܕܓܠܐ;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    فراغ deleting(AstNode* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.deleteNodeVertex(nodep);
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    فراغ checkAll(AstNode* nodep) {
        ܐܢ (AstNode* ثابت subnodep = nodep->dtypep()) {
            ܐܢ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ܐܢ (AstNode* ثابت subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    فراغ checkVarRef(AstNodeVarRef* nodep) ثابت {
        ܐܢ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    فراغ checkDType(AstNodeDType* nodep) {
        ܐܢ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.push_back(nodep);
        }
        ܐܢ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ܐܢ (ثابت AstNodeUOrStructDType* ثابت sdtypep = VN_CAST(nodep, NodeUOrStructDType)) {
            ܐܢ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ܐܢ (AstNode* ثابت subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ܐܢ (AstNode* ثابت subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    فراغ needsTask(AstNodeFTask* taskp, AstNode* containerp) {
        ܐܢ (!taskp) ܦܢܐ;  // Unlinked
        ܐܢ (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } ܐܚܪܢܐ {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        ܐܢ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ܐܢ (nodep->dead()) ܦܢܐ;
        ܐܢ (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            ܐܢ (classp->extendsp()) classp->extendsp()->user1Inc();
            ܐܢ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.push_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    فراغ visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        لكل (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    فراغ visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    فراغ visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ܐܢ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ܐܢ (m_modp->isConstPool()) nodep->user1Inc();
        ܐܢ (!nodep->isTop() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.push_back(nodep);
        }
    }
    فراغ visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.push_back(nodep);
        nodep->modp()->user1Inc();
    }

    فراغ visit(AstNodeVarRef* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ܐܢ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ܐܢ (nodep->varp()) nodep->varp()->user1Inc();
        ܐܢ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!m_sideEffect && !nodep->isPure()) m_sideEffect = ܫܪܪܐ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ܐܢ (nodep->classOrPackagep()) {
            ܐܢ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ܐܚܪܢܐ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    فراغ visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    فراغ visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->varp()) nodep->varp()->user1Inc();
    }
    فراغ visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ܐܢ (nodep->classOrPackagep()) {
            ܐܢ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ܐܚܪܢܐ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    فراغ visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ܐܢ (nodep->classOrPackagep()) {
            ܐܢ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ܐܚܪܢܐ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ܐܢ (nodep->classp()) nodep->classp()->user1Inc();
    }
    فراغ visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ܐܢ (nodep->modportp()) {
            ܐܢ (m_elimCells) {
                nodep->modportp(nullptr);
            } ܐܚܪܢܐ {
                nodep->modportp()->user1Inc();
            }
        }
        ܐܢ (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    فراغ visit(AstNodeDType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->classOrPackagep()) {
            ܐܢ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ܐܚܪܢܐ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    فراغ visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ܐܢ (nodep->varp()) nodep->varp()->user1Inc();
        ܐܢ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    فراغ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ܐܢ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    فراغ visit(AstModport* nodep) override {
        iterateChildren(nodep);
        ܐܢ (m_elimCells) {
            ܐܢ (!nodep->varsp()) {
                deleting(nodep);
                ܦܢܐ;
            }
        }
        checkAll(nodep);
    }
    فراغ visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    فراغ visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.push_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ܐܢ (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    فراغ visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->scopep()) nodep->scopep()->user1Inc();
        ܐܢ (mightElimVar(nodep->varp())) m_vscsp.push_back(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ܐܢ (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ܐܢ (mightElimVar(nodep)) {
            m_varsp.push_back(nodep);
        } ܐܚܪܢܐ {
            ܐܢ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    فراغ visit(AstNodeAssign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ثابت منطقي assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = ܫܪܪܐ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ܐܢ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ܐܚܪܢܐ {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ܐܢ (assignInAssign) m_sideEffect = ܫܪܪܐ;  // Parent assign shouldn't optimize
    }
    فراغ visit(AstNodeFTask* nodep) override {
        ثابت منطقي removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = ܫܪܪܐ;
        iterateChildren(nodep);
        checkAll(nodep);
        ܐܢ (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        ܐܢ (!removable) {
            ܐܢ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ܐܢ (nodep->classOrPackagep()) {
            ܐܢ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ܐܚܪܢܐ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    فراغ visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    فراغ visit(AstNode* nodep) override {
        ܐܢ (!m_sideEffect && !nodep->isPure()) m_sideEffect = ܫܪܪܐ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    فراغ deadCheckTypedefs() {
        لكل (AstTypedef* typedefp : m_typedefsp) {
            ܐܢ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                استمر;
            }
            checkAll(typedefp);
        }
    }
    منطقي shouldDeleteTypedef(ثابت AstTypedef* typedefp) ثابت {
        ܐܢ (ثابت auto* ثابت structp = VN_CAST(typedefp->subDTypep(), NodeUOrStructDType)) {
            ܐܢ (structp->user1() && !structp->packed()) ܦܢܐ ܕܓܠܐ;
        }
        ܦܢܐ m_elimCells && !typedefp->attrPublic();
    }

    فراغ deadCheckTasks() {
        ܟܡܐ (!m_graph.funcsEmpty()) {
            AstNode* ثابت nodep = m_graph.funcsGetPopFront();
            ܐܢ (!nodep) استمر;
            UINFO(9, "Dead " << nodep);
            ܐܢ (AstNodeFTask* ثابت taskp = VN_CAST(nodep, NodeFTask)) {
                ܐܢ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ܐܚܪܢܐ ܐܢ (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } ܐܚܪܢܐ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    فراغ deadCheckDemote() {
        لكل (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* ثابت vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ܐܢ (!vtxp->outSize1()) استمر;
            ܐܢ (!vtxp->isVirtual()) استمر;
            لكل (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                AstNode* ثابت nodep = edge.top()->as<DeadVertex>()->nodep();
                AstNodeFTask* ثابت funcp = VN_AS(nodep, NodeFTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ܐܢ (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(ܕܓܠܐ);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    فراغ deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        لكل (منطقي retry = ܫܪܪܐ; retry;) {
            retry = ܕܓܠܐ;
            AstNodeModule* nextmodp;
            لكل (AstNodeModule* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), NodeModule);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ثابت منطقي keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ܐܢ (modp->dead() || (!modp->isTop() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ܐܢ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ثابت AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ܐܢ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = ܫܪܪܐ;
                }
            }
        }
    }
    منطقي mightElimVar(ثابت AstVar* nodep) ثابت {
        ܐܢ (nodep->isSigPublic()) ܦܢܐ ܕܓܠܐ;  // Can't elim publics!
        ܐܢ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (nodep->isTemp() && !nodep->isTrace()) ܦܢܐ ܫܪܪܐ;
        ܦܢܐ m_elimUserVars;  // Post-Trace can kill most anything
    }

    فراغ deadCheckScope() {
        لكل (منطقي retry = ܫܪܪܐ; retry;) {
            retry = ܕܓܠܐ;
            لكل (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* ثابت scp = *it;
                ܐܢ (!scp) استمر;
                ܐܢ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ܐܢ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = ܫܪܪܐ;
                }
            }
        }
    }

    فراغ deadCheckCells() {
        لكل (AstCell* cellp : m_cellsp) {
            ܐܢ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    فراغ deadCheckClasses() {
        لكل (منطقي retry = ܫܪܪܐ; retry;) {
            retry = ܕܓܠܐ;
            لكل (auto& itr : m_classesp) {
                ܐܢ (AstClass* ثابت nodep = itr) {  // nullptr if deleted earlier
                    ܐܢ (nodep->user1() == 0) {
                        ܐܢ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ܐܢ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = ܫܪܪܐ;
                    }
                }
            }
        }
    }

    فراغ deadCheckVar() {
        // Delete any unused varscopes
        لكل (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ܐܢ (vscp->varp()->user2()) استمر;
            ܐܢ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ثابت std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                لكل (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    AstNodeAssign* ثابت assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ܐܢ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        لكل (منطقي retry = ܫܪܪܐ; retry;) {
            retry = ܕܓܠܐ;
            لكل (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* ثابت varp = *it;
                ܐܢ (!varp) استمر;
                ܐܢ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ܐܢ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = ܫܪܪܐ;
                }
            }
        }
        لكل (std::vector<AstNode*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            ܐܢ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ܐܢ (ثابت AstNodeUOrStructDType* ثابت classp
                    = VN_CAST((*it), NodeUOrStructDType)) {
                    منطقي cont = ܫܪܪܐ;
                    لكل (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ܐܢ (memberp->user1() != 0) {
                            cont = ܕܓܠܐ;
                            اكسر;
                        }
                    }
                    ܐܢ (!cont) استمر;
                }
                deleting(*it);
            }
        }
    }

عام:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, منطقي elimUserVars, منطقي elimDTypes, منطقي elimScopes,
                منطقي elimCells, منطقي elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        ܐܢ (AstVarScope* ثابت vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        لكل (auto& itr : m_dtypePkgsp) {
            ܐܢ (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ܐܢ (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", ܕܓܠܐ);

        ܐܢ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ܐܢ (elimScopes) deadCheckScope();
        ܐܢ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ܐܢ (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clearCacheTree();  // Removing assignments may affect isPure
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

فراغ V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, ܕܓܠܐ, ܕܓܠܐ, ܕܓܠܐ, ܕܓܠܐ, ܕܓܠܐ};
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadModules", 0, dumpTreeEitherLevel() >= 6);
}

فراغ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ܕܓܠܐ, ܫܪܪܐ, ܕܓܠܐ, ܕܓܠܐ, ܫܪܪܐ}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadDtypes", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ܕܓܠܐ, ܫܪܪܐ, ܫܪܪܐ, ܕܓܠܐ, ܕܓܠܐ}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadDtypesScoped", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ܫܪܪܐ, ܫܪܪܐ, ܕܓܠܐ, ܫܪܪܐ, ܫܪܪܐ}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadAll", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, ܫܪܪܐ, ܫܪܪܐ, ܫܪܪܐ, ܫܪܪܐ, ܫܪܪܐ}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadAllScoped", 0, dumpTreeEitherLevel() >= 3);
}
