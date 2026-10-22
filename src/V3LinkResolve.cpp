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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkResolve.h"

#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ವರ್ಗ LinkResolveVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    ಪೂರ್ಣಾಙ್ಕ m_senitem_Cvt_ಅಂಕೆ = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    ತಾರ್ಕಿಕ m_underGenerate = ಅಸತ್ಯ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        ಯದಿ (nodep->dead()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_ಅಂಕೆ);
        m_modp = nodep;
        m_senitem_Cvt_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        ಯದಿ (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_ftaskp) nodep->funcLocal(ಸತ್ಯ);
        ಯದಿ (nodep->isSigModPublic()) {
            nodep->sigModPublic(ಅಸತ್ಯ);  // We're done with this attribute
            m_modp->modPublic(ಸತ್ಯ);  // Avoid flattening if signals are exposed
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(ಸತ್ಯ);
            // Look for where genvar is valid
            ತಾರ್ಕಿಕ ok = ಅಸತ್ಯ;
            // cppcheck-suppress constVariablePointer
            ಪ್ರತಿ (AstGenFor* ಧ್ರುವ forp : m_underGenFors) {
                ಯದಿ (ok) ಭಙ್ಗ;
                ಯದಿ (forp->initsp())
                    forp->initsp()->foreach([&](ಧ್ರುವ AstVarRef* refp) {  //
                        ಯದಿ (refp->varp() == nodep->varp()) ok = ಸತ್ಯ;
                    });
            }
            ಯದಿ (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        ಯದಿ (m_underGenerate) nodep->underGenerate(ಸತ್ಯ);
        // Remember the existing symbol table scope
        ಯದಿ (m_classp) nodep->classMethod(ಸತ್ಯ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ಅಸತ್ಯ);
        ಯದಿ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        ಯದಿ (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ಧ್ರುವ fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ಗಂಟು* ಧ್ರುವ crstp = ನೂತನ AstAssign{
                fvarp->fileline(), ನೂತನ AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                ನೂತನ AstCReset{fvarp->fileline(), fvarp, ಅಸತ್ಯ}};
            fvarp->noCReset(ಸತ್ಯ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        ಯದಿ (nodep->dpiExport()) nodep->scopeNamep(ನೂತನ AstScopeName{nodep->fileline(), ಅಸತ್ಯ});
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            ಯದಿ (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            letp->user2(ಸತ್ಯ);
            ಯದಿ (VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ letStmtp = VN_AS(letp->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = letStmtp->exprp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> portToExprs;
            ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
                ಧ್ರುವ AstVar* ಧ್ರುವ portp = tconnect.first;
                ಧ್ರುವ AstArg* ಧ್ರುವ argp = tconnect.second;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = argp->exprp();
                ಯದಿ (!pinp) ಅನುವರ್ತ;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ಧ್ರುವ auto it = portToExprs.find(refp->varp());
                ಯದಿ (it != portToExprs.end()) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(ಅಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
    }

    ಶೂನ್ಯ visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstStmtPragma* nodep) override {
        ಯದಿ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            ಯದಿ (!v3Global.opt.coverage_ಸಾಲು() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstPragma* nodep) override {
        ಯದಿ (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(ಸತ್ಯ);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(ಸತ್ಯ);
            m_modp->modPublic(ಸತ್ಯ);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(ಸತ್ಯ);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& format, Ast_ಗಂಟು* argp, ತಾರ್ಕಿಕ isScan) {
        // Check display arguments, return new format string
        string newFormat;
        ತಾರ್ಕಿಕ inPct = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ inIgnore = ಅಸತ್ಯ;
        string fmt;
        string parseFormat = format;
        ಯಾವತ್ (!parseFormat.empty() || argp) {
            ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ ch : parseFormat) {
                ಯದಿ (!inPct && ch == '%') {
                    inPct = ಸತ್ಯ;
                    inIgnore = ಅಸತ್ಯ;
                    fmt = ch;
                } ಅನ್ಯಥಾ ಯದಿ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ಅನ್ಯಥಾ ಯದಿ (inPct) {
                    inPct = ಅಸತ್ಯ;
                    fmt += ch;
                    ವಿಕಲ್ಪ (std::tolower(ch)) {
                    ಪಕ್ಷ '%':  // %% - just output a %
                        ಭಙ್ಗ;
                    ಪಕ್ಷ '*':
                        inPct = ಸತ್ಯ;
                        inIgnore = ಸತ್ಯ;
                        ಭಙ್ಗ;
                    ಪಕ್ಷ 'm':  // %m - auto insert "name"
                        ಯದಿ (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        ಭಙ್ಗ;
                    ಪಕ್ಷ 'l':  // %l - auto insert "library"
                        ಯದಿ (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        ಯದಿ (m_modp)
                            fmt = Ast_ಗಂಟು::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        ಭಙ್ಗ;
                    ಸ್ವಭಾವ:  // Most operators, just move to next argument
                        ಯದಿ (!V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } ಅನ್ಯಥಾ ಯದಿ (!inIgnore) {
                            ಯದಿ (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } ಅನ್ಯಥಾ {
                                argp = argp->nextp();
                            }
                        }
                        ಭಙ್ಗ;
                    }  // switch
                    newFormat += fmt;
                } ಅನ್ಯಥಾ {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            ಯದಿ (isScan) ಭಙ್ಗ;
            ಯಾವತ್ (argp) {
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(argp, Const);
                ಧ್ರುವ ತಾರ್ಕಿಕ isFromString = (constp) ? constp->ಅಂಕೆ().isFromString() : ಅಸತ್ಯ;
                ಯದಿ (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } ಅನ್ಯಥಾ {  // New format string
                    parseFormat += constp->ಅಂಕೆ().toString();
                    Ast_ಗಂಟು* ಧ್ರುವ nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp), argp);
                    argp = nextp;
                    ಭಙ್ಗ;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ newFormat;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ expectDescriptor(Ast_ಗಂಟು* /*nodep*/, ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        ಯದಿ (filep && filep->varp()) filep->varp()->attrFileDescr(ಸತ್ಯ);
    }

    ಶೂನ್ಯ visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        ಯದಿ (nodep->user2SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        ಯದಿ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            ಯದಿ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ಅಂಕೆ().isFromString()) {
                AstConst* ಧ್ರುವ fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->ಅಂಕೆ().toString());
                nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(ಅಸತ್ಯ);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(fmtp), fmtp);
            }
        }
        ಯದಿ (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(ಅಸತ್ಯ);
            nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(ಅಸತ್ಯ);
        }
        ಯದಿ ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(ನೂತನ AstScopeName{nodep->fileline(), ಸತ್ಯ});
        }
        ಯದಿ (!nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            ಧ್ರುವ string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), ಅಸತ್ಯ);
            nodep->text(newFormat);
        }
    }

    ಶೂನ್ಯ visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        ಯದಿ (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } ಅನ್ಯಥಾ {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ಧ್ರುವ AstVar* varoutp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                    ಯದಿ (varp->isReadOnly()) {
                    } ಅನ್ಯಥಾ ಯದಿ (varp->isWritable()) {
                        ಯದಿ (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* ಧ್ರುವ ap
                            = ನೂತನ AstAssignW{varp->fileline(),
                                             ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             ನೂತನ AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(ನೂತನ AstAlways{ap});
                    } ಅನ್ಯಥಾ {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ visit(AstSystemCSection* nodep) override {
        ವಿಕಲ್ಪ (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        ಪಕ್ಷ VSystemCSectionType::CTOR:
        ಪಕ್ಷ VSystemCSectionType::DTOR:
        ಪಕ್ಷ VSystemCSectionType::INT: m_modp->modPublic(ಸತ್ಯ); ಭಙ್ಗ;
        ಸ್ವಭಾವ: ಭಙ್ಗ;
        }
        // Has no children
    }

    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    ಶೂನ್ಯ visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ಸತ್ಯ;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.ತೆಗೆ_back();
    }
    ಶೂನ್ಯ visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

ವರ್ಗ LinkBotupVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        ಯದಿ (nodep->modp()->modPublic()) m_modp->modPublic(ಸತ್ಯ);
        //** No iteration for speed
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// V3LinkResolve class functions

ಶೂನ್ಯ V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ಧ್ರುವ LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkresolve", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
