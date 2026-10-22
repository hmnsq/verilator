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
    ثابت VNUser1_وچ_Use m_inuser1;

    // MEMBERS
    string m_اگیتر;  // String prefix to add to class name based on hier
    V3_Unique_ناں m_ناں;  // For unique naming of structs and unions
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    Ast_گرہ_Module* m_classPackagep = خالی_سوچک;  // Package moving into
    ثابت AstScope* m_classScopep = خالی_سوچک;  // Package moving scopes into
    AstScope* m_packageScopep = خالی_سوچک;  // Class package scope
    ثابت Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Current task
    std::سمتیہ<std::pair<Ast_گرہ*, AstScope*>> m_منزل_Scope_چالاں;
    std::سمتیہ<std::pair<Ast_گرہ*, Ast_گرہ_Module*>> m_منزل_Package_چالاں;
    std::سیٹ<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::سیٹ<Ast_گرہ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::قطار<Ast_گرہ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    فراغ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        إذا (nodep->use_Virtual_عوامی()) أرجع;  // Short-circuit to exit diamond cycles
        إذا (nodep->isInterfaceClass()) { nodep->use_Virtual_عوامی(سچ); }
        لكل (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    فراغ ویکھو(AstClass* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        // Move this class
        nodep->name(m_اگیتر + nodep->name());
        nodep->unlink_Fr_پٹھ();
        v3Global.rootp()->جوڑو_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = جديد AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_گنتی_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->جوڑو_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = جديد AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           خالی_سوچک,
                                           خالی_سوچک,
                                           خالی_سوچک};
        cellp->modp(packagep);
        v3Global.rootp()->چوٹی_Modulep()->جوڑو_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = خالی_سوچک;
        لكل (Ast_گرہ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            إذا ((classScopep = VN_CAST(itp, Scope))) اكسر;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = جديد AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->جوڑو_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_اگیتر);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_اگیتر = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_اگیتر);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_اگیتر = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_packageScopep) {
            إذا (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_منزل_Scope_چالاں.emplace_پٹھ(nodep, m_packageScopep);
            }
            إذا (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_منزل_Package_چالاں.emplace_پٹھ(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_منزل_Scope_چالاں.emplace_پٹھ(nodep, m_packageScopep);
            }
        }
    }

    فراغ ویکھو(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        إذا (m_packageScopep && nodep->isStatic()) {
            m_منزل_Scope_چالاں.emplace_پٹھ(nodep, m_packageScopep);
        }
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    فراغ ویکھو(Ast_گرہ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جوڑو_Stmtsp(nodep->unlink_Fr_پٹھ());
    }
    فراغ ویکھو(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_منزل_Scope_چالاں.emplace_پٹھ(nodep, m_packageScopep);
    }
    فراغ ویکھو(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_منزل_Scope_چالاں.emplace_پٹھ(nodep, m_packageScopep);
    }

    فراغ سیٹ_Struct_Modulep(Ast_گرہ_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_گنتی_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_ناں.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        إذا (dtypep->packed()) m_strDtypeps.داخل_کرو(dtypep);

        لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گرہ_UOr_Struct_DType* ثابت subp = itemp->get_بچہ_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            إذا (subp && subp->name().خالی_تھاواں()) سیٹ_Struct_Modulep(subp);
        }
    }
    فراغ ویکھو(AstTypedef* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        Ast_گرہ_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), گرہ_UOr_Struct_DType);
        إذا (dtypep && dtypep->packed()) {
            m_typedefps.داخل_کرو(nodep);
            إذا (nodep->attr_عوامی()) m_pubStrDtypeps.ڈالو(dtypep);
        }
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جوڑو_Stmtsp(nodep->unlink_Fr_پٹھ());

        إذا (dtypep) {
            dtypep->name(nodep->name());
            سیٹ_Struct_Modulep(dtypep);
        }
    }

    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {}  // Short circuit
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {}  // Short circuit
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        لكل (خودکار moved : m_منزل_Scope_چالاں) {
            Ast_گرہ* ثابت nodep = moved.پہلا;
            AstScope* ثابت scopep = moved.دوجا;
            UINFO(9, "moving " << nodep << " to " << scopep);
            إذا (VN_IS(nodep, گرہ_FTask)) {
                scopep->جوڑو_Blocksp(nodep->unlink_Fr_پٹھ());
            } وإلا إذا (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_پٹھ();
                scopep->جوڑو_Varsp(vscp);
            } وإلا إذا (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_پٹھ();
                scopep->جوڑو_Blocksp(nodep);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
        }
        لكل (خودکار moved : m_منزل_Package_چالاں) {
            Ast_گرہ* ثابت nodep = moved.پہلا;
            Ast_گرہ_Module* ثابت modp = moved.دوجا;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_پٹھ();
            modp->جوڑو_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::سیٹ<ثابت Ast_گرہ_UOr_Struct_DType*> pubStrDtypeps;
        طالما (!m_pubStrDtypeps.خالی_تھاواں()) {
            Ast_گرہ_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.نکالو();
            إذا (pubStrDtypeps.داخل_کرو(dtypep).دوجا) {
                لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    إذا (Ast_گرہ_UOr_Struct_DType* ثابت subp = itemp->get_بچہ_Structp())
                        m_pubStrDtypeps.ڈالو(subp);
                }
            }
        }
        لكل (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), گرہ_UOr_Struct_DType);
            إذا (pubStrDtypeps.گنتی(sdtypep)) typedefp->attr_عوامی(سچ);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        لكل (Ast_گرہ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            إذا (!pubStrDtypeps.گنتی(sdtypep)) sdtypep->classOrPackagep(خالی_سوچک);
        }
    }
};

//######################################################################
// Class class functions

فراغ V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("class", 0, dump_درخت_Either_Level() >= 3);
}
