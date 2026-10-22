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
        ສຳລັບ (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ຖ້າ (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, NodeCoverDecl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](AstNodeCoverDecl* nodep) { nodep->user4p(modp); });
    };
    ສຳລັບ (AstNode* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
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
    ຖ້າ (mit != s_memoized.end()) ສົ່ງຄືນ mit->second;
    //
    // Check capitalization
    const string prefixUpper = VString::upcase(prefix);
    string result;
    {
        const auto it = s_ucToPrefix.find(prefixUpper);
        ຖ້າ (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } ບໍ່ດັ່ງນັ້ນ {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    ສົ່ງຄືນ result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(const AstCFunc* nodep, const AstNodeModule* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    ຖ້າ (nodep->isConstructor()) {
        name += "init";
    } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } ບໍ່ດັ່ງນັ້ນ {
        ຖ້າ (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    ສົ່ງຄືນ name;
}

string EmitCBaseVisitorConst::cFuncArgs(const AstCFunc* nodep, bool cpp) {
    // Return argument list for given C function
    string args;
    ຖ້າ (nodep->isLoose() && !nodep->isStatic()) {
        ຖ້າ (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } ບໍ່ດັ່ງນັ້ນ {
            ຖ້າ (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    ຖ້າ (nodep->needProcess()) {
        ຖ້າ (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    ຖ້າ (!nodep->argTypes().empty()) {
        ຖ້າ (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    ສຳລັບ (const AstNode* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        ຖ້າ (const AstVar* const portp = VN_CAST(stmtp, Var)) {
            ຖ້າ (portp->isIO() && !portp->isFuncReturn()) {
                ຖ້າ (!args.empty()) args += ", ";
                ຖ້າ (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(true, false, cpp);
                } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->funcPublic()) {
                    args += portp->cPubArgType(true, false);
                } ບໍ່ດັ່ງນັ້ນ {
                    args += portp->vlArgType(true, false, true);
                }
            }
        }
    }
    ສົ່ງຄືນ args;
}

void EmitCBaseVisitorConst::emitCDefaultConstructor(const AstNodeModule* const modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

void EmitCBaseVisitorConst::emitCFuncHeader(const AstCFunc* funcp, const AstNodeModule* modp,
                                            bool withScope) {
    ຖ້າ (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    ຖ້າ (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        ສົ່ງຄືນ;
    }
    ຖ້າ (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    ຖ້າ (withScope) {
        ຖ້າ (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::topClassName() + "::");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    ຖ້າ (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

void EmitCBaseVisitorConst::emitCFuncDecl(const AstCFunc* funcp, const AstNodeModule* modp,
                                          bool cLinkage) {
    ensureNewLine();
    ຖ້າ (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    ຖ້າ (funcp->isConstructor()) emitCDefaultConstructor(modp);
    ຖ້າ (cLinkage) putns(funcp, "extern \"C\" ");
    ຖ້າ (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    ຖ້າ (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ false);
    const AstClass* const classp = VN_CAST(modp, Class);
    ຖ້າ (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } ບໍ່ດັ່ງນັ້ນ {
        putns(funcp, ";\n");
    }
    ຖ້າ (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, const std::string& prefix,
                                                      const std::string& params,
                                                      const std::string& args,  //
                                                      size_t maxCost, bool support,
                                                      const var_na* descriptionp,
                                                      const std::function<void()>& emitHeader) {
    const auto subFuncName = [&](size_t n) { ສົ່ງຄືນ prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    bool callsSubFuncs = false;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    ໃນຂະນະທີ່ (stmts.size() > 1) {
        const size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        const size_t nStmts = stmts.size();
        ສຳລັບ (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            ໃນຂະນະທີ່ (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
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
            ຖ້າ (callsSubFuncs) {
                puts("\n");
                ສຳລັບ (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            ສຳລັບ (size_t j = splitStart; j < splitEnd; ++j) {
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
    ສົ່ງຄືນ funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, const std::string& prefix,
    const std::string& params, const std::string& args, bool support, const var_na* descriptionp,
    const std::function<void()>& emitHeader) {
    const size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    ຖ້າ (!maxCost) ສົ່ງຄືນ {};
    size_t totalCost = baseCost;
    ສຳລັບ (const TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    ສຳລັບ (const TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    ຖ້າ (totalCost <= maxCost) ສົ່ງຄືນ {};
    v3Global.useParallelBuild(true);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    ສົ່ງຄືນ {std::move(ctorDecl), std::move(dtorDecl)};
}

void EmitCBaseVisitorConst::emitVarDecl(const AstVar* nodep, bool asRef) {
    const AstBasicDType* const basicp = nodep->basicp();
    const bool refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    ຖ້າ (nodep->mtaskCacheLineAlign() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    ຖ້າ (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", false, false));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        ສົ່ງຄືນ;
    }

    const auto emitDeclArrayBrackets = [this](const AstVar* nodep) -> void {
        // This isn't very robust and may need cleanup for other data types
        ສຳລັບ (const AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    ຖ້າ (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        ຖ້າ (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } ບໍ່ດັ່ງນັ້ນ {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        ຖ້າ (asRef) {
            ຖ້າ (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        ຖ້າ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isVLIO()) {
        ຖ້າ (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } ບໍ່ດັ່ງນັ້ນ {
            nodep->v3fatalSrc("Unknown type");
        }

        ຖ້າ (nodep->isQuad()) {
            puts("64");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->widthMin() <= 8) {
            puts("8");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->widthMin() <= 16) {
            puts("16");
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        ຖ້າ (asRef) {
            ຖ້າ (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        ຖ້າ (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        ຖ້າ (nodep->isWide()) puts("," + cvtToStr(nodep->widthWords()));
        puts(");\n");
    } ບໍ່ດັ່ງນັ້ນ {
        // Strings and other fundamental C types
        ຖ້າ (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            const string name = nodep->name();
            const string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            const bool beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            ຖ້າ (beStatic) puts("static thread_local ");
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
    ຖ້າ (nl) puts("\n");
}

std::pair<string, FileLine*> EmitCBaseVisitorConst::scSection(const AstNodeModule* modp,
                                                              VSystemCSectionType type) {
    string text;
    FileLine* fl = nullptr;
    ຈຳນວນເຕັມ last_line = -999;
    ສຳລັບ (const AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        ຖ້າ (scSectionp->sectionType() != type) continue;
        ຖ້າ (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            ຖ້າ (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        ຖ້າ (last_line + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_line = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    ຖ້າ (!text.empty()) {
        ຖ້າ (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        ໃນຂະນະທີ່ ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    ສົ່ງຄືນ std::make_pair(text, fl);
}

void EmitCBaseVisitorConst::emitSystemCSection(const AstNodeModule* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    ຖ້າ (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
