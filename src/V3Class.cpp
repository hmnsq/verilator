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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Class.h"

#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <queue>
#అన్తర్భావ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి ClassVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstClass::user1()       -> bool.  True if iterated already
    //  AstVar::user1p()        -> AstVarScope*  Scope used with this var
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // MEMBERS
    string m_ఉపసర్గ;  // String prefix to add to class name based on hier
    V3_Unique_పేర్లు m_పేర్లు;  // For unique naming of structs and unions
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_కణుపు_Module* m_classPackagep = శూన్యసూచిక;  // Package moving into
    స్థిరరాశి AstScope* m_classScopep = శూన్యసూచిక;  // Package moving scopes into
    AstScope* m_packageScopep = శూన్యసూచిక;  // Class package scope
    స్థిరరాశి Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current task
    std::సదిశ<std::pair<Ast_కణుపు*, AstScope*>> m_గమ్యం_Scope_కదలికలు;
    std::సదిశ<std::pair<Ast_కణుపు*, Ast_కణుపు_Module*>> m_గమ్యం_Package_కదలికలు;
    std::సమితి<AstTypedef*> m_typedefps;  // Contains all typedef nodes
    std::సమితి<Ast_కణుపు_UOr_Struct_DType*> m_strDtypeps;  // Contains all packed structs and unions
    // Contains all public packed structs and unions, using a queue to
    // mark embedded struct / union public by BFS
    std::వరుస<Ast_కణుపు_UOr_Struct_DType*> m_pubStrDtypeps;

    // METHODS

    శూన్య recurseImplements(AstClass* nodep) {
        // In SystemVerilog, we have two inheritance chains:
        // - extends of concrete clasess: mapped to non-virtual C++ inheritance
        //   as there is only single ancestor allowed
        // - implements of concrete classes / extends of interface classes: mapped
        //   to virtual inheritance to allow diamond patterns with multiple ancestors
        ఒకవేళ (nodep->use_Virtual_ప్రజా()) తిరిగి;  // Short-circuit to exit diamond cycles
        ఒకవేళ (nodep->isInterfaceClass()) { nodep->use_Virtual_ప్రజా(నిజం); }
        కొరకు (స్థిరరాశి AstClassExtends* extp = nodep->extendsp(); extp;
             extp = VN_AS(extp->nextp(), ClassExtends)) {
            recurseImplements(extp->classp());
        }
    }

    // VISITORS

    శూన్య సందర్శించు(AstClass* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        // Move this class
        nodep->name(m_ఉపసర్గ + nodep->name());
        nodep->unlink_Fr_వెనుక();
        v3Global.rootp()->కూడు_Modulesp(nodep);
        recurseImplements(nodep);
        // Make containing package
        // Note origName is the same as the class origName so errors look correct
        AstClassPackage* స్థిరరాశి packagep
            = కొత్త AstClassPackage{nodep->fileline(), nodep->origName(), nodep->libname()};
        packagep->name(nodep->name() + "__Vclpkg");
        nodep->edit_లెక్క_Inc();
        nodep->classOrPackagep(packagep);
        packagep->classp(nodep);
        packagep->timeunit(nodep->timeunit());
        v3Global.rootp()->కూడు_Modulesp(packagep);
        // Add package to hierarchy
        AstCell* స్థిరరాశి cellp = కొత్త AstCell{packagep->fileline(),
                                           packagep->fileline(),
                                           packagep->name(),
                                           packagep->name(),
                                           శూన్యసూచిక,
                                           శూన్యసూచిక,
                                           శూన్యసూచిక};
        cellp->modp(packagep);
        v3Global.rootp()->శిఖరం_Modulep()->కూడు_Stmtsp(cellp);
        // Find class's scope
        // Alternative would be to move this and related to V3Scope
        స్థిరరాశి AstScope* classScopep = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* itp = nodep->stmtsp(); itp; itp = itp->nextp()) {
            ఒకవేళ ((classScopep = VN_CAST(itp, Scope))) ఆపు;
        }
        UASSERT_OBJ(classScopep, nodep, "No scope under class");

        // Add scope
        AstScope* స్థిరరాశి scopep
            = కొత్త AstScope{nodep->fileline(), packagep, classScopep->name(),
                           classScopep->aboveScopep(), classScopep->aboveCellp()};
        packagep->కూడు_Stmtsp(scopep);
        // Iterate
        VL_RESTORER_CLEAR(m_ఉపసర్గ);
        VL_RESTORER(m_classPackagep);
        VL_RESTORER(m_classScopep);
        VL_RESTORER(m_packageScopep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_classPackagep = packagep;
        m_classScopep = classScopep;
        m_packageScopep = scopep;
        m_ఉపసర్గ = nodep->name() + "__02e";  // .
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        // Visit for NodeModules that are not AstClass (AstClass is-a AstNodeModule)
        // Classes are always under a Package (perhaps $unit) or a module
        VL_RESTORER_CLEAR(m_ఉపసర్గ);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_ఉపసర్గ = nodep->name() + "__03a__03a";  // ::
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstVar* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_packageScopep) {
            ఒకవేళ (m_ftaskp && m_ftaskp->isStatic()) {
                // Move later, or we wouldn't keep iterating the class
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_గమ్యం_Scope_కదలికలు.emplace_వెనుక(nodep, m_packageScopep);
            }
            ఒకవేళ (!m_ftaskp && (nodep->lifetime().isStatic() || nodep->isParam())) {
                m_గమ్యం_Package_కదలికలు.emplace_వెనుక(nodep, m_classPackagep);
                // We're really moving the VarScope but we might not
                // have a pointer to it yet
                m_గమ్యం_Scope_కదలికలు.emplace_వెనుక(nodep, m_packageScopep);
            }
        }
    }

    శూన్య సందర్శించు(AstVarScope* nodep) override {
        iterateChildren(nodep);
        nodep->varp()->user1p(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        ఒకవేళ (m_packageScopep && nodep->isStatic()) {
            m_గమ్యం_Scope_కదలికలు.emplace_వెనుక(nodep, m_packageScopep);
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        iterateChildren(nodep);
        // Don't move now, or wouldn't keep iterating the class
        // TODO move function statics only
        // if (m_classScopep) {
        //    m_toScopeMoves.emplace_back(nodep, m_classScopep);
        //}
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Decl* nodep) override {
        // Need to declare coverage in package, where we have access to symbol table
        iterateChildren(nodep);
        ఒకవేళ (m_classPackagep) m_classPackagep->కూడు_Stmtsp(nodep->unlink_Fr_వెనుక());
    }
    శూన్య సందర్శించు(AstInitial* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ఒకవేళ (m_packageScopep) m_గమ్యం_Scope_కదలికలు.emplace_వెనుక(nodep, m_packageScopep);
    }
    శూన్య సందర్శించు(AstInitialStatic* nodep) override {
        // But not AstInitialAutomatic, which remains under the class
        iterateChildren(nodep);
        ఒకవేళ (m_packageScopep) m_గమ్యం_Scope_కదలికలు.emplace_వెనుక(nodep, m_packageScopep);
    }

    శూన్య సమితి_Struct_Modulep(Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి dtypep) {
        // Give struct a pointer to its package and a final name
        dtypep->edit_లెక్క_Inc();
        dtypep->classOrPackagep(m_classPackagep ? m_classPackagep : m_modp);
        dtypep->name(
            m_పేర్లు.get(dtypep->name() + (VN_IS(dtypep, UnionDType) ? "__union" : "__struct")));
        ఒకవేళ (dtypep->packed()) m_strDtypeps.చొప్పించు(dtypep);

        కొరకు (AstMemberDType* itemp = dtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి subp = itemp->get_బిడ్డ_Structp();
            // Recurse only into anonymous structs inside this definition,
            // other structs will be reached from another typedefs
            ఒకవేళ (subp && subp->name().ఖాళీలు()) సమితి_Struct_Modulep(subp);
        }
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి dtypep = VN_CAST(nodep->dtypep(), కణుపు_UOr_Struct_DType);
        ఒకవేళ (dtypep && dtypep->packed()) {
            m_typedefps.చొప్పించు(nodep);
            ఒకవేళ (nodep->attr_ప్రజా()) m_pubStrDtypeps.నెట్టు(dtypep);
        }
        iterateChildren(nodep);
        ఒకవేళ (m_classPackagep) m_classPackagep->కూడు_Stmtsp(nodep->unlink_Fr_వెనుక());

        ఒకవేళ (dtypep) {
            dtypep->name(nodep->name());
            సమితి_Struct_Modulep(dtypep);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {}  // Short circuit
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {}  // Short circuit
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ClassVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ClassVisitor() override {
        కొరకు (స్వయంచాలక moved : m_గమ్యం_Scope_కదలికలు) {
            Ast_కణుపు* స్థిరరాశి nodep = moved.మొదటి;
            AstScope* స్థిరరాశి scopep = moved.రెండవ;
            UINFO(9, "moving " << nodep << " to " << scopep);
            ఒకవేళ (VN_IS(nodep, కణుపు_FTask)) {
                scopep->కూడు_Blocksp(nodep->unlink_Fr_వెనుక());
            } లేకపోతే ఒకవేళ (VN_IS(nodep, Var)) {
                AstVarScope* స్థిరరాశి vscp = VN_AS(nodep->user1p(), VarScope);
                vscp->scopep(scopep);
                vscp->unlink_Fr_వెనుక();
                scopep->కూడు_Varsp(vscp);
            } లేకపోతే ఒకవేళ (VN_IS(nodep, Initial) || VN_IS(nodep, InitialStatic)) {
                nodep->unlink_Fr_వెనుక();
                scopep->కూడు_Blocksp(nodep);
            } లేకపోతే {
                nodep->v3fatalSrc("Bad case");
            }
        }
        కొరకు (స్వయంచాలక moved : m_గమ్యం_Package_కదలికలు) {
            Ast_కణుపు* స్థిరరాశి nodep = moved.మొదటి;
            Ast_కణుపు_Module* స్థిరరాశి modp = moved.రెండవ;
            UINFO(9, "moving " << nodep << " to " << modp);
            nodep->unlink_Fr_వెనుక();
            modp->కూడు_Stmtsp(nodep);
        }
        // BFS to mark public typedefs.
        std::సమితి<స్థిరరాశి Ast_కణుపు_UOr_Struct_DType*> pubStrDtypeps;
        అయితే (!m_pubStrDtypeps.ఖాళీలు()) {
            Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి dtypep = m_pubStrDtypeps.front();
            m_pubStrDtypeps.తీయి();
            ఒకవేళ (pubStrDtypeps.చొప్పించు(dtypep).రెండవ) {
                కొరకు (AstMemberDType* itemp = dtypep->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి subp = itemp->get_బిడ్డ_Structp())
                        m_pubStrDtypeps.నెట్టు(subp);
                }
            }
        }
        కొరకు (AstTypedef* typedefp : m_typedefps) {
            స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep
                = VN_AS(typedefp->dtypep(), కణుపు_UOr_Struct_DType);
            ఒకవేళ (pubStrDtypeps.లెక్క(sdtypep)) typedefp->attr_ప్రజా(నిజం);
        }
        // Clear package pointer of non-public packed struct / union type, which will never be
        // exported.
        కొరకు (Ast_కణుపు_UOr_Struct_DType* sdtypep : m_strDtypeps) {
            ఒకవేళ (!pubStrDtypeps.లెక్క(sdtypep)) sdtypep->classOrPackagep(శూన్యసూచిక);
        }
    }
};

//######################################################################
// Class class functions

శూన్య V3Class::classAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("class", 0, dump_వృక్షం_Either_Level() >= 3);
}
