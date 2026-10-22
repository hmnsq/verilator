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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitCBase.h"

#अन्तर्भाव "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ध्रुव auto setAll = [](Ast_ग्रन्थिः_Module* modp) -> शून्य {
        अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ग्रन्थिः_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ग्रन्थिः_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    अत्थाय (Ast_ग्रन्थिः* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, ग्रन्थिः_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(ध्रुव Ast_ग्रन्थिः* nodep) VL_MT_STABLE {
    ध्रुव string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    स्थिर V3Mutex s_mutex;
    ध्रुव V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    स्थिर std::map<std::string, std::string> s_memoized;
    स्थिर std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    ध्रुव auto mit = s_memoized.find(prefix);
    सचे (mit != s_memoized.end()) पटिनिवत्त mit->second;
    //
    // Check capitalization
    ध्रुव string prefixUpper = VString::upcase(prefix);
    string result;
    {
        ध्रुव auto it = s_ucToPrefix.find(prefixUpper);
        सचे (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } अञ्ञथा सचे (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } अञ्ञथा {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    पटिनिवत्त result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ध्रुव AstCFunc* nodep, ध्रुव Ast_ग्रन्थिः_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    सचे (nodep->isConstructor()) {
        name += "init";
    } अञ्ञथा सचे (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } अञ्ञथा {
        सचे (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    पटिनिवत्त name;
}

string EmitCBaseVisitorConst::cFuncArgs(ध्रुव AstCFunc* nodep, तार्किक cpp) {
    // Return argument list for given C function
    string args;
    सचे (nodep->isLoose() && !nodep->isStatic()) {
        सचे (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } अञ्ञथा {
            सचे (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    सचे (nodep->needProcess()) {
        सचे (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    सचे (!nodep->argTypes().empty()) {
        सचे (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    अत्थाय (ध्रुव Ast_ग्रन्थिः* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        सचे (ध्रुव AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
            सचे (portp->isIO() && !portp->isFuncReturn()) {
                सचे (!args.empty()) args += ", ";
                सचे (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(सच्च, मिच्छा, cpp);
                } अञ्ञथा सचे (nodep->funcPublic()) {
                    args += portp->cPubArgType(सच्च, मिच्छा);
                } अञ्ञथा {
                    args += portp->vlArgType(सच्च, मिच्छा, सच्च);
                }
            }
        }
    }
    पटिनिवत्त args;
}

शून्य EmitCBaseVisitorConst::emitCDefaultConstructor(ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

शून्य EmitCBaseVisitorConst::emitCFuncHeader(ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp,
                                            तार्किक withScope) {
    सचे (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    सचे (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        पटिनिवत्त;
    }
    सचे (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    सचे (withScope) {
        सचे (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::शिखरम्_Class_Name() + "::");
        } अञ्ञथा सचे (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    सचे (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

शून्य EmitCBaseVisitorConst::emitCFuncDecl(ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp,
                                          तार्किक cLinkage) {
    ensure_New_पङ्क्तिः();
    सचे (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    सचे (funcp->isConstructor()) emitCDefaultConstructor(modp);
    सचे (cLinkage) putns(funcp, "extern \"C\" ");
    सचे (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    सचे (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ मिच्छा);
    ध्रुव AstClass* ध्रुव classp = VN_CAST(modp, Class);
    सचे (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } अञ्ञथा {
        putns(funcp, ";\n");
    }
    सचे (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, ध्रुव std::string& prefix,
                                                      ध्रुव std::string& params,
                                                      ध्रुव std::string& args,  //
                                                      size_t maxCost, तार्किक support,
                                                      ध्रुव वर्ण* descriptionp,
                                                      ध्रुव std::function<शून्य()>& emitHeader) {
    ध्रुव auto subFuncName = [&](size_t n) { पटिनिवत्त prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    तार्किक callsSubFuncs = मिच्छा;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    याव (stmts.size() > 1) {
        ध्रुव size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        ध्रुव size_t nStmts = stmts.size();
        अत्थाय (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            याव (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, सच्च, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            सचे (callsSubFuncs) {
                puts("\n");
                अत्थाय (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            अत्थाय (size_t j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = सच्च;
        calleeBase = levelBase;
    }
    पटिनिवत्त funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, ध्रुव std::string& prefix,
    ध्रुव std::string& params, ध्रुव std::string& args, तार्किक support, ध्रुव वर्ण* descriptionp,
    ध्रुव std::function<शून्य()>& emitHeader) {
    ध्रुव size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    सचे (!maxCost) पटिनिवत्त {};
    size_t totalCost = baseCost;
    अत्थाय (ध्रुव TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    अत्थाय (ध्रुव TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    सचे (totalCost <= maxCost) पटिनिवत्त {};
    v3Global.useParallelBuild(सच्च);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    पटिनिवत्त {std::move(ctorDecl), std::move(dtorDecl)};
}

शून्य EmitCBaseVisitorConst::emitVarDecl(ध्रुव AstVar* nodep, तार्किक asRef) {
    ध्रुव AstBasicDType* ध्रुव basicp = nodep->basicp();
    ध्रुव तार्किक refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    सचे (nodep->mtask_Cache_पङ्क्तिः_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    सचे (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", मिच्छा, मिच्छा));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        पटिनिवत्त;
    }

    ध्रुव auto emitDeclArrayBrackets = [एतत्](ध्रुव AstVar* nodep) -> शून्य {
        // This isn't very robust and may need cleanup for other data types
        अत्थाय (ध्रुव AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    सचे (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        सचे (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } अञ्ञथा सचे (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } अञ्ञथा सचे (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } अञ्ञथा {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        सचे (asRef) {
            सचे (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        सचे (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } अञ्ञथा सचे (nodep->isVLIO()) {
        सचे (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } अञ्ञथा सचे (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } अञ्ञथा सचे (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } अञ्ञथा {
            nodep->v3fatalSrc("Unknown type");
        }

        सचे (nodep->isQuad()) {
            puts("64");
        } अञ्ञथा सचे (nodep->widthMin() <= 8) {
            puts("8");
        } अञ्ञथा सचे (nodep->widthMin() <= 16) {
            puts("16");
        } अञ्ञथा सचे (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        सचे (asRef) {
            सचे (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        सचे (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        सचे (nodep->isWide()) puts("," + cvtToStr(nodep->width_शब्दाः()));
        puts(");\n");
    } अञ्ञथा {
        // Strings and other fundamental C types
        सचे (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            ध्रुव string name = nodep->name();
            ध्रुव string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ध्रुव तार्किक beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            सचे (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(सच्च, मिच्छा, मिच्छा, asRef));
        puts(";\n");
    }
}

शून्य EmitCBaseVisitorConst::emitVarAccessors(ध्रुव AstVar* nodep) {
    प्रतिज्ञा(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ध्रुव string privateName = nodep->name();
    ध्रुव string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

शून्य EmitCBaseVisitorConst::emitModCUse(ध्रुव Ast_ग्रन्थिः_Module* modp, VUseType useType) {
    तार्किक nl = मिच्छा;
    forModCUse(modp, useType, [&](ध्रुव string& entry) {
        puts(entry);
        nl = सच्च;
    });
    सचे (nl) puts("\n");
}

std::pair<string, File_पङ्क्तिः*> EmitCBaseVisitorConst::scSection(ध्रुव Ast_ग्रन्थिः_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_पङ्क्तिः* fl = nullptr;
    पूर्णाङ्क last_पङ्क्तिः = -999;
    अत्थाय (ध्रुव AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        सचे (scSectionp->sectionType() != type) अनुवर्त;
        सचे (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            सचे (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        सचे (last_पङ्क्तिः + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_पङ्क्तिः = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    सचे (!text.empty()) {
        सचे (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        याव ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    पटिनिवत्त std::make_pair(text, fl);
}

शून्य EmitCBaseVisitorConst::emitSystemCSection(ध्रुव Ast_ग्रन्थिः_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    सचे (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
