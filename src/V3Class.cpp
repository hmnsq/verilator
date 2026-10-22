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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Class.h"

#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <queue>
#அந்தர்பாவ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு ClassVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    மாறிலி VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_முனை_Module* m_classPackagep = nullptr;  // Package moving into
    மாறிலி AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    மாறிலி Ast_முனை_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_முனை*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_முனை*, Ast_முனை_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_முனை_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_முனை_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    வெறுமை recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        எனில் (nodep->useVirtualPublic()) திரும்பு;  // Short-circuit to exit diamond cycles
        எனில் (nodep->isInterfaceClass()) { nodep->useVirtualPublic(உண்மை); }
        க்கு (மாறிலி AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    வெறுமை visit(AstClass* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* மாறிலி packagep
            = புதிய AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_மொத்தம்_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* மாறிலி cellp = புதிய AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->உச்சி_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        மாறிலி AstScope* classScopep = nullptr;
        க்கு (Ast_முனை* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            எனில் ((classScopep = VN_CAST(itp, Scope))) நிறுத்து;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* மாறிலி scopep
            = புதிய AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    வெறுமை visit(AstVar* nodep) override {
        iterateChildren(nodep);
        எனில் (m_packageScopep) {
            எனில் (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            எனில் (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    வெறுமை visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        எனில் (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    வெறுமை visit(Ast_முனை_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        எனில் (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    வெறுமை visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        எனில் (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    வெறுமை visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        எனில் (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    வெறுமை setStructModulep(Ast_முனை_UOr_Struct_DType* மாறிலி dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_மொத்தம்_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        எனில் (dtypep->packed()) m_strDtypeps.செருகு(dtypep);

        க்கு (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_முனை_UOr_Struct_DType* மாறிலி subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            எனில் (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    வெறுமை visit(AstTypedef* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        Ast_முனை_UOr_Struct_DType* மாறிலி dtypep = VN_CAST(nodep->dtypep(), முனை_UOr_Struct_DType);
        எனில் (dtypep && dtypep->packed()) {
            m_typedefps.செருகு(nodep);
            எனில் (nodep->attrPublic()) m_pubStrDtypeps.தள்ளு(dtypep);
        }
        iterateChildren(nodep);
        எனில் (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        எனில் (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    வெறுமை visit(Ast_முனை_கோவை* nodep) override {}  // Short circuit
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {}  // Short circuit
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        க்கு (auto moved : m_toScopeMoves) {
            Ast_முனை* மாறிலி nodep = moved.first;
            AstScope* மாறிலி scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            எனில் (VN_IS(nodep, முனை_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } இல்லையெனில் எனில் (VN_IS(nodep, Var)) {
                AstVarScope* மாறிலி vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } இல்லையெனில் எனில் (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } இல்லையெனில் {
                nodep->v3fatalSrc("Bad case");
            }
        }
        க்கு (auto moved : m_toPackageMoves) {
            Ast_முனை* மாறிலி nodep = moved.first;
            Ast_முனை_Module* மாறிலி modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<மாறிலி Ast_முனை_UOr_Struct_DType*> pubStrDtypeps;
        போது (!m_pubStrDtypeps.empty()) {
            Ast_முனை_UOr_Struct_DType* மாறிலி dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.எடு();
            எனில் (pubStrDtypeps.செருகு(dtypep).second) {
                க்கு (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    எனில் (Ast_முனை_UOr_Struct_DType* மாறிலி subp = itemp->getChildStructp())
                        m_pubStrDtypeps.தள்ளு(subp);
                }
            }
        }
        க்கு (AstTypedef* typedefp : m_typedefps) {
            மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி sdtypep
                = VN_AS(typedefp->dtypep(), முனை_UOr_Struct_DType);
            எனில் (pubStrDtypeps.மொத்தம்(sdtypep)) typedefp->attrPublic(உண்மை);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        க்கு (Ast_முனை_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            எனில் (!pubStrDtypeps.மொத்தம்(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

வெறுமை V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("class", 0, dump_மரம்_Either_Level() >= 3);
}
