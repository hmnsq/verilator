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

#समावेश "V3PchAstMT.h"

#समावेश "V3EmitCBase.h"

#समावेश "V3Task.h"

//######################################################################
// EmitCParentModule implementation

EmitCParentModule::EmitCParentModule() {
    स्थिर स्वतः setAll = [](Ast_गाँठ_Module* modp) -> व्योम {
        क्रम (Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, गाँठ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_गाँठ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    क्रम (Ast_गाँठ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, गाँठ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(स्थिर Ast_गाँठ* nodep) VL_MT_STABLE {
    स्थिर string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    अटल V3Mutex s_mutex;
    स्थिर V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    अटल std::map<std::string, std::string> s_memoized;
    अटल std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    स्थिर स्वतः mit = s_memoized.find(prefix);
    अगर (mit != s_memoized.end()) वापस mit->second;
    //
    // Check capitalization
    स्थिर string prefixUpper = VString::upcase(prefix);
    string परिणाम;
    {
        स्थिर स्वतः it = s_ucToPrefix.find(prefixUpper);
        अगर (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            परिणाम = prefix;
        } अन्यथा अगर (it->second == prefix) {
            परिणाम = prefix;  // Same capitialization as last time
        } अन्यथा {
            VHashSha512 hash{prefix};
            परिणाम = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, परिणाम);
    वापस परिणाम;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(स्थिर AstCFunc* nodep, स्थिर Ast_गाँठ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::लो(nodep);
    string name;
    अगर (nodep->isConstructor()) {
        name += "init";
    } अन्यथा अगर (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } अन्यथा {
        अगर (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    वापस name;
}

string EmitCBaseVisitorConst::cFuncArgs(स्थिर AstCFunc* nodep, bool cpp) {
    // Return argument list for given C function
    string args;
    अगर (nodep->isLoose() && !nodep->isStatic()) {
        अगर (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } अन्यथा {
            अगर (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::लो(nodep));
            args += "* vlSelf";
        }
    }
    अगर (nodep->needProcess()) {
        अगर (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    अगर (!nodep->argTypes().empty()) {
        अगर (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    क्रम (स्थिर Ast_गाँठ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        अगर (स्थिर AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
            अगर (portp->isIO() && !portp->isFuncReturn()) {
                अगर (!args.empty()) args += ", ";
                अगर (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(true, false, cpp);
                } अन्यथा अगर (nodep->funcPublic()) {
                    args += portp->cPubArgType(true, false);
                } अन्यथा {
                    args += portp->vlArgType(true, false, true);
                }
            }
        }
    }
    वापस args;
}

व्योम EmitCBaseVisitorConst::emitCDefaultConstructor(स्थिर Ast_गाँठ_Module* स्थिर modp) {
    माला_दो(EmitCUtil::prefixNameProtect(modp));
    माला_दो("() = default;\n");
}

व्योम EmitCBaseVisitorConst::emitCFuncHeader(स्थिर AstCFunc* funcp, स्थिर Ast_गाँठ_Module* modp,
                                            bool withScope) {
    अगर (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    अगर (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        वापस;
    }
    अगर (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        माला_दो(" ");
    }
    अगर (withScope) {
        अगर (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::शीर्ष_Class_Name() + "::");
        } अन्यथा अगर (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    माला_दो("(" + cFuncArgs(funcp) + ")");
    अगर (funcp->isConst().trueKnown() && funcp->isProperMethod()) माला_दो(" const");
}

व्योम EmitCBaseVisitorConst::emitCFuncDecl(स्थिर AstCFunc* funcp, स्थिर Ast_गाँठ_Module* modp,
                                          bool cLinkage) {
    ensure_New_पंक्ति();
    अगर (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    अगर (funcp->isConstructor()) emitCDefaultConstructor(modp);
    अगर (cLinkage) putns(funcp, "extern \"C\" ");
    अगर (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    अगर (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ false);
    स्थिर AstClass* स्थिर classp = VN_CAST(modp, Class);
    अगर (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } अन्यथा {
        putns(funcp, ";\n");
    }
    अगर (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, स्थिर std::string& prefix,
                                                      स्थिर std::string& params,
                                                      स्थिर std::string& args,  //
                                                      माप_प्रकार maxCost, bool support,
                                                      स्थिर अक्षर* descriptionp,
                                                      स्थिर std::function<व्योम()>& emitHeader) {
    स्थिर स्वतः subFuncName = [&](माप_प्रकार n) { वापस prefix + "__" + std::to_string(n); };
    माप_प्रकार nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    bool callsSubFuncs = false;
    माप_प्रकार calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    जबतक (stmts.आकार() > 1) {
        स्थिर माप_प्रकार levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        स्थिर माप_प्रकार nStmts = stmts.आकार();
        क्रम (माप_प्रकार splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            माप_प्रकार cost = 0;
            जबतक (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
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
            अगर (callsSubFuncs) {
                माला_दो("\n");
                क्रम (माप_प्रकार j = splitStart; j < splitEnd; ++j) {
                    माला_दो("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                माला_दो("\n");
            }
            // Open sub-function definition in the split file
            माला_दो("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            क्रम (माप_प्रकार j = splitStart; j < splitEnd; ++j) {
                ofp()->putsNoTracking("    ");
                ofp()->putsNoTracking(stmts[j].m_text);
                ofp()->putsNoTracking("\n");
            }
            // Close sub-function
            माला_दो("}\n");
            // Close split file
            closeOutputFile();

            // Replace statements with a call to the sub-function
            calls.add(funcName + "(" + args + ");");
        }
        stmts = std::move(calls);
        callsSubFuncs = true;
        calleeBase = levelBase;
    }
    वापस funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, माप_प्रकार baseCost, स्थिर std::string& prefix,
    स्थिर std::string& params, स्थिर std::string& args, bool support, स्थिर अक्षर* descriptionp,
    स्थिर std::function<व्योम()>& emitHeader) {
    स्थिर माप_प्रकार maxCost = static_cast<माप_प्रकार>(v3Global.opt.outputSplitCFuncs());
    अगर (!maxCost) वापस {};
    माप_प्रकार योग_Cost = baseCost;
    क्रम (स्थिर TextStmts::Stmt& stmt : ctorStmts) योग_Cost += stmt.m_cost;
    क्रम (स्थिर TextStmts::Stmt& stmt : dtorStmts) योग_Cost += stmt.m_cost;
    अगर (योग_Cost <= maxCost) वापस {};
    v3Global.useParallelBuild(true);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    वापस {std::move(ctorDecl), std::move(dtorDecl)};
}

व्योम EmitCBaseVisitorConst::emitVarDecl(स्थिर AstVar* nodep, bool asRef) {
    स्थिर AstBasicDType* स्थिर basicp = nodep->basicp();
    स्थिर bool refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    अगर (nodep->mtask_Cache_पंक्ति_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    अगर (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", false, false));
        माला_दो(" const* " + nodep->nameProtect() + " = nullptr;\n");
        वापस;
    }

    स्थिर स्वतः emitDeclArrayBrackets = [यह](स्थिर AstVar* nodep) -> व्योम {
        // This isn't very robust and may need cleanup for other data types
        क्रम (स्थिर AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    अगर (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        अगर (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } अन्यथा अगर (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } अन्यथा अगर (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }
        माला_दो(nodep->scType());
        माला_दो("> ");
        अगर (asRef) {
            अगर (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        अगर (asRef && refNeedParens) माला_दो(")");
        emitDeclArrayBrackets(nodep);
        माला_दो(";\n");
    } अन्यथा अगर (nodep->isVLIO()) {
        अगर (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } अन्यथा अगर (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } अन्यथा अगर (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }

        अगर (nodep->isQuad()) {
            माला_दो("64");
        } अन्यथा अगर (nodep->widthMin() <= 8) {
            माला_दो("8");
        } अन्यथा अगर (nodep->widthMin() <= 16) {
            माला_दो("16");
        } अन्यथा अगर (nodep->isWide()) {
            माला_दो("W");
        }

        माला_दो("(");
        अगर (asRef) {
            अगर (refNeedParens) माला_दो("(");
            माला_दो("&");
        }
        माला_दो(nodep->nameProtect());
        अगर (asRef && refNeedParens) माला_दो(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        माला_दो("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        अगर (nodep->isWide()) माला_दो("," + cvtToStr(nodep->width_शब्दगण()));
        माला_दो(");\n");
    } अन्यथा {
        // Strings and other fundamental C types
        अगर (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            स्थिर string name = nodep->name();
            स्थिर string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            स्थिर bool beStatic = name.आकार() >= suffix.आकार()
                                  && name.substr(name.आकार() - suffix.आकार()) == suffix;
            अगर (beStatic) माला_दो("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(true, false, false, asRef));
        माला_दो(";\n");
    }
}

व्योम EmitCBaseVisitorConst::emitVarAccessors(स्थिर AstVar* nodep) {
    निश्चित(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    स्थिर string privateName = nodep->name();
    स्थिर string publicName = nodep->name().substr(म_माप("__Vm_sig_"));

    माला_दो("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    माला_दो("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

व्योम EmitCBaseVisitorConst::emitModCUse(स्थिर Ast_गाँठ_Module* modp, VUseType useType) {
    bool nl = false;
    forModCUse(modp, useType, [&](स्थिर string& entry) {
        माला_दो(entry);
        nl = true;
    });
    अगर (nl) माला_दो("\n");
}

std::pair<string, File_पंक्ति*> EmitCBaseVisitorConst::scSection(स्थिर Ast_गाँठ_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_पंक्ति* fl = nullptr;
    पूर्णांक last_पंक्ति = -999;
    क्रम (स्थिर AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        अगर (scSectionp->sectionType() != type) जारी;
        अगर (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            अगर (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        अगर (last_पंक्ति + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_पंक्ति = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    अगर (!text.empty()) {
        अगर (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::आकार_type pos;
        जबतक ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::म_माप("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    वापस std::make_pair(text, fl);
}

व्योम EmitCBaseVisitorConst::emitSystemCSection(स्थिर Ast_गाँठ_Module* modp,
                                               VSystemCSectionType type) {
    स्वतः textAndFileline = scSection(modp, type);
    अगर (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
