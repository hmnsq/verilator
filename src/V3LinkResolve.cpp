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
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    AstClass* m_classp = शून्यसूचकः;  // Class we're inside
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Function or task we're inside
    पूर्णाङ्क m_senitem_Cvt_अङ्कः = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    तार्किक m_underGenerate = असत्य;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        यदि (nodep->dead()) प्रत्यागम;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_अङ्कः);
        m_modp = nodep;
        m_senitem_Cvt_अङ्कः = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        यदि (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(AstVar* nodep) override {
        iterateChildren(nodep);
        यदि (m_ftaskp) nodep->funcLocal(सत्य);
        यदि (nodep->is_Sig_Mod_लौकिकम्()) {
            nodep->sig_Mod_लौकिकम्(असत्य);  // We're done with this attribute
            m_modp->mod_लौकिकम्(सत्य);  // Avoid flattening if signals are exposed
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->प्रयुक्तम्_Param(सत्य);
            // Look for where genvar is valid
            तार्किक ok = असत्य;
            // cppcheck-suppress constVariablePointer
            प्रति (AstGenFor* ध्रुव forp : m_underGenFors) {
                यदि (ok) भङ्ग;
                यदि (forp->initsp())
                    forp->initsp()->foreach([&](ध्रुव AstVarRef* refp) {  //
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

    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        यदि (m_underGenerate) nodep->underGenerate(सत्य);
        // Remember the existing symbol table scope
        यदि (m_classp) nodep->class_विधानम्(सत्य);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(असत्य);
        यदि (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        यदि (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ध्रुव fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_ग्रन्थिः* ध्रुव crstp = नूतन Ast_नियोजय{
                fvarp->fileline(), नूतन AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                नूतन AstCReset{fvarp->fileline(), fvarp, असत्य}};
            fvarp->noCReset(सत्य);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        यदि (nodep->dpiExport()) nodep->scopeNamep(नूतन AstScopeName{nodep->fileline(), असत्य});
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        यदि (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            यदि (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
            letp->user2(सत्य);
            यदि (VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_व्यञ्जकम्* ध्रुव letStmtp = VN_AS(letp->stmtsp(), Stmt_व्यञ्जकम्);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = letStmtp->exprp()->clone_वृक्षः(असत्य);
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> पत्तनम्_गन्तव्यम्_Exprs;
            प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
                यदि (!pinp) अनुवर्त;  // Argument error we'll find later
                पत्तनम्_गन्तव्यम्_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ध्रुव स्वचालित it = पत्तनम्_गन्तव्यम्_Exprs.find(refp->varp());
                यदि (it != पत्तनम्_गन्तव्यम्_Exprs.अन्तः_2()) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_वृक्षः(असत्य));
                    VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            दर्शय(newp);
            letp->user2(असत्य);
            प्रत्यागम;
        }
    }

    शून्य दर्शय(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstStmtPragma* nodep) override {
        यदि (nodep->pragp()->pragType() == VPragmaType::COVERAGE_खण्डम्_OFF) {
            // Strip pragma if not needed, may optimize better without
            यदि (!v3Global.opt.coverage_पङ्क्तिः() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
            }
        }
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstPragma* nodep) override {
        यदि (nodep->pragType() == VPragmaType::HIER_खण्डम्) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_खण्डम्(v3Global.opt.श्रेणीबद्धम्());
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->pragType() == VPragmaType::लौकिकम्_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_लौकिकम्(सत्य);
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->pragType() == VPragmaType::लौकिकम्_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_लौकिकम्(सत्य);
            m_modp->mod_लौकिकम्(सत्य);  // Need to get to the task...
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(सत्य);
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_ग्रन्थिः* nodep, ध्रुव string& format, Ast_ग्रन्थिः* argp, तार्किक is_अवलोकनम्) {
        // Check display arguments, return new format string
        string newFormat;
        तार्किक अन्तर्_Pct = असत्य;
        तार्किक अन्तर्_Ignore = असत्य;
        string fmt;
        string parseFormat = format;
        यावत् (!parseFormat.रिक्तानि() || argp) {
            प्रति (ध्रुव वर्ण ch : parseFormat) {
                यदि (!अन्तर्_Pct && ch == '%') {
                    अन्तर्_Pct = सत्य;
                    अन्तर्_Ignore = असत्य;
                    fmt = ch;
                } अन्यथा यदि (अन्तर्_Pct && (std::अङ्कः_वा(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } अन्यथा यदि (अन्तर्_Pct) {
                    अन्तर्_Pct = असत्य;
                    fmt += ch;
                    विकल्प (std::tolower(ch)) {
                    पक्ष '%':  // %% - just output a %
                        भङ्ग;
                    पक्ष '*':
                        अन्तर्_Pct = सत्य;
                        अन्तर्_Ignore = सत्य;
                        भङ्ग;
                    पक्ष 'm':  // %m - auto insert "name"
                        यदि (is_अवलोकनम्) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        भङ्ग;
                    पक्ष 'l':  // %l - auto insert "library"
                        यदि (is_अवलोकनम्) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        यदि (m_modp)
                            fmt = Ast_ग्रन्थिः::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        भङ्ग;
                    स्वभाव:  // Most operators, just move to next argument
                        यदि (!V3_सङ्ख्या::displayedFmtHasArg(ch, is_अवलोकनम्)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } अन्यथा यदि (!अन्तर्_Ignore) {
                            यदि (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } अन्यथा {
                                argp = argp->nextp();
                            }
                        }
                        भङ्ग;
                    }  // switch
                    newFormat += fmt;
                } अन्यथा {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            यदि (is_अवलोकनम्) भङ्ग;
            यावत् (argp) {
                ध्रुव AstConst* ध्रुव constp = VN_CAST(argp, Const);
                ध्रुव तार्किक isFromString = (constp) ? constp->अङ्कः().isFromString() : असत्य;
                यदि (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } अन्यथा {  // New format string
                    parseFormat += constp->अङ्कः().गन्तव्यम्_String();
                    Ast_ग्रन्थिः* ध्रुव nextp = argp->nextp();
                    argp->unlink_Fr_पृष्ठभागः();
                    VL_DO_DANGLING(प्रक्षिप_Deletep(argp), argp);
                    argp = nextp;
                    भङ्ग;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        प्रत्यागम newFormat;
    }

    स्थिर शून्य expectDescriptor(Ast_ग्रन्थिः* /*nodep*/, ध्रुव Ast_ग्रन्थिः_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        यदि (filep && filep->varp()) filep->varp()->attrFileDescr(सत्य);
    }

    शून्य दर्शय(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ग्रन्थिः_Var_Ref));
    }
    शून्य दर्शय(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ग्रन्थिः_Var_Ref));
    }
    शून्य दर्शय(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ग्रन्थिः_Var_Ref));
    }
    शून्य दर्शय(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), ग्रन्थिः_Var_Ref));
    }
    शून्य दर्शय(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), सत्य);
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), सत्य);
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        यदि (nodep->user2_समुच्चयः_Once()) प्रत्यागम;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        यदि (nodep->व्यञ्जकम्_Format_2()) {
            यदि (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->अङ्कः().isFromString()) {
                AstConst* ध्रुव fmtp = VN_AS(nodep->exprsp()->unlink_Fr_पृष्ठभागः(), Const);
                nodep->text(fmtp->अङ्कः().गन्तव्यम्_String());
                nodep->व्यञ्जकम्_Format_2(असत्य);
                VL_DO_DANGLING(प्रक्षिप_Deletep(fmtp), fmtp);
            }
        }
        यदि (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(असत्य);
            nodep->व्यञ्जकम्_Format_2(असत्य);
        }
        यदि ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(नूतन AstScopeName{nodep->fileline(), सत्य});
        }
        यदि (!nodep->व्यञ्जकम्_Format_2()) {
            ध्रुव string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), असत्य);
            nodep->text(newFormat);
        }
    }

    शून्य दर्शय(Ast_Udp_सारणी* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        यदि (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } अन्यथा {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ध्रुव AstVar* varoutp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                    यदि (varp->isReadOnly()) {
                    } अन्यथा यदि (varp->isWritable()) {
                        यदि (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_नियोजय_W* ध्रुव ap
                            = नूतन Ast_नियोजय_W{varp->fileline(),
                                             नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             नूतन AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->सङ्कलय_Stmtsp(नूतन AstAlways{ap});
                    } अन्यथा {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    शून्य दर्शय(AstSystemCSection* nodep) override {
        विकल्प (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        पक्ष VSystemCSectionType::CTOR:
        पक्ष VSystemCSectionType::DTOR:
        पक्ष VSystemCSectionType::INT: m_modp->mod_लौकिकम्(सत्य); भङ्ग;
        स्वभाव: भङ्ग;
        }
        // Has no children
    }

    शून्य दर्शय(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(शून्यसूचकः);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    शून्य दर्शय(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = सत्य;
        m_underGenFors.emplace_पृष्ठभागः(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.रिक्तानि(), nodep, "Underflow");
        m_underGenFors.निष्कासय_पृष्ठभागः();
    }
    शून्य दर्शय(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = सत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = स्वभाव;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

वर्ग LinkBotupVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCell* nodep) override {
        // Parent module inherits child's publicity
        यदि (nodep->modp()->mod_लौकिकम्()) m_modp->mod_लौकिकम्(सत्य);
        //** No iteration for speed
    }
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = स्वभाव;
};

//######################################################################
// V3LinkResolve class functions

शून्य V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ध्रुव LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("linkresolve", 0, dump_वृक्षः_Either_Level() >= 6);
}
