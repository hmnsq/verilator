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
    ثابت خودکار مجموعه_All = [](Ast_گره_Module* modp) -> فراغ {
        لكل (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, گره_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_گره_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    لكل (Ast_گره* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        مجموعه_All(VN_AS(modp, گره_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::پیشوند_Name_Protect(ثابت Ast_گره* nodep) VL_MT_STABLE {
    ثابت string پیشوند = v3Global.opt.mod_پیشوند() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ساكن V3Mutex s_mutex;
    ثابت V3_قفل_Guard قفل_2{s_mutex};  // Otherwise map access is unsafe
    ساكن std::map<std::string, std::string> s_memoized;
    ساكن std::map<std::string, std::string> s_uc_مقصد_پیشوند;
    // Memoize results
    ثابت خودکار mit = s_memoized.پیدا_کن(پیشوند);
    إذا (mit != s_memoized.انتها()) أرجع mit->دومی;
    //
    // Check capitalization
    ثابت string پیشوند_فوقانی = VString::upcase(پیشوند);
    string نتیجه;
    {
        ثابت خودکار it = s_uc_مقصد_پیشوند.پیدا_کن(پیشوند_فوقانی);
        إذا (it == s_uc_مقصد_پیشوند.انتها()) {
            s_uc_مقصد_پیشوند.emplace(پیشوند_فوقانی, پیشوند);
            نتیجه = پیشوند;
        } وإلا إذا (it->دومی == پیشوند) {
            نتیجه = پیشوند;  // Same capitialization as last time
        } وإلا {
            VHashSha512 hash{پیشوند};
            نتیجه = پیشوند + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(پیشوند, نتیجه);
    أرجع نتیجه;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گره_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    إذا (nodep->isConstructor()) {
        name += "init";
    } وإلا إذا (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::پیشوند_Name_Protect(modp);
    } وإلا {
        إذا (nodep->isLoose()) {
            name += EmitCUtil::پیشوند_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    أرجع name;
}

string EmitCBaseVisitorConst::c_Func_آرگومانها(ثابت AstCFunc* nodep, منطقي cpp) {
    // Return argument list for given C function
    string آرگومانها;
    إذا (nodep->isLoose() && !nodep->isStatic()) {
        إذا (nodep->voidSelf()) {
            آرگومانها += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } وإلا {
            إذا (nodep->isConst().true_معلوم()) آرگومانها += "const ";
            آرگومانها += EmitCUtil::پیشوند_Name_Protect(EmitCParentModule::get(nodep));
            آرگومانها += "* vlSelf";
        }
    }
    إذا (nodep->needProcess()) {
        إذا (!آرگومانها.خالی()) آرگومانها += ", ";
        آرگومانها += "VlProcessRef vlProcess";
    }
    إذا (!nodep->argTypes().خالی()) {
        إذا (!آرگومانها.خالی()) آرگومانها += ", ";
        آرگومانها += nodep->argTypes();
    }
    // Might be a user function with argument list.
    لكل (ثابت Ast_گره* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO() && !portp->isFuncReturn()) {
                إذا (!آرگومانها.خالی()) آرگومانها += ", ";
                إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    آرگومانها += portp->dpiArgType(درست, خطأ, cpp);
                } وإلا إذا (nodep->func_عمومی()) {
                    آرگومانها += portp->cPubArgType(درست, خطأ);
                } وإلا {
                    آرگومانها += portp->vlArgType(درست, خطأ, درست);
                }
            }
        }
    }
    أرجع آرگومانها;
}

فراغ EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت Ast_گره_Module* ثابت modp) {
    puts(EmitCUtil::پیشوند_Name_Protect(modp));
    puts("() = default;\n");
}

فراغ EmitCBaseVisitorConst::emit_CFunc_سرآیند(ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp,
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
            putns(funcp, EmitCUtil::بالاترین_Class_Name() + "::");
        } وإلا إذا (funcp->is_Proper_روش()) {
            putns(funcp, EmitCUtil::پیشوند_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_آرگومانها(funcp) + ")");
    إذا (funcp->isConst().true_معلوم() && funcp->is_Proper_روش()) puts(" const");
}

فراغ EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp,
                                          منطقي cLinkage) {
    ensure_New_سطر();
    إذا (!funcp->ifdef().خالی()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    إذا (funcp->isConstructor()) emitCDefaultConstructor(modp);
    إذا (cLinkage) putns(funcp, "extern \"C\" ");
    إذا (funcp->isStatic() && funcp->is_Proper_روش()) putns(funcp, "static ");
    إذا (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_روش(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_سرآیند(funcp, modp, /* withScope: */ خطأ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    إذا (funcp->خالی_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } وإلا {
        putns(funcp, ";\n");
    }
    إذا (!funcp->ifdef().خالی()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_تقسیم_متن_Stmts(متن_Stmts& stmts, ثابت std::string& پیشوند,
                                                      ثابت std::string& params,
                                                      ثابت std::string& آرگومانها,  //
                                                      اندازه_t max_هزینه, منطقي support,
                                                      ثابت حرف* descriptionp,
                                                      ثابت std::function<فراغ()>& emit_سرآیند) {
    ثابت خودکار subFuncName = [&](اندازه_t n) { أرجع پیشوند + "__" + std::مقصد_string(n); };
    اندازه_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    منطقي callsSubFuncs = خطأ;
    اندازه_t callee_پایه = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    طالما (stmts.اندازه() > 1) {
        ثابت اندازه_t level_پایه = nSubFunctions;
        متن_Stmts calls;  // Calls to the sub-functions of this level
        ثابت اندازه_t nStmts = stmts.اندازه();
        لكل (اندازه_t تقسیم_شروع = 0, تقسیم_انتها = 0; تقسیم_شروع < nStmts; تقسیم_شروع = تقسیم_انتها) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            اندازه_t هزینه = 0;
            طالما (((هزینه < max_هزینه) || (تقسیم_انتها - تقسیم_شروع < 2)) && تقسیم_انتها < nStmts) {
                هزینه += stmts[تقسیم_انتها++].m_هزینه;
            }
            UASSERT(تقسیم_شروع < تقسیم_انتها, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_منبع_File(funcName, درست, support, descriptionp);
            // Emit header
            emit_سرآیند();
            // Declare called sub-functions
            إذا (callsSubFuncs) {
                puts("\n");
                لكل (اندازه_t j = تقسیم_شروع; j < تقسیم_انتها; ++j) {
                    puts("void " + subFuncName(callee_پایه + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            لكل (اندازه_t j = تقسیم_شروع; j < تقسیم_انتها; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_متن);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.جمع_کن(funcName + "(" + آرگومانها + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = درست;
        callee_پایه = level_پایه;
    }
    أرجع funcName.خالی() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_تقسیم_Ctor_Dtor(
    متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, اندازه_t پایه_هزینه, ثابت std::string& پیشوند,
    ثابت std::string& params, ثابت std::string& آرگومانها, منطقي support, ثابت حرف* descriptionp,
    ثابت std::function<فراغ()>& emit_سرآیند) {
    ثابت اندازه_t max_هزینه = static_cast<اندازه_t>(v3Global.opt.output_تقسیم_CFuncs());
    إذا (!max_هزینه) أرجع {};
    اندازه_t مجموع_هزینه = پایه_هزینه;
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) مجموع_هزینه += stmt.m_هزینه;
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) مجموع_هزینه += stmt.m_هزینه;
    إذا (مجموع_هزینه <= max_هزینه) أرجع {};
    v3Global.useParallelBuild(درست);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_تقسیم_متن_Stmts(ctorStmts, پیشوند + "__ctor", params, آرگومانها, max_هزینه,
                                              support, descriptionp, emit_سرآیند);
    std::string dtorDecl = emit_تقسیم_متن_Stmts(dtorStmts, پیشوند + "__dtor", params, آرگومانها, max_هزینه,
                                              support, descriptionp, emit_سرآیند);
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
            putns(arrayp, "[" + cvt_مقصد_Str(arrayp->elementsConst()) + "]");
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
        } وإلا إذا (nodep->عرض_Min() <= 8) {
            puts("8");
        } وإلا إذا (nodep->عرض_Min() <= 16) {
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
        puts("," + cvt_مقصد_Str(basicp->lo() + nodep->عرض() - 1) + "," + cvt_مقصد_Str(basicp->lo()));
        إذا (nodep->isWide()) puts("," + cvt_مقصد_Str(nodep->عرض_واژگان()));
        puts(");\n");
    } وإلا {
        // Strings and other fundamental C types
        إذا (nodep->isFuncLocal() && nodep->dtypep()->شامل_است_String()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقي beStatic = name.اندازه() >= suffix.اندازه()
                                  && name.substr(name.اندازه() - suffix.اندازه()) == suffix;
            إذا (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(درست, خطأ, خطأ, asRef));
        puts(";\n");
    }
}

فراغ EmitCBaseVisitorConst::emitVarAccessors(ثابت AstVar* nodep) {
    أكد(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ثابت string خصوصی_Name = nodep->name();
    ثابت string عمومی_Name = nodep->name().substr(طول_رشته("__Vm_sig_"));

    puts("decltype("s + خصوصی_Name + ") "s + عمومی_Name + "() {return "s + خصوصی_Name + ";}\n");
    puts("void "s + عمومی_Name + "(decltype(" + خصوصی_Name + ") v) {"s + خصوصی_Name + "=v;}\n");
}

فراغ EmitCBaseVisitorConst::emitModCUse(ثابت Ast_گره_Module* modp, VUseType useType) {
    منطقي nl = خطأ;
    forModCUse(modp, useType, [&](ثابت string& مدخل) {
        puts(مدخل);
        nl = درست;
    });
    إذا (nl) puts("\n");
}

std::pair<string, File_سطر*> EmitCBaseVisitorConst::scSection(ثابت Ast_گره_Module* modp,
                                                              VSystemCSectionType type) {
    string متن;
    File_سطر* fl = اشاره_تهی;
    صحيح آخرین_سطر = -999;
    لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        إذا (scSectionp->sectionType() != type) استمر;
        إذا (متن.خالی()) {
            fl = scSectionp->fileline();
            متن += "\n";
            إذا (v3Global.opt.decoration()) {
                متن += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        إذا (آخرین_سطر + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            متن += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        آخرین_سطر = scSectionp->fileline()->lineno();
        متن += scSectionp->متن();
    }
    إذا (!متن.خالی()) {
        إذا (v3Global.opt.decoration()) متن += "//*** Above code from `systemc in Verilog file\n";
        متن += "\n";
        // Substitute `systemc_class_name
        string::اندازه_type pos;
        طالما ((pos = متن.پیدا_کن("`systemc_class_name")) != string::npos) {
            متن.replace(pos, std::طول_رشته("`systemc_class_name"),
                         EmitCUtil::پیشوند_Name_Protect(modp));
        }
    }
    أرجع std::make_pair(متن, fl);
}

فراغ EmitCBaseVisitorConst::emitSystemCSection(ثابت Ast_گره_Module* modp,
                                               VSystemCSectionType type) {
    خودکار متن_And_Fileline = scSection(modp, type);
    إذا (!متن_And_Fileline.اولی.خالی()) ofp()->putsNoTracking(متن_And_Fileline.اولی);
}
