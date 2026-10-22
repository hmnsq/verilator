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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkResolve.h"

#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

वर्ग LinkResolveVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    अचर VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_गाँठो_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    पूर्णाङ्क m_senitem_Cvt_अङ्क = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    तार्किक m_underGenerate = असत्य;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        यदि (nodep->dead()) फर्काऊ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_अङ्क);
        m_modp = nodep;
        m_senitem_Cvt_अङ्क = 0;
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        लागि (Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    शून्य visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        यदि (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        यदि (m_ftaskp) nodep->funcLocal(सत्य);
        यदि (nodep->isSigModPublic()) {
            nodep->sigModPublic(असत्य);  // We're done with this attribute
            m_modp->modPublic(सत्य);  // Avoid flattening if signals are exposed
        }
    }

    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(सत्य);
            // Look for where genvar is valid
            तार्किक ok = असत्य;
            // cppcheck-suppress constVariablePointer
            लागि (AstGenFor* अचर forp : m_underGenFors) {
                यदि (ok) रोक;
                यदि (forp->initsp())
                    forp->initsp()->foreach([&](अचर AstVarRef* refp) {  //
                        यदि (refp->varp() == nodep->varp()) ok = सत्य;
                    });
            }
            यदि (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        यदि (m_underGenerate) nodep->underGenerate(सत्य);
        // Remember the existing symbol table scope
        यदि (m_classp) nodep->classMethod(सत्य);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(असत्य);
        यदि (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
            फर्काऊ;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        यदि (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* अचर fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_गाँठो* अचर crstp = नूतन AstAssign{
                fvarp->fileline(), नूतन AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                नूतन AstCReset{fvarp->fileline(), fvarp, असत्य}};
            fvarp->noCReset(सत्य);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        यदि (nodep->dpiExport()) nodep->scopeNamep(नूतन AstScopeName{nodep->fileline(), असत्य});
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        यदि (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            यदि (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                फर्काऊ;
            }
            letp->user2(सत्य);
            यदि (VN_IS(nodep->backp(), Stmt_अभिव्यक्ति)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_अभिव्यक्ति* अचर letStmtp = VN_AS(letp->stmtsp(), Stmt_अभिव्यक्ति);
            Ast_गाँठो_अभिव्यक्ति* अचर newp = letStmtp->exprp()->clone_रूख(असत्य);
            अचर V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*> portToExprs;
            लागि (अचर स्वतः& tconnect : tconnects) {
                अचर AstVar* अचर portp = tconnect.first;
                अचर AstArg* अचर argp = tconnect.second;
                Ast_गाँठो_अभिव्यक्ति* अचर pinp = argp->exprp();
                यदि (!pinp) जारी;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                अचर स्वतः it = portToExprs.find(refp->varp());
                यदि (it != portToExprs.end()) {
                    Ast_गाँठो_अभिव्यक्ति* अचर pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_रूख(असत्य));
                    VL_DO_DANGLING(थप_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(असत्य);
            फर्काऊ;
        }
    }

    शून्य visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstStmtPragma* nodep) override {
        यदि (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            यदि (!v3Global.opt.coverage_पङ्क्ति() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                फर्काऊ;
            }
        }
        iterateChildren(nodep);
    }

    शून्य visit(AstPragma* nodep) override {
        यदि (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(सत्य);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(सत्य);
            m_modp->modPublic(सत्य);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(सत्य);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_गाँठो* nodep, अचर string& format, Ast_गाँठो* argp, तार्किक isScan) {
        // Check display arguments, return new format string
        string newFormat;
        तार्किक inPct = असत्य;
        तार्किक inIgnore = असत्य;
        string fmt;
        string parseFormat = format;
        जबसम्म (!parseFormat.empty() || argp) {
            लागि (अचर वर्ण ch : parseFormat) {
                यदि (!inPct && ch == '%') {
                    inPct = सत्य;
                    inIgnore = असत्य;
                    fmt = ch;
                } अन्यथा यदि (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } अन्यथा यदि (inPct) {
                    inPct = असत्य;
                    fmt += ch;
                    विकल्प (std::छोटे(ch)) {
                    पक्ष '%':  // %% - just output a %
                        रोक;
                    पक्ष '*':
                        inPct = सत्य;
                        inIgnore = सत्य;
                        रोक;
                    पक्ष 'm':  // %m - auto insert "name"
                        यदि (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        रोक;
                    पक्ष 'l':  // %l - auto insert "library"
                        यदि (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        यदि (m_modp)
                            fmt = Ast_गाँठो::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        रोक;
                    स्वभाव:  // Most operators, just move to next argument
                        यदि (!V3_सङ्ख्या::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } अन्यथा यदि (!inIgnore) {
                            यदि (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } अन्यथा {
                                argp = argp->nextp();
                            }
                        }
                        रोक;
                    }  // switch
                    newFormat += fmt;
                } अन्यथा {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            यदि (isScan) रोक;
            जबसम्म (argp) {
                अचर AstConst* अचर constp = VN_CAST(argp, Const);
                अचर तार्किक isFromString = (constp) ? constp->अङ्क().isFromString() : असत्य;
                यदि (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } अन्यथा {  // New format string
                    parseFormat += constp->अङ्क().toString();
                    Ast_गाँठो* अचर nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(थप_Deletep(argp), argp);
                    argp = nextp;
                    रोक;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        फर्काऊ newFormat;
    }

    स्थिर शून्य expectDescriptor(Ast_गाँठो* /*nodep*/, अचर Ast_गाँठो_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        यदि (filep && filep->varp()) filep->varp()->attrFileDescr(सत्य);
    }

    शून्य visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठो_Var_Ref));
    }
    शून्य visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठो_Var_Ref));
    }
    शून्य visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठो_Var_Ref));
    }
    शून्य visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठो_Var_Ref));
    }
    शून्य visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), सत्य);
    }
    शून्य visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), सत्य);
    }
    शून्य visit(AstSFormatF* nodep) override {
        यदि (nodep->user2SetOnce()) फर्काऊ;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        यदि (nodep->अभिव्यक्ति_Format_2()) {
            यदि (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->अङ्क().isFromString()) {
                AstConst* अचर fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->अङ्क().toString());
                nodep->अभिव्यक्ति_Format_2(असत्य);
                VL_DO_DANGLING(थप_Deletep(fmtp), fmtp);
            }
        }
        यदि (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(असत्य);
            nodep->अभिव्यक्ति_Format_2(असत्य);
        }
        यदि ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(नूतन AstScopeName{nodep->fileline(), सत्य});
        }
        यदि (!nodep->अभिव्यक्ति_Format_2()) {
            अचर string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), असत्य);
            nodep->text(newFormat);
        }
    }

    शून्य visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        यदि (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } अन्यथा {
            // Massive hack, just tie off all outputs so our analysis can proceed
            अचर AstVar* varoutp = nullptr;
            लागि (Ast_गाँठो* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                    यदि (varp->isReadOnly()) {
                    } अन्यथा यदि (varp->isWritable()) {
                        यदि (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* अचर ap
                            = नूतन AstAssignW{varp->fileline(),
                                             नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             नूतन AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(नूतन AstAlways{ap});
                    } अन्यथा {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        }
    }

    शून्य visit(AstSystemCSection* nodep) override {
        विकल्प (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        पक्ष VSystemCSectionType::CTOR:
        पक्ष VSystemCSectionType::DTOR:
        पक्ष VSystemCSectionType::INT: m_modp->modPublic(सत्य); रोक;
        स्वभाव: रोक;
        }
        // Has no children
    }

    शून्य visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    शून्य visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = सत्य;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.झिक_back();
    }
    शून्य visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = स्वभाव;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

वर्ग LinkBotupVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        यदि (nodep->modp()->modPublic()) m_modp->modPublic(सत्य);
        //** No iteration for speed
    }
    शून्य visit(Ast_गाँठो_अभिव्यक्ति*) override {}  // Accelerate
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = स्वभाव;
};

//######################################################################
// V3LinkResolve class functions

शून्य V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        अचर LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("linkresolve", 0, dump_रूख_Either_Level() >= 6);
}
