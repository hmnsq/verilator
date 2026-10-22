// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve module/signal name references
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
// LinkResolve TRANSFORMATIONS:
//      Top-down traversal
//          Extracts:
//              Add SUB so that we subtract off the "base 0-start" of the array
//          SelBit: Convert to ArraySel
//              Add SUB so that we subtract off the "base 0-start" of the array
//          File operations
//              Convert normal var to FILE* type
//          SenItems: Convert pos/negedge of non-simple signals to temporaries
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkResolve.h"

#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

తరగతి LinkResolveVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstClass* m_classp = శూన్యసూచిక;  // Class we're inside
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Function or task we're inside
    పూర్ణాంకం m_senitem_Cvt_అంకె = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    తార్కిక m_underGenerate = అబద్ధం;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        ఒకవేళ (nodep->dead()) తిరిగి;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_అంకె);
        m_modp = nodep;
        m_senitem_Cvt_అంకె = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        ఒకవేళ (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_ftaskp) nodep->funcLocal(నిజం);
        ఒకవేళ (nodep->is_Sig_Mod_ప్రజా()) {
            nodep->sig_Mod_ప్రజా(అబద్ధం);  // We're done with this attribute
            m_modp->mod_ప్రజా(నిజం);  // Avoid flattening if signals are exposed
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->వాడిన_Param(నిజం);
            // Look for where genvar is valid
            తార్కిక ok = అబద్ధం;
            // cppcheck-suppress constVariablePointer
            కొరకు (AstGenFor* స్థిరరాశి forp : m_underGenFors) {
                ఒకవేళ (ok) ఆపు;
                ఒకవేళ (forp->initsp())
                    forp->initsp()->foreach([&](స్థిరరాశి AstVarRef* refp) {  //
                        ఒకవేళ (refp->varp() == nodep->varp()) ok = నిజం;
                    });
            }
            ఒకవేళ (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        ఒకవేళ (m_underGenerate) nodep->underGenerate(నిజం);
        // Remember the existing symbol table scope
        ఒకవేళ (m_classp) nodep->class_పద్ధతి(నిజం);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(అబద్ధం);
        ఒకవేళ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        ఒకవేళ (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* స్థిరరాశి fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_కణుపు* స్థిరరాశి crstp = కొత్త Ast_కేటాయించు{
                fvarp->fileline(), కొత్త AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                కొత్త AstCReset{fvarp->fileline(), fvarp, అబద్ధం}};
            fvarp->noCReset(నిజం);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        ఒకవేళ (nodep->dpiExport()) nodep->scopeNamep(కొత్త AstScopeName{nodep->fileline(), అబద్ధం});
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            ఒకవేళ (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
            letp->user2(నిజం);
            ఒకవేళ (VN_IS(nodep->backp(), Stmt_సమాసం)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_సమాసం* స్థిరరాశి letStmtp = VN_AS(letp->stmtsp(), Stmt_సమాసం);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = letStmtp->exprp()->clone_వృక్షం(అబద్ధం);
            స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*> పోర్టు_గమ్యం_Exprs;
            కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
                స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
                స్థిరరాశి AstArg* స్థిరరాశి argp = tconnect.రెండవ;
                Ast_కణుపు_సమాసం* స్థిరరాశి pinp = argp->exprp();
                ఒకవేళ (!pinp) కొనసాగు;  // Argument error we'll find later
                పోర్టు_గమ్యం_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                స్థిరరాశి స్వయంచాలక it = పోర్టు_గమ్యం_Exprs.కనుగొను(refp->varp());
                ఒకవేళ (it != పోర్టు_గమ్యం_Exprs.చివర()) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి pinp = it->రెండవ;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_వృక్షం(అబద్ధం));
                    VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            సందర్శించు(newp);
            letp->user2(అబద్ధం);
            తిరిగి;
        }
    }

    శూన్య సందర్శించు(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstStmtPragma* nodep) override {
        ఒకవేళ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_ఖండం_OFF) {
            // Strip pragma if not needed, may optimize better without
            ఒకవేళ (!v3Global.opt.coverage_పంక్తి() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstPragma* nodep) override {
        ఒకవేళ (nodep->pragType() == VPragmaType::HIER_ఖండం) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_ఖండం(v3Global.opt.క్రమానుగత());
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (nodep->pragType() == VPragmaType::ప్రజా_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_ప్రజా(నిజం);
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (nodep->pragType() == VPragmaType::ప్రజా_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_ప్రజా(నిజం);
            m_modp->mod_ప్రజా(నిజం);  // Need to get to the task...
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(నిజం);
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_కణుపు* nodep, స్థిరరాశి string& format, Ast_కణుపు* argp, తార్కిక is_పరిశీలన) {
        // Check display arguments, return new format string
        string newFormat;
        తార్కిక లోపల_Pct = అబద్ధం;
        తార్కిక లోపల_Ignore = అబద్ధం;
        string fmt;
        string parseFormat = format;
        అయితే (!parseFormat.ఖాళీలు() || argp) {
            కొరకు (స్థిరరాశి అక్షరం ch : parseFormat) {
                ఒకవేళ (!లోపల_Pct && ch == '%') {
                    లోపల_Pct = నిజం;
                    లోపల_Ignore = అబద్ధం;
                    fmt = ch;
                } లేకపోతే ఒకవేళ (లోపల_Pct && (std::అంకెనా(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } లేకపోతే ఒకవేళ (లోపల_Pct) {
                    లోపల_Pct = అబద్ధం;
                    fmt += ch;
                    ఎంపిక (std::tolower(ch)) {
                    స్థితి '%':  // %% - just output a %
                        ఆపు;
                    స్థితి '*':
                        లోపల_Pct = నిజం;
                        లోపల_Ignore = నిజం;
                        ఆపు;
                    స్థితి 'm':  // %m - auto insert "name"
                        ఒకవేళ (is_పరిశీలన) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        ఆపు;
                    స్థితి 'l':  // %l - auto insert "library"
                        ఒకవేళ (is_పరిశీలన) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        ఒకవేళ (m_modp)
                            fmt = Ast_కణుపు::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        ఆపు;
                    అప్రమేయ:  // Most operators, just move to next argument
                        ఒకవేళ (!V3_సంఖ్య::displayedFmtHasArg(ch, is_పరిశీలన)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } లేకపోతే ఒకవేళ (!లోపల_Ignore) {
                            ఒకవేళ (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } లేకపోతే {
                                argp = argp->nextp();
                            }
                        }
                        ఆపు;
                    }  // switch
                    newFormat += fmt;
                } లేకపోతే {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            ఒకవేళ (is_పరిశీలన) ఆపు;
            అయితే (argp) {
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(argp, Const);
                స్థిరరాశి తార్కిక isFromString = (constp) ? constp->అంకె().isFromString() : అబద్ధం;
                ఒకవేళ (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } లేకపోతే {  // New format string
                    parseFormat += constp->అంకె().గమ్యం_String();
                    Ast_కణుపు* స్థిరరాశి nextp = argp->nextp();
                    argp->unlink_Fr_వెనుక();
                    VL_DO_DANGLING(నెట్టు_Deletep(argp), argp);
                    argp = nextp;
                    ఆపు;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        తిరిగి newFormat;
    }

    స్థిర శూన్య expectDescriptor(Ast_కణుపు* /*nodep*/, స్థిరరాశి Ast_కణుపు_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        ఒకవేళ (filep && filep->varp()) filep->varp()->attrFileDescr(నిజం);
    }

    శూన్య సందర్శించు(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), కణుపు_Var_Ref));
    }
    శూన్య సందర్శించు(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), కణుపు_Var_Ref));
    }
    శూన్య సందర్శించు(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), కణుపు_Var_Ref));
    }
    శూన్య సందర్శించు(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), కణుపు_Var_Ref));
    }
    శూన్య సందర్శించు(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->పాఠ్యం(), nodep->exprsp(), నిజం);
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->పాఠ్యం(), nodep->exprsp(), నిజం);
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        ఒకవేళ (nodep->user2_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        ఒకవేళ (nodep->సమాసం_Format_2()) {
            ఒకవేళ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->అంకె().isFromString()) {
                AstConst* స్థిరరాశి fmtp = VN_AS(nodep->exprsp()->unlink_Fr_వెనుక(), Const);
                nodep->పాఠ్యం(fmtp->అంకె().గమ్యం_String());
                nodep->సమాసం_Format_2(అబద్ధం);
                VL_DO_DANGLING(నెట్టు_Deletep(fmtp), fmtp);
            }
        }
        ఒకవేళ (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(అబద్ధం);
            nodep->సమాసం_Format_2(అబద్ధం);
        }
        ఒకవేళ ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(కొత్త AstScopeName{nodep->fileline(), నిజం});
        }
        ఒకవేళ (!nodep->సమాసం_Format_2()) {
            స్థిరరాశి string newFormat = expectFormat(nodep, nodep->పాఠ్యం(), nodep->exprsp(), అబద్ధం);
            nodep->పాఠ్యం(newFormat);
        }
    }

    శూన్య సందర్శించు(Ast_Udp_పట్టిక* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        ఒకవేళ (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } లేకపోతే {
            // Massive hack, just tie off all outputs so our analysis can proceed
            స్థిరరాశి AstVar* varoutp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                    ఒకవేళ (varp->isReadOnly()) {
                    } లేకపోతే ఒకవేళ (varp->isWritable()) {
                        ఒకవేళ (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_కేటాయించు_W* స్థిరరాశి ap
                            = కొత్త Ast_కేటాయించు_W{varp->fileline(),
                                             కొత్త AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             కొత్త AstConst{varp->fileline(), AstConst::బిట్టు_False{}}};
                        m_modp->కూడు_Stmtsp(కొత్త AstAlways{ap});
                    } లేకపోతే {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    శూన్య సందర్శించు(AstSystemCSection* nodep) override {
        ఎంపిక (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        స్థితి VSystemCSectionType::CTOR:
        స్థితి VSystemCSectionType::DTOR:
        స్థితి VSystemCSectionType::INT: m_modp->mod_ప్రజా(నిజం); ఆపు;
        అప్రమేయ: ఆపు;
        }
        // Has no children
    }

    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(శూన్యసూచిక);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    శూన్య సందర్శించు(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = నిజం;
        m_underGenFors.emplace_వెనుక(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.ఖాళీలు(), nodep, "Underflow");
        m_underGenFors.తీయి_వెనుక();
    }
    శూన్య సందర్శించు(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = నిజం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = అప్రమేయ;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

తరగతి LinkBotupVisitor final : బహిరంగ VNVisitorConst {
    // STATE
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        // Parent module inherits child's publicity
        ఒకవేళ (nodep->modp()->mod_ప్రజా()) m_modp->mod_ప్రజా(నిజం);
        //** No iteration for speed
    }
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = అప్రమేయ;
};

//######################################################################
// V3LinkResolve class functions

శూన్య V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        స్థిరరాశి LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("linkresolve", 0, dump_వృక్షం_Either_Level() >= 6);
}
