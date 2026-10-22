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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Class.h"

#समावेश "V3UniqueNames.h"

#समावेश <queue>
#समावेश <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी ClassVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    स्थिर VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_गाँठ_Module* m_classPackagep = nullptr;  // Package moving into
    स्थिर AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    स्थिर Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_गाँठ*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_गाँठ*, Ast_गाँठ_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_गाँठ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_गाँठ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    व्योम recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        अगर (nodep->useVirtualPublic()) वापस;  // Short-circuit to exit diamond cycles
        अगर (nodep->isInterfaceClass()) { nodep->useVirtualPublic(true); }
        क्रम (स्थिर AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    व्योम visit(AstClass* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* स्थिर packagep
            = नया AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_गिनती_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* स्थिर cellp = नया AstCell{packagep->fileline(),
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
        स्थिर AstScope* classScopep = nullptr;
        क्रम (Ast_गाँठ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            अगर ((classScopep = VN_CAST(itp, Scope))) अवरोध;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* स्थिर scopep
            = नया AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    व्योम visit(AstVar* nodep) override {
        iterateChildren(nodep);
        अगर (m_packageScopep) {
            अगर (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            अगर (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    व्योम visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        अगर (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    व्योम visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    व्योम visit(Ast_गाँठ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        अगर (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    व्योम visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        अगर (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    व्योम visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        अगर (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    व्योम setStructModulep(Ast_गाँठ_UOr_Struct_DType* स्थिर dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_गिनती_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.लो(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        अगर (dtypep->packed()) m_strDtypeps.जोड़ो(dtypep);

        क्रम (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाँठ_UOr_Struct_DType* स्थिर subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            अगर (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    व्योम visit(AstTypedef* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        Ast_गाँठ_UOr_Struct_DType* स्थिर dtypep = VN_CAST(nodep->dtypep(), गाँठ_UOr_Struct_DType);
        अगर (dtypep && dtypep->packed()) {
            m_typedefps.जोड़ो(nodep);
            अगर (nodep->attrPublic()) m_pubStrDtypeps.धकेलो(dtypep);
        }
        iterateChildren(nodep);
        अगर (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        अगर (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {}  // Short circuit
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {}  // Short circuit
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        क्रम (स्वतः moved : m_toScopeMoves) {
            Ast_गाँठ* स्थिर nodep = moved.first;
            AstScope* स्थिर scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            अगर (VN_IS(nodep, गाँठ_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } अन्यथा अगर (VN_IS(nodep, Var)) {
                AstVarScope* स्थिर vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } अन्यथा अगर (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
        }
        क्रम (स्वतः moved : m_toPackageMoves) {
            Ast_गाँठ* स्थिर nodep = moved.first;
            Ast_गाँठ_Module* स्थिर modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<स्थिर Ast_गाँठ_UOr_Struct_DType*> pubStrDtypeps;
        जबतक (!m_pubStrDtypeps.empty()) {
            Ast_गाँठ_UOr_Struct_DType* स्थिर dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.निकालो();
            अगर (pubStrDtypeps.जोड़ो(dtypep).second) {
                क्रम (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    अगर (Ast_गाँठ_UOr_Struct_DType* स्थिर subp = itemp->getChildStructp())
                        m_pubStrDtypeps.धकेलो(subp);
                }
            }
        }
        क्रम (AstTypedef* typedefp : m_typedefps) {
            स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर sdtypep
                = VN_AS(typedefp->dtypep(), गाँठ_UOr_Struct_DType);
            अगर (pubStrDtypeps.गिनती(sdtypep)) typedefp->attrPublic(true);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        क्रम (Ast_गाँठ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            अगर (!pubStrDtypeps.गिनती(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

व्योम V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("class", 0, dump_वृक्ष_Either_Level() >= 3);
}
