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
    ثابت auto setAll = [](AstNodeModule* modp) -> فراغ {
        لكل (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            𐡄𐡍 (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, NodeCoverDecl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](AstNodeCoverDecl* nodep) { nodep->user4p(modp); });
    };
    لكل (AstNode* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, NodeModule));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(ثابت AstNode* nodep) VL_MT_STABLE {
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
    𐡄𐡍 (mit != s_memoized.end()) أرجع mit->second;
    //
    // Check capitalization
    ثابت string prefixUpper = VString::upcase(prefix);
    string result;
    {
        ثابت auto it = s_ucToPrefix.find(prefixUpper);
        𐡄𐡍 (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } وإلا 𐡄𐡍 (it->second == prefix) {
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

string EmitCBaseVisitorConst::funcNameProtect(ثابت AstCFunc* nodep, ثابت AstNodeModule* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    𐡄𐡍 (nodep->isConstructor()) {
        name += "init";
    } وإلا 𐡄𐡍 (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } وإلا {
        𐡄𐡍 (nodep->isLoose()) {
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
    𐡄𐡍 (nodep->isLoose() && !nodep->isStatic()) {
        𐡄𐡍 (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } وإلا {
            𐡄𐡍 (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    𐡄𐡍 (nodep->needProcess()) {
        𐡄𐡍 (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    𐡄𐡍 (!nodep->argTypes().empty()) {
        𐡄𐡍 (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    لكل (ثابت AstNode* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        𐡄𐡍 (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            𐡄𐡍 (portp->isIO() && !portp->isFuncReturn()) {
                𐡄𐡍 (!args.empty()) args += ", ";
                𐡄𐡍 (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(𐡒𐡔𐡈, خطأ, cpp);
                } وإلا 𐡄𐡍 (nodep->funcPublic()) {
                    args += portp->cPubArgType(𐡒𐡔𐡈, خطأ);
                } وإلا {
                    args += portp->vlArgType(𐡒𐡔𐡈, خطأ, 𐡒𐡔𐡈);
                }
            }
        }
    }
    أرجع args;
}

فراغ EmitCBaseVisitorConst::emitCDefaultConstructor(ثابت AstNodeModule* ثابت modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

فراغ EmitCBaseVisitorConst::emitCFuncHeader(ثابت AstCFunc* funcp, ثابت AstNodeModule* modp,
                                            منطقي withScope) {
    𐡄𐡍 (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    𐡄𐡍 (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        أرجع;
    }
    𐡄𐡍 (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    𐡄𐡍 (withScope) {
        𐡄𐡍 (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::topClassName() + "::");
        } وإلا 𐡄𐡍 (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    𐡄𐡍 (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

فراغ EmitCBaseVisitorConst::emitCFuncDecl(ثابت AstCFunc* funcp, ثابت AstNodeModule* modp,
                                          منطقي cLinkage) {
    ensureNewLine();
    𐡄𐡍 (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    𐡄𐡍 (funcp->isConstructor()) emitCDefaultConstructor(modp);
    𐡄𐡍 (cLinkage) putns(funcp, "extern \"C\" ");
    𐡄𐡍 (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    𐡄𐡍 (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ خطأ);
    ثابت AstClass* ثابت classp = VN_CAST(modp, Class);
    𐡄𐡍 (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } وإلا {
        putns(funcp, ";\n");
    }
    𐡄𐡍 (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
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
            openNewOutputSourceFile(funcName, 𐡒𐡔𐡈, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            𐡄𐡍 (callsSubFuncs) {
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
        callsSubFuncs = 𐡒𐡔𐡈;
        calleeBase = levelBase;
    }
    أرجع funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, ثابت std::string& prefix,
    ثابت std::string& params, ثابت std::string& args, منطقي support, ثابت حرف* descriptionp,
    ثابت std::function<فراغ()>& emitHeader) {
    ثابت size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    𐡄𐡍 (!maxCost) أرجع {};
    size_t totalCost = baseCost;
    لكل (ثابت TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    لكل (ثابت TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    𐡄𐡍 (totalCost <= maxCost) أرجع {};
    v3Global.useParallelBuild(𐡒𐡔𐡈);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    أرجع {std::move(ctorDecl), std::move(dtorDecl)};
}

فراغ EmitCBaseVisitorConst::emitVarDecl(ثابت AstVar* nodep, منطقي asRef) {
    ثابت AstBasicDType* ثابت basicp = nodep->basicp();
    ثابت منطقي refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    𐡄𐡍 (nodep->mtaskCacheLineAlign() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    𐡄𐡍 (nodep->covergroupRefMember()) {
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

    𐡄𐡍 (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        𐡄𐡍 (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } وإلا 𐡄𐡍 (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } وإلا 𐡄𐡍 (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } وإلا {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        𐡄𐡍 (asRef) {
            𐡄𐡍 (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        𐡄𐡍 (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } وإلا 𐡄𐡍 (nodep->isVLIO()) {
        𐡄𐡍 (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } وإلا 𐡄𐡍 (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } وإلا 𐡄𐡍 (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } وإلا {
            nodep->v3fatalSrc("Unknown type");
        }

        𐡄𐡍 (nodep->isQuad()) {
            puts("64");
        } وإلا 𐡄𐡍 (nodep->widthMin() <= 8) {
            puts("8");
        } وإلا 𐡄𐡍 (nodep->widthMin() <= 16) {
            puts("16");
        } وإلا 𐡄𐡍 (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        𐡄𐡍 (asRef) {
            𐡄𐡍 (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        𐡄𐡍 (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        𐡄𐡍 (nodep->isWide()) puts("," + cvtToStr(nodep->widthWords()));
        puts(");\n");
    } وإلا {
        // Strings and other fundamental C types
        𐡄𐡍 (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            ثابت string name = nodep->name();
            ثابت string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            ثابت منطقي beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            𐡄𐡍 (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(𐡒𐡔𐡈, خطأ, خطأ, asRef));
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

فراغ EmitCBaseVisitorConst::emitModCUse(ثابت AstNodeModule* modp, VUseType useType) {
    منطقي nl = خطأ;
    forModCUse(modp, useType, [&](ثابت string& entry) {
        puts(entry);
        nl = 𐡒𐡔𐡈;
    });
    𐡄𐡍 (nl) puts("\n");
}

std::pair<string, FileLine*> EmitCBaseVisitorConst::scSection(ثابت AstNodeModule* modp,
                                                              VSystemCSectionType type) {
    string text;
    FileLine* fl = nullptr;
    صحيح last_line = -999;
    لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        𐡄𐡍 (scSectionp->sectionType() != type) استمر;
        𐡄𐡍 (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            𐡄𐡍 (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        𐡄𐡍 (last_line + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_line = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    𐡄𐡍 (!text.empty()) {
        𐡄𐡍 (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
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

فراغ EmitCBaseVisitorConst::emitSystemCSection(ثابت AstNodeModule* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    𐡄𐡍 (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
