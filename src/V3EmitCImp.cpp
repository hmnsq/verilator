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

#અન્તર્ભાવ "V3EmitC.h"
#અન્તર્ભાવ "V3EmitCFunc.h"
#અન્તર્ભાવ "V3ThreadPool.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <tuple>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

વર્ગ EmitCImp final : જાહેર EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    અચલ Ast_ગાંઠ_Module* અચલ m_fileModp;
    અચલ તાર્કિક m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    અચલ std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    શૂન્ય openNextOutputFile(અચલ std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, ખોટું, "Design implementation internals");
        emitFileHeader();
    }
    શૂન્ય emitFileHeader() {
        puts("// See " + EmitCUtil::ટોચ_Class_Name() + ".h for the primary calling header\n");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    શૂન્ય emitParamDefns(અચલ Ast_ગાંઠ_Module* modp) {
        તાર્કિક first = સાચું;
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                જો (varp->isParam()) {
                    જો (first) {
                        puts("\n");
                        અચલ string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = ખોટું;
                    }
                    જો (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    puts(";\n");
                }
            }
        }
        જો (!first) puts("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    સ્થિર શૂન્ય addVarCtorDtorStmts(TextStmts& stmts, અચલ AstVar* varp, તાર્કિક destroy) {
        અચલ std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        જો (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            માટે (અચલ AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                માટે (અચલ std::string& elem : elems) {
                    માટે (પૂર્ણાંક i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.ધકેલો_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            જો (destroy) std::reverse(elems.begin(), elems.end());
            માટે (અચલ std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            પાછા;
        }

        // Other variables are constructed whole, even if aggregates
        જો (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            પાછા;
        }
        અચલ AstBasicDType* અચલ basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        જો (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } નહીંતર જો (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->અંક().emitC();
        } નહીંતર જો (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    શૂન્ય createCtorDtorStmts(અચલ Ast_ગાંઠ_Module* modp) {
        // Construct member variables
        માટે (અચલ Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            અચલ AstVar* અચલ varp = VN_CAST(nodep, Var);
            જો (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, ખોટું);
            }
        }

        // Destroy in reverse order
        અચલ Ast_ગાંઠ* અચલ lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        માટે (અચલ Ast_ગાંઠ* nodep = lastp; nodep; nodep = nodep->prevp()) {
            અચલ AstVar* અચલ varp = VN_CAST(nodep, Var);
            જો (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, સાચું);
            }
        }

        // Split into sub-functions if needed
        અચલ std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", ખોટું,
            "Design implementation internals", [આ]() { emitFileHeader(); });
    }
    શૂન્ય emitCtorDtorImp(અચલ Ast_ગાંઠ_Module* modp) {
        અચલ std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        જો (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        જો (!m_ctorSplitDecl.empty()) puts("\n" + m_ctorSplitDecl + "\n");
        puts("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        puts("vlSelf->vlSymsp = symsp;\n");
        જો (modp->is_ટોચ()) {
            puts("vlSelf->vlNamep = strdup(namep);\n");
        } નહીંતર {
            puts("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        માટે (અચલ TextStmts::Stmt& stmt : m_ctorStmts) puts(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        જો (modp->ctorVarReset()) {
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
        જો (!m_dtorSplitDecl.empty()) puts("\n" + m_dtorSplitDecl + "\n");
        puts("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        માટે (અચલ TextStmts::Stmt& stmt : m_dtorStmts) puts(stmt.m_text + "\n");
        puts("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        puts("}\n");

        // C++ destructor
        puts("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        puts(modName + "__dtor(this);\n");
        puts("}\n");
    }

    શૂન્ય emitConfigureImp(અચલ Ast_ગાંઠ_Module* modp) {
        અચલ string modName = EmitCUtil::prefixNameProtect(modp);

        જો (v3Global.opt.coverage()) {
            puts("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        જો (v3Global.opt.coverage()) {
            puts("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            puts("(void)first;  // Prevent unused variable warning\n");
            puts(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            puts("}\n");
        }
    }
    શૂન્ય emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        જો (v3Global.opt.coverage()) {
            puts("\n// Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            puts("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            જો (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } નહીંતર {
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
        જો (v3Global.opt.coverageToggle()) {
            puts("\n// Toggle Coverage\n");
            puts("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            puts("int begin, int end, bool ranged, ");
            puts(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            puts("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            puts("const char* hierp, const char* pagep, const char* commentp) {\n");
            જો (v3Global.opt.threads() > 1) {
                puts("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            puts("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            puts("for (int i = begin; i != end + step; i += step) {\n");
            puts("for (int j = 0; j < 2; j++) {\n");
            જો (v3Global.opt.threads() > 1) {
                puts("uint32_t* count32p = reinterpret_cast<uint32_t*>(countp);\n");
            } નહીંતર {
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
    શૂન્ય emitSavableImp(અચલ Ast_ગાંઠ_Module* modp) {
        જો (v3Global.opt.savable()) {
            puts("\n// Savable\n");
            માટે (પૂર્ણાંક de = 0; de < 2; ++de) {
                અચલ string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                અચલ string funcname = de ? "__Vdeserialize" : "__Vserialize";
                અચલ string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                માટે (Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                        hash.ઉમેરો(varp->name());
                        hash.ઉમેરો(varp->dtypep()->width());
                    }
                }
                ofp()->printf("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                જો (de) {
                    puts("os.readAssert(__Vcheckval);\n");
                } નહીંતર {
                    puts("os << __Vcheckval;\n");
                }

                // Save context
                // If multiple models save the same context we'll save it multiple
                // times, but is harmless, and doing it otherwise would break
                // backwards compatibility.
                puts("os " + op + " vlSymsp->_vm_contextp__;\n");

                // Save all members
                માટે (Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                        જો (varp->isIO() && modp->is_ટોચ() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } નહીંતર જો (varp->isParam()) {
                        } નહીંતર જો (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } નહીંતર {
                            પૂર્ણાંક vects = 0;
                            Ast_ગાંઠ_DType* elementp = varp->dtypeSkipRefp();
                            માટે (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                અચલ પૂર્ણાંક vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                અચલ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                puts("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            અચલ AstBasicDType* અચલ basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            જો (basicp && basicp->keyword().isMTaskState()) ચાલુ;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            જો (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                અચલ પૂર્ણાંક vecnum = vects++;
                                અચલ string ivar = "__Vi"s + cvtToStr(vecnum);
                                puts("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                puts("; " + ivar + " < " + cvtToStr(elementp->width_શબ્દો()));
                                puts("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            માટે (પૂર્ણાંક v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                            puts(";\n");
                            માટે (પૂર્ણાંક v = 0; v < vects; ++v) puts("}\n");
                        }
                    }
                }

                puts("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    તાર્કિક hasCommonImp(અચલ Ast_ગાંઠ_Module* modp) અચલ {
        // Nothing to emit if no module!
        જો (!modp) પાછા ખોટું;
        // We always need the slow file
        જો (m_slow) પાછા સાચું;
        // The fast file is only required when we have `systemc_implementation nodes
        માટે (અચલ AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            જો (scSectionp->sectionType() == VSystemCSectionType::IMP) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    // Actually emit common implementation contents for given AstNodeModule
    શૂન્ય doCommonImp(અચલ Ast_ગાંઠ_Module* modp) {
        જો (m_slow) {
            જો (!VN_IS(modp, Class)) {
                emitParamDefns(modp);
                emitCtorDtorImp(modp);
                emitConfigureImp(modp);
                emitCoverageImp();
            }
            emitSavableImp(modp);
        } નહીંતર {
            // From `systemc_implementation
            emitSystemCSection(modp, VSystemCSectionType::IMP);
        }
    }
    શૂન્ય emitCommonImp(અચલ Ast_ગાંઠ_Module* modp) {
        અચલ AstClass* અચલ classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        જો (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            જો (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            openNextOutputFile(m_fileBaseName);

            doCommonImp(modp);
            જો (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    શૂન્ય emitCFuncImp(અચલ Ast_ગાંઠ_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        અચલ auto gather = [આ, &funcps](અચલ Ast_ગાંઠ_Module* modp) {
            માટે (Ast_ગાંઠ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* અચલ funcp = VN_CAST(nodep, CFunc);
                જો (!funcp) ચાલુ;
                // TRACE_* and DPI handled elsewhere
                જો (funcp->isTrace()) ચાલુ;
                જો (funcp->dpiImportPrototype()) ચાલુ;
                જો (funcp->dpiExportDispatcher()) ચાલુ;
                જો (funcp->slow() != m_slow) ચાલુ;
                funcps.ધકેલો_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        જો (અચલ AstClassPackage* અચલ packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        જો (funcps.empty()) પાછા;

        // Open output file
        openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        // Emit all functions
        માટે (AstCFunc* અચલ funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::get(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    શૂન્ય visit(AstCFunc* nodep) override {
        જો (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(સાચું);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile(m_uniqueNames.get(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(અચલ Ast_ગાંઠ_Module* modp, તાર્કિક slow)
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
    ~EmitCImp() override = મૂળભૂત;

જાહેર:
    સ્થિર std::vector<AstCFile*> મુખ્ય(અચલ Ast_ગાંઠ_Module* modp, તાર્કિક slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        પાછા emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
વર્ગ EmitCTraceTypes final : જાહેર EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    પૂર્ણાંક m_enum_અંક = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_ગાંઠ*, પૂર્ણાંક> m_enum_અંક_Map;  // EnumDType to enumeration number
    પૂર્ણાંક m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    અચલ std::string m_fileBaseName = EmitCUtil::ટોચ_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    અચલ size_t m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<size_t>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<size_t>::max();

    શૂન્ય openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), સાચું, સાચું,
                                "Tracing declarations");
        puts("\n");
        માટે (અચલ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("\n");
        puts("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

જાહેર:
    // METHODS
    પૂર્ણાંક get_Enum_Map_અંક(AstEnumDType* nodep) {
        પૂર્ણાંક& enumNumr = m_enum_અંક_Map[nodep];
        જો (!enumNumr) {
            જો (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(સાચું);
                puts("}\n");
                closeOutputFile();
                openNextOutputFile();
            }

            enumNumr = ++m_enum_અંક;
            પૂર્ણાંક nvals = 0;
            puts("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            puts("= {");
            માટે (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                જો (++nvals > 1) puts(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            puts("};\n");
            nvals = 0;
            puts("const char* " + protect("__VenumItemValues") + "[]\n");
            puts("= {");
            માટે (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* અચલ constp = VN_AS(itemp->valuep(), Const);
                જો (++nvals > 1) puts(", ");
                putbs("\"" + constp->અંક().displayed(nodep, "%0b") + "\"");
            }
            puts("};\n");
            puts("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            puts("}\n");
            splitSizeInc(Ast_ગાંઠ::INSTR_ગણતરી_CALL);
        }
        પાછા enumNumr;
    }

    // Close output file
    શૂન્ય finalize() {
        // Close function definition
        puts("}\n");

        અચલ std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        અચલ std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        માટે (પૂર્ણાંક i = 0; i < m_traceTypeSubs - 1; ++i) {
            puts("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        puts("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        માટે (પૂર્ણાંક i = 0; i < m_traceTypeSubs; ++i) {
            puts(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        puts("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->ટોચ_Modulep();
        openNextOutputFile();
    }
    ~EmitCTraceTypes() override = મૂળભૂત;
};

વર્ગ EmitCTrace final : જાહેર EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    અચલ તાર્કિક m_slow;  // Making slow file
    અચલ std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? નવું EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    અચલ std::string m_fileBaseName = EmitCUtil::ટોચ_Class_Name() + "_" + protect("_Trace");

    // METHODS
    શૂન્ય openNextOutputFile() {
        openNewOutputSourceFile(m_uniqueNames.get(m_fileBaseName), m_slow, સાચું,
                                "Tracing implementation internals");
        puts("\n");
        માટે (અચલ std::string& base : v3Global.opt.traceSourceLangs()) {
            puts("#include \"" + base + ".h\"\n");
        }
        puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        puts("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    તાર્કિક emitTraceIsScBv(અચલ AstTraceInc* nodep) {
        અચલ AstVarRef* અચલ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        જો (!varrefp) પાછા ખોટું;
        અચલ AstVar* અચલ varp = varrefp->varp();
        પાછા varp->isSc() && varp->isScBv();
    }

    તાર્કિક emitTraceIsScBigUint(અચલ AstTraceInc* nodep) {
        અચલ AstVarRef* અચલ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        જો (!varrefp) પાછા ખોટું;
        અચલ AstVar* અચલ varp = varrefp->varp();
        પાછા varp->isSc() && varp->isScBigUint();
    }

    તાર્કિક emitTraceIsScUint(અચલ AstTraceInc* nodep) {
        અચલ AstVarRef* અચલ varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        જો (!varrefp) પાછા ખોટું;
        અચલ AstVar* અચલ varp = varrefp->varp();
        પાછા varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    શૂન્ય emitTraceInitOne(અચલ AstTraceDecl* nodep, પૂર્ણાંક enum_અંક) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* અચલ callp = nodep->dtypeCallp();
        જો (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            પાછા;
        }

        જો (nodep->dtypep()->basicp()->isDouble()) {
            puts("VL_TRACE_DECL_DOUBLE");
        } નહીંતર જો (nodep->isWide()) {
            puts("VL_TRACE_DECL_WIDE");
        } નહીંતર જો (nodep->isQuad()) {
            puts("VL_TRACE_DECL_QUAD");
        } નહીંતર જો (nodep->bitRange().ranged()) {
            puts("VL_TRACE_DECL_BUS");
        } નહીંતર જો (nodep->dtypep()->basicp()->isEvent()) {
            puts("VL_TRACE_DECL_EVENT");
        } નહીંતર {
            puts("VL_TRACE_DECL_BIT");
        }

        જો (nodep->arrayRange().ranged()) {
            puts("_ARRAY(tracep");
        } નહીંતર {
            puts("(tracep");
        }

        // Code
        puts(",c+" + cvtToStr(nodep->code()));
        જો (nodep->arrayRange().ranged()) puts("+i*" + cvtToStr(nodep->width_શબ્દો()));

        // Function index
        puts(",");
        જો (nodep->inDtypeFunc()) {
            puts("fidx");
        } નહીંતર {
            puts(cvtToStr(nodep->fidx()));
        }

        // Name
        puts(",");
        putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->showname(), nodep->protect()));

        // Enum number
        puts("," + cvtToStr(enum_અંક));

        // Direction
        જો (nodep->inDtypeFunc()) {
            puts(", direction");
        } નહીંતર {
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
        જો (nodep->arrayRange().ranged()) {
            જો (nodep->arrayRange().ascending()) {
                puts(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } નહીંતર {
                puts(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        જો (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            puts(", " + cvtToStr(nodep->bitRange().ડાબું()) + ","
                 + cvtToStr(nodep->bitRange().જમણું()));
        }

        //
        puts(");");
    }

    પૂર્ણાંક emitTraceDeclDType(Ast_ગાંઠ_DType* nodep) {
        // Return enum number or -1 for none
        જો (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            જો (AstEnumDType* અચલ enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                પાછા m_emitTypesp->get_Enum_Map_અંક(enump);
            }
        }
        પાછા -1;
    }

    શૂન્ય emitTraceChangeOne(AstTraceInc* nodep, પૂર્ણાંક arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        અચલ std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        તાર્કિક emitWidth = સાચું;
        string stype;
        જો (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = ખોટું;
        } નહીંતર જો (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } નહીંતર જો (nodep->isQuad()) {
            stype = "QData";
        } નહીંતર જો (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } નહીંતર જો (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } નહીંતર જો (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } નહીંતર જો (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = ખોટું;
        } નહીંતર {
            stype = "Bit";
            emitWidth = ખોટું;
        }
        putns(nodep, "bufp->" + func + stype);

        અચલ uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_શબ્દો());
        અચલ uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        puts("(oldp+");
        puts(cvtToStr(code - nodep->baseCode()));
        puts(",");
        અચલ VNumRange& arrayRange = nodep->declp()->arrayRange();
        જો (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_કિંમત(nodep, arrayindex);
        જો (emitWidth) puts("," + cvtToStr(nodep->declp()->widthMin()));
        puts(");\n");
    }

    શૂન્ય emit_Trace_કિંમત(અચલ AstTraceInc* nodep, પૂર્ણાંક arrayindex) {
        જો (AstVarRef* અચલ varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            અચલ AstVar* અચલ varp = varrefp->varp();
            જો (varp->isEvent()) puts("&");
            puts("(");
            જો (emitTraceIsScBigUint(nodep)) {
                puts("(uint32_t*)");
            } નહીંતર જો (emitTraceIsScBv(nodep)) {
                puts("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emitTraceIndex(nodep, arrayindex);
            જો (varp->isSc()) puts(".read()");
            જો (emitTraceIsScUint(nodep)) {
                puts(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } નહીંતર જો (emitTraceIsScBigUint(nodep)) {
                puts(".get_raw()");
            } નહીંતર જો (emitTraceIsScBv(nodep)) {
                puts(")");
            }
            puts(")");
        } નહીંતર {
            puts("(");
            iterateConst(nodep->valuep());
            emitTraceIndex(nodep, arrayindex);
            puts(")");
        }
    }

    શૂન્ય emitTraceIndex(અચલ AstTraceInc* અચલ nodep, પૂર્ણાંક arrayindex) {
        // Tracing only supports 1D arrays
        જો (nodep->declp()->arrayRange().ranged()) {
            જો (arrayindex == -2) {
                puts("[i]");
            } નહીંતર જો (arrayindex == -1) {
                puts("[0]");
            } નહીંતર {
                puts("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    શૂન્ય visit(AstCFunc* nodep) override {
        જો (!nodep->isTrace()) પાછા;
        જો (nodep->slow() != m_slow) પાછા;

        જો (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(સાચું);
            // Close old file
            closeOutputFile();
            // Open a new file
            openNextOutputFile();
        }

        EmitCFunc::visit(nodep);
    }
    શૂન્ય visit(Ast_Trace_ધકેલો_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        જો (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_શબ્દો_If(nodep->prefix(), nodep->protect()));
        } નહીંતર {
            puts(nodep->prefix());
        }
        puts(", VerilatedTracePrefixType::");
        puts(nodep->prefixType().ascii());
        puts(", " + std::to_string(nodep->ડાબું()));
        puts(", " + std::to_string(nodep->જમણું()));
        puts(");\n");
    }
    શૂન્ય visit(Ast_Trace_કાઢો_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    શૂન્ય visit(AstTraceDecl* nodep) override {
        અચલ પૂર્ણાંક enum_અંક = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        જો (nodep->arrayRange().ranged()) {
            puts("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_અંક);
            puts("\n}\n");
        } નહીંતર {
            emitTraceInitOne(nodep, enum_અંક);
            puts("\n");
        }
    }
    શૂન્ય visit(AstTraceInc* nodep) override {
        જો (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            માટે (પૂર્ણાંક i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } નહીંતર {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(તાર્કિક slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->ટોચ_Modulep();
        // Open output file
        openNextOutputFile();
        // Emit functions
        માટે (Ast_ગાંઠ* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            જો (AstCFunc* અચલ funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        જો (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = મૂળભૂત;

જાહેર:
    સ્થિર std::vector<AstCFile*> મુખ્ય(તાર્કિક slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        જો (slow) {
            માટે (AstCFile* અચલ cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        પાછા cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

વર્ગ EmitCFile final : જાહેર EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = મૂળભૂત;

જાહેર:
    સ્થિર શૂન્ય મુખ્ય(AstCFile* cfilep) VL_MT_STABLE {
        જો (!cfilep->tblockp()) પાછા;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

શૂન્ય V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        અચલ EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        માટે (અચલ Ast_ગાંઠ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            જો (VN_IS(nodep, Class)) ચાલુ;  // Imped with ClassPackage
            અચલ Ast_ગાંઠ_Module* અચલ modp = VN_AS(nodep, ગાંઠ_Module);
            જો (modp->isConstPool()) ચાલુ;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::મુખ્ય(modp, /* slow: */ સાચું); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::મુખ્ય(modp, /* slow: */ ખોટું); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        જો (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::મુખ્ય(/* slow: */ સાચું); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::મુખ્ય(/* slow: */ ખોટું); });
        }
    }
    // Add files to netlist
    માટે (અચલ std::vector<AstCFile*>& cfileps : cfiles) {
        માટે (AstCFile* અચલ cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

શૂન્ય V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    માટે (Ast_ગાંઠ_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), ગાંઠ_File);
        જો (AstCFile* અચલ cfilep = VN_CAST(filep, CFile)) EmitCFile::મુખ્ય(cfilep);
    }
}
