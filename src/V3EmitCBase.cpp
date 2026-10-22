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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitCBase.h"

#અન્તર્ભાવ "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    અચલ auto setAll = [](Ast_ગાંઠ_Module* modp) -> શૂન્ય {
        માટે (Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, ગાંઠ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_ગાંઠ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    માટે (Ast_ગાંઠ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, ગાંઠ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(અચલ Ast_ગાંઠ* nodep) VL_MT_STABLE {
    અચલ string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    સ્થિર V3Mutex s_mutex;
    અચલ V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    સ્થિર std::map<std::string, std::string> s_memoized;
    સ્થિર std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    અચલ auto mit = s_memoized.find(prefix);
    જો (mit != s_memoized.end()) પાછા mit->second;
    //
    // Check capitalization
    અચલ string prefixUpper = VString::upcase(prefix);
    string result;
    {
        અચલ auto it = s_ucToPrefix.find(prefixUpper);
        જો (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } નહીંતર જો (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } નહીંતર {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    પાછા result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(અચલ AstCFunc* nodep, અચલ Ast_ગાંઠ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    જો (nodep->isConstructor()) {
        name += "init";
    } નહીંતર જો (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } નહીંતર {
        જો (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    પાછા name;
}

string EmitCBaseVisitorConst::cFuncArgs(અચલ AstCFunc* nodep, તાર્કિક cpp) {
    // Return argument list for given C function
    string args;
    જો (nodep->isLoose() && !nodep->isStatic()) {
        જો (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } નહીંતર {
            જો (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    જો (nodep->needProcess()) {
        જો (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    જો (!nodep->argTypes().empty()) {
        જો (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    માટે (અચલ Ast_ગાંઠ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        જો (અચલ AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
            જો (portp->isIO() && !portp->isFuncReturn()) {
                જો (!args.empty()) args += ", ";
                જો (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(સાચું, ખોટું, cpp);
                } નહીંતર જો (nodep->funcPublic()) {
                    args += portp->cPubArgType(સાચું, ખોટું);
                } નહીંતર {
                    args += portp->vlArgType(સાચું, ખોટું, સાચું);
                }
            }
        }
    }
    પાછા args;
}

શૂન્ય EmitCBaseVisitorConst::emitCDefaultConstructor(અચલ Ast_ગાંઠ_Module* અચલ modp) {
    puts(EmitCUtil::prefixNameProtect(modp));
    puts("() = default;\n");
}

શૂન્ય EmitCBaseVisitorConst::emitCFuncHeader(અચલ AstCFunc* funcp, અચલ Ast_ગાંઠ_Module* modp,
                                            તાર્કિક withScope) {
    જો (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    જો (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        પાછા;
    }
    જો (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        puts(" ");
    }
    જો (withScope) {
        જો (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::ટોચ_Class_Name() + "::");
        } નહીંતર જો (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    puts("(" + cFuncArgs(funcp) + ")");
    જો (funcp->isConst().trueKnown() && funcp->isProperMethod()) puts(" const");
}

શૂન્ય EmitCBaseVisitorConst::emitCFuncDecl(અચલ AstCFunc* funcp, અચલ Ast_ગાંઠ_Module* modp,
                                          તાર્કિક cLinkage) {
    ensure_New_લીટી();
    જો (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    જો (funcp->isConstructor()) emitCDefaultConstructor(modp);
    જો (cLinkage) putns(funcp, "extern \"C\" ");
    જો (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    જો (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ ખોટું);
    અચલ AstClass* અચલ classp = VN_CAST(modp, Class);
    જો (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } નહીંતર {
        putns(funcp, ";\n");
    }
    જો (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, અચલ std::string& prefix,
                                                      અચલ std::string& params,
                                                      અચલ std::string& args,  //
                                                      size_t maxCost, તાર્કિક support,
                                                      અચલ અક્ષર* descriptionp,
                                                      અચલ std::function<શૂન્ય()>& emitHeader) {
    અચલ auto subFuncName = [&](size_t n) { પાછા prefix + "__" + std::to_string(n); };
    size_t nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    તાર્કિક callsSubFuncs = ખોટું;
    size_t calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    જ્યારે (stmts.size() > 1) {
        અચલ size_t levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        અચલ size_t nStmts = stmts.size();
        માટે (size_t splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            size_t cost = 0;
            જ્યારે (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, સાચું, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            જો (callsSubFuncs) {
                puts("\n");
                માટે (size_t j = splitStart; j < splitEnd; ++j) {
                    puts("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                puts("\n");
            }
            // Open sub-function definition in the split file
            puts("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            માટે (size_t j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = સાચું;
        calleeBase = levelBase;
    }
    પાછા funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost, અચલ std::string& prefix,
    અચલ std::string& params, અચલ std::string& args, તાર્કિક support, અચલ અક્ષર* descriptionp,
    અચલ std::function<શૂન્ય()>& emitHeader) {
    અચલ size_t maxCost = static_cast<size_t>(v3Global.opt.outputSplitCFuncs());
    જો (!maxCost) પાછા {};
    size_t totalCost = baseCost;
    માટે (અચલ TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    માટે (અચલ TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    જો (totalCost <= maxCost) પાછા {};
    v3Global.useParallelBuild(સાચું);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    પાછા {std::move(ctorDecl), std::move(dtorDecl)};
}

શૂન્ય EmitCBaseVisitorConst::emitVarDecl(અચલ AstVar* nodep, તાર્કિક asRef) {
    અચલ AstBasicDType* અચલ basicp = nodep->basicp();
    અચલ તાર્કિક refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    જો (nodep->mtask_Cache_લીટી_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    જો (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", ખોટું, ખોટું));
        puts(" const* " + nodep->nameProtect() + " = nullptr;\n");
        પાછા;
    }

    અચલ auto emitDeclArrayBrackets = [આ](અચલ AstVar* nodep) -> શૂન્ય {
        // This isn't very robust and may need cleanup for other data types
        માટે (અચલ AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    જો (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        જો (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } નહીંતર જો (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } નહીંતર જો (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } નહીંતર {
            nodep->v3fatalSrc("Unknown type");
        }
        puts(nodep->scType());
        puts("> ");
        જો (asRef) {
            જો (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        જો (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        puts(";\n");
    } નહીંતર જો (nodep->isVLIO()) {
        જો (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } નહીંતર જો (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } નહીંતર જો (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } નહીંતર {
            nodep->v3fatalSrc("Unknown type");
        }

        જો (nodep->isQuad()) {
            puts("64");
        } નહીંતર જો (nodep->widthMin() <= 8) {
            puts("8");
        } નહીંતર જો (nodep->widthMin() <= 16) {
            puts("16");
        } નહીંતર જો (nodep->isWide()) {
            puts("W");
        }

        puts("(");
        જો (asRef) {
            જો (refNeedParens) puts("(");
            puts("&");
        }
        puts(nodep->nameProtect());
        જો (asRef && refNeedParens) puts(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        puts("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        જો (nodep->isWide()) puts("," + cvtToStr(nodep->width_શબ્દો()));
        puts(");\n");
    } નહીંતર {
        // Strings and other fundamental C types
        જો (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            અચલ string name = nodep->name();
            અચલ string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            અચલ તાર્કિક beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            જો (beStatic) puts("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(સાચું, ખોટું, ખોટું, asRef));
        puts(";\n");
    }
}

શૂન્ય EmitCBaseVisitorConst::emitVarAccessors(અચલ AstVar* nodep) {
    પ્રતિજ્ઞા(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    અચલ string privateName = nodep->name();
    અચલ string publicName = nodep->name().substr(strlen("__Vm_sig_"));

    puts("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    puts("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

શૂન્ય EmitCBaseVisitorConst::emitModCUse(અચલ Ast_ગાંઠ_Module* modp, VUseType useType) {
    તાર્કિક nl = ખોટું;
    forModCUse(modp, useType, [&](અચલ string& entry) {
        puts(entry);
        nl = સાચું;
    });
    જો (nl) puts("\n");
}

std::pair<string, File_લીટી*> EmitCBaseVisitorConst::scSection(અચલ Ast_ગાંઠ_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_લીટી* fl = nullptr;
    પૂર્ણાંક last_લીટી = -999;
    માટે (અચલ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        જો (scSectionp->sectionType() != type) ચાલુ;
        જો (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            જો (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        જો (last_લીટી + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_લીટી = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    જો (!text.empty()) {
        જો (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        જ્યારે ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::strlen("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    પાછા std::make_pair(text, fl);
}

શૂન્ય EmitCBaseVisitorConst::emitSystemCSection(અચલ Ast_ગાંઠ_Module* modp,
                                               VSystemCSectionType type) {
    auto textAndFileline = scSection(modp, type);
    જો (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
