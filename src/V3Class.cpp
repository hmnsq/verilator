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
    स्थिरांक VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    Ast_गाठ_Module* m_classPackagep = nullptr;  // Package moving into
    स्थिरांक AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    स्थिरांक Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_गाठ*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_गाठ*, Ast_गाठ_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_गाठ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_गाठ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    शून्य recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        जर (nodep->useVirtualPublic()) परत;  // Short-circuit to exit diamond cycles
        जर (nodep->isInterfaceClass()) { nodep->useVirtualPublic(सत्य); }
        साठी (स्थिरांक AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    शून्य visit(AstClass* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* स्थिरांक packagep
            = नवीन AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_मोजणी_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* स्थिरांक cellp = नवीन AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->शीर्ष_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        स्थिरांक AstScope* classScopep = nullptr;
        साठी (Ast_गाठ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            जर ((classScopep = VN_CAST(itp, Scope))) थांबा;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* स्थिरांक scopep
            = नवीन AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    शून्य visit(Ast_गाठ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        जर (m_packageScopep) {
            जर (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            जर (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    शून्य visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        जर (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    शून्य visit(Ast_गाठ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        जर (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        जर (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        जर (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    शून्य setStructModulep(Ast_गाठ_UOr_Struct_DType* स्थिरांक dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_मोजणी_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        जर (dtypep->packed()) m_strDtypeps.घाला(dtypep);

        साठी (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाठ_UOr_Struct_DType* स्थिरांक subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            जर (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        Ast_गाठ_UOr_Struct_DType* स्थिरांक dtypep = VN_CAST(nodep->dtypep(), गाठ_UOr_Struct_DType);
        जर (dtypep && dtypep->packed()) {
            m_typedefps.घाला(nodep);
            जर (nodep->attrPublic()) m_pubStrDtypeps.ढकला(dtypep);
        }
        iterateChildren(nodep);
        जर (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        जर (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {}  // Short circuit
    शून्य visit(Ast_गाठ_Stmt* nodep) override {}  // Short circuit
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        साठी (स्वतः moved : m_toScopeMoves) {
            Ast_गाठ* स्थिरांक nodep = moved.first;
            AstScope* स्थिरांक scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            जर (VN_IS(nodep, गाठ_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } अन्यथा जर (VN_IS(nodep, Var)) {
                AstVarScope* स्थिरांक vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } अन्यथा जर (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
        }
        साठी (स्वतः moved : m_toPackageMoves) {
            Ast_गाठ* स्थिरांक nodep = moved.first;
            Ast_गाठ_Module* स्थिरांक modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<स्थिरांक Ast_गाठ_UOr_Struct_DType*> pubStrDtypeps;
        पर्यंत (!m_pubStrDtypeps.empty()) {
            Ast_गाठ_UOr_Struct_DType* स्थिरांक dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.काढा();
            जर (pubStrDtypeps.घाला(dtypep).second) {
                साठी (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    जर (Ast_गाठ_UOr_Struct_DType* स्थिरांक subp = itemp->getChildStructp())
                        m_pubStrDtypeps.ढकला(subp);
                }
            }
        }
        साठी (AstTypedef* typedefp : m_typedefps) {
            स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक sdtypep
                = VN_AS(typedefp->dtypep(), गाठ_UOr_Struct_DType);
            जर (pubStrDtypeps.मोजणी(sdtypep)) typedefp->attrPublic(सत्य);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        साठी (Ast_गाठ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            जर (!pubStrDtypeps.मोजणी(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

शून्य V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("class", 0, dump_वृक्ष_Either_Level() >= 3);
}
