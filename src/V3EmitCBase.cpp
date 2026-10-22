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
    ثابت auto setAll = [](Ast_نود_Module* modp) -> فراغ {
        لكل (Ast_نود* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            إذا (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, نود_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_نود_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    لكل (Ast_نود* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, نود_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(ثابت Ast_نود* nodep) VL_MT_STABLE {
    ثابت string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ساكن V3Mutex s_mutex;
    ثابت V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    ساكن std::map<std::string, std::string> s_memoized;
    ساكن std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    ثابت auto mit = s_memoized.find(prefix);
    إذا (mit != s_memoized.end()) أرجع mit->second;
    //
    // Check capitalization
    ثابت string prefixUpper = VString::upcase(prefix);
    string result;
    {
        ثابت auto it = s_ucToPrefix.find(prefixUpper);
        إذا (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } وإلا إذا (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } وإلا {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    أرجع result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_نود_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    إذا (nodep->isConstructor()) {
        name += "init";
    } وإلا إذا (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } وإلا {
        إذا (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    أرجع name;
}

string EmitCBaseVisitorConst::cFuncArgs(ثابت AstCFunc* nodep, منطقي cpp) {
    // Return argument list for given C function
    string args;
    إذا (nodep->isLoose() && !nodep->isStatic()) {
        إذا (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } وإلا {
            إذا (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    إذا (nodep->needProcess()) {
        إذا (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    إذا (!nodep->argTypes().empty()) {
        إذا (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    لكل (ثابت Ast_نود* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO() && !portp->isFuncReturn()) {
                إذا (!args.empty()) args += ", ";
                إذا (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(true, خطأ, cpp);
                } وإلا إذا (nodep->funcPublic()) {
                    args += portp->cPubArgType(true, خطأ);
                } وإلا {
                    args += portp->vlArgType(true, خطأ, true);
                }
            }
        }
    }
    أرجع args;
}

فراغ EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت Ast_نود_Module* ثابت modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

فراغ EmitCBaseVisitorConst::emitCFuncHeader(ثابت AstCFunc* funcp, ثابت Ast_نود_Module* modp,
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
            putns(funcp, EmitCUtil::اتس_Class_Name() + "::");
        } وإلا إذا (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    إذا (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

فراغ EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_نود_Module* modp,
                                          منطقي cLinkage) {
    ensure_New_باريس();
    إذا (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    إذا (funcp->isConstructor()) emitCDefaultConstructor(modp);
    إذا (cLinkage) putns(funcp, "extern \"C\" ");
    إذا (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    إذا (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ خطأ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    إذا (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } وإلا {
        putns(funcp, ";\n");
    }
    إذا (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, ثابت std::string& prefix,
                                                      ثابت std::string& params,
                                                      ثابت std::string& args,  //
                                                      size_t maxCost, منطقي support,
                                                      ثابت حرف* descriptionp,
                                                      ثابت std::function<فراغ()>& emitHeader) {
    ثابت auto subFuncName = [&](size_t n) { أرجع prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    منطقي callsSubFuncs = خطأ;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    طالما (stmts.size() > 1) {
        ثابت size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        ثابت size_t nStmts = stmts.size();
        لكل (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            طالما (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, true, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            إذا (callsSubFuncs) {
                puts("\n");
                لكل (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            لكل (size_t j = splitStart; j < splitEnd; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_text);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            puts("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.add(funcName + "(" + args + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = true;
        calleeBase = levelBase;
    }
    أرجع funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, ثابت std::string& prefix,
    ثابت std::string& params, ثابت std::string& args, منطقي support, ثابت حرف* descriptionp,
    ثابت std::function<فراغ()>& emitHeader) {
    ثابت size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    إذا (!maxCost) أرجع {};
    size_t totalCost = baseCost;
    لكل (ثابت TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    لكل (ثابت TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    إذا (totalCost <= maxCost) أرجع {};
    v3Global.useParallelBuild(true);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    أرجع {std::move(ctorDecl), std::move(dtorDecl)};
}

فراغ EmitCBaseVisitorConst::emitVarDecl(ثابت AstVar* nodep, منطقي asRef) {
    ثابت AstBasicDType* ثابت basicp = nodep->basicp();
    ثابت منطقي refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    إذا (nodep->mtask_Cache_باريس_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    إذا (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", خطأ, خطأ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        أرجع;
    }

    ثابت auto emitDeclArrayBrackets = [هذا](ثابت AstVar* nodep) -> فراغ {
        // This isn't very robust and may need cleanup for other data types
        لكل (ثابت AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
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
        } وإلا إذا (nodep->widthMin() <= 8) {
            puts("8");
        } وإلا إذا (nodep->widthMin() <= 16) {
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
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        إذا (nodep->isWide()) puts("," + cvtToStr(nodep->width_ڤرکاتاءن()));
        puts(");\n");
    } وإلا {
        // Strings and other fundamental C types
        إذا (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقي beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            إذا (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(true, خطأ, خطأ, asRef));
        puts(";\n");
    }
}

فراغ EmitCBaseVisitorConst::emitVarAccessors(ثابت AstVar* nodep) {
    أكد(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ثابت string privateName = nodep->name();
    ثابت string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

فراغ EmitCBaseVisitorConst::emitModCUse(ثابت Ast_نود_Module* modp, VUseType useType) {
    منطقي nl = خطأ;
    forModCUse(modp, useType, [&](ثابت string& entry) {
        puts(entry);
        nl = true;
    });
    إذا (nl) puts("\n");
}

std::pair<string, File_باريس*> EmitCBaseVisitorConst::scSection(ثابت Ast_نود_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_باريس* fl = nullptr;
    صحيح last_باريس = -999;
    لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        إذا (scSectionp->sectionType() != type) استمر;
        إذا (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            إذا (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        إذا (last_باريس + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_باريس = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    إذا (!text.empty()) {
        إذا (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        طالما ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    أرجع std::make_pair(text, fl);
}

فراغ EmitCBaseVisitorConst::emitSystemCSection(ثابت Ast_نود_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    إذا (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
