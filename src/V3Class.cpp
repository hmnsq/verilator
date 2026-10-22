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
    ध्रुव VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_गाँठ_Module* m_classPackagep = nullptr;  // Package moving into
    ध्रुव AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    ध्रुव Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_गाँठ*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_गाँठ*, Ast_गाँठ_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_गाँठ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_गाँठ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    शून्य recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        यदि (nodep->useVirtualPublic()) प्रत्यागम;  // Short-circuit to exit diamond cycles
        यदि (nodep->isInterfaceClass()) { nodep->useVirtualPublic(सत्य); }
        प्रति (ध्रुव AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    शून्य visit(AstClass* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ध्रुव packagep
            = नूतन AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_गिनती_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* ध्रुव cellp = नूतन AstCell{packagep->fileline(),
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
        ध्रुव AstScope* classScopep = nullptr;
        प्रति (Ast_गाँठ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            यदि ((classScopep = VN_CAST(itp, Scope))) भङ्ग;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ध्रुव scopep
            = नूतन AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    शून्य visit(Ast_गाँठ_Module* nodep) override {
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
        यदि (m_packageScopep) {
            यदि (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            यदि (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
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

    शून्य visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        यदि (m_packageScopep && nodep->isStatic()) {
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
    शून्य visit(Ast_गाँठ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        यदि (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    शून्य visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        यदि (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        यदि (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    शून्य setStructModulep(Ast_गाँठ_UOr_Struct_DType* ध्रुव dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_गिनती_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        यदि (dtypep->packed()) m_strDtypeps.जोड़ो(dtypep);

        प्रति (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाँठ_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            यदि (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;
        Ast_गाँठ_UOr_Struct_DType* ध्रुव dtypep = VN_CAST(nodep->dtypep(), गाँठ_UOr_Struct_DType);
        यदि (dtypep && dtypep->packed()) {
            m_typedefps.जोड़ो(nodep);
            यदि (nodep->attrPublic()) m_pubStrDtypeps.धकेलो(dtypep);
        }
        iterateChildren(nodep);
        यदि (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        यदि (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    शून्य visit(Ast_गाँठ_व्यंजक* nodep) override {}  // Short circuit
    शून्य visit(Ast_गाँठ_Stmt* nodep) override {}  // Short circuit
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        प्रति (स्वतः moved : m_toScopeMoves) {
            Ast_गाँठ* ध्रुव nodep = moved.first;
            AstScope* ध्रुव scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            यदि (VN_IS(nodep, गाँठ_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } अन्यथा यदि (VN_IS(nodep, Var)) {
                AstVarScope* ध्रुव vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } अन्यथा यदि (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
        }
        प्रति (स्वतः moved : m_toPackageMoves) {
            Ast_गाँठ* ध्रुव nodep = moved.first;
            Ast_गाँठ_Module* ध्रुव modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<ध्रुव Ast_गाँठ_UOr_Struct_DType*> pubStrDtypeps;
        यावत् (!m_pubStrDtypeps.empty()) {
            Ast_गाँठ_UOr_Struct_DType* ध्रुव dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.निकालो();
            यदि (pubStrDtypeps.जोड़ो(dtypep).second) {
                प्रति (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    यदि (Ast_गाँठ_UOr_Struct_DType* ध्रुव subp = itemp->getChildStructp())
                        m_pubStrDtypeps.धकेलो(subp);
                }
            }
        }
        प्रति (AstTypedef* typedefp : m_typedefps) {
            ध्रुव Ast_गाँठ_UOr_Struct_DType* ध्रुव sdtypep
                = VN_AS(typedefp->dtypep(), गाँठ_UOr_Struct_DType);
            यदि (pubStrDtypeps.गिनती(sdtypep)) typedefp->attrPublic(सत्य);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        प्रति (Ast_गाँठ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            यदि (!pubStrDtypeps.गिनती(sdtypep)) sdtypep->classOrPackagep(nullptr);
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
