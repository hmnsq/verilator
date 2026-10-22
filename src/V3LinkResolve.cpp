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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkResolve.h"

#شامل "V3String.h"
#شامل "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

جماعت LinkResolveVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  Entire netlist:
    //   AstCaseItem::user2()   // bool     Moved default caseitems
    //   AstNodeFTaskRef::user2()  // bool  Processing - to check for recursion
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    // Below state needs to be preserved between each module call.
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstClass* m_classp = خالی_اشاریہ;  // Class we're inside
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Function or task we're inside
    صحیح m_senitem_Cvt_عدد = 0;  // Temporary signal counter
    std::deque<AstGenFor*> m_underGenFors;  // Stack of GenFor underneath
    منطقی m_underGenerate = جھوٹ;  // Under GenFor/GenIf

    // VISITORS
    // TODO: Most of these visitors are here for historical reasons.
    // TODO: ExpectDescriptor can move to data type resolution, and the rest
    // TODO: could move to V3LinkParse to get them out of the way of elaboration
    // TODO: Some also can move to V3LinkWidth, to happen once post-LinkDot
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        // Module: Create sim table for entire module and iterate
        UINFO(8, "MODULE " << nodep);
        اگر (nodep->dead()) واپس;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_senitem_Cvt_عدد);
        m_modp = nodep;
        m_senitem_Cvt_عدد = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isParam()) varp->varType(VVarType::MEMBER);
            }
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstInitialAutomatic* nodep) override {
        iterateChildren(nodep);
        // Initial assignments under function/tasks can just be simple
        // assignments without the initial
        اگر (m_ftaskp) {
            nodep->replaceWith(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(AstVar* nodep) override {
        iterateChildren(nodep);
        اگر (m_ftaskp) nodep->funcLocal(سچ);
        اگر (nodep->is_Sig_Mod_عوامی()) {
            nodep->sig_Mod_عوامی(جھوٹ);  // We're done with this attribute
            m_modp->mod_عوامی(سچ);  // Avoid flattening if signals are exposed
        }
    }

    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (nodep->varp()) {  // Else due to dead code, might not have var pointer
            // VarRef: Resolve its reference
            nodep->varp()->مستعمل_Param(سچ);
            // Look for where genvar is valid
            منطقی ok = جھوٹ;
            // cppcheck-suppress constVariablePointer
            برائے (AstGenFor* ثابت forp : m_underGenFors) {
                اگر (ok) توڑو;
                اگر (forp->initsp())
                    forp->initsp()->foreach([&](ثابت AstVarRef* refp) {  //
                        اگر (refp->varp() == nodep->varp()) ok = سچ;
                    });
            }
            اگر (nodep->varp()->isGenVar() && !ok) {
                nodep->v3error("Genvar "
                               << nodep->prettyNameQ()
                               << " used outside generate for loop (IEEE 1800-2023 27.4)");
            }
        }
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        // Note AstLet is handled specifically elsewhere
        // NodeTask: Remember its name for later resolution
        اگر (m_underGenerate) nodep->underGenerate(سچ);
        // Remember the existing symbol table scope
        اگر (m_classp) nodep->class_طریقہ(سچ);
        // V3LinkDot moved the isExternDef into the class, the extern proto was
        // checked to exist, and now isn't needed
        nodep->isExternDef(جھوٹ);
        اگر (nodep->isExternProto()) {
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;

        اگر (nodep->lifetime().isAutomatic() && nodep->fvarp()) {
            // Must clear automatic function output variable on function invocation
            AstVar* ثابت fvarp = VN_AS(nodep->fvarp(), Var);
            Ast_گرہ* ثابت crstp = نیا Ast_تفویض{
                fvarp->fileline(), نیا AstVarRef{fvarp->fileline(), fvarp, VAccess::WRITE},
                نیا AstCReset{fvarp->fileline(), fvarp, جھوٹ}};
            fvarp->noCReset(سچ);
            nodep->prependStmtsp(crstp);
        }
        iterateChildren(nodep);
        اگر (nodep->dpiExport()) nodep->scopeNamep(نیا AstScopeName{nodep->fileline(), جھوٹ});
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        اگر (AstLet* letp = VN_CAST(nodep->taskp(), Let)) {
            UINFO(7, "letSubstitute() " << nodep << " <- " << letp);
            اگر (letp->user2()) {
                nodep->v3error("Recursive let substitution " << letp->prettyNameQ());
                nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس;
            }
            letp->user2(سچ);
            اگر (VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                nodep->v3error("Expected statement, not let substitution " << letp->prettyNameQ());
            }
            // UINFOTREE(9, letp, "", "let-let");
            // UINFOTREE(9, nodep, "", "let-ref");
            // cppcheck-suppress constVariablePointer
            Ast_Stmt_اظہاریہ* ثابت letStmtp = VN_AS(letp->stmtsp(), Stmt_اظہاریہ);
            Ast_گرہ_اظہاریہ* ثابت newp = letStmtp->exprp()->clone_درخت(جھوٹ);
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, letp->stmtsp());
            std::map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_مقصد_Exprs;
            برائے (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.پہلا;
                ثابت AstArg* ثابت argp = tconnect.دوسرا;
                Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp();
                اگر (!pinp) جاری;  // Argument error we'll find later
                پورٹ_مقصد_Exprs.emplace(portp, pinp);
            }
            // Replace VarRefs of arguments with the argument values
            newp->foreach([&](AstVarRef* refp) {  //
                ثابت خودکار it = پورٹ_مقصد_Exprs.ڈھونڈو(refp->varp());
                اگر (it != پورٹ_مقصد_Exprs.انتہا()) {
                    Ast_گرہ_اظہاریہ* ثابت pinp = it->دوسرا;
                    UINFO(9, "let pin subst " << refp << " <- " << pinp);
                    // Side effects are copied into pins, to match other simulators
                    refp->replaceWith(pinp->clone_درخت(جھوٹ));
                    VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                }
            });
            // UINFOTREE(9, newp, "", "let-new");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            // Iterate to expand further now, so we can look for recursions
            دورہ(newp);
            letp->user2(جھوٹ);
            واپس;
        }
    }

    خالی دورہ(AstLet* nodep) override {
        // Lets have been (or about to be) substituted, we can remove
        nodep->unlink_Fr_عقب();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstStmtPragma* nodep) override {
        اگر (nodep->pragp()->pragType() == VPragmaType::COVERAGE_بلاک_OFF) {
            // Strip pragma if not needed, may optimize better without
            اگر (!v3Global.opt.coverage_سطر() && !v3Global.opt.coverageBranch()) {
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
        }
        iterateChildren(nodep);
    }

    خالی دورہ(AstPragma* nodep) override {
        اگر (nodep->pragType() == VPragmaType::HIER_بلاک) {
            UASSERT_OBJ(m_modp, nodep, "HIER_BLOCK not under a module");
            // If this is hierarchical mode which is to lib-create,
            // sub modules do not have hier_block meta comment in the source code.
            // But .vlt files may still mark a module which is actually a lib-create wrapper
            // hier_block. AstNodeModule::hierBlock() can be true only when --hierarchical is
            // specified.
            m_modp->hier_بلاک(v3Global.opt.درجہ_وار());
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ اگر (nodep->pragType() == VPragmaType::عوامی_MODULE) {
            UASSERT_OBJ(m_modp, nodep, "PUBLIC_MODULE not under a module");
            m_modp->mod_عوامی(سچ);
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ اگر (nodep->pragType() == VPragmaType::عوامی_TASK) {
            UASSERT_OBJ(m_ftaskp, nodep, "PUBLIC_TASK not under a task");
            m_ftaskp->task_عوامی(سچ);
            m_modp->mod_عوامی(سچ);  // Need to get to the task...
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ اگر (nodep->pragType() == VPragmaType::VERILATOR_LIB) {
            UASSERT_OBJ(m_modp, nodep, "VERILATOR_LIB not under a module");
            m_modp->verilatorLib(سچ);
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ {
            iterateChildren(nodep);
        }
    }

    string expectFormat(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* argp, منطقی is_جائزہ) {
        // Check display arguments, return new format string
        string newFormat;
        منطقی اندر_Pct = جھوٹ;
        منطقی اندر_Ignore = جھوٹ;
        string fmt;
        string parseFormat = format;
        جب (!parseFormat.خالی_خانے() || argp) {
            برائے (ثابت حرف ch : parseFormat) {
                اگر (!اندر_Pct && ch == '%') {
                    اندر_Pct = سچ;
                    اندر_Ignore = جھوٹ;
                    fmt = ch;
                } ورنہ اگر (اندر_Pct && (std::ہندسہ_ہے(ch) || ch == '.' || ch == '-')) {
                    fmt += ch;
                } ورنہ اگر (اندر_Pct) {
                    اندر_Pct = جھوٹ;
                    fmt += ch;
                    انتخاب (std::tolower(ch)) {
                    حالت '%':  // %% - just output a %
                        توڑو;
                    حالت '*':
                        اندر_Pct = سچ;
                        اندر_Ignore = سچ;
                        توڑو;
                    حالت 'm':  // %m - auto insert "name"
                        اگر (is_جائزہ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %m in $fscanf");
                            fmt = "";
                        }
                        توڑو;
                    حالت 'l':  // %l - auto insert "library"
                        اگر (is_جائزہ) {
                            nodep->v3warn(E_UNSUPPORTED, "Unsupported: %l in $fscanf");
                            fmt = "";
                        }
                        اگر (m_modp)
                            fmt = Ast_گرہ::prettyName(m_modp->libname()) + "."
                                  + m_modp->prettyName();
                        توڑو;
                    طےشدہ:  // Most operators, just move to next argument
                        اگر (!V3_نمبر::displayedFmtHasArg(ch, is_جائزہ)) {
                            nodep->v3error("Unknown $display-like format code: '%" << ch << "'");
                        } ورنہ اگر (!اندر_Ignore) {
                            اگر (!argp) {
                                nodep->v3error("Missing arguments for $display-like format");
                            } ورنہ {
                                argp = argp->nextp();
                            }
                        }
                        توڑو;
                    }  // switch
                    newFormat += fmt;
                } ورنہ {
                    newFormat += ch;
                }
            }

            // Find additional arguments (without format) or additional format strings
            parseFormat = "";

            اگر (is_جائزہ) توڑو;
            جب (argp) {
                ثابت AstConst* ثابت constp = VN_CAST(argp, Const);
                ثابت منطقی isFromString = (constp) ? constp->عدد().isFromString() : جھوٹ;
                اگر (!isFromString) {
                    newFormat.append("%?");  // V3Width to figure it out
                    argp = argp->nextp();
                } ورنہ {  // New format string
                    parseFormat += constp->عدد().مقصد_String();
                    Ast_گرہ* ثابت nextp = argp->nextp();
                    argp->unlink_Fr_عقب();
                    VL_DO_DANGLING(ڈالو_Deletep(argp), argp);
                    argp = nextp;
                    توڑو;  // And continue at top of parsing the new parseFormat
                }
            }
        }
        واپس newFormat;
    }

    ساکن خالی expectDescriptor(Ast_گرہ* /*nodep*/, ثابت Ast_گرہ_Var_Ref* filep) {
        // This might fail on complex expressions like arrays
        // We use attrFileDescr() only for lint suppression, so that's ok
        اگر (filep && filep->varp()) filep->varp()->attrFileDescr(سچ);
    }

    خالی دورہ(AstFClose* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    خالی دورہ(AstFError* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    خالی دورہ(AstFEof* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    خالی دورہ(AstFRead* nodep) override {
        iterateChildren(nodep);
        expectDescriptor(nodep, VN_CAST(nodep->filep(), گرہ_Var_Ref));
    }
    خالی دورہ(AstFScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), سچ);
    }
    خالی دورہ(AstSScanF* nodep) override {
        iterateChildren(nodep);
        expectFormat(nodep, nodep->متن(), nodep->exprsp(), سچ);
    }
    خالی دورہ(AstSFormatF* nodep) override {
        اگر (nodep->user2_سیٹ_Once()) واپس;
        iterateChildren(nodep);
        // Cleanup old-school displays without format arguments
        // Similar code in V3Const::visit(AstSFormatF)
        اگر (nodep->اظہاریہ_Format_2()) {
            اگر (VN_IS(nodep->exprsp(), Const)
                && VN_AS(nodep->exprsp(), Const)->عدد().isFromString()) {
                AstConst* ثابت fmtp = VN_AS(nodep->exprsp()->unlink_Fr_عقب(), Const);
                nodep->متن(fmtp->عدد().مقصد_String());
                nodep->اظہاریہ_Format_2(جھوٹ);
                VL_DO_DANGLING(ڈالو_Deletep(fmtp), fmtp);
            }
        }
        اگر (nodep->optionalFormat()) {  // e.g. $display, $swrite; _not_ $sformat/$sformatf
            nodep->optionalFormat(جھوٹ);
            nodep->اظہاریہ_Format_2(جھوٹ);
        }
        اگر ((VN_IS(nodep->backp(), Display)
             && VN_AS(nodep->backp(), Display)->displayType().needScopeTracking())
            || nodep->formatScopeTracking()) {
            nodep->scopeNamep(نیا AstScopeName{nodep->fileline(), سچ});
        }
        اگر (!nodep->اظہاریہ_Format_2()) {
            ثابت string newFormat = expectFormat(nodep, nodep->متن(), nodep->exprsp(), جھوٹ);
            nodep->متن(newFormat);
        }
    }

    خالی دورہ(Ast_Udp_جدول* nodep) override {
        UINFO(5, "UDPTABLE  " << nodep);
        اگر (!v3Global.opt.bboxUnsup()) {
            // We don't warn until V3Inst, so that UDPs that are in libraries and
            // never used won't result in any warnings.
        } ورنہ {
            // Massive hack, just tie off all outputs so our analysis can proceed
            ثابت AstVar* varoutp = خالی_اشاریہ;
            برائے (Ast_گرہ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    اگر (varp->isReadOnly()) {
                    } ورنہ اگر (varp->isWritable()) {
                        اگر (varoutp) {
                            varp->v3error("Multiple outputs not allowed in udp modules");
                        }
                        varoutp = varp;
                        // Tie off
                        Ast_تفویض_W* ثابت ap
                            = نیا Ast_تفویض_W{varp->fileline(),
                                             نیا AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                                             نیا AstConst{varp->fileline(), AstConst::ثنائی_ہندسہ_False{}}};
                        m_modp->جوڑو_Stmtsp(نیا AstAlways{ap});
                    } ورنہ {
                        varp->v3error("Only inputs and outputs are allowed in udp modules");
                    }
                }
            }
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    خالی دورہ(AstSystemCSection* nodep) override {
        انتخاب (nodep->sectionType()) {
        // Constructor, desctructor or special class info means the module must remain public
        حالت VSystemCSectionType::CTOR:
        حالت VSystemCSectionType::DTOR:
        حالت VSystemCSectionType::INT: m_modp->mod_عوامی(سچ); توڑو;
        طےشدہ: توڑو;
        }
        // Has no children
    }

    خالی دورہ(AstIfaceRefDType* nodep) override {
        // LinkDot checked modports, now remove references to them
        // Keeping them later caused problems with InstDeArray,
        // as it needed to make new modport arrays and such
        nodep->modportp(خالی_اشاریہ);
        iterateChildren(nodep);
    }
    //  void visit(AstModport* nodep) override { ... }
    // We keep Modport's themselves around for JSON dump purposes

    خالی دورہ(AstGenFor* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = سچ;
        m_underGenFors.emplace_عقب(nodep);
        iterateChildren(nodep);
        UASSERT_OBJ(!m_underGenFors.خالی_خانے(), nodep, "Underflow");
        m_underGenFors.نکالو_عقب();
    }
    خالی دورہ(AstGenIf* nodep) override {
        VL_RESTORER(m_underGenerate);
        m_underGenerate = سچ;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح LinkResolveVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkResolveVisitor() override = طےشدہ;
};

//######################################################################
// LinkBotupVisitor
//      Recurses cells backwards, so we can pick up those things that propagate
//      from child cells up to the top module.

جماعت LinkBotupVisitor final : عوامی VNVisitorConst {
    // STATE
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.
        iterateChildrenBackwardsConst(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCell* nodep) override {
        // Parent module inherits child's publicity
        اگر (nodep->modp()->mod_عوامی()) m_modp->mod_عوامی(سچ);
        //** No iteration for speed
    }
    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح LinkBotupVisitor(AstNetlist* rootp) { iterateConst(rootp); }
    ~LinkBotupVisitor() override = طےشدہ;
};

//######################################################################
// V3LinkResolve class functions

خالی V3LinkResolve::linkResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    {
        ثابت LinkResolveVisitor visitor{rootp};
        LinkBotupVisitor{rootp};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkresolve", 0, dump_درخت_Either_Level() >= 6);
}
