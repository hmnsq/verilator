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

رده ClassVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ثابت VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_گره_Module* m_modp = nullptr;  // Current module
    Ast_گره_Module* m_classPackagep = nullptr;  // Package moving into
    ثابت AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    ثابت Ast_گره_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_گره*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_گره*, Ast_گره_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_گره_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_گره_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    تهی recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        اگر (nodep->useVirtualPublic()) بازگشت;  // Short-circuit to exit diamond cycles
        اگر (nodep->isInterfaceClass()) { nodep->useVirtualPublic(درست); }
        برای (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    تهی visit(AstClass* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = جدید AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_شمار_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = جدید AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->بالاترین_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = nullptr;
        برای (Ast_گره* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            اگر ((classScopep = VN_CAST(itp, Scope))) بشکن;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = جدید AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->addStmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_prefix = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    تهی visit(AstVar* nodep) override {
        iterateChildren(nodep);
        اگر (m_packageScopep) {
            اگر (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            اگر (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    تهی visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        اگر (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    تهی visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    تهی visit(Ast_گره_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        اگر (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    تهی visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        اگر (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    تهی visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        اگر (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    تهی setStructModulep(Ast_گره_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_شمار_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        اگر (dtypep->packed()) m_strDtypeps.درج(dtypep);

        برای (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گره_UOr_Struct_DType* ثابت subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            اگر (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    تهی visit(AstTypedef* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        Ast_گره_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), گره_UOr_Struct_DType);
        اگر (dtypep && dtypep->packed()) {
            m_typedefps.درج(nodep);
            اگر (nodep->attrPublic()) m_pubStrDtypeps.افزودن(dtypep);
        }
        iterateChildren(nodep);
        اگر (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        اگر (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    تهی visit(Ast_گره_عبارت* nodep) override {}  // Short circuit
    تهی visit(Ast_گره_Stmt* nodep) override {}  // Short circuit
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        برای (auto moved : m_toScopeMoves) {
            Ast_گره* ثابت nodep = moved.first;
            AstScope* ثابت scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            اگر (VN_IS(nodep, گره_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } وگرنه اگر (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } وگرنه اگر (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } وگرنه {
                nodep->v3fatalSrc("Bad case");
            }
        }
        برای (auto moved : m_toPackageMoves) {
            Ast_گره* ثابت nodep = moved.first;
            Ast_گره_Module* ثابت modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<ثابت Ast_گره_UOr_Struct_DType*> pubStrDtypeps;
        وقتی (!m_pubStrDtypeps.empty()) {
            Ast_گره_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.برداشت();
            اگر (pubStrDtypeps.درج(dtypep).second) {
                برای (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    اگر (Ast_گره_UOr_Struct_DType* ثابت subp = itemp->getChildStructp())
                        m_pubStrDtypeps.افزودن(subp);
                }
            }
        }
        برای (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_گره_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), گره_UOr_Struct_DType);
            اگر (pubStrDtypeps.شمار(sdtypep)) typedefp->attrPublic(درست);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        برای (Ast_گره_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            اگر (!pubStrDtypeps.شمار(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

تهی V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("class", 0, dump_درخت_Either_Level() >= 3);
}
