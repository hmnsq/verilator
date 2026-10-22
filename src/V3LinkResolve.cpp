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
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Class we're inside
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Function or task we're inside
    ಪೂರ್ಣಾಂಕ m_senitem_Cvt_ಅಂಕೆ = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    ತಾರ್ಕಿಕ m_underGenerate = ಸುಳ್ಳು;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        ಆದರೆ (nodep->dead()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_ಅಂಕೆ);
        m_modp = nodep;
        m_senitem_Cvt_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        ಆದರೆ (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_ftaskp) nodep->funcLocal(ನಿಜ);
        ಆದರೆ (nodep->is_Sig_Mod_ಬಹಿರಂಗ()) {
            nodep->sig_Mod_ಬಹಿರಂಗ(ಸುಳ್ಳು);  // We're done with this attribute
            m_modp->mod_ಬಹಿರಂಗ(ನಿಜ);  // Avoid flattening if signals are exposed
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->ಬಳಸಿದ_Param(ನಿಜ);
            // Look for where genvar is valid
            ತಾರ್ಕಿಕ ok = ಸುಳ್ಳು;
            // cppcheck-suppress constVariablePointer
            ಗಾಗಿ (AstGenFor* ಸ್ಥಿರಾಂಕ forp : m_underGenFors) {
                ಆದರೆ (ok) ನಿಲ್ಲಿಸು;
                ಆದರೆ (forp->initsp())
                    forp->initsp()->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {  //
                        ಆದರೆ (refp->varp() == nodep->varp()) ok = ನಿಜ;
                    });
            }
            ಆದರೆ (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        ಆದರೆ (m_underGenerate) nodep->underGenerate(ನಿಜ);
        // Remember the existing symbol table scope
        ಆದರೆ (m_classp) nodep->class_ವಿಧಾನ(ನಿಜ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ಸುಳ್ಳು);
        ಆದರೆ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        ಆದರೆ (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ಸ್ಥಿರಾಂಕ fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ crstp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                fvarp->fileline(), ಹೊಸ AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                ಹೊಸ AstCReset{fvarp->fileline(), fvarp, ಸುಳ್ಳು}};
            fvarp->noCReset(ನಿಜ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        ಆದರೆ (nodep->dpiExport()) nodep->scopeNamep(ಹೊಸ AstScopeName{nodep->fileline(), ಸುಳ್ಳು});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            ಆದರೆ (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            letp->user2(ನಿಜ);
            ಆದರೆ (VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ letStmtp = VN_AS(letp->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = letStmtp->exprp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಪೋರ್ಟ್_ಗಮ್ಯ_Exprs;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = argp->exprp();
                ಆದರೆ (!pinp) ಮುಂದುವರಿ;  // Argument error we'll find later
                ಪೋರ್ಟ್_ಗಮ್ಯ_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = ಪೋರ್ಟ್_ಗಮ್ಯ_Exprs.ಕಂಡುಹಿಡಿ(refp->varp());
                ಆದರೆ (it != ಪೋರ್ಟ್_ಗಮ್ಯ_Exprs.ಕೊನೆ()) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = it->ಎರಡನೇ;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            ಭೇಟಿ(newp);
            letp->user2(ಸುಳ್ಳು);
            ಹಿಂತಿರುಗಿ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstStmtPragma* nodep) override {
        ಆದರೆ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_ಖಂಡ_OFF) {
            // Strip pragma if not needed, may optimize better without
            ಆದರೆ (!v3Global.opt.coverage_ಸಾಲು() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstPragma* nodep) override {
        ಆದರೆ (nodep->pragType() == VPragmaType::HIER_ಖಂಡ) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_ಖಂಡ(v3Global.opt.ಶ್ರೇಣೀಕೃತ());
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->pragType() == VPragmaType::ಬಹಿರಂಗ_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_ಬಹಿರಂಗ(ನಿಜ);
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->pragType() == VPragmaType::ಬಹಿರಂಗ_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_ಬಹಿರಂಗ(ನಿಜ);
            m_modp->mod_ಬಹಿರಂಗ(ನಿಜ);  // Need to get to the task...
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(ನಿಜ);
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& format, Ast_ಗಂಟು* argp, ತಾರ್ಕಿಕ is_ಪರಿಶೀಲನೆ) {
        // Check display arguments, return new format string
        string newFormat;
        ತಾರ್ಕಿಕ ಒಳ_Pct = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ ಒಳ_Ignore = ಸುಳ್ಳು;
        string fmt;
        string parseFormat = format;
        ಆಗ (!parseFormat.ಖಾಲಿಗಳು() || argp) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ch : parseFormat) {
                ಆದರೆ (!ಒಳ_Pct && ch == '%') {
                    ಒಳ_Pct = ನಿಜ;
                    ಒಳ_Ignore = ಸುಳ್ಳು;
                    fmt = ch;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Pct && (std::ಅಂಕಿಯೇ(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Pct) {
                    ಒಳ_Pct = ಸುಳ್ಳು;
                    fmt += ch;
                    ಆಯ್ಕೆ (std::tolower(ch)) {
                    ಸ್ಥಿತಿ '%':  // %% - just output a %
                        ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ '*':
                        ಒಳ_Pct = ನಿಜ;
                        ಒಳ_Ignore = ನಿಜ;
                        ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ 'm':  // %m - auto insert "name"
                        ಆದರೆ (is_ಪರಿಶೀಲನೆ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ 'l':  // %l - auto insert "library"
                        ಆದರೆ (is_ಪರಿಶೀಲನೆ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        ಆದರೆ (m_modp)
                            fmt = Ast_ಗಂಟು::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        ನಿಲ್ಲಿಸು;
                    ಪೂರ್ವನಿಯೋಜಿತ:  // Most operators, just move to next argument
                        ಆದರೆ (!V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(ch, is_ಪರಿಶೀಲನೆ)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಒಳ_Ignore) {
                            ಆದರೆ (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                argp = argp->nextp();
                            }
                        }
                        ನಿಲ್ಲಿಸು;
                    }  // switch
                    newFormat += fmt;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            ಆದರೆ (is_ಪರಿಶೀಲನೆ) ನಿಲ್ಲಿಸು;
            ಆಗ (argp) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(argp, Const);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isFromString = (constp) ? constp->ಅಂಕೆ().isFromString() : ಸುಳ್ಳು;
                ಆದರೆ (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } ಇಲ್ಲದಿದ್ದರೆ {  // New format string
                    parseFormat += constp->ಅಂಕೆ().ಗಮ್ಯ_String();
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = argp->nextp();
                    argp->unlink_Fr_ಹಿಂಭಾಗ();
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp), argp);
                    argp = nextp;
                    ನಿಲ್ಲಿಸು;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        ಹಿಂತಿರುಗಿ newFormat;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ expectDescriptor(Ast_ಗಂಟು* /*nodep*/, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        ಆದರೆ (filep && filep->varp()) filep->varp()->attrFileDescr(ನಿಜ);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ಗಂಟು_Var_Ref));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->ಪಠ್ಯ(), nodep->exprsp(), ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->ಪಠ್ಯ(), nodep->exprsp(), ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        ಆದರೆ (nodep->user2_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        ಆದರೆ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            ಆದರೆ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ಅಂಕೆ().isFromString()) {
                AstConst* ಸ್ಥಿರಾಂಕ fmtp = VN_AS(nodep->exprsp()->unlink_Fr_ಹಿಂಭಾಗ(), Const);
                nodep->ಪಠ್ಯ(fmtp->ಅಂಕೆ().ಗಮ್ಯ_String());
                nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(ಸುಳ್ಳು);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(fmtp), fmtp);
            }
        }
        ಆದರೆ (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(ಸುಳ್ಳು);
            nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(ಸುಳ್ಳು);
        }
        ಆದರೆ ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(ಹೊಸ AstScopeName{nodep->fileline(), ನಿಜ});
        }
        ಆದರೆ (!nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            ಸ್ಥಿರಾಂಕ string newFormat = expectFormat(nodep, nodep->ಪಠ್ಯ(), nodep->exprsp(), ಸುಳ್ಳು);
            nodep->ಪಠ್ಯ(newFormat);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Udp_ಕೋಷ್ಟಕ* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        ಆದರೆ (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ಸ್ಥಿರಾಂಕ AstVar* varoutp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                    ಆದರೆ (varp->isReadOnly()) {
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isWritable()) {
                        ಆದರೆ (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap
                            = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{varp->fileline(),
                                             ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             ಹೊಸ AstConst{varp->fileline(), AstConst::ಬಿಟ್_False{}}};
                        m_modp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{ap});
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSystemCSection* nodep) override {
        ಆಯ್ಕೆ (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        ಸ್ಥಿತಿ VSystemCSectionType::CTOR:
        ಸ್ಥಿತಿ VSystemCSectionType::DTOR:
        ಸ್ಥಿತಿ VSystemCSectionType::INT: m_modp->mod_ಬಹಿರಂಗ(ನಿಜ); ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
        }
        // Has no children
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(ಶೂನ್ಯಸೂಚಕ);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    ಶೂನ್ಯ ಭೇಟಿ(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ನಿಜ;
        m_underGenFors.emplace_ಹಿಂಭಾಗ(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.ಖಾಲಿಗಳು(), nodep, "Underflow");
        m_underGenFors.ತೆಗೆ_ಹಿಂಭಾಗ();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ನಿಜ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

ವರ್ಗ LinkBotupVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        // Parent module inherits child's publicity
        ಆದರೆ (nodep->modp()->mod_ಬಹಿರಂಗ()) m_modp->mod_ಬಹಿರಂಗ(ನಿಜ);
        //** No iteration for speed
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// V3LinkResolve class functions

ಶೂನ್ಯ V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ಸ್ಥಿರಾಂಕ LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkresolve", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
