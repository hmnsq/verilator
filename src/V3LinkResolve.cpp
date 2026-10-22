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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkResolve.h"

#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

வகுப்பு LinkResolveVisitor final : பொது VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    மாறிலி VNUser2InUse m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Class we're inside
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    முழு m_senitem_Cvt_இலக்கம் = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    தருக்க m_underGenerate = பொய்;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        எனில் (nodep->dead()) திரும்பு;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_இலக்கம்);
        m_modp = nodep;
        m_senitem_Cvt_இலக்கம் = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        எனில் (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstVar* nodep) override {
        iterateChildren(nodep);
        எனில் (m_ftaskp) nodep->funcLocal(உண்மை);
        எனில் (nodep->isSigModPublic()) {
            nodep->sigModPublic(பொய்);  // We're done with this attribute
            m_modp->modPublic(உண்மை);  // Avoid flattening if signals are exposed
        }
    }

    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->usedParam(உண்மை);
            // Look for where genvar is valid
            தருக்க ok = பொய்;
            // cppcheck-suppress constVariablePointer
            க்கு (AstGenFor* மாறிலி forp : m_underGenFors) {
                எனில் (ok) நிறுத்து;
                எனில் (forp->initsp())
                    forp->initsp()->foreach([&](மாறிலி AstVarRef* refp) {  //
                        எனில் (refp->varp() == nodep->varp()) ok = உண்மை;
                    });
            }
            எனில் (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        எனில் (m_underGenerate) nodep->underGenerate(உண்மை);
        // Remember the existing symbol table scope
        எனில் (m_classp) nodep->classMethod(உண்மை);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(பொய்);
        எனில் (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        எனில் (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* மாறிலி fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_முனை* மாறிலி crstp = புதிய AstAssign{
                fvarp->fileline(), புதிய AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                புதிய AstCReset{fvarp->fileline(), fvarp, பொய்}};
            fvarp->noCReset(உண்மை);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        எனில் (nodep->dpiExport()) nodep->scopeNamep(புதிய AstScopeName{nodep->fileline(), பொய்});
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        எனில் (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            எனில் (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
            letp->user2(உண்மை);
            எனில் (VN_IS(nodep->backp(), Stmt_கோவை)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_கோவை* மாறிலி letStmtp = VN_AS(letp->stmtsp(), Stmt_கோவை);
            Ast_முனை_கோவை* மாறிலி newp = letStmtp->exprp()->clone_மரம்(பொய்);
            மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<மாறிலி AstVar*, Ast_முனை_கோவை*> portToExprs;
            க்கு (மாறிலி auto& tconnect : tconnects) {
                மாறிலி AstVar* மாறிலி portp = tconnect.first;
                மாறிலி AstArg* மாறிலி argp = tconnect.second;
                Ast_முனை_கோவை* மாறிலி pinp = argp->exprp();
                எனில் (!pinp) தொடர்;  // Argument error we'll find later
                portToExprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                மாறிலி auto it = portToExprs.find(refp->varp());
                எனில் (it != portToExprs.end()) {
                    Ast_முனை_கோவை* மாறிலி pinp = it->second;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_மரம்(பொய்));
                    VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            visit(newp);
            letp->user2(பொய்);
            திரும்பு;
        }
    }

    வெறுமை visit(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstStmtPragma* nodep) override {
        எனில் (nodep->pragp()->pragType() == VPragmaType::COVERAGE_BLOCK_OFF) {
            // Strip pragma if not needed, may optimize better without
            எனில் (!v3Global.opt.coverage_வரி() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstPragma* nodep) override {
        எனில் (nodep->pragType() == VPragmaType::HIER_BLOCK) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hierBlock(v3Global.opt.hierarchical());
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (nodep->pragType() == VPragmaType::PUBLIC_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->modPublic(உண்மை);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (nodep->pragType() == VPragmaType::PUBLIC_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->taskPublic(உண்மை);
            m_modp->modPublic(உண்மை);  // Need to get to the task...
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(உண்மை);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_முனை* nodep, மாறிலி string& format, Ast_முனை* argp, தருக்க isScan) {
        // Check display arguments, return new format string
        string newFormat;
        தருக்க inPct = பொய்;
        தருக்க inIgnore = பொய்;
        string fmt;
        string parseFormat = format;
        போது (!parseFormat.empty() || argp) {
            க்கு (மாறிலி எழுத்து ch : parseFormat) {
                எனில் (!inPct && ch == '%') {
                    inPct = உண்மை;
                    inIgnore = பொய்;
                    fmt = ch;
                } இல்லையெனில் எனில் (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } இல்லையெனில் எனில் (inPct) {
                    inPct = பொய்;
                    fmt += ch;
                    தேர்வு (std::tolower(ch)) {
                    நிலை '%':  // %% - just output a %
                        நிறுத்து;
                    நிலை '*':
                        inPct = உண்மை;
                        inIgnore = உண்மை;
                        நிறுத்து;
                    நிலை 'm':  // %m - auto insert "name"
                        எனில் (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        நிறுத்து;
                    நிலை 'l':  // %l - auto insert "library"
                        எனில் (isScan) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        எனில் (m_modp)
                            fmt = Ast_முனை::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        நிறுத்து;
                    இயல்பு:  // Most operators, just move to next argument
                        எனில் (!V3_எண்::displayedFmtHasArg(ch, isScan)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } இல்லையெனில் எனில் (!inIgnore) {
                            எனில் (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } இல்லையெனில் {
                                argp = argp->nextp();
                            }
                        }
                        நிறுத்து;
                    }  // switch
                    newFormat += fmt;
                } இல்லையெனில் {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            எனில் (isScan) நிறுத்து;
            போது (argp) {
                மாறிலி AstConst* மாறிலி constp = VN_CAST(argp, Const);
                மாறிலி தருக்க isFromString = (constp) ? constp->இலக்கம்().isFromString() : பொய்;
                எனில் (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } இல்லையெனில் {  // New format string
                    parseFormat += constp->இலக்கம்().toString();
                    Ast_முனை* மாறிலி nextp = argp->nextp();
                    argp->unlinkFrBack();
                    VL_DO_DANGLING(தள்ளு_Deletep(argp), argp);
                    argp = nextp;
                    நிறுத்து;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        திரும்பு newFormat;
    }

    நிலையான வெறுமை expectDescriptor(Ast_முனை* /*nodep*/, மாறிலி Ast_முனை_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        எனில் (filep && filep->varp()) filep->varp()->attrFileDescr(உண்மை);
    }

    வெறுமை visit(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), முனை_Var_Ref));
    }
    வெறுமை visit(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), முனை_Var_Ref));
    }
    வெறுமை visit(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), முனை_Var_Ref));
    }
    வெறுமை visit(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), முனை_Var_Ref));
    }
    வெறுமை visit(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), உண்மை);
    }
    வெறுமை visit(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->text(), nodep->exprsp(), உண்மை);
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        எனில் (nodep->user2SetOnce()) திரும்பு;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        எனில் (nodep->கோவை_Format_2()) {
            எனில் (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->இலக்கம்().isFromString()) {
                AstConst* மாறிலி fmtp = VN_AS(nodep->exprsp()->unlinkFrBack(), Const);
                nodep->text(fmtp->இலக்கம்().toString());
                nodep->கோவை_Format_2(பொய்);
                VL_DO_DANGLING(தள்ளு_Deletep(fmtp), fmtp);
            }
        }
        எனில் (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(பொய்);
            nodep->கோவை_Format_2(பொய்);
        }
        எனில் ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(புதிய AstScopeName{nodep->fileline(), உண்மை});
        }
        எனில் (!nodep->கோவை_Format_2()) {
            மாறிலி string newFormat = expectFormat(nodep, nodep->text(), nodep->exprsp(), பொய்);
            nodep->text(newFormat);
        }
    }

    வெறுமை visit(AstUdpTable* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        எனில் (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } இல்லையெனில் {
            // Massive hack, just tie off all outputs so our analysis can proceed
            மாறிலி AstVar* varoutp = nullptr;
            க்கு (Ast_முனை* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                    எனில் (varp->isReadOnly()) {
                    } இல்லையெனில் எனில் (varp->isWritable()) {
                        எனில் (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        AstAssignW* மாறிலி ap
                            = புதிய AstAssignW{varp->fileline(),
                                             புதிய AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             புதிய AstConst{varp->fileline(), AstConst::BitFalse{}}};
                        m_modp->addStmtsp(புதிய AstAlways{ap});
                    } இல்லையெனில் {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    வெறுமை visit(AstSystemCSection* nodep) override {
        தேர்வு (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        நிலை VSystemCSectionType::CTOR:
        நிலை VSystemCSectionType::DTOR:
        நிலை VSystemCSectionType::INT: m_modp->modPublic(உண்மை); நிறுத்து;
        இயல்பு: நிறுத்து;
        }
        // Has no children
    }

    வெறுமை visit(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(nullptr);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    வெறுமை visit(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = உண்மை;
        m_underGenFors.emplace_back(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.empty(), nodep, "Underflow");
        m_underGenFors.எடு_back();
    }
    வெறுமை visit(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = உண்மை;
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = இயல்பு;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

வகுப்பு LinkBotupVisitor final : பொது VNVisitorConst {
    // STATE
    Ast_முனை_Module* m_modp = nullptr;  // Current module

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCell* nodep) override {
        // Parent module inherits child's publicity
        எனில் (nodep->modp()->modPublic()) m_modp->modPublic(உண்மை);
        //** No iteration for speed
    }
    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = இயல்பு;
};

//######################################################################
// V3LinkResolve class functions

வெறுமை V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        மாறிலி LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("linkresolve", 0, dump_மரம்_Either_Level() >= 6);
}
