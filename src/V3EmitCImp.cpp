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

#समावेश "V3EmitC.h"
#समावेश "V3EmitCFunc.h"
#समावेश "V3ThreadPool.h"
#समावेश "V3UniqueNames.h"

#समावेश <algorithm>
#समावेश <map>
#समावेश <set>
#समावेश <tuple>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Internal EmitC implementation

श्रेणी EmitCImp final : खुला EmitCFunc {
    // MEMBERS
    // Base module (For non-classes, same as m_modp. For classes, it's the ClassPackage.)
    स्थिर Ast_गाँठ_Module* स्थिर m_fileModp;
    स्थिर bool m_slow;  // Creating __Slow file
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    स्थिर std::string m_fileBaseName = EmitCUtil::prefixNameProtect(m_fileModp);
    TextStmts m_ctorStmts;  // Statements constructing members in 'ctor'
    TextStmts m_dtorStmts;  // Statements destroying members in 'dtor'
    std::string m_ctorSplitDecl;  // Declaration of sub-function 'ctor' was split into, if any
    std::string m_dtorSplitDecl;  // Declaration of sub-function 'dtor' was split into, if any

    // METHODS
    व्योम open_अग्र_Output_File(स्थिर std::string& fileName) {
        openNewOutputSourceFile(fileName, m_slow, false, "Design implementation internals");
        emitFileHeader();
    }
    व्योम emitFileHeader() {
        माला_दो("// See " + EmitCUtil::शीर्ष_Class_Name() + ".h for the primary calling header\n");
        माला_दो("\n");
        माला_दो("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        emitSystemCSection(m_fileModp, VSystemCSectionType::IMP_HDR);
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    व्योम emitParamDefns(स्थिर Ast_गाँठ_Module* modp) {
        bool first = true;
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                अगर (varp->isParam()) {
                    अगर (first) {
                        माला_दो("\n");
                        स्थिर string modName = EmitCUtil::prefixNameProtect(modp);
                        putsDecoration(modp, "// Parameter definitions for " + modName + "\n");
                        first = false;
                    }
                    अगर (varp->dtypep()->isLiteralType()) putns(varp, "VL_CONSTINIT_CXX20 ");
                    putns(varp, "const ");
                    emitVarDefn(varp);
                    माला_दो(";\n");
                }
            }
        }
        अगर (!first) माला_दो("\n");
    }
    // Add statements explicitly constructing, or destroying, a member AstVar
    अटल व्योम addVarCtorDtorStmts(TextStmts& stmts, स्थिर AstVar* varp, bool destroy) {
        स्थिर std::string name = "vlSelf->" + varName(varp);

        // SystemC port arrays are C arrays, so construct them element by element. They are
        // default constructed (unnamed), as the elements of an array member would be.
        अगर (varp->isSc() && VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            std::vector<std::string> elems{name};
            क्रम (स्थिर AstUnpackArrayDType* adtypep
                 = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                 adtypep; adtypep = VN_CAST(adtypep->subDTypep()->skipRefp(), UnpackArrayDType)) {
                std::vector<std::string> subElems;
                क्रम (स्थिर std::string& elem : elems) {
                    क्रम (पूर्णांक i = 0; i < adtypep->elementsConst(); ++i) {
                        subElems.धकेलो_back(elem + "[" + std::to_string(i) + "]");
                    }
                }
                elems = std::move(subElems);
            }
            अगर (destroy) std::reverse(elems.begin(), elems.end());
            क्रम (स्थिर std::string& e : elems) {
                stmts.add((destroy ? "VL_INPLACE_DESTRUCT(" : "VL_INPLACE_CONSTRUCT(") + e + ");");
            }
            वापस;
        }

        // Other variables are constructed whole, even if aggregates
        अगर (destroy) {
            stmts.add("VL_INPLACE_DESTRUCT(" + name + ");");
            वापस;
        }
        स्थिर AstBasicDType* स्थिर basicp = VN_CAST(varp->dtypeSkipRefp(), BasicDType);
        std::string args;
        अगर (varp->isSc()) {
            // SystemC ports need their name
            args = ", \"" + V3OutFormatter::quoteNameControls(varp->nameProtect()) + "\"";
        } अन्यथा अगर (basicp && basicp->keyword().isMTaskState()) {
            // MTask state needs the number of upstream dependencies to wait for
            args = ", " + VN_AS(varp->valuep(), Const)->अंक().emitC();
        } अन्यथा अगर (basicp && basicp->isDelayScheduler()) {
            // Delay scheduler needs the context pointer
            args = ", *vlSelf->vlSymsp->_vm_contextp__";
        }
        stmts.add("VL_INPLACE_CONSTRUCT(" + name + args + ");");
    }
    // Gather the statements constructing/destroying members in anonymous unions, and split
    // them into sub-functions if needed. As this emits files, no output file can be open.
    व्योम createCtorDtorStmts(स्थिर Ast_गाँठ_Module* modp) {
        // Construct member variables
        क्रम (स्थिर Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var);
            अगर (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_ctorStmts, varp, false);
            }
        }

        // Destroy in reverse order
        स्थिर Ast_गाँठ* स्थिर lastp = modp->stmtsp() ? modp->stmtsp()->lastp() : nullptr;
        क्रम (स्थिर Ast_गाँठ* nodep = lastp; nodep; nodep = nodep->prevp()) {
            स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var);
            अगर (varp && isExplicitlyConstructed(varp)) {
                addVarCtorDtorStmts(m_dtorStmts, varp, true);
            }
        }

        // Split into sub-functions if needed
        स्थिर std::string modName = EmitCUtil::prefixNameProtect(modp);
        std::tie(m_ctorSplitDecl, m_dtorSplitDecl) = emitSplitCtorDtor(
            m_ctorStmts, m_dtorStmts, 0, modName, modName + "* vlSelf", "vlSelf", false,
            "Design implementation internals", [यह]() { emitFileHeader(); });
    }
    व्योम emitCtorDtorImp(स्थिर Ast_गाँठ_Module* modp) {
        स्थिर std::string modName = EmitCUtil::prefixNameProtect(modp);

        // ctor
        अगर (modp->ctorVarReset()) {
            m_lazyDecls.emit("\nvoid " + modName + "__", protect("_ctor_var_reset"),
                             "(" + modName + "* vlSelf);\n");
        }

        अगर (!m_ctorSplitDecl.empty()) माला_दो("\n" + m_ctorSplitDecl + "\n");
        माला_दो("\nvoid " + modName + "__ctor(" + modName + "* vlSelf, " + EmitCUtil::symClassName()
             + "* symsp, const char* namep) {\n");
        माला_दो("vlSelf->vlSymsp = symsp;\n");
        अगर (modp->is_शीर्ष()) {
            माला_दो("vlSelf->vlNamep = strdup(namep);\n");
        } अन्यथा {
            माला_दो("vlSelf->vlNamep = strdup(Verilated::catName(symsp->name(), namep));\n");
        }
        putsDecoration(modp, "// Construct members\n");
        क्रम (स्थिर TextStmts::Stmt& stmt : m_ctorStmts) माला_दो(stmt.m_text + "\n");

        putsDecoration(modp, "// Reset structure values\n");
        अगर (modp->ctorVarReset()) {
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
        अगर (!m_dtorSplitDecl.empty()) माला_दो("\n" + m_dtorSplitDecl + "\n");
        माला_दो("\nvoid " + modName + "__dtor(" + modName + "* vlSelf) {\n");
        putsDecoration(modp, "// Destroy members\n");
        क्रम (स्थिर TextStmts::Stmt& stmt : m_dtorStmts) माला_दो(stmt.m_text + "\n");
        माला_दो("VL_DO_DANGLING(std::free(const_cast<char*>(vlSelf->vlNamep)), "
             "vlSelf->vlNamep);\n");
        माला_दो("}\n");

        // C++ destructor
        माला_दो("\n" + modName + "::~" + modName + "() {\n");
        emitSystemCSection(modp, VSystemCSectionType::DTOR);
        माला_दो(modName + "__dtor(this);\n");
        माला_दो("}\n");
    }

    व्योम emitConfigureImp(स्थिर Ast_गाँठ_Module* modp) {
        स्थिर string modName = EmitCUtil::prefixNameProtect(modp);

        अगर (v3Global.opt.coverage()) {
            माला_दो("\n");
            m_lazyDecls.emit("void " + modName + "__", protect("_configure_coverage"),
                             "(" + modName + "* vlSelf, bool first);");
        }

        अगर (v3Global.opt.coverage()) {
            माला_दो("\nvoid " + modName + "::" + protect("__Vconfigure") + "(bool first) {\n");
            माला_दो("(void)first;  // Prevent unused variable warning\n");
            माला_दो(modName + "__" + protect("_configure_coverage") + "(this, first);\n");
            माला_दो("}\n");
        }
    }
    व्योम emitCoverageImp() {
        // Rather than putting out VL_COVER_INSERT calls directly, we do it via this
        // function. This gets around gcc slowness constructing all of the template
        // arguments.
        अगर (v3Global.opt.coverage()) {
            माला_दो("\n// Coverage\n");
            माला_दो("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverInsert(");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp, const char* "
                 "linescovp,\n");
            माला_दो("const char* fsmVarp, const char* fsmFromp, const char* fsmTop, const char* "
                 "fsmTagp) {\n");
            अगर (v3Global.opt.threads() > 1) {
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
        अगर (v3Global.opt.coverageToggle()) {
            माला_दो("\n// Toggle Coverage\n");
            माला_दो("void " + EmitCUtil::prefixNameProtect(m_modp) + "::__vlCoverToggleInsert(");
            माला_दो("int begin, int end, bool ranged, ");
            माला_दो(v3Global.opt.threads() > 1 ? "std::atomic<uint32_t>" : "uint32_t");
            माला_दो("* countp, bool enable, bool localCounter, const char* filenamep, int lineno, "
                 "int column,\n");
            माला_दो("const char* hierp, const char* pagep, const char* commentp) {\n");
            अगर (v3Global.opt.threads() > 1) {
                माला_दो("assert(sizeof(uint32_t) == sizeof(std::atomic<uint32_t>));\n");
            }
            माला_दो("int step = (end >= begin) ? 1 : -1;\n");
            // range is inclusive
            माला_दो("for (int i = begin; i != end + step; i += step) {\n");
            माला_दो("for (int j = 0; j < 2; j++) {\n");
            अगर (v3Global.opt.threads() > 1) {
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
    व्योम emitSavableImp(स्थिर Ast_गाँठ_Module* modp) {
        अगर (v3Global.opt.savable()) {
            माला_दो("\n// Savable\n");
            क्रम (पूर्णांक de = 0; de < 2; ++de) {
                स्थिर string classname = de ? "VerilatedDeserialize" : "VerilatedSerialize";
                स्थिर string funcname = de ? "__Vdeserialize" : "__Vserialize";
                स्थिर string op = de ? ">>" : "<<";
                // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                putns(modp, "void " + EmitCUtil::prefixNameProtect(modp) + "::" + protect(funcname)
                                + "(" + classname + "& os) {\n");
                // Place a computed checksum to ensure proper structure save/restore formatting
                // OK if this hash includes some things we won't dump, since
                // just looking for loading the wrong model
                VHashSha512 hash;
                क्रम (Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                        hash.जोड़ो(varp->name());
                        hash.जोड़ो(varp->dtypep()->width());
                    }
                }
                ofp()->म_लिखो("uint64_t __Vcheckval = 0x%" PRIx64 "ULL;\n",
                              static_cast<uint64_t>(hash.digestUInt64()));
                अगर (de) {
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
                क्रम (Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                    अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                        अगर (varp->isIO() && modp->is_शीर्ष() && optSystemC()) {
                            // System C top I/O doesn't need loading, as the
                            // lower level subinst code does it.
                        } अन्यथा अगर (varp->isParam()) {
                        } अन्यथा अगर (VN_IS(varp->dtypep(), NBACommitQueueDType)) {
                        } अन्यथा {
                            पूर्णांक vects = 0;
                            Ast_गाँठ_DType* elementp = varp->dtypeSkipRefp();
                            क्रम (AstUnpackArrayDType* arrayp = VN_CAST(elementp, UnpackArrayDType);
                                 arrayp; arrayp = VN_CAST(elementp, UnpackArrayDType)) {
                                स्थिर पूर्णांक vecnum = vects++;
                                UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                            "Should have swapped msb & lsb earlier.");
                                स्थिर string ivar = "__Vi"s + cvtToStr(vecnum);
                                माला_दो("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                माला_दो("; " + ivar + " < " + cvtToStr(arrayp->elementsConst()));
                                माला_दो("; ++" + ivar + ") {\n");
                                elementp = arrayp->subDTypep()->skipRefp();
                            }
                            स्थिर AstBasicDType* स्थिर basicp = elementp->basicp();
                            // Do not save MTask state, only matters within an evaluation
                            अगर (basicp && basicp->keyword().isMTaskState()) जारी;
                            // Want to detect types that are represented as arrays
                            // (i.e. packed types of more than 64 bits).
                            अगर (elementp->isWide()
                                && !(basicp && basicp->keyword() == VBasicDTypeKwd::STRING)) {
                                स्थिर पूर्णांक vecnum = vects++;
                                स्थिर string ivar = "__Vi"s + cvtToStr(vecnum);
                                माला_दो("for (int __Vi" + cvtToStr(vecnum) + " = " + cvtToStr(0));
                                माला_दो("; " + ivar + " < " + cvtToStr(elementp->width_शब्दगण()));
                                माला_दो("; ++" + ivar + ") {\n");
                            }
                            putns(varp, "os" + op + varName(varp));
                            क्रम (पूर्णांक v = 0; v < vects; ++v) माला_दो("[__Vi" + cvtToStr(v) + "]");
                            माला_दो(";\n");
                            क्रम (पूर्णांक v = 0; v < vects; ++v) माला_दो("}\n");
                        }
                    }
                }

                माला_दो("}\n");
            }
        }
    }
    // Predicate to check if we actually need to emit anything into the common implementation file.
    // Used to avoid creating empty output files.
    bool hasCommonImp(स्थिर Ast_गाँठ_Module* modp) स्थिर {
        // Nothing to emit if no module!
        अगर (!modp) वापस false;
        // We always need the slow file
        अगर (m_slow) वापस true;
        // The fast file is only required when we have `systemc_implementation nodes
        क्रम (स्थिर AstSystemCSection* scSectionp = modp->scSectionsp(); scSectionp;
             scSectionp = VN_AS(scSectionp->nextp(), SystemCSection)) {
            अगर (scSectionp->sectionType() == VSystemCSectionType::IMP) वापस true;
        }
        वापस false;
    }
    // Actually emit common implementation contents for given AstNodeModule
    व्योम doCommonImp(स्थिर Ast_गाँठ_Module* modp) {
        अगर (m_slow) {
            अगर (!VN_IS(modp, Class)) {
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
    व्योम emitCommonImp(स्थिर Ast_गाँठ_Module* modp) {
        स्थिर AstClass* स्थिर classp
            = VN_IS(modp, ClassPackage) ? VN_AS(modp, ClassPackage)->classp() : nullptr;

        अगर (hasCommonImp(modp) || hasCommonImp(classp)) {
            // Create ctor/dtor statements. If these need splitting, they will be emitted
            // during construction so do this before opening the output file.
            अगर (m_slow && !VN_IS(modp, Class)) createCtorDtorStmts(modp);

            open_अग्र_Output_File(m_fileBaseName);

            doCommonImp(modp);
            अगर (classp) {
                VL_RESTORER(m_modp);
                m_modp = classp;
                doCommonImp(classp);
            }

            closeOutputFile();
        }
    }
    व्योम emitCFuncImp(स्थिर Ast_गाँठ_Module* modp) {
        // Functions to be emitted here
        std::vector<AstCFunc*> funcps;

        स्थिर स्वतः gather = [यह, &funcps](स्थिर Ast_गाँठ_Module* modp) {
            क्रम (Ast_गाँठ* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                AstCFunc* स्थिर funcp = VN_CAST(nodep, CFunc);
                अगर (!funcp) जारी;
                // TRACE_* and DPI handled elsewhere
                अगर (funcp->isTrace()) जारी;
                अगर (funcp->dpiImportPrototype()) जारी;
                अगर (funcp->dpiExportDispatcher()) जारी;
                अगर (funcp->slow() != m_slow) जारी;
                funcps.धकेलो_back(funcp);
            }
        };

        gather(modp);
        VL_RESTORER(m_classOrPackage);
        अगर (स्थिर AstClassPackage* स्थिर packagep = VN_CAST(modp, ClassPackage)) {
            m_classOrPackage = packagep;
            gather(packagep->classp());
        }

        // Do not create empty files
        अगर (funcps.empty()) वापस;

        // Open output file
        open_अग्र_Output_File(m_uniqueNames.लो(m_fileBaseName));
        // Emit all functions
        क्रम (AstCFunc* स्थिर funcp : funcps) {
            VL_RESTORER(m_modp);
            m_modp = EmitCParentModule::लो(funcp);
            iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
    }

    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        अगर (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(true);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_अग्र_Output_File(m_uniqueNames.लो(m_fileBaseName));
        }

        EmitCFunc::visit(nodep);
    }

    explicit EmitCImp(स्थिर Ast_गाँठ_Module* modp, bool slow)
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
    ~EmitCImp() override = शेष;

खुला:
    अटल std::vector<AstCFile*> मुख्य(स्थिर Ast_गाँठ_Module* modp, bool slow) VL_MT_STABLE {
        EmitCImp emitCImp{modp, slow};
        वापस emitCImp.getAndClearCfileps();
    }
};

//######################################################################
// Tracing routines

// Trace type descriptors go in a different file as it needs to be written in
// parallel with the actual trace function source files
श्रेणी EmitCTraceTypes final : खुला EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // STATE
    पूर्णांक m_enum_अंक = 0;  // Enumeration number (whole netlist)
    std::unordered_map<Ast_गाँठ*, पूर्णांक> m_enum_अंक_Map;  // EnumDType to enumeration number
    पूर्णांक m_traceTypeSubs = 0;  // Number of trace type declaration sub-functions
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    स्थिर std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "_" + protect("_TraceDecls");
    // This one uses CSplitTrace for file splitting, which is incorrect but historically accurates
    स्थिर माप_प्रकार m_splitLimit = v3Global.opt.outputSplitCTrace()
                                    ? static_cast<माप_प्रकार>(v3Global.opt.outputSplitCTrace())
                                    : std::numeric_limits<माप_प्रकार>::max();

    व्योम open_अग्र_Output_File() {
        openNewOutputSourceFile(m_uniqueNames.लो(m_fileBaseName), true, true,
                                "Tracing declarations");
        माला_दो("\n");
        क्रम (स्थिर std::string& base : v3Global.opt.traceSourceLangs()) {
            माला_दो("#include \"" + base + ".h\"\n");
        }
        माला_दो("\n");
        माला_दो("\nvoid " + EmitCUtil::prefixNameProtect(m_modp) + "__"
             + protect("traceDeclTypesSub" + std::to_string(m_traceTypeSubs++)) + "("
             + v3Global.opt.traceClassBase() + "* tracep) {\n");
    }

खुला:
    // METHODS
    पूर्णांक get_Enum_Map_अंक(AstEnumDType* nodep) {
        पूर्णांक& enumNumr = m_enum_अंक_Map[nodep];
        अगर (!enumNumr) {
            अगर (splitNeeded(m_splitLimit)) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(true);
                माला_दो("}\n");
                closeOutputFile();
                open_अग्र_Output_File();
            }

            enumNumr = ++m_enum_अंक;
            पूर्णांक nvals = 0;
            माला_दो("{\n");
            putns(nodep, "const char* " + protect("__VenumItemNames") + "[]\n");
            माला_दो("= {");
            क्रम (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                अगर (++nvals > 1) माला_दो(", ");
                putbs("\"" + itemp->prettyName() + "\"");
            }
            माला_दो("};\n");
            nvals = 0;
            माला_दो("const char* " + protect("__VenumItemValues") + "[]\n");
            माला_दो("= {");
            क्रम (AstEnumItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                AstConst* स्थिर constp = VN_AS(itemp->valuep(), Const);
                अगर (++nvals > 1) माला_दो(", ");
                putbs("\"" + constp->अंक().displayed(nodep, "%0b") + "\"");
            }
            माला_दो("};\n");
            माला_दो("tracep->declDTypeEnum(" + std::to_string(enumNumr) + ", \"" + nodep->prettyName()
                 + "\", " + std::to_string(nvals) + ", " + std::to_string(nodep->widthMin()) + ", "
                 + protect("__VenumItemNames") + ", " + protect("__VenumItemValues") + ");\n");
            माला_दो("}\n");
            split_आकार_Inc(Ast_गाँठ::INSTR_गिनती_CALL);
        }
        वापस enumNumr;
    }

    // Close output file
    व्योम finalize() {
        // Close function definition
        माला_दो("}\n");

        स्थिर std::string modName = EmitCUtil::prefixNameProtect(m_modp);
        स्थिर std::string args = v3Global.opt.traceClassBase() + "* tracep";

        // Forward declarations for subs in other files
        क्रम (पूर्णांक i = 0; i < m_traceTypeSubs - 1; ++i) {
            माला_दो("void " + modName + "__" + protect("traceDeclTypesSub" + std::to_string(i)) + "("
                 + args + ");\n");
        }

        // Create top level trace_decl_types function and call each sub-function
        माला_दो("\nvoid " + modName + "__" + protect("trace_decl_types") + "(" + args + ") {\n");
        क्रम (पूर्णांक i = 0; i < m_traceTypeSubs; ++i) {
            माला_दो(modName + "__" + protect("traceDeclTypesSub" + std::to_string(i))
                 + "(tracep);\n");
        }
        माला_दो("}\n");

        closeOutputFile();
    }

    EmitCTraceTypes() {
        m_modp = v3Global.rootp()->शीर्ष_Modulep();
        open_अग्र_Output_File();
    }
    ~EmitCTraceTypes() override = शेष;
};

श्रेणी EmitCTrace final : खुला EmitCFunc {
    // NODE STATE/TYPES
    // None allowed to support threaded emitting

    // MEMBERS
    स्थिर bool m_slow;  // Making slow file
    स्थिर std::unique_ptr<EmitCTraceTypes> m_emitTypesp{m_slow ? नया EmitCTraceTypes{} : nullptr};
    V3UniqueNames m_uniqueNames;  // Generates unique file names
    स्थिर std::string m_fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "_" + protect("_Trace");

    // METHODS
    व्योम open_अग्र_Output_File() {
        openNewOutputSourceFile(m_uniqueNames.लो(m_fileBaseName), m_slow, true,
                                "Tracing implementation internals");
        माला_दो("\n");
        क्रम (स्थिर std::string& base : v3Global.opt.traceSourceLangs()) {
            माला_दो("#include \"" + base + ".h\"\n");
        }
        माला_दो("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
        माला_दो("\n");
        // Need to emit new lazy declarations
        m_lazyDecls.reset();
    }

    bool emitTraceIsScBv(स्थिर AstTraceInc* nodep) {
        स्थिर AstVarRef* स्थिर varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        अगर (!varrefp) वापस false;
        स्थिर AstVar* स्थिर varp = varrefp->varp();
        वापस varp->isSc() && varp->isScBv();
    }

    bool emitTraceIsScBigUint(स्थिर AstTraceInc* nodep) {
        स्थिर AstVarRef* स्थिर varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        अगर (!varrefp) वापस false;
        स्थिर AstVar* स्थिर varp = varrefp->varp();
        वापस varp->isSc() && varp->isScBigUint();
    }

    bool emitTraceIsScUint(स्थिर AstTraceInc* nodep) {
        स्थिर AstVarRef* स्थिर varrefp = VN_CAST(nodep->declp()->valuep(), VarRef);
        अगर (!varrefp) वापस false;
        स्थिर AstVar* स्थिर varp = varrefp->varp();
        वापस varp->isSc() && (varp->isScUint() || varp->isScUintBool());
    }

    व्योम emitTraceInitOne(स्थिर AstTraceDecl* nodep, पूर्णांक enum_अंक) {
        std::string direction;
        direction = nodep->declDirection().traceSigDirection();

        AstCCall* स्थिर callp = nodep->dtypeCallp();
        अगर (callp) {
            callp->argTypes(callp->argTypes() + ", " + cvtToStr(nodep->fidx()) + ", c+"
                            + cvtToStr(nodep->code()) + ", " + direction);
            वापस;
        }

        अगर (nodep->dtypep()->basicp()->isDouble()) {
            माला_दो("VL_TRACE_DECL_DOUBLE");
        } अन्यथा अगर (nodep->isWide()) {
            माला_दो("VL_TRACE_DECL_WIDE");
        } अन्यथा अगर (nodep->isQuad()) {
            माला_दो("VL_TRACE_DECL_QUAD");
        } अन्यथा अगर (nodep->bitRange().ranged()) {
            माला_दो("VL_TRACE_DECL_BUS");
        } अन्यथा अगर (nodep->dtypep()->basicp()->isEvent()) {
            माला_दो("VL_TRACE_DECL_EVENT");
        } अन्यथा {
            माला_दो("VL_TRACE_DECL_BIT");
        }

        अगर (nodep->arrayRange().ranged()) {
            माला_दो("_ARRAY(tracep");
        } अन्यथा {
            माला_दो("(tracep");
        }

        // Code
        माला_दो(",c+" + cvtToStr(nodep->code()));
        अगर (nodep->arrayRange().ranged()) माला_दो("+i*" + cvtToStr(nodep->width_शब्दगण()));

        // Function index
        माला_दो(",");
        अगर (nodep->inDtypeFunc()) {
            माला_दो("fidx");
        } अन्यथा {
            माला_दो(cvtToStr(nodep->fidx()));
        }

        // Name
        माला_दो(",");
        putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->showname(), nodep->protect()));

        // Enum number
        माला_दो("," + cvtToStr(enum_अंक));

        // Direction
        अगर (nodep->inDtypeFunc()) {
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
        अगर (nodep->arrayRange().ranged()) {
            अगर (nodep->arrayRange().ascending()) {
                माला_दो(", (i + " + std::to_string(nodep->arrayRange().lo()) + ")");
            } अन्यथा {
                माला_दो(", (" + std::to_string(nodep->arrayRange().hi()) + " - i)");
            }
        }

        // Bit range
        अगर (!nodep->dtypep()->basicp()->isDouble() && nodep->bitRange().ranged()) {
            माला_दो(", " + cvtToStr(nodep->bitRange().बाँए()) + ","
                 + cvtToStr(nodep->bitRange().दाँए()));
        }

        //
        माला_दो(");");
    }

    पूर्णांक emitTraceDeclDType(Ast_गाँठ_DType* nodep) {
        // Return enum number or -1 for none
        अगर (v3Global.opt.traceEnabledFst()) {
            // Skip over refs-to-refs, but stop before final ref so can get data type name
            // Alternatively back in V3Width we could push enum names from upper typedefs
            अगर (AstEnumDType* स्थिर enump = VN_CAST(nodep->skipRefToEnump(), EnumDType)) {
                वापस m_emitTypesp->get_Enum_Map_अंक(enump);
            }
        }
        वापस -1;
    }

    व्योम emitTraceChangeOne(AstTraceInc* nodep, पूर्णांक arrayindex) {
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        स्थिर std::string func = nodep->traceType() == VTraceType::CHANGE ? "chg" : "full";
        bool emitWidth = true;
        string stype;
        अगर (nodep->dtypep()->basicp()->isDouble()) {
            stype = "Double";
            emitWidth = false;
        } अन्यथा अगर (nodep->isWide() || emitTraceIsScBv(nodep) || emitTraceIsScBigUint(nodep)) {
            stype = "WData";
        } अन्यथा अगर (nodep->isQuad()) {
            stype = "QData";
        } अन्यथा अगर (nodep->declp()->widthMin() > 16) {
            stype = "IData";
        } अन्यथा अगर (nodep->declp()->widthMin() > 8) {
            stype = "SData";
        } अन्यथा अगर (nodep->declp()->widthMin() > 1) {
            stype = "CData";
        } अन्यथा अगर (nodep->dtypep()->basicp()->isEvent()) {
            stype = "Event";
            emitWidth = false;
        } अन्यथा {
            stype = "Bit";
            emitWidth = false;
        }
        putns(nodep, "bufp->" + func + stype);

        स्थिर uint32_t offset = (arrayindex < 0) ? 0 : (arrayindex * nodep->declp()->width_शब्दगण());
        स्थिर uint32_t code = nodep->declp()->code() + offset;
        // Note: Both VTraceType::CHANGE and VTraceType::FULL use the 'full' methods
        माला_दो("(oldp+");
        माला_दो(cvtToStr(code - nodep->baseCode()));
        माला_दो(",");
        स्थिर VNumRange& arrayRange = nodep->declp()->arrayRange();
        अगर (arrayRange.ranged() && !arrayRange.ascending()) {
            arrayindex = arrayRange.elements() - 1 - arrayindex;
        }
        emit_Trace_मूल्य(nodep, arrayindex);
        अगर (emitWidth) माला_दो("," + cvtToStr(nodep->declp()->widthMin()));
        माला_दो(");\n");
    }

    व्योम emit_Trace_मूल्य(स्थिर AstTraceInc* nodep, पूर्णांक arrayindex) {
        अगर (AstVarRef* स्थिर varrefp = VN_CAST(nodep->valuep(), VarRef)) {
            स्थिर AstVar* स्थिर varp = varrefp->varp();
            अगर (varp->isEvent()) माला_दो("&");
            माला_दो("(");
            अगर (emitTraceIsScBigUint(nodep)) {
                माला_दो("(uint32_t*)");
            } अन्यथा अगर (emitTraceIsScBv(nodep)) {
                माला_दो("VL_SC_BV_DATAP(");
            }
            iterateConst(varrefp);  // Put var name out
            emit_Trace_सूचकांक(nodep, arrayindex);
            अगर (varp->isSc()) माला_दो(".read()");
            अगर (emitTraceIsScUint(nodep)) {
                माला_दो(nodep->isQuad() ? ".to_uint64()" : ".to_uint()");
            } अन्यथा अगर (emitTraceIsScBigUint(nodep)) {
                माला_दो(".get_raw()");
            } अन्यथा अगर (emitTraceIsScBv(nodep)) {
                माला_दो(")");
            }
            माला_दो(")");
        } अन्यथा {
            माला_दो("(");
            iterateConst(nodep->valuep());
            emit_Trace_सूचकांक(nodep, arrayindex);
            माला_दो(")");
        }
    }

    व्योम emit_Trace_सूचकांक(स्थिर AstTraceInc* स्थिर nodep, पूर्णांक arrayindex) {
        // Tracing only supports 1D arrays
        अगर (nodep->declp()->arrayRange().ranged()) {
            अगर (arrayindex == -2) {
                माला_दो("[i]");
            } अन्यथा अगर (arrayindex == -1) {
                माला_दो("[0]");
            } अन्यथा {
                माला_दो("[" + cvtToStr(arrayindex) + "]");
            }
        }
    }

    // VISITORS
    using EmitCFunc::visit;  // Suppress hidden overloaded virtual function warning
    व्योम visit(AstCFunc* nodep) override {
        अगर (!nodep->isTrace()) वापस;
        अगर (nodep->slow() != m_slow) वापस;

        अगर (splitNeeded()) {
            // Splitting file, so using parallel build.
            v3Global.useParallelBuild(true);
            // Close old file
            closeOutputFile();
            // Open a new file
            open_अग्र_Output_File();
        }

        EmitCFunc::visit(nodep);
    }
    व्योम visit(Ast_Trace_धकेलो_Prefix* nodep) override {
        putns(nodep, "VL_TRACE_PUSH_PREFIX(tracep, ");
        अगर (nodep->quotedPrefix()) {
            putsQuoted(VIdProtect::protect_शब्दगण_If(nodep->prefix(), nodep->protect()));
        } अन्यथा {
            माला_दो(nodep->prefix());
        }
        माला_दो(", VerilatedTracePrefixType::");
        माला_दो(nodep->prefixType().ascii());
        माला_दो(", " + std::to_string(nodep->बाँए()));
        माला_दो(", " + std::to_string(nodep->दाँए()));
        माला_दो(");\n");
    }
    व्योम visit(Ast_Trace_निकालो_Prefix* nodep) override {  //
        putns(nodep, "VL_TRACE_POP_PREFIX(tracep);\n");
    }
    व्योम visit(AstTraceDecl* nodep) override {
        स्थिर पूर्णांक enum_अंक = emitTraceDeclDType(nodep->dtypep());
        putns(nodep, "");
        अगर (nodep->arrayRange().ranged()) {
            माला_दो("for (int i = 0; i < " + cvtToStr(nodep->arrayRange().elements()) + "; ++i) {\n");
            emitTraceInitOne(nodep, enum_अंक);
            माला_दो("\n}\n");
        } अन्यथा {
            emitTraceInitOne(nodep, enum_अंक);
            माला_दो("\n");
        }
    }
    व्योम visit(AstTraceInc* nodep) override {
        अगर (nodep->declp()->arrayRange().ranged()) {
            // It traces faster if we unroll the loop
            क्रम (पूर्णांक i = 0; i < nodep->declp()->arrayRange().elements(); i++) {
                emitTraceChangeOne(nodep, i);
            }
        } अन्यथा {
            emitTraceChangeOne(nodep, -1);
        }
    }

    explicit EmitCTrace(bool slow)
        : m_slow{slow} {
        m_modp = v3Global.rootp()->शीर्ष_Modulep();
        // Open output file
        open_अग्र_Output_File();
        // Emit functions
        क्रम (Ast_गाँठ* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            अगर (AstCFunc* स्थिर funcp = VN_CAST(nodep, CFunc)) iterateConst(funcp);
        }
        // Close output file
        closeOutputFile();
        अगर (m_slow) m_emitTypesp->finalize();
    }
    ~EmitCTrace() override = शेष;

खुला:
    अटल std::vector<AstCFile*> मुख्य(bool slow) VL_MT_STABLE {
        EmitCTrace emitCTrace{slow};
        std::vector<AstCFile*> cfileps = emitCTrace.getAndClearCfileps();
        अगर (slow) {
            क्रम (AstCFile* स्थिर cfilep : emitCTrace.m_emitTypesp->getAndClearCfileps()) {
                cfileps.emplace_back(cfilep);
            }
        }
        वापस cfileps;
    }
};

//######################################################################
// Existing AstCFile emitter

श्रेणी EmitCFile final : खुला EmitCFunc {
    explicit EmitCFile(AstCFile* cfilep) {
        openOutputFile(cfilep, "Generated C++");
        iterateConst(cfilep->tblockp());
        closeOutputFile();
    }
    ~EmitCFile() override = शेष;

खुला:
    अटल व्योम मुख्य(AstCFile* cfilep) VL_MT_STABLE {
        अगर (!cfilep->tblockp()) वापस;
        EmitCFile{cfilep};
    }
};

//######################################################################
// EmitC class functions

व्योम V3EmitC::emitcImp() {
    UINFO(2, __FUNCTION__ << ":");
    std::list<std::vector<AstCFile*>> cfiles;
    {
        // Make parent module pointers available.
        स्थिर EmitCParentModule emitCParentModule;
        V3ThreadScope threadScope;

        // Process each module in turn
        क्रम (स्थिर Ast_गाँठ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
            अगर (VN_IS(nodep, Class)) जारी;  // Imped with ClassPackage
            स्थिर Ast_गाँठ_Module* स्थिर modp = VN_AS(nodep, गाँठ_Module);
            अगर (modp->isConstPool()) जारी;  // Emitted by V3EmitCConstPool
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([modp, &slow] { slow = EmitCImp::मुख्य(modp, /* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([modp, &fast] { fast = EmitCImp::मुख्य(modp, /* slow: */ false); });
        }

        // Emit trace routines (currently they can only exist in the top module)
        अगर (v3Global.opt.trace() && !v3Global.opt.lintOnly()) {
            cfiles.emplace_back();
            std::vector<AstCFile*>& slow = cfiles.back();
            threadScope.enqueue([&slow] { slow = EmitCTrace::मुख्य(/* slow: */ true); });
            cfiles.emplace_back();
            std::vector<AstCFile*>& fast = cfiles.back();
            threadScope.enqueue([&fast] { fast = EmitCTrace::मुख्य(/* slow: */ false); });
        }
    }
    // Add files to netlist
    क्रम (स्थिर std::vector<AstCFile*>& cfileps : cfiles) {
        क्रम (AstCFile* स्थिर cfilep : cfileps) v3Global.rootp()->addFilesp(cfilep);
    }
}

व्योम V3EmitC::emitcFiles() {
    UINFO(2, __FUNCTION__ << ":");
    क्रम (Ast_गाँठ_File *filep = v3Global.rootp()->filesp(), *nextp; filep; filep = nextp) {
        nextp = VN_AS(filep->nextp(), गाँठ_File);
        अगर (AstCFile* स्थिर cfilep = VN_CAST(filep, CFile)) EmitCFile::मुख्य(cfilep);
    }
}
