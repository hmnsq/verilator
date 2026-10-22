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
    ثابت خپلکاری سېټ_All = [](Ast_غوټه_Module* modp) -> فراغ {
        لكل (Ast_غوټه* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, غوټه_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_غوټه_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    لكل (Ast_غوټه* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        سېټ_All(VN_AS(modp, غوټه_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::مختاړی_Name_Protect(ثابت Ast_غوټه* nodep) VL_MT_STABLE {
    ثابت string مختاړی = v3Global.opt.mod_مختاړی() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ساكن V3Mutex s_mutex;
    ثابت V3_قلف_Guard قلف_2{s_mutex};  // Otherwise map access is unsafe
    ساكن std::map<std::string, std::string> s_memoized;
    ساكن std::map<std::string, std::string> s_uc_منزل_مختاړی;
    // Memoize results
    ثابت خپلکاری mit = s_memoized.پیدا_کړه(مختاړی);
    إذا (mit != s_memoized.پای_ټکی()) أرجع mit->دوهم;
    //
    // Check capitalization
    ثابت string مختاړی_پاسنی = VString::upcase(مختاړی);
    string پایله;
    {
        ثابت خپلکاری it = s_uc_منزل_مختاړی.پیدا_کړه(مختاړی_پاسنی);
        إذا (it == s_uc_منزل_مختاړی.پای_ټکی()) {
            s_uc_منزل_مختاړی.emplace(مختاړی_پاسنی, مختاړی);
            پایله = مختاړی;
        } وإلا إذا (it->دوهم == مختاړی) {
            پایله = مختاړی;  // Same capitialization as last time
        } وإلا {
            VHashSha512 hash{مختاړی};
            پایله = مختاړی + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(مختاړی, پایله);
    أرجع پایله;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_غوټه_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    إذا (nodep->isConstructor()) {
        name += "init";
    } وإلا إذا (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::مختاړی_Name_Protect(modp);
    } وإلا {
        إذا (nodep->isLoose()) {
            name += EmitCUtil::مختاړی_Name_Protect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    أرجع name;
}

string EmitCBaseVisitorConst::c_Func_دلیلونه(ثابت AstCFunc* nodep, منطقي cpp) {
    // Return argument list for given C function
    string دلیلونه;
    إذا (nodep->isLoose() && !nodep->isStatic()) {
        إذا (nodep->voidSelf()) {
            دلیلونه += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } وإلا {
            إذا (nodep->isConst().true_معلوم()) دلیلونه += "const ";
            دلیلونه += EmitCUtil::مختاړی_Name_Protect(EmitCParentModule::get(nodep));
            دلیلونه += "* vlSelf";
        }
    }
    إذا (nodep->needProcess()) {
        إذا (!دلیلونه.تشه()) دلیلونه += ", ";
        دلیلونه += "VlProcessRef vlProcess";
    }
    إذا (!nodep->argTypes().تشه()) {
        إذا (!دلیلونه.تشه()) دلیلونه += ", ";
        دلیلونه += nodep->argTypes();
    }
    // Might be a user function with argument list.
    لكل (ثابت Ast_غوټه* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO() && !portp->isFuncReturn()) {
                إذا (!دلیلونه.تشه()) دلیلونه += ", ";
                إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    دلیلونه += portp->dpiArgType(ریښتیا, خطأ, cpp);
                } وإلا إذا (nodep->func_عامه()) {
                    دلیلونه += portp->cPubArgType(ریښتیا, خطأ);
                } وإلا {
                    دلیلونه += portp->vlArgType(ریښتیا, خطأ, ریښتیا);
                }
            }
        }
    }
    أرجع دلیلونه;
}

فراغ EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت Ast_غوټه_Module* ثابت modp) {
    puts(EmitCUtil::مختاړی_Name_Protect(modp));
    puts("() = default;\n");
}

فراغ EmitCBaseVisitorConst::emit_CFunc_سرلیک(ثابت AstCFunc* funcp, ثابت Ast_غوټه_Module* modp,
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
            putns(funcp, EmitCUtil::سر_Class_Name() + "::");
        } وإلا إذا (funcp->is_Proper_لاره()) {
            putns(funcp, EmitCUtil::مختاړی_Name_Protect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + c_Func_دلیلونه(funcp) + ")");
    إذا (funcp->isConst().true_معلوم() && funcp->is_Proper_لاره()) puts(" const");
}

فراغ EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_غوټه_Module* modp,
                                          منطقي cLinkage) {
    ensure_New_کرښه();
    إذا (!funcp->ifdef().تشه()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    إذا (funcp->isConstructor()) emitCDefaultConstructor(modp);
    إذا (cLinkage) putns(funcp, "extern \"C\" ");
    إذا (funcp->isStatic() && funcp->is_Proper_لاره()) putns(funcp, "static ");
    إذا (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->is_Proper_لاره(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emit_CFunc_سرلیک(funcp, modp, /* withScope: */ خطأ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    إذا (funcp->تشه_Body() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } وإلا {
        putns(funcp, ";\n");
    }
    إذا (!funcp->ifdef().تشه()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emit_وېشل_متن_Stmts(متن_Stmts& stmts, ثابت std::string& مختاړی,
                                                      ثابت std::string& params,
                                                      ثابت std::string& دلیلونه,  //
                                                      اندازه_t max_لګښت, منطقي support,
                                                      ثابت حرف* descriptionp,
                                                      ثابت std::function<فراغ()>& emit_سرلیک) {
    ثابت خپلکاری subFuncName = [&](اندازه_t n) { أرجع مختاړی + "__" + std::منزل_string(n); };
    اندازه_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    منطقي callsSubFuncs = خطأ;
    اندازه_t callee_بنسټ = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    طالما (stmts.اندازه() > 1) {
        ثابت اندازه_t level_بنسټ = nSubFunctions;
        متن_Stmts calls;  // Calls to the sub-functions of this level
        ثابت اندازه_t nStmts = stmts.اندازه();
        لكل (اندازه_t وېشل_پیل = 0, وېشل_پای_ټکی = 0; وېشل_پیل < nStmts; وېشل_پیل = وېشل_پای_ټکی) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            اندازه_t لګښت = 0;
            طالما (((لګښت < max_لګښت) || (وېشل_پای_ټکی - وېشل_پیل < 2)) && وېشل_پای_ټکی < nStmts) {
                لګښت += stmts[وېشل_پای_ټکی++].m_لګښت;
            }
            UASSERT(وېشل_پیل < وېشل_پای_ټکی, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            open_New_Output_سرچینه_File(funcName, ریښتیا, support, descriptionp);
            // Emit header
            emit_سرلیک();
            // Declare called sub-functions
            إذا (callsSubFuncs) {
                puts("\n");
                لكل (اندازه_t j = وېشل_پیل; j < وېشل_پای_ټکی; ++j) {
                    puts("void " + subFuncName(callee_بنسټ + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            لكل (اندازه_t j = وېشل_پیل; j < وېشل_پای_ټکی; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_متن);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.جمع_کړه(funcName + "(" + دلیلونه + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = ریښتیا;
        callee_بنسټ = level_بنسټ;
    }
    أرجع funcName.تشه() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emit_وېشل_Ctor_Dtor(
    متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, اندازه_t بنسټ_لګښت, ثابت std::string& مختاړی,
    ثابت std::string& params, ثابت std::string& دلیلونه, منطقي support, ثابت حرف* descriptionp,
    ثابت std::function<فراغ()>& emit_سرلیک) {
    ثابت اندازه_t max_لګښت = static_cast<اندازه_t>(v3Global.opt.output_وېشل_CFuncs());
    إذا (!max_لګښت) أرجع {};
    اندازه_t ټول_لګښت = بنسټ_لګښت;
    لكل (ثابت متن_Stmts::Stmt& stmt : ctorStmts) ټول_لګښت += stmt.m_لګښت;
    لكل (ثابت متن_Stmts::Stmt& stmt : dtorStmts) ټول_لګښت += stmt.m_لګښت;
    إذا (ټول_لګښت <= max_لګښت) أرجع {};
    v3Global.useParallelBuild(ریښتیا);  // Splitting files, so using parallel build.
    std::string ctorDecl = emit_وېشل_متن_Stmts(ctorStmts, مختاړی + "__ctor", params, دلیلونه, max_لګښت,
                                              support, descriptionp, emit_سرلیک);
    std::string dtorDecl = emit_وېشل_متن_Stmts(dtorStmts, مختاړی + "__dtor", params, دلیلونه, max_لګښت,
                                              support, descriptionp, emit_سرلیک);
    أرجع {std::move(ctorDecl), std::move(dtorDecl)};
}

فراغ EmitCBaseVisitorConst::emitVarDecl(ثابت AstVar* nodep, منطقي asRef) {
    ثابت AstBasicDType* ثابت basicp = nodep->basicp();
    ثابت منطقي refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    إذا (nodep->mtask_Cache_کرښه_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    إذا (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", خطأ, خطأ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        أرجع;
    }

    ثابت خپلکاری emitDeclArrayBrackets = [هذا](ثابت AstVar* nodep) -> فراغ {
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
        } وإلا إذا (nodep->پلنوالی_Min() <= 8) {
            puts("8");
        } وإلا إذا (nodep->پلنوالی_Min() <= 16) {
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
        puts("," + cvt_منزل_Str(basicp->lo() + nodep->پلنوالی() - 1) + "," + cvt_منزل_Str(basicp->lo()));
        إذا (nodep->isWide()) puts("," + cvt_منزل_Str(nodep->پلنوالی_ټکي()));
        puts(");\n");
    } وإلا {
        // Strings and other fundamental C types
        إذا (nodep->isFuncLocal() && nodep->dtypep()->لري_String()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقي beStatic = name.اندازه() >= suffix.اندازه()
                                  && name.substr(name.اندازه() - suffix.اندازه()) == suffix;
            إذا (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(ریښتیا, خطأ, خطأ, asRef));
        puts(";\n");
    }
}

فراغ EmitCBaseVisitorConst::emitVarAccessors(ثابت AstVar* nodep) {
    أكد(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ثابت string شخصي_Name = nodep->name();
    ثابت string عامه_Name = nodep->name().substr(مزي_اوږدوالی("__Vm_sig_"));

    puts("decltype("s + شخصي_Name + ") "s + عامه_Name + "() {return "s + شخصي_Name + ";}\n");
    puts("void "s + عامه_Name + "(decltype(" + شخصي_Name + ") v) {"s + شخصي_Name + "=v;}\n");
}

فراغ EmitCBaseVisitorConst::emitModCUse(ثابت Ast_غوټه_Module* modp, VUseType useType) {
    منطقي nl = خطأ;
    forModCUse(modp, useType, [&](ثابت string& ثبت) {
        puts(ثبت);
        nl = ریښتیا;
    });
    إذا (nl) puts("\n");
}

std::pair<string, File_کرښه*> EmitCBaseVisitorConst::scSection(ثابت Ast_غوټه_Module* modp,
                                                              VSystemCSectionType type) {
    string متن;
    File_کرښه* fl = تش_اشاره;
    صحيح وروستی_کرښه = -999;
    لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        إذا (scSectionp->sectionType() != type) استمر;
        إذا (متن.تشه()) {
            fl = scSectionp->fileline();
            متن += "\n";
            إذا (v3Global.opt.decoration()) {
                متن += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        إذا (وروستی_کرښه + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            متن += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        وروستی_کرښه = scSectionp->fileline()->lineno();
        متن += scSectionp->متن();
    }
    إذا (!متن.تشه()) {
        إذا (v3Global.opt.decoration()) متن += "//*** Above code from `systemc in Verilog file\n";
        متن += "\n";
        // Substitute `systemc_class_name
        string::اندازه_type pos;
        طالما ((pos = متن.پیدا_کړه("`systemc_class_name")) != string::npos) {
            متن.replace(pos, std::مزي_اوږدوالی("`systemc_class_name"),
                         EmitCUtil::مختاړی_Name_Protect(modp));
        }
    }
    أرجع std::make_pair(متن, fl);
}

فراغ EmitCBaseVisitorConst::emitSystemCSection(ثابت Ast_غوټه_Module* modp,
                                               VSystemCSectionType type) {
    خپلکاری متن_And_Fileline = scSection(modp, type);
    إذا (!متن_And_Fileline.لومړی.تشه()) ofp()->putsNoTracking(متن_And_Fileline.لومړی);
}
