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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Class.h"

#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <queue>
#അന്തര്ഭാവ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം ClassVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    സ്ഥിരം VNUser1InUse m_inuser1;

    // MEMBERS
    string m_prefix;  // String prefix to add to class name based on hier
    V3UniqueNames m_names;  // For unique naming of structs and unions
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_കണ്ണി_Module* m_classPackagep = nullptr;  // Package moving into
    സ്ഥിരം AstScope* m_classScopep = nullptr;  // Package moving scopes into
    AstScope* m_packageScopep = nullptr;  // Class package scope
    സ്ഥിരം Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current task
    std::vector<std::pair<Ast_കണ്ണി*, AstScope*>> m_toScopeMoves;
    std::vector<std::pair<Ast_കണ്ണി*, Ast_കണ്ണി_Module*>> m_toPackageMoves;
    std::set<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::set<Ast_കണ്ണി_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::queue<Ast_കണ്ണി_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    ശൂന്യം recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        എങ്കിൽ (nodep->useVirtualPublic()) തിരികെ;  // Short-circuit to exit diamond cycles
        എങ്കിൽ (nodep->isInterfaceClass()) { nodep->useVirtualPublic(സത്യം); }
        വേണ്ടി (സ്ഥിരം AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    ശൂന്യം visit(AstClass* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        // Move this class
        nodep->name(m_prefix + nodep->name());
        nodep->unlinkFrBack();
        v3Global.rootp()->addModulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* സ്ഥിരം packagep
            = പുതിയ AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_എണ്ണം_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->addModulesp(packagep);
        // Add package to hierarchy
        AstCell* സ്ഥിരം cellp = പുതിയ AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(packagep);
        v3Global.rootp()->മുകൾ_Modulep()->addStmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        സ്ഥിരം AstScope* classScopep = nullptr;
        വേണ്ടി (Ast_കണ്ണി* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            എങ്കിൽ ((classScopep = VN_CAST(itp, Scope))) നിർത്തുക;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* സ്ഥിരം scopep
            = പുതിയ AstScope{nodep->fileline(), packagep, classScopep->name(),
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
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_prefix);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_prefix = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstVar* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_packageScopep) {
            എങ്കിൽ (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
            എങ്കിൽ (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_toPackageMoves.emplace_back(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_toScopeMoves.emplace_back(nodep, m_packageScopep);
            }
        }
    }

    ശൂന്യം visit(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (m_packageScopep && nodep->isStatic()) {
            m_toScopeMoves.emplace_back(nodep, m_packageScopep);
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        എങ്കിൽ (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());
    }
    ശൂന്യം visit(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        എങ്കിൽ (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }
    ശൂന്യം visit(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        എങ്കിൽ (m_packageScopep) m_toScopeMoves.emplace_back(nodep, m_packageScopep);
    }

    ശൂന്യം setStructModulep(Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_എണ്ണം_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_names.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        എങ്കിൽ (dtypep->packed()) m_strDtypeps.തിരുകുക(dtypep);

        വേണ്ടി (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം subp = itemp->getChildStructp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            എങ്കിൽ (subp && subp->name().empty()) setStructModulep(subp);
        }
    }
    ശൂന്യം visit(AstTypedef* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം dtypep = VN_CAST(nodep->dtypep(), കണ്ണി_UOr_Struct_DType);
        എങ്കിൽ (dtypep && dtypep->packed()) {
            m_typedefps.തിരുകുക(nodep);
            എങ്കിൽ (nodep->attrPublic()) m_pubStrDtypeps.തള്ളുക(dtypep);
        }
        iterateChildren(nodep);
        എങ്കിൽ (m_classPackagep) m_classPackagep->addStmtsp(nodep->unlinkFrBack());

        എങ്കിൽ (dtypep) {
            dtypep->name(nodep->name());
            setStructModulep(dtypep);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {}  // Short circuit
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {}  // Short circuit
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        വേണ്ടി (auto moved : m_toScopeMoves) {
            Ast_കണ്ണി* സ്ഥിരം nodep = moved.first;
            AstScope* സ്ഥിരം scopep = moved.second;
            UINFO(9, "moving " << nodep << " to " << scopep);
            എങ്കിൽ (VN_IS(nodep, കണ്ണി_FTask)) {
                scopep->addBlocksp(nodep->unlinkFrBack());
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Var)) {
                AstVarScope* സ്ഥിരം vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlinkFrBack();
                scopep->addVarsp(vscp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlinkFrBack();
                scopep->addBlocksp(nodep);
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Bad case");
            }
        }
        വേണ്ടി (auto moved : m_toPackageMoves) {
            Ast_കണ്ണി* സ്ഥിരം nodep = moved.first;
            Ast_കണ്ണി_Module* സ്ഥിരം modp = moved.second;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlinkFrBack();
            modp->addStmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::set<സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType*> pubStrDtypeps;
        ആകുമ്പോൾ (!m_pubStrDtypeps.empty()) {
            Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.എടുക്കുക();
            എങ്കിൽ (pubStrDtypeps.തിരുകുക(dtypep).second) {
                വേണ്ടി (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം subp = itemp->getChildStructp())
                        m_pubStrDtypeps.തള്ളുക(subp);
                }
            }
        }
        വേണ്ടി (AstTypedef* typedefp : m_typedefps) {
            സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep
                = VN_AS(typedefp->dtypep(), കണ്ണി_UOr_Struct_DType);
            എങ്കിൽ (pubStrDtypeps.എണ്ണം(sdtypep)) typedefp->attrPublic(സത്യം);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        വേണ്ടി (Ast_കണ്ണി_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            എങ്കിൽ (!pubStrDtypeps.എണ്ണം(sdtypep)) sdtypep->classOrPackagep(nullptr);
        }
    }
};

//######################################################################
// Class class functions

ശൂന്യം V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("class", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
