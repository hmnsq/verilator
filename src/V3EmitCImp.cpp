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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3EmitC.h"
#அந்தர்பாவ "V3EmitCFunc.h"
#அந்தர்பாவ "V3ThreadPool.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <tuple>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

வகுப்பு EmitCImp final : பொது EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    மாறிலி Ast_முனை_Module* மாறிலி m_fileModp;
    மாறிலி தருக்க m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    மாறிலி std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    வெறுமை openNextOutputFile(மாறிலி std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, பொய், "Design implementation internals");
        emitFileHeader();
    }
    வெறுமை emitFileHeader() {
        puts("// See " + EmitCUtil::உச்சி_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    வெறுமை emitParamDefns(மாறிலி Ast_முனை_Module* modp) {
        தருக்க first = உண்மை;
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isParam()) {
                    எனில் (first) {
                        puts("\n");
                        மாறிலி string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = பொய்;
                    }
                    எனில் (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        எனில் (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    நிலையான வெறுமை addVarCtorDtorStmts(TextStmts& stmts, மாறிலி AstVar* varp, தருக்க destroy) {
        மாறிலி std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        எனில் (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            க்கு (மாறிலி AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                க்கு (மாறிலி std::string& elem : elems) {
                    க்கு (முழு i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.தள்ளு_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            எனில் (destroy) std::reverse(elems.begin(), elems.end());
            க்கு (மாறிலி std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            திரும்பு;
        }

        // Other variables are constructed whole, even if aggregates
        எனில் (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            திரும்பு;
        }
        மாறிலி AstBasicDType* மாறிலி basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        எனில் (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } இல்லையெனில் எனில் (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->இலக்கம்().emitC();
        } இல்லையெனில் எனில் (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    வெறுமை createCtorDtorStmts(மாறிலி Ast_முனை_Module* modp) {
        // Construct member variables
        க்கு (மாறிலி Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var);
            எனில் (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, பொய்);
            }
        }

        // Destroy in reverse order
        மாறிலி Ast_முனை* மாறிலி lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        க்கு (மாறிலி Ast_முனை* nodep = lastp; nodep; nodep = nodep->prevp()) {
            மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var);
            எனில் (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, உண்மை);
            }
        }

        // Split into sub-functions if needed
        மாறிலி std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", பொய்,
            "Design implementation internals", [இது]() { emitFileHeader(); });
    }
    வெறுமை emitCtorDtorImp(மாறிலி Ast_முனை_Module* modp) {
        மாறிலி std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        எனில் (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        எனில் (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        எனில் (modp->is_உச்சி()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } இல்லையெனில் {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        க்கு (மாறிலி TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        எனில் (modp->ctorVarReset()) {
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
        எனில் (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        க்கு (மாறிலி TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    வெறுமை emitConfigureImp(மாறிலி Ast_முனை_Module* modp) {
        மாறிலி string modName = EmitCUtil::prefixNameProtect(modp);

        எனில் (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        எனில் (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    வெறுமை emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        எனில் (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            எனில் (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } இல்லையெனில் {
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
        எனில் (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            எனில் (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            எனில் (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } இல்லையெனில் {
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
    வெறுமை emitSavableImp(மாறிலி Ast_முனை_Module* modp) {
        எனில் (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            க்கு (முழு de = 0; de < 2; ++de) {
                மாறிலி string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                மாறிலி string funcname = de ? "__Vdeserialize" : "__Vserialize";
                மாறிலி string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                        hash.செருகு(varp->name());
                        hash.செருகு(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                எனில் (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } இல்லையெனில் {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                        எனில் (varp->isIO() && modp->is_உச்சி() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } இல்லையெனில் எனில் (varp->isParam()) {
                        } இல்லையெனில் எனில் (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } இல்லையெனில் {
                            முழு vects = 0;
                            Ast_முனை_DType* elementp = varp->dtypeSkipRefp();
                            க்கு (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                மாறிலி முழு vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                மாறிலி string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            மாறிலி AstBasicDType* மாறிலி basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            எனில் (basicp && basicp->keyword().isMTaskState()) தொடர்;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            எனில் (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                மாறிலி முழு vecnum = vects++;
                                மாறிலி string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_சொற்கள்()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            க்கு (முழு v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            க்கு (முழு v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    தருக்க hasCommonImp(மாறிலி Ast_முனை_Module* modp) மாறிலி {
        // Nothing to emit if no module!
        எனில் (!modp) திரும்பு பொய்;
        // We always need the slow file
        எனில் (m_slow) திரும்பு உண்மை;
        // The fast file is only required when we have `systemc_implementation nodes
        க்கு (மாறிலி AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            எனில் (scSectionp->sectionType() == VSystemCSectionType::IMP) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    // Actually emit common implementation contents for given AstNodeModule
    வெறுமை doCommonImp(மாறிலி Ast_முனை_Module* modp) {
        எனில் (m_slow) {
            எனில் (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } இல்லையெனில் {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    வெறுமை emitCommonImp(மாறிலி Ast_முனை_Module* modp) {
        மாறிலி AstClass* மாறிலி classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        எனில் (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            எனில் (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            எனில் (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    வெறுமை emitCFuncImp(மாறிலி Ast_முனை_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        மாறிலி auto gather = [இது, &funcps](மாறிலி Ast_முனை_Module* modp) {
            க்கு (Ast_முனை* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc);
                எனில் (!funcp) தொடர்;
                // TRACE_* and DPI handled elsewhere
                எனில் (funcp->isTrace()) தொடர்;
                எனில் (funcp->dpiImportPrototype()) தொடர்;
                எனில் (funcp->dpiExportDispatcher()) தொடர்;
                எனில் (funcp->slow() != m_slow) தொடர்;
                funcps.தள்ளு_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        எனில் (மாறிலி AstClassPackage* மாறிலி packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        எனில் (funcps.empty()) திரும்பு;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        க்கு (AstCFunc* மாறிலி funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(உண்மை);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(மாறிலி Ast_முனை_Module* modp, தருக்க slow)
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
    ~EmitCImp() override = இயல்பு;

பொது:
    நிலையான std::vector<AstCFile*> முதன்மை(மாறிலி Ast_முனை_Module* modp, தருக்க slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        திரும்பு emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
வகுப்பு EmitCTraceTypes final : பொது EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    முழு m_enum_இலக்கம் = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_முனை*, முழு> m_enum_இலக்கம்_Map;  // EnumDType to enumeration number
    முழு m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    மாறிலி std::string m_fileBaseName = EmitCUtil::உச்சி_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    மாறிலி size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    வெறுமை openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), உண்மை, உண்மை,
                                "Tracing declarations");
        puts("\n");
        க்கு (மாறிலி std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

பொது:
    // METHODS
    முழு get_Enum_Map_இலக்கம்(AstEnumDType* nodep) {
        முழு& enumNumr = m_enum_இலக்கம்_Map[nodep];
        எனில் (!enumNumr) {
            எனில் (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(உண்மை);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_இலக்கம்;
            முழு nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            க்கு (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                எனில் (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            க்கு (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* மாறிலி constp = VN_AS(itemp->valuep(), Const);
                எனில் (++nvals > 1) puts(", ");
                putbs("\"" + constp->இலக்கம்().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_முனை::INSTR_மொத்தம்_CALL);
        }
        திரும்பு enumNumr;
    }

    // Close output file
    வெறுமை finalize() {
        // Close function definition
        puts("}\n");

        மாறிலி std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        மாறிலி std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        க்கு (முழு i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        க்கு (முழு i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->உச்சி_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = இயல்பு;
};

வகுப்பு EmitCTrace final : பொது EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    மாறிலி தருக்க m_slow;  // Making slow file
    மாறிலி std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? புதிய EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    மாறிலி std::string m_fileBaseName = EmitCUtil::உச்சி_Class_Name() + "_" + protect("_Trace");

    // METHODS
    வெறுமை openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, உண்மை,
                                "Tracing implementation internals");
        puts("\n");
        க்கு (மாறிலி std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    தருக்க emitTraceIsScBv(மாறிலி AstTraceInc* nodep) {
        மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        எனில் (!varrefp) திரும்பு பொய்;
        மாறிலி AstVar* மாறிலி varp = varrefp->varp();
        திரும்பு varp->isSc() && varp->isScBv();
    }

    தருக்க emitTraceIsScBigUint(மாறிலி AstTraceInc* nodep) {
        மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        எனில் (!varrefp) திரும்பு பொய்;
        மாறிலி AstVar* மாறிலி varp = varrefp->varp();
        திரும்பு varp->isSc() && varp->isScBigUint();
    }

    தருக்க emitTraceIsScUint(மாறிலி AstTraceInc* nodep) {
        மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        எனில் (!varrefp) திரும்பு பொய்;
        மாறிலி AstVar* மாறிலி varp = varrefp->varp();
        திரும்பு varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    வெறுமை emitTraceInitOne(மாறிலி AstTraceDecl* nodep, முழு enum_இலக்கம்) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* மாறிலி callp = nodep->dtypeCallp();
        எனில் (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            திரும்பு;
        }

        எனில் (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } இல்லையெனில் எனில் (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } இல்லையெனில் எனில் (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } இல்லையெனில் எனில் (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } இல்லையெனில் எனில் (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } இல்லையெனில் {
            puts("VL_TRACE_DECL_BIT");
        }

        எனில் (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } இல்லையெனில் {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        எனில் (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_சொற்கள்()));

        // Function index
        puts(",");
        எனில் (nodep->inDtypeFunc()) {
            puts("fidx");
        } இல்லையெனில் {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_இலக்கம்));

        // Direction
        எனில் (nodep->inDtypeFunc()) {
            puts(", direction");
        } இல்லையெனில் {
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
        எனில் (nodep->arrayRange().ranged()) {
            எனில் (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } இல்லையெனில் {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        எனில் (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().இடது()) + ","
                 + cvtToStr(nodep->bitRange().வலது()));
        }

        //
        puts(");");
    }

    முழு emitTraceDeclDType(Ast_முனை_DType* nodep) {
        // Return enum number or -1 for none
        எனில் (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            எனில் (AstEnumDType* மாறிலி enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                திரும்பு m_emitTypesp->get_Enum_Map_இலக்கம்(enump);
            }
        }
        திரும்பு -1;
    }

    வெறுமை emitTraceChangeOne(AstTraceInc* nodep, முழு arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        மாறிலி std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        தருக்க emitWidth = உண்மை;
        string stype;
        எனில் (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = பொய்;
        } இல்லையெனில் எனில் (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } இல்லையெனில் எனில் (nodep->isQuad()) {
            stype = "QData";
        } இல்லையெனில் எனில் (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } இல்லையெனில் எனில் (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } இல்லையெனில் எனில் (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } இல்லையெனில் எனில் (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = பொய்;
        } இல்லையெனில் {
            stype = "Bit";
            emitWidth = பொய்;
        }
        putns(nodep, "bufp->" + func + stype);

        மாறிலி uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_சொற்கள்());
        மாறிலி uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        மாறிலி VNumRange& arrayRange = nodep->declp()->arrayRange();
        எனில் (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_மதிப்பு(nodep, arrayindex);
        எனில் (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    வெறுமை emit_Trace_மதிப்பு(மாறிலி AstTraceInc* nodep, முழு arrayindex) {
        எனில் (AstVarRef* மாறிலி varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            மாறிலி AstVar* மாறிலி varp = varrefp->varp();
            எனில் (varp->isEvent()) puts("&");
            puts("(");
            எனில் (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } இல்லையெனில் எனில் (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            எனில் (varp->isSc()) puts(".read()");
            எனில் (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } இல்லையெனில் எனில் (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } இல்லையெனில் எனில் (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } இல்லையெனில் {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    வெறுமை emitTraceIndex(மாறிலி AstTraceInc* மாறிலி nodep, முழு arrayindex) {
        // Tracing only supports 1D arrays
        எனில் (nodep->declp()->arrayRange().ranged()) {
            எனில் (arrayindex == -2) {
                puts("[i]");
            } இல்லையெனில் எனில் (arrayindex == -1) {
                puts("[0]");
            } இல்லையெனில் {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (!nodep->isTrace()) திரும்பு;
        எனில் (nodep->slow() != m_slow) திரும்பு;

        எனில் (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(உண்மை);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    வெறுமை visit(Ast_Trace_தள்ளு_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        எனில் (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_சொற்கள்_If(nodep->prefix(), nodep->protect()));
        } இல்லையெனில் {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->இடது()));
        puts(", " + std::to_string(nodep->வலது()));
        puts(");\n");
    }
    வெறுமை visit(Ast_Trace_எடு_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    வெறுமை visit(AstTraceDecl* nodep) override {
        மாறிலி முழு enum_இலக்கம் = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        எனில் (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_இலக்கம்);
            puts("\n}\n");
        } இல்லையெனில் {
            emitTraceInitOne(nodep, enum_இலக்கம்);
            puts("\n");
        }
    }
    வெறுமை visit(AstTraceInc* nodep) override {
        எனில் (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            க்கு (முழு i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } இல்லையெனில் {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(தருக்க slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->உச்சி_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        க்கு (Ast_முனை* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            எனில் (AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        எனில் (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = இயல்பு;

பொது:
    நிலையான std::vector<AstCFile*> முதன்மை(தருக்க slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        எனில் (slow) {
            க்கு (AstCFile* மாறிலி cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        திரும்பு cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

வகுப்பு EmitCFile final : பொது EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = இயல்பு;

பொது:
    நிலையான வெறுமை முதன்மை(AstCFile* cfilep) VL_MT_STABLE {
        எனில் (!cfilep->tblockp()) திரும்பு;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

வெறுமை V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        மாறிலி EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        க்கு (மாறிலி Ast_முனை* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            எனில் (VN_IS(nodep, Class)) தொடர்;  // Imped with ClassPackage
            மாறிலி Ast_முனை_Module* மாறிலி modp = VN_AS(nodep, முனை_Module);
            எனில் (modp->isConstPool()) தொடர்;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::முதன்மை(modp, /* slow: */ உண்மை); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::முதன்மை(modp, /* slow: */ பொய்); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        எனில் (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::முதன்மை(/* slow: */ உண்மை); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::முதன்மை(/* slow: */ பொய்); });
        }
    }
    // Add files to netlist
    க்கு (மாறிலி std::vector<AstCFile*>& cfileps : cfiles) {
        க்கு (AstCFile* மாறிலி cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

வெறுமை V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    க்கு (Ast_முனை_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), முனை_File);
        எனில் (AstCFile* மாறிலி cfilep = VN_CAST(filep, CFile)) EmitCFile::முதன்மை(cfilep);
    }
}
