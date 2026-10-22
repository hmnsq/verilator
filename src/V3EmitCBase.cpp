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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3EmitCBase.h"

#അന്തര്ഭാവ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    സ്ഥിരം auto setAll = [](Ast_കണ്ണി_Module* modp) -> ശൂന്യം {
        വേണ്ടി (Ast_കണ്ണി* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, കണ്ണി_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_കണ്ണി_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    വേണ്ടി (Ast_കണ്ണി* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, കണ്ണി_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(സ്ഥിരം Ast_കണ്ണി* nodep) VL_MT_STABLE {
    സ്ഥിരം string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    സ്ഥിതം V3Mutex s_mutex;
    സ്ഥിരം V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    സ്ഥിതം std::map<std::string, std::string> s_memoized;
    സ്ഥിതം std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    സ്ഥിരം auto mit = s_memoized.find(prefix);
    എങ്കിൽ (mit != s_memoized.end()) തിരികെ mit->second;
    //
    // Check capitalization
    സ്ഥിരം string prefixUpper = VString::upcase(prefix);
    string result;
    {
        സ്ഥിരം auto it = s_ucToPrefix.find(prefixUpper);
        എങ്കിൽ (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } അല്ലെങ്കിൽ എങ്കിൽ (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } അല്ലെങ്കിൽ {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    തിരികെ result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(സ്ഥിരം AstCFunc* nodep, സ്ഥിരം Ast_കണ്ണി_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    എങ്കിൽ (nodep->isConstructor()) {
        name += "init";
    } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    തിരികെ name;
}

string EmitCBaseVisitorConst::cFuncArgs(സ്ഥിരം AstCFunc* nodep, തർക്കം cpp) {
    // Return argument list for given C function
    string args;
    എങ്കിൽ (nodep->isLoose() && !nodep->isStatic()) {
        എങ്കിൽ (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    എങ്കിൽ (nodep->needProcess()) {
        എങ്കിൽ (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    എങ്കിൽ (!nodep->argTypes().empty()) {
        എങ്കിൽ (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
            എങ്കിൽ (portp->isIO() && !portp->isFuncReturn()) {
                എങ്കിൽ (!args.empty()) args += ", ";
                എങ്കിൽ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(സത്യം, അസത്യം, cpp);
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->funcPublic()) {
                    args += portp->cPubArgType(സത്യം, അസത്യം);
                } അല്ലെങ്കിൽ {
                    args += portp->vlArgType(സത്യം, അസത്യം, സത്യം);
                }
            }
        }
    }
    തിരികെ args;
}

ശൂന്യം EmitCBaseVisitorConst::emitCDefaultConstructor(സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

ശൂന്യം EmitCBaseVisitorConst::emitCFuncHeader(സ്ഥിരം AstCFunc* funcp, സ്ഥിരം Ast_കണ്ണി_Module* modp,
                                            തർക്കം withScope) {
    എങ്കിൽ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    എങ്കിൽ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        തിരികെ;
    }
    എങ്കിൽ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    എങ്കിൽ (withScope) {
        എങ്കിൽ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::മുകൾ_Class_Name() + "::");
        } അല്ലെങ്കിൽ എങ്കിൽ (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    എങ്കിൽ (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

ശൂന്യം EmitCBaseVisitorConst::emitCFuncDecl(സ്ഥിരം AstCFunc* funcp, സ്ഥിരം Ast_കണ്ണി_Module* modp,
                                          തർക്കം cLinkage) {
    ensure_New_വരി();
    എങ്കിൽ (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    എങ്കിൽ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    എങ്കിൽ (cLinkage) putns(funcp, "extern \"C\" ");
    എങ്കിൽ (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    എങ്കിൽ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ അസത്യം);
    സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(modp, Class);
    എങ്കിൽ (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } അല്ലെങ്കിൽ {
        putns(funcp, ";\n");
    }
    എങ്കിൽ (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, സ്ഥിരം std::string& prefix,
                                                      സ്ഥിരം std::string& params,
                                                      സ്ഥിരം std::string& args,  //
                                                      size_t maxCost, തർക്കം support,
                                                      സ്ഥിരം അക്ഷരം* descriptionp,
                                                      സ്ഥിരം std::function<ശൂന്യം()>& emitHeader) {
    സ്ഥിരം auto subFuncName = [&](size_t n) { തിരികെ prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    തർക്കം callsSubFuncs = അസത്യം;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    ആകുമ്പോൾ (stmts.size() > 1) {
        സ്ഥിരം size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        സ്ഥിരം size_t nStmts = stmts.size();
        വേണ്ടി (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            ആകുമ്പോൾ (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, സത്യം, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            എങ്കിൽ (callsSubFuncs) {
                puts("\n");
                വേണ്ടി (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            വേണ്ടി (size_t j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = സത്യം;
        calleeBase = levelBase;
    }
    തിരികെ funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, സ്ഥിരം std::string& prefix,
    സ്ഥിരം std::string& params, സ്ഥിരം std::string& args, തർക്കം support, സ്ഥിരം അക്ഷരം* descriptionp,
    സ്ഥിരം std::function<ശൂന്യം()>& emitHeader) {
    സ്ഥിരം size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    എങ്കിൽ (!maxCost) തിരികെ {};
    size_t totalCost = baseCost;
    വേണ്ടി (സ്ഥിരം TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    വേണ്ടി (സ്ഥിരം TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    എങ്കിൽ (totalCost <= maxCost) തിരികെ {};
    v3Global.useParallelBuild(സത്യം);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    തിരികെ {std::move(ctorDecl), std::move(dtorDecl)};
}

ശൂന്യം EmitCBaseVisitorConst::emitVarDecl(സ്ഥിരം AstVar* nodep, തർക്കം asRef) {
    സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = nodep->basicp();
    സ്ഥിരം തർക്കം refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    എങ്കിൽ (nodep->mtask_Cache_വരി_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    എങ്കിൽ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", അസത്യം, അസത്യം));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        തിരികെ;
    }

    സ്ഥിരം auto emitDeclArrayBrackets = [ഇത്](സ്ഥിരം AstVar* nodep) -> ശൂന്യം {
        // This isn't very robust and may need cleanup for other data types
        വേണ്ടി (സ്ഥിരം AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    എങ്കിൽ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        എങ്കിൽ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        എങ്കിൽ (asRef) {
            എങ്കിൽ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        എങ്കിൽ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isVLIO()) {
        എങ്കിൽ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unknown type");
        }

        എങ്കിൽ (nodep->isQuad()) {
            puts("64");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->widthMin() <= 8) {
            puts("8");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->widthMin() <= 16) {
            puts("16");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        എങ്കിൽ (asRef) {
            എങ്കിൽ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        എങ്കിൽ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        എങ്കിൽ (nodep->isWide()) puts("," + cvtToStr(nodep->width_വാക്കുകൾ()));
        puts(");\n");
    } അല്ലെങ്കിൽ {
        // Strings and other fundamental C types
        എങ്കിൽ (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            സ്ഥിരം string name = nodep->name();
            സ്ഥിരം string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            സ്ഥിരം തർക്കം beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            എങ്കിൽ (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(സത്യം, അസത്യം, അസത്യം, asRef));
        puts(";\n");
    }
}

ശൂന്യം EmitCBaseVisitorConst::emitVarAccessors(സ്ഥിരം AstVar* nodep) {
    പ്രതിജ്ഞാ(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    സ്ഥിരം string privateName = nodep->name();
    സ്ഥിരം string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

ശൂന്യം EmitCBaseVisitorConst::emitModCUse(സ്ഥിരം Ast_കണ്ണി_Module* modp, VUseType useType) {
    തർക്കം nl = അസത്യം;
    forModCUse(modp, useType, [&](സ്ഥിരം string& entry) {
        puts(entry);
        nl = സത്യം;
    });
    എങ്കിൽ (nl) puts("\n");
}

std::pair<string, File_വരി*> EmitCBaseVisitorConst::scSection(സ്ഥിരം Ast_കണ്ണി_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_വരി* fl = nullptr;
    പൂർണ്ണസംഖ്യ last_വരി = -999;
    വേണ്ടി (സ്ഥിരം AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        എങ്കിൽ (scSectionp->sectionType() != type) തുടരുക;
        എങ്കിൽ (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            എങ്കിൽ (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        എങ്കിൽ (last_വരി + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_വരി = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    എങ്കിൽ (!text.empty()) {
        എങ്കിൽ (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        ആകുമ്പോൾ ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    തിരികെ std::make_pair(text, fl);
}

ശൂന്യം EmitCBaseVisitorConst::emitSystemCSection(സ്ഥിരം Ast_കണ്ണി_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    എങ്കിൽ (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
