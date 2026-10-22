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
    ثابت خودڪار سيٽ_All = [](Ast_ڳنڍ_Module* modp) -> فراغ {
        لكل (Ast_ڳنڍ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ڳنڍ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ڳنڍ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    لكل (Ast_ڳنڍ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        سيٽ_All(VN_AS(modp, ڳنڍ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::اڳياڙي_Name_Protect(ثابت Ast_ڳنڍ* nodep) VL_MT_STABLE {
    ثابت string اڳياڙي = v3Global.opt.mod_اڳياڙي() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ساكن V3Mutex s_mutex;
    ثابت V3_تالو_Guard تالو_2{s_mutex};  // Otherwise map access is unsafe
    ساكن std::map<std::string, std::string> s_memoized;
    ساكن std::map<std::string, std::string> s_uc_منزل_اڳياڙي;
    // Memoize results
    ثابت خودڪار mit = s_memoized.ڳوليو(اڳياڙي);
    إذا (mit != s_memoized.پڇاڙي()) أرجع mit->ٻيو;
    //
    // Check capitalization
    ثابت string اڳياڙي_مٿانهون = VString::upcase(اڳياڙي);
    string نتيجو;
    {
        ثابت خودڪار it = s_uc_منزل_اڳياڙي.ڳوليو(اڳياڙي_مٿانهون);
        إذا (it == s_uc_منزل_اڳياڙي.پڇاڙي()) {
            s_uc_منزل_اڳياڙي.emplace(اڳياڙي_مٿانهون, اڳياڙي);
            نتيجو = اڳياڙي;
        } وإلا إذا (it->ٻيو == اڳياڙي) {
            نتيجو = اڳياڙي;  // Same capitialization as last time
        } وإلا {
            VHashSha512 hash{اڳياڙي};
            نتيجو = اڳياڙي + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(اڳياڙي, نتيجو);
    أرجع نتيجو;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_ڳنڍ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    إذا (nodep->isConstructor()) {
        name += "init";
    } وإلا إذا (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::اڳياڙي_Name_Protect(modp);
    } وإلا {
        إذا (nodep->isLoose()) {
            name += EmitCUtil::اڳياڙي_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    أرجع name;
}

string EmitCBaseVisitorConst::c_Func_دليلون(ثابت AstCFunc* nodep, منطقي cpp) {
    // Return argument list for given C function
    string دليلون;
    إذا (nodep->isLoose() && !nodep->isStatic()) {
        إذا (nodep->voidSelf()) {
            دليلون += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } وإلا {
            إذا (nodep->isConst().true_معلوم()) دليلون += "const ";
            دليلون += EmitCUtil::اڳياڙي_Name_Protect(EmitCParentModule::get(nodep));
            دليلون += "* vlSelf";
        }
    }
    إذا (nodep->needProcess()) {
        إذا (!دليلون.خالي_جايون()) دليلون += ", ";
        دليلون += "VlProcessRef vlProcess";
    }
    إذا (!nodep->argTypes().خالي_جايون()) {
        إذا (!دليلون.خالي_جايون()) دليلون += ", ";
        دليلون += nodep->argTypes();
    }
    // Might be a user function with argument list.
    لكل (ثابت Ast_ڳنڍ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO() && !portp->isFuncReturn()) {
                إذا (!دليلون.خالي_جايون()) دليلون += ", ";
                إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    دليلون += portp->dpiArgType(سچ, خطأ, cpp);
                } وإلا إذا (nodep->func_عوامي()) {
                    دليلون += portp->cPubArgType(سچ, خطأ);
                } وإلا {
                    دليلون += portp->vlArgType(سچ, خطأ, سچ);
                }
            }
        }
    }
    أرجع دليلون;
}

فراغ EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت Ast_ڳنڍ_Module* ثابت modp) {
    puts(EmitCUtil::اڳياڙي_Name_Protect(modp));
    puts("() = default;\n");
}

فراغ EmitCBaseVisitorConst::emit_CFunc_سرو(ثابت AstCFunc* funcp, ثابت Ast_ڳنڍ_Module* modp,
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
            putns(funcp, EmitCUtil::چوٽي_Class_Name() + "::");
        } وإلا إذا (funcp->is_Proper_طريقو()) {
            putns(funcp, EmitCUtil::اڳياڙي_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_دليلون(funcp) + ")");
    إذا (funcp->isConst().true_معلوم() && funcp->is_Proper_طريقو()) puts(" const");
}

فراغ EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_ڳنڍ_Module* modp,
                                          منطقي cLinkage) {
    ensure_New_سٽ();
    إذا (!funcp->ifdef().خالي_جايون()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    إذا (funcp->isConstructor()) emitCDefaultConstructor(modp);
    إذا (cLinkage) putns(funcp, "extern \"C\" ");
    إذا (funcp->isStatic() && funcp->is_Proper_طريقو()) putns(funcp, "static ");
    إذا (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_طريقو(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_سرو(funcp, modp, /* withScope: */ خطأ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    إذا (funcp->خالي_جايون_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } وإلا {
        putns(funcp, ";\n");
    }
    إذا (!funcp->ifdef().خالي_جايون()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_ورهايو_متن_Stmts(متن_Stmts& stmts, ثابت std::string& اڳياڙي,
                                                      ثابت std::string& params,
                                                      ثابت std::string& دليلون,  //
                                                      سائيز_t max_لاڳت, منطقي support,
                                                      ثابت حرف* descriptionp,
                                                      ثابت std::function<فراغ()>& emit_سرو) {
    ثابت خودڪار subFuncName = [&](سائيز_t n) { أرجع اڳياڙي + "__" + std::منزل_string(n); };
    سائيز_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    منطقي callsSubFuncs = خطأ;
    سائيز_t callee_بنياد = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    طالما (stmts.سائيز() > 1) {
        ثابت سائيز_t level_بنياد = nSubFunctions;
        متن_Stmts calls;  // Calls to the sub-functions of this level
        ثابت سائيز_t nStmts = stmts.سائيز();
        لكل (سائيز_t ورهايو_شروعات = 0, ورهايو_پڇاڙي = 0; ورهايو_شروعات < nStmts; ورهايو_شروعات = ورهايو_پڇاڙي) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            سائيز_t لاڳت = 0;
            طالما (((لاڳت < max_لاڳت) || (ورهايو_پڇاڙي - ورهايو_شروعات < 2)) && ورهايو_پڇاڙي < nStmts) {
                لاڳت += stmts[ورهايو_پڇاڙي++].m_لاڳت;
            }
            UASSERT(ورهايو_شروعات < ورهايو_پڇاڙي, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_ذريعو_File(funcName, سچ, support, descriptionp);
            // Emit header
            emit_سرو();
            // Declare called sub-functions
            إذا (callsSubFuncs) {
                puts("\n");
                لكل (سائيز_t j = ورهايو_شروعات; j < ورهايو_پڇاڙي; ++j) {
                    puts("void " + subFuncName(callee_بنياد + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            لكل (سائيز_t j = ورهايو_شروعات; j < ورهايو_پڇاڙي; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_متن);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.جوڙيو(funcName + "(" + دليلون + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = سچ;
        callee_بنياد = level_بنياد;
    }
    أرجع funcName.خالي_جايون() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_ورهايو_Ctor_Dtor(
    متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, سائيز_t بنياد_لاڳت, ثابت std::string& اڳياڙي,
    ثابت std::string& params, ثابت std::string& دليلون, منطقي support, ثابت حرف* descriptionp,
    ثابت std::function<فراغ()>& emit_سرو) {
    ثابت سائيز_t max_لاڳت = static_cast<سائيز_t>(v3Global.opt.output_ورهايو_CFuncs());
    إذا (!max_لاڳت) أرجع {};
    سائيز_t ڪل_لاڳت = بنياد_لاڳت;
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) ڪل_لاڳت += stmt.m_لاڳت;
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) ڪل_لاڳت += stmt.m_لاڳت;
    إذا (ڪل_لاڳت <= max_لاڳت) أرجع {};
    v3Global.useParallelBuild(سچ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_ورهايو_متن_Stmts(ctorStmts, اڳياڙي + "__ctor", params, دليلون, max_لاڳت,
                                              support, descriptionp, emit_سرو);
    std::string dtorDecl = emit_ورهايو_متن_Stmts(dtorStmts, اڳياڙي + "__dtor", params, دليلون, max_لاڳت,
                                              support, descriptionp, emit_سرو);
    أرجع {std::move(ctorDecl), std::move(dtorDecl)};
}

فراغ EmitCBaseVisitorConst::emitVarDecl(ثابت AstVar* nodep, منطقي asRef) {
    ثابت AstBasicDType* ثابت basicp = nodep->basicp();
    ثابت منطقي refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    إذا (nodep->mtask_Cache_سٽ_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    إذا (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", خطأ, خطأ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        أرجع;
    }

    ثابت خودڪار emitDeclArrayBrackets = [هذا](ثابت AstVar* nodep) -> فراغ {
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
        } وإلا إذا (nodep->ويڪر_Min() <= 8) {
            puts("8");
        } وإلا إذا (nodep->ويڪر_Min() <= 16) {
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
        puts("," + cvt_منزل_Str(basicp->lo() + nodep->ويڪر() - 1) + "," + cvt_منزل_Str(basicp->lo()));
        إذا (nodep->isWide()) puts("," + cvt_منزل_Str(nodep->ويڪر_لفظ()));
        puts(");\n");
    } وإلا {
        // Strings and other fundamental C types
        إذا (nodep->isFuncLocal() && nodep->dtypep()->شامل_آهي_String()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقي beStatic = name.سائيز() >= suffix.سائيز()
                                  && name.substr(name.سائيز() - suffix.سائيز()) == suffix;
            إذا (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(سچ, خطأ, خطأ, asRef));
        puts(";\n");
    }
}

فراغ EmitCBaseVisitorConst::emitVarAccessors(ثابت AstVar* nodep) {
    أكد(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ثابت string ذاتي_Name = nodep->name();
    ثابت string عوامي_Name = nodep->name().substr(لڙي_ڊيگهه("__Vm_sig_"));

    puts("decltype("s + ذاتي_Name + ") "s + عوامي_Name + "() {return "s + ذاتي_Name + ";}\n");
    puts("void "s + عوامي_Name + "(decltype(" + ذاتي_Name + ") v) {"s + ذاتي_Name + "=v;}\n");
}

فراغ EmitCBaseVisitorConst::emitModCUse(ثابت Ast_ڳنڍ_Module* modp, VUseType useType) {
    منطقي nl = خطأ;
    forModCUse(modp, useType, [&](ثابت string& داخلا_2) {
        puts(داخلا_2);
        nl = سچ;
    });
    إذا (nl) puts("\n");
}

std::pair<string, File_سٽ*> EmitCBaseVisitorConst::scSection(ثابت Ast_ڳنڍ_Module* modp,
                                                              VSystemCSectionType type) {
    string متن;
    File_سٽ* fl = خالي_اشارو;
    صحيح آخري_سٽ = -999;
    لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        إذا (scSectionp->sectionType() != type) استمر;
        إذا (متن.خالي_جايون()) {
            fl = scSectionp->fileline();
            متن += "\n";
            إذا (v3Global.opt.decoration()) {
                متن += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        إذا (آخري_سٽ + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            متن += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        آخري_سٽ = scSectionp->fileline()->lineno();
        متن += scSectionp->متن();
    }
    إذا (!متن.خالي_جايون()) {
        إذا (v3Global.opt.decoration()) متن += "//*** Above code from `systemc in Verilog file\n";
        متن += "\n";
        // Substitute `systemc_class_name
        string::سائيز_type pos;
        طالما ((pos = متن.ڳوليو("`systemc_class_name")) != string::npos) {
            متن.replace(pos, std::لڙي_ڊيگهه("`systemc_class_name"),
                         EmitCUtil::اڳياڙي_Name_Protect(modp));
        }
    }
    أرجع std::make_pair(متن, fl);
}

فراغ EmitCBaseVisitorConst::emitSystemCSection(ثابت Ast_ڳنڍ_Module* modp,
                                               VSystemCSectionType type) {
    خودڪار متن_And_Fileline = scSection(modp, type);
    إذا (!متن_And_Fileline.پهريون.خالي_جايون()) ofp()->putsNoTracking(متن_And_Fileline.پهريون);
}
