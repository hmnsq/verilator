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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Class.h"

#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <queue>
#ಅನ್ತರ್ಭಾವ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ ClassVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    Ast_ಗಂಟು_Module* m_classPackagep = nullptr;  // Package moving into
    ಧ್ರುವ AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    ಧ್ರುವ Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_ಗಂಟು*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_ಗಂಟು*, Ast_ಗಂಟು_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_ಗಂಟು_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_ಗಂಟು_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    ಶೂನ್ಯ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        ಯದಿ (nodep->useVirtualPublic()) ಪ್ರತ್ಯಾಗಮ;  // Short-circuit to exit diamond cycles
        ಯದಿ (nodep->isInterfaceClass()) { nodep->useVirtualPublic(ಸತ್ಯ); }
        ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    ಶೂನ್ಯ visit(AstClass* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ಧ್ರುವ packagep
            = ನೂತನ AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_ಎಣಿಕೆ_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* ಧ್ರುವ cellp = ನೂತನ AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->ತುದಿ_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ಧ್ರುವ AstScope* classScopep = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            ಯದಿ ((classScopep = VN_CAST(itp, Scope))) ಭಙ್ಗ;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ಧ್ರುವ scopep
            = ನೂತನ AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_packageScopep) {
            ಯದಿ (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            ಯದಿ (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        ಯದಿ (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        ಯದಿ (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    ಶೂನ್ಯ visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ಯದಿ (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    ಶೂನ್ಯ visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ಯದಿ (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    ಶೂನ್ಯ setStructModulep(Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_ಎಣಿಕೆ_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        ಯದಿ (dtypep->packed()) m_strDtypeps.ಸೇರಿಸು(dtypep);

        ಪ್ರತಿ (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            ಯದಿ (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ dtypep = VN_CAST(nodep->dtypep(), ಗಂಟು_UOr_Struct_DType);
        ಯದಿ (dtypep && dtypep->packed()) {
            m_typedefps.ಸೇರಿಸು(nodep);
            ಯದಿ (nodep->attrPublic()) m_pubStrDtypeps.ತಳ್ಳು(dtypep);
        }
        iterateChildren(nodep);
        ಯದಿ (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        ಯದಿ (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}  // Short circuit
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {}  // Short circuit
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        ಪ್ರತಿ (auto moved : m_toScopeMoves) {
            Ast_ಗಂಟು* ಧ್ರುವ nodep = moved.first;
            AstScope* ಧ್ರುವ scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            ಯದಿ (VN_IS(nodep, ಗಂಟು_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Var)) {
                AstVarScope* ಧ್ರುವ vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Bad case");
            }
        }
        ಪ್ರತಿ (auto moved : m_toPackageMoves) {
            Ast_ಗಂಟು* ಧ್ರುವ nodep = moved.first;
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType*> pubStrDtypeps;
        ಯಾವತ್ (!m_pubStrDtypeps.empty()) {
            Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.ತೆಗೆ();
            ಯದಿ (pubStrDtypeps.ಸೇರಿಸು(dtypep).second) {
                ಪ್ರತಿ (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ subp = itemp->getChildStructp())
                        m_pubStrDtypeps.ತಳ್ಳು(subp);
                }
            }
        }
        ಪ್ರತಿ (AstTypedef* typedefp : m_typedefps) {
            ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep
                = VN_AS(typedefp->dtypep(), ಗಂಟು_UOr_Struct_DType);
            ಯದಿ (pubStrDtypeps.ಎಣಿಕೆ(sdtypep)) typedefp->attrPublic(ಸತ್ಯ);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        ಪ್ರತಿ (Ast_ಗಂಟು_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            ಯದಿ (!pubStrDtypeps.ಎಣಿಕೆ(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

ಶೂನ್ಯ V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("class", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
