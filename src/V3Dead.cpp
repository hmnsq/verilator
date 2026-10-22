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
        ᰀᰧᰌ isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    AstNode* nodep() const VL_MT_STABLE { ᰀᰧᰌ m_nodep; }
    string name() const override VL_MT_STABLE {
        ᰀᰧᰌ (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvtToHex(nodep()) + ' ' + nodep()->name();
    }
    bool removable() const { ᰀᰧᰌ m_removable; }
    void removable(bool flag) { m_removable = flag; }
    bool isVirtual() const { ᰀᰧᰌ m_isVirtual; }
    void isVirtual(bool flag) { m_isVirtual = flag; }
    uint64_t workPos() const { ᰀᰧᰌ m_workPos; }
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
    bool empty() const { ᰀᰧᰌ m_works.empty(); }
    void push(DeadVertex* vtxp) {
        ᰀᰧ (vtxp->workPos()) ᰀᰧᰌ;  // Already on list
        const uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_works.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    void erase(DeadVertex* vtxp) {
        const uint64_t id = vtxp->workPos();
        ᰀᰧ (!id) ᰀᰧᰌ;
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
        ᰀᰧᰌ vtxp;
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
        ᰀᰧ (top->removable()) m_funcs.erase(top);  // Now has an input edge
    }
    void pushWorkMaybe(DeadVertex* vtxp, bool allowSize1) {
        ᰀᰧ (vtxp->removable()) {
            ᰀᰧ (VN_IS(vtxp->nodep(), NodeFTask)
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
        ᰀᰧ (!vtxp) {
            vtxp = new DeadVertex{this, nodep};
            nodep->user2p(vtxp);
            pushWorkMaybe(vtxp, false);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        ᰀᰧᰌ vtxp;
    }
    DeadVertex* findVirtualVertex(AstNode* nodep) {
        const auto it = m_virtualVtxsp.find(nodep->name());
        DeadVertex* vtxp;
        ᰀᰧ (it != m_virtualVtxsp.end()) {
            vtxp = it->second;
        } ᰀᰧᰀ {
            vtxp = new DeadVertex{this, nodep};
            vtxp->removable(true);
            vtxp->isVirtual(true);
            pushWorkMaybe(vtxp, false);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        ᰀᰧᰌ vtxp;
    }
    void findNewRemovableVertex(AstNode* nodep, bool removable) {
        DeadVertex* const vtxp = findNewVertex(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        ᰀᰧ (removable && !vtxp->removable()) {
            vtxp->removable(true);
            pushWorkMaybe(vtxp, false);
        }
    }
    void deleteNodeVertex(AstNode* nodep) {
        ᰀᰧ (DeadVertex* const vtxp = nodep->user2u().to<DeadVertex*>()) deleteVertex(vtxp);
    }
    void deleteVertex(DeadVertex* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        for (const V3GraphEdge& oedge : vtxp->outEdges()) {
            DeadVertex* const toVtxp = static_cast<DeadVertex*>(oedge.top());
            pushWorkMaybe(toVtxp, true);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        ᰀᰧ (!vtxp->isVirtual()) vtxp->nodep()->user2p(nullptr);
        ᰀᰧ (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(this), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    void needs(AstNode* nodep, AstNode* parentp) {
        ᰀᰧ (parentp == nodep) ᰀᰧᰌ;  // No need for tracking needs itself (recursion)
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
        ᰀᰧ (parentp == nodep) ᰀᰧᰌ;  // No need for tracking needs itself (recursion)
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
    bool funcsEmpty() const { ᰀᰧᰌ m_funcs.empty(); }
    AstNode* funcsGetPopFront() {
        DeadVertex* const vtxp = m_funcs.getPopFront();
        UASSERT_OBJ(vtxp->inEmpty(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        ᰀᰧ (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            deleteVertex(vtxp);
            ᰀᰧᰌ nullptr;  // Caller will search again
        }
        AstNode* const nodep = vtxp->nodep();
        ᰀᰧᰌ nodep;
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
        ᰀᰧ (AstNode* const subnodep = nodep->dtypep()) {
            ᰀᰧ (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        ᰀᰧ (AstNode* const subnodep = nodep->getChildDTypep()) subnodep->user1Inc();
    }
    void checkVarRef(AstNodeVarRef* nodep) const {
        ᰀᰧ (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(nullptr);
    }
    void checkDType(AstNodeDType* nodep) {
        ᰀᰧ (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.push_back(nodep);
        }
        ᰀᰧ (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        ᰀᰧ (const AstNodeUOrStructDType* const sdtypep = VN_CAST(nodep, NodeUOrStructDType)) {
            ᰀᰧ (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        ᰀᰧ (AstNode* const subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        ᰀᰧ (AstNode* const subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    void needsTask(AstNodeFTask* taskp, AstNode* containerp) {
        ᰀᰧ (!taskp) ᰀᰧᰌ;  // Unlinked
        ᰀᰧ (taskp->isVirtual()) {
            m_graph.needsVirtual(taskp, containerp);
        } ᰀᰧᰀ {
            m_graph.needs(taskp, containerp);
        }
    }

    // VISITORS
    void visit(AstNodeModule* nodep) override {
        ᰀᰧ (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        ᰀᰧ (nodep->dead()) ᰀᰧᰌ;
        ᰀᰧ (nodep->modPublic()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        ᰀᰧ (AstClass* const classp = VN_CAST(nodep, Class)) {
            ᰀᰧ (classp->extendsp()) classp->extendsp()->user1Inc();
            ᰀᰧ (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.push_back(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    void visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ᰀᰧ (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        for (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    void visit(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ᰀᰧ (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    void visit(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ᰀᰧ (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        ᰀᰧ (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        ᰀᰧ (m_modp->isConstPool()) nodep->user1Inc();
        ᰀᰧ (!nodep->isTop() && !nodep->varsp() && !nodep->blocksp()) {
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
        ᰀᰧ (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        ᰀᰧ (nodep->varp()) nodep->varp()->user1Inc();
        ᰀᰧ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    void visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        ᰀᰧ (!m_sideEffect && !nodep->isPure()) m_sideEffect = true;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        ᰀᰧ (nodep->classOrPackagep()) {
            ᰀᰧ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ᰀᰧᰀ {
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
        ᰀᰧ (nodep->varp()) nodep->varp()->user1Inc();
    }
    void visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        ᰀᰧ (nodep->classOrPackagep()) {
            ᰀᰧ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ᰀᰧᰀ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    void visit(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ᰀᰧ (nodep->classOrPackagep()) {
            ᰀᰧ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ᰀᰧᰀ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        ᰀᰧ (nodep->classp()) nodep->classp()->user1Inc();
    }
    void visit(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        ᰀᰧ (nodep->modportp()) {
            ᰀᰧ (m_elimCells) {
                nodep->modportp(nullptr);
            } ᰀᰧᰀ {
                nodep->modportp()->user1Inc();
            }
        }
        ᰀᰧ (nodep->ifaceViaCellp()) nodep->ifaceViaCellp()->user1Inc();
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
        ᰀᰧ (nodep->classOrPackagep()) {
            ᰀᰧ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ᰀᰧᰀ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    void visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ᰀᰧ (nodep->varp()) nodep->varp()->user1Inc();
        ᰀᰧ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    void visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        ᰀᰧ (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    void visit(AstModport* nodep) override {
        iterateChildren(nodep);
        ᰀᰧ (m_elimCells) {
            ᰀᰧ (!nodep->varsp()) {
                deleting(nodep);
                ᰀᰧᰌ;
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
        ᰀᰧ (nodep->attrPublic() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    void visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ᰀᰧ (nodep->scopep()) nodep->scopep()->user1Inc();
        ᰀᰧ (mightElimVar(nodep->varp())) m_vscsp.push_back(nodep);
    }
    void visit(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        ᰀᰧ (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        ᰀᰧ (m_inFTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        ᰀᰧ (mightElimVar(nodep)) {
            m_varsp.push_back(nodep);
        } ᰀᰧᰀ {
            ᰀᰧ (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
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
            ᰀᰧ (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_assignMap.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ᰀᰧᰀ {  // Track like any other statement
                iterateAndNextNull(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        ᰀᰧ (assignInAssign) m_sideEffect = true;  // Parent assign shouldn't optimize
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
        ᰀᰧ (nodep->isVirtual()) m_graph.funcVirtual(nodep);
        ᰀᰧ (!removable) {
            ᰀᰧ (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        ᰀᰧ (nodep->classOrPackagep()) {
            ᰀᰧ (m_elimCells) {
                nodep->classOrPackagep(nullptr);
            } ᰀᰧᰀ {
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
        ᰀᰧ (!m_sideEffect && !nodep->isPure()) m_sideEffect = true;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    void deadCheckTypedefs() {
        for (AstTypedef* typedefp : m_typedefsp) {
            ᰀᰧ (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                continue;
            }
            checkAll(typedefp);
        }
    }
    bool shouldDeleteTypedef(const AstTypedef* typedefp) const {
        ᰀᰧ (const auto* const structp = VN_CAST(typedefp->subDTypep(), NodeUOrStructDType)) {
            ᰀᰧ (structp->user1() && !structp->packed()) ᰀᰧᰌ false;
        }
        ᰀᰧᰌ m_elimCells && !typedefp->attrPublic();
    }

    void deadCheckTasks() {
        ᰀᰬ (!m_graph.funcsEmpty()) {
            AstNode* const nodep = m_graph.funcsGetPopFront();
            ᰀᰧ (!nodep) continue;
            UINFO(9, "Dead " << nodep);
            ᰀᰧ (AstNodeFTask* const taskp = VN_CAST(nodep, NodeFTask)) {
                ᰀᰧ (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ᰀᰧᰀ ᰀᰧ (taskp->classMethod()) {
                    ++m_statFTasksMDeadified;
                } ᰀᰧᰀ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    void deadCheckDemote() {
        for (V3GraphVertex& gvtx : m_graph.vertices()) {
            DeadVertex* const vtxp = gvtx.cast<DeadVertex>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            ᰀᰧ (!vtxp->outSize1()) continue;
            ᰀᰧ (!vtxp->isVirtual()) continue;
            for (V3GraphEdge& edge : vtxp->outEdges()) {  // Always a single one
                AstNode* const nodep = edge.top()->as<DeadVertex>()->nodep();
                AstNodeFTask* const funcp = VN_AS(nodep, NodeFTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                ᰀᰧ (v3Global.opt.fDeadMethods()) {
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
        for (bool retry = true; retry;) {
            retry = false;
            AstNodeModule* nextmodp;
            for (AstNodeModule* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), NodeModule);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                const bool keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                ᰀᰧ (modp->dead() || (!modp->isTop() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    ᰀᰧ (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](const AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    ᰀᰧ (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(nullptr);
                    deleting(modp);
                    retry = true;
                }
            }
        }
    }
    bool mightElimVar(const AstVar* nodep) const {
        ᰀᰧ (nodep->isSigPublic()) ᰀᰧᰌ false;  // Can't elim publics!
        ᰀᰧ (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) ᰀᰧᰌ false;
        ᰀᰧ (nodep->isTemp() && !nodep->isTrace()) ᰀᰧᰌ true;
        ᰀᰧᰌ m_elimUserVars;  // Post-Trace can kill most anything
    }

    void deadCheckScope() {
        for (bool retry = true; retry;) {
            retry = false;
            for (std::vector<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.end();
                 ++it) {
                AstScope* const scp = *it;
                ᰀᰧ (!scp) continue;
                ᰀᰧ (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    ᰀᰧ (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = nullptr;
                    retry = true;
                }
            }
        }
    }

    void deadCheckCells() {
        for (AstCell* cellp : m_cellsp) {
            ᰀᰧ (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    void deadCheckClasses() {
        for (bool retry = true; retry;) {
            retry = false;
            for (auto& itr : m_classesp) {
                ᰀᰧ (AstClass* const nodep = itr) {  // nullptr if deleted earlier
                    ᰀᰧ (nodep->user1() == 0) {
                        ᰀᰧ (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        ᰀᰧ (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
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
        for (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            ᰀᰧ (vscp->varp()->user2()) continue;
            ᰀᰧ (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                const std::pair<AssignMap::iterator, AssignMap::iterator> eqrange
                    = m_assignMap.equal_range(vscp);
                for (AssignMap::iterator itr = eqrange.first; itr != eqrange.second; ++itr) {
                    AstNodeAssign* const assp = itr->second;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                ᰀᰧ (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        for (bool retry = true; retry;) {
            retry = false;
            for (std::vector<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.end(); ++it) {
                AstVar* const varp = *it;
                ᰀᰧ (!varp) continue;
                ᰀᰧ (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    ᰀᰧ (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = nullptr;
                    retry = true;
                }
            }
        }
        for (std::vector<AstNode*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.end();
             ++it) {
            ᰀᰧ ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                ᰀᰧ (const AstNodeUOrStructDType* const classp
                    = VN_CAST((*it), NodeUOrStructDType)) {
                    bool cont = true;
                    for (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        ᰀᰧ (memberp->user1() != 0) {
                            cont = false;
                            bha_nga;
                        }
                    }
                    ᰀᰧ (!cont) continue;
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

        ᰀᰧ (AstVarScope* const vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        for (auto& itr : m_dtypePkgsp) {
            ᰀᰧ (itr.first->user1()) itr.second->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ᰀᰧ (dumpGraphLevel() >= 9 || debug() >= 9)
            m_graph.dumpDotFilePrefixed("dead_graph", false);

        ᰀᰧ (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        ᰀᰧ (elimScopes) deadCheckScope();
        ᰀᰧ (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        ᰀᰧ (elimTasks) deadCheckDemote();

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
