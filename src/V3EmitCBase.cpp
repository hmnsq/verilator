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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitCBase.h"

#ضمن "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ثابت خودکار سیٹ_All = [](Ast_گرہ_Module* modp) -> فراغ {
        لكل (Ast_گرہ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, گرہ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_گرہ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    لكل (Ast_گرہ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        سیٹ_All(VN_AS(modp, گرہ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::اگیتر_Name_Protect(ثابت Ast_گرہ* nodep) VL_MT_STABLE {
    ثابت string اگیتر = v3Global.opt.mod_اگیتر() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ساكن V3Mutex s_mutex;
    ثابت V3_جندرا_Guard جندرا_2{s_mutex};  // Otherwise map access is unsafe
    ساكن std::map<std::string, std::string> s_memoized;
    ساكن std::map<std::string, std::string> s_uc_منزل_اگیتر;
    // Memoize results
    ثابت خودکار mit = s_memoized.لبھو(اگیتر);
    إذا (mit != s_memoized.سرا()) أرجع mit->دوجا;
    //
    // Check capitalization
    ثابت string اگیتر_اتلا = VString::upcase(اگیتر);
    string نتیجہ;
    {
        ثابت خودکار it = s_uc_منزل_اگیتر.لبھو(اگیتر_اتلا);
        إذا (it == s_uc_منزل_اگیتر.سرا()) {
            s_uc_منزل_اگیتر.emplace(اگیتر_اتلا, اگیتر);
            نتیجہ = اگیتر;
        } وإلا إذا (it->دوجا == اگیتر) {
            نتیجہ = اگیتر;  // Same capitialization as last time
        } وإلا {
            VHashSha512 hash{اگیتر};
            نتیجہ = اگیتر + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(اگیتر, نتیجہ);
    أرجع نتیجہ;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گرہ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    إذا (nodep->isConstructor()) {
        name += "init";
    } وإلا إذا (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::اگیتر_Name_Protect(modp);
    } وإلا {
        إذا (nodep->isLoose()) {
            name += EmitCUtil::اگیتر_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    أرجع name;
}

string EmitCBaseVisitorConst::c_Func_دلیلاں(ثابت AstCFunc* nodep, منطقي cpp) {
    // Return argument list for given C function
    string دلیلاں;
    إذا (nodep->isLoose() && !nodep->isStatic()) {
        إذا (nodep->voidSelf()) {
            دلیلاں += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } وإلا {
            إذا (nodep->isConst().true_معلوم()) دلیلاں += "const ";
            دلیلاں += EmitCUtil::اگیتر_Name_Protect(EmitCParentModule::get(nodep));
            دلیلاں += "* vlSelf";
        }
    }
    إذا (nodep->needProcess()) {
        إذا (!دلیلاں.خالی_تھاواں()) دلیلاں += ", ";
        دلیلاں += "VlProcessRef vlProcess";
    }
    إذا (!nodep->argTypes().خالی_تھاواں()) {
        إذا (!دلیلاں.خالی_تھاواں()) دلیلاں += ", ";
        دلیلاں += nodep->argTypes();
    }
    // Might be a user function with argument list.
    لكل (ثابت Ast_گرہ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO() && !portp->isFuncReturn()) {
                إذا (!دلیلاں.خالی_تھاواں()) دلیلاں += ", ";
                إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    دلیلاں += portp->dpiArgType(سچ, خطأ, cpp);
                } وإلا إذا (nodep->func_عوامی()) {
                    دلیلاں += portp->cPubArgType(سچ, خطأ);
                } وإلا {
                    دلیلاں += portp->vlArgType(سچ, خطأ, سچ);
                }
            }
        }
    }
    أرجع دلیلاں;
}

فراغ EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت Ast_گرہ_Module* ثابت modp) {
    puts(EmitCUtil::اگیتر_Name_Protect(modp));
    puts("() = default;\n");
}

فراغ EmitCBaseVisitorConst::emit_CFunc_سرخی(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp,
                                            منطقي withScope) {
    إذا (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    إذا (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        أرجع;
    }
    إذا (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    إذا (withScope) {
        إذا (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::چوٹی_Class_Name() + "::");
        } وإلا إذا (funcp->is_Proper_طریقہ()) {
            putns(funcp, EmitCUtil::اگیتر_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_دلیلاں(funcp) + ")");
    إذا (funcp->isConst().true_معلوم() && funcp->is_Proper_طریقہ()) puts(" const");
}

فراغ EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp,
                                          منطقي cLinkage) {
    ensure_New_سطر();
    إذا (!funcp->ifdef().خالی_تھاواں()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    إذا (funcp->isConstructor()) emitCDefaultConstructor(modp);
    إذا (cLinkage) putns(funcp, "extern \"C\" ");
    إذا (funcp->isStatic() && funcp->is_Proper_طریقہ()) putns(funcp, "static ");
    إذا (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_طریقہ(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_سرخی(funcp, modp, /* withScope: */ خطأ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    إذا (funcp->خالی_تھاواں_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } وإلا {
        putns(funcp, ";\n");
    }
    إذا (!funcp->ifdef().خالی_تھاواں()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_ونڈو_متن_Stmts(متن_Stmts& stmts, ثابت std::string& اگیتر,
                                                      ثابت std::string& params,
                                                      ثابت std::string& دلیلاں,  //
                                                      ناپ_t max_لاگت, منطقي support,
                                                      ثابت حرف* descriptionp,
                                                      ثابت std::function<فراغ()>& emit_سرخی) {
    ثابت خودکار subFuncName = [&](ناپ_t n) { أرجع اگیتر + "__" + std::منزل_string(n); };
    ناپ_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    منطقي callsSubFuncs = خطأ;
    ناپ_t callee_ادھار = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    طالما (stmts.ناپ() > 1) {
        ثابت ناپ_t level_ادھار = nSubFunctions;
        متن_Stmts calls;  // Calls to the sub-functions of this level
        ثابت ناپ_t nStmts = stmts.ناپ();
        لكل (ناپ_t ونڈو_شروع = 0, ونڈو_سرا = 0; ونڈو_شروع < nStmts; ونڈو_شروع = ونڈو_سرا) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            ناپ_t لاگت = 0;
            طالما (((لاگت < max_لاگت) || (ونڈو_سرا - ونڈو_شروع < 2)) && ونڈو_سرا < nStmts) {
                لاگت += stmts[ونڈو_سرا++].m_لاگت;
            }
            UASSERT(ونڈو_شروع < ونڈو_سرا, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_سروت_File(funcName, سچ, support, descriptionp);
            // Emit header
            emit_سرخی();
            // Declare called sub-functions
            إذا (callsSubFuncs) {
                puts("\n");
                لكل (ناپ_t j = ونڈو_شروع; j < ونڈو_سرا; ++j) {
                    puts("void " + subFuncName(callee_ادھار + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            لكل (ناپ_t j = ونڈو_شروع; j < ونڈو_سرا; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_متن);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.جوڑو(funcName + "(" + دلیلاں + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = سچ;
        callee_ادھار = level_ادھار;
    }
    أرجع funcName.خالی_تھاواں() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_ونڈو_Ctor_Dtor(
    متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, ناپ_t ادھار_لاگت, ثابت std::string& اگیتر,
    ثابت std::string& params, ثابت std::string& دلیلاں, منطقي support, ثابت حرف* descriptionp,
    ثابت std::function<فراغ()>& emit_سرخی) {
    ثابت ناپ_t max_لاگت = static_cast<ناپ_t>(v3Global.opt.output_ونڈو_CFuncs());
    إذا (!max_لاگت) أرجع {};
    ناپ_t کل_لاگت = ادھار_لاگت;
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) کل_لاگت += stmt.m_لاگت;
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) کل_لاگت += stmt.m_لاگت;
    إذا (کل_لاگت <= max_لاگت) أرجع {};
    v3Global.useParallelBuild(سچ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_ونڈو_متن_Stmts(ctorStmts, اگیتر + "__ctor", params, دلیلاں, max_لاگت,
                                              support, descriptionp, emit_سرخی);
    std::string dtorDecl = emit_ونڈو_متن_Stmts(dtorStmts, اگیتر + "__dtor", params, دلیلاں, max_لاگت,
                                              support, descriptionp, emit_سرخی);
    أرجع {std::move(ctorDecl), std::move(dtorDecl)};
}

فراغ EmitCBaseVisitorConst::emitVarDecl(ثابت AstVar* nodep, منطقي asRef) {
    ثابت AstBasicDType* ثابت basicp = nodep->basicp();
    ثابت منطقي refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    إذا (nodep->mtask_Cache_سطر_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    إذا (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", خطأ, خطأ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        أرجع;
    }

    ثابت خودکار emitDeclArrayBrackets = [هذا](ثابت AstVar* nodep) -> فراغ {
        // This isn't very robust and may need cleanup for other data types
        لكل (ثابت AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvt_منزل_Str(arrayp->elementsConst()) + "]");
        }
    };

    إذا (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        إذا (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } وإلا إذا (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } وإلا إذا (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } وإلا {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        إذا (asRef) {
            إذا (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        إذا (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } وإلا إذا (nodep->isVLIO()) {
        إذا (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } وإلا إذا (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } وإلا إذا (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } وإلا {
            nodep->v3fatalSrc("Unknown type");
        }

        إذا (nodep->isQuad()) {
            puts("64");
        } وإلا إذا (nodep->چوڑائی_Min() <= 8) {
            puts("8");
        } وإلا إذا (nodep->چوڑائی_Min() <= 16) {
            puts("16");
        } وإلا إذا (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        إذا (asRef) {
            إذا (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        إذا (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvt_منزل_Str(basicp->lo() + nodep->چوڑائی() - 1) + "," + cvt_منزل_Str(basicp->lo()));
        إذا (nodep->isWide()) puts("," + cvt_منزل_Str(nodep->چوڑائی_الفاظ()));
        puts(");\n");
    } وإلا {
        // Strings and other fundamental C types
        إذا (nodep->isFuncLocal() && nodep->dtypep()->شامل_اے_String()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقي beStatic = name.ناپ() >= suffix.ناپ()
                                  && name.substr(name.ناپ() - suffix.ناپ()) == suffix;
            إذا (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(سچ, خطأ, خطأ, asRef));
        puts(";\n");
    }
}

فراغ EmitCBaseVisitorConst::emitVarAccessors(ثابت AstVar* nodep) {
    أكد(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ثابت string نجی_Name = nodep->name();
    ثابت string عوامی_Name = nodep->name().substr(لڑی_لمبائی("__Vm_sig_"));

    puts("decltype("s + نجی_Name + ") "s + عوامی_Name + "() {return "s + نجی_Name + ";}\n");
    puts("void "s + عوامی_Name + "(decltype(" + نجی_Name + ") v) {"s + نجی_Name + "=v;}\n");
}

فراغ EmitCBaseVisitorConst::emitModCUse(ثابت Ast_گرہ_Module* modp, VUseType useType) {
    منطقي nl = خطأ;
    forModCUse(modp, useType, [&](ثابت string& اندراج) {
        puts(اندراج);
        nl = سچ;
    });
    إذا (nl) puts("\n");
}

std::pair<string, File_سطر*> EmitCBaseVisitorConst::scSection(ثابت Ast_گرہ_Module* modp,
                                                              VSystemCSectionType type) {
    string متن;
    File_سطر* fl = خالی_سوچک;
    صحيح آخری_سطر = -999;
    لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        إذا (scSectionp->sectionType() != type) استمر;
        إذا (متن.خالی_تھاواں()) {
            fl = scSectionp->fileline();
            متن += "\n";
            إذا (v3Global.opt.decoration()) {
                متن += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        إذا (آخری_سطر + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            متن += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        آخری_سطر = scSectionp->fileline()->lineno();
        متن += scSectionp->متن();
    }
    إذا (!متن.خالی_تھاواں()) {
        إذا (v3Global.opt.decoration()) متن += "//*** Above code from `systemc in Verilog file\n";
        متن += "\n";
        // Substitute `systemc_class_name
        string::ناپ_type pos;
        طالما ((pos = متن.لبھو("`systemc_class_name")) != string::npos) {
            متن.replace(pos, std::لڑی_لمبائی("`systemc_class_name"),
                         EmitCUtil::اگیتر_Name_Protect(modp));
        }
    }
    أرجع std::make_pair(متن, fl);
}

فراغ EmitCBaseVisitorConst::emitSystemCSection(ثابت Ast_گرہ_Module* modp,
                                               VSystemCSectionType type) {
    خودکار متن_And_Fileline = scSection(modp, type);
    إذا (!متن_And_Fileline.پہلا.خالی_تھاواں()) ofp()->putsNoTracking(متن_And_Fileline.پہلا);
}
