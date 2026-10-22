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

#شامل "V3PchAstMT.h"

#شامل "V3EmitCBase.h"

#شامل "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ثابت خودکار سیٹ_All = [](Ast_گرہ_Module* modp) -> خالی {
        برائے (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, گرہ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_گرہ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    برائے (Ast_گرہ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        سیٹ_All(VN_AS(modp, گرہ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::سابقہ_Name_Protect(ثابت Ast_گرہ* nodep) VL_MT_STABLE {
    ثابت string سابقہ = v3Global.opt.mod_سابقہ() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ساکن V3Mutex s_mutex;
    ثابت V3_تالا_Guard تالا_2{s_mutex};  // Otherwise map access is unsafe
    ساکن std::map<std::string, std::string> s_memoized;
    ساکن std::map<std::string, std::string> s_uc_مقصد_سابقہ;
    // Memoize results
    ثابت خودکار mit = s_memoized.ڈھونڈو(سابقہ);
    اگر (mit != s_memoized.انتہا()) واپس mit->دوسرا;
    //
    // Check capitalization
    ثابت string سابقہ_بالائی = VString::upcase(سابقہ);
    string نتیجہ;
    {
        ثابت خودکار it = s_uc_مقصد_سابقہ.ڈھونڈو(سابقہ_بالائی);
        اگر (it == s_uc_مقصد_سابقہ.انتہا()) {
            s_uc_مقصد_سابقہ.emplace(سابقہ_بالائی, سابقہ);
            نتیجہ = سابقہ;
        } ورنہ اگر (it->دوسرا == سابقہ) {
            نتیجہ = سابقہ;  // Same capitialization as last time
        } ورنہ {
            VHashSha512 hash{سابقہ};
            نتیجہ = سابقہ + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(سابقہ, نتیجہ);
    واپس نتیجہ;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گرہ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    اگر (nodep->isConstructor()) {
        name += "init";
    } ورنہ اگر (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::سابقہ_Name_Protect(modp);
    } ورنہ {
        اگر (nodep->isLoose()) {
            name += EmitCUtil::سابقہ_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    واپس name;
}

string EmitCBaseVisitorConst::c_Func_دلائل(ثابت AstCFunc* nodep, منطقی cpp) {
    // Return argument list for given C function
    string دلائل;
    اگر (nodep->isLoose() && !nodep->isStatic()) {
        اگر (nodep->voidSelf()) {
            دلائل += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } ورنہ {
            اگر (nodep->isConst().true_معلوم()) دلائل += "const ";
            دلائل += EmitCUtil::سابقہ_Name_Protect(EmitCParentModule::get(nodep));
            دلائل += "* vlSelf";
        }
    }
    اگر (nodep->needProcess()) {
        اگر (!دلائل.خالی_خانے()) دلائل += ", ";
        دلائل += "VlProcessRef vlProcess";
    }
    اگر (!nodep->argTypes().خالی_خانے()) {
        اگر (!دلائل.خالی_خانے()) دلائل += ", ";
        دلائل += nodep->argTypes();
    }
    // Might be a user function with argument list.
    برائے (ثابت Ast_گرہ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        اگر (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            اگر (portp->isIO() && !portp->isFuncReturn()) {
                اگر (!دلائل.خالی_خانے()) دلائل += ", ";
                اگر (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    دلائل += portp->dpiArgType(سچ, جھوٹ, cpp);
                } ورنہ اگر (nodep->func_عوامی()) {
                    دلائل += portp->cPubArgType(سچ, جھوٹ);
                } ورنہ {
                    دلائل += portp->vlArgType(سچ, جھوٹ, سچ);
                }
            }
        }
    }
    واپس دلائل;
}

خالی EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت Ast_گرہ_Module* ثابت modp) {
    puts(EmitCUtil::سابقہ_Name_Protect(modp));
    puts("() = default;\n");
}

خالی EmitCBaseVisitorConst::emit_CFunc_سرخی(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp,
                                            منطقی withScope) {
    اگر (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    اگر (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        واپس;
    }
    اگر (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    اگر (withScope) {
        اگر (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::چوٹی_Class_Name() + "::");
        } ورنہ اگر (funcp->is_Proper_طریقہ()) {
            putns(funcp, EmitCUtil::سابقہ_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_دلائل(funcp) + ")");
    اگر (funcp->isConst().true_معلوم() && funcp->is_Proper_طریقہ()) puts(" const");
}

خالی EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp,
                                          منطقی cLinkage) {
    ensure_New_سطر();
    اگر (!funcp->ifdef().خالی_خانے()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    اگر (funcp->isConstructor()) emitCDefaultConstructor(modp);
    اگر (cLinkage) putns(funcp, "extern \"C\" ");
    اگر (funcp->isStatic() && funcp->is_Proper_طریقہ()) putns(funcp, "static ");
    اگر (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_طریقہ(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_سرخی(funcp, modp, /* withScope: */ جھوٹ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    اگر (funcp->خالی_خانے_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } ورنہ {
        putns(funcp, ";\n");
    }
    اگر (!funcp->ifdef().خالی_خانے()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_بانٹو_متن_Stmts(متن_Stmts& stmts, ثابت std::string& سابقہ,
                                                      ثابت std::string& params,
                                                      ثابت std::string& دلائل,  //
                                                      سائز_t max_لاگت, منطقی support,
                                                      ثابت حرف* descriptionp,
                                                      ثابت std::function<خالی()>& emit_سرخی) {
    ثابت خودکار subFuncName = [&](سائز_t n) { واپس سابقہ + "__" + std::مقصد_string(n); };
    سائز_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    منطقی callsSubFuncs = جھوٹ;
    سائز_t callee_بنیاد = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    جب (stmts.سائز() > 1) {
        ثابت سائز_t level_بنیاد = nSubFunctions;
        متن_Stmts calls;  // Calls to the sub-functions of this level
        ثابت سائز_t nStmts = stmts.سائز();
        برائے (سائز_t بانٹو_آغاز = 0, بانٹو_انتہا = 0; بانٹو_آغاز < nStmts; بانٹو_آغاز = بانٹو_انتہا) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            سائز_t لاگت = 0;
            جب (((لاگت < max_لاگت) || (بانٹو_انتہا - بانٹو_آغاز < 2)) && بانٹو_انتہا < nStmts) {
                لاگت += stmts[بانٹو_انتہا++].m_لاگت;
            }
            UASSERT(بانٹو_آغاز < بانٹو_انتہا, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_ماخذ_File(funcName, سچ, support, descriptionp);
            // Emit header
            emit_سرخی();
            // Declare called sub-functions
            اگر (callsSubFuncs) {
                puts("\n");
                برائے (سائز_t j = بانٹو_آغاز; j < بانٹو_انتہا; ++j) {
                    puts("void " + subFuncName(callee_بنیاد + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            برائے (سائز_t j = بانٹو_آغاز; j < بانٹو_انتہا; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_متن);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.جوڑو(funcName + "(" + دلائل + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = سچ;
        callee_بنیاد = level_بنیاد;
    }
    واپس funcName.خالی_خانے() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_بانٹو_Ctor_Dtor(
    متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, سائز_t بنیاد_لاگت, ثابت std::string& سابقہ,
    ثابت std::string& params, ثابت std::string& دلائل, منطقی support, ثابت حرف* descriptionp,
    ثابت std::function<خالی()>& emit_سرخی) {
    ثابت سائز_t max_لاگت = static_cast<سائز_t>(v3Global.opt.output_بانٹو_CFuncs());
    اگر (!max_لاگت) واپس {};
    سائز_t کل_لاگت = بنیاد_لاگت;
    برائے (ثابت متن_Stmts::Stmt& stmt : ctorStmts) کل_لاگت += stmt.m_لاگت;
    برائے (ثابت متن_Stmts::Stmt& stmt : dtorStmts) کل_لاگت += stmt.m_لاگت;
    اگر (کل_لاگت <= max_لاگت) واپس {};
    v3Global.useParallelBuild(سچ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_بانٹو_متن_Stmts(ctorStmts, سابقہ + "__ctor", params, دلائل, max_لاگت,
                                              support, descriptionp, emit_سرخی);
    std::string dtorDecl = emit_بانٹو_متن_Stmts(dtorStmts, سابقہ + "__dtor", params, دلائل, max_لاگت,
                                              support, descriptionp, emit_سرخی);
    واپس {std::move(ctorDecl), std::move(dtorDecl)};
}

خالی EmitCBaseVisitorConst::emitVarDecl(ثابت AstVar* nodep, منطقی asRef) {
    ثابت AstBasicDType* ثابت basicp = nodep->basicp();
    ثابت منطقی refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    اگر (nodep->mtask_Cache_سطر_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    اگر (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", جھوٹ, جھوٹ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        واپس;
    }

    ثابت خودکار emitDeclArrayBrackets = [یہ](ثابت AstVar* nodep) -> خالی {
        // This isn't very robust and may need cleanup for other data types
        برائے (ثابت AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvt_مقصد_Str(arrayp->elementsConst()) + "]");
        }
    };

    اگر (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        اگر (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } ورنہ اگر (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } ورنہ اگر (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } ورنہ {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        اگر (asRef) {
            اگر (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        اگر (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } ورنہ اگر (nodep->isVLIO()) {
        اگر (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } ورنہ اگر (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } ورنہ اگر (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } ورنہ {
            nodep->v3fatalSrc("Unknown type");
        }

        اگر (nodep->isQuad()) {
            puts("64");
        } ورنہ اگر (nodep->چوڑائی_Min() <= 8) {
            puts("8");
        } ورنہ اگر (nodep->چوڑائی_Min() <= 16) {
            puts("16");
        } ورنہ اگر (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        اگر (asRef) {
            اگر (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        اگر (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvt_مقصد_Str(basicp->lo() + nodep->چوڑائی() - 1) + "," + cvt_مقصد_Str(basicp->lo()));
        اگر (nodep->isWide()) puts("," + cvt_مقصد_Str(nodep->چوڑائی_الفاظ()));
        puts(");\n");
    } ورنہ {
        // Strings and other fundamental C types
        اگر (nodep->isFuncLocal() && nodep->dtypep()->شامل_ہے_String()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقی beStatic = name.سائز() >= suffix.سائز()
                                  && name.substr(name.سائز() - suffix.سائز()) == suffix;
            اگر (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(سچ, جھوٹ, جھوٹ, asRef));
        puts(";\n");
    }
}

خالی EmitCBaseVisitorConst::emitVarAccessors(ثابت AstVar* nodep) {
    أكد(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ثابت string نجی_Name = nodep->name();
    ثابت string عوامی_Name = nodep->name().substr(لڑی_لمبائی("__Vm_sig_"));

    puts("decltype("s + نجی_Name + ") "s + عوامی_Name + "() {return "s + نجی_Name + ";}\n");
    puts("void "s + عوامی_Name + "(decltype(" + نجی_Name + ") v) {"s + نجی_Name + "=v;}\n");
}

خالی EmitCBaseVisitorConst::emitModCUse(ثابت Ast_گرہ_Module* modp, VUseType useType) {
    منطقی nl = جھوٹ;
    forModCUse(modp, useType, [&](ثابت string& اندراج_2) {
        puts(اندراج_2);
        nl = سچ;
    });
    اگر (nl) puts("\n");
}

std::pair<string, File_سطر*> EmitCBaseVisitorConst::scSection(ثابت Ast_گرہ_Module* modp,
                                                              VSystemCSectionType type) {
    string متن;
    File_سطر* fl = خالی_اشاریہ;
    صحیح آخری_سطر = -999;
    برائے (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        اگر (scSectionp->sectionType() != type) جاری;
        اگر (متن.خالی_خانے()) {
            fl = scSectionp->fileline();
            متن += "\n";
            اگر (v3Global.opt.decoration()) {
                متن += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        اگر (آخری_سطر + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            متن += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        آخری_سطر = scSectionp->fileline()->lineno();
        متن += scSectionp->متن();
    }
    اگر (!متن.خالی_خانے()) {
        اگر (v3Global.opt.decoration()) متن += "//*** Above code from `systemc in Verilog file\n";
        متن += "\n";
        // Substitute `systemc_class_name
        string::سائز_type pos;
        جب ((pos = متن.ڈھونڈو("`systemc_class_name")) != string::npos) {
            متن.replace(pos, std::لڑی_لمبائی("`systemc_class_name"),
                         EmitCUtil::سابقہ_Name_Protect(modp));
        }
    }
    واپس std::make_pair(متن, fl);
}

خالی EmitCBaseVisitorConst::emitSystemCSection(ثابت Ast_گرہ_Module* modp,
                                               VSystemCSectionType type) {
    خودکار متن_And_Fileline = scSection(modp, type);
    اگر (!متن_And_Fileline.پہلا.خالی_خانے()) ofp()->putsNoTracking(متن_And_Fileline.پہلا);
}
