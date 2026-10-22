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

#include "V3PchAstMT.h"

#include "V3EmitCBase.h"

#include "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    const auto setAll = [](AstNodeModule* modp) -> void {
        အတွက် (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            အကယ်၍ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, NodeCoverDecl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](AstNodeCoverDecl* nodep) { nodep->user4p(modp); });
    };
    အတွက် (AstNode* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, NodeModule));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(const AstNode* nodep) VL_MT_STABLE {
    const string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    static V3Mutex s_mutex;
    const V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    static std::map<std::string, std::string> s_memoized;
    static std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    const auto mit = s_memoized.find(prefix);
    အကယ်၍ (mit != s_memoized.end()) ပြန်ပေး mit->second;
    //
    // Check capitalization
    const string prefixUpper = VString::upcase(prefix);
    string result;
    {
        const auto it = s_ucToPrefix.find(prefixUpper);
        အကယ်၍ (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } မဟုတ်ပါက အကယ်၍ (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } မဟုတ်ပါက {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    ပြန်ပေး result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(const AstCFunc* nodep, const AstNodeModule* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    အကယ်၍ (nodep->isConstructor()) {
        name += "init";
    } မဟုတ်ပါက အကယ်၍ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } မဟုတ်ပါက {
        အကယ်၍ (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    ပြန်ပေး name;
}

string EmitCBaseVisitorConst::cFuncArgs(const AstCFunc* nodep, bool cpp) {
    // Return argument list for given C function
    string args;
    အကယ်၍ (nodep->isLoose() && !nodep->isStatic()) {
        အကယ်၍ (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } မဟုတ်ပါက {
            အကယ်၍ (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    အကယ်၍ (nodep->needProcess()) {
        အကယ်၍ (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    အကယ်၍ (!nodep->argTypes().empty()) {
        အကယ်၍ (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    အတွက် (const AstNode* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        အကယ်၍ (const AstVar* const portp = VN_CAST(stmtp, Var)) {
            အကယ်၍ (portp->isIO() && !portp->isFuncReturn()) {
                အကယ်၍ (!args.empty()) args += ", ";
                အကယ်၍ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(true, false, cpp);
                } မဟုတ်ပါက အကယ်၍ (nodep->funcPublic()) {
                    args += portp->cPubArgType(true, false);
                } မဟုတ်ပါက {
                    args += portp->vlArgType(true, false, true);
                }
            }
        }
    }
    ပြန်ပေး args;
}

void EmitCBaseVisitorConst::emitCDefaultConstructor(const AstNodeModule* const modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

void EmitCBaseVisitorConst::emitCFuncHeader(const AstCFunc* funcp, const AstNodeModule* modp,
                                            bool withScope) {
    အကယ်၍ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    အကယ်၍ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        ပြန်ပေး;
    }
    အကယ်၍ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    အကယ်၍ (withScope) {
        အကယ်၍ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::topClassName() + "::");
        } မဟုတ်ပါက အကယ်၍ (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    အကယ်၍ (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

void EmitCBaseVisitorConst::emitCFuncDecl(const AstCFunc* funcp, const AstNodeModule* modp,
                                          bool cLinkage) {
    ensureNewLine();
    အကယ်၍ (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    အကယ်၍ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    အကယ်၍ (cLinkage) putns(funcp, "extern \"C\" ");
    အကယ်၍ (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    အကယ်၍ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ false);
    const AstClass* const classp = VN_CAST(modp, Class);
    အကယ်၍ (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } မဟုတ်ပါက {
        putns(funcp, ";\n");
    }
    အကယ်၍ (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, const std::string& prefix,
                                                      const std::string& params,
                                                      const std::string& args,  //
                                                      size_t maxCost, bool support,
                                                      const var_na* descriptionp,
                                                      const std::function<void()>& emitHeader) {
    const auto subFuncName = [&](size_t n) { ပြန်ပေး prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    bool callsSubFuncs = false;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    နေစဉ် (stmts.size() > 1) {
        const size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        const size_t nStmts = stmts.size();
        အတွက် (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            နေစဉ် (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
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
            အကယ်၍ (callsSubFuncs) {
                puts("\n");
                အတွက် (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            အတွက် (size_t j = splitStart; j < splitEnd; ++j) {
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
    ပြန်ပေး funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, const std::string& prefix,
    const std::string& params, const std::string& args, bool support, const var_na* descriptionp,
    const std::function<void()>& emitHeader) {
    const size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    အကယ်၍ (!maxCost) ပြန်ပေး {};
    size_t totalCost = baseCost;
    အတွက် (const TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    အတွက် (const TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    အကယ်၍ (totalCost <= maxCost) ပြန်ပေး {};
    v3Global.useParallelBuild(true);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    ပြန်ပေး {std::move(ctorDecl), std::move(dtorDecl)};
}

void EmitCBaseVisitorConst::emitVarDecl(const AstVar* nodep, bool asRef) {
    const AstBasicDType* const basicp = nodep->basicp();
    const bool refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    အကယ်၍ (nodep->mtaskCacheLineAlign() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    အကယ်၍ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", false, false));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        ပြန်ပေး;
    }

    const auto emitDeclArrayBrackets = [this](const AstVar* nodep) -> void {
        // This isn't very robust and may need cleanup for other data types
        အတွက် (const AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    အကယ်၍ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        အကယ်၍ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } မဟုတ်ပါက အကယ်၍ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } မဟုတ်ပါက အကယ်၍ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } မဟုတ်ပါက {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        အကယ်၍ (asRef) {
            အကယ်၍ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        အကယ်၍ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } မဟုတ်ပါက အကယ်၍ (nodep->isVLIO()) {
        အကယ်၍ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } မဟုတ်ပါက အကယ်၍ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } မဟုတ်ပါက အကယ်၍ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } မဟုတ်ပါက {
            nodep->v3fatalSrc("Unknown type");
        }

        အကယ်၍ (nodep->isQuad()) {
            puts("64");
        } မဟုတ်ပါက အကယ်၍ (nodep->widthMin() <= 8) {
            puts("8");
        } မဟုတ်ပါက အကယ်၍ (nodep->widthMin() <= 16) {
            puts("16");
        } မဟုတ်ပါက အကယ်၍ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        အကယ်၍ (asRef) {
            အကယ်၍ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        အကယ်၍ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        အကယ်၍ (nodep->isWide()) puts("," + cvtToStr(nodep->widthWords()));
        puts(");\n");
    } မဟုတ်ပါက {
        // Strings and other fundamental C types
        အကယ်၍ (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            const string name = nodep->name();
            const string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            const bool beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            အကယ်၍ (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(true, false, false, asRef));
        puts(";\n");
    }
}

void EmitCBaseVisitorConst::emitVarAccessors(const AstVar* nodep) {
    assert(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    const string privateName = nodep->name();
    const string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

void EmitCBaseVisitorConst::emitModCUse(const AstNodeModule* modp, VUseType useType) {
    bool nl = false;
    forModCUse(modp, useType, [&](const string& entry) {
        puts(entry);
        nl = true;
    });
    အကယ်၍ (nl) puts("\n");
}

std::pair<string, FileLine*> EmitCBaseVisitorConst::scSection(const AstNodeModule* modp,
                                                              VSystemCSectionType type) {
    string text;
    FileLine* fl = nullptr;
    ကိန်းပြည့် last_line = -999;
    အတွက် (const AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        အကယ်၍ (scSectionp->sectionType() != type) continue;
        အကယ်၍ (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            အကယ်၍ (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        အကယ်၍ (last_line + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_line = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    အကယ်၍ (!text.empty()) {
        အကယ်၍ (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        နေစဉ် ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    ပြန်ပေး std::make_pair(text, fl);
}

void EmitCBaseVisitorConst::emitSystemCSection(const AstNodeModule* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    အကယ်၍ (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
