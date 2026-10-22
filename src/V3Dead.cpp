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

صنف Dead_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Dead_راس, V3_گراف_راس)
    Ast_ڳنڍ* ثابت m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    منطقي m_removable = خطأ;  // Subject to dead removal
    منطقي m_isVirtual = خطأ;  // Virtual node, indirection

عام:
    Dead_راس(V3_گراف* graphp, Ast_ڳنڍ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Dead_راس() override = افتراضي;
    string dotShape() ثابت override {
        أرجع isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_ڳنڍ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    string name() ثابت override VL_MT_STABLE {
        أرجع (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvt_منزل_Hex(nodep()) + ' ' + nodep()->name();
    }
    منطقي removable() ثابت { أرجع m_removable; }
    فراغ removable(منطقي flag) { m_removable = flag; }
    منطقي isVirtual() ثابت { أرجع m_isVirtual; }
    فراغ isVirtual(منطقي flag) { m_isVirtual = flag; }
    uint64_t workPos() ثابت { أرجع m_workPos; }
    فراغ workPos(uint64_t قدر) { m_workPos = قدر; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
صنف DeadWorkList final {
    // MEMBERS
    ساكن uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, Dead_راس*> m_هلي;  // Work list by sequence
عام:
    // METHODS
    منطقي خالي_جايون() ثابت { أرجع m_هلي.خالي_جايون(); }
    فراغ ڌڪو(Dead_راس* vtxp) {
        إذا (vtxp->workPos()) أرجع;  // Already on list
        ثابت uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_هلي.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    فراغ erase(Dead_راس* vtxp) {
        ثابت uint64_t id = vtxp->workPos();
        إذا (!id) أرجع;
        UINFO(9, "Worklist.erase " << vtxp);
        ثابت خودڪار it = m_هلي.ڳوليو(id);
        UASSERT_OBJ(it != m_هلي.پڇاڙي(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_هلي.erase(it);
        vtxp->workPos(0);
    }
    Dead_راس* get_ڪڍو_Front() {
        UDEBUGONLY(UASSERT(!خالي_جايون(), "Front invalid to call on empty list"););
        ثابت خودڪار it = m_هلي.begin();
        Dead_راس* ثابت vtxp = it->ٻيو;
        m_هلي.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        أرجع vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

صنف Dead_گراف final : عام V3_گراف {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, Dead_راس*> m_virtualVtxsp;

    فراغ new_ڪنارو(Dead_راس* fromp, Dead_راس* چوٽي) {
        جديد V3_گراف_ڪنارو{هذا, fromp, چوٽي, 1, خطأ};
        إذا (چوٽي->removable()) m_funcs.erase(چوٽي);  // Now has an input edge
    }
    فراغ ڌڪو_Work_Maybe(Dead_راس* vtxp, منطقي allowSize1) {
        إذا (vtxp->removable()) {
            إذا (VN_IS(vtxp->nodep(), ڳنڍ_FTask)
                && (vtxp->اندريون_خالي_جايون() || (allowSize1 && vtxp->اندريون_Size1())))
                m_funcs.ڌڪو(vtxp);
        }
    }

عام:
    // METHODS
    Dead_گراف() = افتراضي;
    ~Dead_گراف() override = افتراضي;
    Dead_راس* ڳوليو_New_راس(Ast_ڳنڍ* nodep) {
        Dead_راس* vtxp = nodep->user2u().منزل<Dead_راس*>();
        إذا (!vtxp) {
            vtxp = جديد Dead_راس{هذا, nodep};
            nodep->user2p(vtxp);
            ڌڪو_Work_Maybe(vtxp, خطأ);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        أرجع vtxp;
    }
    Dead_راس* ڳوليو_Virtual_راس(Ast_ڳنڍ* nodep) {
        ثابت خودڪار it = m_virtualVtxsp.ڳوليو(nodep->name());
        Dead_راس* vtxp;
        إذا (it != m_virtualVtxsp.پڇاڙي()) {
            vtxp = it->ٻيو;
        } وإلا {
            vtxp = جديد Dead_راس{هذا, nodep};
            vtxp->removable(سچ);
            vtxp->isVirtual(سچ);
            ڌڪو_Work_Maybe(vtxp, خطأ);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        أرجع vtxp;
    }
    فراغ ڳوليو_New_Removable_راس(Ast_ڳنڍ* nodep, منطقي removable) {
        Dead_راس* ثابت vtxp = ڳوليو_New_راس(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        إذا (removable && !vtxp->removable()) {
            vtxp->removable(سچ);
            ڌڪو_Work_Maybe(vtxp, خطأ);
        }
    }
    فراغ delete_ڳنڍ_راس(Ast_ڳنڍ* nodep) {
        إذا (Dead_راس* ثابت vtxp = nodep->user2u().منزل<Dead_راس*>()) delete_راس(vtxp);
    }
    فراغ delete_راس(Dead_راس* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        لكل (ثابت V3_گراف_ڪنارو& oedge : vtxp->ٻاهريون_ڪنارا()) {
            Dead_راس* ثابت منزل_Vtxp = static_cast<Dead_راس*>(oedge.چوٽي());
            ڌڪو_Work_Maybe(منزل_Vtxp, سچ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        إذا (!vtxp->isVirtual()) vtxp->nodep()->user2p(خالي_اشارو);
        إذا (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(هذا), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    فراغ needs(Ast_ڳنڍ* nodep, Ast_ڳنڍ* parentp) {
        إذا (parentp == nodep) أرجع;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        Dead_راس* ثابت والد_Vtxp = ڳوليو_New_راس(parentp);
        Dead_راس* ثابت ڳنڍ_Vtxp = ڳوليو_New_راس(nodep);
        UINFO(9, "Edge need " << والد_Vtxp << " -> " << ڳنڍ_Vtxp);
        new_ڪنارو(والد_Vtxp, ڳنڍ_Vtxp);
    }
    فراغ needsVirtual(Ast_ڳنڍ_FTask* nodep, Ast_ڳنڍ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        إذا (parentp == nodep) أرجع;  // No need for tracking needs itself (recursion)
        Dead_راس* ثابت والد_Vtxp = ڳوليو_New_راس(parentp);
        Dead_راس* ثابت virtualVtxp = ڳوليو_Virtual_راس(nodep);
        UINFO(9, "Edge needVirtual " << والد_Vtxp << " -> " << virtualVtxp);
        new_ڪنارو(والد_Vtxp, virtualVtxp);
    }
    فراغ funcVirtual(Ast_ڳنڍ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        Dead_راس* ثابت virtualVtxp = ڳوليو_Virtual_راس(nodep);
        Dead_راس* ثابت ڳنڍ_Vtxp = ڳوليو_New_راس(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << ڳنڍ_Vtxp);
        new_ڪنارو(virtualVtxp, ڳنڍ_Vtxp);
    }
    منطقي funcs_خالي_جايون() ثابت { أرجع m_funcs.خالي_جايون(); }
    Ast_ڳنڍ* funcs_Get_ڪڍو_Front() {
        Dead_راس* ثابت vtxp = m_funcs.get_ڪڍو_Front();
        UASSERT_OBJ(vtxp->اندريون_خالي_جايون(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        إذا (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            delete_راس(vtxp);
            أرجع خالي_اشارو;  // Caller will search again
        }
        Ast_ڳنڍ* ثابت nodep = vtxp->nodep();
        أرجع nodep;
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
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;  // For usage information see DeadGraph

    // TYPES
    استعمال سونپيو_Map = std::multimap<AstVarScope*, Ast_ڳنڍ_سونپيو*>;

    // STATE - across all visitors
    ثابت منطقي m_elimUserVars;  // Allow removal of user's vars
    ثابت منطقي m_elimDTypes;  // Allow removal of DTypes
    ثابت منطقي m_elimCells;  // Allow removal of Cells

    Dead_گراف m_گراف;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::ويڪٽر<AstVar*> m_varsp;
    std::ويڪٽر<Ast_ڳنڍ*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_ڳنڍ_DType*, Ast_ڳنڍ_Module*> m_dtypePkgsp;  // Data type's containing package
    std::ويڪٽر<AstVarScope*> m_vscsp;
    std::ويڪٽر<AstScope*> m_scopesp;
    std::ويڪٽر<AstCell*> m_cellsp;
    std::ويڪٽر<AstClass*> m_classesp;
    std::ويڪٽر<AstTypedef*> m_typedefsp;
    سونپيو_Map m_سونپيو_Map;  // List of all simple assignments for each variable
    منطقي m_sideEffect = خطأ;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_اندريون_سونپيو = خطأ;  // Currently in an assign
    Ast_ڳنڍ_DType* m_curDTypep = خالي_اشارو;  // Current NodeDType
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    Ast_Foreach_سرو* m_foreachHeaderp = خالي_اشارو;  // Current foreach header
    Ast_ڳنڍ* m_containingFTaskRefp = خالي_اشارو;  // Parent of ftaskref (e.g. task/module)
    منطقي m_اندريون_FTask = خطأ;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    فراغ deleting(Ast_ڳنڍ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_گراف.delete_ڳنڍ_راس(nodep);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    فراغ checkAll(Ast_ڳنڍ* nodep) {
        إذا (Ast_ڳنڍ* ثابت subnodep = nodep->dtypep()) {
            إذا (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        إذا (Ast_ڳنڍ* ثابت subnodep = nodep->get_ٻار_DTypep()) subnodep->user1Inc();
    }
    فراغ checkVarRef(Ast_ڳنڍ_Var_Ref* nodep) ثابت {
        إذا (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(خالي_اشارو);
    }
    فراغ checkDType(Ast_ڳنڍ_DType* nodep) {
        إذا (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ڌڪو_پٺ(nodep);
        }
        إذا (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(nodep, ڳنڍ_UOr_Struct_DType)) {
            إذا (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        إذا (Ast_ڳنڍ* ثابت subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        إذا (Ast_ڳنڍ* ثابت subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    فراغ needsTask(Ast_ڳنڍ_FTask* taskp, Ast_ڳنڍ* containerp) {
        إذا (!taskp) أرجع;  // Unlinked
        إذا (taskp->isVirtual()) {
            m_گراف.needsVirtual(taskp, containerp);
        } وإلا {
            m_گراف.needs(taskp, containerp);
        }
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        إذا (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        إذا (nodep->dead()) أرجع;
        إذا (nodep->mod_عوامي()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            إذا (classp->extendsp()) classp->extendsp()->user1Inc();
            إذا (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ڌڪو_پٺ(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    فراغ گهمو(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        لكل (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    فراغ گهمو(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    فراغ گهمو(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        إذا (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        إذا (m_modp->isConstPool()) nodep->user1Inc();
        إذا (!nodep->is_چوٽي() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ڌڪو_پٺ(nodep);
        }
    }
    فراغ گهمو(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ڌڪو_پٺ(nodep);
        nodep->modp()->user1Inc();
    }

    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        // Note NodeAssign skips calling this in some cases
        iterateChildren(nodep);
        checkAll(nodep);
        checkVarRef(nodep);
        إذا (nodep->varScopep()) {
            nodep->varScopep()->user1Inc();
            nodep->varScopep()->varp()->user1Inc();
        }
        إذا (nodep->varp()) nodep->varp()->user1Inc();
        إذا (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc();
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (!m_sideEffect && !nodep->is_خالص()) m_sideEffect = سچ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالي_اشارو);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    فراغ گهمو(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    فراغ گهمو(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->varp()) nodep->varp()->user1Inc();
    }
    فراغ گهمو(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالي_اشارو);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    فراغ گهمو(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالي_اشارو);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        إذا (nodep->classp()) nodep->classp()->user1Inc();
    }
    فراغ گهمو(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        إذا (nodep->modportp()) {
            إذا (m_elimCells) {
                nodep->modportp(خالي_اشارو);
            } وإلا {
                nodep->modportp()->user1Inc();
            }
        }
        إذا (nodep->iface_ذريعي_Cellp()) nodep->iface_ذريعي_Cellp()->user1Inc();
    }
    فراغ گهمو(Ast_ڳنڍ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    فراغ گهمو(Ast_Enum_شي_Ref* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالي_اشارو);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->varp()) nodep->varp()->user1Inc();
        إذا (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    فراغ گهمو(AstStructSel* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    فراغ گهمو(AstModport* nodep) override {
        iterateChildren(nodep);
        إذا (m_elimCells) {
            إذا (!nodep->varsp()) {
                deleting(nodep);
                أرجع;
            }
        }
        checkAll(nodep);
    }
    فراغ گهمو(Ast_Foreach_سرو* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    فراغ گهمو(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ڌڪو_پٺ(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        إذا (nodep->attr_عوامي() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    فراغ گهمو(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->scopep()) nodep->scopep()->user1Inc();
        إذا (mightElimVar(nodep->varp())) m_vscsp.ڌڪو_پٺ(nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        إذا (m_اندريون_FTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        إذا (mightElimVar(nodep)) {
            m_varsp.ڌڪو_پٺ(nodep);
        } وإلا {
            إذا (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ثابت منطقي سونپيو_اندريون_سونپيو = m_اندريون_سونپيو;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_اندريون_سونپيو);
            VL_RESTORER(m_sideEffect);
            m_اندريون_سونپيو = سچ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_ايندڙ_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
            إذا (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_سونپيو_Map.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } وإلا {  // Track like any other statement
                iterate_And_ايندڙ_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        إذا (سونپيو_اندريون_سونپيو) m_sideEffect = سچ;  // Parent assign shouldn't optimize
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        ثابت منطقي removable = !(nodep->task_عوامي() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->class_طريقو()));
        m_گراف.ڳوليو_New_Removable_راس(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_اندريون_FTask);
        m_containingFTaskRefp = nodep;
        m_اندريون_FTask = سچ;
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->isVirtual()) m_گراف.funcVirtual(nodep);
        إذا (!removable) {
            إذا (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالي_اشارو);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    فراغ گهمو(Ast_Clocking_شي* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (!m_sideEffect && !nodep->is_خالص()) m_sideEffect = سچ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    فراغ deadCheckTypedefs() {
        لكل (AstTypedef* typedefp : m_typedefsp) {
            إذا (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                استمر;
            }
            checkAll(typedefp);
        }
    }
    منطقي shouldDeleteTypedef(ثابت AstTypedef* typedefp) ثابت {
        إذا (ثابت خودڪار* ثابت structp = VN_CAST(typedefp->subDTypep(), ڳنڍ_UOr_Struct_DType)) {
            إذا (structp->user1() && !structp->packed()) أرجع خطأ;
        }
        أرجع m_elimCells && !typedefp->attr_عوامي();
    }

    فراغ deadCheckTasks() {
        طالما (!m_گراف.funcs_خالي_جايون()) {
            Ast_ڳنڍ* ثابت nodep = m_گراف.funcs_Get_ڪڍو_Front();
            إذا (!nodep) استمر;
            UINFO(9, "Dead " << nodep);
            إذا (Ast_ڳنڍ_FTask* ثابت taskp = VN_CAST(nodep, ڳنڍ_FTask)) {
                إذا (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } وإلا إذا (taskp->class_طريقو()) {
                    ++m_statFTasksMDeadified;
                } وإلا {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    فراغ deadCheckDemote() {
        لكل (V3_گراف_راس& gvtx : m_گراف.vertices()) {
            Dead_راس* ثابت vtxp = gvtx.cast<Dead_راس>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            إذا (!vtxp->ٻاهريون_Size1()) استمر;
            إذا (!vtxp->isVirtual()) استمر;
            لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->ٻاهريون_ڪنارا()) {  // Always a single one
                Ast_ڳنڍ* ثابت nodep = ڪنارو.چوٽي()->as<Dead_راس>()->nodep();
                Ast_ڳنڍ_FTask* ثابت funcp = VN_AS(nodep, ڳنڍ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                إذا (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(خطأ);
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
        لكل (منطقي retry = سچ; retry;) {
            retry = خطأ;
            Ast_ڳنڍ_Module* nextmodp;
            لكل (Ast_ڳنڍ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), ڳنڍ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ثابت منطقي keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                إذا (modp->dead() || (!modp->is_چوٽي() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    إذا (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ثابت AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    إذا (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(خالي_اشارو);
                    deleting(modp);
                    retry = سچ;
                }
            }
        }
    }
    منطقي mightElimVar(ثابت AstVar* nodep) ثابت {
        إذا (nodep->is_Sig_عوامي()) أرجع خطأ;  // Can't elim publics!
        إذا (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) أرجع خطأ;
        إذا (nodep->is_عارضي() && !nodep->isTrace()) أرجع سچ;
        أرجع m_elimUserVars;  // Post-Trace can kill most anything
    }

    فراغ deadCheckScope() {
        لكل (منطقي retry = سچ; retry;) {
            retry = خطأ;
            لكل (std::ويڪٽر<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.پڇاڙي();
                 ++it) {
                AstScope* ثابت scp = *it;
                إذا (!scp) استمر;
                إذا (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    إذا (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = خالي_اشارو;
                    retry = سچ;
                }
            }
        }
    }

    فراغ deadCheckCells() {
        لكل (AstCell* cellp : m_cellsp) {
            إذا (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    فراغ deadCheckClasses() {
        لكل (منطقي retry = سچ; retry;) {
            retry = خطأ;
            لكل (خودڪار& itr : m_classesp) {
                إذا (AstClass* ثابت nodep = itr) {  // nullptr if deleted earlier
                    إذا (nodep->user1() == 0) {
                        إذا (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        إذا (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = خالي_اشارو;
                        retry = سچ;
                    }
                }
            }
        }
    }

    فراغ deadCheckVar() {
        // Delete any unused varscopes
        لكل (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            إذا (vscp->varp()->user2()) استمر;
            إذا (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ثابت std::pair<سونپيو_Map::iterator, سونپيو_Map::iterator> eqrange
                    = m_سونپيو_Map.equal_range(vscp);
                لكل (سونپيو_Map::iterator itr = eqrange.پهريون; itr != eqrange.ٻيو; ++itr) {
                    Ast_ڳنڍ_سونپيو* ثابت assp = itr->ٻيو;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                إذا (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        لكل (منطقي retry = سچ; retry;) {
            retry = خطأ;
            لكل (std::ويڪٽر<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.پڇاڙي(); ++it) {
                AstVar* ثابت varp = *it;
                إذا (!varp) استمر;
                إذا (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    إذا (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = خالي_اشارو;
                    retry = سچ;
                }
            }
        }
        لكل (std::ويڪٽر<Ast_ڳنڍ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.پڇاڙي();
             ++it) {
            إذا ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت classp
                    = VN_CAST((*it), ڳنڍ_UOr_Struct_DType)) {
                    منطقي cont = سچ;
                    لكل (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        إذا (memberp->user1() != 0) {
                            cont = خطأ;
                            اكسر;
                        }
                    }
                    إذا (!cont) استمر;
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

        إذا (AstVarScope* ثابت vscp = nodep->dpiExportTriggerp()) {
            vscp->user1Inc();
            vscp->varp()->user1Inc();
        }

        // If data type has a reference in another package, then keep defining package around
        لكل (خودڪار& itr : m_dtypePkgsp) {
            إذا (itr.پهريون->user1()) itr.ٻيو->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_گراف.remove_Redundant_ڪنارا_Max(&V3_گراف_ڪنارو::followAlwaysTrue);
        إذا (dump_گراف_Level() >= 9 || debug() >= 9)
            m_گراف.dumpDotFilePrefixed("dead_graph", خطأ);

        إذا (elimTasks) deadCheckTasks();
        deadCheckTypedefs();
        deadCheckVar();
        // We only eliminate scopes when in a flattened structure
        // Otherwise we have no easy way to know if a scope is used
        إذا (elimScopes) deadCheckScope();
        إذا (elimCells) deadCheckCells();
        deadCheckClasses();
        // Modules after vars, because might be vars we delete inside a mod we delete
        deadCheckMod();

        // After deleting as much as can, demote some virtual functions
        إذا (elimTasks) deadCheckDemote();

        // We may have removed some datatypes, cleanup
        nodep->typeTablep()->repairCache();
        VIsCached::clear_Cache_وڻ();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::جوڙيو_Stat_جوڙ("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

فراغ V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, خطأ, خطأ, خطأ, خطأ, خطأ};
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("deadModules", 0, dump_وڻ_Either_Level() >= 6);
}

فراغ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, خطأ, سچ, خطأ, خطأ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("deadDtypes", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, خطأ, سچ, سچ, خطأ, خطأ}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("deadDtypesScoped", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, سچ, سچ, خطأ, سچ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("deadAll", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, سچ, سچ, سچ, سچ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("deadAllScoped", 0, dump_وڻ_Either_Level() >= 3);
}
