// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitCBase.h"

#అన్తర్భావ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    స్థిరరాశి స్వయంచాలక సమితి_All = [](Ast_కణుపు_Module* modp) -> శూన్య {
        కొరకు (Ast_కణుపు* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ఒకవేళ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, కణుపు_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_కణుపు_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    కొరకు (Ast_కణుపు* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        సమితి_All(VN_AS(modp, కణుపు_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::ఉపసర్గ_Name_Protect(స్థిరరాశి Ast_కణుపు* nodep) VL_MT_STABLE {
    స్థిరరాశి string ఉపసర్గ = v3Global.opt.mod_ఉపసర్గ() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    స్థిర V3Mutex s_mutex;
    స్థిరరాశి V3_తాళం_Guard తాళం_2{s_mutex};  // Otherwise map access is unsafe
    స్థిర std::map<std::string, std::string> s_memoized;
    స్థిర std::map<std::string, std::string> s_uc_గమ్యం_ఉపసర్గ;
    // Memoize results
    స్థిరరాశి స్వయంచాలక mit = s_memoized.కనుగొను(ఉపసర్గ);
    ఒకవేళ (mit != s_memoized.చివర()) తిరిగి mit->రెండవ;
    //
    // Check capitalization
    స్థిరరాశి string ఉపసర్గ_పై = VString::upcase(ఉపసర్గ);
    string ఫలితం;
    {
        స్థిరరాశి స్వయంచాలక it = s_uc_గమ్యం_ఉపసర్గ.కనుగొను(ఉపసర్గ_పై);
        ఒకవేళ (it == s_uc_గమ్యం_ఉపసర్గ.చివర()) {
            s_uc_గమ్యం_ఉపసర్గ.emplace(ఉపసర్గ_పై, ఉపసర్గ);
            ఫలితం = ఉపసర్గ;
        } లేకపోతే ఒకవేళ (it->రెండవ == ఉపసర్గ) {
            ఫలితం = ఉపసర్గ;  // Same capitialization as last time
        } లేకపోతే {
            VHashSha512 hash{ఉపసర్గ};
            ఫలితం = ఉపసర్గ + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(ఉపసర్గ, ఫలితం);
    తిరిగి ఫలితం;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(స్థిరరాశి AstCFunc* nodep, స్థిరరాశి Ast_కణుపు_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    ఒకవేళ (nodep->isConstructor()) {
        name += "init";
    } లేకపోతే ఒకవేళ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::ఉపసర్గ_Name_Protect(modp);
    } లేకపోతే {
        ఒకవేళ (nodep->isLoose()) {
            name += EmitCUtil::ఉపసర్గ_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    తిరిగి name;
}

string EmitCBaseVisitorConst::c_Func_వాదనలు(స్థిరరాశి AstCFunc* nodep, తార్కిక cpp) {
    // Return argument list for given C function
    string వాదనలు;
    ఒకవేళ (nodep->isLoose() && !nodep->isStatic()) {
        ఒకవేళ (nodep->voidSelf()) {
            వాదనలు += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } లేకపోతే {
            ఒకవేళ (nodep->isConst().true_తెలిసిన()) వాదనలు += "const ";
            వాదనలు += EmitCUtil::ఉపసర్గ_Name_Protect(EmitCParentModule::get(nodep));
            వాదనలు += "* vlSelf";
        }
    }
    ఒకవేళ (nodep->needProcess()) {
        ఒకవేళ (!వాదనలు.ఖాళీలు()) వాదనలు += ", ";
        వాదనలు += "VlProcessRef vlProcess";
    }
    ఒకవేళ (!nodep->argTypes().ఖాళీలు()) {
        ఒకవేళ (!వాదనలు.ఖాళీలు()) వాదనలు += ", ";
        వాదనలు += nodep->argTypes();
    }
    // Might be a user function with argument list.
    కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
            ఒకవేళ (portp->isIO() && !portp->isFuncReturn()) {
                ఒకవేళ (!వాదనలు.ఖాళీలు()) వాదనలు += ", ";
                ఒకవేళ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    వాదనలు += portp->dpiArgType(నిజం, అబద్ధం, cpp);
                } లేకపోతే ఒకవేళ (nodep->func_ప్రజా()) {
                    వాదనలు += portp->cPubArgType(నిజం, అబద్ధం);
                } లేకపోతే {
                    వాదనలు += portp->vlArgType(నిజం, అబద్ధం, నిజం);
                }
            }
        }
    }
    తిరిగి వాదనలు;
}

శూన్య EmitCBaseVisitorConst::emitCDefaultConstructor(స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp) {
    puts(EmitCUtil::ఉపసర్గ_Name_Protect(modp));
    puts("() = default;\n");
}

శూన్య EmitCBaseVisitorConst::emit_CFunc_శీర్షిక(స్థిరరాశి AstCFunc* funcp, స్థిరరాశి Ast_కణుపు_Module* modp,
                                            తార్కిక withScope) {
    ఒకవేళ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    ఒకవేళ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        తిరిగి;
    }
    ఒకవేళ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    ఒకవేళ (withScope) {
        ఒకవేళ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::శిఖరం_Class_Name() + "::");
        } లేకపోతే ఒకవేళ (funcp->is_Proper_పద్ధతి()) {
            putns(funcp, EmitCUtil::ఉపసర్గ_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_వాదనలు(funcp) + ")");
    ఒకవేళ (funcp->isConst().true_తెలిసిన() && funcp->is_Proper_పద్ధతి()) puts(" const");
}

శూన్య EmitCBaseVisitorConst::emitCFuncDecl(స్థిరరాశి AstCFunc* funcp, స్థిరరాశి Ast_కణుపు_Module* modp,
                                          తార్కిక cLinkage) {
    ensure_New_పంక్తి();
    ఒకవేళ (!funcp->ifdef().ఖాళీలు()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    ఒకవేళ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    ఒకవేళ (cLinkage) putns(funcp, "extern \"C\" ");
    ఒకవేళ (funcp->isStatic() && funcp->is_Proper_పద్ధతి()) putns(funcp, "static ");
    ఒకవేళ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_పద్ధతి(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_శీర్షిక(funcp, modp, /* withScope: */ అబద్ధం);
    స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(modp, Class);
    ఒకవేళ (funcp->ఖాళీలు_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } లేకపోతే {
        putns(funcp, ";\n");
    }
    ఒకవేళ (!funcp->ifdef().ఖాళీలు()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_విభజించు_పాఠ్యం_Stmts(పాఠ్యం_Stmts& stmts, స్థిరరాశి std::string& ఉపసర్గ,
                                                      స్థిరరాశి std::string& params,
                                                      స్థిరరాశి std::string& వాదనలు,  //
                                                      కొలత_t max_ఖర్చు, తార్కిక support,
                                                      స్థిరరాశి అక్షరం* descriptionp,
                                                      స్థిరరాశి std::function<శూన్య()>& emit_శీర్షిక) {
    స్థిరరాశి స్వయంచాలక subFuncName = [&](కొలత_t n) { తిరిగి ఉపసర్గ + "__" + std::గమ్యం_string(n); };
    కొలత_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    తార్కిక callsSubFuncs = అబద్ధం;
    కొలత_t callee_పునాది = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    అయితే (stmts.కొలత() > 1) {
        స్థిరరాశి కొలత_t level_పునాది = nSubFunctions;
        పాఠ్యం_Stmts calls;  // Calls to the sub-functions of this level
        స్థిరరాశి కొలత_t nStmts = stmts.కొలత();
        కొరకు (కొలత_t విభజించు_ప్రారంభం = 0, విభజించు_చివర = 0; విభజించు_ప్రారంభం < nStmts; విభజించు_ప్రారంభం = విభజించు_చివర) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            కొలత_t ఖర్చు = 0;
            అయితే (((ఖర్చు < max_ఖర్చు) || (విభజించు_చివర - విభజించు_ప్రారంభం < 2)) && విభజించు_చివర < nStmts) {
                ఖర్చు += stmts[విభజించు_చివర++].m_ఖర్చు;
            }
            UASSERT(విభజించు_ప్రారంభం < విభజించు_చివర, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_ఆధారం_File(funcName, నిజం, support, descriptionp);
            // Emit header
            emit_శీర్షిక();
            // Declare called sub-functions
            ఒకవేళ (callsSubFuncs) {
                puts("\n");
                కొరకు (కొలత_t j = విభజించు_ప్రారంభం; j < విభజించు_చివర; ++j) {
                    puts("void " + subFuncName(callee_పునాది + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            కొరకు (కొలత_t j = విభజించు_ప్రారంభం; j < విభజించు_చివర; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_పాఠ్యం);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.కూడు(funcName + "(" + వాదనలు + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = నిజం;
        callee_పునాది = level_పునాది;
    }
    తిరిగి funcName.ఖాళీలు() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_విభజించు_Ctor_Dtor(
    పాఠ్యం_Stmts& ctorStmts, పాఠ్యం_Stmts& dtorStmts, కొలత_t పునాది_ఖర్చు, స్థిరరాశి std::string& ఉపసర్గ,
    స్థిరరాశి std::string& params, స్థిరరాశి std::string& వాదనలు, తార్కిక support, స్థిరరాశి అక్షరం* descriptionp,
    స్థిరరాశి std::function<శూన్య()>& emit_శీర్షిక) {
    స్థిరరాశి కొలత_t max_ఖర్చు = static_cast<కొలత_t>(v3Global.opt.output_విభజించు_CFuncs());
    ఒకవేళ (!max_ఖర్చు) తిరిగి {};
    కొలత_t మొత్తం_ఖర్చు = పునాది_ఖర్చు;
    కొరకు (స్థిరరాశి పాఠ్యం_Stmts::Stmt& stmt : ctorStmts) మొత్తం_ఖర్చు += stmt.m_ఖర్చు;
    కొరకు (స్థిరరాశి పాఠ్యం_Stmts::Stmt& stmt : dtorStmts) మొత్తం_ఖర్చు += stmt.m_ఖర్చు;
    ఒకవేళ (మొత్తం_ఖర్చు <= max_ఖర్చు) తిరిగి {};
    v3Global.useParallelBuild(నిజం);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_విభజించు_పాఠ్యం_Stmts(ctorStmts, ఉపసర్గ + "__ctor", params, వాదనలు, max_ఖర్చు,
                                              support, descriptionp, emit_శీర్షిక);
    std::string dtorDecl = emit_విభజించు_పాఠ్యం_Stmts(dtorStmts, ఉపసర్గ + "__dtor", params, వాదనలు, max_ఖర్చు,
                                              support, descriptionp, emit_శీర్షిక);
    తిరిగి {std::move(ctorDecl), std::move(dtorDecl)};
}

శూన్య EmitCBaseVisitorConst::emitVarDecl(స్థిరరాశి AstVar* nodep, తార్కిక asRef) {
    స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = nodep->basicp();
    స్థిరరాశి తార్కిక refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    ఒకవేళ (nodep->mtask_Cache_పంక్తి_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    ఒకవేళ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", అబద్ధం, అబద్ధం));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        తిరిగి;
    }

    స్థిరరాశి స్వయంచాలక emitDeclArrayBrackets = [ఇది](స్థిరరాశి AstVar* nodep) -> శూన్య {
        // This isn't very robust and may need cleanup for other data types
        కొరకు (స్థిరరాశి AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvt_గమ్యం_Str(arrayp->elementsConst()) + "]");
        }
    };

    ఒకవేళ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        ఒకవేళ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } లేకపోతే ఒకవేళ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } లేకపోతే ఒకవేళ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } లేకపోతే {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        ఒకవేళ (asRef) {
            ఒకవేళ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        ఒకవేళ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } లేకపోతే ఒకవేళ (nodep->isVLIO()) {
        ఒకవేళ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } లేకపోతే ఒకవేళ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } లేకపోతే ఒకవేళ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } లేకపోతే {
            nodep->v3fatalSrc("Unknown type");
        }

        ఒకవేళ (nodep->isQuad()) {
            puts("64");
        } లేకపోతే ఒకవేళ (nodep->వెడల్పు_Min() <= 8) {
            puts("8");
        } లేకపోతే ఒకవేళ (nodep->వెడల్పు_Min() <= 16) {
            puts("16");
        } లేకపోతే ఒకవేళ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        ఒకవేళ (asRef) {
            ఒకవేళ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        ఒకవేళ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvt_గమ్యం_Str(basicp->lo() + nodep->వెడల్పు() - 1) + "," + cvt_గమ్యం_Str(basicp->lo()));
        ఒకవేళ (nodep->isWide()) puts("," + cvt_గమ్యం_Str(nodep->వెడల్పు_పదాలు()));
        puts(");\n");
    } లేకపోతే {
        // Strings and other fundamental C types
        ఒకవేళ (nodep->isFuncLocal() && nodep->dtypep()->కలిగియుంది_String()) {
            స్థిరరాశి string name = nodep->name();
            స్థిరరాశి string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            స్థిరరాశి తార్కిక beStatic = name.కొలత() >= suffix.కొలత()
                                  && name.substr(name.కొలత() - suffix.కొలత()) == suffix;
            ఒకవేళ (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(నిజం, అబద్ధం, అబద్ధం, asRef));
        puts(";\n");
    }
}

శూన్య EmitCBaseVisitorConst::emitVarAccessors(స్థిరరాశి AstVar* nodep) {
    ప్రతిజ్ఞా(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    స్థిరరాశి string వ్యక్తిగత_Name = nodep->name();
    స్థిరరాశి string ప్రజా_Name = nodep->name().substr(అక్షరశ్రేణి_పొడవు("__Vm_sig_"));

    puts("decltype("s + వ్యక్తిగత_Name + ") "s + ప్రజా_Name + "() {return "s + వ్యక్తిగత_Name + ";}\n");
    puts("void "s + ప్రజా_Name + "(decltype(" + వ్యక్తిగత_Name + ") v) {"s + వ్యక్తిగత_Name + "=v;}\n");
}

శూన్య EmitCBaseVisitorConst::emitModCUse(స్థిరరాశి Ast_కణుపు_Module* modp, VUseType useType) {
    తార్కిక nl = అబద్ధం;
    forModCUse(modp, useType, [&](స్థిరరాశి string& నమోదు_2) {
        puts(నమోదు_2);
        nl = నిజం;
    });
    ఒకవేళ (nl) puts("\n");
}

std::pair<string, File_పంక్తి*> EmitCBaseVisitorConst::scSection(స్థిరరాశి Ast_కణుపు_Module* modp,
                                                              VSystemCSectionType type) {
    string పాఠ్యం;
    File_పంక్తి* fl = శూన్యసూచిక;
    పూర్ణాంకం చివరి_పంక్తి = -999;
    కొరకు (స్థిరరాశి AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        ఒకవేళ (scSectionp->sectionType() != type) కొనసాగు;
        ఒకవేళ (పాఠ్యం.ఖాళీలు()) {
            fl = scSectionp->fileline();
            పాఠ్యం += "\n";
            ఒకవేళ (v3Global.opt.decoration()) {
                పాఠ్యం += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        ఒకవేళ (చివరి_పంక్తి + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            పాఠ్యం += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        చివరి_పంక్తి = scSectionp->fileline()->lineno();
        పాఠ్యం += scSectionp->పాఠ్యం();
    }
    ఒకవేళ (!పాఠ్యం.ఖాళీలు()) {
        ఒకవేళ (v3Global.opt.decoration()) పాఠ్యం += "//*** Above code from `systemc in Verilog file\n";
        పాఠ్యం += "\n";
        // Substitute `systemc_class_name
        string::కొలత_type pos;
        అయితే ((pos = పాఠ్యం.కనుగొను("`systemc_class_name")) != string::npos) {
            పాఠ్యం.replace(pos, std::అక్షరశ్రేణి_పొడవు("`systemc_class_name"),
                         EmitCUtil::ఉపసర్గ_Name_Protect(modp));
        }
    }
    తిరిగి std::make_pair(పాఠ్యం, fl);
}

శూన్య EmitCBaseVisitorConst::emitSystemCSection(స్థిరరాశి Ast_కణుపు_Module* modp,
                                               VSystemCSectionType type) {
    స్వయంచాలక పాఠ్యం_And_Fileline = scSection(modp, type);
    ఒకవేళ (!పాఠ్యం_And_Fileline.మొదటి.ఖాళీలు()) ofp()->putsNoTracking(పాఠ్యం_And_Fileline.మొదటి);
}
