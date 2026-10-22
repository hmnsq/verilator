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
    ثابت VNUser1_دننه_Use m_inuser1;

    // MEMBERS
    string m_مختاړی;  // String prefix to add to class name based on hier
    V3_Unique_نومونه m_نومونه;  // For unique naming of structs and unions
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    Ast_غوټه_Module* m_classPackagep = تش_اشاره;  // Package moving into
    ثابت AstScope* m_classScopep = تش_اشاره;  // Package moving scopes into
    AstScope* m_packageScopep = تش_اشاره;  // Class package scope
    ثابت Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current task
    std::ویکټور<std::pair<Ast_غوټه*, AstScope*>> m_منزل_Scope_حرکتونه;
    std::ویکټور<std::pair<Ast_غوټه*, Ast_غوټه_Module*>> m_منزل_Package_حرکتونه;
    std::سېټ<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::سېټ<Ast_غوټه_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::کتار<Ast_غوټه_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    فراغ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        إذا (nodep->use_Virtual_عامه()) أرجع;  // Short-circuit to exit diamond cycles
        إذا (nodep->isInterfaceClass()) { nodep->use_Virtual_عامه(ریښتیا); }
        لكل (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    فراغ لیدنه(AstClass* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        // Move this class
        nodep->name(m_مختاړی + nodep->name());
        nodep->unlink_Fr_شا();
        v3Global.rootp()->جمع_کړه_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = جديد AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_شمېر_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->جمع_کړه_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = جديد AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           تش_اشاره,
                                           تش_اشاره,
                                           تش_اشاره};
        cellp->modp(packagep);
        v3Global.rootp()->سر_Modulep()->جمع_کړه_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = تش_اشاره;
        لكل (Ast_غوټه* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            إذا ((classScopep = VN_CAST(itp, Scope))) اكسر;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = جديد AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->جمع_کړه_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_مختاړی);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_مختاړی = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_مختاړی);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_مختاړی = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_packageScopep) {
            إذا (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_منزل_Scope_حرکتونه.emplace_شا(nodep, m_packageScopep);
            }
            إذا (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_منزل_Package_حرکتونه.emplace_شا(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_منزل_Scope_حرکتونه.emplace_شا(nodep, m_packageScopep);
            }
        }
    }

    فراغ لیدنه(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        إذا (m_packageScopep && nodep->isStatic()) {
            m_منزل_Scope_حرکتونه.emplace_شا(nodep, m_packageScopep);
        }
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    فراغ لیدنه(Ast_غوټه_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جمع_کړه_Stmtsp(nodep->unlink_Fr_شا());
    }
    فراغ لیدنه(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_منزل_Scope_حرکتونه.emplace_شا(nodep, m_packageScopep);
    }
    فراغ لیدنه(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_منزل_Scope_حرکتونه.emplace_شا(nodep, m_packageScopep);
    }

    فراغ سېټ_Struct_Modulep(Ast_غوټه_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_شمېر_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_نومونه.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        إذا (dtypep->packed()) m_strDtypeps.ورننباسه(dtypep);

        لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_غوټه_UOr_Struct_DType* ثابت subp = itemp->get_ماشوم_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            إذا (subp && subp->name().تشه()) سېټ_Struct_Modulep(subp);
        }
    }
    فراغ لیدنه(AstTypedef* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        Ast_غوټه_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), غوټه_UOr_Struct_DType);
        إذا (dtypep && dtypep->packed()) {
            m_typedefps.ورننباسه(nodep);
            إذا (nodep->attr_عامه()) m_pubStrDtypeps.ټېل(dtypep);
        }
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جمع_کړه_Stmtsp(nodep->unlink_Fr_شا());

        إذا (dtypep) {
            dtypep->name(nodep->name());
            سېټ_Struct_Modulep(dtypep);
        }
    }

    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {}  // Short circuit
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {}  // Short circuit
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        لكل (خپلکاری moved : m_منزل_Scope_حرکتونه) {
            Ast_غوټه* ثابت nodep = moved.لومړی;
            AstScope* ثابت scopep = moved.دوهم;
            UINFO(9, "moving " << nodep << " to " << scopep);
            إذا (VN_IS(nodep, غوټه_FTask)) {
                scopep->جمع_کړه_Blocksp(nodep->unlink_Fr_شا());
            } وإلا إذا (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_شا();
                scopep->جمع_کړه_Varsp(vscp);
            } وإلا إذا (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_شا();
                scopep->جمع_کړه_Blocksp(nodep);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
        }
        لكل (خپلکاری moved : m_منزل_Package_حرکتونه) {
            Ast_غوټه* ثابت nodep = moved.لومړی;
            Ast_غوټه_Module* ثابت modp = moved.دوهم;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_شا();
            modp->جمع_کړه_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::سېټ<ثابت Ast_غوټه_UOr_Struct_DType*> pubStrDtypeps;
        طالما (!m_pubStrDtypeps.تشه()) {
            Ast_غوټه_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.راوباسه();
            إذا (pubStrDtypeps.ورننباسه(dtypep).دوهم) {
                لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    إذا (Ast_غوټه_UOr_Struct_DType* ثابت subp = itemp->get_ماشوم_Structp())
                        m_pubStrDtypeps.ټېل(subp);
                }
            }
        }
        لكل (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_غوټه_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), غوټه_UOr_Struct_DType);
            إذا (pubStrDtypeps.شمېر(sdtypep)) typedefp->attr_عامه(ریښتیا);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        لكل (Ast_غوټه_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            إذا (!pubStrDtypeps.شمېر(sdtypep)) sdtypep->classOrPackagep(تش_اشاره);
        }
    }
};

//######################################################################
// Class class functions

فراغ V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("class", 0, dump_ونه_Either_Level() >= 3);
}
