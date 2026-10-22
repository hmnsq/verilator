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
    ثابت VNUser1_درون_Use m_inuser1;

    // MEMBERS
    string m_پیشوند;  // String prefix to add to class name based on hier
    V3_Unique_نامها m_نامها;  // For unique naming of structs and unions
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    Ast_گره_Module* m_classPackagep = اشاره_تهی;  // Package moving into
    ثابت AstScope* m_classScopep = اشاره_تهی;  // Package moving scopes into
    AstScope* m_packageScopep = اشاره_تهی;  // Class package scope
    ثابت Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Current task
    std::وکتور<std::pair<Ast_گره*, AstScope*>> m_مقصد_Scope_حرکتها;
    std::وکتور<std::pair<Ast_گره*, Ast_گره_Module*>> m_مقصد_Package_حرکتها;
    std::مجموعه<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::مجموعه<Ast_گره_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::صف<Ast_گره_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    فراغ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        إذا (nodep->use_Virtual_عمومی()) أرجع;  // Short-circuit to exit diamond cycles
        إذا (nodep->isInterfaceClass()) { nodep->use_Virtual_عمومی(درست); }
        لكل (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    فراغ بازدید(AstClass* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        // Move this class
        nodep->name(m_پیشوند + nodep->name());
        nodep->unlink_Fr_پشت();
        v3Global.rootp()->جمع_کن_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = جديد AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_شمار_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->جمع_کن_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = جديد AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           اشاره_تهی,
                                           اشاره_تهی,
                                           اشاره_تهی};
        cellp->modp(packagep);
        v3Global.rootp()->بالاترین_Modulep()->جمع_کن_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = اشاره_تهی;
        لكل (Ast_گره* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            إذا ((classScopep = VN_CAST(itp, Scope))) اكسر;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = جديد AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->جمع_کن_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_پیشوند);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_پیشوند = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_پیشوند);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_پیشوند = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    فراغ بازدید(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_packageScopep) {
            إذا (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_مقصد_Scope_حرکتها.emplace_پشت(nodep, m_packageScopep);
            }
            إذا (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_مقصد_Package_حرکتها.emplace_پشت(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_مقصد_Scope_حرکتها.emplace_پشت(nodep, m_packageScopep);
            }
        }
    }

    فراغ بازدید(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        إذا (m_packageScopep && nodep->isStatic()) {
            m_مقصد_Scope_حرکتها.emplace_پشت(nodep, m_packageScopep);
        }
    }
    فراغ بازدید(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    فراغ بازدید(Ast_گره_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جمع_کن_Stmtsp(nodep->unlink_Fr_پشت());
    }
    فراغ بازدید(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_مقصد_Scope_حرکتها.emplace_پشت(nodep, m_packageScopep);
    }
    فراغ بازدید(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_مقصد_Scope_حرکتها.emplace_پشت(nodep, m_packageScopep);
    }

    فراغ مجموعه_Struct_Modulep(Ast_گره_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_شمار_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_نامها.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        إذا (dtypep->packed()) m_strDtypeps.درج(dtypep);

        لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گره_UOr_Struct_DType* ثابت subp = itemp->get_فرزند_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            إذا (subp && subp->name().خالی()) مجموعه_Struct_Modulep(subp);
        }
    }
    فراغ بازدید(AstTypedef* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        Ast_گره_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), گره_UOr_Struct_DType);
        إذا (dtypep && dtypep->packed()) {
            m_typedefps.درج(nodep);
            إذا (nodep->attr_عمومی()) m_pubStrDtypeps.افزودن(dtypep);
        }
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->جمع_کن_Stmtsp(nodep->unlink_Fr_پشت());

        إذا (dtypep) {
            dtypep->name(nodep->name());
            مجموعه_Struct_Modulep(dtypep);
        }
    }

    فراغ بازدید(Ast_گره_عبارت* nodep) override {}  // Short circuit
    فراغ بازدید(Ast_گره_Stmt* nodep) override {}  // Short circuit
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        لكل (خودکار moved : m_مقصد_Scope_حرکتها) {
            Ast_گره* ثابت nodep = moved.اولی;
            AstScope* ثابت scopep = moved.دومی;
            UINFO(9, "moving " << nodep << " to " << scopep);
            إذا (VN_IS(nodep, گره_FTask)) {
                scopep->جمع_کن_Blocksp(nodep->unlink_Fr_پشت());
            } وإلا إذا (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_پشت();
                scopep->جمع_کن_Varsp(vscp);
            } وإلا إذا (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_پشت();
                scopep->جمع_کن_Blocksp(nodep);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
        }
        لكل (خودکار moved : m_مقصد_Package_حرکتها) {
            Ast_گره* ثابت nodep = moved.اولی;
            Ast_گره_Module* ثابت modp = moved.دومی;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_پشت();
            modp->جمع_کن_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::مجموعه<ثابت Ast_گره_UOr_Struct_DType*> pubStrDtypeps;
        طالما (!m_pubStrDtypeps.خالی()) {
            Ast_گره_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.برداشت();
            إذا (pubStrDtypeps.درج(dtypep).دومی) {
                لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    إذا (Ast_گره_UOr_Struct_DType* ثابت subp = itemp->get_فرزند_Structp())
                        m_pubStrDtypeps.افزودن(subp);
                }
            }
        }
        لكل (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), گره_UOr_Struct_DType);
            إذا (pubStrDtypeps.شمار(sdtypep)) typedefp->attr_عمومی(درست);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        لكل (Ast_گره_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            إذا (!pubStrDtypeps.شمار(sdtypep)) sdtypep->classOrPackagep(اشاره_تهی);
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
