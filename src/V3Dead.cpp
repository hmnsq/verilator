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
    Ast_گرہ* ثابت m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    منطقي m_removable = خطأ;  // Subject to dead removal
    منطقي m_isVirtual = خطأ;  // Virtual node, indirection

عام:
    Dead_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Dead_راس() override = افتراضي;
    string dotShape() ثابت override {
        أرجع isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_گرہ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
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
    std::map<uint64_t, Dead_راس*> m_چلدا;  // Work list by sequence
عام:
    // METHODS
    منطقي خالی_تھاواں() ثابت { أرجع m_چلدا.خالی_تھاواں(); }
    فراغ ڈالو(Dead_راس* vtxp) {
        إذا (vtxp->workPos()) أرجع;  // Already on list
        ثابت uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_چلدا.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    فراغ erase(Dead_راس* vtxp) {
        ثابت uint64_t id = vtxp->workPos();
        إذا (!id) أرجع;
        UINFO(9, "Worklist.erase " << vtxp);
        ثابت خودکار it = m_چلدا.لبھو(id);
        UASSERT_OBJ(it != m_چلدا.سرا(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_چلدا.erase(it);
        vtxp->workPos(0);
    }
    Dead_راس* get_نکالو_Front() {
        UDEBUGONLY(UASSERT(!خالی_تھاواں(), "Front invalid to call on empty list"););
        ثابت خودکار it = m_چلدا.begin();
        Dead_راس* ثابت vtxp = it->دوجا;
        m_چلدا.erase(it);
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

    فراغ new_کنارا(Dead_راس* fromp, Dead_راس* چوٹی) {
        جديد V3_گراف_کنارا{هذا, fromp, چوٹی, 1, خطأ};
        إذا (چوٹی->removable()) m_funcs.erase(چوٹی);  // Now has an input edge
    }
    فراغ ڈالو_Work_Maybe(Dead_راس* vtxp, منطقي allowSize1) {
        إذا (vtxp->removable()) {
            إذا (VN_IS(vtxp->nodep(), گرہ_FTask)
                && (vtxp->وچ_خالی_تھاواں() || (allowSize1 && vtxp->وچ_Size1())))
                m_funcs.ڈالو(vtxp);
        }
    }

عام:
    // METHODS
    Dead_گراف() = افتراضي;
    ~Dead_گراف() override = افتراضي;
    Dead_راس* لبھو_New_راس(Ast_گرہ* nodep) {
        Dead_راس* vtxp = nodep->user2u().منزل<Dead_راس*>();
        إذا (!vtxp) {
            vtxp = جديد Dead_راس{هذا, nodep};
            nodep->user2p(vtxp);
            ڈالو_Work_Maybe(vtxp, خطأ);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        أرجع vtxp;
    }
    Dead_راس* لبھو_Virtual_راس(Ast_گرہ* nodep) {
        ثابت خودکار it = m_virtualVtxsp.لبھو(nodep->name());
        Dead_راس* vtxp;
        إذا (it != m_virtualVtxsp.سرا()) {
            vtxp = it->دوجا;
        } وإلا {
            vtxp = جديد Dead_راس{هذا, nodep};
            vtxp->removable(سچ);
            vtxp->isVirtual(سچ);
            ڈالو_Work_Maybe(vtxp, خطأ);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        أرجع vtxp;
    }
    فراغ لبھو_New_Removable_راس(Ast_گرہ* nodep, منطقي removable) {
        Dead_راس* ثابت vtxp = لبھو_New_راس(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        إذا (removable && !vtxp->removable()) {
            vtxp->removable(سچ);
            ڈالو_Work_Maybe(vtxp, خطأ);
        }
    }
    فراغ delete_گرہ_راس(Ast_گرہ* nodep) {
        إذا (Dead_راس* ثابت vtxp = nodep->user2u().منزل<Dead_راس*>()) delete_راس(vtxp);
    }
    فراغ delete_راس(Dead_راس* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        لكل (ثابت V3_گراف_کنارا& oedge : vtxp->باہرلا_کنارے()) {
            Dead_راس* ثابت منزل_Vtxp = static_cast<Dead_راس*>(oedge.چوٹی());
            ڈالو_Work_Maybe(منزل_Vtxp, سچ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        إذا (!vtxp->isVirtual()) vtxp->nodep()->user2p(خالی_سوچک);
        إذا (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(هذا), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    فراغ needs(Ast_گرہ* nodep, Ast_گرہ* parentp) {
        إذا (parentp == nodep) أرجع;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        Dead_راس* ثابت ماپا_Vtxp = لبھو_New_راس(parentp);
        Dead_راس* ثابت گرہ_Vtxp = لبھو_New_راس(nodep);
        UINFO(9, "Edge need " << ماپا_Vtxp << " -> " << گرہ_Vtxp);
        new_کنارا(ماپا_Vtxp, گرہ_Vtxp);
    }
    فراغ needsVirtual(Ast_گرہ_FTask* nodep, Ast_گرہ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        إذا (parentp == nodep) أرجع;  // No need for tracking needs itself (recursion)
        Dead_راس* ثابت ماپا_Vtxp = لبھو_New_راس(parentp);
        Dead_راس* ثابت virtualVtxp = لبھو_Virtual_راس(nodep);
        UINFO(9, "Edge needVirtual " << ماپا_Vtxp << " -> " << virtualVtxp);
        new_کنارا(ماپا_Vtxp, virtualVtxp);
    }
    فراغ funcVirtual(Ast_گرہ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        Dead_راس* ثابت virtualVtxp = لبھو_Virtual_راس(nodep);
        Dead_راس* ثابت گرہ_Vtxp = لبھو_New_راس(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << گرہ_Vtxp);
        new_کنارا(virtualVtxp, گرہ_Vtxp);
    }
    منطقي funcs_خالی_تھاواں() ثابت { أرجع m_funcs.خالی_تھاواں(); }
    Ast_گرہ* funcs_Get_نکالو_Front() {
        Dead_راس* ثابت vtxp = m_funcs.get_نکالو_Front();
        UASSERT_OBJ(vtxp->وچ_خالی_تھاواں(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        إذا (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            delete_راس(vtxp);
            أرجع خالی_سوچک;  // Caller will search again
        }
        Ast_گرہ* ثابت nodep = vtxp->nodep();
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
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;  // For usage information see DeadGraph

    // TYPES
    ورتوں سونپو_Map = std::multimap<AstVarScope*, Ast_گرہ_سونپو*>;

    // STATE - across all visitors
    ثابت منطقي m_elimUserVars;  // Allow removal of user's vars
    ثابت منطقي m_elimDTypes;  // Allow removal of DTypes
    ثابت منطقي m_elimCells;  // Allow removal of Cells

    Dead_گراف m_گراف;  // Tracking graph

    // List of all encountered to avoid another loop through tree
    std::سمتیہ<AstVar*> m_varsp;
    std::سمتیہ<Ast_گرہ*> m_dtypeElimsp;  // Data types might eliminate
    std::map<Ast_گرہ_DType*, Ast_گرہ_Module*> m_dtypePkgsp;  // Data type's containing package
    std::سمتیہ<AstVarScope*> m_vscsp;
    std::سمتیہ<AstScope*> m_scopesp;
    std::سمتیہ<AstCell*> m_cellsp;
    std::سمتیہ<AstClass*> m_classesp;
    std::سمتیہ<AstTypedef*> m_typedefsp;
    سونپو_Map m_سونپو_Map;  // List of all simple assignments for each variable
    منطقي m_sideEffect = خطأ;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_وچ_سونپو = خطأ;  // Currently in an assign
    Ast_گرہ_DType* m_curDTypep = خالی_سوچک;  // Current NodeDType
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    Ast_Foreach_سرخی* m_foreachHeaderp = خالی_سوچک;  // Current foreach header
    Ast_گرہ* m_containingFTaskRefp = خالی_سوچک;  // Parent of ftaskref (e.g. task/module)
    منطقي m_وچ_FTask = خطأ;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    فراغ deleting(Ast_گرہ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_گراف.delete_گرہ_راس(nodep);
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    فراغ checkAll(Ast_گرہ* nodep) {
        إذا (Ast_گرہ* ثابت subnodep = nodep->dtypep()) {
            إذا (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        إذا (Ast_گرہ* ثابت subnodep = nodep->get_بچہ_DTypep()) subnodep->user1Inc();
    }
    فراغ checkVarRef(Ast_گرہ_Var_Ref* nodep) ثابت {
        إذا (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(خالی_سوچک);
    }
    فراغ checkDType(Ast_گرہ_DType* nodep) {
        إذا (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ڈالو_پٹھ(nodep);
        }
        إذا (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(nodep, گرہ_UOr_Struct_DType)) {
            إذا (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        إذا (Ast_گرہ* ثابت subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        إذا (Ast_گرہ* ثابت subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    فراغ needsTask(Ast_گرہ_FTask* taskp, Ast_گرہ* containerp) {
        إذا (!taskp) أرجع;  // Unlinked
        إذا (taskp->isVirtual()) {
            m_گراف.needsVirtual(taskp, containerp);
        } وإلا {
            m_گراف.needs(taskp, containerp);
        }
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        إذا (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        إذا (nodep->dead()) أرجع;
        إذا (nodep->mod_عوامی()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            إذا (classp->extendsp()) classp->extendsp()->user1Inc();
            إذا (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ڈالو_پٹھ(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        لكل (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    فراغ ویکھو(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    فراغ ویکھو(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        إذا (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        إذا (m_modp->isConstPool()) nodep->user1Inc();
        إذا (!nodep->is_چوٹی() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ڈالو_پٹھ(nodep);
        }
    }
    فراغ ویکھو(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ڈالو_پٹھ(nodep);
        nodep->modp()->user1Inc();
    }

    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
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
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        إذا (!m_sideEffect && !nodep->is_خالص()) m_sideEffect = سچ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالی_سوچک);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    فراغ ویکھو(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    فراغ ویکھو(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->varp()) nodep->varp()->user1Inc();
    }
    فراغ ویکھو(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالی_سوچک);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    فراغ ویکھو(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالی_سوچک);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        إذا (nodep->classp()) nodep->classp()->user1Inc();
    }
    فراغ ویکھو(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        إذا (nodep->modportp()) {
            إذا (m_elimCells) {
                nodep->modportp(خالی_سوچک);
            } وإلا {
                nodep->modportp()->user1Inc();
            }
        }
        إذا (nodep->iface_راہیں_Cellp()) nodep->iface_راہیں_Cellp()->user1Inc();
    }
    فراغ ویکھو(Ast_گرہ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    فراغ ویکھو(Ast_Enum_شے_Ref* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالی_سوچک);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->varp()) nodep->varp()->user1Inc();
        إذا (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    فراغ ویکھو(AstStructSel* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    فراغ ویکھو(AstModport* nodep) override {
        iterateChildren(nodep);
        إذا (m_elimCells) {
            إذا (!nodep->varsp()) {
                deleting(nodep);
                أرجع;
            }
        }
        checkAll(nodep);
    }
    فراغ ویکھو(Ast_Foreach_سرخی* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    فراغ ویکھو(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ڈالو_پٹھ(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        إذا (nodep->attr_عوامی() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    فراغ ویکھو(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->scopep()) nodep->scopep()->user1Inc();
        إذا (mightElimVar(nodep->varp())) m_vscsp.ڈالو_پٹھ(nodep);
    }
    فراغ ویکھو(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        إذا (m_وچ_FTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        إذا (mightElimVar(nodep)) {
            m_varsp.ڈالو_پٹھ(nodep);
        } وإلا {
            إذا (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ثابت منطقي سونپو_وچ_سونپو = m_وچ_سونپو;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_وچ_سونپو);
            VL_RESTORER(m_sideEffect);
            m_وچ_سونپو = سچ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_اگلا_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
            إذا (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_سونپو_Map.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } وإلا {  // Track like any other statement
                iterate_And_اگلا_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        إذا (سونپو_وچ_سونپو) m_sideEffect = سچ;  // Parent assign shouldn't optimize
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        ثابت منطقي removable = !(nodep->task_عوامی() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->class_طریقہ()));
        m_گراف.لبھو_New_Removable_راس(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_وچ_FTask);
        m_containingFTaskRefp = nodep;
        m_وچ_FTask = سچ;
        iterateChildren(nodep);
        checkAll(nodep);
        إذا (nodep->isVirtual()) m_گراف.funcVirtual(nodep);
        إذا (!removable) {
            إذا (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        إذا (nodep->classOrPackagep()) {
            إذا (m_elimCells) {
                nodep->classOrPackagep(خالی_سوچک);
            } وإلا {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    فراغ ویکھو(Ast_Clocking_شے* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
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
        إذا (ثابت خودکار* ثابت structp = VN_CAST(typedefp->subDTypep(), گرہ_UOr_Struct_DType)) {
            إذا (structp->user1() && !structp->packed()) أرجع خطأ;
        }
        أرجع m_elimCells && !typedefp->attr_عوامی();
    }

    فراغ deadCheckTasks() {
        طالما (!m_گراف.funcs_خالی_تھاواں()) {
            Ast_گرہ* ثابت nodep = m_گراف.funcs_Get_نکالو_Front();
            إذا (!nodep) استمر;
            UINFO(9, "Dead " << nodep);
            إذا (Ast_گرہ_FTask* ثابت taskp = VN_CAST(nodep, گرہ_FTask)) {
                إذا (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } وإلا إذا (taskp->class_طریقہ()) {
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
            إذا (!vtxp->باہرلا_Size1()) استمر;
            إذا (!vtxp->isVirtual()) استمر;
            لكل (V3_گراف_کنارا& کنارا : vtxp->باہرلا_کنارے()) {  // Always a single one
                Ast_گرہ* ثابت nodep = کنارا.چوٹی()->as<Dead_راس>()->nodep();
                Ast_گرہ_FTask* ثابت funcp = VN_AS(nodep, گرہ_FTask);
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
            Ast_گرہ_Module* nextmodp;
            لكل (Ast_گرہ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), گرہ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ثابت منطقي keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                إذا (modp->dead() || (!modp->is_چوٹی() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    إذا (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ثابت AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    إذا (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(خالی_سوچک);
                    deleting(modp);
                    retry = سچ;
                }
            }
        }
    }
    منطقي mightElimVar(ثابت AstVar* nodep) ثابت {
        إذا (nodep->is_Sig_عوامی()) أرجع خطأ;  // Can't elim publics!
        إذا (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) أرجع خطأ;
        إذا (nodep->is_عارضی() && !nodep->isTrace()) أرجع سچ;
        أرجع m_elimUserVars;  // Post-Trace can kill most anything
    }

    فراغ deadCheckScope() {
        لكل (منطقي retry = سچ; retry;) {
            retry = خطأ;
            لكل (std::سمتیہ<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.سرا();
                 ++it) {
                AstScope* ثابت scp = *it;
                إذا (!scp) استمر;
                إذا (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    إذا (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = خالی_سوچک;
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
            لكل (خودکار& itr : m_classesp) {
                إذا (AstClass* ثابت nodep = itr) {  // nullptr if deleted earlier
                    إذا (nodep->user1() == 0) {
                        إذا (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        إذا (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = خالی_سوچک;
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
                ثابت std::pair<سونپو_Map::iterator, سونپو_Map::iterator> eqrange
                    = m_سونپو_Map.equal_range(vscp);
                لكل (سونپو_Map::iterator itr = eqrange.پہلا; itr != eqrange.دوجا; ++itr) {
                    Ast_گرہ_سونپو* ثابت assp = itr->دوجا;
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
            لكل (std::سمتیہ<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.سرا(); ++it) {
                AstVar* ثابت varp = *it;
                إذا (!varp) استمر;
                إذا (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    إذا (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = خالی_سوچک;
                    retry = سچ;
                }
            }
        }
        لكل (std::سمتیہ<Ast_گرہ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.سرا();
             ++it) {
            إذا ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت classp
                    = VN_CAST((*it), گرہ_UOr_Struct_DType)) {
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
        لكل (خودکار& itr : m_dtypePkgsp) {
            إذا (itr.پہلا->user1()) itr.دوجا->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارا::followAlwaysTrue);
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
        VIsCached::clear_Cache_درخت();  // Removing assignments may affect isPure
        V3ConstPool::invalidateCache();  // Might have deleted constant pool entries
    }
    ~DeadVisitor() override {
        V3Stats::جوڑو_Stat_جوڑ("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::جوڑو_Stat_جوڑ("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::جوڑو_Stat_جوڑ("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::جوڑو_Stat_جوڑ("Optimizations, FTasks, deadified, virtual",
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
    V3Global::dump_Check_Global_درخت("deadModules", 0, dump_درخت_Either_Level() >= 6);
}

فراغ V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, خطأ, سچ, خطأ, خطأ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadDtypes", 0, dump_درخت_Either_Level() >= 3);
}

فراغ V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, خطأ, سچ, سچ, خطأ, خطأ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadDtypesScoped", 0, dump_درخت_Either_Level() >= 3);
}

فراغ V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, سچ, سچ, خطأ, سچ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadAll", 0, dump_درخت_Either_Level() >= 3);
}

فراغ V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, سچ, سچ, سچ, سچ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadAllScoped", 0, dump_درخت_Either_Level() >= 3);
}
