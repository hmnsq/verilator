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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Class.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <queue>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ ClassVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // MEMBERS
    string m_ਅਗੇਤਰ;  // String prefix to add to class name based on hier
    V3_Unique_ਨਾਂ m_ਨਾਂ;  // For unique naming of structs and unions
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਗੰਢ_Module* m_classPackagep = ਖਾਲੀ_ਸੂਚਕ;  // Package moving into
    ਅਚਲ AstScope* m_classScopep = ਖਾਲੀ_ਸੂਚਕ;  // Package moving scopes into
    AstScope* m_packageScopep = ਖਾਲੀ_ਸੂਚਕ;  // Class package scope
    ਅਚਲ Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current task
    std::ਸਦਿਸ਼<std::pair<Ast_ਗੰਢ*, AstScope*>> m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ;
    std::ਸਦਿਸ਼<std::pair<Ast_ਗੰਢ*, Ast_ਗੰਢ_Module*>> m_ਮੰਜ਼ਿਲ_Package_ਚਾਲਾਂ;
    std::ਸੈੱਟ<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::ਸੈੱਟ<Ast_ਗੰਢ_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::ਕਤਾਰ<Ast_ਗੰਢ_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    ਖਾਲੀ recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        ਜੇ (nodep->use_Virtual_ਜਨਤਕ()) ਵਾਪਸ;  // Short-circuit to exit diamond cycles
        ਜੇ (nodep->isInterfaceClass()) { nodep->use_Virtual_ਜਨਤਕ(ਸੱਚ); }
        ਲਈ (ਅਚਲ AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        // Move this class
        nodep->name(m_ਅਗੇਤਰ + nodep->name());
        nodep->unlink_Fr_ਪਿੱਠ();
        v3Global.rootp()->ਜੋੜੋ_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ਅਚਲ packagep
            = ਨਵਾਂ AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_ਗਿਣਤੀ_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->ਜੋੜੋ_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* ਅਚਲ cellp = ਨਵਾਂ AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           ਖਾਲੀ_ਸੂਚਕ,
                                           ਖਾਲੀ_ਸੂਚਕ,
                                           ਖਾਲੀ_ਸੂਚਕ};
        cellp->modp(packagep);
        v3Global.rootp()->ਸਿਖਰ_Modulep()->ਜੋੜੋ_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ਅਚਲ AstScope* classScopep = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            ਜੇ ((classScopep = VN_CAST(itp, Scope))) ਰੋਕੋ;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ਅਚਲ scopep
            = ਨਵਾਂ AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->ਜੋੜੋ_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_ਅਗੇਤਰ);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_ਅਗੇਤਰ = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_ਅਗੇਤਰ);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_ਅਗੇਤਰ = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_packageScopep) {
            ਜੇ (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ.emplace_ਪਿੱਠ(nodep, m_packageScopep);
            }
            ਜੇ (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_ਮੰਜ਼ਿਲ_Package_ਚਾਲਾਂ.emplace_ਪਿੱਠ(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ.emplace_ਪਿੱਠ(nodep, m_packageScopep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        ਜੇ (m_packageScopep && nodep->isStatic()) {
            m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ.emplace_ਪਿੱਠ(nodep, m_packageScopep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        ਜੇ (m_classPackagep) m_classPackagep->ਜੋੜੋ_Stmtsp(nodep->unlink_Fr_ਪਿੱਠ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ਜੇ (m_packageScopep) m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ.emplace_ਪਿੱਠ(nodep, m_packageScopep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ਜੇ (m_packageScopep) m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ.emplace_ਪਿੱਠ(nodep, m_packageScopep);
    }

    ਖਾਲੀ ਸੈੱਟ_Struct_Modulep(Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_ਗਿਣਤੀ_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_ਨਾਂ.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        ਜੇ (dtypep->packed()) m_strDtypeps.ਪਾਓ(dtypep);

        ਲਈ (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ subp = itemp->get_ਬੱਚਾ_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            ਜੇ (subp && subp->name().ਖਾਲੀ_ਥਾਵਾਂ()) ਸੈੱਟ_Struct_Modulep(subp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ dtypep = VN_CAST(nodep->dtypep(), ਗੰਢ_UOr_Struct_DType);
        ਜੇ (dtypep && dtypep->packed()) {
            m_typedefps.ਪਾਓ(nodep);
            ਜੇ (nodep->attr_ਜਨਤਕ()) m_pubStrDtypeps.ਧੱਕੋ(dtypep);
        }
        iterateChildren(nodep);
        ਜੇ (m_classPackagep) m_classPackagep->ਜੋੜੋ_Stmtsp(nodep->unlink_Fr_ਪਿੱਠ());

        ਜੇ (dtypep) {
            dtypep->name(nodep->name());
            ਸੈੱਟ_Struct_Modulep(dtypep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {}  // Short circuit
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {}  // Short circuit
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        ਲਈ (ਸਵੈਚਾਲਿਤ moved : m_ਮੰਜ਼ਿਲ_Scope_ਚਾਲਾਂ) {
            Ast_ਗੰਢ* ਅਚਲ nodep = moved.ਪਹਿਲਾ;
            AstScope* ਅਚਲ scopep = moved.ਦੂਜਾ;
            UINFO(9, "moving " << nodep << " to " << scopep);
            ਜੇ (VN_IS(nodep, ਗੰਢ_FTask)) {
                scopep->ਜੋੜੋ_Blocksp(nodep->unlink_Fr_ਪਿੱਠ());
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Var)) {
                AstVarScope* ਅਚਲ vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_ਪਿੱਠ();
                scopep->ਜੋੜੋ_Varsp(vscp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_ਪਿੱਠ();
                scopep->ਜੋੜੋ_Blocksp(nodep);
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Bad case");
            }
        }
        ਲਈ (ਸਵੈਚਾਲਿਤ moved : m_ਮੰਜ਼ਿਲ_Package_ਚਾਲਾਂ) {
            Ast_ਗੰਢ* ਅਚਲ nodep = moved.ਪਹਿਲਾ;
            Ast_ਗੰਢ_Module* ਅਚਲ modp = moved.ਦੂਜਾ;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_ਪਿੱਠ();
            modp->ਜੋੜੋ_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType*> pubStrDtypeps;
        ਜਦੋਂ (!m_pubStrDtypeps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.ਕੱਢੋ();
            ਜੇ (pubStrDtypeps.ਪਾਓ(dtypep).ਦੂਜਾ) {
                ਲਈ (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ subp = itemp->get_ਬੱਚਾ_Structp())
                        m_pubStrDtypeps.ਧੱਕੋ(subp);
                }
            }
        }
        ਲਈ (AstTypedef* typedefp : m_typedefps) {
            ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep
                = VN_AS(typedefp->dtypep(), ਗੰਢ_UOr_Struct_DType);
            ਜੇ (pubStrDtypeps.ਗਿਣਤੀ_2(sdtypep)) typedefp->attr_ਜਨਤਕ(ਸੱਚ);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        ਲਈ (Ast_ਗੰਢ_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            ਜੇ (!pubStrDtypeps.ਗਿਣਤੀ_2(sdtypep)) sdtypep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
        }
    }
};

//######################################################################
// Class class functions

ਖਾਲੀ V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("class", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
