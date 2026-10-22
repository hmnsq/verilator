// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Interface references for tracing and VPI
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
// V3Interface's Transformations:
//
// Each module:
//      Look for CELL...
//          Keep track of scope and concrete interface along the way
//          Find all interface references
//              Add INTFREF to concrete interface's list of references
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

తరగతి InlineIntfRefVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override { iterateChildrenConst(nodep->శిఖరం_Modulep()); }
    శూన్య సందర్శించు(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        ఒకవేళ (m_scope.ఖాళీలు()) {
            m_scope = nodep->name();
        } లేకపోతే {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_కణుపు_Module* స్థిరరాశి modp = nodep->modp();
        // Pass Cell pointers down to the next module
        కొరకు (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* స్థిరరాశి varp = pinp->modVarp();
            స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(pinp->exprp(), VarRef);
            ఒకవేళ (!varrefp) కొనసాగు;

            స్థిరరాశి AstVar* స్థిరరాశి fromVarp = varrefp->varp();
            స్థిరరాశి AstIfaceRefDType* స్థిరరాశి irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            ఒకవేళ (!irdtp) కొనసాగు;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            ఒకవేళ (!cellp) cellp = irdtp->cellp();
            ఒకవేళ (!cellp) కొనసాగు;
            varp->user1p(cellp);
            స్థిరరాశి string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            స్థిరరాశి AstIfaceRefDType* స్థిరరాశి పోర్టు_Irdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            స్థిరరాశి string modportName = పోర్టు_Irdtp ? పోర్టు_Irdtp->modportName() : irdtp->modportName();
            File_పంక్తి* స్థిరరాశి flp = pinp->fileline();
            cellp->కూడు_Intf_Refsp(కొత్త AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు_Stmt*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన InlineIntfRefVisitor(Ast_కణుపు* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = అప్రమేయ;
};

//######################################################################
// Interface class functions

శూన్య V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_వృక్షం("interface", 0, dump_వృక్షం_Either_Level() >= 3);
}
