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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkResolve.h"

#समावेश "V3String.h"
#समावेश "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

श्रेणी LinkResolveVisitor final : खुला VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    स्थिर VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    पूर्णांक m_senitem_Cvt_अंक = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    bool m_underGenerate = false;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        अगर (nodep->dead()) वापस;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_अंक);
        m_modp = nodep;
        m_senitem_Cvt_अंक = 0;
        iterateChildren(nodep);
    }
    व्योम visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        अगर (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_Back_With_अग्र());
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(AstVar* nodep) override {
        iterateChildren(nodep);
        अगर (m_ftaskp) nodep->funcLocal(true);
        अगर (nodep->isSigModPublic()) {
            nodep->sigModPublic(false);  // We're done with this attribute
            m_modp->modPublic(true);  // Avoid flattening if signals are exposed
        }
    }

    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(true);
            // Look for where genvar is valid
            bool ok = false;
            // cppcheck-suppress constVariablePointer
            क्रम (AstGenFor* स्थिर forp : m_underGenFors) {
                अगर (ok) अवरोध;
                अगर (forp->initsp())
                    forp->initsp()->foreach([&](स्थिर AstVarRef* refp) {  //
                        अगर (refp->varp() == nodep->varp()) ok = true;
                    });
            }
            अगर (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        अगर (m_underGenerate) nodep->underGenerate(true);
        // Remember the existing symbol table scope
        अगर (m_classp) nodep->classMethod(true);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(false);
        अगर (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        अगर (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* स्थिर fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_गाँठ* स्थिर crstp = नया AstAssign{
                fvarp->fileline(), नया AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                नया AstCReset{fvarp->fileline(), fvarp, false}};
            fvarp->noCReset(true);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        अगर (nodep->dpiExport()) nodep->scopeNamep(नया AstScopeName{nodep->fileline(), false});
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        अगर (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            अगर (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            }
            letp->user2(true);
            अगर (VN_IS(nodep->backp(), Stmt_व्यंजक)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_व्यंजक* स्थिर letStmtp = VN_AS(letp->stmtsp(), Stmt_व्यंजक);
            Ast_गाँठ_व्यंजक* स्थिर newp = letStmtp->exprp()->clone_वृक्ष(false);
            स्थिर V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*> portToExprs;
            क्रम (स्थिर स्वतः& tconnect : tconnects) {
                स्थिर AstVar* स्थिर portp = tconnect.first;
                स्थिर AstArg* स्थिर argp = tconnect.second;
                Ast_गाँठ_व्यंजक* स्थिर pinp = argp->exprp();
                अगर (!pinp) जारी;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                स्थिर स्वतः it = portToExprs.find(refp->varp());
                अगर (it != portToExprs.end()) {
                    Ast_गाँठ_व्यंजक* स्थिर pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_वृक्ष(false));
                    VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(false);
            वापस;
        }
    }

    व्योम visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstStmtPragma* nodep) override {
        अगर (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            अगर (!v3Global.opt.coverage_पंक्ति() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstPragma* nodep) override {
        अगर (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(true);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(true);
            m_modp->modPublic(true);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(true);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_गाँठ* nodep, स्थिर string& format, Ast_गाँठ* argp, bool isScan) {
        // Check display arguments, return new format string
        string newFormat;
        bool inPct = false;
        bool inIgnore = false;
        string fmt;
        string parseFormat = format;
        जबतक (!parseFormat.empty() || argp) {
            क्रम (स्थिर अक्षर ch : parseFormat) {
                अगर (!inPct && ch == '%') {
                    inPct = true;
                    inIgnore = false;
                    fmt = ch;
                } अन्यथा अगर (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } अन्यथा अगर (inPct) {
                    inPct = false;
                    fmt += ch;
                    चयन (std::छोटे(ch)) {
                    हाल '%':  // %% - just output a %
                        अवरोध;
                    हाल '*':
                        inPct = true;
                        inIgnore = true;
                        अवरोध;
                    हाल 'm':  // %m - auto insert "name"
                        अगर (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        अवरोध;
                    हाल 'l':  // %l - auto insert "library"
                        अगर (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        अगर (m_modp)
                            fmt = Ast_गाँठ::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        अवरोध;
                    शेष:  // Most operators, just move to next argument
                        अगर (!V3_संख्या::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } अन्यथा अगर (!inIgnore) {
                            अगर (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } अन्यथा {
                                argp = argp->nextp();
                            }
                        }
                        अवरोध;
                    }  // switch
                    newFormat += fmt;
                } अन्यथा {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            अगर (isScan) अवरोध;
            जबतक (argp) {
                स्थिर AstConst* स्थिर constp = VN_CAST(argp, Const);
                स्थिर bool isFromString = (constp) ? constp->अंक().isFromString() : false;
                अगर (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } अन्यथा {  // New format string
                    parseFormat += constp->अंक().toString();
                    Ast_गाँठ* स्थिर nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(धकेलो_Deletep(argp), argp);
                    argp = nextp;
                    अवरोध;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        वापस newFormat;
    }

    अटल व्योम expectDescriptor(Ast_गाँठ* /*nodep*/, स्थिर Ast_गाँठ_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        अगर (filep && filep->varp()) filep->varp()->attrFileDescr(true);
    }

    व्योम visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठ_Var_Ref));
    }
    व्योम visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठ_Var_Ref));
    }
    व्योम visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठ_Var_Ref));
    }
    व्योम visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), गाँठ_Var_Ref));
    }
    व्योम visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), true);
    }
    व्योम visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), true);
    }
    व्योम visit(AstSFormatF* nodep) override {
        अगर (nodep->user2SetOnce()) वापस;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        अगर (nodep->व्यंजक_Format_2()) {
            अगर (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->अंक().isFromString()) {
                AstConst* स्थिर fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->अंक().toString());
                nodep->व्यंजक_Format_2(false);
                VL_DO_DANGLING(धकेलो_Deletep(fmtp), fmtp);
            }
        }
        अगर (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(false);
            nodep->व्यंजक_Format_2(false);
        }
        अगर ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(नया AstScopeName{nodep->fileline(), true});
        }
        अगर (!nodep->व्यंजक_Format_2()) {
            स्थिर string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), false);
            nodep->text(newFormat);
        }
    }

    व्योम visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        अगर (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } अन्यथा {
            // Massive hack, just tie off all outputs so our analysis can proceed
            स्थिर AstVar* varoutp = nullptr;
            क्रम (Ast_गाँठ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                    अगर (varp->isReadOnly()) {
                    } अन्यथा अगर (varp->isWritable()) {
                        अगर (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* स्थिर ap
                            = नया AstAssignW{varp->fileline(),
                                             नया AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             नया AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(नया AstAlways{ap});
                    } अन्यथा {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }

    व्योम visit(AstSystemCSection* nodep) override {
        चयन (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        हाल VSystemCSectionType::CTOR:
        हाल VSystemCSectionType::DTOR:
        हाल VSystemCSectionType::INT: m_modp->modPublic(true); अवरोध;
        शेष: अवरोध;
        }
        // Has no children
    }

    व्योम visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    व्योम visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = true;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.निकालो_back();
    }
    व्योम visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = true;
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = शेष;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

श्रेणी LinkBotupVisitor final : खुला VNVisitorConst {
    // STATE
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        अगर (nodep->modp()->modPublic()) m_modp->modPublic(true);
        //** No iteration for speed
    }
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = शेष;
};

//######################################################################
// V3LinkResolve class functions

व्योम V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        स्थिर LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("linkresolve", 0, dump_वृक्ष_Either_Level() >= 6);
}
