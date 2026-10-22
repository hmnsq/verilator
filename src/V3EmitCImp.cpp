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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitC.h"
#ضمن "V3EmitCFunc.h"
#ضمن "V3ThreadPool.h"
#ضمن "V3UniqueNames.h"

#ضمن <algorithm>
#ضمن <map>
#ضمن <set>
#ضمن <tuple>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

صنف EmitCImp final : عام EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    ثابت AstNodeModule* ثابت m_fileModp;
    ثابت منطقي m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    فراغ openNextOutputFile(ثابت std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, خطأ, "Design implementation internals");
        emitFileHeader();
    }
    فراغ emitFileHeader() {
        puts("// See " + EmitCUtil::topClassName() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    فراغ emitParamDefns(ثابت AstNodeModule* modp) {
        منطقي first = true;
        لكل (ثابت AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                𐤀𐤌 (varp->isParam()) {
                    𐤀𐤌 (first) {
                        puts("\n");
                        ثابت string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = خطأ;
                    }
                    𐤀𐤌 (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        𐤀𐤌 (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    ساكن فراغ addVarCtorDtorStmts(TextStmts& stmts, ثابت AstVar* varp, منطقي destroy) {
        ثابت std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        𐤀𐤌 (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            لكل (ثابت AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                لكل (ثابت std::string& elem : elems) {
                    لكل (صحيح i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.push_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            𐤀𐤌 (destroy) std::reverse(elems.begin(), elems.end());
            لكل (ثابت std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            أرجع;
        }

        // Other variables are constructed whole, even if aggregates
        𐤀𐤌 (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            أرجع;
        }
        ثابت AstBasicDType* ثابت basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        𐤀𐤌 (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } وإلا 𐤀𐤌 (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->num().emitC();
        } وإلا 𐤀𐤌 (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    فراغ createCtorDtorStmts(ثابت AstNodeModule* modp) {
        // Construct member variables
        لكل (ثابت AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            𐤀𐤌 (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, خطأ);
            }
        }

        // Destroy in reverse order
        ثابت AstNode* ثابت lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        لكل (ثابت AstNode* nodep = lastp; nodep; nodep = nodep->prevp()) {
            ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
            𐤀𐤌 (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, true);
            }
        }

        // Split into sub-functions if needed
        ثابت std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", خطأ,
            "Design implementation internals", [هذا]() { emitFileHeader(); });
    }
    فراغ emitCtorDtorImp(ثابت AstNodeModule* modp) {
        ثابت std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        𐤀𐤌 (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        𐤀𐤌 (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        𐤀𐤌 (modp->isTop()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } وإلا {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        لكل (ثابت TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        𐤀𐤌 (modp->ctorVarReset()) {
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
        𐤀𐤌 (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        لكل (ثابت TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    فراغ emitConfigureImp(ثابت AstNodeModule* modp) {
        ثابت string modName = EmitCUtil::prefixNameProtect(modp);

        𐤀𐤌 (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        𐤀𐤌 (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    فراغ emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        𐤀𐤌 (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            𐤀𐤌 (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } وإلا {
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
        𐤀𐤌 (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            𐤀𐤌 (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            𐤀𐤌 (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } وإلا {
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
    فراغ emitSavableImp(ثابت AstNodeModule* modp) {
        𐤀𐤌 (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            لكل (صحيح de = 0; de < 2; ++de) {
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
                لكل (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        hash.insert(varp->name());
                        hash.insert(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                𐤀𐤌 (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } وإلا {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                لكل (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                        𐤀𐤌 (varp->isIO() && modp->isTop() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } وإلا 𐤀𐤌 (varp->isParam()) {
                        } وإلا 𐤀𐤌 (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } وإلا {
                            صحيح vects = 0;
                            AstNodeDType* elementp = varp->dtypeSkipRefp();
                            لكل (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                ثابت صحيح vecnum = vects++;
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
                            𐤀𐤌 (basicp && basicp->keyword().isMTaskState()) استمر;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            𐤀𐤌 (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                ثابت صحيح vecnum = vects++;
                                ثابت string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->widthWords()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            لكل (صحيح v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            لكل (صحيح v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    منطقي hasCommonImp(ثابت AstNodeModule* modp) ثابت {
        // Nothing to emit if no module!
        𐤀𐤌 (!modp) أرجع خطأ;
        // We always need the slow file
        𐤀𐤌 (m_slow) أرجع true;
        // The fast file is only required when we have `systemc_implementation nodes
        لكل (ثابت AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            𐤀𐤌 (scSectionp->sectionType() == VSystemCSectionType::IMP) أرجع true;
        }
        أرجع خطأ;
    }
    // Actually emit common implementation contents for given AstNodeModule
    فراغ doCommonImp(ثابت AstNodeModule* modp) {
        𐤀𐤌 (m_slow) {
            𐤀𐤌 (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } وإلا {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    فراغ emitCommonImp(ثابت AstNodeModule* modp) {
        ثابت AstClass* ثابت classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        𐤀𐤌 (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            𐤀𐤌 (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            𐤀𐤌 (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    فراغ emitCFuncImp(ثابت AstNodeModule* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        ثابت auto gather = [هذا, &funcps](ثابت AstNodeModule* modp) {
            لكل (AstNode* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
                𐤀𐤌 (!funcp) استمر;
                // TRACE_* and DPI handled elsewhere
                𐤀𐤌 (funcp->isTrace()) استمر;
                𐤀𐤌 (funcp->dpiImportPrototype()) استمر;
                𐤀𐤌 (funcp->dpiExportDispatcher()) استمر;
                𐤀𐤌 (funcp->slow() != m_slow) استمر;
                funcps.push_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        𐤀𐤌 (ثابت AstClassPackage* ثابت packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        𐤀𐤌 (funcps.empty()) أرجع;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        لكل (AstCFunc* ثابت funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        𐤀𐤌 (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(true);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(ثابت AstNodeModule* modp, منطقي slow)
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
    ~EmitCImp() override = افتراضي;

عام:
    ساكن std::vector<AstCFile*> main(ثابت AstNodeModule* modp, منطقي slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        أرجع emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
صنف EmitCTraceTypes final : عام EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    صحيح m_enumNum = 0;  // Enumeration number (whole netlist)
    std::unordered_map<AstNode*, صحيح> m_enumNumMap;  // EnumDType to enumeration number
    صحيح m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::topClassName() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    ثابت size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    فراغ openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), true, true,
                                "Tracing declarations");
        puts("\n");
        لكل (ثابت std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

عام:
    // METHODS
    صحيح getEnumMapNum(AstEnumDType* nodep) {
        صحيح& enumNumr = m_enumNumMap[nodep];
        𐤀𐤌 (!enumNumr) {
            𐤀𐤌 (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(true);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enumNum;
            صحيح nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            لكل (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                𐤀𐤌 (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            لكل (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                𐤀𐤌 (++nvals > 1) puts(", ");
                putbs("\"" + constp->num().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(AstNode::INSTR_COUNT_CALL);
        }
        أرجع enumNumr;
    }

    // Close output file
    فراغ finalize() {
        // Close function definition
        puts("}\n");

        ثابت std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        ثابت std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        لكل (صحيح i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        لكل (صحيح i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->topModulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = افتراضي;
};

صنف EmitCTrace final : عام EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    ثابت منطقي m_slow;  // Making slow file
    ثابت std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? جديد EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    ثابت std::string m_fileBaseName = EmitCUtil::topClassName() + "_" + protect("_Trace");

    // METHODS
    فراغ openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, true,
                                "Tracing implementation internals");
        puts("\n");
        لكل (ثابت std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    منطقي emitTraceIsScBv(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        𐤀𐤌 (!varrefp) أرجع خطأ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        أرجع varp->isSc() && varp->isScBv();
    }

    منطقي emitTraceIsScBigUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        𐤀𐤌 (!varrefp) أرجع خطأ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        أرجع varp->isSc() && varp->isScBigUint();
    }

    منطقي emitTraceIsScUint(ثابت AstTraceInc* nodep) {
        ثابت AstVarRef* ثابت varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        𐤀𐤌 (!varrefp) أرجع خطأ;
        ثابت AstVar* ثابت varp = varrefp->varp();
        أرجع varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    فراغ emitTraceInitOne(ثابت AstTraceDecl* nodep, صحيح enumNum) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* ثابت callp = nodep->dtypeCallp();
        𐤀𐤌 (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            أرجع;
        }

        𐤀𐤌 (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } وإلا 𐤀𐤌 (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } وإلا 𐤀𐤌 (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } وإلا 𐤀𐤌 (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } وإلا 𐤀𐤌 (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } وإلا {
            puts("VL_TRACE_DECL_BIT");
        }

        𐤀𐤌 (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } وإلا {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        𐤀𐤌 (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->widthWords()));

        // Function index
        puts(",");
        𐤀𐤌 (nodep->inDtypeFunc()) {
            puts("fidx");
        } وإلا {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protectWordsIf(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enumNum));

        // Direction
        𐤀𐤌 (nodep->inDtypeFunc()) {
            puts(", direction");
        } وإلا {
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
        𐤀𐤌 (nodep->arrayRange().ranged()) {
            𐤀𐤌 (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } وإلا {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        𐤀𐤌 (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().left()) + ","
                 + cvtToStr(nodep->bitRange().right()));
        }

        //
        puts(");");
    }

    صحيح emitTraceDeclDType(AstNodeDType* nodep) {
        // Return enum number or -1 for none
        𐤀𐤌 (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            𐤀𐤌 (AstEnumDType* ثابت enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                أرجع m_emitTypesp->getEnumMapNum(enump);
            }
        }
        أرجع -1;
    }

    فراغ emitTraceChangeOne(AstTraceInc* nodep, صحيح arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        ثابت std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        منطقي emitWidth = true;
        string stype;
        𐤀𐤌 (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = خطأ;
        } وإلا 𐤀𐤌 (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } وإلا 𐤀𐤌 (nodep->isQuad()) {
            stype = "QData";
        } وإلا 𐤀𐤌 (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } وإلا 𐤀𐤌 (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } وإلا 𐤀𐤌 (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } وإلا 𐤀𐤌 (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = خطأ;
        } وإلا {
            stype = "Bit";
            emitWidth = خطأ;
        }
        putns(nodep, "bufp->" + func + stype);

        ثابت uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->widthWords());
        ثابت uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        ثابت VNumRange& arrayRange = nodep->declp()->arrayRange();
        𐤀𐤌 (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emitTraceValue(nodep, arrayindex);
        𐤀𐤌 (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    فراغ emitTraceValue(ثابت AstTraceInc* nodep, صحيح arrayindex) {
        𐤀𐤌 (AstVarRef* ثابت varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            ثابت AstVar* ثابت varp = varrefp->varp();
            𐤀𐤌 (varp->isEvent()) puts("&");
            puts("(");
            𐤀𐤌 (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } وإلا 𐤀𐤌 (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            𐤀𐤌 (varp->isSc()) puts(".read()");
            𐤀𐤌 (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } وإلا 𐤀𐤌 (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } وإلا 𐤀𐤌 (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } وإلا {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    فراغ emitTraceIndex(ثابت AstTraceInc* ثابت nodep, صحيح arrayindex) {
        // Tracing only supports 1D arrays
        𐤀𐤌 (nodep->declp()->arrayRange().ranged()) {
            𐤀𐤌 (arrayindex == -2) {
                puts("[i]");
            } وإلا 𐤀𐤌 (arrayindex == -1) {
                puts("[0]");
            } وإلا {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    فراغ visit(AstCFunc* nodep) override {
        𐤀𐤌 (!nodep->isTrace()) أرجع;
        𐤀𐤌 (nodep->slow() != m_slow) أرجع;

        𐤀𐤌 (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(true);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    فراغ visit(AstTracePushPrefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        𐤀𐤌 (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protectWordsIf(nodep->prefix(), nodep->protect()));
        } وإلا {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->left()));
        puts(", " + std::to_string(nodep->right()));
        puts(");\n");
    }
    فراغ visit(AstTracePopPrefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    فراغ visit(AstTraceDecl* nodep) override {
        ثابت صحيح enumNum = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        𐤀𐤌 (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enumNum);
            puts("\n}\n");
        } وإلا {
            emitTraceInitOne(nodep, enumNum);
            puts("\n");
        }
    }
    فراغ visit(AstTraceInc* nodep) override {
        𐤀𐤌 (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            لكل (صحيح i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } وإلا {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(منطقي slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->topModulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        لكل (AstNode* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            𐤀𐤌 (AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        𐤀𐤌 (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = افتراضي;

عام:
    ساكن std::vector<AstCFile*> main(منطقي slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        𐤀𐤌 (slow) {
            لكل (AstCFile* ثابت cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        أرجع cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

صنف EmitCFile final : عام EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = افتراضي;

عام:
    ساكن فراغ main(AstCFile* cfilep) VL_MT_STABLE {
        𐤀𐤌 (!cfilep->tblockp()) أرجع;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

فراغ V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        ثابت EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        لكل (ثابت AstNode* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            𐤀𐤌 (VN_IS(nodep, Class)) استمر;  // Imped with ClassPackage
            ثابت AstNodeModule* ثابت modp = VN_AS(nodep, NodeModule);
            𐤀𐤌 (modp->isConstPool()) استمر;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::main(modp, /* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::main(modp, /* slow: */ خطأ); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        𐤀𐤌 (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::main(/* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::main(/* slow: */ خطأ); });
        }
    }
    // Add files to netlist
    لكل (ثابت std::vector<AstCFile*>& cfileps : cfiles) {
        لكل (AstCFile* ثابت cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

فراغ V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (AstNodeFile *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), NodeFile);
        𐤀𐤌 (AstCFile* ثابت cfilep = VN_CAST(filep, CFile)) EmitCFile::main(cfilep);
    }
}
