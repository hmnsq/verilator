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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Class.h"

#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <queue>
#अन्तर्भाव <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग ClassVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // MEMBERS
    string m_उपसर्गः;  // String prefix to add to class name based on hier
    V3_Unique_नामानि m_नामानि;  // For unique naming of structs and unions
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    Ast_ग्रन्थिः_Module* m_classPackagep = शून्यसूचकः;  // Package moving into
    ध्रुव AstScope* m_classScopep = शून्यसूचकः;  // Package moving scopes into
    AstScope* m_packageScopep = शून्यसूचकः;  // Class package scope
    ध्रुव Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current task
    std::सदिशम्<std::pair<Ast_ग्रन्थिः*, AstScope*>> m_गन्तव्यम्_Scope_Moves;
    std::सदिशम्<std::pair<Ast_ग्रन्थिः*, Ast_ग्रन्थिः_Module*>> m_गन्तव्यम्_Package_Moves;
    std::समुच्चयः<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::समुच्चयः<Ast_ग्रन्थिः_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_ग्रन्थिः_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    शून्य recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        यदि (nodep->use_Virtual_लौकिकम्()) प्रत्यागम;  // Short-circuit to exit diamond cycles
        यदि (nodep->isInterfaceClass()) { nodep->use_Virtual_लौकिकम्(सत्य); }
        प्रति (ध्रुव AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    शून्य दर्शय(AstClass* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        // Move this class
        nodep->name(m_उपसर्गः + nodep->name());
        nodep->unlink_Fr_पृष्ठभागः();
        v3Global.rootp()->सङ्कलय_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ध्रुव packagep
            = नूतन AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_गणनम्_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->सङ्कलय_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ध्रुव cellp = नूतन AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           शून्यसूचकः,
                                           शून्यसूचकः,
                                           शून्यसूचकः};
        cellp->modp(packagep);
        v3Global.rootp()->शिखरम्_Modulep()->सङ्कलय_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ध्रुव AstScope* classScopep = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            यदि ((classScopep = VN_CAST(itp, Scope))) भङ्ग;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ध्रुव scopep
            = नूतन AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->सङ्कलय_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_उपसर्गः);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_उपसर्गः = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_उपसर्गः);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_उपसर्गः = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstVar* nodep) override {
        iterateChildren(nodep);
        यदि (m_packageScopep) {
            यदि (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_गन्तव्यम्_Scope_Moves.emplace_पृष्ठभागः(nodep, m_packageScopep);
            }
            यदि (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_गन्तव्यम्_Package_Moves.emplace_पृष्ठभागः(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_गन्तव्यम्_Scope_Moves.emplace_पृष्ठभागः(nodep, m_packageScopep);
            }
        }
    }

    शून्य दर्शय(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        यदि (m_packageScopep && nodep->isStatic()) {
            m_गन्तव्यम्_Scope_Moves.emplace_पृष्ठभागः(nodep, m_packageScopep);
        }
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        यदि (m_classPackagep) m_classPackagep->सङ्कलय_Stmtsp(nodep->unlink_Fr_पृष्ठभागः());
    }
    शून्य दर्शय(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        यदि (m_packageScopep) m_गन्तव्यम्_Scope_Moves.emplace_पृष्ठभागः(nodep, m_packageScopep);
    }
    शून्य दर्शय(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        यदि (m_packageScopep) m_गन्तव्यम्_Scope_Moves.emplace_पृष्ठभागः(nodep, m_packageScopep);
    }

    शून्य समुच्चयः_Struct_Modulep(Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_गणनम्_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_नामानि.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        यदि (dtypep->packed()) m_strDtypeps.निवेशय(dtypep);

        प्रति (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            यदि (subp && subp->name().रिक्तानि()) समुच्चयः_Struct_Modulep(subp);
        }
    }
    शून्य दर्शय(AstTypedef* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtypep = VN_CAST(nodep->dtypep(), ग्रन्थिः_UOr_Struct_DType);
        यदि (dtypep && dtypep->packed()) {
            m_typedefps.निवेशय(nodep);
            यदि (nodep->attr_लौकिकम्()) m_pubStrDtypeps.प्रक्षिप(dtypep);
        }
        iterateChildren(nodep);
        यदि (m_classPackagep) m_classPackagep->सङ्कलय_Stmtsp(nodep->unlink_Fr_पृष्ठभागः());

        यदि (dtypep) {
            dtypep->name(nodep->name());
            समुच्चयः_Struct_Modulep(dtypep);
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {}  // Short circuit
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {}  // Short circuit
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        प्रति (स्वचालित moved : m_गन्तव्यम्_Scope_Moves) {
            Ast_ग्रन्थिः* ध्रुव nodep = moved.first;
            AstScope* ध्रुव scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            यदि (VN_IS(nodep, ग्रन्थिः_FTask)) {
                scopep->सङ्कलय_Blocksp(nodep->unlink_Fr_पृष्ठभागः());
            } अन्यथा यदि (VN_IS(nodep, Var)) {
                AstVarScope* ध्रुव vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_पृष्ठभागः();
                scopep->सङ्कलय_Varsp(vscp);
            } अन्यथा यदि (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_पृष्ठभागः();
                scopep->सङ्कलय_Blocksp(nodep);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
        }
        प्रति (स्वचालित moved : m_गन्तव्यम्_Package_Moves) {
            Ast_ग्रन्थिः* ध्रुव nodep = moved.first;
            Ast_ग्रन्थिः_Module* ध्रुव modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_पृष्ठभागः();
            modp->सङ्कलय_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::समुच्चयः<ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType*> pubStrDtypeps;
        यावत् (!m_pubStrDtypeps.रिक्तानि()) {
            Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.निष्कासय();
            यदि (pubStrDtypeps.निवेशय(dtypep).second) {
                प्रति (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    यदि (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp())
                        m_pubStrDtypeps.प्रक्षिप(subp);
                }
            }
        }
        प्रति (AstTypedef* typedefp : m_typedefps) {
            ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep
                = VN_AS(typedefp->dtypep(), ग्रन्थिः_UOr_Struct_DType);
            यदि (pubStrDtypeps.गणनम्(sdtypep)) typedefp->attr_लौकिकम्(सत्य);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        प्रति (Ast_ग्रन्थिः_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            यदि (!pubStrDtypeps.गणनम्(sdtypep)) sdtypep->classOrPackagep(शून्यसूचकः);
        }
    }
};

//######################################################################
// Class class functions

शून्य V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("class", 0, dump_वृक्षः_Either_Level() >= 3);
}
