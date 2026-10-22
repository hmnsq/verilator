// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Handle SV classes
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
// V3Class's Transformations:
//
// Each class:
//      Move to be modules under AstNetlist
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Class.h"

#ضمن "V3UniqueNames.h"

#ضمن <queue>
#ضمن <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف ClassVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ثابت VNUser1_اندريون_Use m_inuser1;

    // MEMBERS
    string m_اڳياڙي;  // String prefix to add to class name based on hier
    V3_Unique_نالا m_نالا;  // For unique naming of structs and unions
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    Ast_ڳنڍ_Module* m_classPackagep = خالي_اشارو;  // Package moving into
    ثابت AstScope* m_classScopep = خالي_اشارو;  // Package moving scopes into
    AstScope* m_packageScopep = خالي_اشارو;  // Class package scope
    ثابت Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current task
    std::ويڪٽر<std::pair<Ast_ڳنڍ*, AstScope*>> m_منزل_Scope_چالون;
    std::ويڪٽر<std::pair<Ast_ڳنڍ*, Ast_ڳنڍ_Module*>> m_منزل_Package_چالون;
    std::سيٽ<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::سيٽ<Ast_ڳنڍ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::قطار<Ast_ڳنڍ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    فراغ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        إذا (nodep->use_Virtual_عوامي()) أرجع;  // Short-circuit to exit diamond cycles
        إذا (nodep->isInterfaceClass()) { nodep->use_Virtual_عوامي(سچ); }
        لكل (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    فراغ گهمو(AstClass* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        // Move this class
        nodep->name(m_اڳياڙي + nodep->name());
        nodep->unlink_Fr_پٺ();
        v3Global.rootp()->جوڙيو_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = جديد AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_ڳڻپ_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->جوڙيو_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = جديد AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           خالي_اشارو,
                                           خالي_اشارو,
                                           خالي_اشارو};
        cellp->modp(packagep);
        v3Global.rootp()->چوٽي_Modulep()->جوڙيو_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = خالي_اشارو;
        لكل (Ast_ڳنڍ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            إذا ((classScopep = VN_CAST(itp, Scope))) اكسر;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = جديد AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->جوڙيو_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_اڳياڙي);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_اڳياڙي = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_اڳياڙي);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_اڳياڙي = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    فراغ گهمو(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_packageScopep) {
            إذا (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_منزل_Scope_چالون.emplace_پٺ(nodep, m_packageScopep);
            }
            إذا (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_منزل_Package_چالون.emplace_پٺ(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_منزل_Scope_چالون.emplace_پٺ(nodep, m_packageScopep);
            }
        }
    }

    فراغ گهمو(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        إذا (m_packageScopep && nodep->isStatic()) {
            m_منزل_Scope_چالون.emplace_پٺ(nodep, m_packageScopep);
        }
    }
    فراغ گهمو(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    فراغ گهمو(Ast_ڳنڍ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جوڙيو_Stmtsp(nodep->unlink_Fr_پٺ());
    }
    فراغ گهمو(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_منزل_Scope_چالون.emplace_پٺ(nodep, m_packageScopep);
    }
    فراغ گهمو(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_منزل_Scope_چالون.emplace_پٺ(nodep, m_packageScopep);
    }

    فراغ سيٽ_Struct_Modulep(Ast_ڳنڍ_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_ڳڻپ_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_نالا.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        إذا (dtypep->packed()) m_strDtypeps.وجهو(dtypep);

        لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ڳنڍ_UOr_Struct_DType* ثابت subp = itemp->get_ٻار_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            إذا (subp && subp->name().خالي_جايون()) سيٽ_Struct_Modulep(subp);
        }
    }
    فراغ گهمو(AstTypedef* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        Ast_ڳنڍ_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), ڳنڍ_UOr_Struct_DType);
        إذا (dtypep && dtypep->packed()) {
            m_typedefps.وجهو(nodep);
            إذا (nodep->attr_عوامي()) m_pubStrDtypeps.ڌڪو(dtypep);
        }
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جوڙيو_Stmtsp(nodep->unlink_Fr_پٺ());

        إذا (dtypep) {
            dtypep->name(nodep->name());
            سيٽ_Struct_Modulep(dtypep);
        }
    }

    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {}  // Short circuit
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {}  // Short circuit
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        لكل (خودڪار moved : m_منزل_Scope_چالون) {
            Ast_ڳنڍ* ثابت nodep = moved.پهريون;
            AstScope* ثابت scopep = moved.ٻيو;
            UINFO(9, "moving " << nodep << " to " << scopep);
            إذا (VN_IS(nodep, ڳنڍ_FTask)) {
                scopep->جوڙيو_Blocksp(nodep->unlink_Fr_پٺ());
            } وإلا إذا (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_پٺ();
                scopep->جوڙيو_Varsp(vscp);
            } وإلا إذا (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_پٺ();
                scopep->جوڙيو_Blocksp(nodep);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
        }
        لكل (خودڪار moved : m_منزل_Package_چالون) {
            Ast_ڳنڍ* ثابت nodep = moved.پهريون;
            Ast_ڳنڍ_Module* ثابت modp = moved.ٻيو;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_پٺ();
            modp->جوڙيو_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::سيٽ<ثابت Ast_ڳنڍ_UOr_Struct_DType*> pubStrDtypeps;
        طالما (!m_pubStrDtypeps.خالي_جايون()) {
            Ast_ڳنڍ_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.ڪڍو();
            إذا (pubStrDtypeps.وجهو(dtypep).ٻيو) {
                لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    إذا (Ast_ڳنڍ_UOr_Struct_DType* ثابت subp = itemp->get_ٻار_Structp())
                        m_pubStrDtypeps.ڌڪو(subp);
                }
            }
        }
        لكل (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), ڳنڍ_UOr_Struct_DType);
            إذا (pubStrDtypeps.ڳڻپ(sdtypep)) typedefp->attr_عوامي(سچ);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        لكل (Ast_ڳنڍ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            إذا (!pubStrDtypeps.ڳڻپ(sdtypep)) sdtypep->classOrPackagep(خالي_اشارو);
        }
    }
};

//######################################################################
// Class class functions

فراغ V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("class", 0, dump_وڻ_Either_Level() >= 3);
}
