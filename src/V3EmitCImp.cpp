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

#شامل "V3PchAstMT.h"

#شامل "V3EmitC.h"
#شامل "V3EmitCFunc.h"
#شامل "V3ThreadPool.h"
#شامل "V3UniqueNames.h"

#شامل <algorithm>
#شامل <map>
#شامل <set>
#شامل <tuple>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

رده EmitCImp final : عمومی EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ثابت Ast_گره_Module* ثابت m_fileModp;
    ثابت منطقی m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    تهی openNextOutputFile(ثابت std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, نادرست, "Design implementation internals");
        emitFileHeader();
    }
    تهی emitFileHeader() {
        puts("// See " + EmitCUtil::بالاترین_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    تهی emitParamDefns(ثابت Ast_گره_Module* modp) {
        منطقی first = درست;
        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isParam()) {
                    اگر (first) {
                        puts("\n");
                        ثابت string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = نادرست;
                    }
                    اگر (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        اگر (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ایستا تهی addVarCtorDtorStmts(TextStmts& stmts, ثابت AstVar* varp, منطقی destroy) {
        ثابت std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        اگر (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            برای (ثابت AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                برای (ثابت std::string& elem : elems) {
                    برای (صحیح i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.افزودن_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            اگر (destroy) std::reverse(elems.begin(), elems.end());
            برای (ثابت std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            بازگشت;
        }

        // Other variables are constructed whole, even if aggregates
        اگر (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            بازگشت;
        }
        ثابت AstBasicDType* ثابت basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        اگر (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } وگرنه اگر (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->رقم().emitC();
        } وگرنه اگر (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    تهی createCtorDtorStmts(ثابت Ast_گره_Module* modp) {
        // Construct member variables
        برای (ثابت Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, نادرست);
            }
        }

        // Destroy in reverse order
        ثابت Ast_گره* ثابت lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        برای (ثابت Ast_گره* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            اگر (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, درست);
            }
        }

        // Split into sub-functions if needed
        ثابت std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", نادرست,
            "Design implementation internals", [این]() { emitFileHeader(); });
    }
    تهی emitCtorDtorImp(ثابت Ast_گره_Module* modp) {
        ثابت std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        اگر (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        اگر (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        اگر (modp->is_بالاترین()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } وگرنه {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        برای (ثابت TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        اگر (modp->ctorVarReset()) {
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
        اگر (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        برای (ثابت TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    تهی emitConfigureImp(ثابت Ast_گره_Module* modp) {
        ثابت string modName = EmitCUtil::prefixNameProtect(modp);

        اگر (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        اگر (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    تهی emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        اگر (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            اگر (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } وگرنه {
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
        اگر (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            اگر (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            اگر (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } وگرنه {
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
    تهی emitSavableImp(ثابت Ast_گره_Module* modp) {
        اگر (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            برای (صحیح de = 0; de < 2; ++de) {
                ثابت string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                ثابت string funcname = de ? "__Vdeserialize" : "__Vserialize";
                ثابت string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                برای (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        hash.درج(varp->name());
                        hash.درج(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                اگر (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } وگرنه {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                برای (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        اگر (varp->isIO() && modp->is_بالاترین() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } وگرنه اگر (varp->isParam()) {
                        } وگرنه اگر (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } وگرنه {
                            صحیح vects = 0;
                            Ast_گره_DType* elementp = varp->dtypeSkipRefp();
                            برای (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ثابت صحیح vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                ثابت string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            ثابت AstBasicDType* ثابت basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            اگر (basicp && basicp->keyword().isMTaskState()) ادامه;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            اگر (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ثابت صحیح vecnum = vects++;
                                ثابت string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_واژگان()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            برای (صحیح v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            برای (صحیح v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    منطقی hasCommonImp(ثابت Ast_گره_Module* modp) ثابت {
        // Nothing to emit if no module!
        اگر (!modp) بازگشت نادرست;
        // We always need the slow file
        اگر (m_slow) بازگشت درست;
        // The fast file is only required when we have `systemc_implementation nodes
        برای (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            اگر (scSectionp->sectionType() == VSystemCSectionType::IMP) بازگشت درست;
        }
        بازگشت نادرست;
    }
    // Actually emit common implementation contents for given AstNodeModule
    تهی doCommonImp(ثابت Ast_گره_Module* modp) {
        اگر (m_slow) {
            اگر (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } وگرنه {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    تهی emitCommonImp(ثابت Ast_گره_Module* modp) {
        ثابت AstClass* ثابت classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        اگر (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            اگر (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            اگر (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    تهی emitCFuncImp(ثابت Ast_گره_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ثابت auto gather = [این, &funcps](ثابت Ast_گره_Module* modp) {
            برای (Ast_گره* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
                اگر (!funcp) ادامه;
                // TRACE_* and DPI handled elsewhere
                اگر (funcp->isTrace()) ادامه;
                اگر (funcp->dpiImportPrototype()) ادامه;
                اگر (funcp->dpiExportDispatcher()) ادامه;
                اگر (funcp->slow() != m_slow) ادامه;
                funcps.افزودن_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        اگر (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        اگر (funcps.empty()) بازگشت;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        برای (AstCFunc* ثابت funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    تهی visit(AstCFunc* nodep) override {
        اگر (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(درست);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ثابت Ast_گره_Module* modp, منطقی slow)
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
    ~EmitCImp() override = پیشفرض;

عمومی:
    ایستا std::vector<AstCFile*> اصلی(ثابت Ast_گره_Module* modp, منطقی slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        بازگشت emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
رده EmitCTraceTypes final : عمومی EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    صحیح m_enum_رقم = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_گره*, صحیح> m_enum_رقم_Map;  // EnumDType to enumeration number
    صحیح m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::بالاترین_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ثابت size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    تهی openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), درست, درست,
                                "Tracing declarations");
        puts("\n");
        برای (ثابت std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

عمومی:
    // METHODS
    صحیح get_Enum_Map_رقم(AstEnumDType* nodep) {
        صحیح& enumNumr = m_enum_رقم_Map[nodep];
        اگر (!enumNumr) {
            اگر (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(درست);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_رقم;
            صحیح nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            برای (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                اگر (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            برای (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                اگر (++nvals > 1) puts(", ");
                putbs("\"" + constp->رقم().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_گره::INSTR_شمار_CALL);
        }
        بازگشت enumNumr;
    }

    // Close output file
    تهی finalize() {
        // Close function definition
        puts("}\n");

        ثابت std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ثابت std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        برای (صحیح i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        برای (صحیح i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->بالاترین_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = پیشفرض;
};

رده EmitCTrace final : عمومی EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ثابت منطقی m_slow;  // Making slow file
    ثابت std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? جدید EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::بالاترین_Class_Name() + "_" + protect("_Trace");

    // METHODS
    تهی openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, درست,
                                "Tracing implementation internals");
        puts("\n");
        برای (ثابت std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    منطقی emitTraceIsScBv(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        اگر (!varrefp) بازگشت نادرست;
        ثابت AstVar* ثابت varp = varrefp->varp();
        بازگشت varp->isSc() && varp->isScBv();
    }

    منطقی emitTraceIsScBigUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        اگر (!varrefp) بازگشت نادرست;
        ثابت AstVar* ثابت varp = varrefp->varp();
        بازگشت varp->isSc() && varp->isScBigUint();
    }

    منطقی emitTraceIsScUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        اگر (!varrefp) بازگشت نادرست;
        ثابت AstVar* ثابت varp = varrefp->varp();
        بازگشت varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    تهی emitTraceInitOne(ثابت AstTraceDecl* nodep, صحیح enum_رقم) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ثابت callp = nodep->dtypeCallp();
        اگر (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            بازگشت;
        }

        اگر (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } وگرنه اگر (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } وگرنه اگر (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } وگرنه اگر (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } وگرنه اگر (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } وگرنه {
            puts("VL_TRACE_DECL_BIT");
        }

        اگر (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } وگرنه {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        اگر (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_واژگان()));

        // Function index
        puts(",");
        اگر (nodep->inDtypeFunc()) {
            puts("fidx");
        } وگرنه {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_واژگان_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_رقم));

        // Direction
        اگر (nodep->inDtypeFunc()) {
            puts(", direction");
        } وگرنه {
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
        اگر (nodep->arrayRange().ranged()) {
            اگر (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } وگرنه {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        اگر (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().چپ()) + ","
                 + cvtToStr(nodep->bitRange().راست()));
        }

        //
        puts(");");
    }

    صحیح emitTraceDeclDType(Ast_گره_DType* nodep) {
        // Return enum number or -1 for none
        اگر (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            اگر (AstEnumDType* ثابت enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                بازگشت m_emitTypesp->get_Enum_Map_رقم(enump);
            }
        }
        بازگشت -1;
    }

    تهی emitTraceChangeOne(AstTraceInc* nodep, صحیح arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ثابت std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        منطقی emitWidth = درست;
        string stype;
        اگر (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = نادرست;
        } وگرنه اگر (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } وگرنه اگر (nodep->isQuad()) {
            stype = "QData";
        } وگرنه اگر (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } وگرنه اگر (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } وگرنه اگر (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } وگرنه اگر (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = نادرست;
        } وگرنه {
            stype = "Bit";
            emitWidth = نادرست;
        }
        putns(nodep, "bufp->" + func + stype);

        ثابت uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_واژگان());
        ثابت uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ثابت VNumRange& arrayRange = nodep->declp()->arrayRange();
        اگر (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_مقدار(nodep, arrayindex);
        اگر (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    تهی emit_Trace_مقدار(ثابت AstTraceInc* nodep, صحیح arrayindex) {
        اگر (AstVarRef* ثابت varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ثابت AstVar* ثابت varp = varrefp->varp();
            اگر (varp->isEvent()) puts("&");
            puts("(");
            اگر (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } وگرنه اگر (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            اگر (varp->isSc()) puts(".read()");
            اگر (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } وگرنه اگر (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } وگرنه اگر (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } وگرنه {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    تهی emitTraceIndex(ثابت AstTraceInc* ثابت nodep, صحیح arrayindex) {
        // Tracing only supports 1D arrays
        اگر (nodep->declp()->arrayRange().ranged()) {
            اگر (arrayindex == -2) {
                puts("[i]");
            } وگرنه اگر (arrayindex == -1) {
                puts("[0]");
            } وگرنه {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    تهی visit(AstCFunc* nodep) override {
        اگر (!nodep->isTrace()) بازگشت;
        اگر (nodep->slow() != m_slow) بازگشت;

        اگر (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(درست);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    تهی visit(Ast_Trace_افزودن_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        اگر (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_واژگان_If(nodep->prefix(), nodep->protect()));
        } وگرنه {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->چپ()));
        puts(", " + std::to_string(nodep->راست()));
        puts(");\n");
    }
    تهی visit(Ast_Trace_برداشت_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    تهی visit(AstTraceDecl* nodep) override {
        ثابت صحیح enum_رقم = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        اگر (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_رقم);
            puts("\n}\n");
        } وگرنه {
            emitTraceInitOne(nodep, enum_رقم);
            puts("\n");
        }
    }
    تهی visit(AstTraceInc* nodep) override {
        اگر (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            برای (صحیح i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } وگرنه {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(منطقی slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->بالاترین_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        برای (Ast_گره* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            اگر (AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        اگر (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = پیشفرض;

عمومی:
    ایستا std::vector<AstCFile*> اصلی(منطقی slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        اگر (slow) {
            برای (AstCFile* ثابت cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        بازگشت cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

رده EmitCFile final : عمومی EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = پیشفرض;

عمومی:
    ایستا تهی اصلی(AstCFile* cfilep) VL_MT_STABLE {
        اگر (!cfilep->tblockp()) بازگشت;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

تهی V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ثابت EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        برای (ثابت Ast_گره* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            اگر (VN_IS(nodep, Class)) ادامه;  // Imped with ClassPackage
            ثابت Ast_گره_Module* ثابت modp = VN_AS(nodep, گره_Module);
            اگر (modp->isConstPool()) ادامه;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::اصلی(modp, /* slow: */ درست); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::اصلی(modp, /* slow: */ نادرست); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        اگر (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::اصلی(/* slow: */ درست); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::اصلی(/* slow: */ نادرست); });
        }
    }
    // Add files to netlist
    برای (ثابت std::vector<AstCFile*>& cfileps : cfiles) {
        برای (AstCFile* ثابت cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

تهی V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    برای (Ast_گره_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), گره_File);
        اگر (AstCFile* ثابت cfilep = VN_CAST(filep, CFile)) EmitCFile::اصلی(cfilep);
    }
}
