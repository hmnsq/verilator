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

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCFunc.h"
#अन्तर्भाव "V3ThreadPool.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <tuple>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

वर्ग EmitCImp final : सार्वजनिक EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    अचर Ast_गाँठो_Module* अचर m_fileModp;
    अचर तार्किक m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    अचर std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    शून्य openNextOutputFile(अचर std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, असत्य, "Design implementation internals");
        emitFileHeader();
    }
    शून्य emitFileHeader() {
        माला_दो("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");
        माला_दो("\n");
        माला_दो("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    शून्य emitParamDefns(अचर Ast_गाँठो_Module* modp) {
        तार्किक first = सत्य;
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                यदि (varp->isParam()) {
                    यदि (first) {
                        माला_दो("\n");
                        अचर string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = असत्य;
                    }
                    यदि (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    माला_दो(";\n");
                }
            }
        }
        यदि (!first) माला_दो("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    स्थिर शून्य addVarCtorDtorStmts(TextStmts& stmts, अचर AstVar* varp, तार्किक destroy) {
        अचर std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        यदि (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            लागि (अचर AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                लागि (अचर std::string& elem : elems) {
                    लागि (पूर्णाङ्क i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.थप_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            यदि (destroy) std::reverse(elems.begin(), elems.end());
            लागि (अचर std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            फर्काऊ;
        }

        // Other variables are constructed whole, even if aggregates
        यदि (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            फर्काऊ;
        }
        अचर AstBasicDType* अचर basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        यदि (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } अन्यथा यदि (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->अङ्क().emitC();
        } अन्यथा यदि (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    शून्य createCtorDtorStmts(अचर Ast_गाँठो_Module* modp) {
        // Construct member variables
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अचर AstVar* अचर varp = VN_CAST(nodep, Var);
            यदि (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, असत्य);
            }
        }

        // Destroy in reverse order
        अचर Ast_गाँठो* अचर lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        लागि (अचर Ast_गाँठो* nodep = lastp; nodep; nodep = nodep->prevp()) {
            अचर AstVar* अचर varp = VN_CAST(nodep, Var);
            यदि (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, सत्य);
            }
        }

        // Split into sub-functions if needed
        अचर std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", असत्य,
            "Design implementation internals", [एतत्]() { emitFileHeader(); });
    }
    शून्य emitCtorDtorImp(अचर Ast_गाँठो_Module* modp) {
        अचर std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        यदि (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        यदि (!m_ctorSplitDecl.empty()) माला_दो("\n" + m_ctorSplitDecl + "\n");
        माला_दो("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        माला_दो("vlSelf->vlSymsp = symsp;\n");
        यदि (modp->is_शीर्ष()) {
            माला_दो("vlSelf->vlNamep = strdup(namep);\n");
        } अन्यथा {
            माला_दो("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        लागि (अचर TextStmts::Stmt& stmt : m_ctorStmts) माला_दो(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        यदि (modp->ctorVarReset()) {
            माला_दो(modName + "__" + protect("_ctor_var_reset") + "(vlSelf);\n");
        }
        माला_दो("}\n");

        // C++ constructor
        माला_दो("\n" + modName + "::" + modName + "(" + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        माला_दो(modName + "__ctor(this, symsp, namep);\n");
        emitSystemCSection(modp, VSystemCSectionType::CTOR);
        माला_दो("}\n");

        // dtor
        यदि (!m_dtorSplitDecl.empty()) माला_दो("\n" + m_dtorSplitDecl + "\n");
        माला_दो("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        लागि (अचर TextStmts::Stmt& stmt : m_dtorStmts) माला_दो(stmt.m_text + "\n");
        माला_दो("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        माला_दो("}\n");

        // C++ destructor
        माला_दो("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        माला_दो(modName + "__dtor(this);\n");
        माला_दो("}\n");
    }

    शून्य emitConfigureImp(अचर Ast_गाँठो_Module* modp) {
        अचर string modName = EmitCUtil::prefixNameProtect(modp);

        यदि (v3Global.opt.coverage()) {
            माला_दो("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        यदि (v3Global.opt.coverage()) {
            माला_दो("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            माला_दो("(void)first;  // Prevent unused variable warning\n");
            माला_दो(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            माला_दो("}\n");
        }
    }
    शून्य emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        यदि (v3Global.opt.coverage()) {
            माला_दो("\n// Coverage\n");
            माला_दो("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            माला_दो("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            यदि (v3Global.opt.threads() > 1) {
                माला_दो("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                माला_दो("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अन्यथा {
                माला_दो("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            माला_दो("static uint32_t fake_zero_count = 0;\n");
            माला_दो("std::string fullhier = std::string{vlNamep} + hierp;\n");
            माला_दो("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            माला_दो("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            माला_दो("*count32p = 0;\n");
            माला_दो("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            माला_दो("  \"filename\",filenamep,");
            माला_दो("  \"lineno\",lineno,");
            माला_दो("  \"column\",column,\n");
            माला_दो("\"hier\",fullhier.c_str(),");
            माला_दो("  \"page\",pagep,");
            माला_दो("  \"comment\",commentp,");
            माला_दो("  (linescovp[0] ? \"linescov\" : \"\"), linescovp,");
            माला_दो("  (fsmVarp[0] ? \"fsm_var\" : \"\"), fsmVarp,");
            माला_दो("  (fsmFromp[0] ? \"fsm_from\" : \"\"), fsmFromp,");
            माला_दो("  (fsmTop[0] ? \"fsm_to\" : \"\"), fsmTop,");
            माला_दो("  (fsmTagp[0] ? \"fsm_tag\" : \"\"), fsmTagp);\n");
            माला_दो("}\n");
        }
        यदि (v3Global.opt.coverageToggle()) {
            माला_दो("\n// Toggle Coverage\n");
            माला_दो("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            माला_दो("int begin, int end, bool ranged, ");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp) {\n");
            यदि (v3Global.opt.threads() > 1) {
                माला_दो("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            माला_दो("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            माला_दो("for (int i = begin; i != end + step; i += step) {\n");
            माला_दो("for (int j = 0; j < 2; j++) {\n");
            यदि (v3Global.opt.threads() > 1) {
                माला_दो("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अन्यथा {
                माला_दो("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            माला_दो("static uint32_t fake_zero_count = 0;\n");
            माला_दो("std::string fullhier = std::string{vlNamep} + hierp;\n");
            माला_दो("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            माला_दो("std::string commentWithIndex = commentp;\n");
            माला_दो("if (ranged) commentWithIndex += '[' + std::to_string(i) + ']';\n");
            माला_दो("commentWithIndex += j ? \":0->1\" : \":1->0\";\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            माला_दो("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            माला_दो("*count32p = 0;\n");
            माला_दो("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            माला_दो("  \"filename\",filenamep,");
            माला_दो("  \"lineno\",lineno,");
            माला_दो("  \"column\",column,\n");
            माला_दो("\"hier\",fullhier.c_str(),");
            माला_दो("  \"page\",pagep,");
            माला_दो("  \"comment\",commentWithIndex.c_str(),");
            माला_दो("  \"\", \"\");\n");  //  linescov argument, but in toggle coverage it is always
                                       //  empty
            माला_दो("++countp;\n");
            माला_दो("}\n");
            माला_दो("}\n");
            माला_दो("}\n");
        }
    }
    शून्य emitSavableImp(अचर Ast_गाँठो_Module* modp) {
        यदि (v3Global.opt.savable()) {
            माला_दो("\n// Savable\n");
            लागि (पूर्णाङ्क de = 0; de < 2; ++de) {
                अचर string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                अचर string funcname = de ? "__Vdeserialize" : "__Vserialize";
                अचर string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                        hash.घुसाउ(varp->name());
                        hash.घुसाउ(varp->dtypep()->width());
                    }
                }
                ofp()->म_लिखो("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                यदि (de) {
                    माला_दो("os.readAssert(__Vcheckval);\n");
                } अन्यथा {
                    माला_दो("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                माला_दो("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                        यदि (varp->isIO() && modp->is_शीर्ष() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } अन्यथा यदि (varp->isParam()) {
                        } अन्यथा यदि (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } अन्यथा {
                            पूर्णाङ्क vects = 0;
                            Ast_गाँठो_DType* elementp = varp->dtypeSkipRefp();
                            लागि (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                अचर पूर्णाङ्क vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                अचर string ivar = "__Vi"s + cvtToStr(vecnum);
                                माला_दो("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                माला_दो("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                माला_दो("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            अचर AstBasicDType* अचर basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            यदि (basicp && basicp->keyword().isMTaskState()) जारी;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            यदि (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                अचर पूर्णाङ्क vecnum = vects++;
                                अचर string ivar = "__Vi"s + cvtToStr(vecnum);
                                माला_दो("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                माला_दो("; " + ivar + " < " + cvtToStr(elementp->width_शब्दहरू()));
                                माला_दो("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            लागि (पूर्णाङ्क v = 0; v < vects; ++v) माला_दो("[__Vi" + cvtToStr(v) + "]");
                            माला_दो(";\n");
                            लागि (पूर्णाङ्क v = 0; v < vects; ++v) माला_दो("}\n");
                        }
                    }
                }

                माला_दो("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    तार्किक hasCommonImp(अचर Ast_गाँठो_Module* modp) अचर {
        // Nothing to emit if no module!
        यदि (!modp) फर्काऊ असत्य;
        // We always need the slow file
        यदि (m_slow) फर्काऊ सत्य;
        // The fast file is only required when we have `systemc_implementation nodes
        लागि (अचर AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            यदि (scSectionp->sectionType() == VSystemCSectionType::IMP) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }
    // Actually emit common implementation contents for given AstNodeModule
    शून्य doCommonImp(अचर Ast_गाँठो_Module* modp) {
        यदि (m_slow) {
            यदि (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } अन्यथा {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    शून्य emitCommonImp(अचर Ast_गाँठो_Module* modp) {
        अचर AstClass* अचर classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        यदि (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            यदि (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            यदि (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    शून्य emitCFuncImp(अचर Ast_गाँठो_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        अचर स्वतः gather = [एतत्, &funcps](अचर Ast_गाँठो_Module* modp) {
            लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* अचर funcp = VN_CAST(nodep, CFunc);
                यदि (!funcp) जारी;
                // TRACE_* and DPI handled elsewhere
                यदि (funcp->isTrace()) जारी;
                यदि (funcp->dpiImportPrototype()) जारी;
                यदि (funcp->dpiExportDispatcher()) जारी;
                यदि (funcp->slow() != m_slow) जारी;
                funcps.थप_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        यदि (अचर AstClassPackage* अचर packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        यदि (funcps.empty()) फर्काऊ;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        लागि (AstCFunc* अचर funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        यदि (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(सत्य);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(अचर Ast_गाँठो_Module* modp, तार्किक slow)
        : m_fileModp{modp}
        , m_slow{slow} {
        UINFO(5, "  Emitting implementation of " << EmitCUtil::prefixNameProtect(modp));

        m_modp = modp;

        // Emit implementation of this module, if this is an AstClassPackage, then put the
        // corresponding AstClass implementation in the same file as often optimizations are
        // possible when both are seen by the compiler
        // TODO: is the above comment still true?

        // Emit implementations of common parts
        emitCommonImp(modp);

        // Emit implementations of all AstCFunc
        emitCFuncImp(modp);
    }
    ~EmitCImp() override = स्वभाव;

सार्वजनिक:
    स्थिर std::vector<AstCFile*> main(अचर Ast_गाँठो_Module* modp, तार्किक slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        फर्काऊ emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
वर्ग EmitCTraceTypes final : सार्वजनिक EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    पूर्णाङ्क m_enum_अङ्क = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_गाँठो*, पूर्णाङ्क> m_enum_अङ्क_Map;  // EnumDType to enumeration number
    पूर्णाङ्क m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    अचर std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    अचर माप_प्रकार m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<माप_प्रकार>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<माप_प्रकार>::max();

    शून्य openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), सत्य, सत्य,
                                "Tracing declarations");
        माला_दो("\n");
        लागि (अचर std::string& base : v3Global.opt.traceSourceLangs()) {
            माला_दो("#include \"" + base + ".h\"\n");
        }
        माला_दो("\n");
        माला_दो("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

सार्वजनिक:
    // METHODS
    पूर्णाङ्क get_Enum_Map_अङ्क(AstEnumDType* nodep) {
        पूर्णाङ्क& enumNumr = m_enum_अङ्क_Map[nodep];
        यदि (!enumNumr) {
            यदि (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                माला_दो("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_अङ्क;
            पूर्णाङ्क nvals = 0;
            माला_दो("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            माला_दो("= {");
            लागि (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                यदि (++nvals > 1) माला_दो(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            माला_दो("};\n");
            nvals = 0;
            माला_दो("const char* " + protect("__VenumItemValues") + "[]\n");
            माला_दो("= {");
            लागि (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* अचर constp = VN_AS(itemp->valuep(), Const);
                यदि (++nvals > 1) माला_दो(", ");
                putbs("\"" + constp->अङ्क().displayed(nodep, "%0b") + "\"");
            }
            माला_दो("};\n");
            माला_दो("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            माला_दो("}\n");
            splitSizeInc(Ast_गाँठो::INSTR_गन्ती_CALL);
        }
        फर्काऊ enumNumr;
    }

    // Close output file
    शून्य finalize() {
        // Close function definition
        माला_दो("}\n");

        अचर std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        अचर std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        लागि (पूर्णाङ्क i = 0; i < m_traceTypeSubs - 1; ++i) {
            माला_दो("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        माला_दो("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        लागि (पूर्णाङ्क i = 0; i < m_traceTypeSubs; ++i) {
            माला_दो(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        माला_दो("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->शीर्ष_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = स्वभाव;
};

वर्ग EmitCTrace final : सार्वजनिक EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    अचर तार्किक m_slow;  // Making slow file
    अचर std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? नूतन EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    अचर std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "_" + protect("_Trace");

    // METHODS
    शून्य openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, सत्य,
                                "Tracing implementation internals");
        माला_दो("\n");
        लागि (अचर std::string& base : v3Global.opt.traceSourceLangs()) {
            माला_दो("#include \"" + base + ".h\"\n");
        }
        माला_दो("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        माला_दो("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    तार्किक emitTraceIsScBv(अचर AstTraceInc* nodep) {
        अचर AstVarRef* अचर varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        यदि (!varrefp) फर्काऊ असत्य;
        अचर AstVar* अचर varp = varrefp->varp();
        फर्काऊ varp->isSc() && varp->isScBv();
    }

    तार्किक emitTraceIsScBigUint(अचर AstTraceInc* nodep) {
        अचर AstVarRef* अचर varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        यदि (!varrefp) फर्काऊ असत्य;
        अचर AstVar* अचर varp = varrefp->varp();
        फर्काऊ varp->isSc() && varp->isScBigUint();
    }

    तार्किक emitTraceIsScUint(अचर AstTraceInc* nodep) {
        अचर AstVarRef* अचर varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        यदि (!varrefp) फर्काऊ असत्य;
        अचर AstVar* अचर varp = varrefp->varp();
        फर्काऊ varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    शून्य emitTraceInitOne(अचर AstTraceDecl* nodep, पूर्णाङ्क enum_अङ्क) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* अचर callp = nodep->dtypeCallp();
        यदि (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            फर्काऊ;
        }

        यदि (nodep->dtypep()->basicp()->isDouble()) {
            माला_दो("VL_TRACE_DECL_DOUBLE");
        } अन्यथा यदि (nodep->isWide()) {
            माला_दो("VL_TRACE_DECL_WIDE");
        } अन्यथा यदि (nodep->isQuad()) {
            माला_दो("VL_TRACE_DECL_QUAD");
        } अन्यथा यदि (nodep->bitRange().ranged()) {
            माला_दो("VL_TRACE_DECL_BUS");
        } अन्यथा यदि (nodep->dtypep()->basicp()->isEvent()) {
            माला_दो("VL_TRACE_DECL_EVENT");
        } अन्यथा {
            माला_दो("VL_TRACE_DECL_BIT");
        }

        यदि (nodep->arrayRange().ranged()) {
            माला_दो("_ARRAY(tracep");
        } अन्यथा {
            माला_दो("(tracep");
        }

        // Code
        माला_दो(",c+" + cvtToStr(nodep->code()));
        यदि (nodep->arrayRange().ranged()) माला_दो("+i*" + cvtToStr(nodep->width_शब्दहरू()));

        // Function index
        माला_दो(",");
        यदि (nodep->inDtypeFunc()) {
            माला_दो("fidx");
        } अन्यथा {
            माला_दो(cvtToStr(nodep->fidx()));
        }

        // Name
        माला_दो(",");
        putsQuoted(VIdProtect::protect_शब्दहरू_If(nodep->showname(), nodep->protect()));

        // Enum number
        माला_दो("," + cvtToStr(enum_अङ्क));

        // Direction
        यदि (nodep->inDtypeFunc()) {
            माला_दो(", direction");
        } अन्यथा {
            माला_दो(", " + direction);
        }

        // Kind
        माला_दो(", VerilatedTraceSigKind::");
        माला_दो(nodep->varType().traceSigKind());

        // Type
        माला_दो(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        माला_दो(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        यदि (nodep->arrayRange().ranged()) {
            यदि (nodep->arrayRange().ascending()) {
                माला_दो(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } अन्यथा {
                माला_दो(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        यदि (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            माला_दो(", " + cvtToStr(nodep->bitRange().बायाँ()) + ","
                 + cvtToStr(nodep->bitRange().दायाँ()));
        }

        //
        माला_दो(");");
    }

    पूर्णाङ्क emitTraceDeclDType(Ast_गाँठो_DType* nodep) {
        // Return enum number or -1 for none
        यदि (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            यदि (AstEnumDType* अचर enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                फर्काऊ m_emitTypesp->get_Enum_Map_अङ्क(enump);
            }
        }
        फर्काऊ -1;
    }

    शून्य emitTraceChangeOne(AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        अचर std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        तार्किक emitWidth = सत्य;
        string stype;
        यदि (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = असत्य;
        } अन्यथा यदि (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } अन्यथा यदि (nodep->isQuad()) {
            stype = "QData";
        } अन्यथा यदि (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } अन्यथा यदि (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } अन्यथा यदि (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } अन्यथा यदि (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = असत्य;
        } अन्यथा {
            stype = "Bit";
            emitWidth = असत्य;
        }
        putns(nodep, "bufp->" + func + stype);

        अचर uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_शब्दहरू());
        अचर uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        माला_दो("(oldp+");
        माला_दो(cvtToStr(code - nodep->baseCode()));
        माला_दो(",");
        अचर VNumRange& arrayRange = nodep->declp()->arrayRange();
        यदि (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_मान(nodep, arrayindex);
        यदि (emitWidth) माला_दो("," + cvtToStr(nodep->declp()->widthMin()));
        माला_दो(");\n");
    }

    शून्य emit_Trace_मान(अचर AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        यदि (AstVarRef* अचर varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            अचर AstVar* अचर varp = varrefp->varp();
            यदि (varp->isEvent()) माला_दो("&");
            माला_दो("(");
            यदि (emitTraceIsScBigUint(nodep)) {
                माला_दो("(uint32_t*)");
            } अन्यथा यदि (emitTraceIsScBv(nodep)) {
                माला_दो("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            यदि (varp->isSc()) माला_दो(".read()");
            यदि (emitTraceIsScUint(nodep)) {
                माला_दो(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } अन्यथा यदि (emitTraceIsScBigUint(nodep)) {
                माला_दो(".get_raw()");
            } अन्यथा यदि (emitTraceIsScBv(nodep)) {
                माला_दो(")");
            }
            माला_दो(")");
        } अन्यथा {
            माला_दो("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            माला_दो(")");
        }
    }

    शून्य emitTraceIndex(अचर AstTraceInc* अचर nodep, पूर्णाङ्क arrayindex) {
        // Tracing only supports 1D arrays
        यदि (nodep->declp()->arrayRange().ranged()) {
            यदि (arrayindex == -2) {
                माला_दो("[i]");
            } अन्यथा यदि (arrayindex == -1) {
                माला_दो("[0]");
            } अन्यथा {
                माला_दो("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    शून्य visit(AstCFunc* nodep) override {
        यदि (!nodep->isTrace()) फर्काऊ;
        यदि (nodep->slow() != m_slow) फर्काऊ;

        यदि (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(सत्य);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    शून्य visit(Ast_Trace_थप_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        यदि (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_शब्दहरू_If(nodep->prefix(), nodep->protect()));
        } अन्यथा {
            माला_दो(nodep->prefix());
        }
        माला_दो(", VerilatedTracePrefixType::");
        माला_दो(nodep->prefixType().ascii());
        माला_दो(", " + std::to_string(nodep->बायाँ()));
        माला_दो(", " + std::to_string(nodep->दायाँ()));
        माला_दो(");\n");
    }
    शून्य visit(Ast_Trace_झिक_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    शून्य visit(AstTraceDecl* nodep) override {
        अचर पूर्णाङ्क enum_अङ्क = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        यदि (nodep->arrayRange().ranged()) {
            माला_दो("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_अङ्क);
            माला_दो("\n}\n");
        } अन्यथा {
            emitTraceInitOne(nodep, enum_अङ्क);
            माला_दो("\n");
        }
    }
    शून्य visit(AstTraceInc* nodep) override {
        यदि (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            लागि (पूर्णाङ्क i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } अन्यथा {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(तार्किक slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->शीर्ष_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        लागि (Ast_गाँठो* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (AstCFunc* अचर funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        यदि (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = स्वभाव;

सार्वजनिक:
    स्थिर std::vector<AstCFile*> main(तार्किक slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        यदि (slow) {
            लागि (AstCFile* अचर cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        फर्काऊ cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

वर्ग EmitCFile final : सार्वजनिक EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य main(AstCFile* cfilep) VL_MT_STABLE {
        यदि (!cfilep->tblockp()) फर्काऊ;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        अचर EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        लागि (अचर Ast_गाँठो* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            यदि (VN_IS(nodep, Class)) जारी;  // Imped with ClassPackage
            अचर Ast_गाँठो_Module* अचर modp = VN_AS(nodep, गाँठो_Module);
            यदि (modp->isConstPool()) जारी;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::main(modp, /* slow: */ सत्य); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::main(modp, /* slow: */ असत्य); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        यदि (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::main(/* slow: */ सत्य); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::main(/* slow: */ असत्य); });
        }
    }
    // Add files to netlist
    लागि (अचर std::vector<AstCFile*>& cfileps : cfiles) {
        लागि (AstCFile* अचर cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

शून्य V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    लागि (Ast_गाँठो_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), गाँठो_File);
        यदि (AstCFile* अचर cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
