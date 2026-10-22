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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Class.h"

#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <queue>
#অন্তর্ভাব <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

বর্গ ClassVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    ধ্রুব VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্রন্থি_Module* m_classPackagep = nullptr;  // Package moving into
    ধ্রুব AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    ধ্রুব Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_গ্রন্থি*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_গ্রন্থি*, Ast_গ্রন্থি_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_গ্রন্থি_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_গ্রন্থি_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    শূন্য recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        যদি (nodep->useVirtualPublic()) প্রত্যাগম;  // Short-circuit to exit diamond cycles
        যদি (nodep->isInterfaceClass()) { nodep->useVirtualPublic(সত্য); }
        প্রতি (ধ্রুব AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    শূন্য visit(AstClass* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* ধ্রুব packagep
            = নূতন AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_গুনতি_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* ধ্রুব cellp = নূতন AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->শীর্ষ_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        ধ্রুব AstScope* classScopep = nullptr;
        প্রতি (Ast_গ্রন্থি* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            যদি ((classScopep = VN_CAST(itp, Scope))) ভঙ্গ;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* ধ্রুব scopep
            = নূতন AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        যদি (m_packageScopep) {
            যদি (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            যদি (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    শূন্য visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        যদি (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    শূন্য visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    শূন্য visit(Ast_গ্রন্থি_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        যদি (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    শূন্য visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        যদি (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    শূন্য visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        যদি (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    শূন্য setStructModulep(Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_গুনতি_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        যদি (dtypep->packed()) m_strDtypeps.ঢোকাও(dtypep);

        প্রতি (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            যদি (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    শূন্য visit(AstTypedef* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব dtypep = VN_CAST(nodep->dtypep(), গ্রন্থি_UOr_Struct_DType);
        যদি (dtypep && dtypep->packed()) {
            m_typedefps.ঢোকাও(nodep);
            যদি (nodep->attrPublic()) m_pubStrDtypeps.ঠেলো(dtypep);
        }
        iterateChildren(nodep);
        যদি (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        যদি (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {}  // Short circuit
    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {}  // Short circuit
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        প্রতি (auto moved : m_toScopeMoves) {
            Ast_গ্রন্থি* ধ্রুব nodep = moved.first;
            AstScope* ধ্রুব scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            যদি (VN_IS(nodep, গ্রন্থি_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } অন্যথা যদি (VN_IS(nodep, Var)) {
                AstVarScope* ধ্রুব vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } অন্যথা যদি (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } অন্যথা {
                nodep->v3fatalSrc("Bad case");
            }
        }
        প্রতি (auto moved : m_toPackageMoves) {
            Ast_গ্রন্থি* ধ্রুব nodep = moved.first;
            Ast_গ্রন্থি_Module* ধ্রুব modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<ধ্রুব Ast_গ্রন্থি_UOr_Struct_DType*> pubStrDtypeps;
        যাবত্ (!m_pubStrDtypeps.empty()) {
            Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.বের_করো();
            যদি (pubStrDtypeps.ঢোকাও(dtypep).second) {
                প্রতি (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    যদি (Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব subp = itemp->getChildStructp())
                        m_pubStrDtypeps.ঠেলো(subp);
                }
            }
        }
        প্রতি (AstTypedef* typedefp : m_typedefps) {
            ধ্রুব Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব sdtypep
                = VN_AS(typedefp->dtypep(), গ্রন্থি_UOr_Struct_DType);
            যদি (pubStrDtypeps.গুনতি(sdtypep)) typedefp->attrPublic(সত্য);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        প্রতি (Ast_গ্রন্থি_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            যদি (!pubStrDtypeps.গুনতি(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

শূন্য V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("class", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
