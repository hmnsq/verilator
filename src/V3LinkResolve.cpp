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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkResolve.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ਵਰਗ LinkResolveVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Class we're inside
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Function or task we're inside
    ਪੂਰਨ m_senitem_Cvt_ਅੰਕ = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    ਤਾਰਕਿਕ m_underGenerate = ਝੂਠ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        ਜੇ (nodep->dead()) ਵਾਪਸ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_ਅੰਕ);
        m_modp = nodep;
        m_senitem_Cvt_ਅੰਕ = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        ਜੇ (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_ftaskp) nodep->funcLocal(ਸੱਚ);
        ਜੇ (nodep->is_Sig_Mod_ਜਨਤਕ()) {
            nodep->sig_Mod_ਜਨਤਕ(ਝੂਠ);  // We're done with this attribute
            m_modp->mod_ਜਨਤਕ(ਸੱਚ);  // Avoid flattening if signals are exposed
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->ਵਰਤਿਆ_Param(ਸੱਚ);
            // Look for where genvar is valid
            ਤਾਰਕਿਕ ok = ਝੂਠ;
            // cppcheck-suppress constVariablePointer
            ਲਈ (AstGenFor* ਅਚਲ forp : m_underGenFors) {
                ਜੇ (ok) ਰੋਕੋ;
                ਜੇ (forp->initsp())
                    forp->initsp()->foreach([&](ਅਚਲ AstVarRef* refp) {  //
                        ਜੇ (refp->varp() == nodep->varp()) ok = ਸੱਚ;
                    });
            }
            ਜੇ (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        ਜੇ (m_underGenerate) nodep->underGenerate(ਸੱਚ);
        // Remember the existing symbol table scope
        ਜੇ (m_classp) nodep->class_ਵਿਧੀ(ਸੱਚ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(ਝੂਠ);
        ਜੇ (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        ਜੇ (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ਅਚਲ fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ਗੰਢ* ਅਚਲ crstp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                fvarp->fileline(), ਨਵਾਂ AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                ਨਵਾਂ AstCReset{fvarp->fileline(), fvarp, ਝੂਠ}};
            fvarp->noCReset(ਸੱਚ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        ਜੇ (nodep->dpiExport()) nodep->scopeNamep(ਨਵਾਂ AstScopeName{nodep->fileline(), ਝੂਠ});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        ਜੇ (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            ਜੇ (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
            letp->user2(ਸੱਚ);
            ਜੇ (VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ letStmtp = VN_AS(letp->stmtsp(), Stmt_ਵਿਅੰਜਕ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = letStmtp->exprp()->clone_ਰੁੱਖ(ਝੂਠ);
            ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਪੋਰਟ_ਮੰਜ਼ਿਲ_Exprs;
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
                ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
                ਅਚਲ AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = argp->exprp();
                ਜੇ (!pinp) ਜਾਰੀ;  // Argument error we'll find later
                ਪੋਰਟ_ਮੰਜ਼ਿਲ_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = ਪੋਰਟ_ਮੰਜ਼ਿਲ_Exprs.ਲੱਭੋ(refp->varp());
                ਜੇ (it != ਪੋਰਟ_ਮੰਜ਼ਿਲ_Exprs.ਸਿਰਾ()) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = it->ਦੂਜਾ;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_ਰੁੱਖ(ਝੂਠ));
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            ਦੇਖੋ(newp);
            letp->user2(ਝੂਠ);
            ਵਾਪਸ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstStmtPragma* nodep) override {
        ਜੇ (nodep->pragp()->pragType() == VPragmaType::COVERAGE_ਖੰਡ_OFF) {
            // Strip pragma if not needed, may optimize better without
            ਜੇ (!v3Global.opt.coverage_ਸਤਰ() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstPragma* nodep) override {
        ਜੇ (nodep->pragType() == VPragmaType::HIER_ਖੰਡ) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_ਖੰਡ(v3Global.opt.ਦਰਜਾਵਾਰ());
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->pragType() == VPragmaType::ਜਨਤਕ_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_ਜਨਤਕ(ਸੱਚ);
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->pragType() == VPragmaType::ਜਨਤਕ_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_ਜਨਤਕ(ਸੱਚ);
            m_modp->mod_ਜਨਤਕ(ਸੱਚ);  // Need to get to the task...
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(ਸੱਚ);
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ਗੰਢ* nodep, ਅਚਲ string& format, Ast_ਗੰਢ* argp, ਤਾਰਕਿਕ is_ਜਾਂਚ) {
        // Check display arguments, return new format string
        string newFormat;
        ਤਾਰਕਿਕ ਵਿੱਚ_Pct = ਝੂਠ;
        ਤਾਰਕਿਕ ਵਿੱਚ_Ignore = ਝੂਠ;
        string fmt;
        string parseFormat = format;
        ਜਦੋਂ (!parseFormat.ਖਾਲੀ_ਥਾਵਾਂ() || argp) {
            ਲਈ (ਅਚਲ ਅੱਖਰ ch : parseFormat) {
                ਜੇ (!ਵਿੱਚ_Pct && ch == '%') {
                    ਵਿੱਚ_Pct = ਸੱਚ;
                    ਵਿੱਚ_Ignore = ਝੂਠ;
                    fmt = ch;
                } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Pct && (std::ਅੰਕ_ਹੈ(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Pct) {
                    ਵਿੱਚ_Pct = ਝੂਠ;
                    fmt += ch;
                    ਚੋਣ (std::tolower(ch)) {
                    ਹਾਲਤ '%':  // %% - just output a %
                        ਰੋਕੋ;
                    ਹਾਲਤ '*':
                        ਵਿੱਚ_Pct = ਸੱਚ;
                        ਵਿੱਚ_Ignore = ਸੱਚ;
                        ਰੋਕੋ;
                    ਹਾਲਤ 'm':  // %m - auto insert "name"
                        ਜੇ (is_ਜਾਂਚ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        ਰੋਕੋ;
                    ਹਾਲਤ 'l':  // %l - auto insert "library"
                        ਜੇ (is_ਜਾਂਚ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        ਜੇ (m_modp)
                            fmt = Ast_ਗੰਢ::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        ਰੋਕੋ;
                    ਮੂਲ:  // Most operators, just move to next argument
                        ਜੇ (!V3_ਸੰਖਿਆ::displayedFmtHasArg(ch, is_ਜਾਂਚ)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } ਨਹੀਂਤਾਂ ਜੇ (!ਵਿੱਚ_Ignore) {
                            ਜੇ (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } ਨਹੀਂਤਾਂ {
                                argp = argp->nextp();
                            }
                        }
                        ਰੋਕੋ;
                    }  // switch
                    newFormat += fmt;
                } ਨਹੀਂਤਾਂ {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            ਜੇ (is_ਜਾਂਚ) ਰੋਕੋ;
            ਜਦੋਂ (argp) {
                ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(argp, Const);
                ਅਚਲ ਤਾਰਕਿਕ isFromString = (constp) ? constp->ਅੰਕ().isFromString() : ਝੂਠ;
                ਜੇ (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } ਨਹੀਂਤਾਂ {  // New format string
                    parseFormat += constp->ਅੰਕ().ਮੰਜ਼ਿਲ_String();
                    Ast_ਗੰਢ* ਅਚਲ nextp = argp->nextp();
                    argp->unlink_Fr_ਪਿੱਠ();
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(argp), argp);
                    argp = nextp;
                    ਰੋਕੋ;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        ਵਾਪਸ newFormat;
    }

    ਸਥਿਰ ਖਾਲੀ expectDescriptor(Ast_ਗੰਢ* /*nodep*/, ਅਚਲ Ast_ਗੰਢ_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        ਜੇ (filep && filep->varp()) filep->varp()->attrFileDescr(ਸੱਚ);
    }

    ਖਾਲੀ ਦੇਖੋ(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ਗੰਢ_Var_Ref));
    }
    ਖਾਲੀ ਦੇਖੋ(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ਗੰਢ_Var_Ref));
    }
    ਖਾਲੀ ਦੇਖੋ(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ਗੰਢ_Var_Ref));
    }
    ਖਾਲੀ ਦੇਖੋ(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ਗੰਢ_Var_Ref));
    }
    ਖਾਲੀ ਦੇਖੋ(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->ਪਾਠ(), nodep->exprsp(), ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->ਪਾਠ(), nodep->exprsp(), ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        ਜੇ (nodep->user2_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        ਜੇ (nodep->ਵਿਅੰਜਕ_Format_2()) {
            ਜੇ (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->ਅੰਕ().isFromString()) {
                AstConst* ਅਚਲ fmtp = VN_AS(nodep->exprsp()->unlink_Fr_ਪਿੱਠ(), Const);
                nodep->ਪਾਠ(fmtp->ਅੰਕ().ਮੰਜ਼ਿਲ_String());
                nodep->ਵਿਅੰਜਕ_Format_2(ਝੂਠ);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(fmtp), fmtp);
            }
        }
        ਜੇ (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(ਝੂਠ);
            nodep->ਵਿਅੰਜਕ_Format_2(ਝੂਠ);
        }
        ਜੇ ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(ਨਵਾਂ AstScopeName{nodep->fileline(), ਸੱਚ});
        }
        ਜੇ (!nodep->ਵਿਅੰਜਕ_Format_2()) {
            ਅਚਲ string newFormat = expectFormat(nodep, nodep->ਪਾਠ(), nodep->exprsp(), ਝੂਠ);
            nodep->ਪਾਠ(newFormat);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Udp_ਸਾਰਣੀ* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        ਜੇ (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } ਨਹੀਂਤਾਂ {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ਅਚਲ AstVar* varoutp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                    ਜੇ (varp->isReadOnly()) {
                    } ਨਹੀਂਤਾਂ ਜੇ (varp->isWritable()) {
                        ਜੇ (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_ਸੌਂਪੋ_W* ਅਚਲ ap
                            = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{varp->fileline(),
                                             ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             ਨਵਾਂ AstConst{varp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}};
                        m_modp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{ap});
                    } ਨਹੀਂਤਾਂ {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstSystemCSection* nodep) override {
        ਚੋਣ (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        ਹਾਲਤ VSystemCSectionType::CTOR:
        ਹਾਲਤ VSystemCSectionType::DTOR:
        ਹਾਲਤ VSystemCSectionType::INT: m_modp->mod_ਜਨਤਕ(ਸੱਚ); ਰੋਕੋ;
        ਮੂਲ: ਰੋਕੋ;
        }
        // Has no children
    }

    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(ਖਾਲੀ_ਸੂਚਕ);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    ਖਾਲੀ ਦੇਖੋ(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ਸੱਚ;
        m_underGenFors.emplace_ਪਿੱਠ(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Underflow");
        m_underGenFors.ਕੱਢੋ_ਪਿੱਠ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = ਸੱਚ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = ਮੂਲ;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

ਵਰਗ LinkBotupVisitor final : ਜਨਤਕ VNVisitorConst {
    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        // Parent module inherits child's publicity
        ਜੇ (nodep->modp()->mod_ਜਨਤਕ()) m_modp->mod_ਜਨਤਕ(ਸੱਚ);
        //** No iteration for speed
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = ਮੂਲ;
};

//######################################################################
// V3LinkResolve class functions

ਖਾਲੀ V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ਅਚਲ LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("linkresolve", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
