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
    स्थिरांक स्वतः setAll = [](Ast_गाठ_Module* modp) -> शून्य {
        साठी (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            जर (VN_IS(nodep, CFunc) || VN_IS(nodep, Var) || VN_IS(nodep, गाठ_Cover_Decl)) {
                nodep->user4p(modp);
            }
        }
        modp->foreach([&](Ast_गाठ_Cover_Decl* nodep) { nodep->user4p(modp); });
    };
    साठी (Ast_गाठ* modp = v3Global.rootp()->modulesp(); modp; modp = modp->nextp()) {
        setAll(VN_AS(modp, गाठ_Module));
    }
}

//######################################################################
// EmitCUtil implementation

string EmitCUtil::prefixNameProtect(स्थिरांक Ast_गाठ* nodep) VL_MT_STABLE {
    स्थिरांक string prefix = v3Global.opt.modPrefix() + "_" + VIdProtect::protect(nodep->name());
    // If all-uppercase prefix conflicts with a previous usage of the
    // prefix with different capitalization, rename to avoid conflict.
    // This is to support OSes where filename compares are non-case significant.
    // STATIC:
    स्थिर V3Mutex s_mutex;
    स्थिरांक V3LockGuard lock{s_mutex};  // Otherwise map access is unsafe
    स्थिर std::map<std::string, std::string> s_memoized;
    स्थिर std::map<std::string, std::string> s_ucToPrefix;
    // Memoize results
    स्थिरांक स्वतः mit = s_memoized.find(prefix);
    जर (mit != s_memoized.end()) परत mit->second;
    //
    // Check capitalization
    स्थिरांक string prefixUpper = VString::upcase(prefix);
    string result;
    {
        स्थिरांक स्वतः it = s_ucToPrefix.find(prefixUpper);
        जर (it == s_ucToPrefix.end()) {
            s_ucToPrefix.emplace(prefixUpper, prefix);
            result = prefix;
        } अन्यथा जर (it->second == prefix) {
            result = prefix;  // Same capitialization as last time
        } अन्यथा {
            VHashSha512 hash{prefix};
            result = prefix + "__Vphsh" + hash.digestSymbol24();
        }
    }
    s_memoized.emplace(prefix, result);
    परत result;
}

//######################################################################
// EmitCBaseVisitor implementation

string EmitCBaseVisitorConst::funcNameProtect(स्थिरांक AstCFunc* nodep, स्थिरांक Ast_गाठ_Module* modp) {
    modp = modp ? modp : EmitCParentModule::get(nodep);
    string name;
    जर (nodep->isConstructor()) {
        name += "init";
    } अन्यथा जर (nodep->isDestructor()) {
        name += "~";
        name += EmitCUtil::prefixNameProtect(modp);
    } अन्यथा {
        जर (nodep->isLoose()) {
            name += EmitCUtil::prefixNameProtect(modp);
            name += "__";
        }
        name += nodep->nameProtect();
    }
    परत name;
}

string EmitCBaseVisitorConst::cFuncArgs(स्थिरांक AstCFunc* nodep, तार्किक cpp) {
    // Return argument list for given C function
    string args;
    जर (nodep->isLoose() && !nodep->isStatic()) {
        जर (nodep->voidSelf()) {
            args += "void* vlVoidSelf";  // Converted to 'vlSelf' in the function body
        } अन्यथा {
            जर (nodep->isConst().trueKnown()) args += "const ";
            args += EmitCUtil::prefixNameProtect(EmitCParentModule::get(nodep));
            args += "* vlSelf";
        }
    }
    जर (nodep->needProcess()) {
        जर (!args.empty()) args += ", ";
        args += "VlProcessRef vlProcess";
    }
    जर (!nodep->argTypes().empty()) {
        जर (!args.empty()) args += ", ";
        args += nodep->argTypes();
    }
    // Might be a user function with argument list.
    साठी (स्थिरांक Ast_गाठ* stmtp = nodep->argsp(); stmtp; stmtp = stmtp->nextp()) {
        जर (स्थिरांक AstVar* स्थिरांक portp = VN_CAST(stmtp, Var)) {
            जर (portp->isIO() && !portp->isFuncReturn()) {
                जर (!args.empty()) args += ", ";
                जर (nodep->dpiImportPrototype() || nodep->dpiExportDispatcher()) {
                    args += portp->dpiArgType(सत्य, असत्य, cpp);
                } अन्यथा जर (nodep->funcPublic()) {
                    args += portp->cPubArgType(सत्य, असत्य);
                } अन्यथा {
                    args += portp->vlArgType(सत्य, असत्य, सत्य);
                }
            }
        }
    }
    परत args;
}

शून्य EmitCBaseVisitorConst::emitCDefaultConstructor(स्थिरांक Ast_गाठ_Module* स्थिरांक modp) {
    माला_दो(EmitCUtil::prefixNameProtect(modp));
    माला_दो("() = default;\n");
}

शून्य EmitCBaseVisitorConst::emitCFuncHeader(स्थिरांक AstCFunc* funcp, स्थिरांक Ast_गाठ_Module* modp,
                                            तार्किक withScope) {
    जर (funcp->slow()) putns(funcp, "VL_ATTR_COLD ");
    जर (funcp->dpiCDeclOverride()) {
        putns(funcp, funcp->dpiCDecl() + ";\n");
        परत;
    }
    जर (!funcp->isDestructor()) {
        putns(funcp, funcp->rtnTypeVoid());
        माला_दो(" ");
    }
    जर (withScope) {
        जर (funcp->dpiExportDispatcher()) {
            putns(funcp, EmitCUtil::शीर्ष_Class_Name() + "::");
        } अन्यथा जर (funcp->isProperMethod()) {
            putns(funcp, EmitCUtil::prefixNameProtect(modp) + "::");
        }
    }
    putns(funcp, funcNameProtect(funcp, modp));
    माला_दो("(" + cFuncArgs(funcp) + ")");
    जर (funcp->isConst().trueKnown() && funcp->isProperMethod()) माला_दो(" const");
}

शून्य EmitCBaseVisitorConst::emitCFuncDecl(स्थिरांक AstCFunc* funcp, स्थिरांक Ast_गाठ_Module* modp,
                                          तार्किक cLinkage) {
    ensure_New_ओळ();
    जर (!funcp->ifdef().empty()) putns(funcp, "#ifdef " + funcp->ifdef() + "\n");
    जर (funcp->isConstructor()) emitCDefaultConstructor(modp);
    जर (cLinkage) putns(funcp, "extern \"C\" ");
    जर (funcp->isStatic() && funcp->isProperMethod()) putns(funcp, "static ");
    जर (funcp->isVirtual()) {
        UASSERT_OBJ(funcp->isProperMethod(), funcp, "Virtual function is not a proper method");
        putns(funcp, "virtual ");
        // Intentionally no emit for "override" instead, as clang will then enable warning
        // on other methods where virtual vs override is needed, and this is not tracked yet
    }
    emitCFuncHeader(funcp, modp, /* withScope: */ असत्य);
    स्थिरांक AstClass* स्थिरांक classp = VN_CAST(modp, Class);
    जर (funcp->emptyBody() && !funcp->isLoose() && !cLinkage
        && !(funcp->isConstructor() && classp && classp->isInterfaceClass())) {
        putns(funcp, " {}\n");
    } अन्यथा {
        putns(funcp, ";\n");
    }
    जर (!funcp->ifdef().empty()) putns(funcp, "#endif  // " + funcp->ifdef() + "\n");
}

std::string EmitCBaseVisitorConst::emitSplitTextStmts(TextStmts& stmts, स्थिरांक std::string& prefix,
                                                      स्थिरांक std::string& params,
                                                      स्थिरांक std::string& args,  //
                                                      माप_प्रकार maxCost, तार्किक support,
                                                      स्थिरांक अक्षर* descriptionp,
                                                      स्थिरांक std::function<शून्य()>& emitHeader) {
    स्थिरांक स्वतः subFuncName = [&](माप_प्रकार n) { परत prefix + "__" + std::to_string(n); };
    माप_प्रकार nSubFunctions = 0;
    std::string funcName;  // Name of last sub-function, which is the root at the end
    // After the first level, statement 'j' is a call to sub-function 'calleeBase + j'
    तार्किक callsSubFuncs = असत्य;
    माप_प्रकार calleeBase = 0;
    // Reduce into a balanced tree of sub-function calls until we end up with a single statement
    पर्यंत (stmts.size() > 1) {
        स्थिरांक माप_प्रकार levelBase = nSubFunctions;
        TextStmts calls;  // Calls to the sub-functions of this level
        स्थिरांक माप_प्रकार nStmts = stmts.size();
        साठी (माप_प्रकार splitStart = 0, splitEnd = 0; splitStart < nStmts; splitStart = splitEnd) {
            // Gather up at at most 'maxCost' worth of statements in this split,
            // but always at least 2 (if less than 2, the reduction makes no
            // progress and the loop will not terminate).
            माप_प्रकार cost = 0;
            पर्यंत (((cost < maxCost) || (splitEnd - splitStart < 2)) && splitEnd < nStmts) {
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
            जर (callsSubFuncs) {
                माला_दो("\n");
                साठी (माप_प्रकार j = splitStart; j < splitEnd; ++j) {
                    माला_दो("void " + subFuncName(calleeBase + j) + "(" + params + ");\n");
                }
                माला_दो("\n");
            }
            // Open sub-function definition in the split file
            माला_दो("void " + funcName + "(" + params + ") {\n");
            // Emit statements
            साठी (माप_प्रकार j = splitStart; j < splitEnd; ++j) {
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
    परत funcName.empty() ? "" : "void " + funcName + "(" + params + ");\n";
}

std::pair<std::string, std::string> EmitCBaseVisitorConst::emitSplitCtorDtor(
    TextStmts& ctorStmts, TextStmts& dtorStmts, माप_प्रकार baseCost, स्थिरांक std::string& prefix,
    स्थिरांक std::string& params, स्थिरांक std::string& args, तार्किक support, स्थिरांक अक्षर* descriptionp,
    स्थिरांक std::function<शून्य()>& emitHeader) {
    स्थिरांक माप_प्रकार maxCost = static_cast<माप_प्रकार>(v3Global.opt.outputSplitCFuncs());
    जर (!maxCost) परत {};
    माप_प्रकार totalCost = baseCost;
    साठी (स्थिरांक TextStmts::Stmt& stmt : ctorStmts) totalCost += stmt.m_cost;
    साठी (स्थिरांक TextStmts::Stmt& stmt : dtorStmts) totalCost += stmt.m_cost;
    जर (totalCost <= maxCost) परत {};
    v3Global.useParallelBuild(सत्य);  // Splitting files, so using parallel build.
    std::string ctorDecl = emitSplitTextStmts(ctorStmts, prefix + "__ctor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    std::string dtorDecl = emitSplitTextStmts(dtorStmts, prefix + "__dtor", params, args, maxCost,
                                              support, descriptionp, emitHeader);
    परत {std::move(ctorDecl), std::move(dtorDecl)};
}

शून्य EmitCBaseVisitorConst::emitVarDecl(स्थिरांक AstVar* nodep, तार्किक asRef) {
    स्थिरांक AstBasicDType* स्थिरांक basicp = nodep->basicp();
    स्थिरांक तार्किक refNeedParens = VN_IS(nodep->dtypeSkipRefp(), UnpackArrayDType);
    जर (nodep->mtask_Cache_ओळ_Align() && !asRef) putns(nodep, "alignas(VL_CACHE_LINE_BYTES) ");
    जर (nodep->covergroupRefMember()) {
        UASSERT_OBJ(!asRef, nodep, "Covergroup reference member emitted as C++ reference");
        putns(nodep, nodep->dtypep()->cType("", असत्य, असत्य));
        माला_दो(" const* " + nodep->nameProtect() + " = nullptr;\n");
        परत;
    }

    स्थिरांक स्वतः emitDeclArrayBrackets = [हे](स्थिरांक AstVar* nodep) -> शून्य {
        // This isn't very robust and may need cleanup for other data types
        साठी (स्थिरांक AstUnpackArrayDType* arrayp = VN_CAST(nodep->dtypeSkipRefp(), UnpackArrayDType);
             arrayp; arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
            putns(arrayp, "[" + cvtToStr(arrayp->elementsConst()) + "]");
        }
    };

    जर (nodep->isPrimaryIO() && nodep->isSc()) {
        UASSERT_OBJ(basicp, nodep, "Unimplemented: Outputting this data type");
        जर (nodep->isInout()) {
            putns(nodep, "sc_core::sc_inout<");
        } अन्यथा जर (nodep->isWritable()) {
            putns(nodep, "sc_core::sc_out<");
        } अन्यथा जर (nodep->isNonOutput()) {
            putns(nodep, "sc_core::sc_in<");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }
        माला_दो(nodep->scType());
        माला_दो("> ");
        जर (asRef) {
            जर (refNeedParens) putns(nodep, "(");
            putns(nodep, "&");
        }
        putns(nodep, nodep->nameProtect());
        जर (asRef && refNeedParens) माला_दो(")");
        emitDeclArrayBrackets(nodep);
        माला_दो(";\n");
    } अन्यथा जर (nodep->isVLIO()) {
        जर (nodep->isInout()) {
            putns(nodep, "VL_INOUT");
        } अन्यथा जर (nodep->isWritable()) {
            putns(nodep, "VL_OUT");
        } अन्यथा जर (nodep->isNonOutput()) {
            putns(nodep, "VL_IN");
        } अन्यथा {
            nodep->v3fatalSrc("Unknown type");
        }

        जर (nodep->isQuad()) {
            माला_दो("64");
        } अन्यथा जर (nodep->widthMin() <= 8) {
            माला_दो("8");
        } अन्यथा जर (nodep->widthMin() <= 16) {
            माला_दो("16");
        } अन्यथा जर (nodep->isWide()) {
            माला_दो("W");
        }

        माला_दो("(");
        जर (asRef) {
            जर (refNeedParens) माला_दो("(");
            माला_दो("&");
        }
        माला_दो(nodep->nameProtect());
        जर (asRef && refNeedParens) माला_दो(")");
        emitDeclArrayBrackets(nodep);
        // If it's a packed struct/array then nodep->width is the whole
        // thing, msb/lsb is just lowest dimension
        माला_दो("," + cvtToStr(basicp->lo() + nodep->width() - 1) + "," + cvtToStr(basicp->lo()));
        जर (nodep->isWide()) माला_दो("," + cvtToStr(nodep->width_शब्द()));
        माला_दो(");\n");
    } अन्यथा {
        // Strings and other fundamental C types
        जर (nodep->isFuncLocal() && nodep->dtypep()->containsString()) {
            स्थिरांक string name = nodep->name();
            स्थिरांक string suffix = V3Task::dpiTemporaryVarSuffix();
            // Temporary variable for DPI-C containing strings needs to be static because c_str()
            // will be passed to C code and the lifetime of the variable must be long enough. See
            // also Issue 2622.
            स्थिरांक तार्किक beStatic = name.size() >= suffix.size()
                                  && name.substr(name.size() - suffix.size()) == suffix;
            जर (beStatic) माला_दो("static thread_local ");
        }
        putns(nodep, nodep->vlArgType(सत्य, असत्य, असत्य, asRef));
        माला_दो(";\n");
    }
}

शून्य EmitCBaseVisitorConst::emitVarAccessors(स्थिरांक AstVar* nodep) {
    प्रतिज्ञा(nodep->name().rfind("__Vm_sig_") == 0 && nodep->isIO());
    स्थिरांक string privateName = nodep->name();
    स्थिरांक string publicName = nodep->name().substr(म_माप("__Vm_sig_"));

    माला_दो("decltype("s + privateName + ") "s + publicName + "() {return "s + privateName + ";}\n");
    माला_दो("void "s + publicName + "(decltype(" + privateName + ") v) {"s + privateName + "=v;}\n");
}

शून्य EmitCBaseVisitorConst::emitModCUse(स्थिरांक Ast_गाठ_Module* modp, VUseType useType) {
    तार्किक nl = असत्य;
    forModCUse(modp, useType, [&](स्थिरांक string& entry) {
        माला_दो(entry);
        nl = सत्य;
    });
    जर (nl) माला_दो("\n");
}

std::pair<string, File_ओळ*> EmitCBaseVisitorConst::scSection(स्थिरांक Ast_गाठ_Module* modp,
                                                              VSystemCSectionType type) {
    string text;
    File_ओळ* fl = nullptr;
    पूर्णांक last_ओळ = -999;
    साठी (स्थिरांक AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
         scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
        जर (scSectionp->sectionType() != type) सुरूठेवा;
        जर (text.empty()) {
            fl = scSectionp->fileline();
            text += "\n";
            जर (v3Global.opt.decoration()) {
                text += "\n//*** Below code from `systemc in Verilog file\n";
            }
        }
        जर (last_ओळ + 1 != scSectionp->fileline()->lineno() && v3Global.opt.decoration()) {
            text += "// From `systemc at " + scSectionp->fileline()->ascii() + "\n";
        }
        last_ओळ = scSectionp->fileline()->lineno();
        text += scSectionp->text();
    }
    जर (!text.empty()) {
        जर (v3Global.opt.decoration()) text += "//*** Above code from `systemc in Verilog file\n";
        text += "\n";
        // Substitute `systemc_class_name
        string::size_type pos;
        पर्यंत ((pos = text.find("`systemc_class_name")) != string::npos) {
            text.replace(pos, std::म_माप("`systemc_class_name"),
                         EmitCUtil::prefixNameProtect(modp));
        }
    }
    परत std::make_pair(text, fl);
}

शून्य EmitCBaseVisitorConst::emitSystemCSection(स्थिरांक Ast_गाठ_Module* modp,
                                               VSystemCSectionType type) {
    स्वतः textAndFileline = scSection(modp, type);
    जर (!textAndFileline.first.empty()) ofp()->putsNoTracking(textAndFileline.first);
}
