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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"

#ଅନ୍ତର୍ଭାଵ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ଅଚଳ auto setAll = [](Ast_ଗଣ୍ଠି_Module* modp) -> ଶୂନ୍ୟ {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ଯଦି (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ଗଣ୍ଠି_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ଗଣ୍ଠି_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    ପାଇଁ (Ast_ଗଣ୍ଠି* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, ଗଣ୍ଠି_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) VL_MT_STABLE {
    ଅଚଳ string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ସ୍ଥିର V3Mutex s_mutex;
    ଅଚଳ V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    ସ୍ଥିର std::map<std::string, std::string> s_memoized;
    ସ୍ଥିର std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    ଅଚଳ auto mit = s_memoized.find(prefix);
    ଯଦି (mit != s_memoized.end()) ଫେରନ୍ତୁ mit->second;
    //
    // Check capitalization
    ଅଚଳ string prefixUpper = VString::upcase(prefix);
    string result;
    {
        ଅଚଳ auto it = s_ucToPrefix.find(prefixUpper);
        ଯଦି (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } ଅନ୍ୟଥା ଯଦି (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } ଅନ୍ୟଥା {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    ଫେରନ୍ତୁ result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ଅଚଳ AstCFunc* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    ଯଦି (nodep->isConstructor()) {
        name += "init";
    } ଅନ୍ୟଥା ଯଦି (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } ଅନ୍ୟଥା {
        ଯଦି (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    ଫେରନ୍ତୁ name;
}

string EmitCBaseVisitorConst::cFuncArgs(ଅଚଳ AstCFunc* nodep, ତାର୍କିକ cpp) {
    // Return argument list for given C function
    string args;
    ଯଦି (nodep->isLoose() && !nodep->isStatic()) {
        ଯଦି (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    ଯଦି (nodep->needProcess()) {
        ଯଦି (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    ଯଦି (!nodep->argTypes().empty()) {
        ଯଦି (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
            ଯଦି (portp->isIO() && !portp->isFuncReturn()) {
                ଯଦି (!args.empty()) args += ", ";
                ଯଦି (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(ସତ୍ୟ, ମିଥ୍ୟା, cpp);
                } ଅନ୍ୟଥା ଯଦି (nodep->funcPublic()) {
                    args += portp->cPubArgType(ସତ୍ୟ, ମିଥ୍ୟା);
                } ଅନ୍ୟଥା {
                    args += portp->vlArgType(ସତ୍ୟ, ମିଥ୍ୟା, ସତ୍ୟ);
                }
            }
        }
    }
    ଫେରନ୍ତୁ args;
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitCDefaultConstructor(ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitCFuncHeader(ଅଚଳ AstCFunc* funcp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp,
                                            ତାର୍କିକ withScope) {
    ଯଦି (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    ଯଦି (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        ଫେରନ୍ତୁ;
    }
    ଯଦି (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    ଯଦି (withScope) {
        ଯଦି (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::ଶୀର୍ଷ_Class_Name() + "::");
        } ଅନ୍ୟଥା ଯଦି (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    ଯଦି (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitCFuncDecl(ଅଚଳ AstCFunc* funcp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp,
                                          ତାର୍କିକ cLinkage) {
    ensure_New_ଧାଡ଼ି();
    ଯଦି (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    ଯଦି (funcp->isConstructor()) emitCDefaultConstructor(modp);
    ଯଦି (cLinkage) putns(funcp, "extern \"C\" ");
    ଯଦି (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    ଯଦି (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ ମିଥ୍ୟା);
    ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(modp, Class);
    ଯଦି (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } ଅନ୍ୟଥା {
        putns(funcp, ";\n");
    }
    ଯଦି (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, ଅଚଳ std::string& prefix,
                                                      ଅଚଳ std::string& params,
                                                      ଅଚଳ std::string& args,  //
                                                      size_t maxCost, ତାର୍କିକ support,
                                                      ଅଚଳ ଅକ୍ଷର* descriptionp,
                                                      ଅଚଳ std::function<ଶୂନ୍ୟ()>& emitHeader) {
    ଅଚଳ auto subFuncName = [&](size_t n) { ଫେରନ୍ତୁ prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    ତାର୍କିକ callsSubFuncs = ମିଥ୍ୟା;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    ଯେତେବେଳେ (stmts.size() > 1) {
        ଅଚଳ size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        ଅଚଳ size_t nStmts = stmts.size();
        ପାଇଁ (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            ଯେତେବେଳେ (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, ସତ୍ୟ, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            ଯଦି (callsSubFuncs) {
                puts("\n");
                ପାଇଁ (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            ପାଇଁ (size_t j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = ସତ୍ୟ;
        calleeBase = levelBase;
    }
    ଫେରନ୍ତୁ funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, ଅଚଳ std::string& prefix,
    ଅଚଳ std::string& params, ଅଚଳ std::string& args, ତାର୍କିକ support, ଅଚଳ ଅକ୍ଷର* descriptionp,
    ଅଚଳ std::function<ଶୂନ୍ୟ()>& emitHeader) {
    ଅଚଳ size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    ଯଦି (!maxCost) ଫେରନ୍ତୁ {};
    size_t totalCost = baseCost;
    ପାଇଁ (ଅଚଳ TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    ପାଇଁ (ଅଚଳ TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    ଯଦି (totalCost <= maxCost) ଫେରନ୍ତୁ {};
    v3Global.useParallelBuild(ସତ୍ୟ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    ଫେରନ୍ତୁ {std::move(ctorDecl), std::move(dtorDecl)};
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitVarDecl(ଅଚଳ AstVar* nodep, ତାର୍କିକ asRef) {
    ଅଚଳ AstBasicDType* ଅଚଳ basicp = nodep->basicp();
    ଅଚଳ ତାର୍କିକ refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    ଯଦି (nodep->mtask_Cache_ଧାଡ଼ି_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    ଯଦି (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", ମିଥ୍ୟା, ମିଥ୍ୟା));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        ଫେରନ୍ତୁ;
    }

    ଅଚଳ auto emitDeclArrayBrackets = [ଏହି](ଅଚଳ AstVar* nodep) -> ଶୂନ୍ୟ {
        // This isn't very robust and may need cleanup for other data types
        ପାଇଁ (ଅଚଳ AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    ଯଦି (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        ଯଦି (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } ଅନ୍ୟଥା ଯଦି (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } ଅନ୍ୟଥା ଯଦି (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        ଯଦି (asRef) {
            ଯଦି (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        ଯଦି (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } ଅନ୍ୟଥା ଯଦି (nodep->isVLIO()) {
        ଯଦି (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } ଅନ୍ୟଥା ଯଦି (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } ଅନ୍ୟଥା ଯଦି (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unknown type");
        }

        ଯଦି (nodep->isQuad()) {
            puts("64");
        } ଅନ୍ୟଥା ଯଦି (nodep->widthMin() <= 8) {
            puts("8");
        } ଅନ୍ୟଥା ଯଦି (nodep->widthMin() <= 16) {
            puts("16");
        } ଅନ୍ୟଥା ଯଦି (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        ଯଦି (asRef) {
            ଯଦି (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        ଯଦି (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        ଯଦି (nodep->isWide()) puts("," + cvtToStr(nodep->width_ଶବ୍ଦଗୁଡ଼ିକ()));
        puts(");\n");
    } ଅନ୍ୟଥା {
        // Strings and other fundamental C types
        ଯଦି (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            ଅଚଳ string name = nodep->name();
            ଅଚଳ string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ଅଚଳ ତାର୍କିକ beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            ଯଦି (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(ସତ୍ୟ, ମିଥ୍ୟା, ମିଥ୍ୟା, asRef));
        puts(";\n");
    }
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitVarAccessors(ଅଚଳ AstVar* nodep) {
    ପ୍ରତିଜ୍ଞା(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ଅଚଳ string privateName = nodep->name();
    ଅଚଳ string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitModCUse(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, VUseType useType) {
    ତାର୍କିକ nl = ମିଥ୍ୟା;
    forModCUse(modp, useType, [&](ଅଚଳ string& entry) {
        puts(entry);
        nl = ସତ୍ୟ;
    });
    ଯଦି (nl) puts("\n");
}

std::pair<string, File_ଧାଡ଼ି*> EmitCBaseVisitorConst::scSection(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_ଧାଡ଼ି* fl = nullptr;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା last_ଧାଡ଼ି = -999;
    ପାଇଁ (ଅଚଳ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        ଯଦି (scSectionp->sectionType() != type) ଜାରି;
        ଯଦି (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            ଯଦି (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        ଯଦି (last_ଧାଡ଼ି + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_ଧାଡ଼ି = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    ଯଦି (!text.empty()) {
        ଯଦି (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        ଯେତେବେଳେ ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    ଫେରନ୍ତୁ std::make_pair(text, fl);
}

ଶୂନ୍ୟ EmitCBaseVisitorConst::emitSystemCSection(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    ଯଦି (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
