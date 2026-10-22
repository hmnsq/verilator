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
    अचर स्वतः setAll = [](Ast_गाँठो_Module* modp) -> शून्य {
        लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, गाँठो_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_गाँठो_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    लागि (Ast_गाँठो* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, गाँठो_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(अचर Ast_गाँठो* nodep) VL_MT_STABLE {
    अचर string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    स्थिर V3Mutex s_mutex;
    अचर V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    स्थिर std::map<std::string, std::string> s_memoized;
    स्थिर std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    अचर स्वतः mit = s_memoized.find(prefix);
    यदि (mit != s_memoized.end()) फर्काऊ mit->second;
    //
    // Check capitalization
    अचर string prefixUpper = VString::upcase(prefix);
    string result;
    {
        अचर स्वतः it = s_ucToPrefix.find(prefixUpper);
        यदि (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } अन्यथा यदि (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } अन्यथा {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    फर्काऊ result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(अचर AstCFunc* nodep, अचर Ast_गाँठो_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    यदि (nodep->isConstructor()) {
        name += "init";
    } अन्यथा यदि (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } अन्यथा {
        यदि (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    फर्काऊ name;
}

string EmitCBaseVisitorConst::cFuncArgs(अचर AstCFunc* nodep, तार्किक cpp) {
    // Return argument list for given C function
    string args;
    यदि (nodep->isLoose() && !nodep->isStatic()) {
        यदि (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } अन्यथा {
            यदि (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    यदि (nodep->needProcess()) {
        यदि (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    यदि (!nodep->argTypes().empty()) {
        यदि (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    लागि (अचर Ast_गाँठो* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        यदि (अचर AstVar* अचर portp = VN_CAST(stmtp, Var)) {
            यदि (portp->isIO() && !portp->isFuncReturn()) {
                यदि (!args.empty()) args += ", ";
                यदि (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(सत्य, असत्य, cpp);
                } अन्यथा यदि (nodep->funcPublic()) {
                    args += portp->cPubArgType(सत्य, असत्य);
                } अन्यथा {
                    args += portp->vlArgType(सत्य, असत्य, सत्य);
                }
            }
        }
    }
    फर्काऊ args;
}

शून्य EmitCBaseVisitorConst::emitCDefaultConstructor(अचर Ast_गाँठो_Module* अचर modp) {
    माला_दो(EmitCUtil::prefixNameProtect(modp));
    माला_दो("() = default;\n");
}

शून्य EmitCBaseVisitorConst::emitCFuncHeader(अचर AstCFunc* funcp, अचर Ast_गाँठो_Module* modp,
                                            तार्किक withScope) {
    यदि (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    यदि (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        फर्काऊ;
    }
    यदि (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        माला_दो(" ");
    }
    यदि (withScope) {
        यदि (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::शीर्ष_Class_Name() + "::");
        } अन्यथा यदि (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    माला_दो("(" + cFuncArgs(funcp) + ")");
    यदि (funcp->isConst().trueKnown() && funcp->isProperMethod()) माला_दो(" const");
}

शून्य EmitCBaseVisitorConst::emitCFuncDecl(अचर AstCFunc* funcp, अचर Ast_गाँठो_Module* modp,
                                          तार्किक cLinkage) {
    ensure_New_पङ्क्ति();
    यदि (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    यदि (funcp->isConstructor()) emitCDefaultConstructor(modp);
    यदि (cLinkage) putns(funcp, "extern \"C\" ");
    यदि (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    यदि (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ असत्य);
    अचर AstClass* अचर classp = VN_CAST(modp, Class);
    यदि (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } अन्यथा {
        putns(funcp, ";\n");
    }
    यदि (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, अचर std::string& prefix,
                                                      अचर std::string& params,
                                                      अचर std::string& args,  //
                                                      माप_प्रकार maxCost, तार्किक support,
                                                      अचर वर्ण* descriptionp,
                                                      अचर std::function<शून्य()>& emitHeader) {
    अचर स्वतः subFuncName = [&](माप_प्रकार n) { फर्काऊ prefix + "__" + std::to_string(n); };
    माप_प्रकार nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    तार्किक callsSubFuncs = असत्य;
    माप_प्रकार calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    जबसम्म (stmts.size() > 1) {
        अचर माप_प्रकार levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        अचर माप_प्रकार nStmts = stmts.size();
        लागि (माप_प्रकार splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            माप_प्रकार cost = 0;
            जबसम्म (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
                cost += stmts[splitEnd++].m_cost;
            }
            UASSERT(splitStart < splitEnd, "Empty split");

            // Name of sub-function we are emitting now
            funcName = subFuncName(nSubFunctions++);

            // Open split file
            openNewOutputSourceFile(funcName, सत्य, support, descriptionp);
            // Emit header
            emitHeader();
            // Declare called sub-functions
            यदि (callsSubFuncs) {
                माला_दो("\n");
                लागि (माप_प्रकार j = splitStart; j < splitEnd; ++j) {
                    माला_दो("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                माला_दो("\n");
            }
            // Open sub-function definition in the split file
            माला_दो("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            लागि (माप_प्रकार j = splitStart; j < splitEnd; ++j) {
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
        callsSubFuncs = सत्य;
        calleeBase = levelBase;
    }
    फर्काऊ funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, माप_प्रकार baseCost, अचर std::string& prefix,
    अचर std::string& params, अचर std::string& args, तार्किक support, अचर वर्ण* descriptionp,
    अचर std::function<शून्य()>& emitHeader) {
    अचर माप_प्रकार maxCost = static_cast<माप_प्रकार>(v3Global.opt.outputSplitCFuncs());
    यदि (!maxCost) फर्काऊ {};
    माप_प्रकार totalCost = baseCost;
    लागि (अचर TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    लागि (अचर TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    यदि (totalCost <= maxCost) फर्काऊ {};
    v3Global.useParallelBuild(सत्य);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    फर्काऊ {std::move(ctorDecl), std::move(dtorDecl)};
}

शून्य EmitCBaseVisitorConst::emitVarDecl(अचर AstVar* nodep, तार्किक asRef) {
    अचर AstBasicDType* अचर basicp = nodep->basicp();
    अचर तार्किक refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    यदि (nodep->mtask_Cache_पङ्क्ति_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    यदि (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", असत्य, असत्य));
        माला_दो(" const* " + nodep->nameProtect() + " = nullptr;\n");
        फर्काऊ;
    }

    अचर स्वतः emitDeclArrayBrackets = [एतत्](अचर AstVar* nodep) -> शून्य {
        // This isn't very robust and may need cleanup for other data types
        लागि (अचर AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    यदि (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        यदि (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } अन्यथा यदि (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } अन्यथा यदि (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }
        माला_दो(nodep->scType());
        माला_दो("> ");
        यदि (asRef) {
            यदि (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        यदि (asRef && refNeedParens) माला_दो(")");
        emitDeclArrayBrackets(nodep);
        माला_दो(";\n");
    } अन्यथा यदि (nodep->isVLIO()) {
        यदि (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } अन्यथा यदि (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } अन्यथा यदि (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }

        यदि (nodep->isQuad()) {
            माला_दो("64");
        } अन्यथा यदि (nodep->widthMin() <= 8) {
            माला_दो("8");
        } अन्यथा यदि (nodep->widthMin() <= 16) {
            माला_दो("16");
        } अन्यथा यदि (nodep->isWide()) {
            माला_दो("W");
        }

        माला_दो("(");
        यदि (asRef) {
            यदि (refNeedParens) माला_दो("(");
            माला_दो("&");
        }
        माला_दो(nodep->nameProtect());
        यदि (asRef && refNeedParens) माला_दो(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        माला_दो("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        यदि (nodep->isWide()) माला_दो("," + cvtToStr(nodep->width_शब्दहरू()));
        माला_दो(");\n");
    } अन्यथा {
        // Strings and other fundamental C types
        यदि (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            अचर string name = nodep->name();
            अचर string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            अचर तार्किक beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            यदि (beStatic) माला_दो("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(सत्य, असत्य, असत्य, asRef));
        माला_दो(";\n");
    }
}

शून्य EmitCBaseVisitorConst::emitVarAccessors(अचर AstVar* nodep) {
    प्रतिज्ञा(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    अचर string privateName = nodep->name();
    अचर string publicName = nodep->name().substr(म_माप("__Vm_sig_"));

    माला_दो("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    माला_दो("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

शून्य EmitCBaseVisitorConst::emitModCUse(अचर Ast_गाँठो_Module* modp, VUseType useType) {
    तार्किक nl = असत्य;
    forModCUse(modp, useType, [&](अचर string& entry) {
        माला_दो(entry);
        nl = सत्य;
    });
    यदि (nl) माला_दो("\n");
}

std::pair<string, File_पङ्क्ति*> EmitCBaseVisitorConst::scSection(अचर Ast_गाँठो_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_पङ्क्ति* fl = nullptr;
    पूर्णाङ्क last_पङ्क्ति = -999;
    लागि (अचर AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        यदि (scSectionp->sectionType() != type) जारी;
        यदि (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            यदि (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        यदि (last_पङ्क्ति + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_पङ्क्ति = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    यदि (!text.empty()) {
        यदि (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        जबसम्म ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::म_माप("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    फर्काऊ std::make_pair(text, fl);
}

शून्य EmitCBaseVisitorConst::emitSystemCSection(अचर Ast_गाँठो_Module* modp,
                                               VSystemCSectionType type) {
    स्वतः textAndFileline = scSection(modp, type);
    यदि (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
