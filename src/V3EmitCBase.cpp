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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3EmitCBase.h"

#অন্তর্ভাব "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    ধ্রুব auto setAll = [](Ast_গ্রন্থি_Module* modp) -> শূন্য {
        প্রতি (Ast_গ্রন্থি* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            যদি (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, গ্রন্থি_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_গ্রন্থি_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    প্রতি (Ast_গ্রন্থি* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, গ্রন্থি_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(ধ্রুব Ast_গ্রন্থি* nodep) VL_MT_STABLE {
    ধ্রুব string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    স্থির V3Mutex s_mutex;
    ধ্রুব V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    স্থির std::map<std::string, std::string> s_memoized;
    স্থির std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    ধ্রুব auto mit = s_memoized.find(prefix);
    যদি (mit != s_memoized.end()) প্রত্যাগম mit->second;
    //
    // Check capitalization
    ধ্রুব string prefixUpper = VString::upcase(prefix);
    string result;
    {
        ধ্রুব auto it = s_ucToPrefix.find(prefixUpper);
        যদি (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } অন্যথা যদি (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } অন্যথা {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    প্রত্যাগম result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(ধ্রুব AstCFunc* nodep, ধ্রুব Ast_গ্রন্থি_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    যদি (nodep->isConstructor()) {
        name += "init";
    } অন্যথা যদি (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } অন্যথা {
        যদি (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    প্রত্যাগম name;
}

string EmitCBaseVisitorConst::cFuncArgs(ধ্রুব AstCFunc* nodep, তার্কিক cpp) {
    // Return argument list for given C function
    string args;
    যদি (nodep->isLoose() && !nodep->isStatic()) {
        যদি (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } অন্যথা {
            যদি (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    যদি (nodep->needProcess()) {
        যদি (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    যদি (!nodep->argTypes().empty()) {
        যদি (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    প্রতি (ধ্রুব Ast_গ্রন্থি* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        যদি (ধ্রুব AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
            যদি (portp->isIO() && !portp->isFuncReturn()) {
                যদি (!args.empty()) args += ", ";
                যদি (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(সত্য, অসত্য, cpp);
                } অন্যথা যদি (nodep->funcPublic()) {
                    args += portp->cPubArgType(সত্য, অসত্য);
                } অন্যথা {
                    args += portp->vlArgType(সত্য, অসত্য, সত্য);
                }
            }
        }
    }
    প্রত্যাগম args;
}

শূন্য EmitCBaseVisitorConst::emitCDefaultConstructor(ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

শূন্য EmitCBaseVisitorConst::emitCFuncHeader(ধ্রুব AstCFunc* funcp, ধ্রুব Ast_গ্রন্থি_Module* modp,
                                            তার্কিক withScope) {
    যদি (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    যদি (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        প্রত্যাগম;
    }
    যদি (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    যদি (withScope) {
        যদি (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::শীর্ষ_Class_Name() + "::");
        } অন্যথা যদি (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    যদি (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

শূন্য EmitCBaseVisitorConst::emitCFuncDecl(ধ্রুব AstCFunc* funcp, ধ্রুব Ast_গ্রন্থি_Module* modp,
                                          তার্কিক cLinkage) {
    ensure_New_সারি();
    যদি (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    যদি (funcp->isConstructor()) emitCDefaultConstructor(modp);
    যদি (cLinkage) putns(funcp, "extern \"C\" ");
    যদি (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    যদি (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ অসত্য);
    ধ্রুব AstClass* ধ্রুব classp = VN_CAST(modp, Class);
    যদি (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } অন্যথা {
        putns(funcp, ";\n");
    }
    যদি (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, ধ্রুব std::string& prefix,
                                                      ধ্রুব std::string& params,
                                                      ধ্রুব std::string& args,  //
                                                      size_t maxCost, তার্কিক support,
                                                      ধ্রুব বর্ণ* descriptionp,
                                                      ধ্রুব std::function<শূন্য()>& emitHeader) {
    ধ্রুব auto subFuncName = [&](size_t n) { প্রত্যাগম prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    তার্কিক callsSubFuncs = অসত্য;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    যাবত্ (stmts.size() > 1) {
        ধ্রুব size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        ধ্রুব size_t nStmts = stmts.size();
        প্রতি (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            যাবত্ (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, সত্য, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            যদি (callsSubFuncs) {
                puts("\n");
                প্রতি (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            প্রতি (size_t j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = সত্য;
        calleeBase = levelBase;
    }
    প্রত্যাগম funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, ধ্রুব std::string& prefix,
    ধ্রুব std::string& params, ধ্রুব std::string& args, তার্কিক support, ধ্রুব বর্ণ* descriptionp,
    ধ্রুব std::function<শূন্য()>& emitHeader) {
    ধ্রুব size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    যদি (!maxCost) প্রত্যাগম {};
    size_t totalCost = baseCost;
    প্রতি (ধ্রুব TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    প্রতি (ধ্রুব TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    যদি (totalCost <= maxCost) প্রত্যাগম {};
    v3Global.useParallelBuild(সত্য);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    প্রত্যাগম {std::move(ctorDecl), std::move(dtorDecl)};
}

শূন্য EmitCBaseVisitorConst::emitVarDecl(ধ্রুব AstVar* nodep, তার্কিক asRef) {
    ধ্রুব AstBasicDType* ধ্রুব basicp = nodep->basicp();
    ধ্রুব তার্কিক refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    যদি (nodep->mtask_Cache_সারি_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    যদি (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", অসত্য, অসত্য));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        প্রত্যাগম;
    }

    ধ্রুব auto emitDeclArrayBrackets = [এতত্](ধ্রুব AstVar* nodep) -> শূন্য {
        // This isn't very robust and may need cleanup for other data types
        প্রতি (ধ্রুব AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    যদি (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        যদি (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } অন্যথা যদি (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } অন্যথা যদি (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } অন্যথা {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        যদি (asRef) {
            যদি (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        যদি (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } অন্যথা যদি (nodep->isVLIO()) {
        যদি (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } অন্যথা যদি (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } অন্যথা যদি (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } অন্যথা {
            nodep->v3fatalSrc("Unknown type");
        }

        যদি (nodep->isQuad()) {
            puts("64");
        } অন্যথা যদি (nodep->widthMin() <= 8) {
            puts("8");
        } অন্যথা যদি (nodep->widthMin() <= 16) {
            puts("16");
        } অন্যথা যদি (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        যদি (asRef) {
            যদি (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        যদি (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        যদি (nodep->isWide()) puts("," + cvtToStr(nodep->width_শব্দগুলি()));
        puts(");\n");
    } অন্যথা {
        // Strings and other fundamental C types
        যদি (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            ধ্রুব string name = nodep->name();
            ধ্রুব string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ধ্রুব তার্কিক beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            যদি (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(সত্য, অসত্য, অসত্য, asRef));
        puts(";\n");
    }
}

শূন্য EmitCBaseVisitorConst::emitVarAccessors(ধ্রুব AstVar* nodep) {
    প্রতিজ্ঞা(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    ধ্রুব string privateName = nodep->name();
    ধ্রুব string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

শূন্য EmitCBaseVisitorConst::emitModCUse(ধ্রুব Ast_গ্রন্থি_Module* modp, VUseType useType) {
    তার্কিক nl = অসত্য;
    forModCUse(modp, useType, [&](ধ্রুব string& entry) {
        puts(entry);
        nl = সত্য;
    });
    যদি (nl) puts("\n");
}

std::pair<string, File_সারি*> EmitCBaseVisitorConst::scSection(ধ্রুব Ast_গ্রন্থি_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_সারি* fl = nullptr;
    পূর্ণাঙ্ক last_সারি = -999;
    প্রতি (ধ্রুব AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        যদি (scSectionp->sectionType() != type) অনুবর্ত;
        যদি (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            যদি (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        যদি (last_সারি + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_সারি = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    যদি (!text.empty()) {
        যদি (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        যাবত্ ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    প্রত্যাগম std::make_pair(text, fl);
}

শূন্য EmitCBaseVisitorConst::emitSystemCSection(ধ্রুব Ast_গ্রন্থি_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    যদি (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
