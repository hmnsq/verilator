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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Class.h"

#شامل "V3UniqueNames.h"

#شامل <queue>
#شامل <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت ClassVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ثابت VNUser1_اندر_Use m_inuser1;

    // MEMBERS
    string m_سابقہ;  // String prefix to add to class name based on hier
    V3_Unique_نام m_نام;  // For unique naming of structs and unions
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_گرہ_Module* m_classPackagep = خالی_اشاریہ;  // Package moving into
    ثابت AstScope* m_classScopep = خالی_اشاریہ;  // Package moving scopes into
    AstScope* m_packageScopep = خالی_اشاریہ;  // Class package scope
    ثابت Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current task
    std::سمتیہ<std::pair<Ast_گرہ*, AstScope*>> m_مقصد_Scope_چالیں;
    std::سمتیہ<std::pair<Ast_گرہ*, Ast_گرہ_Module*>> m_مقصد_Package_چالیں;
    std::سیٹ<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::سیٹ<Ast_گرہ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::قطار<Ast_گرہ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    خالی recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        اگر (nodep->use_Virtual_عوامی()) واپس;  // Short-circuit to exit diamond cycles
        اگر (nodep->isInterfaceClass()) { nodep->use_Virtual_عوامی(سچ); }
        برائے (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    خالی دورہ(AstClass* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        // Move this class
        nodep->name(m_سابقہ + nodep->name());
        nodep->unlink_Fr_عقب();
        v3Global.rootp()->جوڑو_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = نیا AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_گنتی_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->جوڑو_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = نیا AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           خالی_اشاریہ,
                                           خالی_اشاریہ,
                                           خالی_اشاریہ};
        cellp->modp(packagep);
        v3Global.rootp()->چوٹی_Modulep()->جوڑو_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = خالی_اشاریہ;
        برائے (Ast_گرہ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            اگر ((classScopep = VN_CAST(itp, Scope))) توڑو;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = نیا AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->جوڑو_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_سابقہ);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_سابقہ = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_سابقہ);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_سابقہ = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    خالی دورہ(AstVar* nodep) override {
        iterateChildren(nodep);
        اگر (m_packageScopep) {
            اگر (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_مقصد_Scope_چالیں.emplace_عقب(nodep, m_packageScopep);
            }
            اگر (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_مقصد_Package_چالیں.emplace_عقب(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_مقصد_Scope_چالیں.emplace_عقب(nodep, m_packageScopep);
            }
        }
    }

    خالی دورہ(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        اگر (m_packageScopep && nodep->isStatic()) {
            m_مقصد_Scope_چالیں.emplace_عقب(nodep, m_packageScopep);
        }
    }
    خالی دورہ(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    خالی دورہ(Ast_گرہ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        اگر (m_classPackagep) m_classPackagep->جوڑو_Stmtsp(nodep->unlink_Fr_عقب());
    }
    خالی دورہ(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        اگر (m_packageScopep) m_مقصد_Scope_چالیں.emplace_عقب(nodep, m_packageScopep);
    }
    خالی دورہ(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        اگر (m_packageScopep) m_مقصد_Scope_چالیں.emplace_عقب(nodep, m_packageScopep);
    }

    خالی سیٹ_Struct_Modulep(Ast_گرہ_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_گنتی_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_نام.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        اگر (dtypep->packed()) m_strDtypeps.داخل_کرو(dtypep);

        برائے (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گرہ_UOr_Struct_DType* ثابت subp = itemp->get_بچہ_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            اگر (subp && subp->name().خالی_خانے()) سیٹ_Struct_Modulep(subp);
        }
    }
    خالی دورہ(AstTypedef* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        Ast_گرہ_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), گرہ_UOr_Struct_DType);
        اگر (dtypep && dtypep->packed()) {
            m_typedefps.داخل_کرو(nodep);
            اگر (nodep->attr_عوامی()) m_pubStrDtypeps.ڈالو(dtypep);
        }
        iterateChildren(nodep);
        اگر (m_classPackagep) m_classPackagep->جوڑو_Stmtsp(nodep->unlink_Fr_عقب());

        اگر (dtypep) {
            dtypep->name(nodep->name());
            سیٹ_Struct_Modulep(dtypep);
        }
    }

    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {}  // Short circuit
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {}  // Short circuit
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        برائے (خودکار moved : m_مقصد_Scope_چالیں) {
            Ast_گرہ* ثابت nodep = moved.پہلا;
            AstScope* ثابت scopep = moved.دوسرا;
            UINFO(9, "moving " << nodep << " to " << scopep);
            اگر (VN_IS(nodep, گرہ_FTask)) {
                scopep->جوڑو_Blocksp(nodep->unlink_Fr_عقب());
            } ورنہ اگر (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_عقب();
                scopep->جوڑو_Varsp(vscp);
            } ورنہ اگر (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_عقب();
                scopep->جوڑو_Blocksp(nodep);
            } ورنہ {
                nodep->v3fatalSrc("Bad case");
            }
        }
        برائے (خودکار moved : m_مقصد_Package_چالیں) {
            Ast_گرہ* ثابت nodep = moved.پہلا;
            Ast_گرہ_Module* ثابت modp = moved.دوسرا;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_عقب();
            modp->جوڑو_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::سیٹ<ثابت Ast_گرہ_UOr_Struct_DType*> pubStrDtypeps;
        جب (!m_pubStrDtypeps.خالی_خانے()) {
            Ast_گرہ_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.نکالو();
            اگر (pubStrDtypeps.داخل_کرو(dtypep).دوسرا) {
                برائے (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    اگر (Ast_گرہ_UOr_Struct_DType* ثابت subp = itemp->get_بچہ_Structp())
                        m_pubStrDtypeps.ڈالو(subp);
                }
            }
        }
        برائے (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_گرہ_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), گرہ_UOr_Struct_DType);
            اگر (pubStrDtypeps.گنتی(sdtypep)) typedefp->attr_عوامی(سچ);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        برائے (Ast_گرہ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            اگر (!pubStrDtypeps.گنتی(sdtypep)) sdtypep->classOrPackagep(خالی_اشاریہ);
        }
    }
};

//######################################################################
// Class class functions

خالی V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("class", 0, dump_درخت_Either_Level() >= 3);
}
