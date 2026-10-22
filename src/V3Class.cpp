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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Class.h"

#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <queue>
#અન્તર્ભાવ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ ClassVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    અચલ VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ગાંઠ_Module* m_classPackagep = nullptr;  // Package moving into
    અચલ AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    અચલ Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_ગાંઠ*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_ગાંઠ*, Ast_ગાંઠ_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_ગાંઠ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_ગાંઠ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    શૂન્ય recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        જો (nodep->useVirtualPublic()) પાછા;  // Short-circuit to exit diamond cycles
        જો (nodep->isInterfaceClass()) { nodep->useVirtualPublic(સાચું); }
        માટે (અચલ AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    શૂન્ય visit(AstClass* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* અચલ packagep
            = નવું AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_ગણતરી_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* અચલ cellp = નવું AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->ટોચ_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        અચલ AstScope* classScopep = nullptr;
        માટે (Ast_ગાંઠ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            જો ((classScopep = VN_CAST(itp, Scope))) તોડો;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* અચલ scopep
            = નવું AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstVar* nodep) override {
        iterateChildren(nodep);
        જો (m_packageScopep) {
            જો (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            જો (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    શૂન્ય visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        જો (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        જો (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    શૂન્ય visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        જો (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    શૂન્ય visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        જો (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    શૂન્ય setStructModulep(Ast_ગાંઠ_UOr_Struct_DType* અચલ dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_ગણતરી_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        જો (dtypep->packed()) m_strDtypeps.ઉમેરો(dtypep);

        માટે (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ગાંઠ_UOr_Struct_DType* અચલ subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            જો (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    શૂન્ય visit(AstTypedef* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        Ast_ગાંઠ_UOr_Struct_DType* અચલ dtypep = VN_CAST(nodep->dtypep(), ગાંઠ_UOr_Struct_DType);
        જો (dtypep && dtypep->packed()) {
            m_typedefps.ઉમેરો(nodep);
            જો (nodep->attrPublic()) m_pubStrDtypeps.ધકેલો(dtypep);
        }
        iterateChildren(nodep);
        જો (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        જો (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {}  // Short circuit
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {}  // Short circuit
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        માટે (auto moved : m_toScopeMoves) {
            Ast_ગાંઠ* અચલ nodep = moved.first;
            AstScope* અચલ scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            જો (VN_IS(nodep, ગાંઠ_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } નહીંતર જો (VN_IS(nodep, Var)) {
                AstVarScope* અચલ vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } નહીંતર જો (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } નહીંતર {
                nodep->v3fatalSrc("Bad case");
            }
        }
        માટે (auto moved : m_toPackageMoves) {
            Ast_ગાંઠ* અચલ nodep = moved.first;
            Ast_ગાંઠ_Module* અચલ modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<અચલ Ast_ગાંઠ_UOr_Struct_DType*> pubStrDtypeps;
        જ્યારે (!m_pubStrDtypeps.empty()) {
            Ast_ગાંઠ_UOr_Struct_DType* અચલ dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.કાઢો();
            જો (pubStrDtypeps.ઉમેરો(dtypep).second) {
                માટે (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ subp = itemp->getChildStructp())
                        m_pubStrDtypeps.ધકેલો(subp);
                }
            }
        }
        માટે (AstTypedef* typedefp : m_typedefps) {
            અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep
                = VN_AS(typedefp->dtypep(), ગાંઠ_UOr_Struct_DType);
            જો (pubStrDtypeps.ગણતરી(sdtypep)) typedefp->attrPublic(સાચું);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        માટે (Ast_ગાંઠ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            જો (!pubStrDtypeps.ગણતરી(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

શૂન્ય V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("class", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
