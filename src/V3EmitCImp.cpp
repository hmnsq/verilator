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
    ध्रुव Ast_गाठ_Module* ध्रुव m_fileModp;
    ध्रुव तार्किक m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    शून्य openNextOutputFile(ध्रुव std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, असत्य, "Design implementation internals");
        emitFileHeader();
    }
    शून्य emitFileHeader() {
        puts("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    शून्य emitParamDefns(ध्रुव Ast_गाठ_Module* modp) {
        तार्किक first = सत्य;
        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isParam()) {
                    यदि (first) {
                        puts("\n");
                        ध्रुव string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = असत्य;
                    }
                    यदि (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        यदि (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    स्थिर शून्य addVarCtorDtorStmts(TextStmts& stmts, ध्रुव AstVar* varp, तार्किक destroy) {
        ध्रुव std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        यदि (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            प्रति (ध्रुव AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                प्रति (ध्रुव std::string& elem : elems) {
                    प्रति (पूर्णाङ्क i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ढकला_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            यदि (destroy) std::reverse(elems.begin(), elems.end());
            प्रति (ध्रुव std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            प्रत्यागम;
        }

        // Other variables are constructed whole, even if aggregates
        यदि (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            प्रत्यागम;
        }
        ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        यदि (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } अन्यथा यदि (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->अंक().emitC();
        } अन्यथा यदि (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    शून्य createCtorDtorStmts(ध्रुव Ast_गाठ_Module* modp) {
        // Construct member variables
        प्रति (ध्रुव Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, असत्य);
            }
        }

        // Destroy in reverse order
        ध्रुव Ast_गाठ* ध्रुव lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        प्रति (ध्रुव Ast_गाठ* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, सत्य);
            }
        }

        // Split into sub-functions if needed
        ध्रुव std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", असत्य,
            "Design implementation internals", [एतत्]() { emitFileHeader(); });
    }
    शून्य emitCtorDtorImp(ध्रुव Ast_गाठ_Module* modp) {
        ध्रुव std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        यदि (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        यदि (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        यदि (modp->is_शीर्ष()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } अन्यथा {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        प्रति (ध्रुव TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        यदि (modp->ctorVarReset()) {
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
        यदि (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        प्रति (ध्रुव TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    शून्य emitConfigureImp(ध्रुव Ast_गाठ_Module* modp) {
        ध्रुव string modName = EmitCUtil::prefixNameProtect(modp);

        यदि (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        यदि (v3Global.opt.coverage()) {
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
        यदि (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            यदि (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अन्यथा {
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
        यदि (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            यदि (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            यदि (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } अन्यथा {
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
    शून्य emitSavableImp(ध्रुव Ast_गाठ_Module* modp) {
        यदि (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            प्रति (पूर्णाङ्क de = 0; de < 2; ++de) {
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
                प्रति (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                        hash.घाला(varp->name());
                        hash.घाला(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                यदि (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } अन्यथा {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                प्रति (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                        यदि (varp->isIO() && modp->is_शीर्ष() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } अन्यथा यदि (varp->isParam()) {
                        } अन्यथा यदि (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } अन्यथा {
                            पूर्णाङ्क vects = 0;
                            Ast_गाठ_DType* elementp = varp->dtypeSkipRefp();
                            प्रति (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
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
                            यदि (basicp && basicp->keyword().isMTaskState()) अनुवर्त;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            यदि (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ध्रुव पूर्णाङ्क vecnum = vects++;
                                ध्रुव string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_शब्द()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            प्रति (पूर्णाङ्क v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            प्रति (पूर्णाङ्क v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    तार्किक hasCommonImp(ध्रुव Ast_गाठ_Module* modp) ध्रुव {
        // Nothing to emit if no module!
        यदि (!modp) प्रत्यागम असत्य;
        // We always need the slow file
        यदि (m_slow) प्रत्यागम सत्य;
        // The fast file is only required when we have `systemc_implementation nodes
        प्रति (ध्रुव AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            यदि (scSectionp->sectionType() == VSystemCSectionType::IMP) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    // Actually emit common implementation contents for given AstNodeModule
    शून्य doCommonImp(ध्रुव Ast_गाठ_Module* modp) {
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
    शून्य emitCommonImp(ध्रुव Ast_गाठ_Module* modp) {
        ध्रुव AstClass* ध्रुव classp
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
    शून्य emitCFuncImp(ध्रुव Ast_गाठ_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ध्रुव auto gather = [एतत्, &funcps](ध्रुव Ast_गाठ_Module* modp) {
            प्रति (Ast_गाठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
                यदि (!funcp) अनुवर्त;
                // TRACE_* and DPI handled elsewhere
                यदि (funcp->isTrace()) अनुवर्त;
                यदि (funcp->dpiImportPrototype()) अनुवर्त;
                यदि (funcp->dpiExportDispatcher()) अनुवर्त;
                यदि (funcp->slow() != m_slow) अनुवर्त;
                funcps.ढकला_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        यदि (ध्रुव AstClassPackage* ध्रुव packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        यदि (funcps.empty()) प्रत्यागम;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        प्रति (AstCFunc* ध्रुव funcp : funcps) {
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

    explicit EmitCImp(ध्रुव Ast_गाठ_Module* modp, तार्किक slow)
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
    स्थिर std::vector<AstCFile*> main(ध्रुव Ast_गाठ_Module* modp, तार्किक slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        प्रत्यागम emitCImp.getAndClearCfileps();
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
    पूर्णाङ्क m_enum_अंक = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_गाठ*, पूर्णाङ्क> m_enum_अंक_Map;  // EnumDType to enumeration number
    पूर्णाङ्क m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ध्रुव size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    शून्य openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), सत्य, सत्य,
                                "Tracing declarations");
        puts("\n");
        प्रति (ध्रुव std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

सार्वजनिक:
    // METHODS
    पूर्णाङ्क get_Enum_Map_अंक(AstEnumDType* nodep) {
        पूर्णाङ्क& enumNumr = m_enum_अंक_Map[nodep];
        यदि (!enumNumr) {
            यदि (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_अंक;
            पूर्णाङ्क nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            प्रति (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                यदि (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            प्रति (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                यदि (++nvals > 1) puts(", ");
                putbs("\"" + constp->अंक().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_गाठ::INSTR_मोजणी_CALL);
        }
        प्रत्यागम enumNumr;
    }

    // Close output file
    शून्य finalize() {
        // Close function definition
        puts("}\n");

        ध्रुव std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ध्रुव std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        प्रति (पूर्णाङ्क i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        प्रति (पूर्णाङ्क i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

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
    ध्रुव तार्किक m_slow;  // Making slow file
    ध्रुव std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? नूतन EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ध्रुव std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "_" + protect("_Trace");

    // METHODS
    शून्य openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, सत्य,
                                "Tracing implementation internals");
        puts("\n");
        प्रति (ध्रुव std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    तार्किक emitTraceIsScBv(ध्रुव AstTraceInc* nodep) {
        ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        यदि (!varrefp) प्रत्यागम असत्य;
        ध्रुव AstVar* ध्रुव varp = varrefp->varp();
        प्रत्यागम varp->isSc() && varp->isScBv();
    }

    तार्किक emitTraceIsScBigUint(ध्रुव AstTraceInc* nodep) {
        ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        यदि (!varrefp) प्रत्यागम असत्य;
        ध्रुव AstVar* ध्रुव varp = varrefp->varp();
        प्रत्यागम varp->isSc() && varp->isScBigUint();
    }

    तार्किक emitTraceIsScUint(ध्रुव AstTraceInc* nodep) {
        ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        यदि (!varrefp) प्रत्यागम असत्य;
        ध्रुव AstVar* ध्रुव varp = varrefp->varp();
        प्रत्यागम varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    शून्य emitTraceInitOne(ध्रुव AstTraceDecl* nodep, पूर्णाङ्क enum_अंक) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ध्रुव callp = nodep->dtypeCallp();
        यदि (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            प्रत्यागम;
        }

        यदि (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } अन्यथा यदि (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } अन्यथा यदि (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } अन्यथा यदि (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } अन्यथा यदि (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } अन्यथा {
            puts("VL_TRACE_DECL_BIT");
        }

        यदि (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } अन्यथा {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        यदि (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_शब्द()));

        // Function index
        puts(",");
        यदि (nodep->inDtypeFunc()) {
            puts("fidx");
        } अन्यथा {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_शब्द_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_अंक));

        // Direction
        यदि (nodep->inDtypeFunc()) {
            puts(", direction");
        } अन्यथा {
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
        यदि (nodep->arrayRange().ranged()) {
            यदि (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } अन्यथा {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        यदि (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().डावा()) + ","
                 + cvtToStr(nodep->bitRange().उजवा()));
        }

        //
        puts(");");
    }

    पूर्णाङ्क emitTraceDeclDType(Ast_गाठ_DType* nodep) {
        // Return enum number or -1 for none
        यदि (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            यदि (AstEnumDType* ध्रुव enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                प्रत्यागम m_emitTypesp->get_Enum_Map_अंक(enump);
            }
        }
        प्रत्यागम -1;
    }

    शून्य emitTraceChangeOne(AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ध्रुव std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
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

        ध्रुव uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_शब्द());
        ध्रुव uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ध्रुव VNumRange& arrayRange = nodep->declp()->arrayRange();
        यदि (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_मूल्य(nodep, arrayindex);
        यदि (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    शून्य emit_Trace_मूल्य(ध्रुव AstTraceInc* nodep, पूर्णाङ्क arrayindex) {
        यदि (AstVarRef* ध्रुव varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ध्रुव AstVar* ध्रुव varp = varrefp->varp();
            यदि (varp->isEvent()) puts("&");
            puts("(");
            यदि (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } अन्यथा यदि (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            यदि (varp->isSc()) puts(".read()");
            यदि (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } अन्यथा यदि (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } अन्यथा यदि (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } अन्यथा {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    शून्य emitTraceIndex(ध्रुव AstTraceInc* ध्रुव nodep, पूर्णाङ्क arrayindex) {
        // Tracing only supports 1D arrays
        यदि (nodep->declp()->arrayRange().ranged()) {
            यदि (arrayindex == -2) {
                puts("[i]");
            } अन्यथा यदि (arrayindex == -1) {
                puts("[0]");
            } अन्यथा {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    शून्य visit(AstCFunc* nodep) override {
        यदि (!nodep->isTrace()) प्रत्यागम;
        यदि (nodep->slow() != m_slow) प्रत्यागम;

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
    शून्य visit(Ast_Trace_ढकला_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        यदि (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_शब्द_If(nodep->prefix(), nodep->protect()));
        } अन्यथा {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->डावा()));
        puts(", " + std::to_string(nodep->उजवा()));
        puts(");\n");
    }
    शून्य visit(Ast_Trace_काढा_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    शून्य visit(AstTraceDecl* nodep) override {
        ध्रुव पूर्णाङ्क enum_अंक = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        यदि (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_अंक);
            puts("\n}\n");
        } अन्यथा {
            emitTraceInitOne(nodep, enum_अंक);
            puts("\n");
        }
    }
    शून्य visit(AstTraceInc* nodep) override {
        यदि (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            प्रति (पूर्णाङ्क i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
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
        प्रति (Ast_गाठ* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
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
            प्रति (AstCFile* ध्रुव cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        प्रत्यागम cfileps;
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
        यदि (!cfilep->tblockp()) प्रत्यागम;
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
        प्रति (ध्रुव Ast_गाठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            यदि (VN_IS(nodep, Class)) अनुवर्त;  // Imped with ClassPackage
            ध्रुव Ast_गाठ_Module* ध्रुव modp = VN_AS(nodep, गाठ_Module);
            यदि (modp->isConstPool()) अनुवर्त;  // Emitted by V3EmitCConstPool
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
    प्रति (ध्रुव std::vector<AstCFile*>& cfileps : cfiles) {
        प्रति (AstCFile* ध्रुव cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

शून्य V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    प्रति (Ast_गाठ_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), गाठ_File);
        यदि (AstCFile* ध्रुव cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
