// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Removal of named begin blocks
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
// V3Begin's Transformations:
//
// Each module:
//      Look for BEGINs
//          BEGIN(VAR...) -> VAR ... {renamed}
//      FOR -> WHILEs
//      Move static function variables and their AstInitialStatic blocks before a function
//
// There are two scopes; named BEGINs change %m and variable scopes.
// Unnamed BEGINs change only variable, not $display("%m") scope.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Begin.h"

#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    తార్కిక m_any_Func_లోపల_Begin = అబద్ధం;

బహిరంగ:
    BeginState() = అప్రమేయ;
    ~BeginState() = అప్రమేయ;
    శూన్య user_Mark_మారింది(Ast_కణుపు* nodep) {
        nodep->user1(నిజం);
        m_any_Func_లోపల_Begin = నిజం;
    }
    తార్కిక any_Func_లోపల_Begin() స్థిరరాశి { తిరిగి m_any_Func_లోపల_Begin; }
};

//######################################################################

తరగతి BeginVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* స్థిరరాశి m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstCellInline* m_inlinesp = శూన్యసూచిక;  // AstCellInlines to add to m_modp
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_కణుపు_Procedure* m_procedurep = శూన్యసూచిక;  // Current procedure
    Ast_కణుపు* m_liftedp = శూన్యసూచిక;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    పూర్ణాంకం m_if_లోతు = 0;  // Current if depth
    తార్కిక m_keepBegins = అబద్ధం;  // True if begins should not be inlined

    // METHODS

    string dot(స్థిరరాశి string& a, స్థిరరాశి string& b) { తిరిగి VString::dot(a, "__DOT__", b); }

    శూన్య dot_పేర్లు(స్థిరరాశి std::string& name, File_పంక్తి* స్థిరరాశి flp, స్థిరరాశి అక్షరం* స్థిరరాశి ఖండం_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        ఒకవేళ (name == "") తిరిగి;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::కొలత_type pos;
        అయితే ((pos = dottedname.కనుగొను("__DOT__")) != std::string::npos) {
            స్థిరరాశి std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::అక్షరశ్రేణి_పొడవు("__DOT__"));
            ఒకవేళ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            ఒకవేళ (!m_ftaskp) {
                AstCellInline* స్థిరరాశి inlinep = కొత్త AstCellInline{flp, m_unnamedScope, ఖండం_Name};
                m_inlinesp = Ast_కణుపు::కూడు_తదుపరి_Null(m_inlinesp, inlinep);
            }
        }
    }

    శూన్య lift_కణుపు(Ast_కణుపు* nodep) {
        nodep->unlink_Fr_వెనుక();
        ఒకవేళ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            ఒకవేళ (!m_liftedp) {
                m_liftedp = nodep;
            } లేకపోతే {
                m_liftedp->కూడు_తదుపరి(nodep);
            }
        } లేకపోతే {
            // Move to module
            m_modp->కూడు_Stmtsp(nodep);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_గమ్యం_While(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        ఒకవేళ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_మారింది(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = శూన్యసూచిక;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        ఒకవేళ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_మారింది(nodep);
        }
        // BEGIN wrapping a function rename that function, but don't affect
        // the inside function's variables.  We then restart with empty
        // naming; so that any begin's inside the function will rename
        // inside the function.
        // Process children
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_liftedp);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        m_displayScope = dot(m_displayScope, nodep->name());

        m_ftaskp = nodep;
        m_liftedp = శూన్యసూచిక;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* స్థిరరాశి initp) {
            initp->unlink_Fr_వెనుక();
            m_ftaskp->కూడు_ఇక్కడ_This_As_తదుపరి(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_పేర్లు(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_తదుపరి_Null(nodep->itemsp());
        // Repalce node with body then delete
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి itemsp = nodep->itemsp()) {
            nodep->కూడు_తదుపరి_ఇక్కడ(itemsp->unlink_Fr_వెనుక_With_తదుపరి());
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        dot_పేర్లు(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = నిజం;
            iterate_And_తదుపరి_Null(nodep->forksp());
        }
        Ast_కణుపు* addsp = శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి declsp = nodep->declsp()) {
            declsp->unlink_Fr_వెనుక_With_తదుపరి();
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp, declsp);
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp, stmtsp);
        }
        ఒకవేళ (addsp) nodep->కూడు_ఇక్కడ_This_As_తదుపరి(addsp);
        nodep->name("");
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = అబద్ధం;
            dot_పేర్లు(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ఒకవేళ (m_keepBegins) {
            nodep->name("");
            తిరిగి;
        }
        Ast_కణుపు* addsp = శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి declsp = nodep->declsp()) {
            declsp->unlink_Fr_వెనుక_With_తదుపరి();
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp, declsp);
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp, stmtsp);
        }
        ఒకవేళ (addsp) {
            nodep->replaceWith(addsp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_ఖండం* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_పేర్లు(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ఒకవేళ (m_keepBegins) {
            nodep->name("");
            తిరిగి;
        }
        Ast_కణుపు* addsp = శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి declsp = nodep->declsp()) {
            declsp->unlink_Fr_వెనుక_With_తదుపరి();
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp, declsp);
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
            addsp = Ast_కణుపు::కూడు_తదుపరి(addsp, stmtsp);
        }
        ఒకవేళ (addsp) {
            nodep->replaceWith(addsp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) { తిరిగి; }
        // If static variable, move it outside a function.
        ఒకవేళ (nodep->lifetime().isStatic() && m_ftaskp) {
            స్థిరరాశి std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            ఒకవేళ (nodep->isIO()) {
                ఒకవేళ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* స్థిరరాశి portp = nodep->clone_వృక్షం_శుద్ధ(అబద్ధం);
                nodep->replaceWith(portp);

                ఒకవేళ (nodep->is_ఆదానం() || nodep->isInout()) {
                    Ast_కేటాయించు* స్థిరరాశి initAssignp = కొత్త Ast_కేటాయించు{
                        nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        కొత్త AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->కూడు_తదుపరి_ఇక్కడ(initAssignp);
                }

                ఒకవేళ (nodep->isWritable()) {
                    Ast_కేటాయించు* స్థిరరాశి చివర_Assignp = కొత్త Ast_కేటాయించు{
                        nodep->fileline(), కొత్త AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        కొత్త AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->కూడు_Stmtsp(చివర_Assignp);
                }
            } లేకపోతే {
                nodep->unlink_Fr_వెనుక();
            }
            nodep->name(newName);
            m_ftaskp->కూడు_ఇక్కడ_This_As_తదుపరి(nodep);
            nodep->funcLocal(అబద్ధం);
        } లేకపోతే ఒకవేళ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_మారింది(nodep);
            // Move it under enclosing tree
            lift_కణుపు(nodep);
        }
    }
    శూన్య సందర్శించు(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Don't double-add text's
        Ast_కణుపు* wasUnderp = m_ftaskp;
        ఒకవేళ (!m_ftaskp) wasUnderp = m_procedurep;
        ఒకవేళ (wasUnderp) {
            ఒకవేళ (nodep->stmtsp()) {
                Ast_కణుపు* స్థిరరాశి newp = కొత్త AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి()};
                wasUnderp->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        } లేకపోతే {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        ఒకవేళ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_మారింది(nodep);
            // Move it under enclosing tree
            lift_కణుపు(nodep);
        }
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        ఒకవేళ (m_namedScope != "") {
            m_statep->user_Mark_మారింది(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_వెనుక();
            m_modp->కూడు_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        ఒకవేళ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        ఒకవేళ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        స్థిరరాశి std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        ఒకవేళ (!scname.ఖాళీలు()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    శూన్య సందర్శించు(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = అబద్ధం;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_లోతు);
        ఒకవేళ (m_if_లోతు == -1 || v3Global.opt.if_లోతు() < 1) {  // Turned off
        } లేకపోతే ఒకవేళ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->ప్రాధాన్యత_Pragma()) {
            m_if_లోతు = -1;
        } లేకపోతే ఒకవేళ (++m_if_లోతు > v3Global.opt.if_లోతు()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::IFDEPTH, నిజం);  // Warn only once
            m_if_లోతు = -1;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = అబద్ధం;
        iterateChildren(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = అప్రమేయ;
};

//######################################################################

తరగతి BeginRelinkVisitor final : బహిరంగ VNVisitorConst {
    // Replace tasks with new pointer
అంతరంగ:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        ఒకవేళ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        ఒకవేళ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = అప్రమేయ;
};

//######################################################################
// Task class functions

శూన్య V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        ఒకవేళ (state.any_Func_లోపల_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("begin", 0, dump_వృక్షం_Either_Level() >= 3);
}

స్థిర Ast_కణుపు* createForeachLoop(Ast_కణుపు_Foreach* /*nodep*/, Ast_కణుపు* bodysp, తార్కిక arrayMayResize,
                                  Ast_కణుపు_సమాసం* subfromp, AstVar* varp, Ast_కణుపు_సమాసం* leftp,
                                  Ast_కణుపు_సమాసం* rightp, VNType కణుపు_Type) {
    File_పంక్తి* స్థిరరాశి fl = varp->fileline();
    Ast_కణుపు_సమాసం* varRefp = కొత్త AstVarRef{fl, varp, VAccess::READ};
    Ast_కణుపు_సమాసం* condp;
    తార్కిక inc = నిజం;
    ఎంపిక (కణుపు_Type) {
    స్థితి VNType::LteS: condp = కొత్త AstLteS{fl, varRefp, rightp}; ఆపు;
    స్థితి VNType::Lt: condp = కొత్త AstLt{fl, varRefp, rightp}; ఆపు;
    స్థితి VNType::GteS:
        condp = కొత్త AstGteS{fl, varRefp, rightp};
        inc = అబద్ధం;
        ఆపు;
    అప్రమేయ: varp->v3fatalSrc("Missing comparison handling"); ఆపు;
    }
    Ast_కణుపు_సమాసం* incp;
    ఒకవేళ (inc)
        incp = కొత్త Ast_కూడు{fl, varRefp->clone_వృక్షం(అబద్ధం), కొత్త AstConst{fl, 1}};
    లేకపోతే
        incp = కొత్త AstSub{fl, varRefp->clone_వృక్షం(అబద్ధం), కొత్త AstConst{fl, 1}};

    AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{fl};
    loopp->కూడు_Stmtsp(కొత్త AstLoopTest{fl, loopp, condp});
    AstVar* కొలత_Varp = శూన్యసూచిక;
    Ast_కణుపు_సమాసం* కొలత_Getp = శూన్యసూచిక;
    ఒకవేళ (arrayMayResize) {
        కొలత_Varp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->కనుగొను_UInt32_DType()};
        కొలత_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        కొలత_Varp->వాడిన_Loop_Idx(నిజం);  // Not technically an index, but used only inside loop
        varp->కూడు_తదుపరి(కొలత_Varp);
        కొలత_Getp
            = కొత్త AstCMethodHard{fl, subfromp->clone_వృక్షం_శుద్ధ(అబద్ధం), VCMethod::DYN_కొలత, శూన్యసూచిక};
        కొలత_Getp->dtype_సమితి_UInt32();
        loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, కొలత_Varp, VAccess::WRITE}, కొలత_Getp});
    }
    loopp->కూడు_Stmtsp(bodysp);
    Ast_కణుపు_Stmt* incStmtp = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, incp};
    ఒకవేళ (arrayMayResize) {
        incStmtp = కొత్త AstIf{fl,
                             కొత్త AstLte{fl, కొత్త AstVarRef{fl, కొలత_Varp, VAccess::READ},
                                        కొలత_Getp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                             incStmtp};
    }
    loopp->కూడు_Stmtsp(incStmtp);
    Ast_కణుపు* స్థిరరాశి stmtsp = varp;  // New statements for outer loop
    stmtsp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->కూడు_తదుపరి(loopp);
    తిరిగి stmtsp;
}
స్థిర Ast_కణుపు* createForeachLoopRanged(Ast_కణుపు_Foreach* nodep, Ast_కణుపు* bodysp,
                                        Ast_కణుపు_సమాసం* subfromp, AstVar* varp,
                                        స్థిరరాశి VNumRange& declRange) {
    File_పంక్తి* స్థిరరాశి fl = varp->fileline();
    V3_సంఖ్య ఎడమ{nodep, 32};
    V3_సంఖ్య కుడి{nodep, 32};
    ఎడమ.isSigned(నిజం);
    కుడి.isSigned(నిజం);
    ఎడమ.సమితి_Long_S(declRange.ఎడమ());
    కుడి.సమితి_Long_S(declRange.కుడి());
    Ast_కణుపు_సమాసం* స్థిరరాశి leftp = కొత్త AstConst{fl, ఎడమ};
    Ast_కణుపు_సమాసం* స్థిరరాశి rightp = కొత్త AstConst{fl, కుడి};
    తిరిగి createForeachLoop(nodep, bodysp, అబద్ధం /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ఎడమ() <= declRange.కుడి() ? VNType::LteS : VNType::GteS);
}
స్థిర Ast_కణుపు* createForeachAssoc(File_పంక్తి* fl, AstVar* varp, Ast_కణుపు_సమాసం* subfromp,
                                   Ast_కణుపు_DType* fromDtp, Ast_కణుపు* bodyPointp) {
    Ast_కణుపు* loopp = varp;
    AstVar* స్థిరరాశి తదుపరి_varp  // Iterator containing next element (to handle mid-array delete)
        = కొత్త AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    తదుపరి_varp->వాడిన_Loop_Idx(నిజం);
    తదుపరి_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->కూడు_తదుపరి(తదుపరి_varp);

    AstVar* స్థిరరాశి మరిన్ని_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = కొత్త AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_బిట్టు_Packed{}, 1};
    మరిన్ని_varp->వాడిన_Loop_Idx(నిజం);  // Not technically an index, but used only inside loop
    మరిన్ని_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->కూడు_తదుపరి(మరిన్ని_varp);

    Ast_కణుపు_సమాసం* స్థిరరాశి firstp
        = కొత్త AstCMethodHard{fl, subfromp->clone_వృక్షం_శుద్ధ(అబద్ధం), VCMethod::ASSOC_మొదటి,
                             కొత్త AstVarRef{fl, తదుపరి_varp, VAccess::READWRITE}};
    firstp->dtype_సమితి_Int();
    Ast_కణుపు_సమాసం* స్థిరరాశి nextp
        = కొత్త AstCMethodHard{fl, subfromp->clone_వృక్షం_శుద్ధ(అబద్ధం), VCMethod::ASSOC_తదుపరి,
                             కొత్త AstVarRef{fl, తదుపరి_varp, VAccess::READWRITE}};
    nextp->dtype_సమితి_Int();

    // _Vmore = array.first(__Vnext)
    loopp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, మరిన్ని_varp, VAccess::WRITE},
                                 కొత్త AstNeq{fl, కొత్త AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* స్థిరరాశి lp = కొత్త AstLoop{fl};
    loopp->కూడు_తదుపరి(lp);
    lp->కూడు_Stmtsp(కొత్త AstLoopTest{fl, lp, కొత్త AstVarRef{fl, మరిన్ని_varp, VAccess::READ}});

    // index = __Vnext
    lp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                కొత్త AstVarRef{fl, తదుపరి_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, మరిన్ని_varp, VAccess::WRITE},
                                కొత్త AstNeq{fl, కొత్త AstConst{fl, 0}, nextp}});
    lp->కూడు_Stmtsp(bodyPointp);
    తిరిగి loopp;
}

స్థిర తార్కిక arrayMayResizeCheck(Ast_కణుపు* nodesp, AstVar* fromVarp) {
    // IEEE 1800-2023 12.7.3 "If the dimensions of a dynamically sized
    // array are changed while iterating over a foreach-loop construct, th
    // results are undefined".  However UVM calls delete within a foreach
    // in the uvm_reg code and in the tests, and expects the foreach to see
    // all elements post-delete.
    //
    // Cannot just check body for delete() because the body may call a
    // function that does a delete.  Cannot easily have the verilated_types
    // code set a flag on delete, because might be multiple foreach in
    // flight. Could use generation counter and each loop checks, but adds
    // time to every array modification. So, we assume if the array
    // shrinks, we should repeat the loop.
    ఒకవేళ (!nodesp) తిరిగి అబద్ధం;
    తిరిగి nodesp->exists_And_తదుపరి([&](స్థిరరాశి Ast_కణుపు* nodep) -> తార్కిక {
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* varrefp = VN_CAST(nodep, కణుపు_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            ఒకవేళ (varrefp->access().isWriteOrRW()) {
                ఒకవేళ (!fromVarp) తిరిగి నిజం;
                ఒకవేళ (varrefp->varp() == fromVarp) తిరిగి నిజం;
            }
        }
        // No idea what a function might be doing
        ఒకవేళ (VN_IS(nodep, పద్ధతి_Call) || VN_IS(nodep, కణుపు_FTask_Ref)) తిరిగి నిజం;
        తిరిగి అబద్ధం;
    });
}

Ast_కణుపు* V3Begin::convert_గమ్యం_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    స్థిరరాశి Ast_Foreach_శీర్షిక* స్థిరరాశి headerp = nodep->headerp();
    Ast_కణుపు_సమాసం* స్థిరరాశి fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* స్థిరరాశి fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : శూన్యసూచిక;
    Ast_కణుపు_DType* fromDtp = fromp->dtypep()->skipRefp();
    స్థిరరాశి తార్కిక arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_కణుపు* bodyPointp = కొత్త AstBegin{nodep->fileline(), "[EditWrapper]", శూన్యసూచిక, అబద్ధం};
    Ast_కణుపు* newp = శూన్యసూచిక;
    Ast_కణుపు* lastp = nodep;
    AstVar* nestedIndexp = శూన్యసూచిక;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_కణుపు_సమాసం* subfromp = fromp->clone_వృక్షం_శుద్ధ(అబద్ధం);
    // Major dimension first
    కొరకు (Ast_కణుపు *argsp = headerp->elementsp(), *తదుపరి_argsp; argsp; argsp = తదుపరి_argsp) {
        తదుపరి_argsp = argsp->nextp();
        స్థిరరాశి తార్కిక ఖాళీలు = VN_IS(argsp, ఖాళీలు_2);
        AstVar* స్థిరరాశి varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || ఖాళీలు, argsp, "Missing foreach loop variable");
        ఒకవేళ (varp) varp->unlink_Fr_వెనుక()->వాడిన_Loop_Idx(నిజం);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_పంక్తి* స్థిరరాశి fl = argsp->fileline();
        ఒకవేళ (varp) {
            Ast_కణుపు* loopp = శూన్యసూచిక;
            VNRelinker handle;
            lastp->unlink_Fr_వెనుక(&handle);
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(fromDtp, కణుపు_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, BasicDType)) {
                ఒకవేళ (adtypep->isString()) {
                    AstConst* స్థిరరాశి leftp = కొత్త AstConst{fl, 0};
                    Ast_కణుపు_సమాసం* స్థిరరాశి rightp = కొత్త AstLenN{fl, fromp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } లేకపోతే {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } లేకపోతే ఒకవేళ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, వరుస_DType)) {
                AstConst* స్థిరరాశి leftp = కొత్త AstConst{fl, 0};
                Ast_కణుపు_సమాసం* స్థిరరాశి rightp = కొత్త AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), కణుపు_Array_DType)
                        ? కొత్త AstArraySel{fl, subfromp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                          కొత్త AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                    VCMethod::DYN_కొలత};
                AstVarRef* varRefp = కొత్త AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_సమితి_Int();
                rightp->protect(అబద్ధం);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = కొత్త AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } లేకపోతే ఒకవేళ (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = కొత్త AstVarRef{fl, varp, VAccess::READ};
                subfromp = కొత్త AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            ఒకవేళ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_వృక్షం(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_కణుపు* స్థిరరాశి bodyp = nodep->bodyp();
    ఒకవేళ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        ఒకవేళ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_వృక్షం(), bodyPointp);
        తిరిగి శూన్యసూచిక;
    }
    ఒకవేళ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_వెనుక_With_తదుపరి());
    } లేకపోతే {
        bodyPointp->unlink_Fr_వెనుక();
    }
    VL_DO_DANGLING(bodyPointp->delete_వృక్షం(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    తిరిగి newp;
}
