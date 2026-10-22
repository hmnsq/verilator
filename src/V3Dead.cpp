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

جماعت Dead_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Dead_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_nodep;  // Node that created vertex
    uint64_t m_workPos = 0;  // Position on DeadWorkList, 0 = not on list
    منطقی m_removable = جھوٹ;  // Subject to dead removal
    منطقی m_isVirtual = جھوٹ;  // Virtual node, indirection

عوامی:
    Dead_راس(V3_گراف* graphp, Ast_گرہ* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Dead_راس() override = طےشدہ;
    string dotShape() ثابت override {
        واپس isVirtual() ? "diamond" : removable() ? "rectangle" : "ellipse";
    }
    Ast_گرہ* nodep() ثابت VL_MT_STABLE { واپس m_nodep; }
    string name() ثابت override VL_MT_STABLE {
        واپس (isVirtual()   ? "[VIRT] "
                : removable() ? "[R] "s
                              : "[NR] ")
               + nodep()->typeName() + ' ' + cvt_مقصد_Hex(nodep()) + ' ' + nodep()->name();
    }
    منطقی removable() ثابت { واپس m_removable; }
    خالی removable(منطقی flag) { m_removable = flag; }
    منطقی isVirtual() ثابت { واپس m_isVirtual; }
    خالی isVirtual(منطقی flag) { m_isVirtual = flag; }
    uint64_t workPos() ثابت { واپس m_workPos; }
    خالی workPos(uint64_t قدر) { m_workPos = قدر; }
};

// Work list which keeps ordering of elements (so stable), adds always to
// the end of the queue, and allows arbitrary removal
جماعت DeadWorkList final {
    // MEMBERS
    ساکن uint64_t s_sequence;  // Sequence number, inc each push
    std::map<uint64_t, Dead_راس*> m_چلتا;  // Work list by sequence
عوامی:
    // METHODS
    منطقی خالی_خانے() ثابت { واپس m_چلتا.خالی_خانے(); }
    خالی ڈالو(Dead_راس* vtxp) {
        اگر (vtxp->workPos()) واپس;  // Already on list
        ثابت uint64_t id = ++s_sequence;
        vtxp->workPos(id);
        m_چلتا.emplace(id, vtxp);
        UINFO(9, "Worklist.push " << vtxp);
    }
    خالی erase(Dead_راس* vtxp) {
        ثابت uint64_t id = vtxp->workPos();
        اگر (!id) واپس;
        UINFO(9, "Worklist.erase " << vtxp);
        ثابت خودکار it = m_چلتا.ڈھونڈو(id);
        UASSERT_OBJ(it != m_چلتا.انتہا(), vtxp->nodep(),
                    "vertex thought to be on work list but not");
        m_چلتا.erase(it);
        vtxp->workPos(0);
    }
    Dead_راس* get_نکالو_Front() {
        UDEBUGONLY(UASSERT(!خالی_خانے(), "Front invalid to call on empty list"););
        ثابت خودکار it = m_چلتا.begin();
        Dead_راس* ثابت vtxp = it->دوسرا;
        m_چلتا.erase(it);
        vtxp->workPos(0);
        UINFO(9, "Worklist.getPopFront " << vtxp);
        واپس vtxp;
    }
};

uint64_t DeadWorkList::s_sequence = 0;

جماعت Dead_گراف final : عوامی V3_گراف {
    // NODE STATE
    //  AstNodeFTask::user2p()  -> DeadVertex* for this node
    //  See const VNUser2InUse m_inuser2; inside DeadVisitor
    // MEMBERS
    DeadWorkList m_funcs;  // Functions eligble for deletion
    // Each virtual vertex by the ftask name()
    std::unordered_map<std::string, Dead_راس*> m_virtualVtxsp;

    خالی new_کنارہ(Dead_راس* fromp, Dead_راس* چوٹی) {
        نیا V3_گراف_کنارہ{یہ, fromp, چوٹی, 1, جھوٹ};
        اگر (چوٹی->removable()) m_funcs.erase(چوٹی);  // Now has an input edge
    }
    خالی ڈالو_Work_Maybe(Dead_راس* vtxp, منطقی allowSize1) {
        اگر (vtxp->removable()) {
            اگر (VN_IS(vtxp->nodep(), گرہ_FTask)
                && (vtxp->اندر_خالی_خانے() || (allowSize1 && vtxp->اندر_Size1())))
                m_funcs.ڈالو(vtxp);
        }
    }

عوامی:
    // METHODS
    Dead_گراف() = طےشدہ;
    ~Dead_گراف() override = طےشدہ;
    Dead_راس* ڈھونڈو_New_راس(Ast_گرہ* nodep) {
        Dead_راس* vtxp = nodep->user2u().مقصد<Dead_راس*>();
        اگر (!vtxp) {
            vtxp = نیا Dead_راس{یہ, nodep};
            nodep->user2p(vtxp);
            ڈالو_Work_Maybe(vtxp, جھوٹ);
        }
        UASSERT_OBJ(vtxp->nodep() == nodep, nodep, "Vertex points at different node");
        واپس vtxp;
    }
    Dead_راس* ڈھونڈو_Virtual_راس(Ast_گرہ* nodep) {
        ثابت خودکار it = m_virtualVtxsp.ڈھونڈو(nodep->name());
        Dead_راس* vtxp;
        اگر (it != m_virtualVtxsp.انتہا()) {
            vtxp = it->دوسرا;
        } ورنہ {
            vtxp = نیا Dead_راس{یہ, nodep};
            vtxp->removable(سچ);
            vtxp->isVirtual(سچ);
            ڈالو_Work_Maybe(vtxp, جھوٹ);
            m_virtualVtxsp.emplace(nodep->name(), vtxp);
        }
        واپس vtxp;
    }
    خالی ڈھونڈو_New_Removable_راس(Ast_گرہ* nodep, منطقی removable) {
        Dead_راس* ثابت vtxp = ڈھونڈو_New_راس(nodep);
        // Wasn't removable before (due to earlier insert), make removable now
        اگر (removable && !vtxp->removable()) {
            vtxp->removable(سچ);
            ڈالو_Work_Maybe(vtxp, جھوٹ);
        }
    }
    خالی delete_گرہ_راس(Ast_گرہ* nodep) {
        اگر (Dead_راس* ثابت vtxp = nodep->user2u().مقصد<Dead_راس*>()) delete_راس(vtxp);
    }
    خالی delete_راس(Dead_راس* vtxp) {
        UINFO(9, "Delete vertex " << vtxp->name());
        // Mark all about-to-empty downstream vertices onto worklist
        برائے (ثابت V3_گراف_کنارہ& oedge : vtxp->برآمد_کنارے()) {
            Dead_راس* ثابت مقصد_Vtxp = static_cast<Dead_راس*>(oedge.چوٹی());
            ڈالو_Work_Maybe(مقصد_Vtxp, سچ);  // size1 ok as about to delete below
        }
        // Node shouldn't be looking at user2p later as node being deleting, but in case
        اگر (!vtxp->isVirtual()) vtxp->nodep()->user2p(خالی_اشاریہ);
        اگر (vtxp->removable()) m_funcs.erase(vtxp);
        VL_DO_DANGLING(vtxp->unlinkDelete(یہ), vtxp);
    }
    // This only tracks usage dependancy, not "containership",
    // When all needs disappear the related node is eligble for deletion
    خالی needs(Ast_گرہ* nodep, Ast_گرہ* parentp) {
        اگر (parentp == nodep) واپس;  // No need for tracking needs itself (recursion)
        UINFO(9, "Edge node " << nodep << " -> " << parentp);
        Dead_راس* ثابت والد_Vtxp = ڈھونڈو_New_راس(parentp);
        Dead_راس* ثابت گرہ_Vtxp = ڈھونڈو_New_راس(nodep);
        UINFO(9, "Edge need " << والد_Vtxp << " -> " << گرہ_Vtxp);
        new_کنارہ(والد_Vtxp, گرہ_Vtxp);
    }
    خالی needsVirtual(Ast_گرہ_FTask* nodep, Ast_گرہ* parentp) {
        // Virtual call can be to any function in the call hierarchy.
        // For simplicity rather than tracking possible multiple base
        // classes (due to 'implements' classes there can be more than
        // one), we simply assume all virtual functions of the same name
        // can call any other virtual function of the same name
        // Track via an intermediate node.
        //  All calling parents' verticies -> Virtual Vertex -> all ftasks verticies
        اگر (parentp == nodep) واپس;  // No need for tracking needs itself (recursion)
        Dead_راس* ثابت والد_Vtxp = ڈھونڈو_New_راس(parentp);
        Dead_راس* ثابت virtualVtxp = ڈھونڈو_Virtual_راس(nodep);
        UINFO(9, "Edge needVirtual " << والد_Vtxp << " -> " << virtualVtxp);
        new_کنارہ(والد_Vtxp, virtualVtxp);
    }
    خالی funcVirtual(Ast_گرہ_FTask* nodep) {
        //  Virtual Vertex -> all ftasks verticies
        Dead_راس* ثابت virtualVtxp = ڈھونڈو_Virtual_راس(nodep);
        Dead_راس* ثابت گرہ_Vtxp = ڈھونڈو_New_راس(nodep);
        UINFO(9, "Edge funcvirtual " << virtualVtxp << " -> " << گرہ_Vtxp);
        new_کنارہ(virtualVtxp, گرہ_Vtxp);
    }
    منطقی funcs_خالی_خانے() ثابت { واپس m_funcs.خالی_خانے(); }
    Ast_گرہ* funcs_Get_نکالو_Front() {
        Dead_راس* ثابت vtxp = m_funcs.get_نکالو_Front();
        UASSERT_OBJ(vtxp->اندر_خالی_خانے(), vtxp->nodep(), "Non-empty node on work list");
        UASSERT_OBJ(vtxp->removable(), vtxp->nodep(), "Non-removable node on work list");
        اگر (vtxp->isVirtual()) {
            // Emptied (no inbound edge) virtual wrapper; all
            // destinations verticies are now unused too (e.g. all virtual
            // functions of this name may be deleted)
            UINFO(9, "Removing virtual " << vtxp);
            delete_راس(vtxp);
            واپس خالی_اشاریہ;  // Caller will search again
        }
        Ast_گرہ* ثابت nodep = vtxp->nodep();
        واپس nodep;
    }
};

//######################################################################
// Dead state, as a visitor of each AstNode

جماعت DeadVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Entire Netlist:
    //  AstNodeModule::user1()  -> int. Count of number of cells referencing this module.
    //  AstVar::user1()         -> int. Count of number of references
    //  AstVar::user2()         -> bool. Is a formal function argument
    //  AstVarScope::user1()    -> int. Count of number of references
    //  AstNodeDType::user1()   -> int. Count of number of references
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;  // For usage information see DeadGraph

    // TYPES
    استعمال تفویض_Map = std::multimap<AstVarScope*, Ast_گرہ_تفویض*>;

    // STATE - across all visitors
    ثابت منطقی m_elimUserVars;  // Allow removal of user's vars
    ثابت منطقی m_elimDTypes;  // Allow removal of DTypes
    ثابت منطقی m_elimCells;  // Allow removal of Cells

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
    تفویض_Map m_تفویض_Map;  // List of all simple assignments for each variable
    منطقی m_sideEffect = جھوٹ;  // Side effects discovered in assign RHS

    // STATE - for current visit position (use VL_RESTORER)
    منطقی m_اندر_تفویض = جھوٹ;  // Currently in an assign
    Ast_گرہ_DType* m_curDTypep = خالی_اشاریہ;  // Current NodeDType
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_Foreach_سرخی* m_foreachHeaderp = خالی_اشاریہ;  // Current foreach header
    Ast_گرہ* m_containingFTaskRefp = خالی_اشاریہ;  // Parent of ftaskref (e.g. task/module)
    منطقی m_اندر_FTask = جھوٹ;  // Under an AstNodeFTask

    // STATE - Statistic tracking
    VDouble0 m_statFTasksDemoted;
    VDouble0 m_statFTasksMDeadified;
    VDouble0 m_statFTasksNMDeadified;
    VDouble0 m_statFTasksVirtDeadified;

    // METHODS

    خالی deleting(Ast_گرہ* nodep) {
        UINFO(9, "  deleting " << nodep);
        m_گراف.delete_گرہ_راس(nodep);
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    // cppcheck-suppress constParameterPointer
    خالی checkAll(Ast_گرہ* nodep) {
        اگر (Ast_گرہ* ثابت subnodep = nodep->dtypep()) {
            اگر (nodep != subnodep  // Not NodeDTypes reference themselves
                && m_curDTypep != subnodep) {  // Not EnumItem referencing parent Enum
                subnodep->user1Inc();
            }
        }
        اگر (Ast_گرہ* ثابت subnodep = nodep->get_بچہ_DTypep()) subnodep->user1Inc();
    }
    خالی checkVarRef(Ast_گرہ_Var_Ref* nodep) ثابت {
        اگر (nodep->classOrPackagep() && m_elimCells) nodep->classOrPackagep(خالی_اشاریہ);
    }
    خالی checkDType(Ast_گرہ_DType* nodep) {
        اگر (!nodep->generic()  // Don't remove generic types
            && m_elimDTypes  // dtypes stick around until post-widthing
            && !VN_IS(nodep, MemberDType)  // Keep member names iff upper type exists
            && !nodep->undead()  // VoidDType or something Netlist points to
        ) {
            m_dtypeElimsp.ڈالو_عقب(nodep);
        }
        اگر (VN_IS(m_modp, Package) || VN_IS(m_modp, Class)) m_dtypePkgsp.emplace(nodep, m_modp);
        // A struct type moved to the type table refers to its package via classOrPackagep
        اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep = VN_CAST(nodep, گرہ_UOr_Struct_DType)) {
            اگر (sdtypep->classOrPackagep() && sdtypep->classOrPackagep() != m_modp) {
                m_dtypePkgsp.emplace(nodep, sdtypep->classOrPackagep());
            }
        }
        اگر (Ast_گرہ* ثابت subnodep = nodep->virtRefDTypep()) subnodep->user1Inc();
        اگر (Ast_گرہ* ثابت subnodep = nodep->virtRefDType2p()) subnodep->user1Inc();
    }
    خالی needsTask(Ast_گرہ_FTask* taskp, Ast_گرہ* containerp) {
        اگر (!taskp) واپس;  // Unlinked
        اگر (taskp->isVirtual()) {
            m_گراف.needsVirtual(taskp, containerp);
        } ورنہ {
            m_گراف.needs(taskp, containerp);
        }
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (m_modp) m_modp->user1Inc();  // e.g. Class under Package
        VL_RESTORER(m_modp);
        m_modp = nodep;
        VL_RESTORER(m_containingFTaskRefp);
        m_containingFTaskRefp = nodep;
        اگر (nodep->dead()) واپس;
        اگر (nodep->mod_عوامی()) m_modp->user1Inc();
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            اگر (classp->extendsp()) classp->extendsp()->user1Inc();
            اگر (classp->classOrPackagep()) classp->classOrPackagep()->user1Inc();
            m_classesp.ڈالو_عقب(classp);
            // TODO we don't reclaim dead classes yet - graph implementation instead?
            classp->user1Inc();
        }
    }
    خالی دورہ(AstCFunc* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->scopep()) nodep->scopep()->user1Inc();
        // Keep formal arguments
        برائے (AstVar* varp = nodep->argsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            varp->user1Inc();
            varp->user2(1);
        }
    }
    خالی دورہ(AstPin* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->modVarp()) nodep->modVarp()->user1Inc();
    }
    خالی دورہ(AstScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->aboveScopep()) nodep->aboveScopep()->user1Inc();
        // Class packages might have no children, but need to remain as
        // long as the class they refer to is needed
        اگر (VN_IS(m_modp, Class) || VN_IS(m_modp, ClassPackage)) nodep->user1Inc();
        // The constant pool scope must remain for entries created later
        اگر (m_modp->isConstPool()) nodep->user1Inc();
        اگر (!nodep->is_چوٹی() && !nodep->varsp() && !nodep->blocksp()) {
            m_scopesp.ڈالو_عقب(nodep);
        }
    }
    خالی دورہ(AstCell* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        m_cellsp.ڈالو_عقب(nodep);
        nodep->modp()->user1Inc();
    }

    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
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
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        اگر (!m_sideEffect && !nodep->is_خالص()) m_sideEffect = سچ;
        checkAll(nodep);
        needsTask(nodep->taskp(), m_containingFTaskRefp);
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(خالی_اشاریہ);
            } ورنہ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    خالی دورہ(AstModportFTaskRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        needsTask(nodep->ftaskp(), m_containingFTaskRefp);
    }
    خالی دورہ(AstModportVarRef* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->varp()) nodep->varp()->user1Inc();
    }
    خالی دورہ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        UASSERT_OBJ(!(m_elimCells && nodep->typedefp()), nodep,
                    "RefDType should point to data type before typedefs removed");
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(خالی_اشاریہ);
            } ورنہ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }
    خالی دورہ(AstClassRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(خالی_اشاریہ);
            } ورنہ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        اگر (nodep->classp()) nodep->classp()->user1Inc();
    }
    خالی دورہ(AstIfaceRefDType* nodep) override {
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
        اگر (nodep->modportp()) {
            اگر (m_elimCells) {
                nodep->modportp(خالی_اشاریہ);
            } ورنہ {
                nodep->modportp()->user1Inc();
            }
        }
        اگر (nodep->iface_بذریعہ_Cellp()) nodep->iface_بذریعہ_Cellp()->user1Inc();
    }
    خالی دورہ(Ast_گرہ_DType* nodep) override {
        VL_RESTORER(m_curDTypep);
        m_curDTypep = nodep;
        iterateChildren(nodep);
        checkDType(nodep);
        checkAll(nodep);
    }
    خالی دورہ(Ast_Enum_شے_Ref* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(خالی_اشاریہ);
            } ورنہ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
        checkAll(nodep);
    }
    خالی دورہ(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->varp()) nodep->varp()->user1Inc();
        اگر (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // classref
        checkAll(nodep);
    }
    خالی دورہ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->fromp()->dtypep()) nodep->fromp()->dtypep()->user1Inc();  // structdtype
        checkAll(nodep);
    }
    خالی دورہ(AstModport* nodep) override {
        iterateChildren(nodep);
        اگر (m_elimCells) {
            اگر (!nodep->varsp()) {
                deleting(nodep);
                واپس;
            }
        }
        checkAll(nodep);
    }
    خالی دورہ(Ast_Foreach_سرخی* nodep) override {
        // Var under a ForeachHeader means we haven't called V3Width to remove them yet
        VL_RESTORER(m_foreachHeaderp);
        m_foreachHeaderp = nodep;
        iterateChildren(nodep);
        checkAll(nodep);
    }
    خالی دورہ(AstTypedef* nodep) override {
        iterateChildren(nodep);
        m_typedefsp.ڈالو_عقب(nodep);

        // Don't let packages with only public variables disappear
        // Normal modules may disappear, e.g. if they are parameterized then removed
        اگر (nodep->attr_عوامی() && m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
    }
    خالی دورہ(AstVarScope* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->scopep()) nodep->scopep()->user1Inc();
        اگر (mightElimVar(nodep->varp())) m_vscsp.ڈالو_عقب(nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (m_foreachHeaderp) nodep->user1Inc();
        // Keep formal arguments
        اگر (m_اندر_FTask && nodep->isIO()) {
            nodep->user1Inc();
            nodep->user2(1);
        }
        اگر (mightElimVar(nodep)) {
            m_varsp.ڈالو_عقب(nodep);
        } ورنہ {
            اگر (m_modp && VN_IS(m_modp, Package)) m_modp->user1Inc();
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        // See if simple assignments to variables may be eliminated because
        // that variable is never used.
        // Similar code in V3Life
        ثابت منطقی تفویض_اندر_تفویض = m_اندر_تفویض;  // Might be Assign(..., ExprStmt(Assign), ...)
        {
            VL_RESTORER(m_اندر_تفویض);
            VL_RESTORER(m_sideEffect);
            m_اندر_تفویض = سچ;
            m_sideEffect = nodep->isTimingControl();  // Can't remove the delay
            iterate_And_اگلا_Null(nodep->rhsp());
            checkAll(nodep);
            // Has to be direct assignment without any EXTRACTing.
            AstVarRef* ثابت varrefp = VN_CAST(nodep->lhsp(), VarRef);
            اگر (varrefp && !m_sideEffect && v3Global.opt.fDeadAssigns()
                && varrefp->varScopep()) {  // For simplicity, we only remove post-scoping
                m_تفویض_Map.emplace(varrefp->varScopep(), nodep);
                checkAll(varrefp);  // Must track reference to dtype()
                checkVarRef(varrefp);
            } ورنہ {  // Track like any other statement
                iterate_And_اگلا_Null(nodep->lhsp());
            }
            iterateNull(nodep->timingControlp());
        }
        اگر (تفویض_اندر_تفویض) m_sideEffect = سچ;  // Parent assign shouldn't optimize
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        ثابت منطقی removable = !(nodep->task_عوامی() || nodep->dpiExport() || nodep->dpiImport()
                                 || nodep->keepAlive() || nodep->isConstructor()
                                 || (!v3Global.opt.fDeadMethods() && nodep->class_طریقہ()));
        m_گراف.ڈھونڈو_New_Removable_راس(nodep, removable);
        //
        VL_RESTORER(m_containingFTaskRefp);
        VL_RESTORER(m_اندر_FTask);
        m_containingFTaskRefp = nodep;
        m_اندر_FTask = سچ;
        iterateChildren(nodep);
        checkAll(nodep);
        اگر (nodep->isVirtual()) m_گراف.funcVirtual(nodep);
        اگر (!removable) {
            اگر (m_modp && !m_modp->dead() && !m_modp->verilatorLib())
                m_modp->user1Inc();  // Keep container
        }
        اگر (nodep->classOrPackagep()) {
            اگر (m_elimCells) {
                nodep->classOrPackagep(خالی_اشاریہ);
            } ورنہ {
                nodep->classOrPackagep()->user1Inc();
            }
        }
    }

    //-----
    خالی دورہ(Ast_Clocking_شے* nodep) override {
        // Prevent V3Dead from deleting clockvars that are seemingly dead before V3AssertPre. Later
        // the vars will be moved to the containing module so if they are actually dead they will
        // still get deleted.
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (!m_sideEffect && !nodep->is_خالص()) m_sideEffect = سچ;
        iterateChildren(nodep);
        checkAll(nodep);
    }

    // METHODS
    خالی deadCheckTypedefs() {
        برائے (AstTypedef* typedefp : m_typedefsp) {
            اگر (shouldDeleteTypedef(typedefp)) {
                deleting(typedefp);
                جاری;
            }
            checkAll(typedefp);
        }
    }
    منطقی shouldDeleteTypedef(ثابت AstTypedef* typedefp) ثابت {
        اگر (ثابت خودکار* ثابت structp = VN_CAST(typedefp->subDTypep(), گرہ_UOr_Struct_DType)) {
            اگر (structp->user1() && !structp->packed()) واپس جھوٹ;
        }
        واپس m_elimCells && !typedefp->attr_عوامی();
    }

    خالی deadCheckTasks() {
        جب (!m_گراف.funcs_خالی_خانے()) {
            Ast_گرہ* ثابت nodep = m_گراف.funcs_Get_نکالو_Front();
            اگر (!nodep) جاری;
            UINFO(9, "Dead " << nodep);
            اگر (Ast_گرہ_FTask* ثابت taskp = VN_CAST(nodep, گرہ_FTask)) {
                اگر (taskp->isVirtual()) {
                    ++m_statFTasksVirtDeadified;
                } ورنہ اگر (taskp->class_طریقہ()) {
                    ++m_statFTasksMDeadified;
                } ورنہ {
                    ++m_statFTasksNMDeadified;
                }
            }
            deleting(nodep);
        }
    }

    خالی deadCheckDemote() {
        برائے (V3_گراف_راس& gvtx : m_گراف.vertices()) {
            Dead_راس* ثابت vtxp = gvtx.cast<Dead_راس>();
            // A isVirtual vertex with single out means there's only one target virtual function
            // that can be virtually called, so can make it non-virtual for faster execution
            // (UVM benefits from this)
            اگر (!vtxp->برآمد_Size1()) جاری;
            اگر (!vtxp->isVirtual()) جاری;
            برائے (V3_گراف_کنارہ& کنارہ : vtxp->برآمد_کنارے()) {  // Always a single one
                Ast_گرہ* ثابت nodep = کنارہ.چوٹی()->as<Dead_راس>()->nodep();
                Ast_گرہ_FTask* ثابت funcp = VN_AS(nodep, گرہ_FTask);
                UASSERT_OBJ(funcp->isVirtual(), funcp,
                            "Only virtual ftasks should be under DeadVirtualVertex");
                اگر (v3Global.opt.fDeadMethods()) {
                    funcp->isVirtual(جھوٹ);
                    UINFO(9, "Demote to non-virtual " << funcp);
                    ++m_statFTasksDemoted;
                }
            }
        }
    }

    خالی deadCheckMod() {
        // Kill any unused modules
        // V3LinkCells has a graph that is capable of this too, but we need to do it
        // after we've done all the generate blocks
        برائے (منطقی retry = سچ; retry;) {
            retry = جھوٹ;
            Ast_گرہ_Module* nextmodp;
            برائے (Ast_گرہ_Module* modp = v3Global.rootp()->modulesp(); modp; modp = nextmodp) {
                nextmodp = VN_AS(modp->nextp(), گرہ_Module);
                // Keep $unit until m_elimCells stages. Note v3Global.opt.serializeOnly()
                // won't reach this stage, and will always have an empty $unit. That's ok.
                // The constant pool is always kept, entries might be created later.
                ثابت منطقی keep = (!m_elimCells && modp->isDollarUnit()) || modp->isConstPool();
                اگر (modp->dead() || (!modp->is_چوٹی() && modp->user1() == 0 && !keep)) {
                    // > 2 because L1 is the wrapper, L2 is the top user module
                    UINFO(4, "  Dead module " << modp);
                    // And its children may now be killable too; correct counts
                    // Recurse, as cells may not be directly under the module but in a generate
                    اگر (!modp->dead()) {  // If was dead didn't increment user1's
                        modp->foreach([](ثابت AstCell* cellp) {  //
                            cellp->modp()->user1Inc(-1);
                        });
                    }
                    اگر (modp->isDollarUnit()) v3Global.rootp()->dollarUnitPkgp(خالی_اشاریہ);
                    deleting(modp);
                    retry = سچ;
                }
            }
        }
    }
    منطقی mightElimVar(ثابت AstVar* nodep) ثابت {
        اگر (nodep->is_Sig_عوامی()) واپس جھوٹ;  // Can't elim publics!
        اگر (nodep->isPrimaryIO() || nodep->isClassMember() || nodep->sensIfacep()) واپس جھوٹ;
        اگر (nodep->is_عارضی() && !nodep->isTrace()) واپس سچ;
        واپس m_elimUserVars;  // Post-Trace can kill most anything
    }

    خالی deadCheckScope() {
        برائے (منطقی retry = سچ; retry;) {
            retry = جھوٹ;
            برائے (std::سمتیہ<AstScope*>::iterator it = m_scopesp.begin(); it != m_scopesp.انتہا();
                 ++it) {
                AstScope* ثابت scp = *it;
                اگر (!scp) جاری;
                اگر (scp->user1() == 0) {
                    UINFO(4, "  Dead AstScope " << scp);
                    scp->aboveScopep()->user1Inc(-1);
                    اگر (scp->dtypep()) scp->dtypep()->user1Inc(-1);
                    deleting(scp);
                    *it = خالی_اشاریہ;
                    retry = سچ;
                }
            }
        }
    }

    خالی deadCheckCells() {
        برائے (AstCell* cellp : m_cellsp) {
            اگر (cellp->user1() == 0 && v3Global.opt.fDeadCells() &&  //
                !cellp->modp()->stmtsp() && !cellp->modp()->scSectionsp()) {
                cellp->modp()->user1Inc(-1);
                deleting(cellp);
            }
        }
    }
    خالی deadCheckClasses() {
        برائے (منطقی retry = سچ; retry;) {
            retry = جھوٹ;
            برائے (خودکار& itr : m_classesp) {
                اگر (AstClass* ثابت nodep = itr) {  // nullptr if deleted earlier
                    اگر (nodep->user1() == 0) {
                        اگر (nodep->extendsp()) nodep->extendsp()->user1Inc(-1);
                        اگر (nodep->classOrPackagep()) nodep->classOrPackagep()->user1Inc(-1);
                        deleting(nodep);
                        itr = خالی_اشاریہ;
                        retry = سچ;
                    }
                }
            }
        }
    }

    خالی deadCheckVar() {
        // Delete any unused varscopes
        برائے (AstVarScope* vscp : m_vscsp) {
            // Keep formal arguments
            اگر (vscp->varp()->user2()) جاری;
            اگر (vscp->user1() == 0) {
                UINFO(4, "  Dead " << vscp);
                ثابت std::pair<تفویض_Map::iterator, تفویض_Map::iterator> eqrange
                    = m_تفویض_Map.equal_range(vscp);
                برائے (تفویض_Map::iterator itr = eqrange.پہلا; itr != eqrange.دوسرا; ++itr) {
                    Ast_گرہ_تفویض* ثابت assp = itr->دوسرا;
                    UINFO(4, "    Dead assign " << assp);
                    assp->dtypep()->user1Inc(-1);
                    deleting(assp);
                }
                اگر (vscp->scopep()) vscp->scopep()->user1Inc(-1);
                vscp->dtypep()->user1Inc(-1);
                deleting(vscp);
            }
        }
        برائے (منطقی retry = سچ; retry;) {
            retry = جھوٹ;
            برائے (std::سمتیہ<AstVar*>::iterator it = m_varsp.begin(); it != m_varsp.انتہا(); ++it) {
                AstVar* ثابت varp = *it;
                اگر (!varp) جاری;
                اگر (varp->user1() == 0) {
                    UINFO(4, "  Dead " << varp);
                    اگر (varp->dtypep()) varp->dtypep()->user1Inc(-1);
                    deleting(varp);
                    *it = خالی_اشاریہ;
                    retry = سچ;
                }
            }
        }
        برائے (std::سمتیہ<Ast_گرہ*>::iterator it = m_dtypeElimsp.begin(); it != m_dtypeElimsp.انتہا();
             ++it) {
            اگر ((*it)->user1() == 0) {
                // It's possible that there if a reference to each individual member, but
                // not to the dtype itself.  Check and don't remove the parent dtype if
                // members are still alive.
                اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت classp
                    = VN_CAST((*it), گرہ_UOr_Struct_DType)) {
                    منطقی cont = سچ;
                    برائے (AstMemberDType* memberp = classp->membersp(); memberp;
                         memberp = VN_AS(memberp->nextp(), MemberDType)) {
                        اگر (memberp->user1() != 0) {
                            cont = جھوٹ;
                            توڑو;
                        }
                    }
                    اگر (!cont) جاری;
                }
                deleting(*it);
            }
        }
    }

عوامی:
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
        برائے (خودکار& itr : m_dtypePkgsp) {
            اگر (itr.پہلا->user1()) itr.دوسرا->user1Inc();
        }

        // Simplify redundant edges (e.g. function calls another function many times)
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);
        اگر (dump_گراف_Level() >= 9 || debug() >= 9)
            m_گراف.dumpDotFilePrefixed("dead_graph", جھوٹ);

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
        V3Stats::جوڑو_Stat_مجموع("Optimizations, FTasks, virtual-to-nonvirtual demotion",
                            m_statFTasksDemoted);
        V3Stats::جوڑو_Stat_مجموع("Optimizations, FTasks, deadified, methods", m_statFTasksMDeadified);
        V3Stats::جوڑو_Stat_مجموع("Optimizations, FTasks, deadified, non-methods",
                            m_statFTasksNMDeadified);
        V3Stats::جوڑو_Stat_مجموع("Optimizations, FTasks, deadified, virtual",
                            m_statFTasksVirtDeadified);
    };
};

//######################################################################
// Dead class functions

خالی V3Dead::deadifyModules(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {  // node, elimUserVars, elimDTypes, elimScopes, elimCells, elimTasks
        DeadVisitor{nodep, جھوٹ, جھوٹ, جھوٹ, جھوٹ, جھوٹ};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadModules", 0, dump_درخت_Either_Level() >= 6);
}

خالی V3Dead::deadifyDTypes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, جھوٹ, سچ, جھوٹ, جھوٹ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadDtypes", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3Dead::deadifyDTypesScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, جھوٹ, سچ, سچ, جھوٹ, جھوٹ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadDtypesScoped", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3Dead::deadifyAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, سچ, سچ, جھوٹ, سچ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadAll", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3Dead::deadifyAllScoped(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DeadVisitor{nodep, سچ, سچ, سچ, سچ, سچ}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deadAllScoped", 0, dump_درخت_Either_Level() >= 3);
}
