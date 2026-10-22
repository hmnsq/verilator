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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Dead.h"

#include "V3ConstPool.h"
#include "V3Graph.h"
#include "V3Stats.h"

#include <queue>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Dead tracking graph

class DeadVertex final : public V3GraphVertex {
    VL_RTTI_IMPL(DeadVertex, V3GraphVertex)
    AstNode* const m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    bool m_removable = false;  // Subject to dead removal
    bool m_isVirtual = false;  // Virtual node, indirection

public:
    DeadVertex(V3Graph* graphp, AstNode* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~DeadVertex() override = default;
    string dotShape() const override {
        คืน isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    AstNode* nodep() const VL_MT_STABLE { คืน m_nodep; }
    string name() const override VL_MT_STABLE {
        คืน (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    bool removable() const { คืน m_removable; }
    void removable(bool flag) { m_removable = flag; }
    bool isVirtual() const { คืน m_isVirtual; }
    void isVirtual(bool flag) { m_isVirtual = flag; }
    uint64_t workPos() const { คืน m_workPos; }
    void workPos(uint64_t value) { m_workPos = value; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
class DeadWorkList final {
    // MEMBERS
    static uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, DeadVertex*> m_works;  // Work list by sequence
public:
    // METHODS
    bool empty() const { คืน m_works.empty(); }
    void push(DeadVertex* vtxp) {
        ถ้า (vtxp->workPos()) คืน;  // Already on list
        const uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    void erase(DeadVertex* vtxp) {
        const uint64_t id = vtxp->workPos();
        ถ้า (!id) คืน;
        UINFO(9, "Worklist.erase " << vtxp);
        const auto it = m_works.find(id);
        UASSERT_OBJ(it != m_works.end(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_works.erase(it);
        vtxp->workPos(0);
    }
    DeadVertex* getPopFront() {
        UDEBUGONLY(UASSERT(!empty(), "Front invalid to call on empty list"););
        const auto it = m_works.begin();
        DeadVertex* const vtxp = it->second;
        m_works.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        คืน vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

class DeadGraph final : public V3Graph {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, DeadVertex*> m_virtualVtxsp;

    void newEdge(DeadVertex* fromp, DeadVertex* top) {
        new V3GraphEdge{this, fromp, top, 1, false};
        ถ้า (top->removable()) m_funcs.erase(top);  // Now has an input edge
    }
    void pushWorkMaybe(DeadVertex* vtxp, bool allowSize1) {
        ถ้า (vtxp->removable()) {
            ถ้า (VN_IS(vtxp->nodep(), NodeFTask)
                && (vtxp->inEmpty() || (allowSize1 && vtxp->inSize1())))
                m_funcs.push(vtxp);
        }
    }

public:
    // METHODS
    DeadGraph() = default;
    ~DeadGraph() override = default;
    DeadVertex* findNewVertex(AstNode* nodep) {
        DeadVertex* vtxp = nodep->user2u().to<DeadVertex*>();
        ถ้า (!vtxp) {
            vtxp = new DeadVertex{this, nodep};
            nodep->user2p(vtxp);
            pushWorkMaybe(vtxp, false);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        คืน vtxp;
    }
    DeadVertex* findVirtualVertex(AstNode* nodep) {
        const auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        ถ้า (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } มิฉะนั้น {
            vtxp = new DeadVertex{this, nodep};
            vtxp->removable(true);
            vtxp->isVirtual(true);
            pushWorkMaybe(vtxp, false);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        คืน vtxp;
    }
    void findNewRemovableVertex(AstNode* nodep, bool removable) {
        DeadVertex* const vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ถ้า (removable && !vtxp->removable()) {
            vtxp->removable(true);
            pushWorkMaybe(vtxp, false);
        }
    }
    void deleteNodeVertex(AstNode* nodep) {
        ถ้า (DeadVertex* const vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    void deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        สำหรับ (const V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* const toVtxp = static_cast<DeadVertex*>(oedge.top());
            pushWorkMaybe(toVtxp, true);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ถ้า (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        ถ้า (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(this), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    void needs(AstNode* nodep, AstNode* parentp) {
        ถ้า (parentp == nodep) คืน;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        DeadVertex* const parentVtxp = findNewVertex(parentp);
        DeadVertex* const nodeVtxp = findNewVertex(nodep);
        UINFO(9, "Edge need " << parentVtxp << " -> " << nodeVtxp);
        newEdge(parentVtxp, nodeVtxp);
    }
    void needsVirtual(AstNodeFTask* nodep, AstNode* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        ถ้า (parentp == nodep) คืน;  // No need for tracking needs itself (recursion)
        DeadVertex* const parentVtxp = findNewVertex(parentp);
        DeadVertex* const virtualVtxp = findVirtualVertex(nodep);
        UINFO(9, "Edge needVirtual " << parentVtxp << " -> " << virtualVtxp);
        newEdge(parentVtxp, virtualVtxp);
    }
    void funcVirtual(AstNodeFTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        DeadVertex* const virtualVtxp = findVirtualVertex(nodep);
        DeadVertex* const nodeVtxp = findNewVertex(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << nodeVtxp);
        newEdge(virtualVtxp, nodeVtxp);
    }
    bool funcsEmpty() const { คืน m_funcs.empty(); }
    AstNode* funcsGetPopFront() {
        DeadVertex* const vtxp = m_funcs.getPopFront();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ถ้า (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            คืน nullptr;  // Caller will search again
        }
        AstNode* const nodep = vtxp->nodep();
        คืน nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

class DeadVisitor final : public VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;  // For usage information see DeadGraph

    // TYPES
    using AssignMap = std::multimap<AstVarScope*, AstNodeAssign*>;

    // STATE - across all visitors
    const bool m_elimUserVars;  // Allow removal of user's vars
    const bool m_elimDTypes;  // Allow removal of DTypes
    const bool m_elimCells;  // Allow removal of Cells

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
    bool m_sideEffect = false;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    bool m_inAssign = false;  // Currently in an assign
    AstNodeDType* m_curDTypep = nullptr;  // Current NodeDType
    AstNodeModule* m_modp = nullptr;  // Current module
    AstForeachHeader* m_foreachHeaderp = nullptr;  // Current foreach header
    AstNode* m_containingFTaskRefp = nullptr;  // Parent of ftaskref (e.g. task/module)
    bool m_inFTask = false;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    void deleting(AstNode* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_graph.deleteNodeVertex(nodep);
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    void checkAll(AstNode* nodep) {
        ถ้า (AstNode* const subnodep = nodep->dtypep()) {
            ถ้า (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ถ้า (AstNode* const subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    void checkVarRef(AstNodeVarRef* nodep) const {
        ถ้า (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    void checkDType(AstNodeDType* nodep) {
        ถ้า (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.push_back(nodep);
        }
        ถ้า (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ถ้า (const AstNodeUOrStructDType* const sdtypep = VN_CAST(nodep, NodeUOrStructDType)) {
            ถ้า (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ถ้า (AstNode* const subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ถ้า (AstNode* const subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    void needsTask(AstNodeFTask* taskp, AstNode* containerp) {
        ถ้า (!taskp) คืน;  // Unlinked
        ถ้า (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } มิฉะนั้น {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    void visit(AstNodeModule* nodep) override {
        ถ้า (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ถ้า (nodep->dead()) คืน;
        ถ้า (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (AstClass* const classp = VN_CAST(nodep, Class)) {
            ถ้า (classp->extendsp()) classp->extendsp()->user1Inc();
            ถ้า (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.push_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    void visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        สำหรับ (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    void visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    void visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ถ้า (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ถ้า (m_modp->isConstPool()) nodep->user1Inc();
        ถ้า (!nodep->isTop() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.push_back(nodep);
        }
    }
    void visit(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.push_back(nodep);
        nodep->modp()->user1Inc();
    }

    void visit(AstNodeVarRef* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        ถ้า (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ถ้า (nodep->varp()) nodep->varp()->user1Inc();
        ถ้า (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    void visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        ถ้า (!m_sideEffect && !nodep->isPure()) m_sideEffect = true;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ถ้า (nodep->classOrPackagep()) {
            ถ้า (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } มิฉะนั้น {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    void visit(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    void visit(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->varp()) nodep->varp()->user1Inc();
    }
    void visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ถ้า (nodep->classOrPackagep()) {
            ถ้า (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } มิฉะนั้น {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    void visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ถ้า (nodep->classOrPackagep()) {
            ถ้า (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } มิฉะนั้น {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ถ้า (nodep->classp()) nodep->classp()->user1Inc();
    }
    void visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ถ้า (nodep->modportp()) {
            ถ้า (m_elimCells) {
                nodep->modportp(nullptr);
            } มิฉะนั้น {
                nodep->modportp()->user1Inc();
            }
        }
        ถ้า (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
    }
    void visit(AstNodeDType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    void visit(AstEnumItemRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->classOrPackagep()) {
            ถ้า (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } มิฉะนั้น {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    void visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ถ้า (nodep->varp()) nodep->varp()->user1Inc();
        ถ้า (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    void visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ถ้า (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    void visit(AstModport* nodep) override {
        iterateChildren(nodep);
        ถ้า (m_elimCells) {
            ถ้า (!nodep->varsp()) {
                deleting(nodep);
                คืน;
            }
        }
        checkAll(nodep);
    }
    void visit(AstForeachHeader* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    void visit(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.push_back(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        ถ้า (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    void visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->scopep()) nodep->scopep()->user1Inc();
        ถ้า (mightElimVar(nodep->varp())) m_vscsp.push_back(nodep);
    }
    void visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ถ้า (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ถ้า (mightElimVar(nodep)) {
            m_varsp.push_back(nodep);
        } มิฉะนั้น {
            ถ้า (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    void visit(AstNodeAssign* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        const bool assignInAssign = m_inAssign;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_inAssign);
            VL_RESTORER(m_sideEffect);
            m_inAssign = true;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterateAndNextNull(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* const varrefp = VN_CAST(nodep->lhsp(), VarRef);
            ถ้า (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } มิฉะนั้น {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ถ้า (assignInAssign) m_sideEffect = true;  // Parent assign shouldn't optimize
    }
    void visit(AstNodeFTask* nodep) override {
        const bool removable = !(nodep->taskPublic() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->classMethod()));
        m_graph.findNewRemovableVertex(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_inFTask);
        m_containingFTaskRefp = nodep;
        m_inFTask = true;
        iterateChildren(nodep);
        checkAll(nodep);
        ถ้า (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        ถ้า (!removable) {
            ถ้า (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ถ้า (nodep->classOrPackagep()) {
            ถ้า (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } มิฉะนั้น {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    void visit(AstClockingItem* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    void visit(AstNode* nodep) override {
        ถ้า (!m_sideEffect && !nodep->isPure()) m_sideEffect = true;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    void deadCheckTypedefs() {
        สำหรับ (AstTypedef* typedefp : m_typedefsp) {
            ถ้า (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                continue;
            }
            checkAll(typedefp);
        }
    }
    bool shouldDeleteTypedef(const AstTypedef* typedefp) const {
        ถ้า (const auto* const structp = VN_CAST(typedefp->subDTypep(), NodeUOrStructDType)) {
            ถ้า (structp->user1() && !structp->packed()) คืน false;
        }
        คืน m_elimCells && !typedefp->attrPublic();
    }

    void deadCheckTasks() {
        ขณะ (!m_graph.funcsEmpty()) {
            AstNode* const nodep = m_graph.funcsGetPopFront();
            ถ้า (!nodep) continue;
            UINFO(9, "Dead " << nodep);
            ถ้า (AstNodeFTask* const taskp = VN_CAST(nodep, NodeFTask)) {
                ถ้า (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } มิฉะนั้น ถ้า (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } มิฉะนั้น {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    void deadCheckDemote() {
        สำหรับ (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* const vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ถ้า (!vtxp->outSize1()) continue;
            ถ้า (!vtxp->isVirtual()) continue;
            สำหรับ (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                AstNode* const nodep = edge.top()->as<DeadVertex>()->nodep();
                AstNodeFTask* const funcp = VN_AS(nodep, NodeFTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ถ้า (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(false);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    void deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        สำหรับ (bool retry = true; retry;) {
            retry = false;
            AstNodeModule* nextmodp;
            สำหรับ (AstNodeModule* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), NodeModule);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                const bool keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ถ้า (modp->dead() || (!modp->isTop() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ถ้า (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](const AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ถ้า (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = true;
                }
            }
        }
    }
    bool mightElimVar(const AstVar* nodep) const {
        ถ้า (nodep->isSigPublic()) คืน false;  // Can't elim publics!
        ถ้า (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) คืน false;
        ถ้า (nodep->isTemp() && !nodep->isTrace()) คืน true;
        คืน m_elimUserVars;  // Post-Trace can kill most anything
    }

    void deadCheckScope() {
        สำหรับ (bool retry = true; retry;) {
            retry = false;
            สำหรับ (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* const scp = *it;
                ถ้า (!scp) continue;
                ถ้า (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ถ้า (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = true;
                }
            }
        }
    }

    void deadCheckCells() {
        สำหรับ (AstCell* cellp : m_cellsp) {
            ถ้า (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    void deadCheckClasses() {
        สำหรับ (bool retry = true; retry;) {
            retry = false;
            สำหรับ (auto& itr : m_classesp) {
                ถ้า (AstClass* const nodep = itr) {  // nullptr if deleted earlier
                    ถ้า (nodep->user1() == 0) {
                        ถ้า (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ถ้า (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = nullptr;
                        retry = true;
                    }
                }
            }
        }
    }

    void deadCheckVar() {
        // Delete any unused varscopes
        สำหรับ (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ถ้า (vscp->varp()->user2()) continue;
            ถ้า (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                const std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                สำหรับ (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    AstNodeAssign* const assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ถ้า (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        สำหรับ (bool retry = true; retry;) {
            retry = false;
            สำหรับ (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* const varp = *it;
                ถ้า (!varp) continue;
                ถ้า (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ถ้า (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = true;
                }
            }
        }
        สำหรับ (std::vector<AstNode*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            ถ้า ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ถ้า (const AstNodeUOrStructDType* const classp
                    = VN_CAST((*it), NodeUOrStructDType)) {
                    bool cont = true;
                    สำหรับ (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ถ้า (memberp->user1() != 0) {
                            cont = false;
                            bha_nga;
                        }
                    }
                    ถ้า (!cont) continue;
                }
                deleting(*it);
            }
        }
    }

public:
    // CONSTRUCTORS
    DeadVisitor(AstNetlist* nodep, bool elimUserVars, bool elimDTypes, bool elimScopes,
                bool elimCells, bool elimTasks)
        : m_elimUserVars{elimUserVars}
        , m_elimDTypes{elimDTypes}
        , m_elimCells{elimCells}
        , m_containingFTaskRefp{nodep} {
        // Prepare to remove some datatypes
        nodep->typeTablep()->clearCache();

        // Operate on whole netlist
        iterate(nodep);

        ถ้า (AstVarScope* const vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        สำหรับ (auto& itr : m_dtypePkgsp) {
            ถ้า (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ถ้า (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", false);

        ถ้า (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ถ้า (elimScopes) deadCheckScope();
        ถ้า (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ถ้า (elimTasks) deadCheckDemote();

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

void V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, false, false, false, false, false};
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadModules", 0, dumpTreeEitherLevel() >= 6);
}

void V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, false, true, false, false, true}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadDtypes", 0, dumpTreeEitherLevel() >= 3);
}

void V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, false, true, true, false, false}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadDtypesScoped", 0, dumpTreeEitherLevel() >= 3);
}

void V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, true, true, false, true, true}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadAll", 0, dumpTreeEitherLevel() >= 3);
}

void V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, true, true, true, true, true}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("deadAllScoped", 0, dumpTreeEitherLevel() >= 3);
}
