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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

#ಅನ್ತರ್ಭಾವ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ಧ್ರುವ auto setAll = [](Ast_ಗಂಟು_Module* modp) -> ಶೂನ್ಯ {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ಯದಿ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ಗಂಟು_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ಗಂಟು_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    ಪ್ರತಿ (Ast_ಗಂಟು* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, ಗಂಟು_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(ಧ್ರುವ Ast_ಗಂಟು* nodep) VL_MT_STABLE {
    ಧ್ರುವ string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    ಸ್ಥಿರ V3Mutex s_mutex;
    ಧ್ರುವ V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    ಸ್ಥಿರ std::map<std::string, std::string> s_memoized;
    ಸ್ಥಿರ std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    ಧ್ರುವ auto mit = s_memoized.find(prefix);
    ಯದಿ (mit != s_memoized.end()) ಪ್ರತ್ಯಾಗಮ mit->second;
    //
    // Check capitalization
    ಧ್ರುವ string prefixUpper = VString::upcase(prefix);
    string result;
    {
        ಧ್ರುವ auto it = s_ucToPrefix.find(prefixUpper);
        ಯದಿ (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } ಅನ್ಯಥಾ ಯದಿ (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } ಅನ್ಯಥಾ {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    ಪ್ರತ್ಯಾಗಮ result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ಧ್ರುವ AstCFunc* nodep, ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    ಯದಿ (nodep->isConstructor()) {
        name += "init";
    } ಅನ್ಯಥಾ ಯದಿ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } ಅನ್ಯಥಾ {
        ಯದಿ (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    ಪ್ರತ್ಯಾಗಮ name;
}

string EmitCBaseVisitorConst::cFuncArgs(ಧ್ರುವ AstCFunc* nodep, ತಾರ್ಕಿಕ cpp) {
    // Return argument list for given C function
    string args;
    ಯದಿ (nodep->isLoose() && !nodep->isStatic()) {
        ಯದಿ (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    ಯದಿ (nodep->needProcess()) {
        ಯದಿ (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    ಯದಿ (!nodep->argTypes().empty()) {
        ಯದಿ (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
            ಯದಿ (portp->isIO() && !portp->isFuncReturn()) {
                ಯದಿ (!args.empty()) args += ", ";
                ಯದಿ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(ಸತ್ಯ, ಅಸತ್ಯ, cpp);
                } ಅನ್ಯಥಾ ಯದಿ (nodep->funcPublic()) {
                    args += portp->cPubArgType(ಸತ್ಯ, ಅಸತ್ಯ);
                } ಅನ್ಯಥಾ {
                    args += portp->vlArgType(ಸತ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ);
                }
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ args;
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitCDefaultConstructor(ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitCFuncHeader(ಧ್ರುವ AstCFunc* funcp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp,
                                            ತಾರ್ಕಿಕ withScope) {
    ಯದಿ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    ಯದಿ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಯದಿ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    ಯದಿ (withScope) {
        ಯದಿ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::ತುದಿ_Class_Name() + "::");
        } ಅನ್ಯಥಾ ಯದಿ (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    ಯದಿ (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitCFuncDecl(ಧ್ರುವ AstCFunc* funcp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp,
                                          ತಾರ್ಕಿಕ cLinkage) {
    ensure_New_ಸಾಲು();
    ಯದಿ (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    ಯದಿ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    ಯದಿ (cLinkage) putns(funcp, "extern \"C\" ");
    ಯದಿ (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    ಯದಿ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ ಅಸತ್ಯ);
    ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(modp, Class);
    ಯದಿ (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } ಅನ್ಯಥಾ {
        putns(funcp, ";\n");
    }
    ಯದಿ (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, ಧ್ರುವ std::string& prefix,
                                                      ಧ್ರುವ std::string& params,
                                                      ಧ್ರುವ std::string& args,  //
                                                      size_t maxCost, ತಾರ್ಕಿಕ support,
                                                      ಧ್ರುವ ವರ್ಣ* descriptionp,
                                                      ಧ್ರುವ std::function<ಶೂನ್ಯ()>& emitHeader) {
    ಧ್ರುವ auto subFuncName = [&](size_t n) { ಪ್ರತ್ಯಾಗಮ prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    ತಾರ್ಕಿಕ callsSubFuncs = ಅಸತ್ಯ;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    ಯಾವತ್ (stmts.size() > 1) {
        ಧ್ರುವ size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        ಧ್ರುವ size_t nStmts = stmts.size();
        ಪ್ರತಿ (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            ಯಾವತ್ (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, ಸತ್ಯ, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            ಯದಿ (callsSubFuncs) {
                puts("\n");
                ಪ್ರತಿ (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            ಪ್ರತಿ (size_t j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = ಸತ್ಯ;
        calleeBase = levelBase;
    }
    ಪ್ರತ್ಯಾಗಮ funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, ಧ್ರುವ std::string& prefix,
    ಧ್ರುವ std::string& params, ಧ್ರುವ std::string& args, ತಾರ್ಕಿಕ support, ಧ್ರುವ ವರ್ಣ* descriptionp,
    ಧ್ರುವ std::function<ಶೂನ್ಯ()>& emitHeader) {
    ಧ್ರುವ size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    ಯದಿ (!maxCost) ಪ್ರತ್ಯಾಗಮ {};
    size_t totalCost = baseCost;
    ಪ್ರತಿ (ಧ್ರುವ TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    ಪ್ರತಿ (ಧ್ರುವ TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    ಯದಿ (totalCost <= maxCost) ಪ್ರತ್ಯಾಗಮ {};
    v3Global.useParallelBuild(ಸತ್ಯ);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    ಪ್ರತ್ಯಾಗಮ {std::move(ctorDecl), std::move(dtorDecl)};
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitVarDecl(ಧ್ರುವ AstVar* nodep, ತಾರ್ಕಿಕ asRef) {
    ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = nodep->basicp();
    ಧ್ರುವ ತಾರ್ಕಿಕ refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    ಯದಿ (nodep->mtask_Cache_ಸಾಲು_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    ಯದಿ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", ಅಸತ್ಯ, ಅಸತ್ಯ));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಧ್ರುವ auto emitDeclArrayBrackets = [ಏತತ್](ಧ್ರುವ AstVar* nodep) -> ಶೂನ್ಯ {
        // This isn't very robust and may need cleanup for other data types
        ಪ್ರತಿ (ಧ್ರುವ AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    ಯದಿ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        ಯದಿ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        ಯದಿ (asRef) {
            ಯದಿ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        ಯದಿ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } ಅನ್ಯಥಾ ಯದಿ (nodep->isVLIO()) {
        ಯದಿ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unknown type");
        }

        ಯದಿ (nodep->isQuad()) {
            puts("64");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->widthMin() <= 8) {
            puts("8");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->widthMin() <= 16) {
            puts("16");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        ಯದಿ (asRef) {
            ಯದಿ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        ಯದಿ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        ಯದಿ (nodep->isWide()) puts("," + cvtToStr(nodep->width_ಪದಗಳು()));
        puts(");\n");
    } ಅನ್ಯಥಾ {
        // Strings and other fundamental C types
        ಯದಿ (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            ಧ್ರುವ string name = nodep->name();
            ಧ್ರುವ string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ಧ್ರುವ ತಾರ್ಕಿಕ beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            ಯದಿ (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(ಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ, asRef));
        puts(";\n");
    }
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitVarAccessors(ಧ್ರುವ AstVar* nodep) {
    ಪ್ರತಿಜ್ಞಾ(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ಧ್ರುವ string privateName = nodep->name();
    ಧ್ರುವ string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitModCUse(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, VUseType useType) {
    ತಾರ್ಕಿಕ nl = ಅಸತ್ಯ;
    forModCUse(modp, useType, [&](ಧ್ರುವ string& entry) {
        puts(entry);
        nl = ಸತ್ಯ;
    });
    ಯದಿ (nl) puts("\n");
}

std::pair<string, File_ಸಾಲು*> EmitCBaseVisitorConst::scSection(ಧ್ರುವ Ast_ಗಂಟು_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_ಸಾಲು* fl = nullptr;
    ಪೂರ್ಣಾಙ್ಕ last_ಸಾಲು = -999;
    ಪ್ರತಿ (ಧ್ರುವ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        ಯದಿ (scSectionp->sectionType() != type) ಅನುವರ್ತ;
        ಯದಿ (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            ಯದಿ (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        ಯದಿ (last_ಸಾಲು + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_ಸಾಲು = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    ಯದಿ (!text.empty()) {
        ಯದಿ (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        ಯಾವತ್ ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    ಪ್ರತ್ಯಾಗಮ std::make_pair(text, fl);
}

ಶೂನ್ಯ EmitCBaseVisitorConst::emitSystemCSection(ಧ್ರುವ Ast_ಗಂಟು_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    ಯದಿ (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
