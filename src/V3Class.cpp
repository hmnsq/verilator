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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // MEMBERS
    string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // String prefix to add to class name based on hier
    V3_Unique_ಹೆಸರುಗಳು m_ಹೆಸರುಗಳು;  // For unique naming of structs and unions
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_ಗಂಟು_Module* m_classPackagep = ಶೂನ್ಯಸೂಚಕ;  // Package moving into
    ಸ್ಥಿರಾಂಕ AstScope* m_classScopep = ಶೂನ್ಯಸೂಚಕ;  // Package moving scopes into
    AstScope* m_packageScopep = ಶೂನ್ಯಸೂಚಕ;  // Class package scope
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current task
    std::ಸದಿಶ<std::pair<Ast_ಗಂಟು*, AstScope*>> m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು;
    std::ಸದಿಶ<std::pair<Ast_ಗಂಟು*, Ast_ಗಂಟು_Module*>> m_ಗಮ್ಯ_Package_ಚಲನೆಗಳು;
    std::ಗಣ<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::ಗಣ<Ast_ಗಂಟು_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::ಸರತಿ<Ast_ಗಂಟು_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    ಶೂನ್ಯ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        ಆದರೆ (nodep->use_Virtual_ಬಹಿರಂಗ()) ಹಿಂತಿರುಗಿ;  // Short-circuit to exit diamond cycles
        ಆದರೆ (nodep->isInterfaceClass()) { nodep->use_Virtual_ಬಹಿರಂಗ(ನಿಜ); }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        // Move this class
        nodep->name(m_ಪೂರ್ವಪ್ರತ್ಯಯ + nodep->name());
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        v3Global.rootp()->ಕೂಡಿಸು_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ಸ್ಥಿರಾಂಕ packagep
            = ಹೊಸ AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_ಎಣಿಕೆ_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->ಕೂಡಿಸು_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ಸ್ಥಿರಾಂಕ cellp = ಹೊಸ AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           ಶೂನ್ಯಸೂಚಕ,
                                           ಶೂನ್ಯಸೂಚಕ,
                                           ಶೂನ್ಯಸೂಚಕ};
        cellp->modp(packagep);
        v3Global.rootp()->ತುದಿ_Modulep()->ಕೂಡಿಸು_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ಸ್ಥಿರಾಂಕ AstScope* classScopep = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            ಆದರೆ ((classScopep = VN_CAST(itp, Scope))) ನಿಲ್ಲಿಸು;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ಸ್ಥಿರಾಂಕ scopep
            = ಹೊಸ AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->ಕೂಡಿಸು_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_ಪೂರ್ವಪ್ರತ್ಯಯ);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_ಪೂರ್ವಪ್ರತ್ಯಯ = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_ಪೂರ್ವಪ್ರತ್ಯಯ);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_ಪೂರ್ವಪ್ರತ್ಯಯ = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_packageScopep) {
            ಆದರೆ (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು.emplace_ಹಿಂಭಾಗ(nodep, m_packageScopep);
            }
            ಆದರೆ (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_ಗಮ್ಯ_Package_ಚಲನೆಗಳು.emplace_ಹಿಂಭಾಗ(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು.emplace_ಹಿಂಭಾಗ(nodep, m_packageScopep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        ಆದರೆ (m_packageScopep && nodep->isStatic()) {
            m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು.emplace_ಹಿಂಭಾಗ(nodep, m_packageScopep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        ಆದರೆ (m_classPackagep) m_classPackagep->ಕೂಡಿಸು_Stmtsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ಆದರೆ (m_packageScopep) m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು.emplace_ಹಿಂಭಾಗ(nodep, m_packageScopep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ಆದರೆ (m_packageScopep) m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು.emplace_ಹಿಂಭಾಗ(nodep, m_packageScopep);
    }

    ಶೂನ್ಯ ಗಣ_Struct_Modulep(Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_ಎಣಿಕೆ_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_ಹೆಸರುಗಳು.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        ಆದರೆ (dtypep->packed()) m_strDtypeps.ಸೇರಿಸು(dtypep);

        ಗಾಗಿ (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ subp = itemp->get_ಮಗು_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            ಆದರೆ (subp && subp->name().ಖಾಲಿಗಳು()) ಗಣ_Struct_Modulep(subp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep->dtypep(), ಗಂಟು_UOr_Struct_DType);
        ಆದರೆ (dtypep && dtypep->packed()) {
            m_typedefps.ಸೇರಿಸು(nodep);
            ಆದರೆ (nodep->attr_ಬಹಿರಂಗ()) m_pubStrDtypeps.ತಳ್ಳು(dtypep);
        }
        iterateChildren(nodep);
        ಆದರೆ (m_classPackagep) m_classPackagep->ಕೂಡಿಸು_Stmtsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());

        ಆದರೆ (dtypep) {
            dtypep->name(nodep->name());
            ಗಣ_Struct_Modulep(dtypep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}  // Short circuit
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {}  // Short circuit
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ moved : m_ಗಮ್ಯ_Scope_ಚಲನೆಗಳು) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = moved.ಮೊದಲ;
            AstScope* ಸ್ಥಿರಾಂಕ scopep = moved.ಎರಡನೇ;
            UINFO(9, "moving " << nodep << " to " << scopep);
            ಆದರೆ (VN_IS(nodep, ಗಂಟು_FTask)) {
                scopep->ಕೂಡಿಸು_Blocksp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Var)) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_ಹಿಂಭಾಗ();
                scopep->ಕೂಡಿಸು_Varsp(vscp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
                scopep->ಕೂಡಿಸು_Blocksp(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Bad case");
            }
        }
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ moved : m_ಗಮ್ಯ_Package_ಚಲನೆಗಳು) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = moved.ಮೊದಲ;
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = moved.ಎರಡನೇ;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            modp->ಕೂಡಿಸು_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType*> pubStrDtypeps;
        ಆಗ (!m_pubStrDtypeps.ಖಾಲಿಗಳು()) {
            Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.ತೆಗೆ_2();
            ಆದರೆ (pubStrDtypeps.ಸೇರಿಸು(dtypep).ಎರಡನೇ) {
                ಗಾಗಿ (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ಆದರೆ (Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ subp = itemp->get_ಮಗು_Structp())
                        m_pubStrDtypeps.ತಳ್ಳು(subp);
                }
            }
        }
        ಗಾಗಿ (AstTypedef* typedefp : m_typedefps) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ sdtypep
                = VN_AS(typedefp->dtypep(), ಗಂಟು_UOr_Struct_DType);
            ಆದರೆ (pubStrDtypeps.ಎಣಿಕೆ_2(sdtypep)) typedefp->attr_ಬಹಿರಂಗ(ನಿಜ);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        ಗಾಗಿ (Ast_ಗಂಟು_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            ಆದರೆ (!pubStrDtypeps.ಎಣಿಕೆ_2(sdtypep)) sdtypep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
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
