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

वग्ग EmitCImp final : सार्वजनिक EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ध्रुव Ast_ग्रन्थिः_Module* ध्रुव m_fileModp;
    ध्रुव तार्किक m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    शून्य openNextOutputFile(ध्रुव std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, मिच्छा, "Design implementation internals");
        emitFileHeader();
    }
    शून्य emitFileHeader() {
        puts("// See " + EmitCUtil::शिखरम्_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    शून्य emitParamDefns(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        तार्किक first = सच्च;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                सचे (varp->isParam()) {
                    सचे (first) {
                        puts("\n");
                        ध्रुव string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = मिच्छा;
                    }
                    सचे (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        सचे (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    स्थिर शून्य addVarCtorDtorStmts(TextStmts& stmts, ध्रुव AstVar* varp, तार्किक destroy) {
        ध्रुव std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        सचे (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            अत्थाय (ध्रुव AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                अत्थाय (ध्रुव std::string& elem : elems) {
                    अत्थाय (पूर्णाङ्क i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.प्रक्षिप_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            सचे (destroy) std::reverse(elems.begin(), elems.end());
            अत्थाय (ध्रुव std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            पटिनिवत्त;
        }

        // Other variables are constructed whole, even if aggregates
        सचे (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            पटिनिवत्त;
        }
        ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        सचे (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } अञ्ञथा सचे (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->अङ्कः().emitC();
        } अञ्ञथा सचे (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    शून्य createCtorDtorStmts(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Construct member variables
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            सचे (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, मिच्छा);
            }
        }

        // Destroy in reverse order
        ध्रुव Ast_ग्रन्थिः* ध्रुव lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            सचे (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, सच्च);
            }
        }

        // Split into sub-functions if needed
        ध्रुव std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", मिच्छा,
            "Design implementation internals", [एतत्]() { emitFileHeader(); });
    }
    शून्य emitCtorDtorImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        सचे (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        सचे (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        सचे (modp->is_शिखरम्()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } अञ्ञथा {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        अत्थाय (ध्रुव TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        सचे (modp->ctorVarReset()) {
            puts(modName + "__" + protect("_ctor_var_reset") + "(vlSelf);\n");
        }
        puts("}\n");

        // C++ constructor
        puts("\n" + modName + "::" + modName + "(" + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts(modName + "__ctor(this, symsp, namep);\n");
        emitSystemCSection(modp, VSystemCSectionType::CTOR);
        puts("}\n");

        // dtor
        सचे (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        अत्थाय (ध्रुव TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    शून्य emitConfigureImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव string modName = EmitCUtil::prefixNameProtect(modp);

        सचे (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        सचे (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    शून्य emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        सचे (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            सचे (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अञ्ञथा {
                puts("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            puts("static uint32_t fake_zero_count = 0;\n");
            puts("std::string fullhier = std::string{vlNamep} + hierp;\n");
            puts("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            puts("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            puts("*count32p = 0;\n");
            puts("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            puts("  \"filename\",filenamep,");
            puts("  \"lineno\",lineno,");
            puts("  \"column\",column,\n");
            puts("\"hier\",fullhier.c_str(),");
            puts("  \"page\",pagep,");
            puts("  \"comment\",commentp,");
            puts("  (linescovp[0] ? \"linescov\" : \"\"), linescovp,");
            puts("  (fsmVarp[0] ? \"fsm_var\" : \"\"), fsmVarp,");
            puts("  (fsmFromp[0] ? \"fsm_from\" : \"\"), fsmFromp,");
            puts("  (fsmTop[0] ? \"fsm_to\" : \"\"), fsmTop,");
            puts("  (fsmTagp[0] ? \"fsm_tag\" : \"\"), fsmTagp);\n");
            puts("}\n");
        }
        सचे (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            सचे (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            सचे (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अञ्ञथा {
                puts("uint32_t* count32p = countp;\n");
            }
            // static doesn't need save-restore as is constant
            puts("static uint32_t fake_zero_count = 0;\n");
            puts("std::string fullhier = std::string{vlNamep} + hierp;\n");
            puts("if (!fullhier.empty() && fullhier[0] == '.') fullhier = fullhier.substr(1);\n");
            puts("std::string commentWithIndex = commentp;\n");
            puts("if (ranged) commentWithIndex += '[' + std::to_string(i) + ']';\n");
            puts("commentWithIndex += j ? \":0->1\" : \":1->0\";\n");
            // Global-counter users still redirect later duplicate instances
            // to fake_zero_count for default collapsed coverage. Object-local
            // counters must keep the real pointer so forcePerInstance can
            // report each hierarchy independently.
            puts("if (!enable && !localCounter) count32p = &fake_zero_count;\n");
            puts("*count32p = 0;\n");
            puts("VL_COVER_INSERT(vlSymsp->_vm_contextp__->coveragep(), vlNamep, count32p,");
            puts("  \"filename\",filenamep,");
            puts("  \"lineno\",lineno,");
            puts("  \"column\",column,\n");
            puts("\"hier\",fullhier.c_str(),");
            puts("  \"page\",pagep,");
            puts("  \"comment\",commentWithIndex.c_str(),");
            puts("  \"\", \"\");\n");  //  linescov argument, but in toggle coverage it is always
                                       //  empty
            puts("++countp;\n");
            puts("}\n");
            puts("}\n");
            puts("}\n");
        }
    }
    शून्य emitSavableImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        सचे (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            अत्थाय (पूर्णाङ्क de = 0; de < 2; ++de) {
                ध्रुव string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ध्रुव string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ध्रुव string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                        hash.निवेशय(varp->name());
                        hash.निवेशय(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                सचे (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } अञ्ञथा {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                        सचे (varp->isIO() && modp->is_शिखरम्() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } अञ्ञथा सचे (varp->isParam()) {
                        } अञ्ञथा सचे (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } अञ्ञथा {
                            पूर्णाङ्क vects = 0;
                            Ast_ग्रन्थिः_DType* elementp = varp->dtypeSkipRefp();
                            अत्थाय (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ध्रुव पूर्णाङ्क vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ध्रुव string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ध्रुव AstBasicDType* ध्रुव basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            सचे (basicp && basicp->keyword().isMTaskState()) अनुवर्त;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            सचे (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ध्रुव पूर्णाङ्क vecnum = vects++;
                                ध्रुव string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_शब्दाः()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            अत्थाय (पूर्णाङ्क v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            अत्थाय (पूर्णाङ्क v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    तार्किक hasCommonImp(ध्रुव Ast_ग्रन्थिः_Module* modp) ध्रुव {
        // Nothing to emit if no module!
        सचे (!modp) पटिनिवत्त मिच्छा;
        // We always need the slow file
        सचे (m_slow) पटिनिवत्त सच्च;
        // The fast file is only required when we have `systemc_implementation nodes
        अत्थाय (ध्रुव AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            सचे (scSectionp->sectionType() == VSystemCSectionType::IMP) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }
    // Actually emit common implementation contents for given AstNodeModule
    शून्य doCommonImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        सचे (m_slow) {
            सचे (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } अञ्ञथा {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    शून्य emitCommonImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव AstClass* ध्रुव classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        सचे (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            सचे (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            सचे (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    शून्य emitCFuncImp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ध्रुव auto gather = [एतत्, &funcps](ध्रुव Ast_ग्रन्थिः_Module* modp) {
            अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
                सचे (!funcp) अनुवर्त;
                // TRACE_* and DPI handled elsewhere
                सचे (funcp->isTrace()) अनुवर्त;
                सचे (funcp->dpiImportPrototype()) अनुवर्त;
                सचे (funcp->dpiExportDispatcher()) अनुवर्त;
                सचे (funcp->slow() != m_slow) अनुवर्त;
                funcps.प्रक्षिप_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        सचे (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        सचे (funcps.empty()) पटिनिवत्त;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        अत्थाय (AstCFunc* ध्रुव funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        सचे (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(सच्च);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ध्रुव Ast_ग्रन्थिः_Module* modp, तार्किक slow)
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
    स्थिर std::vector<AstCFile*> main(ध्रुव Ast_ग्रन्थिः_Module* modp, तार्किक slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        पटिनिवत्त emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
वग्ग EmitCTraceTypes final : सार्वजनिक EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    पूर्णाङ्क m_enum_अङ्कः = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ग्रन्थिः*, पूर्णाङ्क> m_enum_अङ्कः_Map;  // EnumDType to enumeration number
    पूर्णाङ्क m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::शिखरम्_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ध्रुव size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    शून्य openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), सच्च, सच्च,
                                "Tracing declarations");
        puts("\n");
        अत्थाय (ध्रुव std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

सार्वजनिक:
    // METHODS
    पूर्णाङ्क get_Enum_Map_अङ्कः(AstEnumDType* nodep) {
        पूर्णाङ्क& enumNumr = m_enum_अङ्कः_Map[nodep];
        सचे (!enumNumr) {
            सचे (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सच्च);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_अङ्कः;
            पूर्णाङ्क nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            अत्थाय (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                सचे (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            अत्थाय (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                सचे (++nvals > 1) puts(", ");
                putbs("\"" + constp->अङ्कः().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_ग्रन्थिः::INSTR_गणनम्_CALL);
        }
        पटिनिवत्त enumNumr;
    }

    // Close output file
    शून्य finalize() {
        // Close function definition
        puts("}\n");

        ध्रुव std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ध्रुव std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        अत्थाय (पूर्णाङ्क i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        अत्थाय (पूर्णाङ्क i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->शिखरम्_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = स्वभाव;
};

वग्ग EmitCTrace final : सार्वजनिक EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ध्रुव तार्किक m_slow;  // Making slow file
    ध्रुव std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? नूतन EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::शिखरम्_Class_Name() + "_" + protect("_Trace");

    // METHODS
    शून्य openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, सच्च,
                                "Tracing implementation internals");
        puts("\n");
        अत्थाय (ध्रुव std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    तार्किक emitTraceIsScBv(ध्रुव AstTraceInc* nodep) {
        ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        सचे (!varrefp) पटिनिवत्त मिच्छा;
        ध्रुव AstVar* ध्रुव varp = varrefp->varp();
        पटिनिवत्त varp->isSc() && varp->isScBv();
    }

    तार्किक emitTraceIsScBigUint(ध्रुव AstTraceInc* nodep) {
        ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        सचे (!varrefp) पटिनिवत्त मिच्छा;
        ध्रुव AstVar* ध्रुव varp = varrefp->varp();
        पटिनिवत्त varp->isSc() && varp->isScBigUint();
    }

    तार्किक emitTraceIsScUint(ध्रुव AstTraceInc* nodep) {
        ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        सचे (!varrefp) पटिनिवत्त मिच्छा;
        ध्रुव AstVar* ध्रुव varp = varrefp->varp();
        पटिनिवत्त varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    शून्य emitTraceInitOne(ध्रुव AstTraceDecl* nodep, पूर्णाङ्क enum_अङ्कः) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ध्रुव callp = nodep->dtypeCallp();
        सचे (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            पटिनिवत्त;
        }

        सचे (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } अञ्ञथा सचे (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } अञ्ञथा सचे (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } अञ्ञथा सचे (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } अञ्ञथा सचे (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } अञ्ञथा {
            puts("VL_TRACE_DECL_BIT");
        }

        सचे (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } अञ्ञथा {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        सचे (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_शब्दाः()));

        // Function index
        puts(",");
        सचे (nodep->inDtypeFunc()) {
            puts("fidx");
        } अञ्ञथा {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_अङ्कः));

        // Direction
        सचे (nodep->inDtypeFunc()) {
            puts(", direction");
        } अञ्ञथा {
            puts(", " + direction);
        }

        // Kind
        puts(", VerilatedTraceSigKind::");
        puts(nodep->varType().traceSigKind());

        // Type
        puts(", VerilatedTraceSigType::");
        UASSERT_OBJ(nodep->dtypep()->basicp()->keyword() != VBasicDTypeKwd::UNKNOWN, nodep,
                    "DType is unknown");
        puts(nodep->dtypep()->basicp()->keyword().traceSigType());

        // Array range
        सचे (nodep->arrayRange().ranged()) {
            सचे (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } अञ्ञथा {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        सचे (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().वामः()) + ","
                 + cvtToStr(nodep->bitRange().दक्षिणः()));
        }

        //
        puts(");");
    }

    पूर्णाङ्क emitTraceDeclDType(Ast_ग्रन्थिः_DType* nodep) {
        // Return enum number or -1 for none
        सचे (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            सचे (AstEnumDType* ध्रुव enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                पटिनिवत्त m_emitTypesp->get_Enum_Map_अङ्कः(enump);
            }
        }
        पटिनिवत्त -1;
    }

    शून्य emitTraceChangeOne(AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ध्रुव std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        तार्किक emitWidth = सच्च;
        string stype;
        सचे (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = मिच्छा;
        } अञ्ञथा सचे (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } अञ्ञथा सचे (nodep->isQuad()) {
            stype = "QData";
        } अञ्ञथा सचे (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } अञ्ञथा सचे (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } अञ्ञथा सचे (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } अञ्ञथा सचे (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = मिच्छा;
        } अञ्ञथा {
            stype = "Bit";
            emitWidth = मिच्छा;
        }
        putns(nodep, "bufp->" + func + stype);

        ध्रुव uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_शब्दाः());
        ध्रुव uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ध्रुव VNumRange& arrayRange = nodep->declp()->arrayRange();
        सचे (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_मानम्(nodep, arrayindex);
        सचे (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    शून्य emit_Trace_मानम्(ध्रुव AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        सचे (AstVarRef* ध्रुव varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ध्रुव AstVar* ध्रुव varp = varrefp->varp();
            सचे (varp->isEvent()) puts("&");
            puts("(");
            सचे (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } अञ्ञथा सचे (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            सचे (varp->isSc()) puts(".read()");
            सचे (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } अञ्ञथा सचे (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } अञ्ञथा सचे (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } अञ्ञथा {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    शून्य emitTraceIndex(ध्रुव AstTraceInc* ध्रुव nodep, पूर्णाङ्क arrayindex) {
        // Tracing only supports 1D arrays
        सचे (nodep->declp()->arrayRange().ranged()) {
            सचे (arrayindex == -2) {
                puts("[i]");
            } अञ्ञथा सचे (arrayindex == -1) {
                puts("[0]");
            } अञ्ञथा {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    शून्य visit(AstCFunc* nodep) override {
        सचे (!nodep->isTrace()) पटिनिवत्त;
        सचे (nodep->slow() != m_slow) पटिनिवत्त;

        सचे (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(सच्च);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    शून्य visit(Ast_Trace_प्रक्षिप_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        सचे (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_शब्दाः_If(nodep->prefix(), nodep->protect()));
        } अञ्ञथा {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->वामः()));
        puts(", " + std::to_string(nodep->दक्षिणः()));
        puts(");\n");
    }
    शून्य visit(Ast_Trace_निष्कासय_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    शून्य visit(AstTraceDecl* nodep) override {
        ध्रुव पूर्णाङ्क enum_अङ्कः = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        सचे (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_अङ्कः);
            puts("\n}\n");
        } अञ्ञथा {
            emitTraceInitOne(nodep, enum_अङ्कः);
            puts("\n");
        }
    }
    शून्य visit(AstTraceInc* nodep) override {
        सचे (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            अत्थाय (पूर्णाङ्क i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } अञ्ञथा {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(तार्किक slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->शिखरम्_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        अत्थाय (Ast_ग्रन्थिः* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        सचे (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = स्वभाव;

सार्वजनिक:
    स्थिर std::vector<AstCFile*> main(तार्किक slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        सचे (slow) {
            अत्थाय (AstCFile* ध्रुव cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        पटिनिवत्त cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

वग्ग EmitCFile final : सार्वजनिक EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य main(AstCFile* cfilep) VL_MT_STABLE {
        सचे (!cfilep->tblockp()) पटिनिवत्त;
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
        ध्रुव EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            सचे (VN_IS(nodep, Class)) अनुवर्त;  // Imped with ClassPackage
            ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = VN_AS(nodep, ग्रन्थिः_Module);
            सचे (modp->isConstPool()) अनुवर्त;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::main(modp, /* slow: */ सच्च); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::main(modp, /* slow: */ मिच्छा); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        सचे (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::main(/* slow: */ सच्च); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::main(/* slow: */ मिच्छा); });
        }
    }
    // Add files to netlist
    अत्थाय (ध्रुव std::vector<AstCFile*>& cfileps : cfiles) {
        अत्थाय (AstCFile* ध्रुव cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

शून्य V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    अत्थाय (Ast_ग्रन्थिः_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ग्रन्थिः_File);
        सचे (AstCFile* ध्रुव cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
