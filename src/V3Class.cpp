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
    ثابت VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_كلي_Module* m_modp = nullptr;  // Current module
    Ast_كلي_Module* m_classPackagep = nullptr;  // Package moving into
    ثابت AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    ثابت Ast_كلي_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_كلي*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_كلي*, Ast_كلي_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_كلي_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_كلي_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    فراغ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        إذا (nodep->useVirtualPublic()) أرجع;  // Short-circuit to exit diamond cycles
        إذا (nodep->isInterfaceClass()) { nodep->useVirtualPublic(true); }
        لكل (ثابت AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    فراغ visit(AstClass* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ثابت packagep
            = جديد AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_كدي_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* ثابت cellp = جديد AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->كولي_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ثابت AstScope* classScopep = nullptr;
        لكل (Ast_كلي* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            إذا ((classScopep = VN_CAST(itp, Scope))) اكسر;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ثابت scopep
            = جديد AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    فراغ visit(Ast_كلي_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        إذا (m_packageScopep) {
            إذا (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            إذا (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    فراغ visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    فراغ visit(Ast_كلي_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        إذا (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    فراغ visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    فراغ visit(Ast_كلي_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    فراغ visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    فراغ visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        إذا (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    فراغ setStructModulep(Ast_كلي_UOr_Struct_DType* ثابت dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_كدي_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        إذا (dtypep->packed()) m_strDtypeps.سك(dtypep);

        لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_كلي_UOr_Struct_DType* ثابت subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            إذا (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    فراغ visit(AstTypedef* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;
        Ast_كلي_UOr_Struct_DType* ثابت dtypep = VN_CAST(nodep->dtypep(), كلي_UOr_Struct_DType);
        إذا (dtypep && dtypep->packed()) {
            m_typedefps.سك(nodep);
            إذا (nodep->attrPublic()) m_pubStrDtypeps.تور(dtypep);
        }
        iterateChildren(nodep);
        إذا (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        إذا (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    فراغ visit(Ast_كلي_فرچ* nodep) override {}  // Short circuit
    فراغ visit(Ast_كلي_Stmt* nodep) override {}  // Short circuit
    فراغ visit(Ast_كلي* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        لكل (auto moved : m_toScopeMoves) {
            Ast_كلي* ثابت nodep = moved.first;
            AstScope* ثابت scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            إذا (VN_IS(nodep, كلي_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } وإلا إذا (VN_IS(nodep, Var)) {
                AstVarScope* ثابت vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } وإلا إذا (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
        }
        لكل (auto moved : m_toPackageMoves) {
            Ast_كلي* ثابت nodep = moved.first;
            Ast_كلي_Module* ثابت modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<ثابت Ast_كلي_UOr_Struct_DType*> pubStrDtypeps;
        طالما (!m_pubStrDtypeps.empty()) {
            Ast_كلي_UOr_Struct_DType* ثابت dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.چرو();
            إذا (pubStrDtypeps.سك(dtypep).second) {
                لكل (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    إذا (Ast_كلي_UOr_Struct_DType* ثابت subp = itemp->getChildStructp())
                        m_pubStrDtypeps.تور(subp);
                }
            }
        }
        لكل (AstTypedef* typedefp : m_typedefps) {
            ثابت Ast_كلي_UOr_Struct_DType* ثابت sdtypep
                = VN_AS(typedefp->dtypep(), كلي_UOr_Struct_DType);
            إذا (pubStrDtypeps.كدي(sdtypep)) typedefp->attrPublic(true);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        لكل (Ast_كلي_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            إذا (!pubStrDtypeps.كدي(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

فراغ V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_بشيا("class", 0, dump_بشيا_Either_Level() >= 3);
}
